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
		const int first=variant==0?2:4;
		check(r.entries[0].owner==first&&r.delivered==10,"recipient preference/fallback/eligibility");
		check(!q.hasWork()&&p.party.activeRosterIds()==std::vector<std::uint8_t>({4,2,4}),"queue/membership altered");
		for(unsigned i=0;i<9;++i)check(same(p.roster.at(29).miscellaneous[i],untouched[i]),"inactive recipient");
		check(!r.lost,"Category overflow must replace rather than lose");
	}
	for(int condition=0;condition<17;++condition){Fixture f;f.initial.party=XeenParty::fromRosterIds({0});
		if(condition<16)f.initial.roster.at(0).conditions[condition]=1;
		XeenPendingRewards q;q.enqueue({1,2,3,4});
		auto r=xeenDeliverRewards(q,f.initial,{});check(r.delivered==1,"Unrestricted fallback accepts every condition");}
	for(int kind=0;kind<4;++kind){Fixture f;if(kind<2)full(f.initial,kind==1);
		if(kind==2)for(auto id:f.initial.party.activeRosterIds())f.initial.roster.at(id).conditions[13]=1;
		if(kind==3)f.initial.party=XeenParty::fromRosterIds({});
		check(xeenPacksGloballyFull(f.initial)==(kind==1),"global full distinct from recipient availability");
		XeenPendingRewards q;q.enqueue({1,2,3,4});auto r=xeenDeliverRewards(q,f.initial,{});
		check(r.delivered==unsigned(kind==0||kind==2) && r.lost==unsigned(kind==1||kind==3),"Only global fullness or empty party loses items");
		if(r.lost)check(r.entries[0].loss==(kind==1?XeenRewardLoss::GloballyFull:XeenRewardLoss::EmptyParty),"loss classification");}
}
void categories(){
    Fixture f;auto &p=f.initial;p.purse=XeenPurse{};
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
    Fixture fullPacks;fullPacks.initial.purse=XeenPurse{};full(fullPacks.initial,true);
    XeenPendingRewards gold;gold.credit(34,100);gold.credit(35,5);
    auto lost=xeenDeliverRewards(gold,fullPacks.initial,{});
    check(lost.gold==100 && lost.gems==5 && fullPacks.initial.purse->gold==100 && fullPacks.initial.purse->gems==5,"Globally full packs retain staged currency");
    for(auto owner:fullPacks.initial.party.activeRosterIds())for(unsigned category=0;category<4;++category)
      xeenRewardItems(fullPacks.initial.roster.at(owner),XeenRewardCategory(category)).fill({1,1,0,0});
    auto &last=fullPacks.initial.roster.at(fullPacks.initial.party.activeRosterIds()[0]).miscellaneous;last[8]={};
    XeenPendingRewards filling;filling.credit(34,100);filling.enqueue({1,1,1,0});
    lost=xeenDeliverRewards(filling,fullPacks.initial,{});
    check(lost.delivered==1 && lost.gold==100,"Filling the last free slot retains currency");
    Fixture finalSlot;auto &lastParty=finalSlot.initial;lastParty.party=XeenParty::fromRosterIds({0,1});
    lastParty.purse=XeenPurse{};
    for(auto owner:lastParty.party.activeRosterIds())for(unsigned category=0;category<4;++category)
      xeenRewardItems(lastParty.roster.at(owner),XeenRewardCategory(category)).fill({1,1,0,0});
    lastParty.roster.at(0).miscellaneous={};lastParty.roster.at(1).miscellaneous[8]={};
    XeenPendingRewards lastTen;for(unsigned i=0;i<10;++i)lastTen.enqueue({1,1,0,0});lastTen.credit(34,100);
    const auto final=xeenDeliverRewards(lastTen,lastParty,{});
    check(final.delivered==10 && final.gold==100 && xeenPacksGloballyFull(lastParty),
      "Last miscellaneous slot must not add a further capacity check");
}
void dosDelivery() {
 for(unsigned category=0;category<4;++category) {
  const auto kind=XeenRewardCategory(category);
  for(int preference=0;preference<3;++preference)for(unsigned condition:{8u,11u,12u,13u,14u,15u}) {
   Fixture f;f.initial.roster.at(0).conditions[condition]=1;
   XeenPendingRewards q;q.enqueue({3,2,5,0},kind);
   const auto r=xeenDeliverRewards(q,f.initial,preference==0?std::optional<std::size_t>{}:preference==1?99:0);
   check(r.entries[0].owner==(preference==2?1:0),"Valid preferred uses eligible scan; absent/invalid goes straight to capacity");
   f.initial.roster.at(1).conditions[13]=1;q.enqueue({3,2,5,0},kind);
   check(xeenDeliverRewards(q,f.initial,0).entries[0].owner==0,"Valid preferred exhausted eligible scan falls back to disabled/dead");
  }
  Fixture f;auto &p=f.initial;p.purse=XeenPurse{};
  for(auto id:p.party.activeRosterIds())xeenRewardItems(p.roster.at(id),kind).fill({0,1,0,0});
  auto &tail=xeenRewardItems(p.roster.at(0),kind).back();tail.frame=category==0?4:category==1?3:category==2?8:7;
  const auto frame=tail.frame;XeenPendingRewards q;q.enqueue({3,2,5,0},kind);q.credit(34,23);q.credit(35,7);
  const auto r=xeenDeliverRewards(q,p,1);
  check(r.entries[0].owner==0 && r.delivered==1 && same(tail,{3,2,5,frame}) && same(r.entries[0].item,tail),"Category replacement keeps first owner's equipped mark and receipt bytes");
  auto saved=save_test::sample();
  for(unsigned owner=0;owner<30;++owner)for(unsigned category=0;category<4;++category)
   xeenRewardItems(saved.characters[owner],XeenRewardCategory(category))=xeenRewardItems(p.roster.at(owner),XeenRewardCategory(category));
  saved.journey->purse->gold=p.purse->gold;saved.journey->purse->gems=p.purse->gems;
  const auto bytes=XeenSaveFormat::encode(saved);
  sameSnapshot(saved,XeenSaveFormat::decode(bytes));check(XeenSaveFormat::encode(XeenSaveFormat::decode(bytes))==bytes,"Delivered inventory generic save round-trip");
 }
 Fixture f;f.initial.purse=XeenPurse{};
 for(auto id:f.initial.party.activeRosterIds())for(unsigned category=0;category<4;++category)
  xeenRewardItems(f.initial.roster.at(id),XeenRewardCategory(category)).fill({1,1,1,0});
 f.initial.roster.at(0).weapons[8]={};
 XeenPendingRewards q;q.enqueue({0,2,0,0},XeenRewardCategory::Weapons);q.enqueue({1,2,3,0});q.credit(34,9);q.credit(35,4);
 auto r=xeenDeliverRewards(q,f.initial,{});
 check(r.delivered==1 && r.lost==1 && r.entries[1].loss==XeenRewardLoss::GloballyFull && r.gold==9 && r.gems==4,"Becoming globally full discards remaining items only");
}
void stagedDelivery() {
 Fixture f;auto &party=f.initial;party.purse=XeenPurse{};
 party.questItems.increment(17);
 XeenPendingRewards pending;pending.enqueue({0,6,0,0},XeenRewardCategory::Weapons);
 pending.enqueueQuest(99);pending.credit(34,27);pending.credit(35,4);
 auto receipt=xeenPrepareRewardReceipt(pending);
 check(receipt.count==2 && receipt.entries[1].quest && !receipt.delivered && !party.purse->gold,
  "Preparing treasure published rewards");
 xeenDeliverRewardEntry(receipt,0,party,{},false);
 auto &weapons=party.roster.at(0).weapons;
 check(weapons.back().id==6 && !weapons.front().id && receipt.delivered==1 && !party.purse->gold,
  "Transfer must precede sorting and currency");
 bool duplicate=false;try{xeenDeliverRewardEntry(receipt,0,party,{},false);}catch(const std::logic_error &){duplicate=true;}
 check(duplicate,"Duplicate item publication accepted");
 xeenSortRewardEntry(receipt.entries[0],party);
 check(weapons.front().id==6 && !weapons.back().id,"Found-line transition did not sort inventory");
 const auto inventory=weapons;xeenDeliverRewardEntry(receipt,1,party,{},false);xeenSortRewardEntry(receipt.entries[1],party);
 check(std::equal(weapons.begin(),weapons.end(),inventory.begin(),same) && party.questItems.at(17)==1 && receipt.delivered==2 && !party.purse->gold,
  "Quest presentation inserted inventory or granted its counter twice");
 xeenCreditRewardCurrency(pending,receipt,party);
 check(!pending.hasWork() && party.purse->gold==27 && party.purse->gems==4,
  "Final acknowledgment did not settle staged currency");
}
int main(){try{model();recipients();categories();dosDelivery();stagedDelivery();std::cout<<"Bounded item model and staged recipient matrix passed\n";return 0;}catch(const std::exception&e){std::cerr<<e.what()<<'\n';return 1;}}
