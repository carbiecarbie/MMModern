#ifndef MMODERN_JOURNEY_GAMEPLAY_TEST_SUPPORT_H
#define MMODERN_JOURNEY_GAMEPLAY_TEST_SUPPORT_H
#include "XeenCompletedGameplayOracle.h"
namespace journey_gameplay_test {
using namespace combat_gameplay_test;
struct JourneyOracle {
 XeenSaveSnapshot expected;
 std::vector<XeenActor> actors;
 explicit JourneyOracle(const XeenGameplayServices &s) {
  const auto initial=s.resources.loadInitialParty();
  constexpr int hp[]{12,16,12,10,7,5},sp[]{2,0,2,0,7,9};
  for(unsigned i=0;i<6;++i){const auto &c=initial.roster.at(kXeenCombatOwners[i]);
   check(c.currentHp==hp[i]&&c.currentSp==sp[i]&&c.conditions==std::array<std::uint8_t,16>{},"literal original HP/SP/conditions prestate");}
  check(xeenSameItem(initial.roster.at(6).accessories[1],{86,1,0,0})&&xeenSameItem(initial.roster.at(0).accessories[1],{}),"literal original ring prestate");
  expected.resources=s.resources.signature;expected.camera=XeenActorApproach::kEntry;
  expected.characters=initial.roster.characters();expected.activeRosterIds=initial.party.activeRosterIds();
  expected.questItems=initial.questItems.counts();expected.questFlags=initial.questFlags.values();expected.gameFlags=s.initialFlags().values();
  expected.journey.emplace();auto &j=*expected.journey;
  j.context=s.resources.loadInitialContext();j.skeletonSeed=56;
  const auto chr=s.resources.loadInitialCharacters();
  for(unsigned i=0;i<30;++i) { j.supplements[i].owner=i;j.supplements[i].inputs=XeenCharacterFormat::parseCombatInputs(chr,i); }
  for(auto owner:kXeenCombatOwners)check(j.supplements[owner].inputs.experience==0,"literal original active XP");
  const auto mob=s.objects(20);const auto mon=s.resources.loadMonsterStatistics();
  for(unsigned i=0;i<27;++i) {
   XeenActor a;a.id={20,i};a.original=mob.entities.monsters.at(i);a.x=a.original.x;a.y=a.original.y;
   if(a.original.hasResource()){a.statistics=mon.at(a.original.resourceId);a.hp=a.statistics->baseHp();a.lifecycle=XeenActorLifecycle::Present;}
   if(a.original.isDisabled())a.lifecycle=XeenActorLifecycle::Disabled;
   if(i==5)a.activated=true;actors.push_back(a);
  }
 }
 void state(Harness &h) const {
  const auto &p=*h.party;const auto &w=*h.world;
  for(unsigned i=0;i<30;++i){remove_test::checkSameCharacter(expected.characters[i],p.roster.at(i));
   check(p.roster.combatInputs(i)&&xeen_state::sameInputs(expected.journey->supplements[i].inputs,*p.roster.combatInputs(i)),"complete supplement equality");}
  check(p.encounterContext==expected.journey->context&&xeen_state::sameCamera(expected.camera,*h.camera),"literal context/camera");
  check(p.party.activeRosterIds()==expected.activeRosterIds&&p.firstSerializedCount==6&&p.effectiveSerializedCount==6,"exact membership");
  check(p.questItems.counts()==expected.questItems&&p.questFlags.values()==expected.questFlags&&h.flags->values()==expected.gameFlags,"quest/game flags");
  check(w.sessionState().disabledObjects().empty()&&w.sessionState().disabledEvents().empty(),"original empty overlays");
  check(w.sessionState().skeletonSeed()==expected.journey->skeletonSeed,"retained seed");sameActors(actors,w.sessionState().actors());
  check(w.sessionState().accountedMonsters().size()==(actors[5].lifecycle==XeenActorLifecycle::Defeated?1u:0u),"identity accounting");
  check(w.sessionState().accountedMonsters().count({20,5})==(actors[5].lifecycle==XeenActorLifecycle::Defeated?1u:0u),"exact accounted identity");
 }
 void ring(bool onArturius,bool equipped) {
  expected.characters[onArturius?6:0].accessories[1]={};
  expected.characters[onArturius?0:6].accessories[1]={86,1,0,static_cast<std::uint8_t>(equipped?8:0)};
 }
 void victory() {
  auto &r=expected.characters[1];r.currentHp=-17;r.conditions[12]=r.conditions[13]=1;
  r.armor[0]={0,2,128,3};r.armor[1]={38,10,128,9};
  for(unsigned owner:kXeenCombatOwners)if(owner!=1)expected.journey->supplements[owner].inputs.experience+=100;
  auto &a=actors[5];a.x=a.y=-128;a.hp=0;a.activated=false;a.lifecycle=XeenActorLifecycle::Defeated;
  expected.journey->context->minutes=492;expected.journey->context->ctr24=1;
 }
};

}
#endif
