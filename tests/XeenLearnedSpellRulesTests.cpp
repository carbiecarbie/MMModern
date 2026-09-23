#include "games/xeen/XeenLearnedSpellRules.h"
#include "games/xeen/XeenCharacterRules.h"
#include "games/xeen/XeenRestoreGuard.h"
#include <array>
#include <iostream>
#include <memory>
#include <stdexcept>

using namespace mmodern;
namespace {
void check(bool value,const char *message) {if(!value)throw std::runtime_error(message);}
template<class F> void rejects(F operation) {bool rejected=false;try{operation();}catch(const std::exception &){rejected=true;}check(rejected,"invalid spell preparation accepted");}
XeenPartyState fixture() {
	XeenPartyState party;
	party.party=XeenParty::fromRosterIds({1,6,14,1});
	for(unsigned owner=0;owner<30;++owner) {
		auto &c=party.roster.at(owner);c.rosterId=owner;
		c.characterClass=XeenCharacterClass::Cleric;c.race=XeenRace::Human;
		c.permanentLevel=3;c.endurance.permanent=18;c.birthYear=592;
		c.currentHp=12;c.currentSp=1;c.hasSpells=true;
		c.learnedSpells=XeenCharacter::XeenLearnedSpells{};
	}
	party.roster.at(1).learnedSpells->at(14)=2;
	party.roster.at(6).characterClass=XeenCharacterClass::Sorcerer;
	party.roster.at(6).learnedSpells->at(0)=255;
	party.roster.at(14).characterClass=XeenCharacterClass::Ranger;
	party.roster.at(14).learnedSpells->at(1)=1;
	return party;
}
void mapping() {
	using C=XeenCharacterClass;using S=XeenSpellCategory;
	constexpr std::array<std::array<std::uint8_t,39>,3> expected{{
		{{0,1,2,3,5,6,7,8,9,10,12,14,16,23,26,27,28,30,31,32,33,42,46,48,49,50,52,55,56,58,59,62,64,65,67,68,71,73,74}},
		{{1,4,11,13,15,17,18,19,20,21,22,24,25,29,34,35,36,37,38,39,40,41,42,43,44,45,47,51,53,54,57,60,61,63,66,69,70,72,75}},
		{{0,1,2,3,4,5,7,9,10,20,25,26,27,28,30,31,34,38,40,41,42,43,44,45,49,50,52,53,55,59,60,61,62,67,68,72,73,74,75}}
	}};
	for(auto value:{C::Paladin,C::Cleric})check(XeenLearnedSpellRules::categoryForClass(value)==S::Clerical,"clerical category");
	for(auto value:{C::Archer,C::Sorcerer})check(XeenLearnedSpellRules::categoryForClass(value)==S::Wizardry,"wizardry category");
	for(auto value:{C::Druid,C::Ranger})check(XeenLearnedSpellRules::categoryForClass(value)==S::Druidic,"druidic category");
	for(auto value:{C::Knight,C::Robber,C::Ninja,C::Barbarian,static_cast<C>(255)})
		check(!XeenLearnedSpellRules::categoryForClass(value),"noncaster category");
	for(unsigned category=0;category<3;++category) {
		std::array<bool,76> seen{};
		for(unsigned slot=0;slot<39;++slot) {
			const auto id=XeenLearnedSpellRules::spellForSlot(static_cast<S>(category),slot);
			check(id && *id==expected[category][slot] && *id<76 && !seen[*id],
				"literal bounded category mapping");seen[*id]=true;
		}
		check(!XeenLearnedSpellRules::spellForSlot(static_cast<S>(category),39),"sentinel rejected");
	}
	check(!XeenLearnedSpellRules::spellForSlot(static_cast<S>(3),0),"invalid category rejected");
	check(XeenLearnedSpellRules::spellForSlot(S::Clerical,14)==26 &&
		XeenLearnedSpellRules::spellForSlot(S::Druidic,11)==26 &&
		XeenLearnedSpellRules::spellForSlot(S::Wizardry,0)==1,"selected global IDs");
	check(!XeenLearnedSpellRules::supported(42) && !XeenLearnedSpellRules::supported(76),"unsupported identity rejected");
	rejects([&]{(void)XeenLearnedSpellNames::parse(std::vector<std::uint8_t>{});});
}
void eligibilityAndEffects() {
	auto p=fixture();
	check(XeenLearnedSpellRules::eligible(p,0,14) && XeenLearnedSpellRules::eligible(p,1,0) &&
		XeenLearnedSpellRules::eligible(p,2,1) && XeenLearnedSpellRules::eligible(p,3,14),"raw nonzero knowledge and aliases");
	check(!XeenLearnedSpellRules::eligible(p,0,1) && !XeenLearnedSpellRules::eligible(p,1,14) &&
		!XeenLearnedSpellRules::eligible(p,4,14) && !XeenLearnedSpellRules::eligible(p,0,39),"unknown/invalid support refusal");
	auto &caster=p.roster.at(1);
	caster.currentSp=0;check(!XeenLearnedSpellRules::eligible(p,0,14),"zero SP refusal");
	caster.currentSp=-1;check(!XeenLearnedSpellRules::eligible(p,0,14),"minus-one SP refusal");
	caster.currentSp=-32768;check(!XeenLearnedSpellRules::eligible(p,0,14),"negative SP refusal");
	caster.currentSp=32767;check(XeenLearnedSpellRules::eligible(p,0,14),"above-maximum SP admitted");
	caster.currentSp=1;
	for(unsigned condition:{8u,11u,12u,13u,14u,15u}) {
		caster.conditions.fill(0);caster.conditions[condition]=1;
		check(!XeenLearnedSpellRules::eligible(p,0,14),"disabling worst condition refusal");
	}
	caster.conditions.fill(0);caster.conditions[3]=1;caster.conditions[8]=1;
	check(!XeenLearnedSpellRules::eligible(p,0,14),"simultaneous condition precedence");
	caster.conditions.fill(0);caster.permanentLevel=99;
	check(XeenLearnedSpellRules::eligible(p,0,14),"level does not scale cost");
	caster.permanentLevel=3;caster.hasSpells=false;check(!XeenLearnedSpellRules::eligible(p,0,14),"capability independent of knowledge");
	caster.hasSpells=true;caster.learnedSpells.reset();check(!XeenLearnedSpellRules::eligible(p,0,14),"absent book");
	caster.learnedSpells=XeenCharacter::XeenLearnedSpells{};check(!XeenLearnedSpellRules::eligible(p,0,14),"present empty book");
	caster.learnedSpells->at(14)=2;
	const auto maximum=XeenCharacterRules::maxHp(caster,{610});
	caster.currentHp=maximum-4;caster.conditions[12]=1;
	auto first=XeenLearnedSpellRules::prepareFirstAid(p,0,610);
	check(!first.failed && first.effects.size()==1 && first.effects[0].owner==1 &&
		first.effects[0].hp==maximum && !first.effects[0].conditions[12],"bounded self heal and Unconscious clearing");
	caster.currentHp=maximum+2;first=XeenLearnedSpellRules::prepareFirstAid(p,3,610);
	check(first.effects.size()==1 && first.effects[0].owner==1 && first.effects[0].hp==maximum+2,
		"alias self target preserves surplus");
	caster.conditions[13]=1;first=XeenLearnedSpellRules::prepareFirstAid(p,0,610);
	check(first.failed && first.effects.empty(),"terminal First Aid failure");
	caster.conditions[13]=0;caster.currentHp=-10;first=XeenLearnedSpellRules::prepareFirstAid(p,0,610);
	check(first.effects[0].hp==-4 && first.effects[0].conditions[12]==1,"negative HP remains Unconscious");
	caster.currentHp=0;first=XeenLearnedSpellRules::prepareFirstAid(p,0,610);
	check(first.effects[0].hp==6 && !first.effects[0].conditions[12],"zero HP can recover");
	caster.currentHp=maximum;first=XeenLearnedSpellRules::prepareFirstAid(p,0,610);
	check(first.effects[0].hp==maximum,"healthy First Aid still prepares effect");
	caster.conditions[8]=1;first=XeenLearnedSpellRules::prepareFirstAid(p,0,610);
	check(first.effects[0].conditions[8]==1,"First Aid retains Sleep");caster.conditions[8]=0;
	for(unsigned condition:{13u,14u,15u}) {
		caster.conditions[condition]=1;first=XeenLearnedSpellRules::prepareFirstAid(p,0,610);
		check(first.failed && first.effects.empty(),"each terminal target fails after cost");
		caster.conditions[condition]=0;
	}
	p.roster.at(6).conditions[8]=1;p.roster.at(6).conditions[3]=2;
	p.roster.at(14).conditions[8]=1;p.roster.at(14).conditions[13]=1;p.roster.at(14).conditions[12]=1;
	const auto awaken=XeenLearnedSpellRules::prepareAwaken(p);
	check(awaken.effects.size()==3 && awaken.effects[0].owner==1 && awaken.effects[1].owner==6 &&
		awaken.effects[2].owner==14,"Awaken active owner order and alias deduplication");
	check(!awaken.effects[1].conditions[8] && awaken.effects[1].conditions[3]==2 &&
		!awaken.effects[2].conditions[8] && !awaken.effects[2].conditions[12] &&
		awaken.effects[2].conditions[13]==1,"Awaken exact terminal/other-condition behavior");
	p.roster.at(2).conditions[8]=1;
	check(awaken.effects.size()==3 && p.roster.at(2).conditions[8]==1,"inactive owner unaffected");
	rejects([&]{(void)XeenLearnedSpellRules::prepareFirstAid(p,4,610);});
}
void resourcePreimage() {
	XeenWorld world([](XeenMapIdentity){return XeenMap{};});
	auto party=fixture();XeenCamera camera;XeenGameFlags flags;
	for(unsigned owner=0;owner<30;++owner) {
		XeenRestoreGuard ownerGuard(world,party,camera,flags);
		auto &book=*party.roster.at(owner).learnedSpells;
		const auto original=book[0];book[0]^=255;
		check(!ownerGuard.current(),"all thirty learned owners are covered by preimage");
		rejects([&]{ownerGuard.check();});
		book[0]=original;
		check(!ownerGuard.current(),"learned owner mutation and reversion cannot reopen preimage");
	}
	{
		XeenRestoreGuard presence(world,party,camera,flags);
		party.roster.at(29).learnedSpells.reset();
		check(!presence.current(),"absent and present-empty knowledge are distinct");
		party.roster.at(29).learnedSpells=XeenCharacter::XeenLearnedSpells{};
	}
	XeenRestoreGuard guard(world,party,camera,flags);
	XeenLearnedSpellNames original;original.names[1]="Awaken";
	guard.admitLearnedSpellNames(original);
	guard.verifyLearnedSpellNames(original);
	auto changed=original;changed.names[1]="Changed";
	rejects([&]{guard.verifyLearnedSpellNames(changed);});
	check(!guard.current(),"changed name preimage is monotonic");
	rejects([&]{guard.admitLearnedSpellNames(original);});
}
void ownerIdentityAndLifetime() {
	XeenWorld world([](XeenMapIdentity){return XeenMap{};});
	XeenCamera camera;XeenGameFlags flags;
	{
		auto party=fixture();
		XeenRestoreGuard guard(world,party,camera,flags);
		const auto same=party;
		party=same;
		check(!guard.current(),"equal-value party replacement retained obsolete authority");
	}
	{
		auto party=fixture();
		XeenRestoreGuard guard(world,party,camera,flags);
		const auto same=party.roster;
		party.roster=same;
		check(!guard.current(),"equal-value roster replacement retained obsolete authority");
	}
	{
		auto party=std::make_unique<XeenPartyState>(fixture());
		auto guard=std::make_unique<XeenRestoreGuard>(world,*party,camera,flags);
		party.reset();
		check(!guard->ownersAlive() && !guard->current(),"destroyed party retained casting preimage");
		rejects([&]{guard->check();});
	}
	{
		auto party=fixture();
		auto cameraOwner=std::make_unique<XeenCamera>();
		auto guard=std::make_unique<XeenRestoreGuard>(world,party,*cameraOwner,flags);
		cameraOwner.reset();
		check(!guard->ownersAlive() && !guard->current(),"destroyed camera retained casting preimage");
	}
}
}
int main() {try {mapping();eligibilityAndEffects();resourcePreimage();ownerIdentityAndLifetime();std::cout<<"Learned spell mapping and pure effects passed\n";return 0;}
catch(const std::exception &e){std::cerr<<e.what()<<'\n';return 1;}}
