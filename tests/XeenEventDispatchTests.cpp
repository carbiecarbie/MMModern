#include "XeenRegionalSaveGameplayTestSupport.h"
#include "XeenMonsterTestAccess.h"
#include "games/xeen/XeenEventInterpreter.h"
#include "games/xeen/XeenJourneyCapture.h"
#include "games/xeen/XeenEventTrigger.h"
#include <iostream>
#define SDL_MAIN_HANDLED
#include <SDL.h>
#include "app/XeenTitleFlow.h"
#include "platform/sdl/XeenMainScreenInput.h"
using namespace gameplay_test;
namespace mmodern {
struct XeenEventDispatchTestAccess {
 static bool death(const XeenEventFlow &f){return f._death.has_value();}
 static bool won(const XeenEventFlow &f){return f._encounter->_retiredCombatResult.phase==XeenCombatPhase::Victory;}
 static bool owed(const XeenEventFlow &f){return f._explorationPass || f._encounter->_regionalAutomatic || f._encounter->_postCombatCalls;}
 static void whoWill(XeenEventFlow &f){auto value=f._world.treasureStaging().value();value.whoWill=3;f._world.treasureStaging().publish(value);f._encounter->retainJourney();}
 static void treasure(XeenEventFlow &f){auto value=f._world.treasureStaging().value();value.gold=123;f._world.treasureStaging().publish(value);f._encounter->retainJourney();}
 static bool door(const XeenEventFlow &f){return f._transition && f._transition->eventFlags?f._transition->doorText:f._encounter->_doorTextUp;}
 static void flags(XeenEventFlow &f,bool skipped,bool door,bool slot=false){f._program.eventCallSkipped=skipped;f._encounter->_doorTextUp=door;f._encounter->_drawnSlot0=slot;}
 static void fragileParty(XeenEventFlow &f){for(auto owner:f._party.party.activeRosterIds())f._party.roster.at(owner).currentHp=1;f._encounter->retainJourney();}
 static IndexedFrame callers(XeenEventFlow &f,bool door,bool draw,bool slot=false,bool survivingInput=false) {
  f._encounter->_postCombatCalls=true;f._encounter->_combatDoorText=door;f._encounter->_combatDrawCaller=draw;
  f._encounter->_drawnSlot0=slot;f.armExplorationPass();f._explorationPass=survivingInput;
  check(!f.canSave(),"save refused before first post-combat caller");
  save_test::rejects([&]{f.beginSave();});
  f._dispatching=true;
  try {f.prepareJourneyTransition();auto frame=f.renderEncounter();f._dispatching=false;return frame;}
  catch(...){f._dispatching=false;throw;}
 }
 static IndexedFrame engagedDraw(XeenEventFlow &f) {
  // Synthetic closed draw gate: use the real Spawn and arrival classification,
  // leaving its admitted contact for the exploration reader to consume.
  auto candidate=f._world.transitionCandidate();candidate->applySpawn(f._camera.mapId,0,f._camera.x,f._camera.y,0);
  const auto view=candidate->prepareTransitionArrival(f._camera);f._world.publishTransition(*candidate);
  auto &e=*f._encounter;e.retainJourney();e.publishArrival(view);e._drawnSlot0=bool(view.slots[0]);e.holdJourneyFrame();
  return f.renderEncounter();
 }
 static IndexedFrame rollback(XeenEventFlow &f) {
  f._dispatching=true;f._encounter->beginJourneyEvent();
  auto frame=f.journeyEventWork([&]{
   f.beginDetachedEvent(xeenRegionalInteraction(f._encounter->_journeyEvents,f._camera));
   check(f._transition && !f._transition->skipped && f._transition->doorText,"candidate has both new DOS values");
   throw std::runtime_error("injected candidate rollback");
  });f._dispatching=false;return frame;
 }
 static auto initialized(const XeenSaveResourceSignature &r,const XeenPartyState &p,const XeenCamera &c,const XeenGameFlags &f,const XeenWorld &w) {
  return XeenSaveState::captureInitialized(r,p,c,f,w);
 }
};
}
namespace {
struct Fixture {
 XeenProgramState program{mmodern::XeenWallClock::fixed(0)};
 gameplay_test::Fixture resources;
 XeenPartyState party=resources.initial;
 XeenCamera camera{1,1,1,XeenDirection::North};
 XeenGameFlags flags;
 bool flagged=true, object=false;
 XeenEventFile file{1,"test.evt",true,{record(1,1,0,0x12)}};
 XeenWorld world{[&](auto id){auto m=remove_test::map(id);for(auto &c:m.geometry.cells)c.rawAttributes=flagged?0x10:0;return m;},
  [&](auto id){XeenObjectFile o{id,"test.mob",true,{}};if(object)o.entities.objects.push_back({camera.x,camera.y,0,0,1});return o;}};
 XeenEventSystem events{[&](auto){return XeenEventScript(file);}};
 unsigned automatic=0,manual=0;
 XeenEventFlow flow{program,world,events,party,camera,flags,resources.font,
  [](auto){return XeenEventFlow::Composition{IndexedFrame{320,200,Bytes(64000)},false};}};
 Fixture() {
  flow.dialogText=&dos_test::text();
  party.encounterContext=regional_test::snapshot().journey->context;
  flow.reportAutomatic=[&](const auto &r){if(std::holds_alternative<XeenAutomaticEventCompleted>(r))++automatic;};
  flow.reportManual=[&](const auto &r){if(std::holds_alternative<XeenManualEventCompleted>(r))++manual;};
  flow.refresh(true);
 }
 void send(const PlayerAction &a){flow.handle(a);}
};

void ordinaryCartographer() {
 Fixture f;f.party.encounterContext.reset();
 f.party.roster.at(f.party.party.activeRosterIds().front()).skills[4]=1;
 check(!f.world.sampleCell(f.camera.mapId,f.camera.x,f.camera.y)->cell->stepped,"Ordinary refresh starts blank");
 f.world.scenePresentation().markLightingStep();f.flow.refresh(true);
 check(f.world.sampleCell(f.camera.mapId,f.camera.x,f.camera.y)->cell->stepped && !f.party.encounterContext,
  "Ordinary scene refresh marks without an encounter context");
 const auto explored=f.world.sessionState().exploredTiles();f.flow.refresh(true);
 check(f.world.sessionState().exploredTiles()==explored,"Ordinary reconstruction does not add marks");
}
void skippedLineHeads() {
 // M85 Part 0: 0x26797 precedes 0x26abd; every admitted scan clears 9756,
 // including no-record scans. A label draw rebuilds slots before the next head.
 for(bool between:{false,true}) {
  Fixture f;f.object=!between;bool held=!between,skipped=false;unsigned heads=0,draws=0;
  auto staging=f.world.treasureStaging().value();staging.whoWill=3;f.world.treasureStaging().publish(staging);
  f.file.records={record(1,1,0,2,{0}),record(1,1,1,0x12)};
  XeenEventInterpreter interpreter;
  interpreter.eventLineSkipped=[&]{++heads;return skipped=held;};
  interpreter.eventDraw=[&](auto &,const auto &,bool label){if(label){held=true;++draws;}};
  const auto script=[&](auto){return XeenEventScript(f.file);};
  const auto text=[](auto id){return XeenEventTextFile{id,"test.txt",true,{"A label"}};};
  auto result=interpreter.begin(f.camera,f.party,f.flags,f.world,script,text);
  check(std::holds_alternative<XeenEventExecutionSuspended>(result),"skip preserves tail or label presentation");
  const auto pending=std::get<XeenEventExecutionSuspended>(result);
  check(pending.state.treasure.whoWill==0,"even skipped entry clears WhoWill");
  check(pending.state.instructionCount==(between?1u:0u),"skip dispatches no later instructions");
  check(pending.request.kind==(between?XeenPresentationKind::SceneLabelReduced:XeenPresentationKind::NothingHere),"entry skip retains window 38 tail");
  result=interpreter.resume(pending.state,between?XeenPresentationResponse::Presented:XeenPresentationResponse::Acknowledged,
      f.party,f.world,script,text);
  check(std::holds_alternative<XeenEventExecutionCompleted>(result) && skipped,"skip finishes the ordinary call");
  check(heads==(between?2u:1u) && draws==(between?1u:0u),"one generic head covers entry and between-line abort");
  check(f.world.treasureStaging().value().whoWill==0,"completed skipped call publishes entry WhoWill clear");
  held=false;f.object=false;f.camera.x=2;f.file.records.clear();
  result=interpreter.begin(f.camera,f.party,f.flags,f.world,script,text);
  check(std::holds_alternative<XeenEventExecutionCompleted>(result) && !skipped,"no-record scan clears stale skipped flag");
 }
}
void outerInputs() {
 Fixture f;
 for(const PlayerAction &a:std::vector<PlayerAction>{WaitAction{},CancelInteractionAction{},NavigationAction::MoveForward,
    NavigationAction::TurnLeft,RestAction{},CastSpellAction{},BashAction{},ShootAction{},UseItemAction{},ExplorationKeyAction{'.'},UnsupportedMainScreenAction{"Dismiss"}}) {
  const auto before=f.automatic;f.send(a);check(f.automatic==before+1,"one pass for completed non-Journey input or refusal");
 }
 const auto before=f.automatic;f.send(InfoAction{});check(f.automatic==before,"Info retains outer pass");
 f.send(CancelInteractionAction{});check(f.automatic==before+1,"Info close completes one pass");
 f.flow.refresh();f.flow.refresh();check(f.automatic==before+1,"redraw adds no pass");
}
void manualAndFlags() {
 for(bool wait:{false,true}) {
  Fixture f;f.send(InteractionAction{wait});check(f.manual==1 && f.automatic==1,"manual interaction then one flagged pass");
 }
 Fixture f;f.flagged=false;f.world.discardMapCache();f.send(WaitAction{});check(!f.automatic,"unflagged Wait runs no automatic Event");
 f.send(InteractionAction{false});check(f.manual==1 && !f.automatic,"unflagged manual still runs");
}
void mouseViewAndPanel() {
 // Part 0: the decoder includes x224/y135, while the outer manual predicate
 // reaches the panel's y137..140 only after its Tab code has been decoded.
 for(int y=137;y<=140;++y) {
  Fixture f;unsigned panels=0;
  f.flow.continueExplorationPanel=[&](const auto &)->std::optional<IndexedFrame> {
   check(f.manual==1 && f.automatic==0,"manual Event must finish before opening the panel");++panels;return f.flow.frame();
  };
  const auto click=xeenMainScreenClick(109,y,MainScreen::Exploration);
  check(click && std::get<InteractionAction>(*click).openPanel,"top panel rows enter manual path");
  f.send(*click);check(panels==1 && f.automatic==0,"opening panel retains the input pass");
  f.flow.completeExplorationInput();check(f.automatic==1,"closing panel makes its one pass");
 }
 Fixture f;const auto click=xeenMainScreenClick(224,135,MainScreen::Exploration);
 check(click.has_value(),"inclusive DOS view corner");f.send(*click);
 check(f.manual==1 && f.automatic==1,"view click runs manual plus pass without Wait");
 for(int y=136;y<=140;++y)check(!xeenMainScreenClick(8,y,MainScreen::Exploration),"empty pixels supply no code");
}
void emptyCalls() {
 for(unsigned kind=0;kind<3;++kind) {
  Fixture f;f.file.records.clear();
  if(kind==1)f.file.records.push_back(record(1,1,0,0x12,{},1));
  if(kind==2){f.camera.x=f.camera.y=0;f.file.records.push_back(record(0,0,0,0x12));}
  f.send(WaitAction{});check(f.automatic==1,"record-less, wrong-facing and origin North calls complete normally");
 }
}
void whoWillPerPass() {
 Fixture f;f.file.records.clear();
 for(const PlayerAction &action:std::vector<PlayerAction>{WaitAction{},CancelInteractionAction{},ShootAction{}}) {
  XeenTreasureStaging staged;staged.whoWill=5;f.world.treasureStaging().publish(staged);
  f.send(action);check(f.world.treasureStaging().value().whoWill==0,"each flagged record-less pass clears WhoWill at entry 0x26797");
 }
 XeenTreasureStaging staged;staged.whoWill=4;f.world.treasureStaging().publish(staged);
 f.flagged=false;f.world.discardMapCache();f.send(WaitAction{});
 check(f.world.treasureStaging().value().whoWill==4,"unflagged pass leaves WhoWill alone");
 f.send(InteractionAction{false});check(f.world.treasureStaging().value().whoWill==0,"record-less manual call clears WhoWill too");
}
void tailMarkers() {
 // DOS: Exit (0x28d50) and opcode 47 (0x28d30) leave the marker clear;
 // CallEvent marks (0x283c0..0x28b2b).
 for(unsigned kind=0;kind<7;++kind) {
  Fixture f;f.object=true;f.file.records.clear();
  if(kind==1)f.file.records.push_back(record(1,1,0,0x12,{},1));
  if(kind==2)f.file.records.push_back(record(1,1,0,0));
  if(kind==3)f.file.records.push_back(record(1,1,0,0x12));
  if(kind==4)f.file.records={record(1,1,0,0x19,{2,2,0}),record(2,2,0,0)};
  if(kind==5)f.file.records={record(1,1,0,0x09,{20,10,2})};
  if(kind==6)f.file.records.push_back(record(1,1,0,0x2f));
  f.send(WaitAction{});
  const bool window=kind==0 || kind==2 || kind==3 || kind==6;
  check(bool(f.flow.presentationGeneration())==window,"tail uses executed and other-facing markers");
  if(window){f.send(AcknowledgeAction{});check(f.automatic==1,"tail acknowledgment finishes its existing pass");}
 }
 // A failed take ends the Event before 0x2790d marks (0x278ff).
 Fixture f;f.object=true;f.party.purse=XeenPurse{};f.party.purse->gold=500;
 f.file.records={record(1,1,0,0x0c,{34,0xff,0xff,0xff,0,0,0})};
 XeenEventInterpreter interpreter;
 const auto script=[&](auto){return XeenEventScript(f.file);};
 const auto text=[](auto id){return XeenEventTextFile{id,"test.txt",true,{}};};
 auto result=interpreter.begin(f.camera,f.party,f.flags,f.world,script,text);
 check(std::holds_alternative<XeenEventExecutionSuspended>(result) &&
  std::get<XeenEventExecutionSuspended>(result).request.notEnough==std::uint8_t(0),"failed take shows its refusal");
 result=interpreter.resume(std::get<XeenEventExecutionSuspended>(result).state,XeenPresentationResponse::Acknowledged,f.party,f.world,script,text);
 check(std::holds_alternative<XeenEventExecutionSuspended>(result) &&
  std::get<XeenEventExecutionSuspended>(result).request.kind==XeenPresentationKind::NothingHere,"failed take leaves the marker clear");
}
struct Driver {
 regional_save_test::Fixture &f;
 const SdlWindow::FrameUpdateHandler &handler;
 const SdlWindow::IdleFrameHandler &idle;
 IndexedFrame shown;
 Driver(regional_save_test::Fixture &f,const SdlWindow::FrameUpdateHandler &h,const SdlWindow::IdleFrameHandler &i,const IndexedFrame &first):
  f(f),handler(h),idle(i),shown(first){present();}
 void present(){handler.framePresented(shown.presentation());if(handler.completeInputHandoff)handler.completeInputHandoff(shown.presentation());}
 void tick(){f.now+=125;handler.beginCycle(++f.cycle);if(auto frame=idle())shown=*frame;present();}
 void send(const PlayerAction &a){present();handler.beginCycle(++f.cycle);auto token=handler.displayedInput();
  check(bool(token),"displayed outer input");if(auto frame=handler.withPresentedInput(a,*token,shown.presentation()))shown=*frame;present();}
 void settle(bool respond=false){for(unsigned n=0;!f.flow->canSave();++n){check(n<1500,"outer input settlement bounded");
  if(respond && !f.flow->canCancelInteraction()) {
   const auto context=handler.inputContext(shown.presentation());
   if(context.readyForAction && context.dialog && context.dialog->anyKey)send(AcknowledgeAction{});
  }tick();}}
};
void quietMonsters(XeenSaveSnapshot &save) {
 for(auto &m:save.maps)if(m.monsters)for(auto &a:m.monsters->slots){a.x=a.y=-128;a.lifecycle=XeenActorLifecycle::Defeated;a.accounted=true;}
}
struct JourneyCase {
 regional_save_test::Fixture f;
 unsigned calls=0,saves=0;
 bool blocked=false,records=true,allowRest=false,keepMonsters=false,flagCells=true;
 std::vector<XeenEventRecord> script{record(9,11,0,0x09,{20,10,2})};
 std::optional<XeenSaveSnapshot> captured;
 XeenTitleFlow *panel=nullptr;
 std::filesystem::path path=std::filesystem::current_path()/"m84-outer-test.mmsave";
 XeenGameplayServices services() {
  auto s=f.services();
  s.maps=[&](auto id){auto m=regional_test::map(id);for(auto &c:m.geometry.cells)c.rawAttributes|=(flagCells?0x10:0) | (allowRest?0:0x40);
   if(keepMonsters)m.geometry.difficulties[7]=101;
   if(blocked)xeenGet<XeenOutdoorLayers>(m.geometry.cells[11*16+8].geometry).middle=1;return m;};
  s.resources.loadEvents=[&](auto id){auto e=regional_test::events(id);e.records=records?script:std::vector<XeenEventRecord>{};return e;};
  const auto configure=s.configureFlow;s.configureFlow=[&,configure](auto &flow,const auto &c){configure(flow,c);
   flow.mapName=[](auto){return "Test";};flow.drawDialogSprite=[](auto &,auto,unsigned,int,int){};
   flow.loadJournalText=[](bool){return Bytes(2000,0);};
   flow.loadRestDream=[]{return IndexedFrame{320,200,Bytes(64000)};};
   flow.reportManual=[&](const auto &r){if(std::holds_alternative<XeenManualEventCompleted>(r))++calls;};
  };
  return s;
 }
 void run(XeenGameplayServices s,const std::function<void(Driver &)> &body) {
  if(!keepMonsters)quietMonsters(f.saved);XeenSaveFile::write(path,f.saved);
  s.show=[&](const auto &first,const auto &h,const auto &,const auto &idle,const auto &){Driver d(f,h,idle,first);
   check(calls==0,"arrival presentation precedes entry Event");d.tick();d.settle(true);body(d);return true;};
  check(Application(s.program).playGameplay(s,{},path,true)==0,"outer-input Application completed");
  std::filesystem::remove(path);
 }
 void addPanel(XeenGameplayServices &s) {
  s.initialSlot=0;f.saved.name="Test";
  s.writeManaged=[&](unsigned,const auto &value,const auto &checkSource){checkSource();captured=value;++saves;checkSource();};
  s.panel=[&](const auto &base,bool combat,bool restricted,bool saveable,auto slot,const auto &name){
   XeenTitleFlow::Services p{dos_test::text(),f.font,base,{}, {},[](auto &,auto,unsigned,int,int){},
    []{return std::array<XeenSaveFile::Slot,10>{};},[&](unsigned){return path;}};
   p.panel=true;p.combat=combat;p.saveRestricted=restricted;p.saveable=saveable;p.currentSlot=slot;p.currentName=name;
   auto flow=std::make_unique<XeenTitleFlow>(std::move(p));panel=flow.get();return flow;
  };
 }
};
void queuedSdlEvents() {
 for(unsigned kind=0;kind<3;++kind) {
  JourneyCase t;t.script={record(9,11,0,0x02,{0}),kind==1?record(9,11,1,0x07,{23,9,11}):record(9,11,1,0x12)};
  auto s=t.services();const auto configure=s.configureFlow;bool injected=false;unsigned settled=0,cycles=0;unsigned inputActions=0,finalMinutes=0;
  s.configureFlow=[&,configure](auto &flow,const auto &camera){configure(flow,camera);const auto report=flow.reportManual;
   flow.reportManual=[&,report](const auto &result){report(result);
    if(kind<2 && !injected && std::holds_alternative<XeenEventExecutionSuspended>(result)) {
     injected=true;
     for(const auto key:{SDLK_PERIOD,SDLK_w})for(const auto type:{SDL_KEYDOWN,SDL_KEYUP}) {
      SDL_Event e{};e.type=type;e.key.keysym.sym=key;e.key.timestamp=SDL_GetTicks();
      check(SDL_PeepEvents(&e,1,SDL_ADDEVENT,0,0)==1,"queue taps inside executed non-modal Event");
     }
    }
   };
  };
  quietMonsters(t.f.saved);XeenSaveFile::write(t.path,t.f.saved);
  s.show=[&](const auto &first,const auto &handler,const auto &cancel,const auto &idle,const auto &status){
   auto wrapped=handler;
   wrapped.beginCycle=[&](auto cycle){t.f.now+=125;++cycles;check(cycles<500,"SDL Event test bounded");handler.beginCycle(cycle);};
   wrapped.withPresentedInput=[&](const auto &a,auto token,const auto &origin){++inputActions;return handler.withPresentedInput(a,token,origin);};
   return SdlWindow().showInteractive(first,"DOS Event queue",wrapped,cancel,[&]()->std::optional<IndexedFrame>{
    auto frame=idle();
    if(t.f.flow->canSave()) {
     if(kind==2 && !injected && t.calls==1) {
      injected=true;
      for(const auto type:{SDL_MOUSEBUTTONDOWN,SDL_MOUSEBUTTONUP}) {
       SDL_Event e{};e.type=type;e.button.button=SDL_BUTTON_RIGHT;e.button.x=8;e.button.y=8;
       e.button.timestamp=SDL_GetTicks();check(SDL_PeepEvents(&e,1,SDL_ADDEVENT,0,0)==1,"right view click");
      }
     }
     if(injected && ++settled>10) {finalMinutes=t.f.party->encounterContext->minutes;SDL_Event e{};e.type=SDL_QUIT;SDL_PushEvent(&e);}
    }
    return frame;
   },status);
  };
  check(Application(s.program).playGameplay(s,{},t.path,true)==0,"production SDL Event queue completed");
  check(injected && t.calls==(kind==1?1u:3u),"DOS non-modal taps survive; teleport clears; view click runs manual plus pass");
  check(inputActions==(kind==1?0u:kind==2?1u:2u),"cleared keys or key releases made a pass");
  check(finalMinutes==t.f.saved.journey->context->minutes+(kind==0?10:0),"only queued W waits; view click and cleared W do not");
  std::filesystem::remove(t.path);
 }
}
void queuedCombatInput() {
 JourneyCase t;t.flagCells=false;t.records=false;auto s=t.services();quietMonsters(t.f.saved);
 s.objects=[](auto id){auto objects=regional_test::objects(id);for(auto &object:objects.entities.objects)object.x=object.y=1;return objects;};
 XeenSaveFile::write(t.path,t.f.saved);unsigned attacks=0,cycles=0;bool queued=false;
 s.show=[&](const auto &first,const auto &handler,const auto &cancel,const auto &idle,const auto &status){
  Driver d(t.f,handler,idle,first);d.tick();d.settle();
  d.shown=XeenEventDispatchTestAccess::engagedDraw(*t.f.flow);d.present();
  auto native=handler;native.beginCycle=[&](auto cycle){check(++cycles<300,"queued combat input bounded");t.f.now+=125;handler.beginCycle(++t.f.cycle);};
  native.withPresentedInput=[&](const auto &action,auto token,const auto &origin){
   if(std::holds_alternative<AttackAction>(action)) {check(bool(t.f.flow->encounter()->combat()),"queued A belongs to combat");++attacks;}
   return handler.withPresentedInput(action,token,origin);
  };
  return SdlWindow().showInteractive(d.shown,"DOS combat queue",native,cancel,[&]()->std::optional<IndexedFrame>{
   if(!queued){queued=true;
  for(const auto key:{SDLK_SPACE,SDLK_a})for(const auto type:{SDL_KEYDOWN,SDL_KEYUP}) {
   SDL_Event e{};e.type=type;e.key.keysym.sym=key;e.key.timestamp=SDL_GetTicks();
   check(SDL_PeepEvents(&e,1,SDL_ADDEVENT,0,0)==1,"queue Space and Attack before the admitted fight");
  }
   }
   auto frame=idle();if(attacks){SDL_Event e{};e.type=SDL_QUIT;SDL_PushEvent(&e);}return frame;
  },status);
 };
 const auto status=Application(s.program).playGameplay(s,{},t.path,true);
 check(status==0 && attacks==1,"pre-fight queued key reaches combat once");
 std::filesystem::remove(t.path);
}

void bufferedWordPrompt() {
 JourneyCase t;t.script={record(9,11,0,0x02,{0}),record(9,11,1,0x15,{1,3,0,0}),record(9,11,2,0x12),record(9,11,3,0x12)};
 auto s=t.services();s.texts=[](auto id){auto text=regional_test::texts(id);text.strings[1]="A";return text;};s.resources.loadRegionalText=s.texts;
 const auto configure=s.configureFlow;bool injected=false;unsigned cycles=0;std::string letters;unsigned confirms=0;
 s.configureFlow=[&,configure](auto &flow,const auto &camera){configure(flow,camera);const auto report=flow.reportManual;
  flow.reportManual=[&,report](const auto &result){report(result);
   if(!injected && std::holds_alternative<XeenEventExecutionSuspended>(result)) {
    injected=true;
    for(const auto code:{SDLK_a,SDLK_RETURN})for(const auto type:{SDL_KEYDOWN,SDL_KEYUP}) {
     SDL_Event e{};e.type=type;e.key.keysym.sym=code;e.key.timestamp=SDL_GetTicks();
     check(SDL_PeepEvents(&e,1,SDL_ADDEVENT,0,0)==1,"type before the Event word prompt");
    }
   }
  };
 };
 quietMonsters(t.f.saved);XeenSaveFile::write(t.path,t.f.saved);
 s.show=[&](const auto &first,const auto &handler,const auto &cancel,const auto &idle,const auto &status){
  auto native=handler;native.beginCycle=[&](auto cycle){check(++cycles<500,"buffered word prompt bounded");t.f.now+=125;handler.beginCycle(cycle);};
  native.withPresentedInput=[&](const auto &action,auto token,const auto &origin){
   if(const auto *text=std::get_if<TextInputAction>(&action))letters+=text->text;
   if(const auto *key=std::get_if<DialogKeyAction>(&action);key && key->key==13)++confirms;
   return handler.withPresentedInput(action,token,origin);
  };
  return SdlWindow().showInteractive(first,"Buffered Event word",native,cancel,[&]()->std::optional<IndexedFrame>{
   auto frame=idle();if(t.calls==1 && t.f.flow->canSave()) {
    check(letters=="a" && confirms==1,"retained printable make and Enter reach the modal reader once");
    SDL_Event e{};e.type=SDL_QUIT;SDL_PushEvent(&e);
   }return frame;
  },status);
 };
 check(Application(s.program).playGameplay(s,{},t.path,true)==0 && t.calls==1,"modal replies complete the existing outer pass only");
 std::filesystem::remove(t.path);
}
void mapLoadInputSignal() {
 JourneyCase t;auto services=t.services();
 t.run(services,[&](Driver &d){
  const auto map=t.f.camera->mapId;auto &world=*t.f.world;
  const auto before=d.handler.inputContext(d.shown.presentation()).keyboardClear;
  // Publish a real same-map reload through the existing detached map owner.
  auto candidate=world.transitionCandidate();candidate->loadEventDestination(map,map);
  check(world.inputMapLoadGeneration()==0,"unpublished map work must not clear live input");
  world.publishTransition(*candidate);
  const auto cleared=d.handler.inputContext(d.shown.presentation()).keyboardClear;
  check(cleared==before+1,"ordinary same-map load reports the DOS keyboard clear");
  check(d.handler.inputContext(d.shown.presentation()).keyboardClear==cleared,"repeated context reads do not clear again");
 });
}
void actionTable() {
 for(const PlayerAction &a:std::vector<PlayerAction>{WaitAction{},CancelInteractionAction{},
     NavigationAction::TurnLeft,NavigationAction::TurnRight,NavigationAction::MoveForward,NavigationAction::MoveBackward,
     NavigationAction::StrafeLeft,NavigationAction::StrafeRight,ToggleAutomapAction{},ShootAction{},UseItemAction{},
     BashAction{},UnsupportedMainScreenAction{"Test refusal"}}) {
  JourneyCase t;auto s=t.services();t.run(s,[&](Driver &d){const auto before=t.calls;d.send(a);d.settle(true);
   check(t.calls==before+1,"one pass per completed action, including refusals");});
 }
 JourneyCase blocked;blocked.blocked=true;auto b=blocked.services();blocked.run(b,[&](Driver &d){auto before=blocked.calls;
  const auto camera=*blocked.f.camera;d.send(NavigationAction::MoveForward);d.settle();
  check(blocked.f.camera->x==camera.x && blocked.f.camera->y==camera.y && blocked.calls==before+1,"blocked movement still passes");});
 for(const PlayerAction &open:std::vector<PlayerAction>{InfoAction{},QuickReferenceAction{},MapAction{},QuestsAction{},
     InspectInventoryAction{},SelectMemberAction{0},CastSpellAction{},RestAction{}}) {
  JourneyCase t;auto s=t.services();t.run(s,[&](Driver &d){const auto before=t.calls;d.send(open);
   check(t.calls==before,"opening a dialog retains one outer obligation");
   check(!XeenSaveState::canCapture(*t.f.party,*t.f.camera,*t.f.world),"armed dialog excludes ordinary capture");
   save_test::rejects([&]{XeenEventDispatchTestAccess::initialized(regional_test::signature(),*t.f.party,*t.f.camera,*t.f.flags,*t.f.world);});
   d.send(CancelInteractionAction{});d.settle(true);check(t.calls==before+1,"dialog cancellation or close passes exactly once");
  });
 }
}
void currentKeyAndCapture() {
 for(bool wait:{false,true}) {
  JourneyCase t;auto s=t.services();t.run(s,[&](Driver &d){const auto calls=t.calls;
   const auto context=*t.f.party->encounterContext;d.send(InteractionAction{wait});d.settle();
   check(t.calls==calls+2,"Space and 3D click make one manual call and one final pass");
   check(t.f.party->encounterContext->minutes==context.minutes+(wait?10:0) &&
    t.f.party->encounterContext->ctr24==(context.ctr24+(wait?1:0))%24,"only surviving Space advances time and the monster step");});
 }

 for(const PlayerAction &reply:std::vector<PlayerAction>{WaitAction{},DialogKeyAction{'w'},DialogKeyAction{InputKey::Left},DialogKeyAction{'i'}}) {
  JourneyCase t;t.script={record(9,11,0,0)};auto s=t.services();
  t.run(s,[&](Driver &d){const auto before=t.calls;const auto time=t.f.party->encounterContext->minutes;
   d.send(InteractionAction{false});

   check(t.f.flow->presentationGeneration().has_value(),"manual zero opcode reaches Nothing here");
   d.send(reply);
   check(!XeenSaveState::canCapture(*t.f.party,*t.f.camera,*t.f.world),"manual admission/acquisition never exposes armed capture");
   save_test::rejects([&]{XeenEventDispatchTestAccess::initialized(regional_test::signature(),*t.f.party,*t.f.camera,*t.f.flags,*t.f.world);});
   d.send(SaveGameAction{});check(t.calls==before+1,"refused F9 cannot arm a second pass over retained manual completion");
   const auto camera=*t.f.camera;d.send(NavigationAction::MoveForward);
   check(t.f.camera->x==camera.x && t.f.camera->y==camera.y,"next input cannot steal the final pass cell");
   if(std::holds_alternative<DialogKeyAction>(reply) && std::get<DialogKeyAction>(reply).key=='i') {
    d.tick();check(t.calls==before+1,"replacement Info stays within manual input");d.send(CancelInteractionAction{});
   }
   d.settle(true);check(t.calls==before+2,"manual and flagged pass each run once");
   const bool wait=std::holds_alternative<WaitAction>(reply) || (std::get_if<DialogKeyAction>(&reply) && std::get<DialogKeyAction>(reply).key=='w');
   check(t.f.party->encounterContext->minutes==time+(wait?10:0),"actual replacement key determines Wait");
  });
 }
}
void panelSavesAndContinuation() {
 for(unsigned count:{0u,1u,2u}) {
  JourneyCase t;auto s=t.services();t.addPanel(s);
  t.run(s,[&](Driver &d){const auto before=t.calls;d.send(ControlPanelAction{});
   check(t.calls==before && t.f.flow->canSave(),"panel saves precede arming the close pass");
   for(unsigned n=0;n<count;++n){d.send(DialogKeyAction{dos_test::text().buttons("PANEL")[3].key});
    check(t.calls==before && t.saves==n+1,"nested Save adds no pass");d.send(AcknowledgeAction{});}
   d.send(CancelInteractionAction{});d.settle();check(t.calls==before+1,"zero, one or two panel saves have one close pass");
  });
 }
 JourneyCase renamed;auto as=renamed.services();renamed.addPanel(as);
 renamed.run(as,[&](Driver &d){const auto before=renamed.calls;d.send(ControlPanelAction{});
  d.send(DialogKeyAction{dos_test::text().buttons("PANEL")[4].key});
  d.send(DialogKeyAction{dos_test::text().buttons("CHOOSER")[2].key});d.send(DialogKeyAction{InputKey::Enter});
  d.send(TextInputAction{"Copy"});d.send(DialogKeyAction{InputKey::Enter});
  check(renamed.saves==1 && renamed.calls==before,"Save As naming and publication are nested in the panel");
  d.send(AcknowledgeAction{});d.send(CancelInteractionAction{});d.send(CancelInteractionAction{});d.settle();
  check(renamed.calls==before+1,"Save As has one panel-close pass");});
 JourneyCase source;source.script={record(9,11,0,0x2b),record(9,11,1,0x0c,{0,0,20,25}),record(9,11,2,0x12)};
 auto s=source.services();source.addPanel(s);std::optional<XeenSaveSnapshot> uninterrupted;
 source.run(s,[&](Driver &d){d.send(ControlPanelAction{});d.send(DialogKeyAction{dos_test::text().buttons("PANEL")[3].key});
  check(bool(source.captured),"panel captured current save");d.send(AcknowledgeAction{});d.send(CancelInteractionAction{});d.settle();
  uninterrupted=source.f.capture();check(source.calls==2,"source entry plus panel-close pass");});
 JourneyCase loaded;loaded.script=source.script;loaded.f.saved=*source.captured;auto ls=loaded.services();
 loaded.run(ls,[&](Driver &d){check(loaded.calls==1,"loaded branch makes one entry pass");
  check(XeenSaveFormat::encode(loaded.f.capture())==XeenSaveFormat::encode(*uninterrupted),"Save/ack/close/pass equals Load entry in state and RNG");});
}
void f9OutcomesAndRetries() {
 for(unsigned outcome=0;outcome<3;++outcome) {
  JourneyCase t;auto s=t.services();
  if(outcome==1)s.observeSaveStage=[](auto){throw std::runtime_error("injected disk failure");};
  if(outcome==2)s.saveUnavailable="unavailable for test";
  t.run(s,[&](Driver &d){const auto before=t.calls;d.send(SaveGameAction{});d.settle();
   check(t.calls==before+1,"successful, failed and unavailable F9 each pass once");});
 }
 JourneyCase t;auto s=t.services();auto compose=s.composeEncounter;bool fail=false;
 s.composeEncounter=[&](auto &w,const auto &p,const auto &c,auto phase,auto actor){if(fail){fail=false;throw std::bad_alloc();}return compose(w,p,c,phase,actor);};
 t.run(s,[&](Driver &d){const auto before=t.calls;fail=true;d.send(WaitAction{});d.settle();check(t.calls==before+1,"presentation reconstruction adds no pass");});
}

void restAndCombat() {
 JourneyCase rested;rested.allowRest=true;rested.f.saved.food=100;auto rs=rested.services();
 rested.run(rs,[&](Driver &d){const auto before=rested.calls;d.send(RestAction{});d.send(DialogKeyAction{'y'});d.settle(true);
  check(rested.calls==before+1,"Rest confirmation and completion acknowledgments are nested in one pass");});
 for(unsigned scenario=0;scenario<4;++scenario) {
  const bool run=scenario%2;JourneyCase t;t.keepMonsters=true;
  t.script=scenario<2?std::vector<XeenEventRecord>{record(8,11,0,0x12)}:std::vector<XeenEventRecord>{};quietMonsters(t.f.saved);
  auto &actor=t.f.saved.map(23).monsters->slots[0];actor={7,11,XeenActorLifecycle::Present,XeenActorStatus::Physical,false,-1};
  t.f.saved.journey->random=XeenCombatRandom(101).continuation();
  for(auto owner:t.f.saved.activeRosterIds){auto &c=t.f.saved.characters[owner];c.currentHp=1000;c.permanentLevel=100;
   t.f.saved.journey->supplements[owner].inputs.might={200,0};t.f.saved.journey->supplements[owner].inputs.accuracy={200,0};}
  auto s=t.services();t.run(s,[&](Driver &d){const auto before=t.calls;d.send(NavigationAction::MoveForward);
   for(unsigned n=0;!t.f.flow->encounter()->combat() && n<20;++n)d.tick();
   // Part 0: the originating pass enters and skips before combat attaches.
   check(t.f.flow->encounter()->combat() && t.calls==before+1,"engaging input calls Events before combat");
   const bool skipped=t.f.programState.eventCallSkipped;
   for(unsigned n=0;!t.f.flow->canSave();++n) {
    check(n<1500,"deferred combat pass settlement bounded");
    auto *combat=t.f.flow->encounter()->combat();
    if(combat && combat->phase()==XeenCombatPhase::PlayerReady)d.send(run?PlayerAction{RunAction{}}:PlayerAction{AttackAction{}});
    else if(!combat){const auto context=d.handler.inputContext(d.shown.presentation());
     if(context.readyForAction && context.dialog && context.dialog->anyKey)d.send(AcknowledgeAction{});}
    d.tick();
   }
   // Caller 3 scans the current cell even on an unflagged Run landing.
   check(t.calls==before+1+(skipped?1:0),"only a skipped pass adds caller 3 after combat");
   const auto calls=t.calls;d.send(ExplorationKeyAction{'.'});d.settle(true);
   check(t.calls==calls+1,"the next input has exactly its ordinary flagged-cell pass");
  });
 }
}

void engagedInput() {
 for(unsigned path=0;path<3;++path) {
  const bool shoot=path==1;
  JourneyCase t;t.flagCells=false;t.records=false;
  for(auto owner:t.f.saved.activeRosterIds){auto &c=t.f.saved.characters[owner];c.currentHp=1000;c.permanentLevel=100;
   t.f.saved.journey->supplements[owner].inputs.might={200,0};t.f.saved.journey->supplements[owner].inputs.accuracy={200,0};}
  t.f.saved.characters[t.f.saved.activeRosterIds[0]].weapons[8]={0,30,0,4};
  auto s=t.services();t.addPanel(s);t.run(s,[&](Driver &d){
   XeenEventDispatchTestAccess::flags(*t.f.flow,true,false);
   d.shown=XeenEventDispatchTestAccess::engagedDraw(*t.f.flow);d.present();
   const auto input=d.handler.inputContext(d.shown.presentation());
   check(input.readyForAction,"occupied slot alone does not close exploration input");
   const auto time=t.f.party->encounterContext->minutes;const auto before=t.calls;
   d.send(shoot?PlayerAction{ShootAction{}}:PlayerAction{InteractionAction{true}});
   for(unsigned n=0;!t.f.flow->encounter()->combat();++n){check(n<100,"engaged input attaches combat");
    const auto current=d.handler.inputContext(d.shown.presentation());
    if(current.readyForAction && current.dialog && current.dialog->anyKey)d.send(AcknowledgeAction{});d.tick();}
   check(t.f.party->encounterContext->minutes==time,"admitted fight precedes manual Space Wait or Shoot charge");
   check(t.calls==before+unsigned(!shoot),"manual skip completes before combat; direct Shoot makes no entry Event");
   check(d.handler.inputContext(d.shown.presentation()).keyboardClear==input.keyboardClear,"combat entry preserves queued keys");
   // Part 0 combat-result table: Load refuses within the fight; no return
   // caller runs and no new exploration pass is owed by the nested panel.
   for(unsigned n=0;t.f.flow->encounter()->combat()->phase()!=XeenCombatPhase::PlayerReady;++n){check(n<100,"combat panel reaches ready");d.tick();}
   // Like SDL, sample input after the draw interval and before idle redraws.
   t.f.now+=125;
   const auto combatCalls=t.calls;d.send(ControlPanelAction{});
   check(t.panel && t.panel->screen()==XeenTitleFlow::Screen::Panel,"combat control panel opens");
   d.send(DialogKeyAction{dos_test::text().buttons("PANEL")[2].key});
   check(t.panel && t.panel->screen()==XeenTitleFlow::Screen::Notice && !t.panel->entry() && t.f.flow->encounter()->combat() && t.calls==combatCalls,"combat Load refuses without making either caller");
   d.send(AcknowledgeAction{});d.send(CancelInteractionAction{});
   if(path==2) {
    t.f.now+=125;d.send(ControlPanelAction{});
    check(t.panel && t.panel->screen()==XeenTitleFlow::Screen::Panel,"combat Quit panel opens");
    d.send(DialogKeyAction{dos_test::text().buttons("PANEL")[5].key});d.send(DialogKeyAction{'y'});
    check(t.panel->entry() && t.panel->entry()->kind==XeenSessionEntry::Kind::Exit && t.calls==combatCalls && t.f.programState.eventCallSkipped,"Quit leaves combat without either post-combat caller");return;
   }
   std::optional<unsigned> exitMinutes;
   for(unsigned n=0;!t.f.flow->canSave();++n){check(n<1500,"engaged input fight settles");
    auto *combat=t.f.flow->encounter()->combat();
    if(combat && combat->phase()==XeenCombatPhase::PlayerReady)d.send(AttackAction{});
    else if(!combat){if(!exitMinutes)exitMinutes=t.f.party->encounterContext->minutes;const auto current=d.handler.inputContext(d.shown.presentation());if(current.readyForAction && current.dialog && current.dialog->anyKey)d.send(AcknowledgeAction{});}
    d.tick();}
   if(shoot)check(exitMinutes && t.f.party->encounterContext->minutes==*exitMinutes+10,"direct Shoot resumes its outdoor charge after combat and before the pass");
   check(t.calls==before+(shoot?0u:2u),"manual fight has unflagged caller 3; direct Shoot has none");
   check(t.f.programState.eventCallSkipped==shoot,"direct Shoot preserves stale process flag");
   check(d.handler.inputContext(d.shown.presentation()).keyboardClear>input.keyboardClear,"ordinary combat exit clears queued keys");
  });
 }
}

void defeatCallers() {
 JourneyCase t;t.records=false;
 for(auto owner:t.f.saved.activeRosterIds){t.f.saved.characters[owner].currentHp=1;t.f.saved.characters[owner].conditions.fill(0);}
 auto s=t.services();const auto configure=s.configureFlow;
 s.configureFlow=[&,configure](auto &flow,const auto &camera){configure(flow,camera);flow.loadSampleDuration=[](const auto &,auto){return 100u;};};
 t.run(s,[&](Driver &d){
  const auto before=t.calls;const auto gold=t.f.party->purse->gold;
  XeenEventDispatchTestAccess::flags(*t.f.flow,true,true);XeenEventDispatchTestAccess::treasure(*t.f.flow);
  d.shown=XeenEventDispatchTestAccess::engagedDraw(*t.f.flow);d.present();d.send(InteractionAction{true});
  for(unsigned n=0;!XeenEventDispatchTestAccess::death(*t.f.flow);++n){check(n<2000,"defeat reaches callers and death without an ordinary pass deadlock");
   auto *combat=t.f.flow->encounter()->combat();
   if(combat && combat->phase()==XeenCombatPhase::PlayerReady)d.send(BlockAction{});
   else if(!combat){const auto input=d.handler.inputContext(d.shown.presentation());if(input.readyForAction && input.dialog && input.dialog->anyKey)d.send(AcknowledgeAction{});}
   // Delivery mode 2 retains the same visible-slot gate after defeat.
   if(t.calls>before+1)check(t.f.party->purse->gold==gold && t.f.world->treasureStaging().value().gold==123,"held contact defers combat treasure without blocking defeat callers");
   d.tick();
  }
  check(t.calls==before+3 && !XeenEventDispatchTestAccess::owed(*t.f.flow),"defeat reaches both skipped callers, then drops the deceased input pass");
 });
}

void postCombatCalls() {
 // DOS 0x2eb85 calls delivery before caller 4, but only mode 9 bypasses
 // delivery's visible-slot gate (0x2414e..0x241e4), not combat mode 2.
 for(bool visible:{false,true}) {
  JourneyCase t;t.keepMonsters=true;t.records=false;quietMonsters(t.f.saved);
  if(visible)t.f.saved.map(23).monsters->slots[1]={6,11,XeenActorLifecycle::Present,XeenActorStatus::Physical,false,-1};
  for(auto owner:t.f.saved.activeRosterIds){auto &c=t.f.saved.characters[owner];c.currentHp=1000;c.permanentLevel=100;
   t.f.saved.journey->supplements[owner].inputs.might={200,0};t.f.saved.journey->supplements[owner].inputs.accuracy={200,0};}
  auto s=t.services();t.run(s,[&](Driver &d){
   if(visible)XeenMonsterTestAccess::statusLive(*t.f.flow,*t.f.world,23,1,XeenActorStatus::DragonSleep);
   const auto gold=t.f.party->purse->gold;
   XeenEventDispatchTestAccess::flags(*t.f.flow,false,true);XeenEventDispatchTestAccess::treasure(*t.f.flow);
   d.shown=XeenEventDispatchTestAccess::engagedDraw(*t.f.flow);d.present();d.send(InteractionAction{false});
   for(unsigned n=0;!t.f.flow->encounter()->combat();++n){check(n<100,"delivery fixture reaches combat");
    const auto input=d.handler.inputContext(d.shown.presentation());if(input.readyForAction && input.dialog && input.dialog->anyKey)d.send(AcknowledgeAction{});d.tick();}
   bool caller4=false;const auto report=t.f.flow->reportManual;
   t.f.flow->reportManual=[&](const auto &result){
    if(!caller4 && std::holds_alternative<XeenManualEventCompleted>(result)) {
     caller4=true;check(XeenEventDispatchTestAccess::won(*t.f.flow),"caller 4 follows a real victory");
     const auto view=XeenActorApproach::classify(t.f.world->sessionState().regionalActors(t.f.camera->mapId),*t.f.camera);
     check(!view.slots[0] && std::any_of(view.slots.begin(),view.slots.end(),[](const auto &slot){return bool(slot);})==visible,"victory has the requested non-contact visibility");
     const auto staged=t.f.world->treasureStaging().value().gold;
     check(visible?(staged>=123 && t.f.party->purse->gold==gold):(staged==0 && t.f.party->purse->gold>=gold+123),
      "visible monster defers delivery but not caller 4; empty view delivers before caller 4");
    }
    report(result);
   };
   for(unsigned n=0;!t.f.flow->canSave();++n){check(n<1500,"victory delivery and caller settle");
    auto *combat=t.f.flow->encounter()->combat();if(combat && combat->phase()==XeenCombatPhase::PlayerReady)d.send(AttackAction{});
    else if(!combat){const auto input=d.handler.inputContext(d.shown.presentation());if(input.readyForAction && input.dialog && input.dialog->anyKey)d.send(AcknowledgeAction{});}
    d.tick();}
   check(caller4 && (visible?t.f.world->treasureStaging().value().gold>=123:t.f.world->treasureStaging().value().gold==0),"caller 4 completes with the correct staged treasure");
   t.f.flow->reportManual=report;
  });
 }
 // Part 0: callers precede the surviving input suffix. A lethal caller leaves
 // no surviving input, just as combat defeat does; it must not owe a new Pass.
 JourneyCase lethal;lethal.script={record(9,11,0,0x0f,{0}),record(9,11,1,0x16,{50,0,3}),record(9,11,2,0x12)};
 for(auto owner:lethal.f.saved.activeRosterIds)lethal.f.saved.characters[owner].currentHp=1000;
 auto lethalServices=lethal.services();const auto configure=lethalServices.configureFlow;
 lethalServices.configureFlow=[&,configure](auto &flow,const auto &camera){configure(flow,camera);flow.loadSampleDuration=[](const auto &,auto){return 100u;};};
 lethal.run(lethalServices,[&](Driver &d){const auto before=lethal.calls;
  XeenEventDispatchTestAccess::fragileParty(*lethal.f.flow);XeenEventDispatchTestAccess::flags(*lethal.f.flow,true,false);
  d.shown=XeenEventDispatchTestAccess::callers(*lethal.f.flow,true,true,false,true);d.present();
  for(unsigned n=0;!XeenEventDispatchTestAccess::death(*lethal.f.flow);++n){check(n<1000,"lethal caller reaches death without an ordinary pass deadlock");d.tick();}
  check(lethal.calls==before+1 && !XeenEventDispatchTestAccess::owed(*lethal.f.flow),"lethal caller 4 suppresses caller 3 and drops the deceased input pass");
 });
 // Part 0 combat-result table: caller 4 tests its saved door flag AND the
 // current cell; caller 3 tests only the process flag and originating draw.
 for(bool flagged:{false,true})for(bool door:{false,true})for(bool draw:{false,true})for(bool skipped:{false,true}) {
  JourneyCase t;t.flagCells=flagged;t.records=false;auto s=t.services();
  t.run(s,[&](Driver &d){const auto before=t.calls;
   XeenEventDispatchTestAccess::flags(*t.f.flow,skipped,false);XeenEventDispatchTestAccess::whoWill(*t.f.flow);
   d.shown=XeenEventDispatchTestAccess::callers(*t.f.flow,door,draw);d.present();d.settle(true);
   const bool called=(door && flagged) || (draw && skipped);
   check(t.f.world->treasureStaging().value().whoWill==(called?0:3),"every admitted caller clears WhoWill");
   check(t.calls==before+unsigned(called),"caller predicates; a scanning caller 4 suppresses caller 3");
   check(t.f.programState.eventCallSkipped==(skipped && !called),"direct Shoot preserves stale skip unless caller 4 scans");
   const auto calls=t.calls;d.send(ExplorationKeyAction{'.'});d.settle(true);
   check(t.calls==calls+unsigned(flagged),"one ordinary pass for the next input after callers");
  });
 }
 JourneyCase both;both.records=false;auto s=both.services();
 both.run(s,[&](Driver &d){const auto before=both.calls;
  XeenEventDispatchTestAccess::flags(*both.f.flow,true,false);
  d.shown=XeenEventDispatchTestAccess::callers(*both.f.flow,true,true,true);d.present();d.settle(true);
  check(both.calls==before+2 && !both.f.programState.eventCallSkipped,"skipped caller 4 leaves caller 3 owed; its scan clears the flag");
 });
 JourneyCase stale;stale.flagCells=false;stale.records=false;auto ss=stale.services();
 stale.run(ss,[&](Driver &d){XeenEventDispatchTestAccess::flags(*stale.f.flow,true,false);
  d.send(ExplorationKeyAction{'.'});d.settle();check(stale.f.programState.eventCallSkipped,"quiet unflagged input retains stale skip");
  const auto before=stale.calls;d.shown=XeenEventDispatchTestAccess::callers(*stale.f.flow,false,true);d.present();d.settle(true);
  check(stale.calls==before+1 && !stale.f.programState.eventCallSkipped,"later draw-origin fight consumes stale flag on an unflagged cell");
 });
}
void candidateFlagsAndDoorText() {
 JourneyCase t;t.script={record(9,11,0,2,{0}),record(9,11,1,0x12)};auto s=t.services();
 t.run(s,[&](Driver &d){
  check(XeenEventDispatchTestAccess::door(*t.f.flow),"label opcode sets door text");
  XeenEventDispatchTestAccess::flags(*t.f.flow,true,false);
  d.shown=XeenEventDispatchTestAccess::rollback(*t.f.flow);d.present();d.settle(true);
  check(t.f.programState.eventCallSkipped && !XeenEventDispatchTestAccess::door(*t.f.flow),"rolled-back candidate leaves both live values unchanged");
 });
 for(unsigned opcode:{2u,3u,4u,39u}) {
  JourneyCase label;label.script={record(9,11,0,opcode,{0},3),record(9,11,1,0x12,{},3)};auto ls=label.services();
  label.run(ls,[&](Driver &d){check(XeenEventDispatchTestAccess::door(*label.f.flow),"each DOS label opcode sets door text");
   d.send(InfoAction{});check(XeenEventDispatchTestAccess::door(*label.f.flow),"ordinary dialog retains door text");
   d.send(CancelInteractionAction{});d.settle(true);
   d.send(NavigationAction::TurnLeft);d.settle(true);check(!XeenEventDispatchTestAccess::door(*label.f.flow),"turn clears label when the new direction has no label");
  });
 }
}

void flagLifetimeAndInput() {
 JourneyCase t;t.flagCells=false;t.f.programState.eventCallSkipped=true;auto s=t.services();
 t.run(s,[&](Driver &d){
  check(t.f.programState.eventCallSkipped && !XeenEventDispatchTestAccess::door(*t.f.flow),"Load keeps process skip and clears gameplay door text");
  XeenEventDispatchTestAccess::flags(*t.f.flow,true,true);
  d.send(WaitAction{});d.settle();
  check(t.f.programState.eventCallSkipped && !XeenEventDispatchTestAccess::door(*t.f.flow),"unflagged Wait clears only door text");
 });
 check(t.f.programState.eventCallSkipped,"return to title retains process skip");
 JourneyCase word;word.script={record(9,11,0,2,{0}),record(9,11,1,0x15,{1,3,0,0}),record(9,11,2,0x12),record(9,11,3,0x12)};
 auto ws=word.services();const auto configure=ws.configureFlow;bool prompt=false;
 ws.configureFlow=[&,configure](auto &flow,const auto &camera){configure(flow,camera);
  const auto report=flow.reportManual;flow.reportManual=[&,report](const auto &result){report(result);
   if(const auto *p=std::get_if<XeenEventExecutionSuspended>(&result);p && p->request.kind==XeenPresentationKind::WordInput)prompt=true;
  };
 };
 quietMonsters(word.f.saved);XeenSaveFile::write(word.path,word.f.saved);
 ws.show=[&](const auto &first,const auto &h,const auto &,const auto &idle,const auto &){Driver d(word.f,h,idle,first);
  for(unsigned n=0;!prompt || !d.handler.inputContext(d.shown.presentation()).readyForAction;++n){check(n<100,"word input opens");d.tick();}
  check(!XeenEventDispatchTestAccess::door(*word.f.flow),"text input temporarily clears door text");
  d.send(CancelInteractionAction{});d.settle(true);
  check(XeenEventDispatchTestAccess::door(*word.f.flow),"text input restores its saved door flag");return true;
 };
 check(Application(ws.program).playGameplay(ws,{},word.path,true)==0,"word flag lifetime");std::filesystem::remove(word.path);
}

void newGamePublication() {
 JourneyCase t;t.f.programState.eventCallSkipped=true;auto s=t.services();s.originalStart=XeenDifficulty::Adventurer;
 s.loadInitialCamera=[]{return XeenCamera{28,18,4,XeenDirection::West};};
 s.resources.loadEvents=regional_test::events;
 s.objects=[](auto id){auto o=regional_test::objects(id);if(id==XeenMapIdentity(28))
  for(unsigned n=0;n<46;++n)o.entities.monsters.push_back({1,1,0,0,8});return o;};
 s.sampleJourneySeed=[]{return 1u;};s.saveName="New";s.initialSlot=0;
 bool published=false;
 s.publishInitial=[&](const auto &save,const auto &checkSource){checkSource();
  check(t.calls==0 && !t.f.flow->presentationGeneration(),"New publication captures before arming its pass");
  t.captured=save;published=true;return true;};
 s.show=[&](const auto &first,const auto &h,const auto &,const auto &idle,const auto &){
  check(t.f.programState.eventCallSkipped && !XeenEventDispatchTestAccess::door(*t.f.flow),"New Game preserves process skip before its first scan");
  check(published && t.calls==0,"New arrival view exists after publication and before entry pass");
  Driver d(t.f,h,idle,first);check(t.calls==0,"New presentation callback does not fabricate an Event frame");
  d.tick();d.settle(true);check(t.calls==1,"New publication completes its one entry pass after actual arrival");return true;};
 check(Application(s.program).playGameplay(s,{},std::nullopt,false,XeenEncounterEntry::Journey)==0,"New publication Application completed");
}

void journeyPasses() {
 regional_save_test::Fixture f;unsigned calls=0;
 for(auto &map:f.saved.maps)if(map.monsters)for(auto &a:map.monsters->slots) {
  a.x=a.y=-128;a.lifecycle=XeenActorLifecycle::Defeated;a.accounted=true;
 }
 auto services=f.services();
 services.maps=[](auto id){auto m=regional_test::map(id);m.geometry.cells[11*16+9].rawAttributes|=0x10;return m;};
 services.resources.loadEvents=[](auto id){auto e=regional_test::events(id);e.records={record(9,11,0,0x09,{20,10,2})};return e;};
 const auto configure=services.configureFlow;
 services.configureFlow=[&](auto &flow,const auto &camera){configure(flow,camera);
  flow.reportManual=[&](const auto &r){if(std::holds_alternative<XeenManualEventCompleted>(r))++calls;};};
 services.show=[&](const auto &,const auto &handler,const auto &,const auto &idle,const auto &) {
  f.present(handler);
  const auto settle=[&] {
   for(unsigned n=0;!f.flow->canSave();++n) {
    check(n<100,"Journey outer pass settles");f.now+=100;handler.beginCycle(++f.cycle);idle();f.present(handler);
   }
  };
  check(calls==0,"arrival view is displayed before the entry pass");
  f.now+=100;handler.beginCycle(++f.cycle);idle();f.present(handler);
  settle();check(calls==1,"Load entry runs a flagged cell exactly once");
  for(const PlayerAction &a:std::vector<PlayerAction>{WaitAction{},CancelInteractionAction{}}) {
   const auto before=calls;f.send(handler,a);settle();check(calls==before+1,"Journey Wait and Escape each pass once");
  }
  auto before=calls;f.send(handler,InfoAction{});check(calls==before,"Info opening keeps the outer obligation");
  check(!XeenSaveState::canCapture(*f.party,*f.camera,*f.world),"direct capture excludes an armed dialog pass");
  save_test::rejects([&]{XeenSaveState::capture(regional_test::signature(),*f.party,*f.camera,*f.flags,*f.world);});
  f.send(handler,CancelInteractionAction{});settle();check(calls==before+1,"Info closing runs exactly once");
  before=calls;f.send(handler,SaveGameAction{});settle();check(calls==before+1,"successful F9 captures before its following pass");
  return true;
 };
 const auto path=std::filesystem::current_path()/"m84-dispatch-test.mmsave";
 XeenSaveFile::write(path,f.saved);
 check(Application(services.program).playGameplay(services,{},path,true)==0,"Journey dispatch Application completed");
 std::filesystem::remove(path);
}

}
int main(int argc,char **) {
 try {ordinaryCartographer();if(argc>1){queuedSdlEvents();bufferedWordPrompt();queuedCombatInput();return 0;}skippedLineHeads();outerInputs();mouseViewAndPanel();mapLoadInputSignal();whoWillPerPass();manualAndFlags();emptyCalls();tailMarkers();journeyPasses();actionTable();currentKeyAndCapture();panelSavesAndContinuation();f9OutcomesAndRetries();restAndCombat();engagedInput();defeatCallers();postCombatCalls();candidateFlagsAndDoorText();flagLifetimeAndInput();newGamePublication();std::cout<<"M84 Event dispatch and tail OK\n";return 0;}
 catch(const std::exception &e){std::cerr<<e.what()<<'\n';return 1;}
}


