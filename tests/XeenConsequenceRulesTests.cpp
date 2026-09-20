#include "games/xeen/XeenCombatRules.h"
#include "games/xeen/XeenRegionalRules.h"
#include "games/xeen/XeenCharacterRules.h"
#include "formats/xeen/XeenCharacterFormat.h"
#include <iostream>
#include <limits>
#include <stdexcept>
using namespace mmodern;
namespace {
void check(bool v,const char *m) { if (!v) throw std::runtime_error(m); }
template<class F> void rejects(F f) { bool refused=false;try { f(); } catch (const std::exception &) { refused=true; } check(refused,"Expected refusal"); }
// Explicit artificial rule fixtures; these are not original-resource journeys.
XeenConsequenceCharacters characters() {
	XeenConsequenceCharacters result;
	for (unsigned i=0;i<6;++i) { auto &c=result[i];c.rosterId=kXeenCombatOwners[i];c.permanentLevel=3;c.birthYear=592;c.currentHp=50;c.intellect=c.personality=c.endurance={15,0}; }
	return result;
}
XeenConsequenceInputs inputs() {
	XeenConsequenceInputs result;
	for (auto &i:result) { i.might=i.speed=i.accuracy={15,0};i.luck=XeenAttributeValue{15,0};i.resistances=XeenCombatResistances{}; }
	return result;
}
XeenMonsterRecord profile(unsigned hates=1,unsigned special=0) {
	XeenMonsterRecord m;m.raw[22]=5;m.raw[25]=hates;m.raw[26]=1;m.raw[28]=10;m.raw[30]=special;m.raw[31]=5;return m;
}
template<class C> void finish(C &c,XeenCombatRandom &rng,unsigned budget=64) {
	for (unsigned service=0;service<1000;++service) { XeenConsequenceDraw draw{rng,budget,{}};if (c.service(draw)) return; }
	throw std::runtime_error("Candidate did not complete bounded service");
}
void physical() {
	auto c=characters();auto i=inputs();
	c[0].characterClass=XeenCharacterClass::Paladin;
	XeenCombatRandom miss(std::vector<XeenCombatRandom::Draw>{{0,5,1},{1,20,1}});
	XeenEnemyAttackCandidate noPreference(c,i,profile(),610);finish(noPreference,miss,1);
	check(noPreference.result.targetOwner==c[1].rosterId && noPreference.result.injuryCount==0 && miss.position()==2,"Hates one is random and natural one ends the attack");
	c[0].characterClass=XeenCharacterClass::Cleric;c[0].conditions[8]=1;
	XeenCombatRandom asleep(std::vector<XeenCombatRandom::Draw>{{1,10,4}});
	XeenEnemyAttackCandidate preferred(c,i,profile(3),610);finish(preferred,asleep,1);
	check(preferred.characters[0].currentHp==46 && !preferred.characters[0].conditions[8] && asleep.position()==1,"Sleeping Cleric remains preferred and skips hit draws");
	for (auto &owner:c) owner.conditions[8]=1;
	c[5].conditions[13]=1;
	std::vector<XeenCombatRandom::Draw> tape;
	for (unsigned n=0;n<6;++n) { tape.push_back({1,10,2});tape.push_back({1,25,25}); }
	XeenCombatRandom partyTape(tape);XeenEnemyAttackCandidate all(c,i,profile(16,9),610);finish(all,partyTape,1);
	check(all.result.injuryCount==6 && partyTape.position()==12,"Hates party includes dead targets without selection draws");
	for (const auto &owner:all.characters) check(owner.currentHp==48 && owner.conditions[8]==1,"Wake precedes Sleep reapplication");
	c=characters();c[0].characterClass=XeenCharacterClass::Cleric;
	// Noncritical total 15 reaches ordinary AC(2)+10 but not blocked AC+16.
	std::array<bool,6> blocked{};blocked[0]=true;
	XeenCombatRandom blockTape(std::vector<XeenCombatRandom::Draw>{{1,20,10},{1,5,4}});
	XeenEnemyAttackCandidate block(c,i,profile(3),610,blocked);finish(block,blockTape);
	check(block.result.injuryCount==0,"Block threshold");
	XeenCombatRandom ordinaryTape(std::vector<XeenCombatRandom::Draw>{{1,20,10},{1,5,4},{1,10,3}});
	XeenEnemyAttackCandidate ordinary(c,i,profile(3),610);finish(ordinary,ordinaryTape);
	check(ordinary.characters[0].currentHp==47,"Ordinary threshold");
	c[0].conditions[3]=255;c[0].conditions[8]=1;
	XeenCombatRandom overflowTape(std::vector<XeenCombatRandom::Draw>{{1,10,1},{1,25,25}});
	XeenEnemyAttackCandidate overflow(c,i,profile(3,5),610);rejects([&] { finish(overflow,overflowTape); });
	check(c[0].currentHp==50 && c[0].conditions[3]==255,"Pure overflow does not mutate source");
}
void shootAndLoot() {
	auto c=characters();auto i=inputs();c[0].weapons[0]={0,30,0,4};i[0].accuracy={18,0};
	auto m=profile();m.raw[40]=0;
	XeenCombatRandom seed(3);XeenPhysicalPlayerCandidate shot(c[0],i[0],m,6,610,true);finish(shot,seed,1);
	check(shot.hit && shot.damage==9 && seed.position()==5,"Seed three wound arithmetic");
	XeenCombatRandom lootTape(std::vector<XeenCombatRandom::Draw>{{1,100,8},{0,100,48},{0,100,50},{1,7,2},{1,100,46}});
	XeenMonsterTreasure purse;purse.gold=800;purse.gems=10;
	XeenMonsterDropCandidate loot(purse,9);finish(loot,lootTape,1);
	check(loot.outcome==XeenMonsterDropOutcome::Item && loot.armor && loot.treasure.armor[0].item.id==2 && loot.treasure.pendingGold==10,"Armor production");
	auto delivery=xeenPrepareMonsterDelivery(loot.treasure,c);
	check(delivery.records[0].recipient==c[0].rosterId && delivery.characters[0].armor[0].id==2 && delivery.treasure.gold==800 && delivery.treasure.pendingGold==10,"Items before receipt, gold still pending");
	const auto paid=xeenPrepareMonsterGoldCredit(delivery.treasure);
	check(paid.gold==810 && paid.gems==10 && !paid.pendingMask,"Final credit");
	XeenCombatRandom wandTape(std::vector<XeenCombatRandom::Draw>{{1,100,10},{0,100,86},{0,100,0},{1,9,3},{1,100,1},{1,15,8},{1,8,7}});
	XeenMonsterDropCandidate wand(purse,0);finish(wand,wandTape,1);
	check(wand.outcome==XeenMonsterDropOutcome::ReferenceMiscellaneousDropLoss && wandTape.position()==7 && wand.generated.item.material==3 && wand.generated.item.id==8 && wand.generated.item.state==7,"Reference wand loss consumes all draws");
	for (unsigned id=1;id<=34;++id) check(xeenOrdinaryWeaponDice(id).count && xeenOrdinaryWeaponDice(id).sides,"Complete weapon table");
	rejects([] { xeenOrdinaryWeaponDice(35); });
	purse.gold=std::numeric_limits<std::uint32_t>::max()-9;rejects([&] { XeenMonsterDropCandidate invalid(purse,0); });
}
void additionalTapes() {
 auto c=characters();auto i=inputs();
 for(auto &v:c)v.conditions[13]=1;c[2].conditions[13]=0;
 XeenCombatRandom fallback(std::vector<XeenCombatRandom::Draw>{{0,5,0},{0,0,0},{1,20,1}});
 XeenEnemyAttackCandidate single(c,i,profile(),610);finish(single,fallback,1);
 check(single.result.targetOwner==c[2].rosterId && fallback.position()==3,"One surviving fallback still draws [0,0]");
 c=characters();c[0].characterClass=XeenCharacterClass::Cleric;
 XeenCombatRandom critical(std::vector<XeenCombatRandom::Draw>{{1,20,20},{1,10,1},{1,25,25},{1,5,1},{1,10,1},{1,25,5}});
 XeenEnemyAttackCandidate poison(c,i,profile(3,5),610);finish(poison,critical,1);
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
 for(unsigned n=0;n<6;++n){tape.push_back({1,10,1});tape.push_back({1,40,40});tape.push_back({0,9,1});tape.push_back({1,40,1});}
 XeenCombatRandom tick(tape);XeenConditionTimeCandidate minute(context,1,c,i);finish(minute,tick,1);
 check(minute.context.minutes==960 && tick.position()==24,"Round/End crossing and zero-resistance [1,40] branch draws");
 c[0].conditions[13]=255;XeenCombatRandom overflow(tape);XeenConditionTimeCandidate death(context,1,c,i);rejects([&]{finish(death,overflow,1);});check(c[0].conditions[13]==255,"Dead255 tick is unpublished");
 XeenMonsterTreasure purse;purse.gold=800;
 XeenCombatRandom noDrop(std::vector<XeenCombatRandom::Draw>{{1,100,11}});XeenMonsterDropCandidate eleven(purse,0);finish(eleven,noDrop,1);check(eleven.outcome==XeenMonsterDropOutcome::None && noDrop.position()==1,"Drop11 boundary");
 const std::array<unsigned,6> sub{30,31,60,61,85,86};
 const std::array<unsigned,6> lo{1,7,7,18,18,30},hi{6,17,17,29,29,33};
 for(unsigned n=0;n<6;++n){XeenCombatRandom rng(std::vector<XeenCombatRandom::Draw>{{1,100,10},{0,100,40},{0,100,sub[n]},{lo[n],hi[n],lo[n]},{1,100,100}});XeenMonsterDropCandidate drop(purse,0);finish(drop,rng,1);check(drop.generated.item.id==lo[n] && !drop.armor && rng.position()==5,"Weapon subcategory endpoints and ignored enchantment");}
 for(unsigned cat:{41u,85u}){XeenCombatRandom rng(std::vector<XeenCombatRandom::Draw>{{1,100,10},{0,100,cat},{0,100,100},{1,7,7},{1,100,1}});XeenMonsterDropCandidate drop(purse,0);finish(drop,rng,1);check(drop.generated.item.id==7 && drop.armor,"Armor category boundaries");}
 for(unsigned source=0;source<11;++source){XeenCombatRandom rng(std::vector<XeenCombatRandom::Draw>{{1,100,10},{0,100,0},{0,100,0},{1,6,1},{1,100,1}});XeenMonsterDropCandidate drop(purse,source);finish(drop,rng,1);check(rng.position()==5 && drop.outcome==(source==10?XeenMonsterDropOutcome::CategoryCapacityLoss:XeenMonsterDropOutcome::Item),"Eleventh category item lost only after complete generation");purse=drop.treasure;}
 check(purse.pendingGold==110 && purse.weapons[9].source==9,"All lethal sources retain gold despite item overflow");
 c=characters();for(auto &v:c)for(auto *items:{&v.weapons,&v.armor,&v.accessories,&v.miscellaneous})(*items)[8]={0,1,0,0};
 const auto full=xeenPrepareMonsterDelivery(purse,c);check(full.globallyFull && full.count==10 && full.treasure.pendingGold==110 && !full.treasure.weapons[0].item.id,"Global four-tail warning clears item queues, preserves gold");
 for(auto &v:c)v.armor[8]={};c[0].conditions[8]=1;c[1].weapons[8]={};
 const auto delivered=xeenPrepareMonsterDelivery(purse,c);check(!delivered.globallyFull && delivered.records[0].recipient==c[1].rosterId,"First eligible recipient skips sleeping owner");
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
 auto c=characters();XeenMonsterTreasure treasure;treasure.pendingMask=1;treasure.pendingGold=10;treasure.weapons[0]={0,{0,30,0,0}};
 for(auto &v:c)v.weapons[8]={0,1,0,0};
 auto full=xeenPrepareMonsterDelivery(treasure,c);check(!full.globallyFull && full.records[0].loss==XeenMonsterDeliveryLoss::CategoryTailsFull && full.treasure.pendingGold==10,"Category full tails refuse despite holes; gold retained");
 for(auto &v:c){v.weapons={};v.conditions[8]=1;}
 auto disabled=xeenPrepareMonsterDelivery(treasure,c);check(disabled.records[0].loss==XeenMonsterDeliveryLoss::NoEligibleRecipient && disabled.treasure.pendingGold==10,"All disabled recipients lose item without losing gold");
}

void rejectionBudgets() {
 auto c=characters();auto in=inputs();c[0].characterClass=XeenCharacterClass::Cleric;
 std::vector<XeenCombatRandom::Draw> tape(64,{1,20,0,true});tape.push_back({1,20,1});
 XeenCombatRandom random(tape);XeenEnemyAttackCandidate attack(c,in,profile(3),610);XeenConsequenceDraw draw{random,64,{}};
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
	XeenRegionalOpportunityCandidate opportunity(map,actors,camera,c,i,610);
	check(opportunity.shotCount==12,"Every activated Orc queues once before moving, including joining sources");
	std::vector<XeenCombatRandom::Draw> tape;
	for (unsigned n=0;n<12;++n) {
		tape.push_back({0,5,0});tape.push_back({0,4,0});tape.push_back({1,20,20});
		tape.push_back({1,10,1});tape.push_back({1,5,5});tape.push_back({1,10,1});
	}
	XeenCombatRandom rng(tape);XeenConsequenceDraw first{rng};
	check(!opportunity.service(first) && rng.position()==64,"Whole opportunity yields at sixty-four raw draws");
	check(c[1].currentHp==30000 && actors[0].x==8 && actors[0].y==9,"Unpublished sources remain unchanged");
	XeenConsequenceDraw next{rng};check(opportunity.service(next) && rng.position()==72,"Retained opportunity resumes without repeated draws");
	check(opportunity.characters[1].currentHp==29976,"All twenty-four critical damage applications retained");
	for (unsigned n=0;n<12;++n) check(opportunity.shots[n].attack.injuryCount==2,"No shot observation is truncated");
}
void timeAndInputs() {
	auto c=characters();auto i=inputs();XeenGameplayContext context;context.year=610;context.day=8;context.minutes=950;
	std::vector<XeenCombatRandom::Draw> tape;
	for (unsigned n=0;n<6;++n) { tape.push_back({1,10,2});tape.push_back({0,9,0}); }
	XeenCombatRandom tickTape(tape);XeenConditionTimeCandidate tick(context,10,c,i);finish(tick,tickTape,1);
	check(tick.context.minutes==960 && tickTape.position()==12 && tick.characters[0].currentHp==50,"Twelve-draw clean tick");
	for (auto &owner:c) { owner.conditions[3]=1;owner.conditions[4]=1;owner.conditions[8]=1; }
	i[0].might.permanent=1;
	XeenCombatRandom noDraws(std::vector<XeenCombatRandom::Draw>{});XeenConditionTimeCandidate death(context,10,c,i);finish(death,noDraws);
	check(death.characters[0].conditions[13]==2 && death.characters[0].currentHp==50 && death.characters[0].conditions[8]==1 && noDraws.position()==0,"Stat death retains HP and Sleep, nonzero conditions skip draws");
	context.minutes=1250;rejects([&] { XeenConditionTimeCandidate dusk(context,10,c,i); });
	std::vector<std::uint8_t> chr(30*354);chr[313]=7;chr[314]=2;chr[315]=9;chr[316]=3;
	check(!XeenCharacterFormat::parseCombatInputs(chr,0,true).resistances,"Legacy optional absence");
	const auto r=XeenCharacterFormat::parseCombatInputs(chr,0,true,true).resistances;
	check(r && r->coldPermanent==7 && r->coldTemporary==2 && r->electricalPermanent==9 && r->electricalTemporary==3,"Four exact resistance inputs");
	std::vector<std::uint8_t> pty(646);pty[638]=0x20;pty[639]=3;pty[642]=10;
	const auto purse=XeenCharacterFormat::parseMonsterPurse(pty);check(purse.gold==800 && purse.gems==10,"Resource purse offsets");
	pty.pop_back();rejects([&] { XeenCharacterFormat::parseMonsterPurse(pty); });
}
}
int main() { try { physical();shootAndLoot();additionalTapes();completeWeaponRules();missileClassAndZeroDamage();rejectionBudgets();rangedOpportunity();timeAndInputs();std::cout<<"M33 artificial pure-rule controls passed\n";return 0; } catch (const std::exception &e) { std::cerr<<e.what()<<'\n';return 1; } }