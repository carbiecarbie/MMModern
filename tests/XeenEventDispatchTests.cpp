#include "XeenRegionalSaveGameplayTestSupport.h"
#include "games/xeen/XeenEventInterpreter.h"
#include <iostream>
#include "app/XeenTitleFlow.h"
using namespace gameplay_test;
namespace mmodern {
struct XeenEventDispatchTestAccess {
 static auto initialized(const XeenSaveResourceSignature &r,const XeenPartyState &p,const XeenCamera &c,const XeenGameFlags &f,const XeenWorld &w) {
  return XeenSaveState::captureInitialized(r,p,c,f,w);
 }
};
}
namespace {
struct Fixture {
 XeenProgramState program;
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
void outerInputs() {
 Fixture f;
 for(const PlayerAction &a:std::vector<PlayerAction>{WaitAction{},CancelInteractionAction{},NavigationAction::MoveForward,
    NavigationAction::TurnLeft,RestAction{},CastSpellAction{},BashAction{},ShootAction{},UseItemAction{},UnsupportedMainScreenAction{"Dismiss"}}) {
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
 for(unsigned kind=0;kind<6;++kind) {
  Fixture f;f.object=true;f.file.records.clear();
  if(kind==1)f.file.records.push_back(record(1,1,0,0x12,{},1));
  if(kind==2)f.file.records.push_back(record(1,1,0,0));
  if(kind==3)f.file.records.push_back(record(1,1,0,0x12));
  if(kind==4)f.file.records={record(1,1,0,0x19,{2,2,0}),record(2,2,0,0)};
  if(kind==5)f.file.records={record(1,1,0,0x09,{20,10,2})};
  f.send(WaitAction{});
  const bool window=kind==0 || kind==2 || kind==4;
  check(bool(f.flow.presentationGeneration())==window,"tail uses executed and other-facing markers");
  if(window){f.send(AcknowledgeAction{});check(f.automatic==1,"tail acknowledgment finishes its existing pass");}
 }
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
 bool blocked=false,records=true,allowRest=false,keepMonsters=false;
 std::vector<XeenEventRecord> script{record(9,11,0,0x12)};
 std::optional<XeenSaveSnapshot> captured;
 XeenTitleFlow *panel=nullptr;
 std::filesystem::path path=std::filesystem::current_path()/"m84-outer-test.mmsave";
 XeenGameplayServices services() {
  auto s=f.services();
  s.maps=[&](auto id){auto m=regional_test::map(id);for(auto &c:m.geometry.cells)c.rawAttributes|=allowRest?0x10:0x50;
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
   for(unsigned n=0;!t.f.flow->encounter()->combat() && n<20;++n){check(t.calls==before,"actor settlement precedes the deferred pass");d.tick();}
   check(t.f.flow->encounter()->combat() && t.calls==before,"engaging input defers its Event until combat retirement");
   for(unsigned n=0;!t.f.flow->canSave();++n) {
    check(n<1500,"deferred combat pass settlement bounded");
    auto *combat=t.f.flow->encounter()->combat();
    if(combat && combat->phase()==XeenCombatPhase::PlayerReady)d.send(run?PlayerAction{RunAction{}}:PlayerAction{AttackAction{}});
    else if(!combat){const auto context=d.handler.inputContext(d.shown.presentation());
     if(context.readyForAction && context.dialog && context.dialog->anyKey)d.send(AcknowledgeAction{});}
    d.tick();
   }
   check(t.calls==before+(run?0:1),"win executes the deferred pass once; Run clears it; treasure acknowledgments add none");
  });
 }
}

void newGamePublication() {
 JourneyCase t;auto s=t.services();s.originalStart=XeenDifficulty::Adventurer;
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
 services.resources.loadEvents=[](auto id){auto e=regional_test::events(id);e.records={record(9,11,0,0x12)};return e;};
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
int main() {
 try {outerInputs();whoWillPerPass();manualAndFlags();emptyCalls();tailMarkers();journeyPasses();actionTable();currentKeyAndCapture();panelSavesAndContinuation();f9OutcomesAndRetries();restAndCombat();newGamePublication();std::cout<<"M84 Event dispatch and tail OK\n";return 0;}
 catch(const std::exception &e){std::cerr<<e.what()<<'\n';return 1;}
}


