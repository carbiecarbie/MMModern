#include "XeenRegionalTestSupport.h"
#include "games/xeen/XeenMovement.h"
#include <iostream>
#include "games/xeen/XeenJourneyCapture.h"
using namespace regional_test;
namespace mmodern {
struct XeenEventDispatchTestAccess {
 static XeenSaveSnapshot beforeFrame(XeenEncounterFlow &flow,const XeenSaveResourceSignature &signature,const XeenPartyState &party,
   const XeenCamera &camera,const XeenGameFlags &flags,const XeenWorld &world) {
  // Only the test admits capture of the restored values before presentation.
  // Production Load still cannot Save until its arrival frame is acquired.
  const auto pending=flow._journeyCapture->initializationPending;
  flow._journeyCapture->initializationPending=true;
  const auto result=XeenSaveState::captureInitialized(signature,party,camera,flags,world);
  flow._journeyCapture->initializationPending=pending;return result;
 }
};
}
namespace {
struct LightingFixture {
    mmodern::XeenProgramState programState{mmodern::XeenWallClock::fixed(0)};
 XeenPartyState p;XeenCamera camera=xeenJourneyContent().entry;XeenGameFlags flags;
 XeenWorld w{[](auto id) {auto m=map(id);m.geometry.flags2|=0x4000;
     for(auto &cell:m.geometry.cells)cell.rawAttributes|=0x20;
     // Refused movement must not mark stepped or publish any other suffix.
     auto &cell=m.geometry.cells[11*16+8];xeenGet<XeenOutdoorLayers>(cell.geometry).middle=1;return m;},
     [](auto id) {return objects(id);}};
 std::unique_ptr<XeenEncounterFlow> flow;
 explicit LightingFixture(const std::optional<XeenSaveSnapshot> &saved={}) {
  if(saved) {
   XeenSaveState::restoreBeforeGameplay(*saved,resources(),p,camera,flags,w,[](auto &,const auto &,const auto &,const auto &){});
   flow=std::make_unique<XeenEncounterFlow>(programState,w,p,camera,flags,[]{return 0;},XeenJourneyRestoreTag{});
  } else {
   auto bytes=characterBytes();p=XeenPartyLoader().loadFromResources(bytes,partyBytes());
   const auto r=resources();
   XeenJourneySetup setup{bytes,XeenGameplayContextFormat::parse(partyBytes()),statistics(),events(23),1,r.regionalManifest};
   setup.purse=XeenPurse{};setup.worldFlags=XeenWorldFlags{};setup.regionalText=texts(23);
   setup.learnedNames=XeenLearnedSpellNames{};setup.learnedNamesProvider=r.loadLearnedSpellNames;
   setup.bank=XeenBankBalances{};setup.cityEventsProvider=[]{return events(28);};
   flow=std::make_unique<XeenEncounterFlow>(programState,w,p,camera,flags,[]{return 0;},setup);
  }
  if(saved)check(XeenSaveFormat::encode(XeenEventDispatchTestAccess::beforeFrame(*flow,signature(),p,camera,flags,w))==XeenSaveFormat::encode(*saved),"Restore is byte-exact before the arrival frame");
  w.scenePresentation().markLightingStep();
  present();
 }
 void present() {
  check(flow->prepareJourneyFrame(flow->ticket(),[]{}),"Lighting scene preparation failed");
  check(flow->presentJourney(flow->ticket()),"Lighting frame publication failed");
 }
 void settle(XeenEncounterAction action) {
  auto r=flow->journeyAction(flow->ticket(),action);
  check(r.outcome!=XeenEncounterOutcome::Refused && r.outcome!=XeenEncounterOutcome::Stopped,"Lighting action refused");
  for(unsigned n=0;n<30;++n) {
   if(w.sessionState().journeyActivity()==XeenJourneyActivity::Presentation)present();
   if(flow->journeyQuiet())return;
   flow->journeyPulse(flow->ticket());
  }
  throw std::runtime_error("Lighting action failed to settle");
 }
 XeenSaveSnapshot snapshot() {return XeenSaveState::capture(signature(),p,camera,flags,w);}
};
void drainAndRestore() {
 LightingFixture entry;auto source=entry.snapshot();source.journey->context->lightAndResistances[0]=3;
 for(auto &a:source.map(23).monsters->slots) {a.x=a.y=-128;a.lifecycle=XeenActorLifecycle::Defeated;a.accounted=true;}
 LightingFixture f(source);
 check(f.p.encounterContext->lightAndResistances[0]==2,"First arrival frame drains exactly once after exact restore");
 check(!f.w.scenePresentation().lightingStepped,"Completed redraw clears stepped");
 const auto before=XeenSaveFormat::encode(f.snapshot());
 f.settle(XeenEncounterAction::Forward);
 check(before==XeenSaveFormat::encode(f.snapshot()),"Skill refusal changed position, time, RNG, monsters, exploration or light");
 check(!f.w.scenePresentation().lightingStepped,"Blocked move marked stepped");
 check(f.flow->prepareJourneyFrame(f.flow->ticket(),[]{},true) &&
       f.flow->prepareJourneyFrame(f.flow->ticket(),[]{},true),"Cosmetic redraw failed");
 check(before==XeenSaveFormat::encode(f.snapshot()),"Repeated redraw spent light or changed gameplay");
 LightingFixture restored(f.snapshot());
 auto afterArrival=f.snapshot();afterArrival.journey->context->lightAndResistances[0]=1;
 check(XeenSaveFormat::encode(afterArrival)==XeenSaveFormat::encode(restored.snapshot()),"Arrival changes only its one drain after exact restore");
 f.settle(XeenEncounterAction::Left);
 check(f.p.encounterContext->lightAndResistances[0]==1,"Turn on a drain cell must spend exactly one light");
 f.settle(XeenEncounterAction::Wait);
 check(f.p.encounterContext->lightAndResistances[0]==0 && f.w.sceneIsBlack(f.p,f.camera),"Wait drain reaches black at zero");
 f.settle(XeenEncounterAction::Right);
 check(f.p.encounterContext->lightAndResistances[0]==0,"Zero light never underflows");
 LightingFixture retry(source);
 retry.flow->journeyAction(retry.flow->ticket(),XeenEncounterAction::Left);
 check(!retry.flow->prepareJourneyFrame(retry.flow->ticket(),[]{throw std::runtime_error("Synthetic draw failure");}),"Throwing composition was accepted");
 check(retry.p.encounterContext->lightAndResistances[0]==1,"Failed composition did not retain prepared light");
 retry.present();
 check(retry.p.encounterContext->lightAndResistances[0]==1,"Composition retry drained light twice");
 LightingFixture mutated(source);
 mutated.flow->journeyAction(mutated.flow->ticket(),XeenEncounterAction::Left);
 check(!mutated.flow->prepareJourneyFrame(mutated.flow->ticket(),[&] {
     mutated.w.scenePresentation().markLightingStep();mutated.w.scenePresentation().lightingPrepared=true;
 }),"Composition callback changed transient lighting authority through ABA");
}
void providerMutation() {
 XeenWorld *owner=nullptr;
 XeenWorld world([&](auto id) {owner->scenePresentation().completeLightingDraw();return map(id);});owner=&world;
 XeenPartyState party;party.encounterContext.emplace();party.encounterContext->lightAndResistances[0]=2;
 XeenCamera camera{23,9,11,XeenDirection::West};XeenGameFlags flags;
 world.scenePresentation().markLightingStep();
 XeenRestoreGuard guard(world,party,camera,flags);
 bool rejected=false;
 try {XeenRestoreGuard::Providers providers(guard,world);(void)world.lightAfterScenePreparation(party,camera);}
 catch(const std::exception &) {rejected=true;}
 check(rejected && !guard.current(),"Cache-miss provider suppressed the pending lighting check");
}
void retainedAndDestroyedOwners() {
 XeenWorld world([](auto id){return map(id);});
 auto party=std::make_unique<XeenPartyState>();
 XeenCamera camera{23,9,11,XeenDirection::West};XeenGameFlags flags;
 XeenRestoreGuard guard(world,*party,camera,flags);
 world.scenePresentation().markLightingStep();world.scenePresentation().completeLightingDraw();
 check(guard.current(),"Authorized redraw transients invalidated a retained service guard");
 {
  XeenRestoreGuard::Providers providers(guard,world);
  party.reset();
  check(!guard.current(),"Destroyed callback owner retained guard authority");
 } // Observation unwinding must not inspect the destroyed party.
}
}
int main() {try {drainAndRestore();providerMutation();retainedAndDestroyedOwners();std::cout<<"Traversal refusal, lighting drain, redraw, restore and callback guards passed\n";return 0;}
 catch(const std::exception &e) {std::cerr<<e.what()<<'\n';return 1;}}
