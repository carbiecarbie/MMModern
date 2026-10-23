#include "XeenTestInstallation.h"
#include "XeenTrainingTestSupport.h"
#include "games/xeen/XeenEventTrigger.h"
#include <iostream>
#include <sstream>
using namespace training_test;
// F3 mainland Events run in play through the same detached candidate as Vertigo:
// the Journey RNG, the restore guard and one publication, then an exact restore.
namespace {
XeenSaveSnapshot mainland(Inputs &in,XeenCamera camera) {
 auto s=in.base();s.camera=camera;
 // No monster may interrupt the Event with an approach or combat.
 for(auto &a:s.map(23).monsters->slots){a.x=a.y=-128;a.lifecycle=XeenActorLifecycle::Defeated;a.accounted=true;}
 return s;
}
struct Run {
 Fixture &f;unsigned failures=0,results=0;std::optional<XeenPresentationRequest> request;
 explicit Run(Fixture &f):f(f) {
  f.flow->reportManual=[this](const XeenManualEventResult &result) {
   ++results;request.reset();
   if(const auto *s=std::get_if<XeenEventExecutionSuspended>(&result))request=s->request;
   if(const auto *e=std::get_if<XeenEventExecutionError>(&result)){++failures;std::cerr<<"F3 Event: "<<e->message<<'\n';}
  };
  f.flow->reportAutomatic=[this](const XeenAutomaticEventResult &){++failures;std::cerr<<"F3 Event ran on the live path\n";};
 }
 void settle() {
  for(unsigned n=0;n<2000 && !f.flow->canSave();++n) {
   if(request && request->response!=XeenPresentationResponseRequirement::Presented) {
    const auto kind=request->kind;const auto response=request->response;request.reset();
    if(kind==XeenPresentationKind::CharacterSelection)f.act(SelectMemberAction{0});
    else if(response==XeenPresentationResponseRequirement::YesNo)f.act(YesAction{});
    else f.act(AcknowledgeAction{});
   } else {
    // Timed portrait effects advance with the clock; dialogs wait for a key.
    f.now+=125;f.flow->beginCycle(++f.cycle);if(auto frame=f.flow->updatePresentation())f.present(*frame);
    if(!request && f.flow->presentationGeneration() && f.flow->inputContext(f.flow->frame().presentation()).dialog)f.act(AcknowledgeAction{});
   }
  }
  if(!f.flow->canSave())std::cerr<<"stuck activity="<<unsigned(f.w.sessionState().journeyActivity())<<" results="<<results<<" request="<<(request?int(request->kind):-1)
   <<" presenting="<<bool(f.flow->presentationGeneration())<<" refusal='"<<f.flow->encounter()->journeyRefusal()<<"'\n";
  check(f.flow->canSave() && !failures,"F3 Event did not settle without failure");
 }
};
void roundTrip(Inputs &in,Fixture &f,const char *what) {
 const auto bytes=XeenSaveFormat::encode(f.snapshot());
 Fixture restored(in,XeenSaveFormat::decode(bytes));
 check(XeenSaveFormat::encode(restored.snapshot())==bytes,what);
}
// Space at a fountain and a statue: refused as unsupported before this change.
void manual(Inputs &in) {
 for(const XeenCamera camera:{XeenCamera{23,0,1,XeenDirection::North},XeenCamera{23,12,2,XeenDirection::North}}) {
  const bool fountain=camera.x==0;
  auto s=mainland(in,camera);
  Fixture f(in,s);Run run(f);
  const auto owner=f.p.party.activeRosterIds()[0];const auto before=f.p.roster.at(owner);
  const auto random=f.w.sessionState().journeyRandom();const auto flags=*f.p.worldFlags;
  f.act(InteractionAction{});run.settle();
  const auto &after=f.p.roster.at(owner);
  check(run.results && f.flow->encounter()->journeyRefusal().empty(),"F3 manual Event was refused");
  // maze0023.evt @437/@463: the fountain gives +5 temporary levels once and sets world flag 22.
  if(fountain)check(after.temporaryLevel==before.temporaryLevel+5 && !flags.values[22] && f.p.worldFlags->values[22],
   "F3 fountain did not publish its give and flag once");
  // The statue teaches its spell only to some classes; the first member is not one.
  else check(after.learnedSpells==before.learnedSpells && after.temporaryLevel==before.temporaryLevel,"F3 statue changed an ineligible member");
  // Part 0: WhoWill rewrites the key and NPC Enter replaces Space (0x25299, 0x25ba6); neither leaves Wait.
  check(f.w.sessionState().journeyRandom()==random && f.p.encounterContext->minutes==s.journey->context->minutes,
   "F3 manual Event drew or charged differently");
  roundTrip(in,f,"F3 manual Event did not restore exactly");
 }
}
// Stepping onto the mainland sign, F3's only automatic Event (maze0023.evt @495).
void automatic(Inputs &in) {
 auto s=mainland(in,{23,5,8,XeenDirection::North});
 Fixture f(in,s);Run run(f);unsigned signs=0;
 const auto report=f.flow->reportManual;
 f.flow->reportManual=[&](const XeenManualEventResult &result) {
  report(result);if(run.request && run.request->kind==XeenPresentationKind::SceneLabelSign)++signs;
 };
 f.act(NavigationAction::MoveForward);run.settle();
 check(f.c.x==5 && f.c.y==9 && signs==1 && f.flow->encounter()->journeyRefusal().empty(),"F3 sign did not run once on arrival");
 roundTrip(in,f,"F3 automatic Event did not restore exactly");
}
}
// The Pyramid remains outside Part A. The Dwarf Mine is exercised by
// XeenEntranceSceneTests through its original script.
void entrances(Inputs &in) {
 for(const XeenCamera camera:{XeenCamera{23,8,10,XeenDirection::North}}) {
  auto s=mainland(in,camera);Fixture f(in,s);
  unsigned errors=0;
  f.flow->reportManual=[&](const XeenManualEventResult &result){if(std::get_if<XeenEventExecutionError>(&result))++errors;};
  const auto before=XeenSaveFormat::encode(f.snapshot());const auto scene=f.flow->frame();
  // Part 0: isolate the unsupported Event with click; Space would continue as Wait.
  f.act(InteractionAction{false});f.finishDeparture();
  const auto notice=drawXeenErrorScroll(scene,in.font,"This Event is not supported yet");
  check(errors==1 && f.flow->canSave() && f.flow->encounter()->journeyRefusal()=="This Event is not supported yet",
   "F3 entrance scene was not refused");
  // Every pixel the notice scroll paints must be on screen; the scene under it may move on.
  const auto &shown=f.flow->frame().pixels;unsigned painted=0,missing=0;
  for(std::size_t i=0;i<shown.size();++i)if(notice.pixels[i]!=scene.pixels[i]){++painted;missing+=shown[i]!=notice.pixels[i];}
  check(painted && !missing,"F3 entrance refusal notice is not drawn on the scene");
  check(XeenSaveFormat::encode(f.snapshot())==before,"F3 entrance refusal changed the game");
 }
}
int main(int argc,char **argv) {try {
 check(argc==2,"usage: mainland-events <installation>");auto install=xeenTestInstallationDetector().detect(argv[1]);
 check(bool(install),"Missing installation");Inputs in(*install);
 // Journey captures print diagnostics; keep them out of the test log.
 std::ostringstream captured;auto *old=std::cout.rdbuf(captured.rdbuf());
 try {manual(in);automatic(in);entrances(in);}catch(...){std::cout.rdbuf(old);throw;}
 std::cout.rdbuf(old);
 std::cout<<"F3 mainland manual and automatic Events publish once and restore exactly\n";return 0;
}catch(const std::exception &e){std::cerr<<e.what()<<'\n';return 1;}}
