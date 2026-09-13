#include "XeenJourneyGameplayTestSupport.h"
#include "XeenRestoreReplayProbe.h"
#define SDL_MAIN_HANDLED
#include <SDL.h>
#include <iostream>
using namespace journey_gameplay_test;
namespace fs=std::filesystem;
namespace {
void sdlBoundaries() {
 for(bool lost:{false,true}) {
  Harness h;auto s=h.services();unsigned stage=0,stable=0,loops=0,noEvents=0;bool done=false;
  s.show=[&](const auto &first,const auto &handler,const auto &escape,const auto &idle,const auto &status) {
   const auto key=[](SDL_Keycode code,Uint32 type=SDL_KEYDOWN,Uint8 repeat=0){SDL_Event e{};e.type=type;e.key.keysym.sym=code;e.key.repeat=repeat;
    check(SDL_PushEvent(&e)==1,"Journey SDL queued key");};
   h.flow->reportManual=[&](const auto &){++noEvents;};
   const auto driver=[&]()->std::optional<IndexedFrame>{
    check(++loops<150,"bounded Journey SDL input test");
    auto frame=idle();if(frame){stable=0;return frame;}if(++stable<2)return frame;stable=0;
    switch(stage++) {
    case 0:key(SDLK_RIGHT);key(SDLK_RIGHT,SDL_KEYDOWN,1);key(SDLK_F9);key(SDLK_F9,SDL_KEYUP);key(SDLK_SPACE);key(SDLK_SPACE,SDL_KEYUP);key(SDLK_i);key(SDLK_i,SDL_KEYUP);break;
    case 1:check(h.camera->direction==XeenDirection::East&&h.party->encounterContext->ctr24==1&&!h.flow->inventoryOpen()&&h.saves==0&&noEvents==0,"fixed poll batch protects whole Journey");key(SDLK_RIGHT);break;
    case 2:check(h.camera->direction==XeenDirection::East,"held navigation cannot repeat");key(SDLK_RIGHT,SDL_KEYUP);break;
    case 3:key(SDLK_RIGHT);key(SDLK_RIGHT,SDL_KEYUP);break;
    case 4:check(h.camera->direction==XeenDirection::South&&h.party->encounterContext->ctr24==2,"released fresh navigation accepted");key(SDLK_i);break;
    case 5:check(h.flow->inventoryOpen()&&!XeenSaveState::canCapture(*h.party,*h.camera,*h.world),"SDL modal capture lease");key(SDLK_i);break;
    case 6:check(h.flow->inventoryOpen(),"held I cannot cross inventory boundary");key(SDLK_i,SDL_KEYUP);break;
    case 7:key(SDLK_i);key(SDLK_i,SDL_KEYUP);break;
    default:check(!h.flow->inventoryOpen()&&h.flow->canSave(),"matching closed inventory handoff");done=true;
     if(lost){h.flow->refresh(true);return std::nullopt;}
     {SDL_Event e{};e.type=SDL_QUIT;SDL_PushEvent(&e);}break;
    }
    return frame;
   };
   const bool ok=SdlWindow().showInteractive(first,"Journey SDL boundary",handler,escape,driver,status);
   check(done&&ok==!lost&&!h.flow->canSave()&&!XeenSaveState::canCapture(*h.party,*h.camera,*h.world),"SDL lost-upload/shutdown closes Journey");
   return ok;
  };
  check(Application().playGameplay(s,XeenActorApproach::kEntry,{},false,XeenEncounterEntry::Journey,56)==(lost?4:0),"Journey SDL boundary result");
 }
}
void press(Harness &h,const SdlWindow::FrameUpdateHandler &handler,const PlayerAction &a) {
 handler.beginCycle(++h.cycle);
 handler.withDisplayedInput(a,*handler.displayedInput());
 check(handler.frameCurrent(),"current production frame");handler.framePresented();
}
void tick(Harness &h,const SdlWindow::FrameUpdateHandler &handler,const SdlWindow::IdleFrameHandler &idle) {
 h.now+=100;handler.beginCycle(++h.cycle);idle();check(handler.frameCurrent(),"current idle frame");handler.framePresented();
}
void ring(Harness &h,const SdlWindow::FrameUpdateHandler &handler,bool returning) {
 for(const PlayerAction &a:std::vector<PlayerAction>{InspectInventoryAction{},SelectMemberAction{returning?0u:5u},
  NavigationAction::TurnRight,NavigationAction::TurnRight,SelectInventorySlotAction{1},TransferInventoryAction{},
  SelectMemberAction{returning?5u:0u},AcknowledgeAction{},SelectMemberAction{returning?5u:0u},SelectInventorySlotAction{1},
  EquipmentInventoryAction{},InspectInventoryAction{}})press(h,handler,a);
}
void boundaries(const fs::path &dir) {
 for(unsigned mode=0;mode<11;++mode) {
  Harness h;auto s=h.services();JourneyOracle oracle(s);const auto path=dir/("boundary-"+std::to_string(mode)+".mmsave");
  XeenSaveFile::write(path,save_test::sample());const auto old=completed_test::diskBytes(path);
  bool armed=false,injected=false;unsigned attempts=0,samples=0;
  s.sampleJourneySeed=[&]{++samples;return mode==10?0u:56u;};
  if(mode==10)oracle.expected.journey->skeletonSeed=1;
  const auto compose=s.composeEncounter;
  s.composeEncounter=[&](auto &w,const auto &p,const auto &c,auto phase,auto actor) {
   if(armed&&mode==0&&++attempts==1){injected=true;throw std::runtime_error("isolated composition fault");}
   return compose(w,p,c,phase,actor);
  };
  s.show=[&](const auto &,const auto &handler,const auto &,const auto &idle,const auto &) {
   check(samples==1,"default seed sampled once");handler.framePresented();oracle.state(h);armed=true;
   if(mode==0||mode==1) {
    if(mode==1)h.flow->beforeEncounterFrameCopy=[&]{if(!injected){injected=true;throw std::runtime_error("frame copy fault");}};
    const auto generation=*handler.displayedInput();
    handler.withDisplayedInput(InspectInventoryAction{},generation);
    check(injected&&!h.flow->canSave()&&!XeenSaveState::canCapture(*h.party,*h.camera,*h.world),"recovered modal frame capture closed");
    handler.withDisplayedInput(SaveGameAction{},generation);check(h.saves==0,"stale F9 before recovered upload");
    handler.framePresented();check(h.flow->inventoryOpen(),"recovery kept modal state");
    check(!XeenSaveState::canCapture(*h.party,*h.camera,*h.world),"modal world capture lease");
    press(h,handler,InspectInventoryAction{});check(h.flow->canSave(),"matching close handoff opens capture");oracle.state(h);
   } else if(mode==2) {
    unsigned reports=0;h.flow->reportManual=[&](const auto &){++reports;throw std::runtime_error("no-event report fault");};
    press(h,handler,InteractionAction{});check(reports==1&&h.flow->canSave(),"no-event bounded recovery without replay");oracle.state(h);
   } else if(mode==3) {
    const auto original=h.party->roster.at(0).currentHp;
    const_cast<XeenPartyState*>(h.party)->roster.at(0).currentHp++;
    check(!XeenSaveState::canCapture(*h.party,*h.camera,*h.world),"direct capture latches unauthorized mutation");
    const_cast<XeenPartyState*>(h.party)->roster.at(0).currentHp=original;
    check(!XeenSaveState::canCapture(*h.party,*h.camera,*h.world)&&!h.flow->canSave(),"exact restoration cannot revive authority");
    handler.withDisplayedInput(SaveGameAction{},*handler.displayedInput());check(h.saves==0,"integrity F9 before providers");return false;
   } else if(mode==4||mode==5) {
    h.flow->refresh(true);if(mode==4)handler.failed();else handler.closed();
    check(!XeenSaveState::canCapture(*h.party,*h.camera,*h.world),"lost upload / shutdown closure");
    handler.withDisplayedInput(SaveGameAction{},*handler.displayedInput());check(h.saves==0,"closed F9");return false;
   } else if(mode==6) {
    h.flow->reportEquipment=[&](const auto &){injected=true;throw std::runtime_error("published item report fault");};
    for(const PlayerAction &a:std::vector<PlayerAction>{InspectInventoryAction{},SelectMemberAction{5},NavigationAction::TurnRight,
     NavigationAction::TurnRight,SelectInventorySlotAction{1},EquipmentInventoryAction{}})press(h,handler,a);
    check(injected&&!h.flow->inventoryOpen(),"failed reporting unwinds modal");
    oracle.ring(false,true);oracle.state(h);check(h.flow->canSave(),"published item survives recovery");
   } else if(mode==7) {
    press(h,handler,NavigationAction::TurnRight);press(h,handler,NavigationAction::MoveForward);
    const auto pending=h.flow->encounter()->state().pending();const auto t=*h.party->encounterContext;
    press(h,handler,SaveGameAction{});press(h,handler,InspectInventoryAction{});press(h,handler,InteractionAction{});
    check(pending==2&&h.flow->encounter()->state().pending()==2&&*h.party->encounterContext==t&&h.saves==0,"pending actions/save do not drain or dispatch");
    tick(h,handler,idle);tick(h,handler,idle);check(h.flow->canSave(),"actual pulses settle moved anchor");
    check(h.world->sessionState().actors()[5].x==13&&h.world->sessionState().actors()[5].y==1,"moved literal anchor");
   } else if(mode==8) {
    const auto t=*h.party->encounterContext;const auto c=*h.camera;
    press(h,handler,AcknowledgeAction{});press(h,handler,RevisitCompletedAction{});
    press(h,handler,NavigationAction::MoveBackward);
    check(!h.flow->encounter()->combat()&&*h.party->encounterContext==t&&xeen_state::sameCamera(c,*h.camera),"Enter/R/edge cannot publish Journey actions");
   } else if(mode==9) {
    // Save callbacks are observed through the supplied service function below.
    press(h,handler,SaveGameAction{});
    check(injected&&completed_test::diskBytes(path)==old&&h.flow->canSave(),"failed detached save preserves target and live authority");
   }
   return true;
  };
  s.observeSaveStage=[&](auto stage){++h.saves;if(mode==9&&stage==XeenGameplayServices::SaveStage::Preflight){injected=true;throw std::runtime_error("detached preflight failure");}};
  const int expected=mode>=3&&mode<=5?4:0;
  check(Application().playGameplay(s,XeenActorApproach::kEntry,path,false,XeenEncounterEntry::Journey)==expected,"Journey boundary matrix");
 }
}
void readiness(const fs::path &dir) {
 Harness h;auto s=h.services();auto characters=chr();const auto at=6*354+166+2*36+2*4;
 characters[at]=105;characters[at+1]=1;
 s.resources.loadInitialParty=[&]{return XeenPartyLoader().loadFromResources(characters,pty());};
 s.resources.loadInitialCharacters=[&]{return characters;};
 s.show=[&](const auto &,const auto &handler,const auto &,const auto &,const auto &status){
  handler.framePresented();
  const auto equipment=[&]{for(const PlayerAction &a:std::vector<PlayerAction>{InspectInventoryAction{},SelectMemberAction{5},NavigationAction::TurnRight,
   NavigationAction::TurnRight,SelectInventorySlotAction{2},EquipmentInventoryAction{},InspectInventoryAction{}})press(h,handler,a);};
  equipment();check(h.party->roster.at(6).accessories[2].frame==8,"existing UI equips Journey-valid unsupported contribution");
  const auto context=*h.party->encounterContext;
  press(h,handler,WaitAction{});
  check(*h.party->encounterContext==context&&!h.flow->encounter()->combat()&&h.flow->canSave(),"melee refusal before action publication remains mutable");
  check(h.flow->encounter()->journeyRefusal().find("Unsupported")!=std::string::npos,"specific current-state refusal");
  press(h,handler,SaveGameAction{});check(h.saves==3&&status().find("Saved")!=std::string::npos,"melee-unready Journey-valid state saves");
  equipment();check(h.party->roster.at(6).accessories[2].frame==0,"same UI removes unsupported contribution");
  press(h,handler,WaitAction{});check(h.flow->encounter()->combat(),"repaired current state automatically attaches");return true;
 };
 check(Application().playGameplay(s,XeenActorApproach::kEntry,dir/"readiness.mmsave",false,XeenEncounterEntry::Journey,56)==0,"mutable Journey readiness boundary");
}
void run(const fs::path &dir,const std::optional<fs::path> &game,unsigned fault=0) {
 const auto path=dir/"connected.mmsave";
 Harness h(game);auto s=h.services(56);JourneyOracle oracle(s);
 const auto initializations=replay_test::journeyInitializations,constructions=replay_test::journeyConstructions,retirements=replay_test::retirements;
 JourneyOracle published(s);published.ring(true,true);published.victory();
 bool injected=false;const auto compose=s.composeEncounter;
 s.composeEncounter=[&](auto &w,const auto &p,const auto &c,auto phase,auto actor) {
  if(fault&&!injected&&h.world==&w&&w.sessionState().actors().at(5).lifecycle==XeenActorLifecycle::Defeated&&
   (fault==1?h.flow->encounter()->combat()!=nullptr:h.flow->encounter()->combat()==nullptr)) {
   injected=true;published.expected.journey->context->minutes=fault==1?491:492;published.state(h);
   check(!XeenSaveState::canCapture(p,c,w),"lethal / retired unpresented frame cannot capture");
   throw std::runtime_error("fallible lethal/return composition after publication");
  }
  return compose(w,p,c,phase,actor);
 };
 unsigned samples=0;s.sampleJourneySeed=[&]{++samples;return 56;};
 s.show=[&](const auto &,const auto &handler,const auto &,const auto &idle,const auto &status) {
  check(!h.flow->canSave()&&!h.flow->encounter()->combat(),"fresh no combat / first-frame save closure");oracle.state(h);
  const auto first=*handler.displayedInput();handler.withDisplayedInput(SaveGameAction{},first);check(h.saves==0,"F9 before first handoff");
  handler.framePresented();ring(h,handler,false);oracle.ring(true,true);oracle.state(h);
  check(!h.flow->encounter()->combat(),"inventory did not construct combat");
  unsigned noEvents=0;h.flow->reportManual=[&](const auto &r){check(std::holds_alternative<XeenManualEventNoEvent>(r),"real no-event result");++noEvents;};
  press(h,handler,InteractionAction{});check(noEvents==1,"Journey actual interaction");
  const auto stale=*handler.displayedInput();press(h,handler,WaitAction{});
  check(h.flow->encounter()->combat(),"automatic engagement attachment");
  handler.withDisplayedInput(InteractionAction{},stale);handler.withDisplayedInput(SaveGameAction{},stale);check(h.saves==0,"stale engagement F9");
  unsigned commands=0,steps=0;
  while(h.flow->encounter()->combat()&&++steps<200) {
   check(!h.flow->encounter()->terminal(),"unexpected Journey combat terminal");
   if(h.flow->encounter()->combat()->phase()==Phase::PlayerReady)press(h,handler,commands++<6?PlayerAction{BlockAction{}}:PlayerAction{InteractionAction{}});
   else tick(h,handler,idle);
  }
  check(commands==8&&!h.flow->encounter()->combat(),"seed56 eight commands and successful-End retirement");
  oracle.victory();oracle.state(h);check(h.flow->canSave(),"mutable returned frame");
  handler.withDisplayedInput(SaveGameAction{},stale);check(h.saves==0,"stale returned F9 before providers");
  for(const PlayerAction &a:std::vector<PlayerAction>{NavigationAction::TurnRight,NavigationAction::MoveForward,NavigationAction::TurnLeft,NavigationAction::MoveForward})press(h,handler,a);
  press(h,handler,SaveGameAction{});check(h.saves==0,"pending approach F9 never drains work");
  press(h,handler,InspectInventoryAction{});check(!h.flow->inventoryOpen(),"pending inventory refused");
  tick(h,handler,idle);tick(h,handler,idle);
  oracle.expected.camera={20,14,2,XeenDirection::North};oracle.expected.journey->context->minutes=512;oracle.expected.journey->context->ctr24=5;oracle.state(h);
  ring(h,handler,true);oracle.ring(false,true);oracle.state(h);
  press(h,handler,SaveGameAction{});check(h.saves==3&&status().find("Saved")!=std::string::npos,"real F9 saves v4");return true;
 };
 check(Application().playGameplay(s,XeenActorApproach::kEntry,path,false,XeenEncounterEntry::Journey,56)==0,"connected producer");
 check(samples==0,"explicit seed bypasses sampling");
 check(replay_test::journeyInitializations==initializations+1&&replay_test::journeyConstructions==constructions+1&&
  replay_test::retirements==retirements+1,"one initialization/attachment/retirement; replay probes have positive controls");
 check(!fault||injected,"actual lethal/return failure seam reached");
 for(unsigned pass=0;pass<2;++pass) {
  Harness next(game);auto load=next.services();
  load.sampleJourneySeed=[]()->std::uint32_t{throw std::runtime_error("load sampled seed");};
  load.resources.loadInitialParty=[]()->XeenPartyState{throw std::runtime_error("load initialized party");};
  load.resources.loadInitialCharacters=[]()->Bytes{throw std::runtime_error("load initialized supplements");};
  load.resources.loadInitialContext=[]()->XeenGameplayContext{throw std::runtime_error("load initialized context");};
  const auto observe=load.observeGameplay;
  load.observeGameplay=[&](auto &w,auto &e,const auto &p,auto &c,const auto &f){observe(w,e,p,c,f);oracle.state(next);check(!next.flow->inventoryOpen()&&!next.flow->encounter()->combat(),"restored closed mutable startup");};
  load.show=[&](const auto &,const auto &handler,const auto &,const auto &idle,const auto &status){
   check(replay_test::unexpected==0,"restored startup no gameplay replay");
   --replay_test::depth;
   struct ResumeProbe { ~ResumeProbe(){++replay_test::depth;} } resumeProbe;
   handler.framePresented();oracle.state(next);
   if(pass==0) {
    for(const PlayerAction &a:std::vector<PlayerAction>{InspectInventoryAction{},SelectMemberAction{5},NavigationAction::TurnRight,NavigationAction::TurnRight,
     SelectInventorySlotAction{1},EquipmentInventoryAction{},InspectInventoryAction{},NavigationAction::TurnLeft,NavigationAction::MoveForward})press(next,handler,a);
    tick(next,handler,idle);tick(next,handler,idle);
    oracle.ring(false,false);oracle.expected.camera={20,13,2,XeenDirection::West};oracle.expected.journey->context->minutes=522;oracle.expected.journey->context->ctr24=7;
   }
   oracle.state(next);press(next,handler,SaveGameAction{});check(status().find("Saved")!=std::string::npos,"restart F9");return true;
  };
  replay_test::Scope scope;
  check(Application().playGameplay(load,{},path,true)==0,"connected consumer");
 }
}
}
int main(int argc,char **argv) { try {
 if(argc==2&&std::string(argv[1])=="sdl"){sdlBoundaries();std::cout<<"Journey SDL generations, held keys and missing upload passed\n";return 0;}
 const auto dir=fs::current_path()/"journey-gameplay-tests";fs::create_directories(dir);
 if(argc==1){boundaries(dir);readiness(dir);}
 run(dir,argc==2?std::optional<fs::path>{argv[1]}:std::nullopt);
 if(argc==1){run(dir,{},1);run(dir,{},2);}
 std::cout<<"Connected Journey production controls and literal seed56 oracle passed\n";return 0;
}catch(const std::exception &e){std::cerr<<e.what()<<'\n';return 1;} }
