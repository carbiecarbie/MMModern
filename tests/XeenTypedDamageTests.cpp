#include "games/xeen/XeenCombatRules.h"
#include "formats/xeen/XeenCharacterFormat.h"
#include <iostream>
#include <limits>
#include <stdexcept>

using namespace mmodern;
namespace {
void check(bool value,const char *message) {if (!value) throw std::runtime_error(message);}
template<class F> void rejects(F f) {
	bool rejected=false;try {f();} catch (const std::exception &) {rejected=true;}
	check(rejected,"Expected invalid input refusal");
}
// Synthetic CHR input, not a copy of commercial data. Parsing binds the same
// immutable details used by fresh gameplay and current-format restore.
struct Fixture {
	XeenConsequenceCharacters characters;
	XeenConsequenceInputs inputs;
	explicit Fixture(unsigned permanent=0,unsigned temporary=0,bool skill=true) {
		std::vector<std::uint8_t> bytes(30*XeenCharacter::kSerializedSize);
		for (unsigned n=0;n<30;++n) {
			auto *p=bytes.data()+n*XeenCharacter::kSerializedSize;
			for (unsigned attribute=0;attribute<7;++attribute) p[20+2*attribute]=15;
			p[35]=3;p[39]=skill;p[342]=100;p[346]=592&255;p[347]=592>>8;
			for (unsigned resistance=0;resistance<6;++resistance) {
				p[311+2*resistance]=permanent;p[312+2*resistance]=temporary;
			}
		}
		const auto roster=XeenCharacterFormat::parseRoster(bytes);
		for (unsigned n=0;n<6;++n) {
			characters[n]=roster.at(n);
			inputs[n]=XeenCharacterFormat::parseCombatInputs(bytes,n,true,true,true);
		}
	}
};
template<class C> void finish(C &candidate,XeenCombatRandom &rng,unsigned budget=1) {
	for (unsigned n=0;n<1000;++n) {
		XeenConsequenceDraw draw{rng,budget,{}};
		if (candidate.service(draw)) return;
	}
	throw std::runtime_error("Unfinished bounded damage candidate");
}
void resistanceInputs() {
	Fixture f(3,2);
	// Every type maps to its original CHR pair; supplement values are live.
	f.inputs[0].resistances->coldPermanent=11;f.inputs[0].resistances->coldTemporary=4;
	f.inputs[0].resistances->electricalPermanent=12;f.inputs[0].resistances->electricalTemporary=5;
	f.inputs[0].poisonResistance->permanent=13;f.inputs[0].poisonResistance->temporary=6;
	constexpr int expected[]{5,5,17,15,19,5};
	for (unsigned type=1;type<=6;++type)
		check(XeenCharacterRules::damageSaveValue(f.characters[0],f.inputs[0],XeenDamageType(type),{610})==expected[type-1],
			"Typed save must bind original pairs or the authoritative live supplement");
	// Independent original elemental-category table oracle, including every
	// material boundary, all equipment categories and unavailable equipment.
	constexpr unsigned limits[]{8,15,20,25,33,36},materials[]{1,9,16,21,26,34};
	constexpr unsigned types[]{2,3,4,5,6,1};
	constexpr int bonuses[]{0,5,7,9,12,15,20,25,30,5,7,9,12,15,20,25,5,10,15,20,25,10,15,20,25,40,5,7,9,11,13,15,20,25,5,10,20};
	Fixture zero;
	for (unsigned material=0;material<37;++material) {
		unsigned element=0;while (limits[element]<material) ++element;
		for (unsigned category=0;category<3;++category)
			for (unsigned state:{0u,64u,128u,192u}) for (unsigned frame:{0u,1u}) {
				auto c=zero.characters[0];
				auto &items=category==0 ? c.weapons : category==1 ? c.armor : c.accessories;
				items[0]={std::uint8_t(material),2,std::uint8_t(state),std::uint8_t(frame)};
				for (unsigned e=0;e<6;++e) {
					const auto want=category && frame && !state && e==element ? bonuses[material] : 0;
					check(XeenCharacterRules::damageSaveValue(c,zero.inputs[0],XeenDamageType(types[e]),{610})==want,
						"Elemental saving equipment differs from pinned itemScan");
				}
			}
	}
	for (unsigned e=0;e<6;++e) {
		auto c=zero.characters[0];c.armor[0]={std::uint8_t(materials[e]),2,0,1};c.accessories[0]=c.armor[0];
		check(XeenCharacterRules::damageSaveValue(c,zero.inputs[0],XeenDamageType(types[e]),{610})==2*bonuses[materials[e]],"Armor and accessory resistance stack");
	}
	XeenCharacter missing;XeenCombatInputs absent;
	rejects([&]{XeenCharacterRules::damageSaveValue(missing,absent,XeenDamageType::Fire,{610});});
	rejects([&]{XeenCharacterRules::damageSaveValue(missing,absent,XeenDamageType::Physical,{610});});
	rejects([&]{XeenCharacterRules::damageSaveValue(f.characters[0],f.inputs[0],XeenDamageType(7),{610});});
	f.characters[0].conditions[0]=3;
	check(XeenCharacterRules::damageSaveValue(f.characters[0],f.inputs[0],XeenDamageType::Physical,{610})==3,
		"Physical save uses condition-adjusted Luck bonus plus current level");
}
void damageOrder() {
	for (unsigned type=0;type<=6;++type) {
		Fixture f(20,3);f.characters[0].conditions[8]=1;
		XeenDamageProtection protection{{9,9,9,9},7};
		std::vector<XeenCombatRandom::Draw> tape;
		if (type) tape={{1,63,23},{1,63,23},{1,63,63}};
		XeenCombatRandom rng(tape);
		XeenTypedDamageCandidate c(f.characters,f.inputs,100,XeenDamageType(type),610,1,protection);
		finish(c,rng);
		const int expected=type==0 ? 93 : type>=2 && type<=5 ? 15 : 18;
		check(c.result.injuryCount==1 && c.result.damage==expected && c.characters[0].currentHp==100-expected &&
			!c.characters[0].conditions[8] && rng.position()==tape.size(),"Resistance/save/shield/wake order");
		check(f.characters[0].currentHp==100 && f.characters[0].conditions[8]==1,"Damage candidate must leave original owner unchanged");
		constexpr unsigned frames[]{0,6,1,2,3,4,5};check(c.portraitFrame==frames[type],"Original damage portrait frame");
		finish(c,rng);check(rng.position()==tape.size(),"Completed typed damage must not replay");
	}
	for (int amount:{-3,0,1,8}) {
		Fixture f(5);
		std::vector<XeenCombatRandom::Draw> tape;
		int remaining=amount;
		do {tape.push_back({1,45,5});if (remaining<=0) break;remaining/=2;} while (true);
		XeenCombatRandom rng(tape);XeenTypedDamageCandidate c(f.characters,f.inputs,amount,XeenDamageType::Fire,610,1,{});
		finish(c,rng);check(rng.position()==tape.size() && c.result.damage==0,"Save is called before each positive-damage test, including final zero");
	}
	Fixture f;
	XeenCombatRandom none(std::vector<XeenCombatRandom::Draw>{});
	XeenTypedDamageCandidate carry(f.characters,f.inputs,20,XeenDamageType::Physical,610,5,{{},3});finish(carry,none);
	check(carry.result.injuryCount==2 && carry.result.injuries[0].amount==17 && carry.result.injuries[1].amount==14 &&
		carry.result.injuries[1].owner==2 && !none.position(),"giveCharDamage carries reduced damage in member order");
	XeenCombatRandom resistanceTape(std::vector<XeenCombatRandom::Draw>{{1,40,40},{1,40,40}});
	XeenTypedDamageCandidate shared(f.characters,f.inputs,20,XeenDamageType::Cold,610,3,{{0,0,4,0},2});finish(shared,resistanceTape);
	check(shared.result.injuries[0].amount==14 && shared.result.injuries[1].amount==8,"Party resistance precedes save and shield for each member");
	XeenCombatRandom zeroTape(std::vector<XeenCombatRandom::Draw>{{1,40,40}});
	XeenTypedDamageCandidate negative(f.characters,f.inputs,3,XeenDamageType::Poison,610,1,{{0,0,0,4},2});finish(negative,zeroTape);
	check(negative.characters[0].currentHp==100 && zeroTape.position()==1,"Negative resisted damage still saves then clamps without healing");
}
void injuryAndPublication() {
	Fixture f;f.characters[0].armor[0]={0,2,0,1};f.characters[0].currentHp=1;
	for (int damage:{1,10,11,100}) {
		XeenCombatRandom none(std::vector<XeenCombatRandom::Draw>{});
		XeenTypedDamageCandidate c(f.characters,f.inputs,damage,XeenDamageType::Physical,610,1,{});finish(c,none);
		const auto &ch=c.characters[0];
		const bool dead=XeenCharacterRules::maxHp(ch,{610})+ch.currentHp<1;
		check(ch.currentHp==1-damage && ch.conditions[dead?13:12]==1,"HP and unconscious/dead threshold");
		check(bool(ch.armor[0].state&128)==(dead || ch.currentHp<=-10),"Original equipped armor break threshold");
		check(c.result.injuries[0].beforeHp==1 && c.result.injuries[0].afterHp==ch.currentHp &&
			c.result.injuries[0].beforeAc==6 && c.result.injuries[0].afterAc==(dead || ch.currentHp<=-10 ? 2 : 6),"Injury retains HP and armor-class observation");
	}
	XeenCombatRandom none(std::vector<XeenCombatRandom::Draw>{});
	XeenTypedDamageCandidate staged(f.characters,f.inputs,11,XeenDamageType::Physical,610,1,{});staged.deferInjury=true;
	XeenConsequenceDraw draw{none};
	check(!staged.service(draw) && staged.injuryReady && staged.impactOwner==0 && staged.characters[0].currentHp==1 &&
		!staged.result.injuryCount,"Portrait acknowledgement precedes detached HP injury");
	staged.injuryAcknowledged=true;
	check(!staged.service(draw) && staged.injuryApplied && staged.characters[0].currentHp==-10 && staged.result.injuryCount==1,"One acknowledged injury");
	check(staged.service(draw) && staged.result.injuryCount==1,"Next service completes without repeating injury");
	f.characters[0].currentHp=std::numeric_limits<std::int16_t>::min();
	XeenTypedDamageCandidate overflow(f.characters,f.inputs,1,XeenDamageType::Physical,610,1,{});
	rejects([&]{finish(overflow,none);});check(f.characters[0].currentHp==std::numeric_limits<std::int16_t>::min(),"Overflow cannot publish to original owner");
	rejects([&]{XeenTypedDamageCandidate invalid(f.characters,f.inputs,1,XeenDamageType::Fire,610,64,{});});
	rejects([&]{XeenTypedDamageCandidate invalid(f.characters,f.inputs,1,XeenDamageType::Fire,610,1,{{},-1});});
	Fixture good(5);std::vector<XeenCombatRandom::Draw> rejected(64,{1,45,0,true});rejected.push_back({1,45,45});
	XeenCombatRandom tape(rejected);XeenTypedDamageCandidate pending(good.characters,good.inputs,3,XeenDamageType::Energy,610,1,{});
	XeenConsequenceDraw first{tape};check(!pending.service(first) && tape.position()==64 && !pending.result.injuryCount,"Rejected raw draws yield at budget");
	finish(pending,tape);check(pending.result.damage==3 && tape.position()==65,"Rejected save resumes exactly");
	XeenCombatRandom stale(std::vector<XeenCombatRandom::Draw>{{1,45,45}});
	XeenTypedDamageCandidate unpublished(good.characters,good.inputs,3,XeenDamageType::Magical,610,1,{});
	XeenConsequenceDraw checked{stale,64,[]{throw std::runtime_error("Stale retained owner");}};
	rejects([&]{unpublished.service(checked);});check(!unpublished.result.injuryCount && good.characters[0].currentHp==100,"Stale draw callback cannot publish damage");
}
void thievery() {
	Fixture f;
	constexpr int racial[]{0,10,5,10,-10};
	for (unsigned cls=0;cls<10;++cls) for (unsigned race=0;race<5;++race) {
		auto c=f.characters[0];c.characterClass=XeenCharacterClass(cls);c.race=XeenRace(race);c.temporaryLevel=2;
		c.weapons[0]={121,1,0,1};c.armor[0]={121,2,0,1};c.accessories[0]={121,1,0,1};
		const int expected=10+(cls==5?30:cls==6?15:0)+racial[race]+12;
		check(XeenCharacterRules::thievery(c)==std::max(expected,0),"Thievery level/class/race/equipment contributions");
		c.weapons[0].state=128;c.armor[0].state=64;c.accessories[0].frame=0;
		check(XeenCharacterRules::thievery(c)==std::max(expected-12,0),"Thievery excludes broken/cursed/unequipped items");
	}
	Fixture noSkill(0,0,false);auto c=noSkill.characters[0];c.characterClass=XeenCharacterClass::Robber;
	check(XeenCharacterRules::thievery(c)==0,"Thievery skill gates all bonuses");
	c=f.characters[0];c.race=XeenRace::HalfOrc;c.permanentLevel=0;c.temporaryLevel=-1;
	check(XeenCharacterRules::thievery(c)==0,"Thievery clamps negative results");
	rejects([]{XeenCharacterRules::thievery(XeenCharacter{});});
	c.permanentLevel=std::numeric_limits<int>::max();c.temporaryLevel=1;
	rejects([&]{XeenCharacterRules::thievery(c);});
}
}
int main() {
	try {resistanceInputs();damageOrder();injuryAndPublication();thievery();std::cout<<"Typed damage and Thievery rules passed\n";return 0;}
	catch (const std::exception &e) {std::cerr<<e.what()<<'\n';return 1;}
}
