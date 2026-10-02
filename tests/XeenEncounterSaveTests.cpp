#include "XeenRegionalTestSupport.h"
#include "XeenEncounterTestSupport.h"
#include <iostream>
using namespace mmodern;
using save_test::check;using save_test::rejects;
namespace {
struct Destination {
 XeenPartyState party=regional_test::resources().loadInitialParty();
 XeenCamera camera{23,9,11,XeenDirection::West};XeenGameFlags flags;
 XeenWorld world{regional_test::map,regional_test::objects};
};
void rejectionBoundaries() {
 const auto saved=regional_test::snapshot();
 for(unsigned kind=0;kind<3;++kind) {
  Destination f;
  f.world.disableObject({23,0});
  auto events=regional_test::events(23);events.records[0].x=4;events.records[0].y=4;
  f.world.disableEventsAtCell({23,4,4,XeenDirection::North},events);
  std::unique_ptr<regional_test::Fixture> live;
  if(kind==0)f.world.markEncounterSession();
  if(kind==1)f.party.encounterContext=regional_test::resources().loadInitialContext();
  if(kind==2){live=std::make_unique<regional_test::Fixture>();live->flow->holdJourneyWork(XeenCombatBoundary::Work::Event);}
  auto &party=live?live->p:f.party;auto &camera=live?live->camera:f.camera;
  auto &world=live?live->w:f.world;auto &flags=live?live->flags:f.flags;
  auto before=regional_test::resources().loadInitialParty();
  for(unsigned owner=0;owner<30;++owner)before.roster.at(owner)=party.roster.at(owner);
  before.party=XeenParty::fromRosterIds(party.party.activeRosterIds());
  before.questItems=party.questItems;before.questFlags=party.questFlags;before.encounterContext=party.encounterContext;
  before.firstSerializedCount=party.firstSerializedCount;before.effectiveSerializedCount=party.effectiveSerializedCount;before.diagnostics=party.diagnostics;
  const auto view=camera;const auto flagValues=flags.values();
  const std::vector<XeenActor> actors=world.sessionState().actors();
  const std::set<XeenObjectIdentity> objects=world.sessionState().disabledObjects();
  const std::set<XeenEventIdentity> disabledEvents=world.sessionState().disabledEvents();
  const auto marked=world.sessionState().encounterMarked();unsigned calls=0;
  auto r=regional_test::resources();r.loadInitialParty=[&]{++calls;return regional_test::resources().loadInitialParty();};
  r.loadEvents=[&](auto id){++calls;return regional_test::events(id);};
  r.loadMonsterStatistics=[&]{++calls;return regional_test::statistics();};
  for(unsigned attempt=0;attempt<2;++attempt) {
   rejects([&]{XeenSaveState::capture(r.signature,party,camera,flags,world);},"encounter");
   rejects([&]{XeenSaveState::restoreBeforeGameplay(saved,r,party,camera,flags,world,[&](auto &,const auto &,const auto &,const auto &){++calls;});},"encounter");
   if(kind!=1)rejects([&]{world.restoreSessionState({}, {}, {});},"encounter");
   world.discardMapCache();
  }
  check(calls==0&&party.encounterContext==before.encounterContext&&save_test::sameCamera(camera,view)&&flags.values()==flagValues&&
   world.sessionState().disabledObjects()==objects&&world.sessionState().disabledEvents()==disabledEvents&&world.sessionState().encounterMarked()==marked,
   "Rejected restore changed destination values or invoked providers");
  encounter_test::sameParty(before,party);encounter_test::sameActors(actors,world.sessionState().actors());
  if(kind==2){party.encounterContext.reset();rejects([&]{XeenSaveState::capture(r.signature,party,camera,flags,world);},"encounter");
   rejects([&]{world.restoreSessionState({}, {}, {});},"encounter");encounter_test::sameActors(actors,world.sessionState().actors());}
 }
}
void candidateFailures() {
 const auto saved=regional_test::snapshot();
 for(bool preflight:{false,true}) {
  Destination f;const auto before=f.party;const auto camera=f.camera;auto r=regional_test::resources();bool fired=false;
  if(!preflight)r.loadMonsterStatistics=[&]{fired=true;throw std::runtime_error("Injected monster provider failure");return regional_test::statistics();};
  rejects([&]{XeenSaveState::restoreBeforeGameplay(saved,r,f.party,f.camera,f.flags,f.world,[&](auto &w,const auto &,const auto &,const auto &){fired=true;w.markEncounterSession();});});
  check(fired&&!f.party.encounterContext&&!f.world.hasEncounterState()&&save_test::sameCamera(camera,f.camera),"Bad provider/preflight partially restored");
  encounter_test::sameParty(before,f.party);
 }
 regional_test::Fixture restored(saved);save_test::sameSnapshot(saved,restored.snapshot());
}
}
int main(){try{rejectionBoundaries();candidateFailures();std::cout<<"Regional destination rejection and preservation passed\n";return 0;}
 catch(const std::exception &e){std::cerr<<e.what()<<'\n';return 1;}}
