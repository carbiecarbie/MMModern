#include "XeenSaveGameplayTestSupport.h"
#include "games/xeen/XeenCharacterRules.h"
#include <iostream>
using namespace gameplay_test;
using Status = XeenTransferStatus;
using Category = XeenInventoryCategory;
namespace {
void same(const XeenPartyState &a,const XeenPartyState &b) {
	check(a.party.activeRosterIds()==b.party.activeRosterIds(),"membership changed");
	for (unsigned i=0;i<30;++i) remove_test::checkSameCharacter(a.roster.at(i),b.roster.at(i));
	check(a.questItems.counts()==b.questItems.counts() && a.questFlags.values()==b.questFlags.values(),"quest changed");
}
void refused(XeenPartyState &p, Status status, std::size_t src=0,std::size_t dst=1,Category cat=Category::Weapons,std::size_t slot=0) {
	const auto before=p;
	check(xeenTransferItem(p,src,dst,cat,slot).status==status,"wrong transfer refusal precedence");
	same(p,before);
}
void arrays() {
	for (unsigned cat=0;cat<4;++cat) for(unsigned slot : {0U,4U,8U}) {
		Fixture f; auto &p=f.initial; p.party=XeenParty::fromRosterIds({0,1,1});
		const auto category=static_cast<Category>(cat);
		auto &src=*xeenInventoryItems(p.roster.at(0),category), &dst=*xeenInventoryItems(p.roster.at(1),category);
		for(unsigned i=0;i<9;++i) src[i]={255,static_cast<std::uint8_t>(100+i),128,static_cast<std::uint8_t>(200+i)};
		dst={{{1,8,2,3},{100,0,55,66},{2,9,4,5},{},{},{},{},{},{123,0,255,255}}};
		p.roster.at(29).miscellaneous[7]={111,222,123,234};
		p.questItems.increment(17);p.questFlags.set(2);
		auto expected=p;
		auto &es=*xeenInventoryItems(expected.roster.at(0),category), &ed=*xeenInventoryItems(expected.roster.at(1),category);
		es={}; unsigned next=0;
		for(unsigned i=0;i<9;++i) if(i!=slot) es[next++]={255,static_cast<std::uint8_t>(100+i),128,static_cast<std::uint8_t>(200+i)};
		ed={{{1,8,2,3},{2,9,4,5},{255,static_cast<std::uint8_t>(100+slot),128,0},{},{},{},{},{},{}}};
		const auto result=xeenTransferItem(p,0,2,category,slot);
		check(result.status==Status::Success && result.destinationSlot==2,"all-category insertion failed");
		same(p,expected); check(p.party.member(p.roster,1).rosterId==p.party.member(p.roster,2).rosterId,"destination alias detached");
	}
	Fixture f;auto &p=f.initial;
	p.roster.at(0).weapons={{{90,0,8,1},{3,4,5,6},{},{7,8,9,10},{},{},{},{},{}}};
	auto expected=p;expected.roster.at(0).weapons={{{7,8,9,10},{},{},{},{},{},{},{},{}}};
	expected.roster.at(1).weapons={{{3,4,5,0},{},{},{},{},{},{},{},{}}};
	check(xeenTransferItem(p,0,1,Category::Weapons,1).status==Status::Success,"source holes");same(p,expected);
}
void refusals() {
	Fixture f;auto &p=f.initial;
	refused(p,Status::SameOwner,0,0,static_cast<Category>(255),99);
	p.party=XeenParty::fromRosterIds({0,0,1});refused(p,Status::SameOwner);
	p.party=XeenParty::fromRosterIds({0,1});
	refused(p,Status::InvalidParticipant,0,29);refused(p,Status::InvalidParticipant,9,0);
	refused(p,Status::InvalidCategory,0,1,static_cast<Category>(255));
	refused(p,Status::InvalidSlot,0,1,Category::Weapons,9);refused(p,Status::EmptySource);
	p.roster.at(0).weapons[0]={99,12,192,255};p.roster.at(1).weapons[8]={1,2,3,4};
	refused(p,Status::Cursed);p.roster.at(0).weapons[0].state=64;refused(p,Status::Cursed);
	p.roster.at(0).weapons[0].state=128;refused(p,Status::DestinationFull);
	p.roster.at(1).rosterId=29;refused(p,Status::InvalidOwner);p.roster.at(1).rosterId=1;
	p.party=XeenParty::fromRosterIds({});refused(p,Status::EmptyParty);
}
void conditions() {
	for(unsigned condition=0;condition<=16;++condition) for(bool recipient:{false,true}) for(int hp:{-32768,0,32767}) {
		Fixture f;auto &p=f.initial;auto &c=p.roster.at(recipient?1:0);
		if(condition<16)c.conditions[condition]=255;
		c.currentHp=hp;c.currentSp=hp;
		p.roster.at(0).characterClass=XeenCharacterClass::Sorcerer;
		p.roster.at(0).armor[0]={255,255,191,255};
		auto expected=p;expected.roster.at(0).armor={};expected.roster.at(1).armor[0]={255,255,191,0};
		check(xeenTransferItem(p,0,1,Category::Armor,0).status==Status::Success,"condition/class gate added");same(p,expected);
	}
}
void rules() {
	{
		Fixture f;auto &p=f.initial;auto &c=p.roster.at(0);
		c.birthYear=592;c.permanentLevel=1;c.intellect.permanent=c.personality.permanent=c.endurance.permanent=11;c.hasSpells=true;
		c.miscellaneous[0]={110,37,1,255};
		check(XeenCharacterRules::maxHp(c,{610})==10&&XeenCharacterRules::maxSp(c,{610})==1,"miscellaneous introduced effects");
		check(xeenTransferItem(p,0,1,Category::Miscellaneous,0).status==Status::Success,"misc modifier transfer");
		check(XeenCharacterRules::maxHp(c,{610})==10&&XeenCharacterRules::maxSp(c,{610})==1,"misc transfer changed maxima");
	}
	for(bool sp:{false,true})for(int current:{-12,sp?6:16}) {
		Fixture f;auto &p=f.initial;
		for(unsigned i=0;i<2;++i) {
			auto &c=p.roster.at(i);c.birthYear=592;c.permanentLevel=1;
			c.characterClass=XeenCharacterClass::Paladin;c.endurance.permanent=19;
			c.personality.permanent=13;c.hasSpells=true;c.currentHp=current;c.currentSp=current;
		}
		auto &src=p.roster.at(0),&dst=p.roster.at(1);
		src.weapons[0]={static_cast<std::uint8_t>(sp?110:105),12,0,1};
		check(XeenCharacterRules::maxHp(src,{610})==(sp?12:16) && XeenCharacterRules::maxSp(src,{610})==(sp?6:2),"independent before maxima");
		check(xeenTransferItem(p,0,1,Category::Weapons,0).status==Status::Success,"modifier move");
		check(XeenCharacterRules::maxHp(src,{610})==12 && XeenCharacterRules::maxSp(src,{610})==2 &&
			XeenCharacterRules::maxHp(dst,{610})==12 && XeenCharacterRules::maxSp(dst,{610})==2,"modifier removal/recipient equip");
		check(src.currentHp==current && src.currentSp==current && dst.currentHp==current && dst.currentSp==current,"currents changed");
	}
	for(unsigned material:{69U,77U}) {
		Fixture f;auto &p=f.initial;
		for(unsigned i=0;i<2;++i){auto &c=p.roster.at(i);c.birthYear=592;c.intellect.permanent=c.personality.permanent=11;c.weapons[7]={static_cast<std::uint8_t>(material),0,0,1};}
		p.roster.at(0).weapons[0]={1,2,0,0};
		for(unsigned i=0;i<2;++i)check((material==69?XeenCharacterRules::effectiveIntellect(p.roster.at(i),{610}):XeenCharacterRules::effectivePersonality(p.roster.at(i),{610}))==13,"ID-zero bonus absent");
		check(xeenTransferItem(p,0,1,Category::Weapons,0).status==Status::Success,"empty metadata compaction");
		for(unsigned i=0;i<2;++i)check((material==69?XeenCharacterRules::effectiveIntellect(p.roster.at(i),{610}):XeenCharacterRules::effectivePersonality(p.roster.at(i),{610}))==11,"both empty bonuses removed");
	}
	for(bool destination:{false,true}) {
		Fixture f;auto &p=f.initial;auto &c=p.roster.at(destination?1:0);
		c.birthYear=592;c.intellect.permanent=std::numeric_limits<int>::min();c.conditions[static_cast<unsigned>(XeenCondition::Weak)]=1;
		c.weapons[7]={69,0,0,1};p.roster.at(0).weapons[0]={0,12,0,1};
		XeenCharacterRules::validateForUse(c,{610});
		refused(p,Status::UnsafeRules);
	}
}
}
int main(){try{arrays();refusals();conditions();rules();std::cout<<"Item transfer arrays, precedence, conditions and independent rules passed\n";return 0;}catch(const std::exception &e){std::cerr<<e.what()<<'\n';return 1;}}
