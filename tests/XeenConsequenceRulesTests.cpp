#include "XeenTreasureDeliveryTestSupport.h"
#include "games/xeen/XeenItemRewards.h"
#include "games/xeen/XeenCombatRules.h"
#include "games/xeen/XeenRegionalRules.h"
#include "games/xeen/XeenJourneyProgression.h"
#include "games/xeen/XeenCharacterRules.h"
#include "formats/xeen/XeenCharacterFormat.h"
#include "games/xeen/XeenServiceDay.h"
#include "games/xeen/XeenMerchantTables.h"
#include "games/xeen/XeenItemGeneration.h"
#include "XeenSaveTestSupport.h"
#include <iostream>
#include <limits>
#include <stdexcept>
using namespace mmodern;
namespace {
void check(bool v,const char *m) { if (!v) throw std::runtime_error(m); }
// Orc treasure inputs (resource 6): 10 gold, no gems, drop field 1.
XeenMonsterRecord orc() {XeenMonsterRecord r;r.raw[42]=10;r.raw[45]=1;return r;}
template<class F> void rejects(F f) { bool refused=false;try { f(); } catch (const std::exception &) { refused=true; } check(refused,"Expected refusal"); }
// Explicit artificial rule fixtures; these are not original-resource journeys.
XeenConsequenceCharacters characters() {
	XeenConsequenceCharacters result;
	for (unsigned i=0;i<6;++i) { auto &c=result[i];c.rosterId=kXeenCombatOwners[i];c.permanentLevel=3;c.birthYear=592;c.currentHp=50;c.intellect=c.personality=c.endurance={15,0}; }
	return result;
}
// The shared delivery (0x24128) over a six-member party built from fixture
// characters: every item in category/slot order, preferring the staged
// WhoWill as every delivery does, then the staged currency.
struct Delivered {
	struct Record { XeenItem item; XeenRewardCategory category{}; std::optional<std::uint8_t> recipient; XeenRewardLoss loss=XeenRewardLoss::None; };
	XeenConsequenceCharacters characters;
	XeenTreasureStaging treasure;
	std::array<Record,40> records{};
	std::size_t count=0;
	bool globallyFull=false;
};
Delivered deliverAll(const XeenTreasureStaging &staged,const XeenConsequenceCharacters &c) {
	if(!staged.pending())throw std::invalid_argument("Treasure is not ready for delivery");
	XeenPartyState party;std::vector<std::uint8_t> ids;
	for(const auto &v:c) {party.roster.at(v.rosterId)=v;ids.push_back(v.rosterId);}
	party.party=XeenParty::fromRosterIds(ids);
	Delivered result;result.globallyFull=xeenPacksGloballyFull(party);
	const auto receipt=xeenTestDeliverTreasure(staged,party,staged.preferredRecipient());
	for(unsigned i=0;i<6;++i)result.characters[i]=party.roster.at(c[i].rosterId);
	result.count=receipt.count;
	for(std::size_t i=0;i<receipt.count;++i) {
		const auto &e=receipt.entries[i];result.records[i]={e.item,e.category,e.owner,e.loss};
	}
	result.treasure=staged;
	for(unsigned category=0;category<4;++category)result.treasure.items(static_cast<XeenRewardCategory>(category)).fill({});
	return result;
}
struct Credited { XeenPurse purse; XeenTreasureStaging treasure; };
Credited creditAll(const XeenPurse &purse,const XeenTreasureStaging &staged) {
	if(staged.storedItems())throw std::invalid_argument("Items must be delivered before the currency credit");
	return {xeenCreditTreasure(purse,staged),XeenTreasureStaging{}};
}
XeenConsequenceInputs inputs() {
	XeenConsequenceInputs result;
	for (auto &i:result) { i.might=i.speed=i.accuracy={15,0};i.luck=XeenAttributeValue{15,0};i.resistances=XeenCombatResistances{}; }
	return result;
}
XeenMonsterRecord profile(unsigned hates=1,unsigned special=0) {
	XeenMonsterRecord m;m.raw[24]=1;m.raw[22]=5;m.raw[25]=hates;m.raw[26]=1;m.raw[28]=10;m.raw[30]=special;m.raw[31]=5;return m;
}
template<class C> void finish(C &c,XeenCombatRandom &rng,unsigned budget=64) {
	for (unsigned service=0;service<1000;++service) { XeenConsequenceDraw draw{rng,budget,{}};if (c.service(draw)) return; }
	throw std::runtime_error("Candidate did not complete bounded service");
}
void physical() {
	auto c=characters();auto i=inputs();
	c[0].characterClass=XeenCharacterClass::Paladin;
	XeenCombatRandom miss(std::vector<XeenCombatRandom::Draw>{{1,6,1},{0,4,1},{1,20,1}});
	XeenEnemyAttackCandidate noPreference(c,i,profile(),610,0x3f);finish(noPreference,miss,1);
	check(noPreference.result.targetOwner==c[1].rosterId && noPreference.result.injuryCount==0 && miss.position()==3,"Hates one is random and natural one ends the attack");
	c[0].characterClass=XeenCharacterClass::Cleric;c[0].conditions[8]=1;
	XeenCombatRandom asleep(std::vector<XeenCombatRandom::Draw>{{1,10,4}});
	XeenEnemyAttackCandidate preferred(c,i,profile(3),610,0x3f);finish(preferred,asleep,1);
	check(preferred.characters[0].currentHp==46 && !preferred.characters[0].conditions[8] && asleep.position()==1,"Sleeping Cleric remains preferred and skips hit draws");
	for (auto &owner:c) owner.conditions[8]=1;
	c[5].conditions[13]=1;
	std::vector<XeenCombatRandom::Draw> tape;
	for (unsigned n=0;n<6;++n) { tape.push_back({1,10,2});tape.push_back({1,25,25}); }
	XeenCombatRandom partyTape(tape);XeenEnemyAttackCandidate all(c,i,profile(15,9),610,0x3f);finish(all,partyTape,1);
	check(all.result.injuryCount==6 && partyTape.position()==12,"Hates party includes dead targets without selection draws");
	for (const auto &owner:all.characters) check(owner.currentHp==48 && owner.conditions[8]==1,"Wake precedes Sleep reapplication");
	c=characters();c[0].characterClass=XeenCharacterClass::Cleric;
	// Noncritical total 15 reaches ordinary AC(2)+10 but not blocked AC+16.
	std::array<bool,6> blocked{};blocked[0]=true;
	XeenCombatRandom blockTape(std::vector<XeenCombatRandom::Draw>{{1,20,10},{1,5,4}});
	XeenEnemyAttackCandidate block(c,i,profile(3),610,0x3f,blocked);finish(block,blockTape);
	check(block.result.injuryCount==0,"Block threshold");
	XeenCombatRandom ordinaryTape(std::vector<XeenCombatRandom::Draw>{{1,20,10},{1,5,4},{1,10,3}});
	XeenEnemyAttackCandidate ordinary(c,i,profile(3),610,0x3f);finish(ordinary,ordinaryTape);
	check(ordinary.characters[0].currentHp==47,"Ordinary threshold");
	c[0].conditions[3]=255;c[0].conditions[8]=1;
	XeenCombatRandom overflowTape(std::vector<XeenCombatRandom::Draw>{{1,10,1},{1,25,25}});
	XeenEnemyAttackCandidate overflow(c,i,profile(3,5),610,0x3f);finish(overflow,overflowTape);check(overflow.characters[0].conditions[3]==255,"Combat Poison saturates");
	check(c[0].currentHp==50 && c[0].conditions[3]==255,"Pure overflow does not mutate source");
}
void targetingAndCounts() {
 auto c=characters();auto in=inputs();
 // Each source owns its whole attack loop, including ranged callers.
 for(unsigned count=0;count<256;++count) {
  auto monster=profile(16);monster.raw[24]=count;
  std::vector<XeenCombatRandom::Draw> tape;
  for(unsigned n=0;n<count;++n){tape.push_back({1,6,1});tape.push_back({0,4,2});tape.push_back({1,20,1});}
  XeenCombatRandom rng(tape);XeenEnemyAttackCandidate attack(c,in,monster,610,63);
  unsigned completed=0;do{finish(attack,rng,1);++completed;}while(attack.nextAttack());
  check(completed==std::max(count,1u),"Source loop count differs from record");
  check(rng.position()==3*count && !attack.result.injuryCount,"Source attack count, zero count or repeated target differs");
 }
 c[1].race=XeenRace::Dwarf;c[3].race=XeenRace::Dwarf;c[1].conditions[12]=1;
 XeenCombatRandom none(std::vector<XeenCombatRandom::Draw>{});
 XeenMonsterTargetCandidate dwarf(c,12,63);finish(dwarf,none,1);
 check(dwarf.mask==8 && !none.position(),"First eligible dwarf requires no selection draw");
 XeenMonsterTargetCandidate absent(c,12,5);XeenCombatRandom absentRng(std::vector<XeenCombatRandom::Draw>{{0,1,1}});finish(absent,absentRng,1);
 check(absent.mask==4 && absentRng.position()==1,"Absent dwarf falls back over participating party");
 for(unsigned hated=0;hated<10;++hated) {
  auto matching=characters();for(auto &member:matching)member.characterClass=static_cast<XeenCharacterClass>((hated+1)%10);
  matching[4].characterClass=matching[5].characterClass=static_cast<XeenCharacterClass>(hated);
  matching[4].conditions[12]=1;matching[5].conditions[8]=1;
  std::vector<XeenCombatRandom::Draw> tape;if(hated==1){tape.push_back({1,6,1});tape.push_back({0,4,0});}
  XeenCombatRandom rng(tape);XeenMonsterTargetCandidate selector(matching,hated,63);finish(selector,rng,1);
  check(selector.mask==(hated==1?1u:32u) && rng.position()==tape.size(),
   "Every recognized class selects its first eligible match; Paladin hatred remains random");
 }
 // First injury disables the preferred member; second attack reselects.
 c=characters();c[0].characterClass=c[1].characterClass=XeenCharacterClass::Cleric;c[0].currentHp=1;
 auto monster=profile(3);monster.raw[24]=2;
 XeenCombatRandom lethalRng(std::vector<XeenCombatRandom::Draw>{{1,20,19},{1,5,5},{1,10,10},{1,20,1}});
 XeenEnemyAttackCandidate lethal(c,in,monster,610,63);finish(lethal,lethalRng,1);
 check(lethal.result.targetedMembers==1 && lethal.result.injuryCount==1 && lethal.nextAttack(),"First source attack observation missing");
 finish(lethal,lethalRng,1);
 check(lethal.result.targetedMembers==2 && !lethal.result.injuryCount && lethal.characters[0].currentHp==-9,
  "Lethal first attack must re-evaluate eligible preferred targets");
 // Party hatred includes disabled members on every pass in original order.
 c=characters();for(auto &ch:c)ch.conditions[8]=1;
 monster=profile(15);monster.raw[24]=3;
 std::vector<XeenCombatRandom::Draw> allTape;
 for(unsigned n=0;n<18;++n) {
  if(n>=6){allTape.push_back({1,20,19});allTape.push_back({1,5,5});}
  allTape.push_back({1,10,1});
 }
 XeenCombatRandom allRng(allTape);XeenEnemyAttackCandidate all(c,in,monster,610,63);
 for(unsigned ordinal=0;ordinal<3;++ordinal) {
  finish(all,allRng,1);check(all.result.injuryCount==6,"Whole-party pass lost injuries");
  for(unsigned n=0;n<6;++n)check(all.result.injury(n).owner==kXeenCombatOwners[n] && all.result.injury(n).attackOrdinal==ordinal,
   "Source attack ordinal/party order changed");
  check(all.nextAttack()==(ordinal<2),"Whole-party source count differs");
 }
 const auto before=allRng.position();finish(all,allRng);check(allRng.position()==before,"Completed source replays RNG");
}
void runAndParticipation() {
 // Artificial unsigned-threshold (DOS 0x2e1cc) and every six-owner subset controls:
 // a byte of 128 or more always succeeds.
 for (int threshold:{0,1,2,99,100,101,127,128,200,255}) for (unsigned roll=1;roll<=100;++roll) {
  XeenCombatRandom rng(std::vector<XeenCombatRandom::Draw>{{1,100,roll}});
  XeenRunCandidate run(threshold);finish(run,rng,1);
  check(run.roll==roll && run.success==(int(roll)<threshold) && rng.position()==1,"Run unsigned threshold and exact inclusive draw");
  finish(run,rng,1);check(rng.position()==1,"Completed Run does not redraw");
 }
 rejects([]{XeenRunCandidate invalid(-1);});rejects([]{XeenRunCandidate invalid(256);});
 std::vector<XeenCombatRandom::Draw> rejected(64,{1,100,0,true});rejected.push_back({1,100,99});
 XeenCombatRandom rng(rejected);XeenRunCandidate run(100);XeenConsequenceDraw first{rng};
 check(!run.service(first) && !run.roll && rng.position()==64,"Run yields exactly at raw budget");
 finish(run,rng,1);check(run.success && rng.position()==65,"Run resumes rejected conversion prefix");
 XeenCombatRandom stale(std::vector<XeenCombatRandom::Draw>{{1,100,1}});XeenRunCandidate unpublished(100);
 XeenConsequenceDraw checked{stale,64,[]{throw std::runtime_error("stale artificial authority");}};
 rejects([&]{unpublished.service(checked);});check(!unpublished.roll && !unpublished.success,"Authority refusal leaves Run observation unpublished");
 XeenCombatRandom overflow(XeenJourneyRandomState{1,3,std::numeric_limits<std::uint64_t>::max()});XeenRunCandidate exhausted(100);
 rejects([&]{finish(exhausted,overflow);});check(!exhausted.roll && overflow.position()==std::numeric_limits<std::uint64_t>::max(),"Run cursor overflow publishes nothing");
 const auto in=inputs();
 for (unsigned mask=0;mask<64;++mask) {
  auto c=characters();std::vector<unsigned> slots;std::vector<XeenCombatRandom::Draw> tape;
  for (unsigned owner=0;owner<6;++owner) { c[owner].conditions[8]=1;if(mask&(1u<<owner)){slots.push_back(owner);tape.push_back({1,10,2});} }
  c[5].conditions[13]=1;
  XeenCombatRandom allTape(tape);XeenEnemyAttackCandidate all(c,in,profile(15),610,mask);finish(all,allTape,1);
  check(all.result.injuryCount==slots.size() && all.result.targetedMembers==mask && allTape.position()==slots.size(),"All-party visits only participants including dead in fixed order");
  for(unsigned owner=0;owner<6;++owner) check(all.characters[owner].currentHp==((mask&(1u<<owner))?48:50) && all.characters[owner].conditions[8]==((mask&(1u<<owner))?0:1),"Escaped HP and Sleep bytes are untouched");
  for(unsigned n=0;n<slots.size();++n)check(all.result.injuries[n].owner==c[slots[n]].rosterId,"Participant injuries retain stable owner order");
  if(slots.empty()) {check(all.result.attackOutcome==XeenCombatAttackOutcome::NoParticipants,"Explicit no-participants result");continue;}
  c=characters();for(auto &owner:c)owner.conditions[13]=1;
  std::vector<XeenCombatRandom::Draw> initial;
  if(slots.size()==6)initial={{1,6,1},{0,4,0}};
  else if(slots.size()>1)initial={{0,unsigned(slots.size()-1),0}};
  XeenCombatRandom noTarget(initial);
  XeenEnemyAttackCandidate dead(c,in,profile(),610,mask);finish(dead,noTarget,1);
  check(noTarget.position()==initial.size() && !dead.result.targetOwner && !dead.result.injuryCount,"Nonempty exhausted participants retain initial random draw only");
  c[slots.back()].conditions[13]=0;
  auto fallback=initial;
  if(slots.size()>1)fallback.push_back({0,0,0});fallback.push_back({1,20,1});
  XeenCombatRandom targetTape(fallback);XeenEnemyAttackCandidate target(c,in,profile(),610,mask);finish(target,targetTape,1);
  check(target.result.targetOwner==c[slots.back()].rosterId && targetTape.position()==fallback.size(),"Ordered masked fallback retains singleton draw");
  c=characters();for(auto &owner:c)owner.characterClass=XeenCharacterClass::Cleric;
  XeenCombatRandom preference(std::vector<XeenCombatRandom::Draw>{{1,20,1}});
  XeenEnemyAttackCandidate cleric(c,in,profile(3),610,mask);finish(cleric,preference,1);
  check(cleric.result.targetOwner==c[slots.front()].rosterId && preference.position()==1,"Preferred class excludes escaped owners");
 }
 auto c=characters();rejects([&]{XeenEnemyAttackCandidate invalid(c,in,profile(),610,64);});
 XeenCombatRandom none(std::vector<XeenCombatRandom::Draw>{});XeenEnemyAttackCandidate empty(c,in,profile(),610,0);finish(empty,none);
 check(empty.result.attackOutcome==XeenCombatAttackOutcome::NoParticipants && none.position()==0,"Empty random-target view never draws an invalid interval");
 XeenActor actor;actor.id={23,9};actor.hp=25;actor.lifecycle=XeenActorLifecycle::Present;actor.statistics=profile();actor.statistics->raw[16]=61;
 for(unsigned mask=1;mask<64;++mask){
  std::array<const XeenCharacter *,6> owners{};auto xpInputs=in;unsigned count=0;
  for(unsigned n=0;n<6;++n){owners[n]=&c[n];xpInputs[n].experience=100+n;if(mask&(1u<<n))++count;}
  const auto lethal=xeenPrepareJourneyLethal(actor,owners,xpInputs,{},mask);
  for(unsigned n=0;n<6;++n)check(lethal.experience[n]==100+n+((mask&(1u<<n))?(actor.statistics->experience()/count)*2:0),"XP divides only among participants and preserves excluded supplements");
  rejects([&]{xeenPrepareJourneyLethal(actor,owners,xpInputs,{},0);});
 }
 // The eligibility domains intentionally differ: Sleep is targetable but not
 // actionable, and Unconscious still participates in XP division.
 for (int condition:{-1,3,4,8,12,13}) {
  c=characters();if(condition>=0)c[1].conditions[condition]=2;
  const bool targetable=condition!=12 && condition!=13;
  check(xeenCombatTargetable(c[1])==targetable,"Continuing-life predicate differs from action eligibility");
  std::array<const XeenCharacter *,6> owners{};auto xpInputs=in;
  for(unsigned n=0;n<6;++n){owners[n]=&c[n];xpInputs[n].experience=100;}
  const auto kill=xeenPrepareJourneyLethal(actor,owners,xpInputs,{},3);
  check(kill.experience[0]==(condition==13?222u:160u) && kill.experience[1]==(condition==13?100u:160u),"XP includes Sleep and Unconscious, excludes Dead");
  check(kill.experience[2]==100,"Excluded XP owner retained");
 }
 c=characters();c[1].conditions[3]=2;c[1].conditions[4]=3;c[1].conditions[8]=4;c[1].conditions[12]=5;
 std::array<const XeenCharacter *,6> owners{};auto xpInputs=in;
 for(unsigned n=0;n<6;++n){owners[n]=&c[n];xpInputs[n].experience=100;}
 xpInputs[5].experience=std::numeric_limits<std::uint32_t>::max();
 const auto exact=c[1];const auto kill=xeenPrepareJourneyLethal(actor,owners,xpInputs,{},3);
 check(kill.experience[0]==160 && kill.experience[1]==160 && kill.experience[5]==xpInputs[5].experience && c[1].conditions==exact.conditions,"Simultaneous conditions preserve XP domain; escaped maximum XP cannot overflow");
 xpInputs[1].experience=std::numeric_limits<std::uint32_t>::max();
 rejects([&]{xeenPrepareJourneyLethal(actor,owners,xpInputs,{},3);});
 c[1].conditions[13]=7;const auto dead=xeenPrepareJourneyLethal(actor,owners,xpInputs,{},3);
 check(dead.experience[1]==xpInputs[1].experience && c[1].conditions[13]==7,"Dead participant's XP and exact death counter survive lethal publication");

}
// Mutations: skip drop generation at full capacity, shift charge offset, replay gold,
// or discard dormant items on Run. Literal tapes also reject extra/missing draws.
void shootAndLoot() {
	auto c=characters();auto i=inputs();c[0].weapons[0]={0,30,0,4};i[0].accuracy={18,0};
	auto m=profile();m.raw[40]=0;
	XeenCombatRandom seed(3);XeenPhysicalPlayerCandidate shot(c[0],i[0],m,6,610,true);finish(shot,seed,1);
	check(shot.hit && shot.damage==9 && seed.position()==5,"Seed three wound arithmetic");
	XeenCombatRandom lootTape(std::vector<XeenCombatRandom::Draw>{{1,100,8},{1,100,48},{1,100,50},{1,7,2},{1,100,46}});
	XeenPurse purse;purse.gold=800;purse.gems=10;
	XeenMonsterDropCandidate loot(XeenTreasureStaging{},orc(),XeenMapIdentity(23));finish(loot,lootTape,1);
	check(loot.outcome==XeenMonsterDropOutcome::Item && loot.category==XeenRewardCategory::Armor && loot.treasure.armor[0].id==2 &&
		loot.treasure.gold==10 && loot.treasure.ready,"Armor production");
	auto delivery=deliverAll(loot.treasure,c);
	check(delivery.records[0].recipient==c[0].rosterId && delivery.characters[0].armor[0].id==2 && delivery.treasure.gold==10,"Items before receipt, gold still pending");
	const auto paid=creditAll(purse,delivery.treasure);
	check(paid.purse.gold==810 && paid.purse.gems==10 && !paid.treasure.pending(),"Final credit");
	XeenCombatRandom wandTape(std::vector<XeenCombatRandom::Draw>{{1,100,10},{1,100,86},{1,100,1},{1,9,3},{1,100,1},{1,15,8},{1,8,7}});
	XeenMonsterDropCandidate wand(XeenTreasureStaging{},orc(),XeenMapIdentity(23));finish(wand,wandTape,1);
	check(wand.outcome==XeenMonsterDropOutcome::Item && wandTape.position()==7 && wand.generated.material==3 && wand.generated.id==8 && wand.generated.state==9 &&
		xeenSameItem(wand.treasure.miscellaneous[0],wand.generated),"Misc staging preserves all seven ordered draws and generated bytes");
	auto misc=deliverAll(wand.treasure,c);
	check(misc.count==1 && misc.records[0].category==XeenRewardCategory::Miscellaneous && misc.records[0].recipient==c[0].rosterId &&
		xeenSameItem(misc.characters[0].miscellaneous[0],wand.generated) && !misc.treasure.storedItems(),"Misc uses shared delivery");
	rejects([&]{creditAll(purse,wand.treasure);});
	auto dormant=wand.treasure;dormant.clearPending();
	check(dormant.dormant() && xeenSameItem(dormant.miscellaneous[0],wand.generated),"Run retains dormant Misc");
	XeenCombatRandom nextTape(std::vector<XeenCombatRandom::Draw>{{1,100,11}});
	XeenMonsterDropCandidate next(dormant,orc(),XeenMapIdentity(23));finish(next,nextTape,1);
	check(next.treasure.pending() && !next.treasure.ready && next.treasure.gold==10 && xeenSameItem(next.treasure.miscellaneous[0],wand.generated),
		"Later Orc reactivates Misc without old gold or readiness");
	for(auto &v:c)v.miscellaneous[8]={1,1,1,0};
	misc=deliverAll(wand.treasure,c);
	check(misc.records[0].recipient==c[0].rosterId && !misc.treasure.storedItems(),"Misc full category replaces first tail");
	c[1].miscellaneous={};c[1].conditions[8]=1;c[2].miscellaneous={};
	misc=deliverAll(wand.treasure,c);
	check(misc.records[0].recipient==c[1].rosterId,"Misc fallback accepts sleeping member with room");
	for(auto &v:c)v.conditions[8]=1;
	misc=deliverAll(wand.treasure,c);
	check(misc.records[0].recipient==c[1].rosterId,"Misc unrestricted fallback accepts all sleeping party");
	for(auto &v:c)for(auto *items:{&v.weapons,&v.armor,&v.accessories,&v.miscellaneous})(*items)[8]={1,1,1,0};
	misc=deliverAll(wand.treasure,c);
	check(misc.globallyFull && misc.records[0].loss==XeenRewardLoss::GloballyFull &&
		creditAll(purse,misc.treasure).purse.gold==810,"Globally full Misc uses existing gold rule");
	XeenTreasureStaging many;
	for(unsigned source=0;source<11;++source) {
		XeenCombatRandom tape(std::vector<XeenCombatRandom::Draw>{{1,100,10},{1,100,100},{1,100,100},{1,9,9},{1,100,100},{1,15,15},{1,8,8}});
		XeenMonsterDropCandidate drop(many,orc(),XeenMapIdentity(23));finish(drop,tape,1);
		check(tape.position()==7 && drop.outcome==(source==10?XeenMonsterDropOutcome::CategoryCapacityLoss:XeenMonsterDropOutcome::Item),"Full Misc staging still consumes all generation draws");
		many=drop.treasure;
	}
	check(many.miscellaneous[9].material==9 && many.gold==110,"Ten Misc slots preserve bounds and gold");
	for (unsigned id=1;id<=34;++id) check(xeenOrdinaryWeaponDice(id).count && xeenOrdinaryWeaponDice(id).sides,"Complete weapon table");
	rejects([] { xeenOrdinaryWeaponDice(35); });
	// Staged gold wraps modulo 2^32 like the DOS add/carry pair; no refusal.
	XeenTreasureStaging high;high.gold=std::numeric_limits<std::uint32_t>::max()-9;
	XeenMonsterDropCandidate wrapped(high,orc(),XeenMapIdentity(23));check(wrapped.treasure.gold==0,"Staged gold did not wrap modulo 2^32");
}
void additionalTapes() {
 auto c=characters();auto i=inputs();
 for(auto &v:c)v.conditions[13]=1;c[2].conditions[13]=0;
 XeenCombatRandom fallback(std::vector<XeenCombatRandom::Draw>{{1,6,1},{0,4,0},{0,0,0},{1,20,1}});
 XeenEnemyAttackCandidate single(c,i,profile(),610,0x3f);finish(single,fallback,1);
 check(single.result.targetOwner==c[2].rosterId && fallback.position()==4,"One surviving fallback still draws [0,0]");
 c=characters();c[0].characterClass=XeenCharacterClass::Cleric;
 XeenCombatRandom critical(std::vector<XeenCombatRandom::Draw>{{1,20,20},{1,10,1},{1,25,25},{1,5,1},{1,10,1},{1,25,5}});
 XeenEnemyAttackCandidate poison(c,i,profile(3,5),610,0x3f);finish(poison,critical,1);
 check(poison.result.injuryCount==2 && poison.characters[0].conditions[3]==1 && poison.characters[0].currentHp==48 && critical.position()==6,"Critical first injury then parameter, equality saves second Poison");
 check(poison.result.injuries[0].beforeAc==2 && poison.result.injuries[0].afterAc==1 && poison.result.injuries[1].beforeAc==1,"Poison injury observation retains pre-special AC and next-application AC");
 for(unsigned id=30;id<=33;++id){
  c=characters();c[0].weapons[0]={0,std::uint8_t(id),128,4};constexpr unsigned dice[]{3,5,4,2};
  std::vector<XeenCombatRandom::Draw> tape(dice[id-30],{1,2,2});tape.push_back({1,20,1});
  XeenCombatRandom rng(tape);XeenPhysicalPlayerCandidate miss(c[0],i[0],profile(),6,610,true);finish(miss,rng,1);
  check(!miss.hit && !miss.damage && rng.position()==dice[id-30]+1,"All missile dice precede hit; broken base contribution retained");
 }
 c=characters();XeenGameplayContext context;context.year=610;context.day=8;context.minutes=959;
 std::vector<XeenCombatRandom::Draw> tape;
 for(unsigned n=0;n<6;++n){c[n].conditions[3]=c[n].conditions[4]=1;tape.push_back({1,10,1});tape.push_back({1,40,40});tape.push_back({1,10,1});tape.push_back({1,40,1});}
 XeenCombatRandom tick(tape);XeenConditionTimeCandidate minute(context,1,c,i);finish(minute,tick,1);
 check(minute.context.minutes==960 && tick.position()==24,"Round/End crossing and zero-resistance [1,40] branch draws");
 c[0].conditions[13]=255;XeenCombatRandom overflow(tape);XeenConditionTimeCandidate death(context,1,c,i);finish(death,overflow,1);check(death.characters[0].conditions[13]==255,"Time Dead saturates");check(c[0].conditions[13]==255,"Dead255 tick is unpublished");
 XeenTreasureStaging staged;
 XeenCombatRandom noDrop(std::vector<XeenCombatRandom::Draw>{{1,100,11}});XeenMonsterDropCandidate eleven(staged,orc(),XeenMapIdentity(23));finish(eleven,noDrop,1);check(eleven.outcome==XeenMonsterDropOutcome::None && noDrop.position()==1,"Drop11 boundary");
 const std::array<unsigned,6> sub{30,31,60,61,85,86};
 const std::array<unsigned,6> lo{1,7,7,18,18,30},hi{6,17,17,29,29,33};
 for(unsigned n=0;n<6;++n){XeenCombatRandom rng(std::vector<XeenCombatRandom::Draw>{{1,100,10},{1,100,40},{1,100,sub[n]},{lo[n],hi[n],lo[n]},{1,100,100}});XeenMonsterDropCandidate drop(staged,orc(),XeenMapIdentity(23));finish(drop,rng,1);check(drop.generated.id==lo[n] && drop.category==XeenRewardCategory::Weapons && rng.position()==5,"Weapon subcategory endpoints and ignored enchantment");}
 for(unsigned cat:{41u,85u}){XeenCombatRandom rng(std::vector<XeenCombatRandom::Draw>{{1,100,10},{1,100,cat},{1,100,100},{1,7,7},{1,100,1}});XeenMonsterDropCandidate drop(staged,orc(),XeenMapIdentity(23));finish(drop,rng,1);check(drop.generated.id==7 && drop.category==XeenRewardCategory::Armor,"Armor category boundaries");}
 for(unsigned source=0;source<11;++source){XeenCombatRandom rng(std::vector<XeenCombatRandom::Draw>{{1,100,10},{1,100,1},{1,100,1},{1,6,1},{1,100,1}});XeenMonsterDropCandidate drop(staged,orc(),XeenMapIdentity(23));finish(drop,rng,1);check(rng.position()==5 && drop.outcome==(source==10?XeenMonsterDropOutcome::CategoryCapacityLoss:XeenMonsterDropOutcome::Item),"Eleventh category item lost only after complete generation");staged=drop.treasure;}
 check(staged.gold==110 && staged.weapons[9].id==1,"All lethal sources retain gold despite item overflow");
 c=characters();for(auto &v:c)for(auto *items:{&v.weapons,&v.armor,&v.accessories,&v.miscellaneous})(*items)[8]={0,1,0,0};
 const auto full=deliverAll(staged,c);check(full.globallyFull && full.count==10 && full.treasure.gold==110 && !full.treasure.weapons[0].id,"Global four-tail warning clears item queues, preserves gold");
 for(auto &v:c)v.armor[8]={};c[0].conditions[8]=1;c[1].weapons[8]={};
 const auto delivered=deliverAll(staged,c);check(!delivered.globallyFull && delivered.records[0].recipient==c[1].rosterId,"Capacity fallback skips the full first owner");
}

void missileClassAndZeroDamage() {
 constexpr unsigned divisors[]{1,2,2,3,4,2,2,1,3,2};
 for(unsigned type=0;type<10;++type)for(bool hit:{false,true}) {
  auto c=characters();auto in=inputs();c[0].characterClass=static_cast<XeenCharacterClass>(type);c[0].permanentLevel=12;
  in[0].accuracy={12,0};in[0].might={255,0};c[0].weapons[0]={0,30,0,4};
  auto m=profile();m.raw[22]=12/divisors[type]+2;m.raw[40]=100;
  std::vector<XeenCombatRandom::Draw> tape{{1,2,1},{1,2,1},{1,2,1},{1,20,hit?7u:6u}};
  if(hit)tape.push_back({1,56,56});
  XeenCombatRandom rng(tape);XeenPhysicalPlayerCandidate shot(c[0],in[0],m,6,610,true);finish(shot,rng,1);
  check(shot.hit==hit && shot.damage==0 && rng.position()==tape.size(),"Every missile class divisor: exact threshold, no Might/multiple melee attacks, zero resisted hit still saves");
 }
 auto c=characters();XeenTreasureStaging treasure;treasure.gold=10;treasure.ready=true;treasure.weapons[0]={0,30,0,0};
 for(auto &v:c)v.weapons[8]={0,1,0,0};
 auto full=deliverAll(treasure,c);check(!full.globallyFull && full.records[0].recipient==c[0].rosterId && full.treasure.gold==10,"Category full tail replaced despite holes; gold retained");
 for(auto &v:c){v.weapons={};v.conditions[8]=1;}
 auto disabled=deliverAll(treasure,c);check(disabled.records[0].recipient==c[0].rosterId && disabled.treasure.gold==10,"Disabled recipient receives item and gold remains pending");
}

void dosMonsterDelivery() {
 auto c=characters();XeenTreasureStaging t;t.gold=30;t.ready=true;XeenPurse purse;purse.gold=100;purse.gems=7;
 t.weapons[0]={0,30,0,0};t.armor[0]={0,2,0,0};t.miscellaneous[0]={3,8,7,0};
 for(auto &v:c){v.weapons.fill({0,1,0,0});v.armor.fill({0,1,0,0});v.miscellaneous.fill({1,1,1,0});v.conditions[13]=1;}
 c[0].weapons.back().frame=4;c[0].armor.back().frame=3;c[0].miscellaneous.back().frame=7;
 auto d=deliverAll(t,c);
 check(d.count==3 && d.records[0].recipient==c[0].rosterId && d.records[1].recipient==c[0].rosterId && d.records[2].recipient==c[0].rosterId,"Dead first owner receives category replacements");
 check(xeenSameItem(d.characters[0].weapons.back(),{0,30,0,4}) && xeenSameItem(d.characters[0].armor.back(),{0,2,0,3}) && xeenSameItem(d.characters[0].miscellaneous.back(),{3,8,7,7}),"Monster category replacements preserve marks");
 check(d.records[0].item.frame==4 && d.records[1].item.frame==3 && d.records[2].item.frame==7,"Monster receipt retains delivered frame");
 for(auto &v:c)v.accessories.fill({0,1,0,0});c[0].weapons.back()={};
 d=deliverAll(t,c);
 check(d.records[0].recipient==c[0].rosterId && d.records[1].loss==XeenRewardLoss::GloballyFull && d.records[2].loss==XeenRewardLoss::GloballyFull,"Monster delivery rechecks fullness after each item");
 const auto paid=creditAll(purse,d.treasure);check(paid.purse.gold==130 && paid.purse.gems==7 && !paid.treasure.storedItems(),"Full monster packs retain currency");
}
void rejectionBudgets() {
 auto c=characters();auto in=inputs();c[0].characterClass=XeenCharacterClass::Cleric;
 std::vector<XeenCombatRandom::Draw> tape(64,{1,20,0,true});tape.push_back({1,20,1});
 XeenCombatRandom random(tape);XeenEnemyAttackCandidate attack(c,in,profile(3),610,0x3f);XeenConsequenceDraw draw{random,64,{}};
 check(!attack.service(draw) && random.position()==64 && c[0].currentHp==50,"64 rejected raw conversions yield unpublished");finish(attack,random,1);check(random.position()==65 && attack.result.injuryCount==0,"Rejected continuation consumes exact next raw draw");
 c[0].weapons[0]={0,30,0,4};tape={{1,2,1},{1,2,1},{1,2,1}};for(unsigned n=0;n<70;++n)tape.push_back({1,20,20});tape.push_back({1,20,1});tape.push_back({1,56,56});
 XeenCombatRandom exploding(tape);XeenPhysicalPlayerCandidate shot(c[0],in[0],profile(),6,610,true);XeenConsequenceDraw first{exploding,64,{}};
 check(!shot.service(first) && exploding.position()==64,"Exploding hit prefix bounded before publication");finish(shot,exploding,1);check(shot.hit && shot.damage==9 && exploding.position()==75,"Exploding continuation never repeats weapon dice");
}
void completeWeaponRules() {
 constexpr unsigned count[]{3,2,3,2,2,4,1,2,4,2,3,2,2,1,1,1,1,4,4,3,2,4,2,2,2,5,3,3,3,3,5,4,2,6};
 constexpr unsigned sides[]{3,3,4,5,4,2,3,3,3,3,3,2,4,10,6,8,9,4,3,6,8,5,6,4,5,3,5,6,7,2,2,2,2,4};
 for(unsigned id=1;id<=34;++id){
  const auto dice=xeenOrdinaryWeaponDice(id);check(dice.count==count[id-1] && dice.sides==sides[id-1],"Every literal ordinary weapon dice pair");
  auto c=characters();auto in=inputs();c[0].permanentLevel=1;in[0].might={12,0};
  const bool missile=id>=30 && id<=33;c[0].weapons[0]={0,std::uint8_t(id),128,std::uint8_t(missile?4:id<18?1:13)};
  auto mon=profile();mon.raw[22]=0;mon.raw[40]=0;
  std::vector<XeenCombatRandom::Draw> tape(count[id-1],{1,sides[id-1],sides[id-1]});tape.push_back({1,20,19});
  if(missile)tape.push_back({1,56,56});
  XeenCombatRandom rng(tape);XeenPhysicalPlayerCandidate attack(c[0],in[0],mon,6,610,missile);finish(attack,rng,1);
  if(!attack.hit || attack.damage!=int(count[id-1]*sides[id-1]*3))throw std::runtime_error("Weapon ID "+std::to_string(id)+" damage="+std::to_string(attack.damage)+" expected="+std::to_string(count[id-1]*sides[id-1]*3));
 }
 auto c=characters();auto in=inputs();auto old=c[0];c[0].conditions[3]=3;c[0].conditions[4]=4;
 for(auto a:{XeenCharacterRules::PhysicalAttribute::Might,XeenCharacterRules::PhysicalAttribute::Speed,XeenCharacterRules::PhysicalAttribute::Accuracy})check(XeenCharacterRules::effectivePhysical(c[0],in[0],a,{610})==XeenCharacterRules::effectivePhysical(old,in[0],a,{610})-3,"Poison changes precisely physical stats");
 check(XeenCharacterRules::effectiveIntellect(c[0],{610})==XeenCharacterRules::effectiveIntellect(old,{610})-4 && XeenCharacterRules::effectivePersonality(c[0],{610})==XeenCharacterRules::effectivePersonality(old,{610})-4 && XeenCharacterRules::effectiveEndurance(c[0],{610})==XeenCharacterRules::effectiveEndurance(old,{610})-4,"Disease changes mental/endurance stats");
 check(c[0].currentHp==old.currentHp && c[0].currentSp==old.currentSp,"Derived changes never clamp HP/SP");
}
void partyVolley() {
 XeenPartyVolley volley;volley.eligible.fill(true);
 for(unsigned i=0;i<6;++i) {
  check(volley.remaining() && volley.select() && volley.shooter==i,"Volley party order");
  volley.settle(i%2==0);
 }
 check(!volley.select() && volley.remaining(),"Volley retried a miss on the same target");
 volley.nextTarget();
 for(unsigned i:{1u,3u,5u}) {
  check(volley.select() && volley.shooter==i,"Volley miss did not retain the shot");volley.settle(true);
 }
 volley.nextTarget();check(!volley.select() && !volley.remaining(),"Volley spent more than one shot per member");
 XeenPartyVolley sparse;sparse.eligible[2]=true;sparse.eligible[5]=true;
 check(sparse.select() && sparse.shooter==2,"Volley ineligible member fired");sparse.settle(true);
 check(sparse.select() && sparse.shooter==5,"Volley sparse order");sparse.settle(true);
 check(!sparse.remaining(),"Volley sparse exhaustion");
}
void poisonVolley() {
 for(bool weapon:{false,true})for(bool save:{false,true})for(unsigned resistance:{0u,50u,100u}) {
  auto c=characters();auto in=inputs();auto m=profile();
  m.raw[34]=resistance;m.raw[39]=100;m.raw[40]=100; // Neither magic gate nor physical resistance applies.
  c[0].permanentLevel=20;c[0].characterClass=XeenCharacterClass::Knight;
  if(weapon)c[0].weapons[0]={1,30,0,4}; // Fire material, three d2 dice still drawn.
  std::vector<XeenCombatRandom::Draw> tape;
  if(weapon)for(unsigned n=0;n<3;++n)tape.push_back({1,2,2});
  tape.push_back({1,20,1});tape.push_back({1,56,save?6u:7u});
  XeenCombatRandom rng(tape);XeenSpellDamageCandidate shot(c[0],in[0],m,6,610,XeenDifficulty::Warrior,XeenActorStatus::Sleep,true);
  finish(shot,rng,1);
  check(!shot.miss && shot.impact && shot.damage==(10+(weapon?2*(100-int(resistance))/100:0))/(save?2:1) && shot.status==XeenActorStatus::Physical && rng.position()==tape.size(),"Poison Volley base/element/save/draw order");
 }
 auto c=characters();auto in=inputs();auto m=profile();
 c[0].permanentLevel=1;c[0].characterClass=XeenCharacterClass::Knight;in[0].accuracy={12,0};
 c[0].weapons[0]={37,30,0,4};
 for(bool clouds:{false,true}) {
  std::vector<XeenCombatRandom::Draw> tape{{1,2,2},{1,2,2},{1,2,2},{1,20,1}};
  if(!clouds)tape.push_back({1,139,139});
  XeenCombatRandom rng(tape);XeenSpellDamageCandidate shot(c[0],in[0],m,89,610,XeenDifficulty::Warrior,XeenActorStatus::Physical,clouds);
  finish(shot,rng,1);
  check(!shot.miss && shot.damage==(clouds?0:10) && shot.impact==!clouds && rng.position()==tape.size(),"Poison Volley negative hit/refusal/save boundary");
 }
}
void unsignedPlayerHit() {
 for(bool shoot:{false,true})for(int total:{-1,0,1,14,15}) {
  auto c=characters();auto in=inputs();auto m=profile();
  c[0].permanentLevel=1;c[0].characterClass=XeenCharacterClass::Knight;
  // Metal 37 supplies -3 hit: class base 1 minus 3, then U(1,20).
  in[0].accuracy={12,0};in[0].might={15,0};
  c[0].weapons[0]={37,std::uint8_t(shoot?30:1),0,std::uint8_t(shoot?4:1)};
  std::vector<XeenCombatRandom::Draw> tape;
  for(unsigned n=0;n<3;++n)tape.push_back({1,shoot?2u:3u,2});
  tape.push_back({1,20,unsigned(total+2)});
  const bool hit=total==-1 || total>=15;
  if(shoot && hit)tape.push_back({1,56,56});
  XeenCombatRandom rng(tape);
  XeenPhysicalPlayerCandidate candidate(c[0],in[0],m,6,610,shoot,XeenDifficulty::Warrior);
  finish(candidate,rng,1);
  check(candidate.hit==hit && (candidate.damage>0)==hit && rng.position()==tape.size(),"DOS unsigned hit at -1/0/positive differs");
 }
}
void difficultyOracles() {
 // combat.cpp:1725: weapon multiplication precedes Might and resistance;
 // difficulty changes the base hit without consuming any additional draws.
 for(auto difficulty:{XeenDifficulty::Adventurer,XeenDifficulty::Warrior})
 for(bool shoot:{false,true})for(bool armed:{false,true})for(unsigned roll:{8u,9u,13u,14u,19u}) {
  auto c=characters();auto in=inputs();auto m=profile();
  c[0].permanentLevel=1;c[0].characterClass=XeenCharacterClass::Knight;
  in[0].accuracy={12,0};in[0].might={17,0};m.raw[22]=5;m.raw[40]=25;
  if(armed)c[0].weapons[0]={0,std::uint8_t(shoot?30:1),0,std::uint8_t(shoot?4:1)};
  std::vector<XeenCombatRandom::Draw> tape;
  if(armed)for(unsigned n=0;n<3;++n)tape.push_back({1,shoot?2u:3u,2});
  tape.push_back({1,20,roll});
  const bool hit=int(roll)+1+(difficulty==XeenDifficulty::Adventurer?5:0)>=15;
  const int weapon=armed?6*(difficulty==XeenDifficulty::Adventurer?3:1):0;
  const int expected=hit?(shoot?weapon:std::max(weapon+3,1))*75/100:0;
  if(shoot && hit && weapon)tape.push_back({1,56,56});
  XeenCombatRandom rng(tape);
  XeenPhysicalPlayerCandidate attack(c[0],in[0],m,6,610,shoot,difficulty);finish(attack,rng,1);
  check(attack.hit==hit && attack.damage==expected && rng.position()==tape.size(),"Difficulty hit/damage/draw oracle");
 }
 auto c=characters();auto i=inputs();rejects([&]{XeenPhysicalPlayerCandidate bad(c[0],i[0],profile(),6,610,false,static_cast<XeenDifficulty>(2));});
}
// M67 worked examples (docs/rules/combat.md): metal hit/damage, elemental damage
// per material range, effectiveness per species, Shoot and the Slayer bypass.
void equippedContributions() {
 const auto knight=[](bool shoot=false,std::uint8_t material=0,std::uint8_t state=0,std::uint8_t id=1){
  auto c=characters();c[0].characterClass=XeenCharacterClass::Knight;c[0].permanentLevel=1;
  c[0].weapons[0]={material,id,state,std::uint8_t(shoot?4:1)};return c;};
 const auto in12=[]{auto in=inputs();in[0].accuracy={12,0};in[0].might={15,0};return in;};
 const auto monster=[](unsigned physical=0,unsigned type=255){auto m=profile();m.raw[22]=5;m.raw[40]=std::uint8_t(physical);m.raw[33]=std::uint8_t(type);return m;};
 using Draw=XeenCombatRandom::Draw;using Warrior=XeenDifficulty;
 // Metal 45 (index 8): +6 hit, +10 damage before the 3d3 dice; Warrior keeps it untripled.
 {auto c=knight(false,45);auto in=in12();
  XeenCombatRandom rng(std::vector<Draw>{{1,3,2},{1,3,2},{1,3,2},{1,20,8}});XeenPhysicalPlayerCandidate a(c[0],in[0],monster(),6,610,false,Warrior::Warrior);finish(a,rng,1);
  check(a.hit && a.damage==18 && a.physical==18 && a.elemental==0 && !a.elementalFrame && rng.position()==4,"Metal 45: 10+6 dice +2 Might, hit at 1+6+8");
  XeenCombatRandom miss(std::vector<Draw>{{1,3,2},{1,3,2},{1,3,2},{1,20,7}});XeenPhysicalPlayerCandidate b(c[0],in[0],monster(),6,610,false,Warrior::Warrior);finish(b,miss,1);
  check(!b.hit && b.damage==0,"Metal hit bonus decides the threshold");
  XeenCombatRandom adventurer(std::vector<Draw>{{1,3,2},{1,3,2},{1,3,2},{1,20,3}});XeenPhysicalPlayerCandidate d(c[0],in[0],monster(),6,610,false);finish(d,adventurer,1);
  check(d.hit && d.damage==50,"Adventurer triples metal+dice (48) before Might, hit at 6+6+3");}
 // Metal 38 (index 1): -4 hit, -6 damage; a negative weapon amount clamps to zero, Might still counts.
 {auto c=knight(false,38,0,2);auto in=in12();
  XeenCombatRandom rng(std::vector<Draw>{{1,3,1},{1,3,1},{1,20,18}});XeenPhysicalPlayerCandidate a(c[0],in[0],monster(),6,610,false,Warrior::Warrior);finish(a,rng,1);
  check(a.hit && a.damage==2,"Metal 38: 2d3=2-6 clamps to 0, max(0+2,1)=2, hit at 1-4+18");}
 // Elemental fire 5 (damage 10) once after physical resistance; resistance 100 removes it.
 for(unsigned fire:{0u,50u,100u}) {auto c=knight(false,5);auto in=in12();auto m=monster(25);m.raw[34]=std::uint8_t(fire);
  XeenCombatRandom rng(std::vector<Draw>{{1,3,2},{1,3,2},{1,3,2},{1,20,14}});XeenPhysicalPlayerCandidate a(c[0],in[0],m,6,610,false,Warrior::Warrior);finish(a,rng,1);
  const int elemental=fire==100?0:int(10*(100-fire)/100);
  check(a.hit && a.physical==6 && a.elemental==elemental && a.damage==6+elemental && a.elementalFrame==(elemental?1u:0u) && rng.position()==4,"Fire material 5: 8*75/100=6 plus 10*(100-r)/100 once, no extra draw");}
 {auto c=knight(false,5);auto in=in12();XeenCombatRandom rng(std::vector<Draw>{{1,3,2},{1,3,2},{1,3,2},{1,20,13}});
  XeenPhysicalPlayerCandidate a(c[0],in[0],monster(25),6,610,false,Warrior::Warrior);finish(a,rng,1);
  check(!a.hit && a.damage==0 && a.elemental==0 && !a.elementalFrame,"A miss adds no elemental damage");}
 // Every element category reads its own monster resistance byte (fire 34 .. magic 39).
 {const std::uint8_t materials[]{5,9,16,21,26,34};const int base[]{10,2,2,2,2,5};
  for(unsigned e=0;e<6;++e) {auto c=knight(false,materials[e]);auto in=in12();
   for(unsigned other=0;other<6;++other){auto m=monster();for(unsigned r=34;r<40;++r)m.raw[r]=std::uint8_t(r==34+other?100:0);
    XeenCombatRandom rng(std::vector<Draw>{{1,3,2},{1,3,2},{1,3,2},{1,20,14}});XeenPhysicalPlayerCandidate a(c[0],in[0],m,6,610,false,Warrior::Warrior);finish(a,rng,1);
    check(a.hit && a.physical==8 && a.elemental==(other==e?0:base[e]) && a.elementalFrame==(other==e?0u:e+1),"Element category selects its resistance byte and frame");}}}
 // Three Barbarian attempts accumulate physical hits; the elemental term is added once, untripled by Adventurer.
 for(auto difficulty:{Warrior::Warrior,Warrior::Adventurer}) {auto c=knight(false,5);c[0].characterClass=XeenCharacterClass::Barbarian;c[0].permanentLevel=8;auto in=in12();
  std::vector<Draw> tape;for(unsigned n=0;n<3;++n){for(unsigned d=0;d<3;++d)tape.push_back({1,3,1});tape.push_back({1,20,difficulty==Warrior::Warrior?7u:2u});}
  XeenCombatRandom rng(tape);XeenPhysicalPlayerCandidate a(c[0],in[0],monster(),6,610,false,difficulty);finish(a,rng,1);
  const int perHit=difficulty==Warrior::Warrior?5:11;
  check(a.hit && a.physical==3*perHit && a.elemental==10 && a.damage==3*perHit+10 && rng.position()==12,"Elemental once after three accumulated attempts");}
 // Effectiveness: counter 4 (Insect) triples against species 2, including a bad weapon and an equipped missile weapon; two matches give nine.
 {const auto run=[&](std::uint8_t state,unsigned type,bool missileCompanion,std::uint8_t companionState){auto c=knight(false,0,state);
   if(missileCompanion)c[0].weapons[1]={0,30,companionState,4};auto in=in12();
   XeenCombatRandom rng(std::vector<Draw>{{1,3,2},{1,3,2},{1,3,2},{1,20,14}});XeenPhysicalPlayerCandidate a(c[0],in[0],monster(0,type),6,610,false,Warrior::Warrior);finish(a,rng,1);
   check(a.hit && rng.position()==4,"Effectiveness consumes no draw and rolls only the melee weapon");return a.damage;};
  check(run(4,2,false,0)==24 && run(4,1,false,0)==8 && run(0,2,false,0)==8,"Counter 4 triples 8 against Insect (2) only");
  check(run(0x84,2,false,0)==24,"A broken effective weapon still triples");
  check(run(0,2,true,4)==24 && run(4,2,true,4)==72,"An equipped missile weapon's counter triples melee; two matches give x9");
  check(run(1,6,false,0)==24 && run(2,4,false,0)==24 && run(3,5,false,0)==24 && run(5,0,false,0)==24 && run(6,1,false,0)==24 && run(6,0,false,0)==8,"Dragon/Undead/Golem/Monsters/Animal selectors");
  auto c=knight(false,5,4);auto in=in12();XeenCombatRandom rng(std::vector<Draw>{{1,3,2},{1,3,2},{1,3,2},{1,20,14}});
  XeenPhysicalPlayerCandidate a(c[0],in[0],monster(0,2),6,610,false,Warrior::Warrior);finish(a,rng,1);
  check(a.physical==24 && a.elemental==10 && a.damage==34,"Effectiveness multiplies the physical share only");}
 // Shoot: metal and elemental apply, effectiveness does not, the save halves the combined amount.
 {auto c=knight(true,0,4,30);auto in=in12();XeenCombatRandom rng(std::vector<Draw>{{1,2,2},{1,2,2},{1,2,2},{1,20,14},{1,56,56}});
  XeenPhysicalPlayerCandidate a(c[0],in[0],monster(0,2),6,610,true,Warrior::Warrior);finish(a,rng,1);check(a.hit && a.damage==6,"Shoot ignores effectiveness");
  for(unsigned save:{56u,6u}) {auto e=knight(true,5,0,30);XeenCombatRandom tape(std::vector<Draw>{{1,2,2},{1,2,2},{1,2,2},{1,20,14},{1,56,save}});
   XeenPhysicalPlayerCandidate s(e[0],in[0],monster(50),6,610,true,Warrior::Warrior);finish(s,tape,1);
   check(s.hit && s.physical==3 && s.elemental==10 && s.elementalFrame==1 && s.damage==(save==6?6:13) && tape.position()==5,"Shoot: 6*50/100=3 plus 10, save halves 13 to 6");}
  auto z=knight(true,38,0,30);XeenCombatRandom zero(std::vector<Draw>{{1,2,1},{1,2,1},{1,2,1},{1,20,18}});
  XeenPhysicalPlayerCandidate s(z[0],in[0],monster(),6,610,true,Warrior::Warrior);finish(s,zero,1);
  check(s.hit && s.damage==0 && zero.position()==4,"Metal 38 missile: 3-6 clamps to 0, hit at 1-4+18, a zero input draws no save");
  auto h=knight(true,45,0,30);XeenCombatRandom hit(std::vector<Draw>{{1,2,1},{1,2,1},{1,2,1},{1,20,8},{1,56,56}});
  XeenPhysicalPlayerCandidate m(h[0],in[0],monster(),6,610,true,Warrior::Warrior);finish(m,hit,1);check(m.hit && m.damage==13,"Metal 45 missile: 10+3, hit at 1+6+8");
  // The Cursed count lowers the hit total before the d20 chain: Cursed 1 turns the
  // 15 above into a miss; Cursed 30 reaches -15, which the unsigned comparison hits.
  auto one=knight(true,45,0,30);one[0].conditions[unsigned(XeenCondition::Cursed)]=1;
  XeenCombatRandom missTape(std::vector<Draw>{{1,2,1},{1,2,1},{1,2,1},{1,20,8}});
  XeenPhysicalPlayerCandidate missed(one[0],in[0],monster(),6,610,true,Warrior::Warrior);finish(missed,missTape,1);
  check(!missed.hit && missed.damage==0 && missTape.position()==4,"Cursed 1: 1+6-1+8 misses AC5+10");
  auto many=knight(true,45,0,30);many[0].conditions[unsigned(XeenCondition::Cursed)]=30;
  XeenCombatRandom wrapTape(std::vector<Draw>{{1,2,1},{1,2,1},{1,2,1},{1,20,8},{1,56,56}});
  XeenPhysicalPlayerCandidate wrapped(many[0],in[0],monster(),6,610,true,Warrior::Warrior);finish(wrapped,wrapTape,1);
  check(wrapped.hit && wrapped.damage==13 && wrapTape.position()==5,"Cursed 30: -15 is an unsigned word and hits");}
 // The Xeen Slayer Sword (34) bypasses physical resistance; ordinary IDs do not. Material above 130 adds nothing.
 {auto c=knight(false,0,0,34);auto in=in12();XeenCombatRandom rng(std::vector<Draw>{{1,4,1},{1,4,1},{1,4,1},{1,4,1},{1,4,1},{1,4,1},{1,20,14}});
  XeenPhysicalPlayerCandidate a(c[0],in[0],monster(50),6,610,false,Warrior::Warrior);finish(a,rng,1);check(a.hit && a.damage==8,"Slayer Sword ignores 50% physical resistance");
  auto o=knight(false,0,0,29);XeenCombatRandom tape(std::vector<Draw>{{1,7,1},{1,7,1},{1,7,1},{1,20,14}});
  XeenPhysicalPlayerCandidate b(o[0],in[0],monster(50),6,610,false,Warrior::Warrior);finish(b,tape,1);check(b.hit && b.damage==2,"Ordinary weapon keeps 50% resistance");
  auto p=knight(false,200);XeenCombatRandom plain(std::vector<Draw>{{1,3,2},{1,3,2},{1,3,2},{1,20,14}});
  XeenPhysicalPlayerCandidate n(p[0],in[0],monster(),6,610,false,Warrior::Warrior);finish(n,plain,1);check(n.hit && n.damage==8 && !n.elemental,"Material outside the tables contributes nothing");}
 // Cursed/broken elemental and metal weapons keep only their dice.
 {auto c=knight(false,5,0x40);auto in=in12();XeenCombatRandom rng(std::vector<Draw>{{1,3,2},{1,3,2},{1,3,2},{1,20,14}});
  XeenPhysicalPlayerCandidate a(c[0],in[0],monster(),6,610,false,Warrior::Warrior);finish(a,rng,1);check(a.hit && a.damage==8 && !a.elemental,"Cursed elemental weapon skips its material");
  auto b=knight(false,45,0x80);XeenCombatRandom tape(std::vector<Draw>{{1,3,2},{1,3,2},{1,3,2},{1,20,14}});
  XeenPhysicalPlayerCandidate m(b[0],in[0],monster(),6,610,false,Warrior::Warrior);finish(m,tape,1);check(m.hit && m.damage==8,"Broken metal weapon skips hit and damage modifiers");}
 // A monster resistance byte above 100 is refused before any damage is published.
 {auto c=knight(false,5);auto in=in12();auto m=monster();m.raw[34]=101;XeenCombatRandom rng(std::vector<Draw>{{1,3,2},{1,3,2},{1,3,2},{1,20,14}});
  XeenPhysicalPlayerCandidate a(c[0],in[0],m,6,610,false,Warrior::Warrior);rejects([&]{finish(a,rng,1);});}
}
void rangedOpportunity() {
	// Artificial multi-shot capacity/continuation control, not a route witness.
	XeenMap map;map.geometry.id=23;map.geometry.flags2=0x8000;
	map.geometry.surfaceTypes[2]=1;
	for (auto &cell:map.geometry.cells) cell.geometry=XeenOutdoorLayers{2,0,0,0};
	XeenCamera camera{23,8,8,XeenDirection::North};
	std::vector<XeenActor> actors;
	const std::array<std::pair<int,int>,4> origins{{{8,9},{9,8},{8,7},{7,8}}};
	for (unsigned n=0;n<12;++n) {
		XeenActor a;a.id={23,n};a.original.resourceId=6;
		a.x=origins[n/3].first;a.y=origins[n/3].second;a.original.x=a.x;a.original.y=a.y;
		a.hp=25;a.activated=true;a.lifecycle=XeenActorLifecycle::Present;
		a.statistics=profile();a.statistics->raw[20]=25;a.statistics->raw[32]=1;
		actors.push_back(a);
	}
	auto c=characters();c[0].conditions[12]=1;c[0].currentHp=0;
	for (unsigned n=1;n<6;++n) c[n].currentHp=30000;
	const auto i=inputs();
	XeenRegionalOpportunityCandidate opportunity(map,actors,camera,c,i,610,0x3f);
	check(opportunity.shotCount==12,"Every activated Orc queues once before moving, including joining sources");
	std::vector<XeenCombatRandom::Draw> tape;
	for (unsigned n=0;n<12;++n) {
		tape.push_back({1,6,1});tape.push_back({0,4,0});tape.push_back({0,4,0});tape.push_back({1,20,20});
		tape.push_back({1,10,1});tape.push_back({1,5,5});tape.push_back({1,10,1});
	}
	XeenCombatRandom rng(tape);XeenConsequenceDraw first{rng};
	check(!opportunity.service(first) && rng.position()==64,"Whole opportunity yields at sixty-four raw draws");
	check(c[1].currentHp==30000 && actors[0].x==8 && actors[0].y==9,"Unpublished sources remain unchanged");
	XeenConsequenceDraw next{rng};check(opportunity.service(next) && rng.position()==84,"Retained opportunity resumes without repeated draws");
	check(opportunity.characters[1].currentHp==29976,"All twenty-four critical damage applications retained");
	for (unsigned n=0;n<12;++n) check(opportunity.shots[n].attack.injuryCount==2,"No shot observation is truncated");
 XeenRegionalOpportunityCandidate escaped(map,actors,camera,c,i,610,0);
 XeenCombatRandom empty(std::vector<XeenCombatRandom::Draw>{});finish(escaped,empty,1);
 check(escaped.shotCount==12 && empty.position()==0,"Empty participant view retains all owed shot bookkeeping without target draws");
 for(unsigned n=0;n<12;++n)check(escaped.shots[n].source==opportunity.shots[n].source && escaped.shots[n].attack.attackOutcome==XeenCombatAttackOutcome::NoParticipants && !escaped.shots[n].attack.injuryCount,"Every owed empty-view shot retains source and explicit no-participant observation");
 XeenRegionalOpportunityCandidate singleton(map,actors,camera,c,i,610,1u<<5);
 std::vector<XeenCombatRandom::Draw> singles;
 for(unsigned n=0;n<12;++n)singles.push_back({1,20,1});
 XeenCombatRandom singleTape(singles);finish(singleton,singleTape,1);
 for(unsigned n=0;n<12;++n)check(singleton.shots[n].attack.targetOwner==c[5].rosterId,"Every owed shot targets the remaining stable owner");
 // One ranged source runs all original attacks, including whole-party passes.
 auto group=characters();for(auto &member:group)member.conditions[8]=1;
 auto multiActors=std::vector<XeenActor>{actors.front()};
 multiActors[0].statistics->raw[24]=2;multiActors[0].statistics->raw[25]=15;
 XeenRegionalOpportunityCandidate multi(map,multiActors,camera,group,i,610,63);
 std::vector<XeenCombatRandom::Draw> multiTape(6,{1,10,1});
 for(unsigned target=0;target<6;++target)multiTape.insert(multiTape.end(),{{1,20,20},{1,10,1},{1,5,5},{1,10,1}});
 XeenCombatRandom multiRng(multiTape);finish(multi,multiRng,1);
 const auto &result=multi.shots[0].attack;
 check(multi.shotCount==1 && result.injuryCount==18 && result.additionalInjuries && result.critical &&
       result.damage==18 && multiRng.position()==30,"Ranged source multiplicity, criticals or overflow observations lost");
 for(unsigned injury=0;injury<18;++injury)check(result.injury(injury).attackOrdinal==(injury<6?0u:1u),
  "Ranged attack ordinals did not survive source aggregation");
 multiActors[0].statistics->raw[24]=0;
 XeenRegionalOpportunityCandidate zero(map,multiActors,camera,group,i,610,63);
 XeenCombatRandom noAttacks(std::vector<XeenCombatRandom::Draw>{});finish(zero,noAttacks,1);
 check(zero.shotCount==1 && !zero.shots[0].attack.injuryCount && !noAttacks.position(),
  "Zero-attack ranged source invented an injury or RNG draw");
}
// M81 Part C: record-driven monster treasure. Fixed gold and gems precede the
// single chance draw of every nonzero drop field, which is also makeItem's level.
// Mutations caught: an Orc-only gate, no draw at 100%, a wrong threshold, a
// level-1 generator, currency after the chance draw, Arena loot or currency,
// and fields above 6 accepted.
void recordDrops() {
 using D=XeenCombatRandom::Draw;using Outcome=XeenMonsterDropOutcome;
 const auto record=[](unsigned gold,unsigned gems,unsigned field){
  XeenMonsterRecord r;r.raw[42]=std::uint8_t(gold);r.raw[43]=std::uint8_t(gold>>8);r.raw[44]=std::uint8_t(gems);r.raw[45]=std::uint8_t(field);return r;};
 const XeenMapIdentity map(30);
 XeenCombatRandom none(std::vector<D>{});
 XeenMonsterDropCandidate quiet(XeenTreasureStaging{},record(300,7,0),map);finish(quiet,none,1);
 check(!none.position() && quiet.outcome==Outcome::None && quiet.treasure.gold==300 && quiet.treasure.gems==7 &&
  !quiet.treasure.ready && quiet.treasure.pending(),"Field zero stages its currency without a draw");
 constexpr unsigned chance[]{0,10,20,50,100,100,100};
 for(unsigned field=1;field<=3;++field) {
  XeenCombatRandom miss(std::vector<D>{{1,100,chance[field]+1}});
  XeenMonsterDropCandidate drop(XeenTreasureStaging{},record(0,0,field),map);finish(drop,miss,1);
  check(miss.position()==1 && drop.outcome==Outcome::None && !drop.treasure.pending(),"A draw above the field threshold stages nothing");
 }
 // Threshold 20 at field 2: a level-2 accessory (category 61, subcategory 30)
 // with an attribute enchantment, six draws in all.
 const auto attribute=merchant::attributes[0][0];
 const std::vector<D> ringTape{{1,100,20},{1,100,61},{1,100,30},{1,100,61},{1,100,1},{attribute.lo,attribute.hi,attribute.lo}};
 XeenCombatRandom ring(ringTape);
 XeenMonsterDropCandidate accessory(XeenTreasureStaging{},record(40,2,2),map);finish(accessory,ring,1);
 check(ring.position()==6 && accessory.outcome==Outcome::Item && accessory.category==XeenRewardCategory::Accessories &&
  accessory.generated.id==1 && accessory.generated.material==attribute.lo+58 && accessory.treasure.ready &&
  xeenSameItem(accessory.treasure.accessories[0],accessory.generated) && accessory.treasure.gold==40 && accessory.treasure.gems==2,
  "Field 2 accessory at the threshold");
 XeenTreasureStaging fullAccessories;for(auto &v:fullAccessories.accessories)v={0,3,0,0};
 XeenCombatRandom lostRing(ringTape);
 XeenMonsterDropCandidate lost(fullAccessories,record(40,2,2),map);finish(lost,lostRing,1);
 check(lostRing.position()==6 && lost.outcome==Outcome::CategoryCapacityLoss && !lost.treasure.ready && lost.treasure.gems==2,
  "A full accessory array loses the item after all its draws");
 // Darzog's field 6: 50 gems, one draw even at 100%, a level-6 weapon whose
 // subcategory draw is R(1,80) (eight draws with the effectiveness roll).
 const auto material=merchant::material[0][4];
 XeenCombatRandom sword(std::vector<D>{{1,100,100},{1,100,1},{1,80,1},{1,6,6},{1,100,1},{1,100,1},{material.lo,material.hi,material.lo},{1,20,10},{1,6,3}});
 XeenTreasureStaging gems;gems.gems=std::numeric_limits<std::uint32_t>::max()-49;
 XeenMonsterDropCandidate darzog(gems,record(0,50,6),map);finish(darzog,sword,1);
 check(sword.position()==9 && darzog.outcome==Outcome::Item && darzog.category==XeenRewardCategory::Weapons &&
  darzog.generated.id==6 && darzog.generated.material==material.lo+36 && darzog.generated.state==3 &&
  darzog.treasure.gems==0 && !darzog.treasure.gold,"Field 6 draws once at 100% and wraps 32-bit gems");
 // Each level matches an independent shared generator at that level, after
 // exactly one chance draw; the reachable categories follow the level.
 for(unsigned field=1;field<=6;++field) {
  std::array<bool,4> seen{};
  for(std::uint32_t seed=1;seed<=4000;++seed) {
   XeenCombatRandom live(seed),oracle(seed);
   XeenMonsterDropCandidate drop(XeenTreasureStaging{},record(1,1,field),map);finish(drop,live);
   XeenConsequenceDraw draw{oracle,64,{}};const auto roll=*draw.draw(1,100);
   std::optional<XeenItemCandidate> item;if(roll<=chance[field]){item.emplace(field);finish(*item,oracle);}
   check(live.continuation()==oracle.continuation() && (item?drop.outcome==Outcome::Item && xeenSameItem(drop.generated,item->item()) &&
    unsigned(drop.category)==item->category():drop.outcome==Outcome::None),"Drop differs from one chance draw and makeItem at the field level");
   if(item)seen[item->category()]=true;
  }
  check(seen[0] && seen[1] && seen[2]==(field>1) && seen[3]==(field<6),"Reachable treasure categories by level");
 }
 // Map 85 leaves after XP: no currency and no draw; earlier items stay.
 XeenTreasureStaging dormant;dormant.armor[0]={0,2,0,0};
 XeenCombatRandom arena(std::vector<D>{});
 XeenMonsterDropCandidate guarded(dormant,record(300,7,4),XeenMapIdentity(85));finish(guarded,arena,1);
 check(!arena.position() && guarded.outcome==Outcome::None && guarded.treasure==dormant,"Arena kill staged treasure or drew");
 XeenCombatRandom zeroChance(std::vector<D>{{1,100,1}});
 XeenMonsterDropCandidate fieldSeven(XeenTreasureStaging{},record(0,0,7),XeenMapIdentity(XeenSide::Darkside,35));
 finish(fieldSeven,zeroChance,1);
 check(zeroChance.position()==1 && fieldSeven.outcome==Outcome::None && !fieldSeven.treasure.pending(),"Field 7 draws once and drops nothing");
 // Fields above 7 are refused before any draw.
 for(unsigned field:{8u,17u,21u,255u}) {
  const auto bad=record(10,0,field);
  rejects([&]{(void)bad.dropLevel();});rejects([&]{bad.validateAttackCapabilities();});
  rejects([&]{XeenMonsterDropCandidate refused(XeenTreasureStaging{},bad,map);});
  rejects([&]{XeenMonsterDropCandidate refused(XeenTreasureStaging{},bad,XeenMapIdentity(85));});
 }
 for(unsigned field=0;field<=7;++field){const auto good=record(0,0,field);check(good.dropLevel()==field,"Drop level accessor");good.validateAttackCapabilities();}
}
// M81 Part C: WhoWill (DOS 47c9:0008) lives in the staging value. Mutations
// caught: Run, Load or New Game clearing it, equality ignoring it, a delivery
// without the preference or an off-by-one member.
void whoWillStaging() {
 XeenTreasureStaging staged;check(!staged.preferredRecipient(),"Zero WhoWill prefers nobody");
 staged.whoWill=3;check(staged.preferredRecipient()==std::optional<std::size_t>(2) && staged!=XeenTreasureStaging{},"WhoWill is one-based and part of the value");
 auto run=staged;run.gold=5;run.ready=true;run.clearPending();check(run.whoWill==3,"Run cleared WhoWill");
 XeenTreasureStagingState owner;owner.publish(staged);owner.enterGame();check(owner.value().whoWill==3,"Load or New Game cleared WhoWill");
 XeenTreasureStaging loot;loot.gold=1;loot.insert(XeenRewardCategory::Armor,{0,2,0,0});
 const auto c=characters();
 for(unsigned whoWill:{0u,3u,7u}) {
  loot.whoWill=std::uint8_t(whoWill);const auto d=deliverAll(loot,c);
  check(d.records[0].recipient==c[whoWill==3?2:0].rosterId,"Delivery ignored or misread WhoWill");
 }
}
void dormantTreasure() {
 // Artificial semantic fixture: no production witness or actor accounting claim.
 XeenTreasureStaging ready;ready.gold=10;ready.ready=true;ready.weapons[0]={0,30,0,0};
 auto dormant=ready;dormant.clearPending();
 check(dormant.dormant() && !dormant.pending() && dormant.storedItems() && xeenSameItem(dormant.weapons[0],ready.weapons[0]),"Run clears currency and readiness, keeping items");
 rejects([&]{deliverAll(dormant,characters());});
 auto again=dormant;again.clearPending();check(again==dormant,"Repeated forfeiture is idempotent");
 XeenMonsterDropCandidate later(dormant,orc(),XeenMapIdentity(23));
 XeenCombatRandom tape(std::vector<XeenCombatRandom::Draw>{{1,100,11}});finish(later,tape,1);
 check(later.treasure.pending() && !later.treasure.ready && later.treasure.gold==10 && xeenSameItem(later.treasure.weapons[0],dormant.weapons[0]),"No-item Orc reactivates older items without old gold");
 const auto delivered=deliverAll(later.treasure,characters());
 check(delivered.count==1 && xeenSameItem(delivered.records[0].item,dormant.weapons[0]) && !delivered.treasure.storedItems(),"Dormant item delivers in original order");
 XeenPurse purse;purse.gold=800;purse.gems=10;
 const auto credited=creditAll(purse,delivered.treasure);
 check(credited.purse.gold==810 && credited.purse.gems==10 && !credited.treasure.pending(),"Only later Orc gold credits");
}
// M81 Part A: the running program's staging value. Four ten-slot category
// arrays without contributor identity, readiness only on insertion, 32-bit
// currency that wraps, and one delivery of up to forty records in category order.
void stagingState() {
 // Mutations caught: a twelve-source limit, a shared or bounded-by-twelve item
 // store, readiness without insertion, saturating sums and a missing category.
 XeenTreasureStaging staged;
 const auto weapon=[](unsigned id){return std::vector<XeenCombatRandom::Draw>{{1,100,10},{1,100,40},{1,100,30},{1,6,id},{1,100,100}};};
 for(unsigned kill=0;kill<25;++kill) {
  XeenCombatRandom rng(kill<12?weapon(kill%6+1):std::vector<XeenCombatRandom::Draw>{{1,100,11}});
  XeenMonsterDropCandidate drop(staged,orc(),XeenMapIdentity(23));finish(drop,rng,1);staged=drop.treasure;
  check(drop.outcome==(kill<10?XeenMonsterDropOutcome::Item:kill<12?XeenMonsterDropOutcome::CategoryCapacityLoss:XeenMonsterDropOutcome::None),
   "Weapon staging uses ten first-empty slots, then loses items");
 }
 check(staged.gold==250 && staged.ready && staged.weapons[9].id==4 && !staged.armor[0].id,"Twenty-five kills stage every gold amount and ten weapons");
 XeenTreasureStaging quiet;XeenCombatRandom none(std::vector<XeenCombatRandom::Draw>{{1,100,11}});
 XeenMonsterDropCandidate miss(quiet,orc(),XeenMapIdentity(23));finish(miss,none,1);
 check(miss.treasure.gold==10 && !miss.treasure.ready && miss.treasure.pending(),"Readiness is set only on insertion; gold alone is pending");
 // Four categories, ten slots each: a forty-record delivery in category/slot order.
 XeenTreasureStaging full;full.gold=std::numeric_limits<std::uint32_t>::max();full.gems=5;full.ready=true;
 for(unsigned slot=0;slot<10;++slot) {
  full.weapons[slot]={0,std::uint8_t(slot+1),0,0};full.armor[slot]={0,std::uint8_t(slot%7+1),0,0};
  full.accessories[slot]={0,std::uint8_t(slot+1),0,0};full.miscellaneous[slot]={std::uint8_t(slot%9+1),1,3,0};
 }
 check(&full.items(XeenRewardCategory::Accessories)==&full.accessories,"Accessory array is distinct");
 auto c=characters();for(auto &v:c){v.weapons={};v.armor={};v.accessories={};v.miscellaneous={};}
 const auto d=deliverAll(full,c);
 check(d.count==40 && !d.treasure.storedItems() && d.treasure.gold==full.gold,"Forty records deliver from four arrays");
 for(unsigned i=0;i<40;++i)check(d.records[i].category==static_cast<XeenRewardCategory>(i/10) &&
  xeenSameItem(d.records[i].item,full.items(static_cast<XeenRewardCategory>(i/10))[i%10]),"Delivery follows category then slot order");
 unsigned delivered=0;for(unsigned i=0;i<40;++i)delivered+=bool(d.records[i].recipient);
 check(delivered==40,"Six empty packs accept forty items");
 // Purse credit wraps both currencies modulo 2^32 and clears readiness.
 XeenPurse purse;purse.gold=11;purse.gems=std::numeric_limits<std::uint32_t>::max();
 const auto credit=creditAll(purse,d.treasure);
 check(credit.purse.gold==10 && credit.purse.gems==4 && credit.treasure==XeenTreasureStaging{},"Purse credit did not wrap modulo 2^32 or clear the staging");
 // A full category loses an Event-independent drop after generation.
 XeenTreasureStaging armorFull;for(auto &v:armorFull.armor)v={0,2,0,0};
 XeenCombatRandom lost(std::vector<XeenCombatRandom::Draw>{{1,100,10},{1,100,41},{1,100,100},{1,7,7},{1,100,1}});
 XeenMonsterDropCandidate overflow(armorFull,orc(),XeenMapIdentity(23));finish(overflow,lost,1);
 check(overflow.outcome==XeenMonsterDropOutcome::CategoryCapacityLoss && !overflow.treasure.ready && lost.position()==5,"Full armor category consumed its draws and set no readiness");
 // Revisioned owner: stale prepared work is refused, entry keeps the arrays.
 XeenTreasureStagingState owner;owner.publish(full);
 XeenMonsterDropCandidate prepared(owner,orc(),XeenMapIdentity(23));owner.publish(full);
 rejects([&]{owner.check(prepared.base);});
 const auto before=owner.save();owner.enterGame();
 check(owner.value().gold==0 && owner.value().gems==0 && !owner.value().ready && owner.value().storedItems() &&
  xeenSameItem(owner.value().miscellaneous[9],full.miscellaneous[9]),"Entry clears currency and readiness only");
 owner.restore(before);check(owner.value()==full && owner.revision()==before.revision,"Failed entry restores the exact owner");
}
// Danger Sense is the movement pass's own result (0x1fdc1): set for a monster
// within three cells whatever its state, absent when the pass returns early.
void dangerSenseOpportunity() {
 XeenMap map;map.geometry.id=23;map.geometry.flags2=0x8000;map.geometry.surfaceTypes[2]=1;
 for(auto &cell:map.geometry.cells)cell.geometry=XeenOutdoorLayers{2,0,0,0};
 const XeenCamera camera{23,8,8,XeenDirection::North};
 const auto c=characters();const auto i=inputs();
 const auto at=[&](int x,int y) {
  XeenActor a;a.id={23,0};a.original.resourceId=6;a.x=x;a.y=y;a.original.x=x;a.original.y=y;a.hp=25;
  a.lifecycle=XeenActorLifecycle::Present;a.activated=false;a.statistics=profile();a.statistics->raw[20]=25;
  return std::vector<XeenActor>{a};
 };
 XeenRegionalOpportunityCandidate near(map,at(11,8),camera,c,i,610,63);
 check(near.dangerSense==true,"Unaware monster three cells away not sensed by the movement pass");
 XeenRegionalOpportunityCandidate far(map,at(8,12),camera,c,i,610,63);
 check(far.dangerSense==false,"Monster four cells away sensed by the movement pass");
 XeenRegionalOpportunityCandidate sleeping(map,at(5,5),camera,c,i,610,63,{},XeenActorOpportunityContext{true});
 check(sleeping.dangerSense==true,"Rest's movement pass did not sense a monster");
 XeenRegionalOpportunityCandidate disabled(map,at(9,9),camera,c,i,610,63,{},XeenActorOpportunityContext{false,false});
 XeenRegionalOpportunityCandidate shooting(map,at(9,9),camera,c,i,610,63,{},XeenActorOpportunityContext{false,true,true});
 check(!disabled.dangerSense && !shooting.dangerSense,"An early-returning movement pass changed Danger Sense");
}
void indoorOpportunity() {
 XeenWorld world([](auto id) {XeenMap m;m.geometry.id=id.number;m.geometry.difficulties[0]=7;
  for(auto &cell:m.geometry.cells)cell.geometry=XeenIndoorWalls{};return m;});
 const XeenCamera camera{33,8,8,XeenDirection::North};
 auto c=characters();auto in=inputs();c[0].characterClass=XeenCharacterClass::Cleric;
 XeenActor a;a.id={33,0};a.original.resourceId=42;a.x=8;a.y=11;a.hp=25;
 a.lifecycle=XeenActorLifecycle::Present;a.activated=true;a.statistics=profile(3);
 a.statistics->raw[20]=25;a.statistics->raw[32]=1;
 const std::vector<XeenActor> before{a};
 XeenRegionalOpportunityCandidate op(world,before,camera,c,in,610,63);op.staged=true;
 check(op.shotCount==1 && op.shots[0].distance==3 && op.shots[0].direction==XeenDirection::North &&
  op.actors[0].y==10 && before[0].y==11,"Indoor supported ranged source schedules before detached movement");
 XeenCombatRandom rng(std::vector<XeenCombatRandom::Draw>{{1,20,19},{1,5,5},{1,10,3}});
 XeenConsequenceDraw draw{rng};
 check(!op.service(draw) && op.travelStarted && !rng.position() && op.characters[0].currentHp==50,
  "Indoor ranged travel must precede attack RNG and injury");
 op.travelPresented=true;
 check(!op.service(draw) && op.impactOwner==c[0].rosterId && rng.position()==3 &&
  op.characters[0].currentHp==50,"Indoor enemy hit must wait on live-target portrait acknowledgment");
 check(!op.service(draw) && rng.position()==3 && op.characters[0].currentHp==50,
  "Unacknowledged enemy impact repeated draws or injury");
 op.impactPresented=true;
 check(!op.service(draw) && op.impactApplied && op.characters[0].currentHp==47,
  "Indoor enemy impact did not publish prepared HP after acknowledgment");
 check(op.service(draw) && rng.position()==3 && op.shots[0].attack.damage==3,
  "Indoor enemy settled injury was repeated or lost");
 auto bad=before;bad[0].statistics->raw[29]=7;
 rejects([&]{XeenRegionalOpportunityCandidate unsupported(world,bad,camera,c,in,610,63);});
 check(before[0].y==11 && c[0].currentHp==50 && rng.position()==3,
  "Unsupported indoor enemy attack mutated inputs or consumed RNG");
}
// DOS 0x1ff27..0x1ff98: a ranged monster on the party's own cell may fire
// unless it holds one of the three front slots. With four stacked ranged
// monsters there, only the hidden fourth queues a shot, indoors and outdoors.
void stackedRangedOpportunity() {
 auto c=characters();const auto in=inputs();
 auto stacked=[](XeenMapIdentity map,int x,int y,unsigned resource,const XeenMonsterRecord &record) {
  std::vector<XeenActor> actors;
  for(unsigned n=0;n<4;++n) {
   XeenActor a;a.id={map,n};a.original.resourceId=resource;a.x=x;a.y=y;a.original.x=x;a.original.y=y;
   a.hp=25;a.activated=true;a.lifecycle=XeenActorLifecycle::Present;
   a.statistics=record;a.statistics->raw[20]=25;a.statistics->raw[32]=1;
   actors.push_back(a);
  }
  return actors;
 };
 XeenMap map;map.geometry.id=23;map.geometry.flags2=0x8000;map.geometry.surfaceTypes[2]=1;
 for(auto &cell:map.geometry.cells)cell.geometry=XeenOutdoorLayers{2,0,0,0};
 const XeenCamera outdoorCamera{23,8,8,XeenDirection::North};
 const auto outdoorActors=stacked(23,8,8,6,profile());
 XeenRegionalOpportunityCandidate outdoor(map,outdoorActors,outdoorCamera,c,in,610,0x3f);
 check(outdoor.shotCount==1 && outdoor.shots[0].source==XeenMonsterIdentity{23,3} && outdoor.shots[0].distance==0,
  "Outdoor hidden fourth ranged monster on the party cell did not fire, or a front-slot monster fired");
 XeenWorld world([](auto id) {XeenMap m;m.geometry.id=id.number;m.geometry.difficulties[0]=7;
  for(auto &cell:m.geometry.cells)cell.geometry=XeenIndoorWalls{};return m;});
 const XeenCamera indoorCamera{33,8,8,XeenDirection::North};
 const auto indoorActors=stacked(33,8,8,42,profile(3));
 XeenRegionalOpportunityCandidate indoor(world,indoorActors,indoorCamera,c,in,610,63);
 check(indoor.shotCount==1 && indoor.shots[0].source==XeenMonsterIdentity{33,3} && indoor.shots[0].distance==0,
  "Indoor hidden fourth ranged monster on the party cell did not fire, or a front-slot monster fired");
}
void timeAndInputs() {
	auto c=characters();auto i=inputs();XeenGameplayContext context;context.year=610;context.day=8;context.minutes=950;
	std::vector<XeenCombatRandom::Draw> tape;
	XeenCombatRandom tickTape(tape);XeenConditionTimeCandidate tick(context,10,c,i);finish(tick,tickTape,1);
	check(tick.context.minutes==960 && tickTape.position()==0 && tick.characters[0].currentHp==50,"Healthy tick consumes no RNG");
	for (auto &owner:c) { owner.conditions[2]=1;owner.conditions[8]=1; }
	i[0].might.permanent=1;
	XeenCombatRandom noDraws(std::vector<XeenCombatRandom::Draw>{});XeenConditionTimeCandidate death(context,10,c,i);finish(death,noDraws);
	check(death.characters[0].conditions[13]==2 && death.characters[0].currentHp==50 && death.characters[0].conditions[8]==0 && noDraws.position()==0,"Stat death precedes Weak replacement and retains HP; the block clears Sleep");
	context.minutes=1250;XeenConditionTimeCandidate dusk(context,10,c,i);finish(dusk,noDraws);check(dusk.context.minutes==1260,"Dusk is playable");
	std::vector<std::uint8_t> chr(30*354);chr[313]=7;chr[314]=2;chr[315]=9;chr[316]=3;
	check(!XeenCharacterFormat::parseCombatInputs(chr,0,true).resistances,"Legacy optional absence");
	const auto r=XeenCharacterFormat::parseCombatInputs(chr,0,true,true).resistances;
	check(r && r->coldPermanent==7 && r->coldTemporary==2 && r->electricalPermanent==9 && r->electricalTemporary==3,"Four exact resistance inputs");
	std::vector<std::uint8_t> pty(646);pty[638]=0x20;pty[639]=3;pty[642]=10;
	const auto purse=XeenCharacterFormat::parseMonsterPurse(pty);check(purse.gold==800 && purse.gems==10,"Resource purse offsets");
	pty.pop_back();rejects([&] { XeenCharacterFormat::parseMonsterPurse(pty); });
}
}
#include "XeenDailyTimeOracles.h"
int main() { try { partyVolley();poisonVolley();unsignedPlayerHit();dailyTimeOracles();dailyConditionChecks();difficultyOracles();dormantTreasure();stagingState();recordDrops();whoWillStaging();physical();targetingAndCounts();runAndParticipation();shootAndLoot();additionalTapes();completeWeaponRules();equippedContributions();missileClassAndZeroDamage();dosMonsterDelivery();rejectionBudgets();rangedOpportunity();dangerSenseOpportunity();indoorOpportunity();stackedRangedOpportunity();timeAndInputs();std::cout<<"Shared consequence pure-rule controls passed\n";return 0; } catch (const std::exception &e) { std::cerr<<e.what()<<'\n';return 1; } }
