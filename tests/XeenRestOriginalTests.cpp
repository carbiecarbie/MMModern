#include "XeenTestInstallation.h"
#include "app/XeenAudio.h"
#include "formats/xeen/XeenVoc.h"
#include "XeenTrainingTestSupport.h"
#include "platform/sdl/XeenMainScreenInput.h"
#include "games/xeen/XeenEventTrigger.h"
#include <iostream>
using namespace training_test;
namespace mmodern {
struct XeenRestTestAccess {
 static bool active(const XeenEventFlow &f){return bool(f._encounter->_rest);}
 static bool complete(const XeenEventFlow &f){return f._encounter->_rest && f._encounter->_rest->phase==XeenEncounterFlow::RestContinuation::Phase::Complete;}
 static bool confirm(const XeenEventFlow &f){return f._encounter->_rest && f._encounter->_rest->phase==XeenEncounterFlow::RestContinuation::Phase::Confirm;}
 static bool refused(const XeenEventFlow &f){return f._encounter->_rest && f._encounter->_rest->phase==XeenEncounterFlow::RestContinuation::Phase::Refused;}
 static bool portrait(const XeenEventFlow &f){return bool(f._portraitSequence);}
 static bool falling(const XeenEventFlow &f){return bool(f._fallSequence);}
 static bool eventPortrait(const XeenEventFlow &f){return f._portraitSequence && f._portraitSequence->event;}
 static bool manualTravel(const XeenEventFlow &f){return f._transition && f._transition->manualStep && f._transition->manualStep->opportunity && f._transition->manualStep->opportunity->travelStarted;}
 static unsigned manualOpportunities(const XeenEventFlow &f){return f._transition->manualStep->result.movementOpportunities;}
 static bool manualCandidate(const XeenEventFlow &f){return bool(f._transition);}
 static bool manualReady(const XeenEventFlow &f){return f._transition && f._transition->terrainDone && bool(f._transition->manualStep);}
 static int manualPortrait(const XeenEventFlow &f){return !f._portraitSequence?-1:f._portraitSequence->effect.stage==XeenEventPortraitStage::DamageOverlay?0:f._portraitSequence->effect.stage==XeenEventPortraitStage::DamageHold?1:2;}
 static int stagedHp(const XeenEventFlow &f,unsigned owner){return f._transition->party.roster.at(owner).currentHp;}
 static bool recovering(const XeenEventFlow &f){return f._encounter->_rest && f._encounter->_rest->phase==XeenEncounterFlow::RestContinuation::Phase::Recovery;}
 static void transitionComposer(XeenEventFlow &f,XeenEventFlow::TransitionCompose value){f._transitionCompose=std::move(value);}
  using Dream=XeenEncounterFlow::RestContinuation::Dream;
 static bool dream(const XeenEventFlow &f){return f._encounter->_rest && f._encounter->_rest->phase==XeenEncounterFlow::RestContinuation::Phase::Dream;}
 static Dream dreamPhase(const XeenEventFlow &f){return f._encounter->_rest->dreamPhase;}
 static unsigned fadeStep(const XeenEventFlow &f){return f._encounter->_rest->fadeStep;}
 static unsigned scale(const XeenEventFlow &f){return f._encounter->_rest->dreamScale();}
 static bool dreamScene(const XeenEventFlow &f){return f._encounter->_rest->dreamScene();}
 static bool fade(const XeenEventFlow &f){return f._encounter->_rest->dreamFade();}
 static std::uint64_t deadline(const XeenEventFlow &f){return f._encounter->_rest->dreamDeadline;}
 static unsigned charges(const XeenEventFlow &f){return f._encounter->_rest->charges;}
 static void pending(XeenEventFlow &f,unsigned value){auto &e=*f._encounter;e._state._pending=value;f._encounterFrame=e.ticket();}
 static void scene(XeenEventFlow &f,const XeenGameplayContext &context) {
  f._encounterCompose=[&context](auto,auto) {
   auto image=training_test::frame();image.pixels[0]=(context.minutes<300 || context.minutes>=1260)?2:1;
   for(unsigned n=0;n<image.palette.size();++n)image.palette[n]=(n%64)<<2;
   return XeenEventFlow::Composition{image,false};
  };
 }
 static IndexedFrame background(const XeenEventFlow &f){return f._encounter->_rest->background;}
 static std::string completionText(const XeenEventFlow &f) {
  const auto &rest=*f._encounter->_rest;
  return xeenDialogFormat(f.dosText().scalar("REST_COMPLETE"),{
   std::string(f.dosText().scalar(rest.starving?"PARTY_IS_STARVING":"HIT_SPELL_POINTS_RESTORED")),std::to_string(rest.consumed)});
 }
};
}
namespace {
// Rest fixtures retain the pre-M78 dryland component. Standing and resting at
// an admitted saved location do not require the skills needed to walk there.
constexpr XeenMovement::Capabilities kRestFixtureTraversal{false,false,false,true};
void wire(Fixture &f){
 f.flow->loadRestDream=[&]{return f.in.assets.restDreamImage();};
 f.flow->loadSampleDuration=[&](const char *name,XeenSceneArchive archive){
  return xeenVocDurationNanoseconds(f.in.assets.readArchiveResource(name,archive));
 };
}
void pulse(Fixture &f){f.now+=100;f.flow->beginCycle(++f.cycle);if(const auto frame=f.flow->updatePresentation())f.present(*frame);}
void finish(Fixture &f) {
 for(unsigned n=0;n<1000;++n) {
  if(XeenRestTestAccess::falling(*f.flow)) {
   const auto input=f.flow->inputContext(f.flow->frame().presentation());
   check(!input.readyForAction && input.dialog && !input.dialog->key('z'),"Fall exposed the pending Rest summary input");
   f.act(AcknowledgeAction{});
   check(f.w.sessionState().journeyActivity()==XeenJourneyActivity::Event,"Fall input retired Rest before terrain publication");
   pulse(f);continue;
  }
  if(XeenRestTestAccess::confirm(*f.flow)) {f.act(DialogKeyAction{'y'});continue;}
  if(XeenRestTestAccess::complete(*f.flow)) {
   if(XeenRestTestAccess::portrait(*f.flow)){pulse(f);continue;}
   const auto oracle=drawXeenErrorScroll(f.flow->frame(),f.in.font,XeenRestTestAccess::completionText(*f.flow));
   check(oracle.pixels==f.flow->frame().pixels,"Rest completion exact DAT text/colors in window 6");
   const auto input=f.flow->inputContext(f.flow->frame().presentation()).dialog;
   check(input && input->key('z') && input->click(319,0) && input->click(0,0),"Rest summary ordinary-key/either-mouse parity");
   f.act(*input->click(0,0));continue;
  }
  if(f.flow->canSave())return;
  check(!f.flow->encounter()->combat(),"Successful Rest fixture entered combat");pulse(f);
 }
 throw std::runtime_error("Rest did not reach a quiet boundary: "+f.flow->encounter()->notice());
}
std::vector<XeenActor> actors(Inputs &in,const XeenSaveSnapshot &s) {
 auto result=XeenActorApproach::actorsFromResources(in.maps.loadObjects(in.assets,s.camera.mapId),in.statistics);
 const auto &saved=s.map(s.camera.mapId).monsters->slots;
 for(unsigned n=0;n<result.size();++n) {
  // A load rebuilds MON HP, asleep (decision 8).
  result[n].x=saved[n].x;result[n].y=saved[n].y;if(saved[n].lifecycle==XeenActorLifecycle::Defeated)result[n].hp=0;
  result[n].activated=false;result[n].lifecycle=saved[n].lifecycle;
 }
 return result;
}
XeenSaveSnapshot source(Inputs &in,bool indoor,bool nearby=false,bool ranged=false,bool seam=false) {
 auto s=indoor?in.service():in.base();XeenWorld world(in.mapLoader(),in.objectLoader());
 const auto live=actors(in,s);const auto mainland=XeenMovement::component(world.map(23),9,11,kRestFixtureTraversal);
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
  s.camera=camera; // Saved monsters load asleep; the arrival view wakes them (decision 8).
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
 const auto draw=[&](IndexedFrame &image,const char *name,unsigned id,int x,int y){in.assets.drawDialogSprite(image,name,id,x,y);};
 check(drawXeenConfirm(declined.flow->frame(),in.font,std::string(in.assets.uiText().scalar("SOME_CHARS_MAY_DIE")),false,draw).pixels==declined.flow->frame().pixels,
  "Rest warning uses original DAT text, confirmation window and sprites");
 declined.act(DialogKeyAction{'n'});finish(declined);
 check(XeenSaveFormat::encode(declined.snapshot())==before,"Declined Rest preserves gameplay/time/RNG");
 auto valid=source(in,true,false,false,true);
 const auto restricted=[&](auto id){auto map=in.maps.loadGeometryMap(in.assets,id);if(id==XeenMapIdentity(28))map.geometry.flags|=0x4000;
  if(id!=XeenMapIdentity(23))for(auto &cell:map.geometry.cells)cell.flags=0xf8;return map;};
 Fixture refusal(in,valid,false,nullptr,restricted);wire(refusal);const auto refusalBefore=XeenSaveFormat::encode(refusal.snapshot());
 refusal.act(RestAction{});check(XeenRestTestAccess::refused(*refusal.flow),"Primary Rest restriction");
 check(drawXeenErrorScroll(refusal.flow->frame(),in.font,std::string(in.assets.uiText().scalar("TOO_DANGEROUS_TO_REST"))).pixels==refusal.flow->frame().pixels,
  "Rest refusal exact DAT text/window 6");
 const auto refusalInput=refusal.flow->inputContext(refusal.flow->frame().presentation()).dialog;
 check(refusalInput && refusalInput->key('z') && refusalInput->click(0,0),"Rest refusal generic acknowledgment");
 refusal.act(*refusalInput->key('z'));finish(refusal);
 check(XeenSaveFormat::encode(refusal.snapshot())==refusalBefore,"Rest refusal preserves gameplay");
 const auto cellFlags=[&](auto id){auto map=in.maps.loadGeometryMap(in.assets,id);if(id!=XeenMapIdentity(23))for(auto &cell:map.geometry.cells)cell.flags=0xf8;return map;};
 Fixture allowed(in,valid,false,nullptr,cellFlags);wire(allowed);allowed.act(RestAction{});finish(allowed);
 const auto neighborRestriction=[&](auto id){auto map=in.maps.loadGeometryMap(in.assets,id);if(id==XeenMapIdentity(109) || id==XeenMapIdentity(110) || id==XeenMapIdentity(111))map.geometry.flags|=0x4000;return map;};
 Fixture neighbor(in,valid,false,nullptr,neighborRestriction);wire(neighbor);neighbor.act(RestAction{});finish(neighbor);
}
void interruption(Inputs &in,bool ranged,unsigned pending,bool indoor=false) {
 auto start=source(in,indoor,true,ranged);start.journey->random=XeenCombatRandom(ranged?17:23).continuation();
 check(start.camera.x==(indoor?6:ranged?3:2) && start.camera.y==(ranged?8:1) && start.camera.direction==XeenDirection::North,
  "Fixed interruption camera changed");
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
 const unsigned interruptingCharge=ranged?1:2;
 check(f.p.encounterContext->minutes==480+interruptingCharge*(indoor?1:10) &&
  f.w.sessionState().journeyRandom()->count==(ranged?4:0),"Fixed interrupting charge/RNG count changed");
 check(contact==!ranged && (f.flow->encounter()->state().phase()==XeenEncounterPhase::Engaged)==!ranged,
  "Fixed wake reason must distinguish contact from ranged-only return");
 for(unsigned n=0;n<6;++n) {
  const auto &member=f.p.roster.at(start.activeRosterIds[n]);
  check(member.conditions[8]==(ranged && n<2?0:1),"Fixed awake/asleep member mask changed");
  check(member.currentHp==(ranged && n<2?int(n==0?993:994):1000),"Fixed interruption injuries changed");
 }
 XeenConsequenceCharacters observed;for(unsigned n=0;n<6;++n)observed[n]=f.p.roster.at(start.activeRosterIds[n]);
 check(xeenRestRangedWake(observed)==ranged,"Literal Depressed/Confused/Good ranged wake reason changed");
 const auto beforeActors=actors(in,start);const auto live=f.w.sessionState().regionalActors(f.c.mapId);
 for(unsigned n=0;n<live.size();++n) {
  int x=beforeActors[n].x,y=beforeActors[n].y;
  if(indoor) {if(n>=14 && n<=16){x=5;y=3;}if(n==31 || n==32){x=6;y=1;}}
  else if(ranged) {if(n==7 || n==8){x=3;y=9;}}
  else {if(n==12){x=4;y=0;}if(n==17){x=2;y=2;}if(n==18){x=2;y=1;}}
  check(live[n].x==x && live[n].y==y,"Fixed interruption actor positions changed");
 }
 if(ranged) {
  const auto &shots=*f.flow->encounter()->result().consequences;
  check(shots.count==2 && shots.shots[0].source.recordIndex==7 && shots.shots[1].source.recordIndex==8 &&
   shots.shots[0].attack.targetedMembers==1 && shots.shots[1].attack.targetedMembers==2,"Fixed ranged source/target order changed");
 }
 std::cout<<"Fixed interruption charge="<<interruptingCharge<<" reason="<<(ranged?"ranged-only":"contact")<<" passed\n";
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
 check(stillAsleep,"Combat/save restoration cleared unhit members' persistent sleep");
 std::cout<<"Interrupted-rest aftermath asleep="<<stillAsleep<<" exact round-trip\n";
}
void reentrant(Inputs &in) {
 const auto start=source(in,true);Fixture f(in,start);wire(f);unsigned calls=0;
 const auto input=f.flow->displayedInput();const auto frame=f.flow->frame().presentation();
 f.flow->loadRestDream=[&] {
  ++calls;f.flow->handle(RestAction{},input,frame);
  check(!f.p.roster.at(start.activeRosterIds[0]).conditions[8] && f.p.encounterContext==start.journey->context && f.p.food==start.food,
   "Fallible Rest preflight published sleep/time/food early");
  return in.assets.restDreamImage();
 };
 f.act(RestAction{});finish(f);check(calls==1 && f.p.food==start.food-6,"Reentrant Rest entry double-charged");
}
void sampleFailure(Inputs &in) {
 const auto start=source(in,true);
 for(bool invalid:{false,true}) {
  Fixture f(in,start);wire(f);
  f.flow->loadSampleDuration=[invalid](const char*,XeenSceneArchive)->std::uint64_t {
   if(invalid)return 0;
   throw std::runtime_error("Synthetic Rest sample metadata failure");
  };
  bool failed=false;try{f.act(RestAction{});}catch(const std::exception &){failed=true;}
  check(failed && !XeenRestTestAccess::active(*f.flow),"Invalid sample metadata admitted Rest");
  check(f.p.food==start.food && f.p.encounterContext==start.journey->context &&
   f.w.sessionState().journeyRandom()==start.journey->random,"Sample preflight failure published food/time/RNG");
  for(auto owner:start.activeRosterIds)check(f.p.roster.at(owner).conditions==start.characters[owner].conditions &&
   f.p.roster.at(owner).currentHp==start.characters[owner].currentHp,"Sample preflight failure published sleep/recovery");
 }
}void combatOutcomes(Inputs &in,bool run) {
 auto start=source(in,false,true);start.journey->random=XeenCombatRandom(run?101:29).continuation();
 for(auto owner:start.activeRosterIds)start.characters[owner].currentHp=run?1000:1;
 Fixture f(in,start);wire(f);f.act(RestAction{});
 {
 struct QuietOutput {std::ostringstream output;std::streambuf *before;QuietOutput():before(std::cout.rdbuf(output.rdbuf())){}~QuietOutput(){std::cout.rdbuf(before);}} quiet;
 for(unsigned n=0;n<3000;++n) {
  const auto combat=f.flow->encounter()->combat();
  if(run && f.flow->canSave())break;
  if(!run && f.flow->encounter()->state().reason()==XeenEncounterStop::Defeat)break;
  if(combat && combat->phase()==XeenCombatPhase::PlayerReady)f.act(run?PlayerAction{RunAction{}}:PlayerAction{AttackAction{}});
  else if(f.w.sessionState().journeyActivity()==XeenJourneyActivity::Reward)f.act(AcknowledgeAction{});
  else pulse(f);
 }
 }
 check(!XeenRestTestAccess::active(*f.flow) && f.p.food==start.food && !f.p.encounterContext->rested,"Run/death cannot resume or complete interrupted Rest");
 if(run)check(f.flow->canSave() && f.c.x==10 && f.c.y==12,"Interrupted Rest uses original Run destination and quiet continuation");
 // M85 Part 0: defeat retires combat to reach the Event callers before death
 // presentation; the encounter retains the terminal result after retirement.
 else check(f.flow->encounter()->state().reason()==XeenEncounterStop::Defeat &&
  f.flow->encounter()->combatResult().phase==XeenCombatPhase::Defeat && !f.flow->canSave(),"Interrupted Rest uses ordinary party-death handling");
 std::cout<<"Interrupted Rest "<<(run?"Run":"defeat")<<" passed\n";
}
void conditionDeath(Inputs &in) {
 auto start=source(in,true);start.journey->context->minutes=1439;
 for(auto owner:start.activeRosterIds)start.characters[owner].conditions[2]=255; // Unsigned Weak is lethal before its eight-hour replacement.
 Fixture f(in,start);wire(f);f.act(RestAction{});check(XeenRestTestAccess::confirm(*f.flow),"All-member stat danger confirmation missing");
 f.act(DialogKeyAction{'y'});
 for(unsigned n=0;n<500 && !XeenRestTestAccess::complete(*f.flow);++n)pulse(f);
 check(XeenRestTestAccess::complete(*f.flow) && f.p.encounterContext->minutes==479 && f.p.food==start.food,
  "Condition deaths must finish eight hours and the original completion, consuming no food for dead members");
 for(auto owner:start.activeRosterIds)check(f.p.roster.at(owner).conditions[13] && !f.p.roster.at(owner).conditions[8],"Condition-dead member recovery order");
 f.act(AcknowledgeAction{});check(f.flow->encounter()->state().reason()==XeenEncounterStop::Defeat && !f.flow->canSave(),"Rest's final party-death check missing");
}
void noTargets(Inputs &in) {
 auto start=source(in,false,true,true);start.journey->context->minutes=479;
 start.journey->random=XeenCombatRandom(17).continuation();
 for(auto owner:start.activeRosterIds)start.characters[owner].conditions[2]=254;
 Fixture f(in,start);wire(f);XeenRestTestAccess::pending(*f.flow,3);
 f.act(RestAction{});check(XeenRestTestAccess::confirm(*f.flow),"noTargets danger confirmation");f.act(DialogKeyAction{'y'});
 for(unsigned n=0;n<500 && XeenRestTestAccess::active(*f.flow);++n)pulse(f);
 const auto &result=f.flow->encounter()->result();
 check(!XeenRestTestAccess::active(*f.flow) && !f.flow->encounter()->combat() &&
  f.flow->encounter()->state().reason()==XeenEncounterStop::Defeat,"Ranged noTargets did not end Rest in ordinary defeat");
 check(result.consequences && result.consequences->count && !result.view.engaged(),"noTargets must be ranged-only, not contact");
 for(unsigned n=0;n<result.consequences->count;++n)
  check(!result.consequences->shots[n].attack.targetedMembers && !result.consequences->shots[n].attack.injuryCount,"noTargets unexpectedly selected/injured a member");
 check(f.p.food==start.food && f.p.encounterContext->minutes==489 && !f.p.encounterContext->rested && !f.flow->canSave(),"noTargets published recovery/completion or remaining hours");
 // The first charge crosses minute 480; that eight-hour block kills the members
 // (Weak 254) and clears their sleep (DOS 0x253fb..0x25424), not the shots.
 for(auto owner:start.activeRosterIds)check(f.p.roster.at(owner).conditions[13] && !f.p.roster.at(owner).conditions[8],"noTargets: the eight-hour block clears sleep");
 // The healthy condition block draws nothing. Each of the two shooters still
 // draws an initial target among participants before finding no eligible one.
 check(result.consequences->count==2 && f.w.sessionState().journeyRandom()->count==2 &&
  f.w.sessionState().journeyRandom()->state==1150042706u,"Only the two noTargets selection draws advance RNG");
 const auto original=actors(in,start);const auto live=f.w.sessionState().regionalActors(23);
 for(unsigned n=0;n<live.size();++n)check(live[n].x==(n==7 || n==8?3:int(original[n].x)) &&
  live[n].y==(n==7 || n==8?9:int(original[n].y)),"noTargets fixed actor positions changed");
 std::cout<<"noTargets charge=1 draws=2 passed\n";
}
void sleepingHit(Inputs &in) {
 // Synthetic supported Sleep/ranged MON profile on a city-only species.
 // Restoration binds both actors and the city's immutable statistics to this
 // same fixture; every production geometry/identity/publication guard runs.
 auto start=in.service();start.camera={28,11,1,XeenDirection::South};
 start.journey->random=XeenCombatRandom(1).continuation();
 for(auto owner:start.activeRosterIds)start.characters[owner].currentHp=1000;
 start.characters[0].characterClass=XeenCharacterClass::Knight;
 start.characters[0].permanentLevel=1;start.journey->supplements[0].inputs.luck->permanent=0;
 for(unsigned n=0;n<start.map(28).monsters->slots.size();++n) {
  auto &a=start.map(28).monsters->slots.at(n);if(a.lifecycle!=XeenActorLifecycle::Present)continue;
  if(n==0){a.x=11;a.y=4;}
  else {a.x=a.y=-128;a.lifecycle=XeenActorLifecycle::Defeated;a.accounted=true;}
 }
 const auto previous=in.statistics[2];auto &monster=in.statistics[2];
 monster.raw[24]=1;monster.raw[25]=unsigned(start.characters[0].characterClass);monster.raw[26]=1;monster.raw[27]=0;
 monster.raw[28]=1;monster.raw[29]=0;monster.raw[30]=9;monster.raw[31]=5;monster.raw[32]=1;
 const auto loader=[&](auto id){auto map=in.maps.loadGeometryMap(in.assets,id);
  if(id!=XeenMapIdentity(23))for(auto &cell:map.geometry.cells){cell.rawWord=0;xeenGet<XeenIndoorWalls>(cell.geometry).walls.fill(0);}return map;};
 Fixture f(in,start,false,nullptr,loader);in.statistics[2]=previous;wire(f);XeenRestTestAccess::pending(*f.flow,3);
 f.act(RestAction{});if(XeenRestTestAccess::confirm(*f.flow))f.act(DialogKeyAction{'y'});
 for(unsigned n=0;n<500 && XeenRestTestAccess::active(*f.flow) && !XeenRestTestAccess::charges(*f.flow);++n)pulse(f);
 check(XeenRestTestAccess::active(*f.flow) && XeenRestTestAccess::charges(*f.flow)==1 && !f.flow->encounter()->combat(),"Sleeping ranged hit incorrectly ended Rest");
 check(f.p.roster.at(0).currentHp==999 && f.p.roster.at(0).worstCondition()==XeenCondition::Asleep &&
  f.p.encounterContext->minutes==481 && f.p.food==start.food,"Sleep ability must leave the hit member Asleep without recovery");
 const auto live=f.w.sessionState().regionalActors(28);
 check(live[0].x==11 && live[0].y==3 && f.w.sessionState().journeyRandom()->count==2,"Sleeping hit movement/RNG auto-hit oracle changed");
 for(auto owner:start.activeRosterIds)check(f.p.roster.at(owner).conditions[8],"Sleeping hit unexpectedly woke a member");
 for(unsigned n=0;n<500 && XeenRestTestAccess::active(*f.flow);++n)pulse(f);
 check(!XeenRestTestAccess::active(*f.flow) && f.flow->encounter()->combat() && f.p.encounterContext->minutes==483 &&
  f.p.food==start.food,"Rest must continue after sleepy hit until later contact");
 std::cout<<"Sleeping hit continues after charge 1; contact interrupts charge 3\n";
}
// Admission is independent of traversal skills. Refusal controls instead
// simulate a missing destination resource, with no retained destination state.
void omitFallDestination(XeenSaveSnapshot &state) {
 state.maps.erase(std::remove_if(state.maps.begin(),state.maps.end(),
  [](const XeenSaveMapState &m){return m.id==XeenMapIdentity(23);}),state.maps.end());
}
XeenMap missingFallLanding(XeenMap map) {
 if(map.identity()==XeenMapIdentity(23))throw std::runtime_error("Synthetic missing fall destination resource");
 return map;
}
void terrain(Inputs &in) {
 const auto manifest=[&](unsigned surface) -> XeenRegionalManifest {
  // Explicit synthetic surface table, checked in addition to every original
  // manifest invariant. Restore only that fixture field in a detached copy
  // for the original validator; no production guard or source file changes.
  return [&,surface](const auto &m,const auto &o,const auto &e,const auto &s) {
   for(auto value:m.geometry.surfaceTypes)check(value==surface,"Synthetic terrain table changed");
   auto original=m;original.geometry.surfaceTypes=in.maps.loadGeometryMap(in.assets,23).geometry.surfaceTypes;
   xeenValidateRegionalManifest(original,o,e,s,in.assets.readInitialResource("maze0023.dat"),in.assets.readInitialResource("maze0023.mob"),in.assets.readInitialResource("maze0023.evt"));
  };
 };
 // Scan a superset of Vertigo's reachable cells, including all neighbor tiles,
 // and the mainland traversal component. Use the primary map's surface table.
 XeenWorld world(in.mapLoader(),in.objectLoader());
 const auto mainland=XeenMovement::component(world.map(23),9,11,kRestFixtureTraversal);
 for(bool indoor:{false,true}) {
  std::array<unsigned,16> count{};unsigned cells=0;
  const auto primary=indoor?28u:23u;
  for(int y=0;y<(indoor?32:16);++y)for(int x=0;x<(indoor?32:16);++x) {
   if(!indoor && !mainland[y*16+x])continue;
   const auto sampled=world.sampleCell(primary,x,y);if(!sampled)continue;
   ++cells;++count[world.map(primary).geometry.surfaceTypes[sampled->cell->surfaceIndex]];
  }
  std::cout<<"Surface audit map="<<primary<<" cells="<<cells;
  for(unsigned s:{5u,6u,10u,13u,15u}){std::cout<<" surface"<<s<<'='<<count[s];check(!count[s],"Original reachable hazardous Rest surface found");}
  std::cout<<'\n';
 }
 for(bool indoor:{false,true})for(unsigned surface:{5u,6u,10u,13u}) {
  auto start=source(in,indoor);
  for(auto owner:start.activeRosterIds)start.characters[owner].permanentLevel=100;
  if(surface==13)start.journey->context->effects[XeenPartyEffect::Levitate]=1;
  const bool unavailableLanding=indoor && (surface==10 || surface==13);
  if(unavailableLanding)omitFallDestination(start);
  const auto loader=[&](auto id) {
   auto map=in.maps.loadGeometryMap(in.assets,id);if(id==start.camera.mapId)map.geometry.surfaceTypes.fill(surface);
   return unavailableLanding?missingFallLanding(std::move(map)):map;
  };
  Fixture f(in,start,false,nullptr,loader,indoor?XeenRegionalManifest{}:manifest(surface));wire(f);unsigned providers=0;
  f.flow->loadRestDream=[&]{++providers;return in.assets.restDreamImage();};
  const auto before=XeenSaveFormat::encode(f.snapshot());f.act(RestAction{});
  if(indoor && (surface==10 || surface==13)) {
   check(XeenRestTestAccess::refused(*f.flow) && !providers,"Missing fall destination was not refused before Rest");
   f.act(AcknowledgeAction{});finish(f);check(XeenSaveFormat::encode(f.snapshot())==before,"Refused fall leaked Rest state");continue;
  }
  check(!XeenRestTestAccess::refused(*f.flow) && providers,"Hazardous surface incorrectly refuses Rest");
  const bool successfulFall=!indoor && (surface==10 || surface==13);
  unsigned fallCompositions=0;
  if(successfulFall) {
   // At this retained Rest boundary all ten sleeping opportunities and the
   // remainder have finished; the next operation refills, then falls. Wound
   // one distant original actor here so only the map-load lifetime can refill
   // it. No original resource or production admission is changed.
   for(unsigned n=0;!XeenRestTestAccess::recovering(*f.flow);++n) {
    check(n<1000 && !XeenRestTestAccess::complete(*f.flow),"Rest fall did not expose its recovery boundary");pulse(f);
   }
   const auto wounded=XeenTrainingTestAccess::woundBeforeFall(*f.flow);
   XeenRestTestAccess::transitionComposer(*f.flow,[&,wounded](auto &candidate,const auto &party,const auto &camera,auto,auto) {
    ++fallCompositions;
    if(fallCompositions==1) {
     check(save_test::sameCamera(camera,start.camera),"Fall must capture its original source view");
     const auto staged=candidate.sessionState().regionalActors(23);
     check(staged[wounded].hp==1 && staged[wounded].activated,"Fall source must precede the destination reload");
     for(auto owner:start.activeRosterIds)check(party.roster.at(owner).currentHp==XeenCharacterRules::maxHp(party.roster.at(owner),{610}),
      "Fall source must follow Rest refill and precede damage");
     return XeenEventFlow::Composition{frame(),false};
    }
    check(camera.mapId==XeenMapIdentity(23) && camera.x==12 && camera.y==10,"Fall composed an intermediate destination");
    const auto live=f.w.sessionState().regionalActors(23),staged=candidate.sessionState().regionalActors(23);
    check(live[wounded].hp==1 && live[wounded].activated,"Fall reload escaped before destination publication");
    check(staged[wounded].hp==staged[wounded].statistics->baseHp() && !staged[wounded].activated,
     "Fall destination must reload original MON HP/asleep before composing damage/publication");
    for(auto owner:start.activeRosterIds) {
     const auto maximum=XeenCharacterRules::maxHp(party.roster.at(owner),{610});
     check(f.p.roster.at(owner).currentHp==maximum,"Fall damage escaped before destination publication");
     check(party.roster.at(owner).currentHp==maximum-100,"Fall applies fresh physical100 independently after Rest refill");
    }
    return XeenEventFlow::Composition{frame(),false};
   });
  }
  finish(f);check(f.p.encounterContext->minutes==start.journey->context->minutes+480+(surface==6 && !indoor?170:0),"Rest terrain suffix time");
  if(successfulFall) {
   check(fallCompositions==2,"Fall must capture source/destination once each and reuse them through publication");
   for(auto owner:start.activeRosterIds)check(f.p.roster.at(owner).currentHp==XeenCharacterRules::maxHp(f.p.roster.at(owner),{610})-100,
    "Published fall damage differs from the prepared physical100");
   const auto saved=f.snapshot();const auto bytes=XeenSaveFormat::encode(saved);
   Fixture loaded(in,XeenSaveFormat::decode(bytes),false,nullptr,loader,manifest(surface));wire(loaded);
   check(XeenSaveFormat::encode(loaded.snapshot())==bytes,"Current-format fall restore replayed terrain/time/RNG or changed actor positions");
   pulse(loaded);
   check(XeenSaveFormat::encode(loaded.snapshot())==bytes,"First restored presentation replayed fall arrival");
  }
  if(surface==5)for(auto owner:start.activeRosterIds)check(f.p.roster.at(owner).currentHp<XeenCharacterRules::maxHp(f.p.roster.at(owner),{610}),"Lava damages after refill");
  if(surface==10 || surface==13)check(f.c.mapId==XeenMapIdentity(23) && f.c.x==12 && f.c.y==10 && f.c.direction==XeenDirection::North,"Rest fall uses staged shared destination and loses Levitate");
 }
 {
  // Synthetic Navigator skill restored through the authoritative saved owner.
  // Commercial resources remain read-only, and all production owners/guards run.
  auto start=source(in,false);start.characters[start.activeRosterIds[0]].skills[10]=1;
  const auto loader=[&](auto id){auto map=in.maps.loadGeometryMap(in.assets,id);if(id==start.camera.mapId)map.geometry.surfaceTypes.fill(6);return map;};
  Fixture f(in,start,false,nullptr,loader,manifest(6));wire(f);f.act(RestAction{});finish(f);
  check(f.p.encounterContext->minutes==start.journey->context->minutes+480,"Navigator did not suppress desert addTime");
 }
 // Actual original map13 lava, and a map25 sky fall use original geometry,
 // not synthetic resource replacements. Shared Journey admission must resolve
 // the same destination without applying the arrival itself.
 XeenWorld original(in.mapLoader(),in.objectLoader());const auto start=in.base();
 XeenConsequenceCharacters c;XeenConsequenceInputs inputs;
 for(unsigned n=0;n<6;++n){const auto owner=start.activeRosterIds[n];c[n]=start.characters[owner];inputs[n]=start.journey->supplements[owner].inputs;c[n].currentHp=1000;}
 const auto lava=original.prepareTerrainArrival({13,9,11,XeenDirection::North},c,*start.journey->context);
 check(lava.surface==5 && lava.damage==100 && lava.type==XeenDamageType::Fire,"Original map13 lava arrival");
 const auto originalActors=XeenActorApproach::actorsFromResources(original.objectFile(13),in.statistics);
 const auto fixtureView=XeenActorApproach::classify(originalActors,{13,8,11,XeenDirection::East});
 std::cout<<"Hazard fixture: --event-fixture <game-dir> 13 8 11 east; immediateContact="<<fixtureView.engaged()<<'\n';
 const auto restFixtureView=XeenActorApproach::classify(originalActors,{13,9,11,XeenDirection::North});
 check(!restFixtureView.engaged(),"Direct original lava Rest fixture has immediate contact");
 std::cout<<"Direct Rest fixture: --event-fixture <game-dir> 13 9 11 north; immediateContact="<<restFixtureView.engaged()<<'\n';
 XeenCombatRandom random(7);XeenTerrainArrivalCandidate effect(lava,c,inputs,*start.journey->context,&*start.journey->serviceEconomy);
 for(unsigned n=0;;++n){check(n<100,"Original lava bounded");XeenConsequenceDraw draw{random,64,{}};if(effect.service(draw))break;}
 for(const auto &v:effect.characters)check(v.currentHp<1000,"Original lava damage reaches every member");
 bool found=false;
 for(int y=0;y<16 && !found;++y)for(int x=0;x<16 && !found;++x) {
  const auto rule=original.prepareTerrainArrival({25,x,y,XeenDirection::West},c,*start.journey->context);
  if(!rule.falling)continue;
  check(rule.transition.destination.mapId==XeenMapIdentity(24) && rule.transition.destination.x==11 && rule.transition.destination.y==9,"Original map25 fall destination");
  original.markEncounterSession(XeenEncounterEntry::Journey);
  const auto admitted=original.prepareTerrainArrival({25,x,y,XeenDirection::West},c,*start.journey->context);
  check(admitted && admitted.transition.destination.mapId==XeenMapIdentity(24) &&
   admitted.transition.destination.x==11 && admitted.transition.destination.y==9 &&
   admitted.transition.destination.direction==XeenDirection::West && admitted.damage==100,
   "Original fall uses shared Journey destination admission");found=true;
 }
 check(found,"Original map25 sky witness absent");
 {
  auto state=source(in,false);for(auto owner:state.activeRosterIds){state.characters[owner].permanentLevel=100;state.characters[owner].currentHp=1000;}
  for(auto owner:state.activeRosterIds)for(auto &weapon:state.characters[owner].weapons)if(weapon.frame==4)weapon.frame=0;
  const auto loader=[&](auto id){auto map=in.maps.loadGeometryMap(in.assets,id);if(id==XeenMapIdentity(23))map.geometry.surfaceTypes.fill(5);return map;};
  Fixture f(in,state,false,nullptr,loader,manifest(5));wire(f);
  const auto idle=XeenSaveFormat::encode(f.snapshot());
  f.act(ShootAction{});finish(f);check(XeenSaveFormat::encode(f.snapshot())==idle,"No shooter must not apply lava arrival or spend a draw");
  f.act(CastSpellAction{});f.act(CancelInteractionAction{});finish(f);
  check(XeenSaveFormat::encode(f.snapshot())==idle,"Cancelled Cast must not apply lava arrival or spend a draw");
  const auto beforeMinutes=f.p.encounterContext->minutes;const auto beforeHp=f.p.roster.at(state.activeRosterIds[0]).currentHp;
  f.act(NavigationAction::TurnLeft);finish(f);
  check(f.p.encounterContext->minutes==beforeMinutes && f.p.roster.at(state.activeRosterIds[0]).currentHp<beforeHp,"Turning reapplies lava without chargeStep time");
 }
 {
  auto state=source(in,true);const auto cell=original.sampleCell(state.camera.mapId,state.camera.x,state.camera.y);
  bool blocked=false;for(unsigned d=0;d<4;++d)if(wallAt(*cell->cell,XeenDirection(d))>=original.map(28).geometry.difficulties[0]) {
   state.camera.direction=XeenDirection(d);blocked=true;break;
  }
  check(blocked,"Original blocked movement witness absent");
  const auto loader=[&](auto id){auto map=in.maps.loadGeometryMap(in.assets,id);if(id==XeenMapIdentity(28))map.geometry.surfaceTypes.fill(5);return map;};
  Fixture f(in,state,false,nullptr,loader);wire(f);const auto before=XeenSaveFormat::encode(f.snapshot());
  f.act(NavigationAction::MoveForward);finish(f);
  check(XeenSaveFormat::encode(f.snapshot())==before,"Blocked movement must not apply lava arrival");
 }
 {
  auto state=source(in,true);
  const auto loader=[&](auto id){auto map=in.maps.loadGeometryMap(in.assets,id);if(id==XeenMapIdentity(28))map.geometry.surfaceTypes.fill(15);return map;};
  Fixture f(in,state,false,nullptr,loader);wire(f);f.act(RestAction{});
  for(unsigned n=0;n<1000 && !XeenRestTestAccess::complete(*f.flow);++n)pulse(f);
  check(XeenRestTestAccess::complete(*f.flow),"Space Rest reaches completion before party defeat");
  for(auto owner:state.activeRosterIds)check(!f.p.roster.at(owner).conditions[13],"Space must not invent member Dead conditions");
 }
}
void terrainCallsites(Inputs &in) {
 const auto armed=[&](bool indoor) {
  auto s=indoor?in.service():in.base();s.camera=indoor?XeenCamera{28,20,20,XeenDirection::North}:XeenCamera{23,9,11,XeenDirection::East};
  for(auto &map:s.maps)if(map.monsters)for(auto &a:map.monsters->slots) {a.x=a.y=-128;a.lifecycle=XeenActorLifecycle::Defeated;a.accounted=true;}
  for(auto owner:s.activeRosterIds) {
   auto &c=s.characters[owner];c.currentHp=1000;c.permanentLevel=100;
   for(auto &w:c.weapons)if(w.frame!=4)w.frame=0;
   for(auto *items:{&c.armor,&c.accessories})for(auto &item:*items)item.frame=0;
   s.journey->supplements[owner].inputs.resistances=XeenCombatResistances{};
  }
  s.journey->context->effects.fill(0);s.journey->context->lightAndResistances.fill(0);
  return s;
 };
 const auto lavaLoader=[&](auto id) {auto m=in.maps.loadGeometryMap(in.assets,id);if(id==XeenMapIdentity(23))m.geometry.surfaceTypes.fill(5);return m;};
 const XeenRegionalManifest lavaManifest=[&](const auto &m,const auto &o,const auto &e,const auto &s) {
  auto original=m;original.geometry.surfaceTypes=in.maps.loadGeometryMap(in.assets,23).geometry.surfaceTypes;
  xeenValidateRegionalManifest(original,o,e,s,in.assets.readInitialResource("maze0023.dat"),in.assets.readInitialResource("maze0023.mob"),in.assets.readInitialResource("maze0023.evt"));
 };
 for(unsigned action=0;action<4;++action) {
  auto state=armed(false);auto &caster=state.characters[state.activeRosterIds[4]];
  const unsigned spell=action==3?40u:1u;caster.characterClass=XeenCharacterClass::Druid;caster.hasSpells=true;caster.currentSp=100;
  for(auto &w:caster.weapons)w.frame=0;
  caster.learnedSpells.emplace();caster.currentSpell=*XeenLearnedSpellRules::slotForSpell(XeenSpellCategory::Druidic,spell);caster.learnedSpells->at(caster.currentSpell)=1;
  Fixture f(in,state,false,nullptr,lavaLoader,lavaManifest);wire(f);
  if(action==0)f.act(WaitAction{});
  else if(action==1)f.act(ShootAction{});
  else {f.act(CastSpellAction{});f.act(SelectMemberAction{4});f.act(DialogKeyAction{'c'});}
  finish(f);
  for(auto owner:state.activeRosterIds)check(f.p.roster.at(owner).currentHp==900,"Wait/Shoot/Cast/learnedJump must each apply one lava arrival");
  check(f.p.encounterContext->minutes==state.journey->context->minutes+10,"Arrival callsite charged wrong time");
  if(action==3)check(f.c.x==11 && f.c.y==11,"Learned Jump reached wrong lava destination");
 }
 const auto row=[](unsigned x,unsigned y,unsigned opcode,std::vector<std::uint8_t> p={}) {
  XeenEventRecord r;r.x=x;r.y=y;r.direction=4;r.line=0;r.opcode=opcode;r.parameters=std::move(p);r.lengthField=5+r.parameters.size();return r;
 };
 for(unsigned kind=0;kind<3;++kind) {
  auto state=armed(true);const bool automatic=kind==1,refused=kind==2;if(automatic)state.camera.y=19;
  if(refused)omitFallDestination(state);
  XeenEventFile script{28,"maze0028.evt",true,automatic?
   std::vector<XeenEventRecord>{row(20,20,0x12)}:
   std::vector<XeenEventRecord>{row(20,20,0x1f,{28,20,21}),row(20,21,0x1f,{28,20,22}),row(20,22,0x12)}};
  const auto loader=[&](auto id) {
   auto m=in.maps.loadGeometryMap(in.assets,id);if(!m.geometry.isOutdoors()) {
    m.geometry.surfaceTypes[0]=0;m.geometry.surfaceTypes[1]=refused?10:5;
    for(auto &cell:m.geometry.cells){cell.rawWord=0;cell.geometry=XeenIndoorWalls{};cell.rawAttributes=1;cell.surfaceIndex=1;cell.flags=0;}
    // Logical(20,19) is physical(4,3), the automatic move's safe source.
    auto &start=m.geometry.cells[3*16+4];start.rawAttributes=0;start.surfaceIndex=0;
    if(automatic && id==XeenMapIdentity(111))m.geometry.cells[4*16+4].rawAttributes|=kXeenAutomaticEventFlag;
   }return refused?missingFallLanding(std::move(m)):m;
  };
  Fixture f(in,state,false,&script,loader);wire(f);const auto before=f.snapshot();
  f.act(automatic?PlayerAction{NavigationAction::MoveForward}:PlayerAction{InteractionAction{}});finish(f);
  if(refused){
   // Part 0: teleport clears Space, so no Wait tests the missing fall landing.
   auto expected=before;expected.camera.y=22;save_test::sameSnapshot(expected,f.snapshot());
  }
  else {
   check(f.c.x==20 && f.c.y==(automatic?20:22),"Event arrival published an intermediate teleport");
   // Part 0: teleport/map load clears Space; only the automatic move charges time.
   check(f.p.encounterContext->minutes==state.journey->context->minutes+(automatic?1:0),"Teleport chain retained a cleared Space Wait");
   for(auto owner:state.activeRosterIds)check(f.p.roster.at(owner).currentHp==(automatic?900:1000),
    "Cleared Space must not add a terrain arrival; automatic movement arrives once");
  }
 }
 // The original mainland sign used the old direct-publication branch. It must
 // now receive the same outdoor Space suffix as the detached city Events.
 for(bool sign:{false,true}) {
  auto state=armed(false);state.camera=sign?XeenCamera{23,5,9,XeenDirection::North}:XeenCamera{23,10,11,XeenDirection::North};
  const auto script=in.events.load(23);
  check(sign?xeenRegionalSign(script,state.camera):!xeenRegionalEvent(script,state.camera),"Manual Space fixture source changed");
  Fixture f(in,state,false,nullptr,lavaLoader,lavaManifest);wire(f);f.act(InteractionAction{});finish(f);
  for(auto owner:state.activeRosterIds)check(f.p.roster.at(owner).currentHp==900,"Mainland sign/no-Event Space omitted or repeated lava arrival");
  check(f.p.encounterContext->minutes==state.journey->context->minutes+10 && f.p.encounterContext->ctr24==(state.journey->context->ctr24+1)%24,
   "Mainland sign/no-Event Space must charge10 then call stepTime once");
 }
 // Dawn expires Levitate before cloud arrival, including Event-free Space.
 const auto cloudLoader=[&](auto id){auto m=in.maps.loadGeometryMap(in.assets,id);if(id==XeenMapIdentity(23))m.geometry.surfaceTypes.fill(13);return m;};
 for(bool sign:{false,true}) {
  auto state=armed(false);state.camera=sign?XeenCamera{23,5,9,XeenDirection::North}:XeenCamera{23,10,11,XeenDirection::North};
  state.journey->context->minutes=299;state.journey->context->newDay=true;state.journey->context->rested=true;state.journey->context->effects[XeenPartyEffect::Levitate]=1;
  Fixture f(in,state,false,nullptr,cloudLoader,lavaManifest);wire(f);f.act(InteractionAction{});finish(f);
  check(f.c.x==12 && f.c.y==10 && f.c.direction==XeenDirection::North && f.p.encounterContext->minutes==309 && !f.p.encounterContext->effects[XeenPartyEffect::Levitate],
   "Space must charge across dawn before checking cloud protection/fall");
  for(auto owner:state.activeRosterIds)check(f.p.roster.at(owner).currentHp==900,"Post-dawn Space fall must apply physical100 once");
 }
 // A flag and a visible physical damage prefix remain detached when the
 // completed manual chain has a missing fall destination. No prefix may leak.
 {
  auto state=armed(true);state.questFlags[3]=false;
  omitFallDestination(state);
  auto flag=row(20,20,0x0c,{0,0,104,3});auto damage=row(20,20,0x16,{10,0,0});damage.line=1;
  // Part 0: retain Space here; a teleport would clear it and suppress the fall.
  auto end=row(20,20,0x12);end.line=2;
  XeenEventFile script{28,"maze0028.evt",true,{flag,damage,end}};
  const auto loader=[&](auto id){auto m=in.maps.loadGeometryMap(in.assets,id);if(!m.geometry.isOutdoors()) {
   m.geometry.surfaceTypes.fill(10);for(auto &c:m.geometry.cells){c.rawWord=0;c.geometry=XeenIndoorWalls{};c.flags=0;}
  }return missingFallLanding(std::move(m));};
  Fixture f(in,state,false,&script,loader);wire(f);const auto bytes=XeenSaveFormat::encode(f.snapshot());
  f.act(InteractionAction{});bool portrait=false;
  for(unsigned n=0;!f.flow->canSave();++n){check(n<1000,"Refused prefix fixture did not settle");portrait=portrait || XeenRestTestAccess::eventPortrait(*f.flow);pulse(f);}
  check(portrait,"Refused fall fixture did not reach its visible damage prefix");
  check(XeenSaveFormat::encode(f.snapshot())==bytes,"Refused post-Event fall leaked a published flag/damage/time/RNG prefix");
 }
 // Already displayed damage/sparkle prefixes are not armed again by final
 // publication. Their Event holds must not grow a standalone presentation tail.
 for(bool sparkle:{false,true}) {
  auto state=armed(true);auto select=row(20,20,0x0f,{0});auto damage=sparkle?row(20,20,0x0c,{0,0,16,1,0,0,0}):row(20,20,0x16,{10,0,0});damage.line=1;auto exit=row(20,20,0x12);exit.line=2;
  XeenEventFile script{28,"maze0028.evt",true,{select,damage,exit}};
  const auto loader=[&](auto id){auto m=in.maps.loadGeometryMap(in.assets,id);if(!m.geometry.isOutdoors()) {
   m.geometry.surfaceTypes.fill(0);for(auto &c:m.geometry.cells){c.rawWord=0;c.geometry=XeenIndoorWalls{};c.flags=0;}
  }return m;};
  Fixture f(in,state,false,&script,loader);wire(f);f.act(InteractionAction{});bool shown=false;
  for(unsigned n=0;!f.flow->canSave();++n) {
   check(n<1000,"Successful damage prefix did not settle");
   if(XeenRestTestAccess::portrait(*f.flow)){shown=true;check(XeenRestTestAccess::eventPortrait(*f.flow),"Final manual publication replayed an already displayed damage portrait");}
   pulse(f);
  }
  check(shown && f.p.encounterContext->minutes==state.journey->context->minutes+1,"Successful portrait prefix omitted the Space suffix");
  for(auto owner:state.activeRosterIds)check(f.p.roster.at(owner).currentHp==(sparkle?1000:990) &&
   f.p.roster.combatInputs(owner)->experience==state.journey->supplements[owner].inputs.experience+unsigned(sparkle),"Successful damage/sparkle prefix was lost or applied twice");
 }
 // Use the existing supported ranged profile to prove the Event suffix keeps
 // travel, pre-HP overlay and post-HP hold, then arrives. Compare every saved
 // value with the existing Wait owner, including random continuation and actors.
 {
  constexpr unsigned pending=0;
  auto state=armed(true);state.camera={28,11,1,XeenDirection::North};state.journey->random=XeenCombatRandom(1).continuation();
  state.characters[0].characterClass=XeenCharacterClass::Knight;state.characters[0].permanentLevel=1;
  state.journey->supplements[0].inputs.luck->permanent=0;
  auto &actor=state.map(28).monsters->slots[0];actor.x=11;actor.y=pending?5:4;actor.lifecycle=XeenActorLifecycle::Present;actor.accounted=false;
  const auto previous=in.statistics[2];auto &monster=in.statistics[2];
  monster.raw[24]=1;monster.raw[25]=unsigned(state.characters[0].characterClass);monster.raw[26]=1;monster.raw[27]=0;
  monster.raw[28]=1;monster.raw[29]=0;monster.raw[30]=9;monster.raw[31]=5;monster.raw[32]=1;
  const auto loader=[&](auto id){auto m=in.maps.loadGeometryMap(in.assets,id);if(!m.geometry.isOutdoors()) {
   m.geometry.surfaceTypes.fill(5);for(auto &c:m.geometry.cells){c.rawWord=0;c.geometry=XeenIndoorWalls{};c.flags=0;}
  }return m;};
  auto choice=row(11,1,0x20,{0,0}),exit=row(11,1,0x12);exit.line=1;
  XeenEventFile script{28,"maze0028.evt",true,pending?std::vector<XeenEventRecord>{choice,exit}:std::vector<XeenEventRecord>{row(11,1,0x12)}};
  Fixture manual(in,state,false,&script,loader),wait(in,state,false,&script,loader);in.statistics[2]=previous;wire(manual);wire(wait);
  XeenRestTestAccess::pending(*wait.flow,pending);
  bool retried=false,travel=false,overlay=false,hold=false,blocked=false;
  XeenRestTestAccess::transitionComposer(*manual.flow,[&](auto &,const auto &,const auto &,auto,auto) {
   if(XeenRestTestAccess::manualReady(*manual.flow) && !retried){retried=true;throw std::runtime_error("Synthetic final Space frame allocation failure");}
   return XeenEventFlow::Composition{frame(),false};
  });
  const auto initialRandom=manual.w.sessionState().journeyRandom();manual.act(InteractionAction{});
  if(pending){check(manual.flow->canCancelInteraction(),"Owed opportunity fixture did not reach WhoWill");XeenRestTestAccess::pending(*manual.flow,pending);manual.act(SelectMemberAction{0});}
  for(unsigned n=0;!manual.flow->canSave();++n) {
   check(n<1000,"Retained manual ranged opportunity did not settle");
   travel=travel || XeenRestTestAccess::manualTravel(*manual.flow);
   const auto portrait=XeenRestTestAccess::manualPortrait(*manual.flow);overlay=overlay || portrait==0;hold=hold || portrait==1;
   if(XeenRestTestAccess::manualCandidate(*manual.flow))check(manual.p.roster.at(0).currentHp==1000 && manual.p.encounterContext->minutes==state.journey->context->minutes &&
    manual.w.sessionState().journeyRandom()==initialRandom,"Manual opportunity published damage/time/RNG before final suffix admission");
   if(portrait==0)check(XeenRestTestAccess::stagedHp(*manual.flow,0)==1000,"Pre-HP overlay applied ranged injury early");
   if(portrait==1)check(XeenRestTestAccess::stagedHp(*manual.flow,0)==999,"Post-HP hold did not retain the worked physical1 injury");
   if(travel && !blocked){blocked=true;manual.act(InteractionAction{});}
   pulse(manual);
  }
  wait.act(WaitAction{});finish(wait);
  check(travel && overlay && hold && retried,"Manual Space lost travel/portrait holds or retry coverage");
  check(manual.p.roster.at(0).currentHp==899 && manual.w.sessionState().regionalActors(28)[0].y==3,
   "Manual Space must service owed/forced opportunities, apply original physical1, then lava100");
  save_test::sameSnapshot(manual.snapshot(),wait.snapshot());
  check(XeenSaveFormat::encode(manual.snapshot())==XeenSaveFormat::encode(wait.snapshot()),"Manual Space retry repeated Events/opportunities or differs from the shared Wait suffix");
 }

 // An owed chargeStep opportunity and Space's forced opportunity remain two
 // distinct operations. Inject the countdown only after ordinary Event entry;
 // normal UI admission still starts Space at its existing quiet boundary.
 {
  auto state=armed(true);auto choice=row(20,20,0x20,{0,0}),text=row(20,20,0x31,{0,0}),exit=row(20,20,0x12);text.line=1;exit.line=2;
  XeenEventFile script{28,"maze0028.evt",true,{choice,text,exit}};
  const auto loader=[&](auto id){auto m=in.maps.loadGeometryMap(in.assets,id);if(!m.geometry.isOutdoors()) {
   m.geometry.surfaceTypes.fill(5);for(auto &c:m.geometry.cells){c.rawWord=0;c.geometry=XeenIndoorWalls{};c.flags=0;}
  }return m;};
  Fixture manual(in,state,false,&script,loader),wait(in,state,false,&script,loader);wire(manual);wire(wait);unsigned opportunities=0;
  XeenRestTestAccess::transitionComposer(*manual.flow,[&](auto &,const auto &,const auto &,auto,auto) {
   if(XeenRestTestAccess::manualReady(*manual.flow))opportunities=XeenRestTestAccess::manualOpportunities(*manual.flow);
   return XeenEventFlow::Composition{frame(),false};
  });
  manual.act(InteractionAction{});check(manual.flow->canCancelInteraction(),"Owed opportunity fixture did not reach WhoWill");
  XeenRestTestAccess::pending(*manual.flow,3);manual.act(SelectMemberAction{0});
  // Part 0: the member code replaces Space; final text response W replaces it again.
  manual.act(WaitAction{});finish(manual);
  XeenRestTestAccess::pending(*wait.flow,3);wait.act(WaitAction{});finish(wait);
  check(opportunities==2,"Replacement W did not consume both owed and forced opportunities");
  check(XeenSaveFormat::encode(manual.snapshot())==XeenSaveFormat::encode(wait.snapshot()),"Manual owed-opportunity settlement differs from existing Wait or leaks its Event ticket");
 }

}
void rolloverAndDream(Inputs &in,const GameInstallation &installation,bool efx=true,bool device=false) {
 auto start=source(in,true);start.food=3;start.journey->context->day=99;start.journey->context->minutes=1439;
 start.journey->context->effects[XeenPartyEffect::WizardEye]=1;start.journey->context->lightAndResistances[0]=4;
 start.characters[start.activeRosterIds[0]].conditions[2]=255;
 Fixture f(in,start);wire(f);f.act(RestAction{});finish(f);
 check(f.p.encounterContext->day==0 && f.p.encounterContext->year==611 && f.p.encounterContext->minutes==479 &&
  !f.p.encounterContext->newDay && f.p.roster.at(start.activeRosterIds[0]).conditions[2]==0,"Rest year rollover, sleeping dawn and unconditional Weak replacement");
 check(f.p.roster.at(start.activeRosterIds[0]).conditions[13] && f.p.food==0,"Weak 255 kills before replacement and makes the member food-ineligible");
 const auto save=f.snapshot();Fixture loaded(in,save);wire(loaded);f.act(RestAction{});loaded.act(RestAction{});finish(f);finish(loaded);
 f.act(NavigationAction::TurnLeft);loaded.act(NavigationAction::TurnLeft);finish(f);finish(loaded);
 save_test::sameSnapshot(f.snapshot(),loaded.snapshot());
 auto dreamStart=source(in,true);dreamStart.food=8;dreamStart.journey->context->minutes=1000;
 // Healthy eight-hour checks draw nothing; seed 17's first raw value
 // is 4596240, yielding 1 for the subsequent inclusive 1..20 dream draw.
 constexpr unsigned chosen=17;
 {
  auto context=*dreamStart.journey->context;XeenConsequenceCharacters c;XeenConsequenceInputs inputs;
  for(unsigned n=0;n<6;++n) {c[n]=dreamStart.characters[kXeenCombatOwners[n]];c[n].conditions[8]=1;inputs[n]=dreamStart.journey->supplements[kXeenCombatOwners[n]].inputs;}
  XeenCombatRandom random(chosen);
  for(unsigned step=0;step<11;++step) {
   XeenConditionTimeCandidate time(context,step<10?1:470,c,inputs,&*dreamStart.journey->serviceEconomy,XeenTimeMode::Sleeping);
   for(unsigned n=0;;++n) {check(n<1000,"Dream oracle bounded");XeenConsequenceDraw draw{random,64,{}};if(time.service(draw))break;}
   context=time.context;c=time.characters;inputs=time.inputs;
  }
  check(random.position()==0 && random.draw(1,20)==1,"Healthy tick leaves the first draw for the dream");
 }
 check(chosen,"Fixed dream seed absent");dreamStart.journey->random=XeenCombatRandom(chosen).continuation();
 dreamStart.soundOptions.efx=efx;
 Fixture dream(in,dreamStart);wire(dream);
 std::vector<std::string> voices;
 dream.programState.audio=XeenAudio::installed(installation,in.assets,0,device);
 dream.programState.audio->digitalSink([&](const auto &name,auto){voices.push_back(name);});
 unsigned sampleReads=0;const auto originalDuration=dream.flow->loadSampleDuration;
 dream.flow->loadSampleDuration=[&](const char *name,XeenSceneArchive archive){++sampleReads;return originalDuration(name,archive);};XeenRestTestAccess::scene(*dream.flow,*dream.p.encounterContext);
 dream.act(RestAction{});
 using D=XeenRestTestAccess::Dream;
 unsigned frames=0,fadeFrames=0,intervals=0,retries=0;
 std::array<unsigned,8> stages{};
 std::optional<XeenJourneyRandomState> cursor;unsigned minutes=0;std::optional<unsigned> overall;
 const auto dreamImage=in.assets.restDreamImage();std::optional<IndexedFrame> restored;
 check(dreamImage.pixels==in.assets.readArchiveResource("scene1.raw",XeenSceneArchive::Darkside),"Rest dream pixels are original INTRO scene1.raw");
 for(unsigned n=0;!XeenRestTestAccess::dream(*dream.flow);++n){check(n<500,"Chosen dream not reached");pulse(dream);}
 const auto foodBefore=dream.p.food;
 auto step=[&](std::uint64_t at,bool acquire=true) {
  dream.now=at;dream.flow->beginCycle(++dream.cycle);
  const auto next=dream.flow->updatePresentation();if(next && acquire)dream.present(*next);return next;
 };
 for(unsigned n=0;n<200 && XeenRestTestAccess::dream(*dream.flow);++n) {
  ++frames;
  const auto stage=XeenRestTestAccess::dreamPhase(*dream.flow);
  const auto phase=unsigned(stage);++stages[phase];
  if(!cursor){cursor=dream.w.sessionState().journeyRandom();minutes=dream.p.encounterContext->minutes;}
  if(!overall)overall=dream.w.scenePresentation().overallFrame;
  check(dream.w.scenePresentation().overallFrame==*overall,"Dream advanced ordinary scene animation");
  check(dream.w.sessionState().journeyRandom()==cursor && dream.p.encounterContext->minutes==minutes && dream.p.food==foodBefore,
   "Dream consumes no gameplay RNG, time or recovery food");
  const auto background=XeenRestTestAccess::background(*dream.flow);restored=background;
  check(background.pixels[0]==2 && minutes==40,"Dream retained wrong pre-changeTime background");
  const bool scene=XeenRestTestAccess::dreamScene(*dream.flow);
  check(dream.flow->frame().pixels==(scene?dreamImage.pixels:background.pixels),"Dream original image/background order");
  check(dream.flow->inputContext(dream.flow->frame().presentation()).hideCursor==scene,"Dream cursor transition order");
  const auto val=XeenRestTestAccess::scale(*dream.flow);
  for(unsigned i=0;i<background.palette.size();++i)
   check(dream.flow->frame().palette[i]==(((i%64)<<2)*val*2>>8),"Dream palette arithmetic");
  const auto input=dream.flow->displayedInput();const auto frame=dream.flow->frame().presentation();
  for(const PlayerAction action:{PlayerAction{CancelInteractionAction{}},PlayerAction{RestAction{}},PlayerAction{AcknowledgeAction{}}}) {
   dream.act(action);check(XeenRestTestAccess::dream(*dream.flow) && XeenRestTestAccess::dreamPhase(*dream.flow)==stage && !dream.flow->canSave(),
    "Escape/input skipped dream or admitted a save");
  }
  const auto deadline=XeenRestTestAccess::deadline(*dream.flow);
  check(dream.flow->nextPresentationDeadline()==deadline,"Dream platform deadline disagrees with retained stage");
  if(XeenRestTestAccess::fade(*dream.flow))++fadeFrames;
  else {
   ++intervals;
   const auto now=PresentationClock::fromMilliseconds(dream.now);
   const auto expected=stage==D::BeforeDreams || stage==D::AfterLaff?PresentationClock::after(now,7):
    PresentationClock::checkedAdd(now,efx?(stage==D::Dreams?(24480ULL-10+1)*68000:(28925ULL-10+1)*68000):0);
   check(deadline==expected,"Dream single-reset seven ticks or exact sequential VOC duration");
  }
  // A repaint is not another original palette write, and the deadline is a
  // strict gate even when the requested sample ends between milliseconds.
  const auto justBefore=(deadline-1)/1000000;
  if(justBefore>=dream.now)check(!step(justBefore),"Dream advanced before its exact stage deadline");
  const auto due=PresentationClock::ceilMilliseconds(deadline)+(stage==D::BeforeDreams?1000:0);
  if(stage==D::FadeInDream && XeenRestTestAccess::fadeStep(*dream.flow)==17)
   dream.flow->beforeEncounterFrameCopy=[&]{++retries;throw std::runtime_error("Synthetic dream upload failure");};
  const auto next=step(due,false);check(bool(next),"Dream did not publish its next stage at deadline");
  dream.flow->beforeEncounterFrameCopy={};
  const auto retainedPhase=XeenRestTestAccess::dreamPhase(*dream.flow);
  const auto retainedStep=XeenRestTestAccess::fadeStep(*dream.flow);
  check(!step(due,false),"Unacquired dream frame advanced on another host wake");
  check(XeenRestTestAccess::dreamPhase(*dream.flow)==retainedPhase && XeenRestTestAccess::fadeStep(*dream.flow)==retainedStep,
   "Host wake skipped a retained dream frame");
  // Acquisition after a failed upload is the retrace reset for fades; sample
  // and BIOS holds retain the original call's deadline instead.
  dream.now=due;dream.present(*next);
  dream.flow->handle(CancelInteractionAction{},input,frame);
 }
 check(retries==1,"Dream frame retry witness did not run");
 check(frames==136 && fadeFrames==132 && intervals==4,"Dream must retain four 33-level fades and four whole intervals");
 check(stages[0]==33 && stages[1]==33 && stages[2]==1 && stages[3]==1 && stages[4]==1 && stages[5]==1 && stages[6]==33 && stages[7]==33,
  "Dream retained stage ordering/counts");
 check(restored && !dream.flow->inputContext(dream.flow->frame().presentation()).hideCursor,"Dream restores cursor before recovery");
 const auto recoveryInput=dream.flow->displayedInput();const auto recoveryFrame=dream.flow->frame().presentation();
 finish(dream);const auto after=XeenSaveFormat::encode(dream.snapshot());
 dream.flow->handle(AcknowledgeAction{},recoveryInput,recoveryFrame);
 check(XeenSaveFormat::encode(dream.snapshot())==after && dream.p.food==2,"Dream recovery must publish exactly once despite stale input");
 check(efx?voices==std::vector<std::string>{"dreams2.voc","laff1.voc"}:voices.empty(),"Dream digital starts/gate/repaint duplication");
 check(efx?sampleReads==2:sampleReads==0,"Efx-off Rest queried VOC metadata/busy span");
 std::cout<<"Dream seed="<<chosen<<" four 33-level retrace fades; 7 ticks, 1.664504s, 1.966764s, 7 ticks passed\n";
}
}
int main(int argc,char **argv) {try {
 check(argc==2,"Rest original usage: installation");const auto installation=xeenTestInstallationDetector().detect(argv[1]);check(bool(installation),"Original installation absent");
 Inputs in(*installation);sleepingHit(in);terrain(in);terrainCallsites(in);success(in,false);success(in,true);entry(in);interruption(in,false,0);interruption(in,true,3);interruption(in,false,0,true);rolloverAndDream(in,*installation);rolloverAndDream(in,*installation,false);rolloverAndDream(in,*installation,true,true);reentrant(in);sampleFailure(in);combatOutcomes(in,true);combatOutcomes(in,false);conditionDeath(in);noTargets(in);
 std::cout<<"Original-resource Rest, interruption, food, UI path and mid-sequence save tests passed\n";return 0;
 }catch(const std::exception &e){std::cerr<<e.what()<<'\n';return 1;}}
