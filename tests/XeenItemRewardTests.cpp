#include "XeenRewardTestSupport.h"
#include "XeenPartySnapshotTestSupport.h"
#include <iostream>
using namespace reward_test;
bool same(XeenItem a,XeenItem b){return a.material==b.material&&a.id==b.id&&a.state==b.state&&a.frame==b.frame;}
void model(){
	Fixture f; auto &c=f.initial.roster.at(0);
	c.miscellaneous[0]={91,0,92,93};c.miscellaneous[2]={3,42,193,5};c.miscellaneous[6]={6,43,194,8};
	c.miscellaneous[8]={9,44,195,10};const auto before=c.miscellaneous;
	check(!xeenInsertMiscellaneous(c,{10,37,1,0}),"hole incorrectly supplied capacity");
	for(unsigned i=0;i<9;++i)check(same(before[i],c.miscellaneous[i]),"full mutation");
	c.miscellaneous[8]={11,0,99,44};
	check(xeenInsertMiscellaneous(c,{255,255,255,255}),"tail insertion refused opaque record");
	check(same(c.miscellaneous[0],before[2])&&same(c.miscellaneous[1],before[6])&&same(c.miscellaneous[2],{255,255,255,255}),"slot-8 compaction order/fields");
	for(unsigned i=3;i<9;++i)check(same(c.miscellaneous[i],{}),"empty metadata not compacted");
	for(int i=0;i<6;++i)check(xeenInsertMiscellaneous(c,{10,37,1,0}),"exact remaining capacity");
	check(!xeenInsertMiscellaneous(c,{10,37,1,0})&&!xeenInsertMiscellaneous(c,{}),"capacity/empty rejection");
	XeenPendingRewards q;check(q.enqueue({})==XeenRewardEnqueue::InvalidEmpty,"empty input");
	for(unsigned i=0;i<10;++i)check(q.enqueue({10,static_cast<std::uint8_t>(i+1),193,7})==XeenRewardEnqueue::Accepted,"ten records");
	check(q.enqueue({1,99,2,3})==XeenRewardEnqueue::IgnoredOverflow&&q.size()==10&&q.overflow()==1&&q.invalid()==1,"overflow");
	for(unsigned i=0;i<10;++i)check(q.at(i).id==i+1,"overflow overwrote/order");
}
void recipients(){
	{Fixture f;save_test::distinctiveInitialItems(f.initial.roster);
		f.initial.roster.at(0).miscellaneous={};f.initial.roster.at(1).conditions[13]=1;
		const auto before=f.initial.roster.characters();XeenPendingRewards q;q.enqueue({10,37,193,7});
		xeenDeliverRewards(q,f.initial,{});
		for(unsigned id=0;id<30;++id){auto expected=before[id];if(id==0)expected.miscellaneous[0]={10,37,193,7};
			remove_test::checkSameCharacter(expected,f.initial.roster.at(id));}}
	for(int variant=0;variant<7;++variant){
		Fixture f;auto &p=f.initial;p.party=XeenParty::fromRosterIds({4,2,4});
		p.roster.at(4).currentHp=-10;p.roster.at(4).currentSp=-10;
		p.roster.at(2).miscellaneous[7]={99,0,99,99};
		if(variant==1)p.roster.at(2).conditions[13]=1;
		if(variant==2)p.roster.at(2).miscellaneous[8]={1,2,3,4};
		if(variant==3)p.roster.at(4).conditions[8]=1;
		if(variant==4){p.roster.at(4).conditions[8]=1;p.roster.at(4).conditions[10]=1;}
		XeenPendingRewards q;for(int i=0;i<10;++i)q.enqueue({10,static_cast<std::uint8_t>(37+i),193,7});
		const auto untouched=p.roster.at(29).miscellaneous;
		const auto r=xeenDeliverRewards(q,p,variant<3?std::optional<std::size_t>(1):variant==5?std::optional<std::size_t>(99):std::nullopt);
		const int first=variant==0||variant==3?2:4;
		check(r.entries[0].owner==first&&r.delivered==(variant==1||variant==2||variant==3?9U:10U),"recipient preference/fallback/eligibility");
		check(!q.hasWork()&&p.party.activeRosterIds()==std::vector<std::uint8_t>({4,2,4}),"queue/membership altered");
		for(unsigned i=0;i<9;++i)check(same(p.roster.at(29).miscellaneous[i],untouched[i]),"inactive recipient");
		if(r.lost)check(r.entries[9].loss==XeenRewardLoss::CategoryFull,"capacity loss reason");
	}
	for(int condition=0;condition<17;++condition){Fixture f;f.initial.party=XeenParty::fromRosterIds({0});
		if(condition<16)f.initial.roster.at(0).conditions[condition]=1;
		const bool eligible=f.initial.roster.at(0).canAct();XeenPendingRewards q;q.enqueue({1,2,3,4});
		auto r=xeenDeliverRewards(q,f.initial,{});check(r.delivered==unsigned(eligible),"worst-condition predicate");}
	for(int kind=0;kind<4;++kind){Fixture f;if(kind<2)full(f.initial,kind==1);
		if(kind==2)for(auto id:f.initial.party.activeRosterIds())f.initial.roster.at(id).conditions[13]=1;
		if(kind==3)f.initial.party=XeenParty::fromRosterIds({});
		check(xeenPacksGloballyFull(f.initial)==(kind==1),"global full distinct from recipient availability");
		XeenPendingRewards q;q.enqueue({1,2,3,4});auto r=xeenDeliverRewards(q,f.initial,{});
		check(r.delivered==0&&r.lost==1&&r.entries[0].loss==(kind<2?XeenRewardLoss::CategoryFull:kind==2?XeenRewardLoss::NoEligibleMember:XeenRewardLoss::EmptyParty),"loss classification");}
}
void categories(){
    Fixture f;auto &p=f.initial;p.monsterTreasure=XeenMonsterTreasure{};
    XeenPendingRewards q;
    for(unsigned category=0;category<4;++category) {
      for(unsigned i=0;i<10;++i)check(q.enqueue({1,std::uint8_t(i+1),3,0},XeenRewardCategory(category))==XeenRewardEnqueue::Accepted,"Category staging capacity");
      check(q.enqueue({1,11,3,0},XeenRewardCategory(category))==XeenRewardEnqueue::IgnoredOverflow,"Per-category overflow");
    }
    q.credit(34,100);q.credit(35,7);
    const auto r=xeenDeliverRewards(q,p,1);
    check(r.count==40 && r.delivered==40 && r.overflow==4 && r.gold==100 && r.gems==7 && !q.hasWork(),"Four-category settlement");
    for(unsigned category=0;category<4;++category) {
      check(r.entries[category*10].category==XeenRewardCategory(category) && r.entries[category*10].owner==1,"Category order/preference");
      check(xeenRewardItems(p.roster.at(1),XeenRewardCategory(category))[8].id==9,"Nine inventory slots");
      check(xeenRewardItems(p.roster.at(0),XeenRewardCategory(category))[0].id==10,"Party fallback");
    }
    Fixture fullPacks;fullPacks.initial.monsterTreasure=XeenMonsterTreasure{};full(fullPacks.initial,true);
    XeenPendingRewards gold;gold.credit(34,100);gold.credit(35,5);
    auto lost=xeenDeliverRewards(gold,fullPacks.initial,{});
    check(lost.currencyLost && !lost.gold && !fullPacks.initial.monsterTreasure->gold,"Globally full packs retain staged currency");
    for(auto owner:fullPacks.initial.party.activeRosterIds())for(unsigned category=0;category<4;++category)
      xeenRewardItems(fullPacks.initial.roster.at(owner),XeenRewardCategory(category)).fill({1,1,0,0});
    auto &last=fullPacks.initial.roster.at(fullPacks.initial.party.activeRosterIds()[0]).miscellaneous;last[8]={};
    XeenPendingRewards filling;filling.credit(34,100);filling.enqueue({1,1,1,0});
    lost=xeenDeliverRewards(filling,fullPacks.initial,{});
    check(lost.delivered==1 && lost.currencyLost,"Empty treasure slots must still check global capacity");
    Fixture finalSlot;auto &lastParty=finalSlot.initial;lastParty.party=XeenParty::fromRosterIds({0,1});
    lastParty.monsterTreasure=XeenMonsterTreasure{};
    for(auto owner:lastParty.party.activeRosterIds())for(unsigned category=0;category<4;++category)
      xeenRewardItems(lastParty.roster.at(owner),XeenRewardCategory(category)).fill({1,1,0,0});
    lastParty.roster.at(0).miscellaneous={};lastParty.roster.at(1).miscellaneous[8]={};
    XeenPendingRewards lastTen;for(unsigned i=0;i<10;++i)lastTen.enqueue({1,1,0,0});lastTen.credit(34,100);
    const auto final=xeenDeliverRewards(lastTen,lastParty,{});
    check(final.delivered==10 && !final.currencyLost && final.gold==100 && xeenPacksGloballyFull(lastParty),
      "Last miscellaneous slot must not add a further capacity check");
}
int main(){try{model();recipients();categories();std::cout<<"Bounded item model and recipient matrix passed\n";return 0;}catch(const std::exception&e){std::cerr<<e.what()<<'\n';return 1;}}
