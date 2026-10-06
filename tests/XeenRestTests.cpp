#include "games/xeen/XeenRestRules.h"
#include "games/xeen/XeenRegionalRules.h"
#include "XeenSaveTestSupport.h"
#include <iostream>
using namespace mmodern;
using save_test::check;
namespace {
XeenConsequenceCharacters characters() {
 XeenConsequenceCharacters c;
 for(unsigned n=0;n<6;++n) {auto &v=c[n];v.rosterId=kXeenCombatOwners[n];v.permanentLevel=3;v.birthYear=592;
  v.intellect=v.personality=v.endurance={15,2};v.currentHp=1;v.conditions[8]=1;}
 return c;
}
XeenConsequenceInputs inputs() {
 XeenConsequenceInputs i;for(auto &v:i) {
  v.might=v.speed=v.accuracy={15,2};v.luck=XeenAttributeValue{15,2};v.temporaryAc=3;
  v.resistances=XeenCombatResistances{};v.resistances->fireTemporary=9;v.poisonResistance=XeenAttributeValue{5,3};
 }return i;
}
void recovery() {
 auto c=characters();auto i=inputs();XeenGameplayContext context;context.year=610;context.day=8;context.minutes=960;
 c[0].conditions[13]=1;c[1].conditions[14]=1;c[2].conditions[15]=1;
 c[3].conditions[12]=1;c[3].currentHp=-1;c[4].conditions[2]=2;
 context.effects.fill(7);context.lightAndResistances.fill(7);
 // Literal rest loop: zero food is tested before Dead..Eradicated eligibility.
 for(unsigned food:{0u,1u,2u,3u,4u}) {
  XeenRestRecovery r(c,i,context,food);
  check(r.consumed==std::min(food,3u) && r.food==food-std::min(food,3u),"Rest eligible food allocation");
  check(r.starving==(food<3),"Rest starvation is per-member before eligibility, not ending food zero");
  check(r.context.rested==(food>0),"Food sets rested even for dead members");
  for(unsigned n=0;n<6;++n) {
   const auto &v=r.characters[n];check(!v.conditions[8] && !v.temporaryLevel && !v.endurance.temporary,"Rest wake and temp reset");
   check(!r.inputs[n].temporaryAc && !r.inputs[n].resistances->fireTemporary && !r.inputs[n].poisonResistance->temporary,"Rest supplement reset");
   const bool fed=n>=3 && n<3+food;
   check(v.currentHp==(fed?XeenCharacterRules::maxHp(v,{610}):int(c[n].currentHp)),"Rest HP allocation in active order");
  }
  check(r.characters[4].conditions[2]==2,"No explicit post-refill Weak cure");
  check(r.context.effects[1]==7 && r.context.lightAndResistances[1]==7,"Rest retains automap and torches");
 }
 auto allDead=c;for(auto &v:allDead)v.conditions[13]=1;
 XeenRestRecovery dead(allDead,i,context,1);check(dead.context.rested && !dead.starving && dead.food==1,"All-dead food eligibility");
}
void time() {
 for(bool outdoor:{false,true})for(unsigned food:{0u,6u}) {
  auto c=characters();auto i=inputs();XeenGameplayContext context;context.year=610;context.day=8;context.minutes=480;context.ctr24=17;
  context.profile=XeenBehaviorProfile::WorldOfXeenClouds;c[0].conditions[2]=5;c[0].conditions[7]=2;
  XeenCombatRandom rng(17);
  for(unsigned step=0;step<11;++step) {
   XeenConditionTimeCandidate candidate(context,step<10?(outdoor?10:1):(outdoor?380:470),c,i,nullptr,XeenTimeMode::Sleeping);
   for(unsigned slice=0;;++slice) {check(slice<1000,"Rest time bounded");XeenConsequenceDraw draw{rng,1,{}};if(candidate.service(draw))break;}
   context=candidate.context;c=candidate.characters;i=candidate.inputs;
  }
  check(context.minutes==960 && context.day==8 && context.ctr24==17,"Ten charges plus suffix total 480 without stepTime drift");
  check(c[0].conditions[2]==2 && !c[0].conditions[7],"Weak/Drunk conversion precedes recovery independently of food");
  XeenRestRecovery result(c,i,context,food);check(result.characters[0].conditions[2]==2,"Recovery retains original converted Weak");
 }
}
void movement() {
 XeenMonsterRecord monster;monster.raw[20]=10;monster.raw[22]=1;
 XeenActor a;a.id={23,0};a.x=2;a.y=5;a.original.resourceId=3;a.statistics=monster;
 a.lifecycle=XeenActorLifecycle::Present;a.status=XeenActorStatus::Physical;
 const XeenCamera camera{23,5,5,XeenDirection::North};
 const auto terrain=[](const auto &,int,int){return XeenMonsterTerrain::Allowed;};
 check(XeenActorApproach::move({a},camera,terrain)[0].x==2,"Interactive unactivated actor remains stationary");
 auto asleep=XeenActorApproach::move({a},camera,terrain,true,{},XeenActorOpportunityContext{true});
 check(asleep[0].x==3 && !asleep[0].activated,"Sleeping opportunity moves without permanent activation");
 check(XeenActorApproach::move({a},camera,[](const auto &,int,int){return XeenMonsterTerrain::Blocked;},true,{},XeenActorOpportunityContext{true})[0].x==2,
  "Sleeping movement retains terrain/wall refusal");
 for(const auto context:{XeenActorOpportunityContext{true,false,false},XeenActorOpportunityContext{true,true,true}})
  check(XeenActorApproach::move({a},camera,terrain,true,{},context)[0].x==2,"Sleeping movement/shooting guard");
 auto c=characters();check(!xeenRestRangedWake(c),"Unhit sleepers keep sleeping");
 c[0].conditions[9]=1;check(xeenRestRangedWake(c),"Depressed worst condition wakes mode even with Asleep");
 c[0].conditions[11]=1;check(!xeenRestRangedWake(c),"Paralyzed worst condition does not wake mode");
 c[0].conditions.fill(0);c[0].conditions[2]=1;check(!xeenRestRangedWake(c),"Awake Weak member does not satisfy literal ranged wake test");
 c[0].conditions[2]=0;check(xeenRestRangedWake(c),"Good member wakes mode");
}
}
int main() {try {recovery();time();movement();std::cout<<"Rest recovery/time/movement reference oracles passed\n";return 0;}
 catch(const std::exception &e){std::cerr<<e.what()<<'\n';return 1;}}
