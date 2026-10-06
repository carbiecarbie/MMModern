#include "XeenTrainingTestSupport.h"
#include "platform/sdl/XeenMainScreenInput.h"
#include <iostream>
using namespace training_test;
namespace mmodern {
struct XeenRestTestAccess {
 static bool active(const XeenEventFlow &f){return bool(f._encounter->_rest);}
 static bool complete(const XeenEventFlow &f){return f._encounter->_rest && f._encounter->_rest->phase==XeenEncounterFlow::RestContinuation::Phase::Complete;}
 static bool confirm(const XeenEventFlow &f){return f._encounter->_rest && f._encounter->_rest->phase==XeenEncounterFlow::RestContinuation::Phase::Confirm;}
 static bool refused(const XeenEventFlow &f){return f._encounter->_rest && f._encounter->_rest->phase==XeenEncounterFlow::RestContinuation::Phase::Refused;}
 static unsigned dream(const XeenEventFlow &f){return f._encounter->_rest && f._encounter->_rest->phase==XeenEncounterFlow::RestContinuation::Phase::Dream?f._encounter->_rest->dreamBeat+1:0;}
 static void pending(XeenEventFlow &f,unsigned value){auto &e=*f._encounter;e._state._pending=value;f._encounterFrame=e.ticket();}
};
}
namespace {
void wire(Fixture &f){f.flow->loadRestDream=[&]{return f.in.assets.restDreamImage();};}
void pulse(Fixture &f){f.now+=100;f.flow->beginCycle(++f.cycle);if(const auto frame=f.flow->updatePresentation())f.present(*frame);}
void finish(Fixture &f) {
 for(unsigned n=0;n<1000;++n) {
  if(XeenRestTestAccess::confirm(*f.flow)) {f.act(DialogKeyAction{'y'});continue;}
  if(XeenRestTestAccess::complete(*f.flow)) {f.act(AcknowledgeAction{});continue;}
  if(f.flow->canSave())return;
  check(!f.flow->encounter()->combat(),"Successful Rest fixture entered combat");pulse(f);
 }
 throw std::runtime_error("Rest did not reach a quiet boundary: "+f.flow->encounter()->notice());
}
std::vector<XeenActor> actors(Inputs &in,const XeenSaveSnapshot &s) {
 auto result=XeenActorApproach::actorsFromResources(in.maps.loadObjects(in.assets,s.camera.mapId),in.statistics);
 const auto &saved=s.camera.mapId==XeenMapIdentity(23)?s.journey->actors:*s.journey->vertigoActors;
 for(unsigned n=0;n<result.size();++n) {
  result[n].x=saved[n].x;result[n].y=saved[n].y;result[n].hp=saved[n].hp;
  result[n].activated=saved[n].activated;result[n].lifecycle=saved[n].lifecycle;
 }
 return result;
}
XeenSaveSnapshot source(Inputs &in,bool indoor,bool nearby=false,bool ranged=false,bool seam=false) {
 auto s=indoor?in.service():in.base();XeenWorld world(in.mapLoader(),in.objectLoader());
 const auto live=actors(in,s);const auto mainland=XeenMovement::component(world.map(23),9,11,{});
 for(int y=1;y<(indoor?31:15);++y)for(int x=1;x<(indoor?31:15);++x)for(unsigned d=0;d<4;++d) {
  if(!indoor && !mainland[y*16+x])continue;
  if(seam && x<16 && y<16)continue;
  const XeenCamera camera{indoor?28u:23u,x,y,XeenDirection(d)};
  const auto cell=world.sampleCell(camera.mapId,x,y);if(!cell || (cell->cell->rawAttributes&0x80))continue;
  if(indoor && cell->cell->surfaceIndex==4)continue;
  auto view=indoor?XeenIndoorScene().classifyActors(world,camera,live):XeenActorApproach::classify(live,camera);
  if(view.engaged())continue;
  bool close=false,ray=false;
  for(const auto &a:live)if(a.lifecycle==XeenActorLifecycle::Present) {
   if(std::abs(int(a.x)-x)<=3 && std::abs(int(a.y)-y)<=3)close=true;
   if(a.statistics->raw[32] && std::abs(int(a.x)-x)+std::abs(int(a.y)-y)>=2 &&
    std::abs(int(a.x)-x)<=3 && std::abs(int(a.y)-y)<=3 &&
    (indoor?xeenIndoorRangedRay(world,camera,a):xeenOutdoorRangedRay(world.map(23),camera,a)))ray=true;
  }
  if(nearby?!close || (ranged && !ray):close)continue;
  if(indoor && nearby && !ranged) {
   const auto moved=XeenActorApproach::move(live,camera,[&](const auto &a,int x,int y){return xeenIndoorActorTerrain(world,a,x,y);},true,{},XeenActorOpportunityContext{true});
   if(!XeenIndoorScene().classifyActors(world,camera,moved).engaged())continue;
  }
  s.camera=camera;auto &saved=indoor?*s.journey->vertigoActors:s.journey->actors;
  for(unsigned n=0;n<saved.size();++n)saved[n].activated=saved[n].activated || view.activation[n];
  std::cout<<"Rest fixture "<<camera.mapId<<' '<<x<<','<<y<<" facing "<<d<<" nearby="<<nearby<<" ranged="<<ranged<<'\n';
  return s;
 }
 throw std::runtime_error("No original Rest fixture");
}
void success(Inputs &in,bool indoor) {
 auto start=source(in,indoor);start.food=8;
 for(auto owner:start.activeRosterIds) {start.characters[owner].currentHp=1;start.characters[owner].currentSp=0;start.characters[owner].endurance.temporary=2;}
 Fixture key(in,start),mouse(in,start);wire(key);wire(mouse);
 key.act(RestAction{});const auto click=xeenMainScreenClick(290,80,MainScreen::Exploration);
 check(click && std::holds_alternative<RestAction>(*click),"Original Rest button path");mouse.act(*click);
 check(!key.flow->canSave() && key.flow->inputContext(key.flow->frame().presentation()).mainScreen==MainScreen::None,"Rest owns strict input and save boundary");
 const auto stale=key.flow->frame().presentation();const auto input=*key.flow->displayedInput();
 key.flow->handle(RestAction{},input,stale); // Acquired busy frame cannot restart Rest.
 finish(key);finish(mouse);save_test::sameSnapshot(key.snapshot(),mouse.snapshot());
 check(key.p.food==2 && key.p.encounterContext->minutes==start.journey->context->minutes+480 &&
  key.p.encounterContext->ctr24==start.journey->context->ctr24,"Original eight-hour Rest and six food, no stepTime");
 for(auto owner:start.activeRosterIds)check(!key.p.roster.at(owner).conditions[8] && key.p.roster.at(owner).currentHp==XeenCharacterRules::maxHp(key.p.roster.at(owner),{key.p.encounterContext->year}),"Original Rest wake/max HP");
 const auto saved=key.snapshot();Fixture loaded(in,saved);wire(loaded);save_test::sameSnapshot(saved,loaded.snapshot());
 key.act(RestAction{});loaded.act(RestAction{});finish(key);finish(loaded);
 check(key.p.food==0,"Partial-food exhaustion");save_test::sameSnapshot(key.snapshot(),loaded.snapshot());
 key.act(RestAction{});finish(key);check(key.p.food==0,"Repeated zero-food Rest permitted");
 const auto before=XeenSaveFormat::encode(key.snapshot());
 key.flow->handle(RestAction{},input,stale);check(XeenSaveFormat::encode(key.snapshot())==before,"Stale Rest input replays no state");
}
void entry(Inputs &in) {
 auto start=source(in,true);
 // Stat danger from temporary curse counters, preserving canonical birth year.
 start.characters[start.activeRosterIds[0]].birthYear=592;
 start.journey->supplements[start.activeRosterIds[0]].inputs.might.permanent=0;
 Fixture declined(in,start);wire(declined);const auto before=XeenSaveFormat::encode(declined.snapshot());
 declined.act(RestAction{});check(XeenRestTestAccess::confirm(*declined.flow),"Original SOME_CHARS_MAY_DIE confirmation");
 declined.act(DialogKeyAction{'n'});finish(declined);
 check(XeenSaveFormat::encode(declined.snapshot())==before,"Declined Rest preserves gameplay/time/RNG");
 auto valid=source(in,true,false,false,true);
 const auto restricted=[&](auto id){auto map=in.maps.loadGeometryMap(in.assets,id);if(id==XeenMapIdentity(28))map.geometry.flags|=0x4000;
  if(id!=XeenMapIdentity(23))for(auto &cell:map.geometry.cells)cell.flags=0xf8;return map;};
 Fixture refusal(in,valid,false,nullptr,restricted);wire(refusal);const auto refusalBefore=XeenSaveFormat::encode(refusal.snapshot());
 refusal.act(RestAction{});check(XeenRestTestAccess::refused(*refusal.flow),"Primary Rest restriction");refusal.act(AcknowledgeAction{});finish(refusal);
 check(XeenSaveFormat::encode(refusal.snapshot())==refusalBefore,"Rest refusal preserves gameplay");
 const auto cellFlags=[&](auto id){auto map=in.maps.loadGeometryMap(in.assets,id);if(id!=XeenMapIdentity(23))for(auto &cell:map.geometry.cells)cell.flags=0xf8;return map;};
 Fixture allowed(in,valid,false,nullptr,cellFlags);wire(allowed);allowed.act(RestAction{});finish(allowed);
 const auto neighborRestriction=[&](auto id){auto map=in.maps.loadGeometryMap(in.assets,id);if(id==XeenMapIdentity(109) || id==XeenMapIdentity(110) || id==XeenMapIdentity(111))map.geometry.flags|=0x4000;return map;};
 Fixture neighbor(in,valid,false,nullptr,neighborRestriction);wire(neighbor);neighbor.act(RestAction{});finish(neighbor);
}
void interruption(Inputs &in,bool ranged,unsigned pending,bool indoor=false) {
 auto start=source(in,indoor,true,ranged);start.journey->random=XeenCombatRandom(ranged?17:23).continuation();
 for(auto owner:start.activeRosterIds)start.characters[owner].currentHp=1000;
 Fixture f(in,start);wire(f);XeenRestTestAccess::pending(*f.flow,pending);
 if(pending) {const auto context=f.flow->inputContext(f.flow->frame().presentation());
  check(context.restAvailable && !context.readyForAction,"Positive countdown must admit only Rest on its acquired frame");}
 f.act(RestAction{});bool contact=false;
 for(unsigned n=0;n<200 && XeenRestTestAccess::active(*f.flow);++n) {pulse(f);contact=contact || bool(f.flow->encounter()->combat());}
 check(!XeenRestTestAccess::active(*f.flow),"Nearby actors did not interrupt Rest");
 check(f.p.food==start.food && f.p.encounterContext->minutes<start.journey->context->minutes+100 &&
  !f.p.encounterContext->rested,"Interrupted Rest has only elapsed charges, no food/recovery/dream");
 bool asleep=false;for(auto owner:start.activeRosterIds)asleep=asleep || f.p.roster.at(owner).conditions[8];
 check(asleep,"Unhit interrupted members remain asleep");
 std::cout<<"Interruption contact="<<contact<<" minutes="<<f.p.encounterContext->minutes<<" draws="<<f.w.sessionState().journeyRandom()->count<<'\n';
 {
 struct QuietOutput {std::ostringstream output;std::streambuf *before;QuietOutput():before(std::cout.rdbuf(output.rdbuf())){}~QuietOutput(){std::cout.rdbuf(before);}} quiet;
 for(unsigned n=0;n<3000 && !f.flow->canSave();++n) {
  if(const auto combat=f.flow->encounter()->combat();combat && combat->phase()==XeenCombatPhase::PlayerReady)f.act(AttackAction{});
  else if(f.w.sessionState().journeyActivity()==XeenJourneyActivity::Reward)f.act(AcknowledgeAction{});
  else pulse(f);
  check(f.flow->encounter()->state().phase()!=XeenEncounterPhase::SupportStopped,"Interrupted Rest ordinary combat stopped");
 }
 }
 check(f.flow->canSave() && f.p.food==start.food && !XeenRestTestAccess::active(*f.flow),"Combat returns to ordinary quiet without resuming Rest");
 const auto aftermath=f.snapshot();Fixture loaded(in,aftermath);wire(loaded);save_test::sameSnapshot(aftermath,loaded.snapshot());
 bool stillAsleep=false;for(auto owner:start.activeRosterIds)stillAsleep=stillAsleep || loaded.p.roster.at(owner).conditions[8];
 std::cout<<"Interrupted-rest aftermath asleep="<<stillAsleep<<" exact round-trip\n";
}
void reentrant(Inputs &in) {
 const auto start=source(in,true);Fixture f(in,start);unsigned calls=0;
 const auto input=f.flow->displayedInput();const auto frame=f.flow->frame().presentation();
 f.flow->loadRestDream=[&] {
  ++calls;f.flow->handle(RestAction{},input,frame);
  check(!f.p.roster.at(start.activeRosterIds[0]).conditions[8] && f.p.encounterContext==start.journey->context && f.p.food==start.food,
   "Fallible Rest preflight published sleep/time/food early");
  return in.assets.restDreamImage();
 };
 f.act(RestAction{});finish(f);check(calls==1 && f.p.food==start.food-6,"Reentrant Rest entry double-charged");
}
void combatOutcomes(Inputs &in,bool run) {
 auto start=source(in,false,true);start.journey->random=XeenCombatRandom(run?101:29).continuation();
 for(auto owner:start.activeRosterIds)start.characters[owner].currentHp=run?1000:1;
 Fixture f(in,start);wire(f);f.act(RestAction{});
 {
 struct QuietOutput {std::ostringstream output;std::streambuf *before;QuietOutput():before(std::cout.rdbuf(output.rdbuf())){}~QuietOutput(){std::cout.rdbuf(before);}} quiet;
 for(unsigned n=0;n<3000;++n) {
  const auto combat=f.flow->encounter()->combat();
  if(run && f.flow->canSave())break;
  if(!run && combat && combat->phase()==XeenCombatPhase::Defeat)break;
  if(combat && combat->phase()==XeenCombatPhase::PlayerReady)f.act(run?PlayerAction{RunAction{}}:PlayerAction{AttackAction{}});
  else if(f.w.sessionState().journeyActivity()==XeenJourneyActivity::Reward)f.act(AcknowledgeAction{});
  else pulse(f);
 }
 }
 check(!XeenRestTestAccess::active(*f.flow) && f.p.food==start.food && !f.p.encounterContext->rested,"Run/death cannot resume or complete interrupted Rest");
 if(run)check(f.flow->canSave() && f.c.x==10 && f.c.y==12,"Interrupted Rest uses original Run destination and quiet continuation");
 else check(f.flow->encounter()->combat() && f.flow->encounter()->combat()->phase()==XeenCombatPhase::Defeat && !f.flow->canSave(),"Interrupted Rest uses ordinary party-death handling");
 std::cout<<"Interrupted Rest "<<(run?"Run":"defeat")<<" passed\n";
}
void conditionDeath(Inputs &in) {
 auto start=source(in,true);start.journey->context->minutes=1439;
 for(auto owner:start.activeRosterIds)start.characters[owner].conditions[2]=255;
 Fixture f(in,start);wire(f);f.act(RestAction{});check(XeenRestTestAccess::confirm(*f.flow),"All-member stat danger confirmation missing");
 f.act(DialogKeyAction{'y'});
 for(unsigned n=0;n<500 && !XeenRestTestAccess::complete(*f.flow);++n)pulse(f);
 check(XeenRestTestAccess::complete(*f.flow) && f.p.encounterContext->minutes==479 && f.p.food==start.food,
  "Condition deaths must finish eight hours and the original completion, consuming no food for dead members");
 for(auto owner:start.activeRosterIds)check(f.p.roster.at(owner).conditions[13] && !f.p.roster.at(owner).conditions[8],"Condition-dead member recovery order");
 f.act(AcknowledgeAction{});check(f.flow->encounter()->state().reason()==XeenEncounterStop::Defeat && !f.flow->canSave(),"Rest's final party-death check missing");
}
void rolloverAndDream(Inputs &in) {
 auto start=source(in,true);start.food=3;start.journey->context->day=99;start.journey->context->minutes=1439;
 start.journey->context->effects[2]=1;start.journey->context->lightAndResistances[0]=4;
 start.characters[start.activeRosterIds[0]].conditions[2]=255;
 Fixture f(in,start);wire(f);f.act(RestAction{});finish(f);
 check(f.p.encounterContext->day==0 && f.p.encounterContext->year==611 && f.p.encounterContext->minutes==479 &&
  !f.p.encounterContext->newDay && f.p.roster.at(start.activeRosterIds[0]).conditions[2]==255,"Rest year rollover, sleeping dawn and provisional Weak sentinel");
 const auto save=f.snapshot();Fixture loaded(in,save);wire(loaded);f.act(RestAction{});loaded.act(RestAction{});finish(f);finish(loaded);
 f.act(NavigationAction::TurnLeft);loaded.act(NavigationAction::TurnLeft);finish(f);finish(loaded);
 save_test::sameSnapshot(f.snapshot(),loaded.snapshot());
 auto dreamStart=source(in,true);unsigned chosen=0;
 for(unsigned seed=1;seed<100 && !chosen;++seed) {
  auto context=*dreamStart.journey->context;XeenConsequenceCharacters c;XeenConsequenceInputs inputs;
  for(unsigned n=0;n<6;++n) {c[n]=dreamStart.characters[kXeenCombatOwners[n]];c[n].conditions[8]=1;inputs[n]=dreamStart.journey->supplements[kXeenCombatOwners[n]].inputs;}
  XeenCombatRandom random(seed);
  for(unsigned step=0;step<11;++step) {
   XeenConditionTimeCandidate time(context,step<10?1:470,c,inputs,&*dreamStart.journey->serviceEconomy,XeenTimeMode::Sleeping);
   for(unsigned n=0;;++n) {check(n<1000,"Dream oracle bounded");XeenConsequenceDraw draw{random,64,{}};if(time.service(draw))break;}
   context=time.context;c=time.characters;inputs=time.inputs;
  }
  if(random.draw(1,20)==1)chosen=seed;
 }
 check(chosen,"Fixed dream seed absent");dreamStart.journey->random=XeenCombatRandom(chosen).continuation();
 Fixture dream(in,dreamStart);wire(dream);dream.act(RestAction{});unsigned beats=0,hidden=0;
 std::optional<XeenJourneyRandomState> cursor;unsigned minutes=0;std::optional<unsigned> overall;
 for(unsigned n=0;n<500 && !XeenRestTestAccess::complete(*dream.flow);++n) {
  if(XeenRestTestAccess::dream(*dream.flow)) {
   ++beats;hidden+=dream.flow->inputContext(dream.flow->frame().presentation()).hideCursor;
   if(!cursor){cursor=dream.w.sessionState().journeyRandom();minutes=dream.p.encounterContext->minutes;}
   if(!overall)overall=dream.w.scenePresentation().overallFrame;
   check(dream.w.scenePresentation().overallFrame==*overall,"Dream advanced ordinary scene animation");
   check(dream.w.sessionState().journeyRandom()==cursor && dream.p.encounterContext->minutes==minutes,"Dream consumes no gameplay RNG or time");
   dream.act(RestAction{});check(!dream.flow->canSave(),"Dream reentrant input escaped boundary");
  }
  pulse(dream);
 }
 check(beats==146 && hidden==80,"Four original 33-step fades, two seven-tick holds and cursor restoration");finish(dream);
 std::cout<<"Dream seed="<<chosen<<" 146 visual beats; audio waits deferred\n";
}
}
int main(int argc,char **argv) {try {
 check(argc==2,"Rest original usage: installation");const auto installation=XeenInstallationDetector().detect(argv[1]);check(bool(installation),"Original installation absent");
 Inputs in(*installation);success(in,false);success(in,true);entry(in);interruption(in,false,0);interruption(in,true,3);interruption(in,false,0,true);rolloverAndDream(in);reentrant(in);combatOutcomes(in,true);combatOutcomes(in,false);conditionDeath(in);
 std::cout<<"Original-resource Rest, interruption, food, UI path and mid-sequence save tests passed\n";return 0;
 }catch(const std::exception &e){std::cerr<<e.what()<<'\n';return 1;}}
