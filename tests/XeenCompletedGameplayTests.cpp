#include "XeenCompletedGameplayOracle.h"
#define SDL_MAIN_HANDLED
#include <SDL.h>
#include <iostream>
#include <fstream>
using namespace completed_test;
namespace fs=std::filesystem;
namespace {
void complete(Harness &h,const SdlWindow::FrameUpdateHandler &handler,const SdlWindow::IdleFrameHandler &idle,unsigned seed=1){
 handler.framePresented();
 if(seed==56){
  for(const PlayerAction &a:std::vector<PlayerAction>{InspectInventoryAction{},SelectMemberAction{5},NavigationAction::TurnRight,
   NavigationAction::TurnRight,SelectInventorySlotAction{1},TransferInventoryAction{},SelectMemberAction{0},AcknowledgeAction{},
   SelectMemberAction{0},SelectInventorySlotAction{1},EquipmentInventoryAction{},InspectInventoryAction{}})h.press(handler,a);
 }
 h.press(handler,AcknowledgeAction{});h.press(handler,WaitAction{});
 unsigned commands=0,steps=0;
 while(!h.flow->completed()&&++steps<200){
  check(!h.flow->encounter()->terminal(),"unexpected producer terminal");
  if(h.phase()==Phase::PlayerReady){h.press(handler,commands++<6?PlayerAction{BlockAction{}}:PlayerAction{InteractionAction{}});}
  else h.tick(handler,idle);
 }
 check(h.flow->completed()&&!h.flow->encounter()->combat(),"production retired completed adapter");
 check(commands==(seed==1?15u:8u),"literal producer displayed command count");
}
void directLifecycle(const fs::path &dir){
 for(unsigned seed:{1u,56u}){
  const auto path=dir/(std::to_string(seed)+".mmsave");
  Harness h;auto s=h.services(seed);Oracle oracle(h,s,seed);
  s.show=[&](const auto &,const auto &handler,const auto &,const auto &idle,const auto &status){
   complete(h,handler,idle,seed);oracle.live(h);check(h.flow->canSave(),"completed idle save eligibility");
   h.press(handler,SaveGameAction{});check(status().find("Saved")!=std::string::npos,"production completed F9");oracle.disk(path);
   return true;
  };
  check(Application().playGameplay(s,XeenActorApproach::kEntry,path,false,XeenEncounterEntry::Diagnostic27)==0,"completed producer");
  for(unsigned restart=0;restart<2;++restart){
   Harness next;auto load=next.services(seed);unsigned preparations=0;
   load.prepareCombat=[&](auto &,auto &,auto &,auto &)->std::unique_ptr<XeenCombat>{++preparations;throw std::runtime_error("replayed combat preparation");};
   const auto observe=load.observeGameplay;
   load.observeGameplay=[&](auto &w,auto &e,const auto &p,auto &c,const auto &f){observe(w,e,p,c,f);oracle.live(w,p,c,f);check(next.flow->completed(),"effective completed mode before first frame");};
   load.show=[&](const auto &,const auto &handler,const auto &,const auto &,const auto &){
    handler.framePresented();const auto bytes=diskBytes(path);const auto input=*handler.displayedInput();
    next.press(handler,InspectInventoryAction{});check(next.flow->inventoryOpen()&&!next.flow->inventoryConfirmation(),"read-only inspection open");
    for(const PlayerAction &a:std::vector<PlayerAction>{SelectMemberAction{4},NavigationAction::TurnRight,SelectInventorySlotAction{1},TransferInventoryAction{},EquipmentInventoryAction{},AcknowledgeAction{},RevisitCompletedAction{}})next.press(handler,a);
    const auto calls=next.compositions;next.press(handler,SaveGameAction{});
    check(next.compositions==calls&&next.saves==0&&diskBytes(path)==bytes,"inspection F9 performs zero providers/stages/I/O");
    oracle.live(next);check(!next.flow->inventoryConfirmation(),"inspection never arms certificate");next.press(handler,InspectInventoryAction{});
    for(unsigned entry=1;entry<=2;++entry){
     const auto maps=next.mapCalls,mobs=next.mobCalls;
     next.press(handler,RevisitCompletedAction{});
     check(next.world->completedEntryGeneration()==entry&&next.mapCalls>maps&&next.mobCalls>mobs,"true warm-cache resource entry");
     oracle.live(next);check(diskBytes(path)==bytes,"inspection/revisit changed disk without F9");
     handler.withDisplayedInput(RevisitCompletedAction{},input);check(next.world->completedEntryGeneration()==entry,"stale displayed revisit refused");
    }
    next.world->discardMapCache();next.flow->refresh(true);handler.framePresented();oracle.live(next);
    next.press(handler,SaveGameAction{});oracle.disk(path);return true;
   };
   check(Application().playGameplay(load,{},path,true)==0&&preparations==0,"fresh owner completed restart without preparation");
  }
 }
}
void saveCallbacks(const fs::path &dir){
 for(unsigned mode=0;mode<12;++mode){
  Harness h;auto s=h.services();Oracle oracle(h,s,1);const auto path=dir/("save-fault-"+std::to_string(mode)+".mmsave");
  XeenSaveFile::write(path,save_test::sample());const auto previous=diskBytes(path);
  bool armed=false,injected=false;SdlWindow::FrameUpdateHandler input;
  const auto original=s.composeEncounter;
  const auto mutate=[&]{injected=true;if(mode<3)const_cast<XeenPartyState*>(h.party)->roster.at(0).currentHp++;
   else if(mode==3)const_cast<XeenGameFlags*>(h.flags)->set(7);
   else if(mode==4)h.camera->x=14;
   else if(mode==5)h.flow->invalidateInventory();
   else if(mode==6){input(SaveGameAction{});h.flow->handle(RevisitCompletedAction{},h.flow->displayedInput());}
   else throw std::runtime_error("isolated save preflight fault");};
  s.observeSaveStage=[&](auto stage){++h.saves;if(!armed||injected)return;
   if(mode<3&&unsigned(stage)==mode)mutate();};
  s.composeEncounter=[&](auto &w,const auto &p,const auto &c,auto phase,auto actor){
   if(armed&&!injected&&mode>=3&&mode<=7)mutate();return original(w,p,c,phase,actor);};
  if(mode>=8){
   const auto initial=s.resources.loadInitialParty;
   if(mode==8)s.resources.loadInitialParty=[&,initial]{if(armed&&!injected)mutate();return initial();};
   const auto chr=s.resources.loadInitialCharacters;
   if(mode==9)s.resources.loadInitialCharacters=[&,chr]{if(armed&&!injected)mutate();return chr();};
   const auto mon=s.resources.loadMonsterStatistics;
   if(mode==10)s.resources.loadMonsterStatistics=[&,mon]{if(armed&&!injected)mutate();return mon();};
   const auto events=s.resources.loadEvents;
   if(mode==11)s.resources.loadEvents=[&,events](auto id){if(armed&&!injected)mutate();return events(id);};
  }
  bool observed=false;
  s.show=[&](const auto &,const auto &handler,const auto &,const auto &idle,const auto &){
   complete(h,handler,idle);input=handler;armed=true;
   bool threw=false;try{h.press(handler,SaveGameAction{});}catch(const std::exception &){threw=true;}
   armed=false;check(injected,"actual production save callback reached");
   if(mode==6){check(!threw&&h.saves==3,"nested save/revisit refused; outer operation remains current");oracle.disk(path);}
   else {check(diskBytes(path)==previous,"failed/stale save replaced prior file");
    if(mode>=7){check(!threw&&h.flow->canSave(),"isolated save failure preserves live authority");h.press(handler,SaveGameAction{});oracle.disk(path);}
    else check(threw&&!h.flow->canSave(),"source mutation rejects and closes stale continuation");}
   observed=true;return mode>=6;
  };
  const auto result=Application().playGameplay(s,XeenActorApproach::kEntry,path,false,XeenEncounterEntry::Diagnostic27);
  check(observed&&result==(mode>=6?0:4),"save callback lifecycle result");
 }
}
void presentationFailures(const fs::path &dir){
 for(unsigned mode=0;mode<6;++mode){
  Harness h;auto s=h.services();Oracle oracle(h,s,1);const auto path=dir/("presentation-"+std::to_string(mode)+".mmsave");
  bool injected=false,recovered=false;unsigned attempts=0;
  const auto compose=s.composeEncounter;
  s.composeEncounter=[&](auto &w,const auto &p,const auto &c,auto phase,auto actor){
   if(w.sessionState().completion()==XeenEncounterCompletion::VictoryQuiescent&&(mode==0||mode>=3)){
    ++attempts;
    if(!injected){injected=true;if(mode==4)const_cast<XeenPartyState&>(p).roster.at(0).currentHp++;
     if(mode==5)const_cast<XeenGameFlags*>(h.flags)->set(8);
     throw std::runtime_error("completed composition failure");}
    if(mode==3)throw std::runtime_error("completed recovery failure");
   }
   return compose(w,p,c,phase,actor);
  };
  const auto configure=s.configureFlow;
  s.configureFlow=[&](auto &flow,const auto &camera){configure(flow,camera);
   flow.beforeEncounterFrameCopy=[&]{if(mode==1&&flow.completed()&&!injected){injected=true;throw std::runtime_error("completed return copy");}};
   flow.reportText=[&](const auto &){if(mode==2&&flow.completed()&&!injected){injected=true;throw std::runtime_error("completed report");}};
  };
  bool observed=false;
  s.show=[&](const auto &,const auto &handler,const auto &,const auto &idle,const auto &){
   handler.framePresented();h.press(handler,AcknowledgeAction{});h.press(handler,WaitAction{});unsigned commands=0;
   try{for(unsigned step=0;!h.flow->completed()&&step<200;++step){
    if(h.phase()==Phase::PlayerReady)h.press(handler,commands++<6?PlayerAction{BlockAction{}}:PlayerAction{InteractionAction{}});
    else {h.now+=100;handler.beginCycle(++h.cycle);idle();if(!h.flow->completed())handler.framePresented();}
   }}catch(const std::exception &){check(mode>=3,"unexpected recovery failure");}
   check(injected&&h.world->sessionState().completion()==XeenEncounterCompletion::VictoryQuiescent,"successful End survives presentation failure");
   check(!XeenSaveState::canCapture(*h.party,*h.camera,*h.world),"lease/fatal failure blocks direct capture before handoff");
   if(mode<3){
    oracle.live(h);check(handler.frameCurrent(),"authorized reconstructed frame exists");
    handler(SaveGameAction{});check(h.saves==0&&!fs::exists(path),"unhanded recovery F9 refuses without providers");
    handler.framePresented();check(h.flow->canSave(),"actual handoff releases recovery lease");
    h.press(handler,SaveGameAction{});oracle.disk(path);recovered=true;
   }else check(!h.flow->canSave()&&!fs::exists(path),"fatal/integrity recovery never permits save");
   observed=true;return mode<3;
  };
  check(Application().playGameplay(s,XeenActorApproach::kEntry,path,false,XeenEncounterEntry::Diagnostic27)==(mode<3?0:4)&&observed&&recovered==(mode<3),"completed presentation lifecycle");
 }
}
void reentryFailures(const fs::path &dir){
 for(unsigned mode=0;mode<6;++mode){
  Harness h;auto s=h.services();Oracle oracle(h,s,1);const auto path=dir/("reentry-"+std::to_string(mode)+".mmsave");
  bool armed=false,injected=false;const auto mon=s.resources.loadMonsterStatistics;
  s.resources.loadMonsterStatistics=[&]{
   if(armed&&!injected&&mode<3){injected=true;
    if(mode==0)throw std::runtime_error("re-entry MON failure");
    if(mode==1)const_cast<XeenPartyState*>(h.party)->roster.at(0).currentHp++;
    if(mode==2)const_cast<XeenGameFlags*>(h.flags)->set(19);
   }return mon();};
  const auto compose=s.composeEncounter;
  s.composeEncounter=[&](auto &w,const auto &p,const auto &c,auto phase,auto actor){
   if(armed&&mode>=3&&!injected){injected=true;
    if(mode==3)throw std::runtime_error("detached re-entry preflight failure");
    if(mode==4)const_cast<XeenActor&>(h.world->sessionState().actors()[0]).hp++;
    if(mode==5)h.flow->closeGameplay();
   }return compose(w,p,c,phase,actor);};
  s.show=[&](const auto &,const auto &handler,const auto &,const auto &idle,const auto &){
   complete(h,handler,idle);h.press(handler,SaveGameAction{});const auto bytes=diskBytes(path);armed=true;bool threw=false;
   try{h.press(handler,RevisitCompletedAction{});}catch(const std::exception &){threw=true;}armed=false;
   check(injected&&h.world->completedEntryGeneration()==0&&diskBytes(path)==bytes,"failed entry preserved old incarnation and save");
   if(mode==0||mode==3){check(!threw&&h.flow->canSave(),"checked failed-operation release renewed Flow");oracle.live(h);
    h.press(handler,RevisitCompletedAction{});oracle.live(h);check(h.world->completedEntryGeneration()==1,"fresh R after failed candidate");}
   else check(threw&&!h.flow->canSave(),"mutated/fatal re-entry cannot renew source authority");
   return mode==0||mode==3;
  };
  check(Application().playGameplay(s,XeenActorApproach::kEntry,path,false,XeenEncounterEntry::Diagnostic27)==(mode==0||mode==3?0:4),"re-entry failure lifecycle");
 }
}
void ordinarySourceGuards(const fs::path &dir){
 for(unsigned mode=0;mode<3;++mode){
  gameplay_test::Fixture fixture;auto s=fixture.services();const auto path=dir/("ordinary-source-"+std::to_string(mode)+".mmsave");
  XeenSaveFile::write(path,save_test::sample());const auto bytes=diskBytes(path);bool injected=false;
  const XeenPartyState *party=nullptr;const auto observe=s.observeGameplay;
  s.observeGameplay=[&](auto &w,auto &e,const auto &p,auto &c,const auto &f){observe(w,e,p,c,f);party=&p;};
  s.observeSaveStage=[&](auto stage){if(stage==XeenGameplayServices::SaveStage::Write){injected=true;
   if(mode==0)const_cast<XeenPartyState*>(party)->roster.at(0).currentHp++;
   if(mode==1)fixture.flow->invalidateInventory();
   if(mode==2)fixture.flow->closeGameplay();}};
  s.show=[&](const auto &,const auto &handler,const auto &,const auto &,const auto &status){
   handler(SaveGameAction{});check(injected&&diskBytes(path)==bytes&&status().find("Save failed")!=std::string::npos,"ordinary source/UI callback must prevent replacement");return true;};
  check(Application().playGameplay(s,{1,1,1,XeenDirection::North},path,false)==0,"ordinary source guard callback route");
 }
}
void disposedRecovery(){
 CombatFixture f;f.enter();f.blockRound();
 for(unsigned n=0;f.combat->phase()!=Phase::Victory&&n<200;++n){if(f.combat->phase()==Phase::PlayerReady)f.action(Command::Attack);else f.service();}
 f.combat->retireCompletedVictory(f.combat->ticket());f.combat.reset();
 XeenGameFlags flags;const auto before=f.p.roster.characters();const auto actors=f.w.sessionState().actors();
 XeenEventSystem events([](auto){return XeenEventScript(combat_test::events());},[](auto){return XeenEventTextFile{};});
 XeenFontFormat font(gameplay_test::fontBytes());auto evt=combat_test::events();XeenEncounterSetup setup{evt,{},{}};
 bool fail=false;unsigned calls=0;
 auto flow=std::make_unique<XeenEventFlow>(f.w,events,f.p,f.camera,flags,font,
  [](auto){return XeenEventFlow::Composition{};},XeenEventPresenter::NpcDraw{},XeenEventPresenter::Clock{[]{return 0;}},
  XeenEventPresenter::RandomFrame{},nullptr,&setup,[&](auto,auto){if(fail&&++calls==1)throw std::runtime_error("dispose pending recovery");
   return XeenEventFlow::Composition{IndexedFrame{320,200,Bytes(64000)},false};});
 flow->framePresented();fail=true;flow->refresh(true);
 check(!XeenSaveState::canCapture(f.p,f.camera,f.w),"pending reconstructed frame holds presentation lease");
 flow.reset();check(!XeenSaveState::canCapture(f.p,f.camera,f.w),"Flow disposal cannot clear world presentation lease");
 for(unsigned i=0;i<30;++i)remove_test::checkSameCharacter(before[i],f.p.roster.at(i));sameActors(actors,f.w.sessionState().actors());
}
void nestedSourceAndLifetime(const fs::path &dir){
 for(unsigned mode=0;mode<5;++mode){
  Harness h;auto s=h.services();const auto path=dir/("nested-source-"+std::to_string(mode)+".mmsave");
  XeenSaveFile::write(path,save_test::sample());const auto previous=diskBytes(path);
  const auto maps=s.maps;s.maps=[&,maps](auto id){auto value=maps(id);value.geometry.id=id.number;return value;};
  bool armed=false,injected=false;const auto compose=s.composeEncounter;
  s.composeEncounter=[&](auto &w,const auto &p,const auto &c,auto phase,auto actor){
   if(armed&&!injected){injected=true;
    if(mode<2){const auto &m=h.world->map(21);if(mode==1)const_cast<XeenMap&>(m).geometry.cells[0].rawWord^=1;}
    if(mode==2){auto *owner=const_cast<XeenPartyState*>(h.party);owner->~XeenPartyState();new(owner) XeenPartyState;}
    if(mode==3){h.world->~XeenWorld();new(h.world) XeenWorld([](auto id){auto m=combat_test::map();m.geometry.id=id.number;return m;});}
    if(mode==4){auto *owner=const_cast<XeenGameFlags*>(h.flags);owner->~XeenGameFlags();new(owner) XeenGameFlags;}
   }
   return compose(w,p,c,phase,actor);
  };
  bool observed=false;
  s.show=[&](const auto &,const auto &handler,const auto &,const auto &idle,const auto &){
   complete(h,handler,idle);armed=true;bool threw=false;
   try{h.press(handler,SaveGameAction{});}catch(const std::exception &){threw=true;}armed=false;
   check(injected,"nested retained source/lifetime seam");
   if(mode==0){check(!threw&&h.flow->canSave(),"trusted newly populated live source cache admitted");check(diskBytes(path)!=previous,"valid source cache save completed");}
   else check(threw&&diskBytes(path)==previous,"nested source corruption/replacement prevents write");
   observed=true;return mode==0;
  };
  const auto result=Application().playGameplay(s,XeenActorApproach::kEntry,path,false,XeenEncounterEntry::Diagnostic27);
  check(observed&&result==(mode==0?0:mode==1?4:3),"nested source/lifetime exit boundary");
 }
}
void inspectionAndHandoff(const fs::path &dir){
 for(unsigned mode=0;mode<4;++mode){
  Harness h;auto s=h.services();Oracle oracle(h,s,1);bool armed=false,injected=false;
  const auto compose=s.composeEncounter;
  s.composeEncounter=[&](auto &w,const auto &p,const auto &c,auto phase,auto actor){
   if(armed&&mode==0&&!injected){injected=true;throw std::runtime_error("inspection composition failure");}
   return compose(w,p,c,phase,actor);};
  const auto configure=s.configureFlow;s.configureFlow=[&](auto &flow,const auto &c){configure(flow,c);
   flow.beforeEncounterFrameCopy=[&]{if(armed&&mode==1&&!injected){injected=true;throw std::runtime_error("inspection copy failure");}};};
  const auto path=dir/("inspection-"+std::to_string(mode)+".mmsave");
  s.show=[&](const auto &,const auto &handler,const auto &,const auto &idle,const auto &){
   complete(h,handler,idle);armed=true;
   if(mode<2){
    handler.withDisplayedInput(InspectInventoryAction{},*handler.displayedInput());
    check(injected&&handler.frameCurrent()&&h.flow->inventoryOpen()&&!XeenSaveState::canCapture(*h.party,*h.camera,*h.world),"inspection recovery retained until handoff");
    handler.framePresented();oracle.live(h);h.press(handler,InspectInventoryAction{});
    check(h.flow->canSave(),"closing recovered inspection releases only matching lease");h.press(handler,SaveGameAction{});oracle.disk(path);
   }else {
    if(mode==2){h.flow->refresh(true);handler.failed();}else handler.closed();
    check(!XeenSaveState::canCapture(*h.party,*h.camera,*h.world),"upload failure or shutdown permanently unavailable");
    handler(SaveGameAction{});check(h.saves==0&&!fs::exists(path),"closed application cannot save");oracle.live(h);
   }
   return mode<2;
  };
  check(Application().playGameplay(s,XeenActorApproach::kEntry,path,false,XeenEncounterEntry::Diagnostic27)==(mode<2?0:4),"inspection/handoff lifecycle");
 }
}
void image(const fs::path &path,const IndexedFrame &frame){
 std::ofstream out(path,std::ios::binary);out<<"P6\n"<<frame.width<<' '<<frame.height<<"\n255\n";
 for(auto pixel:frame.pixels)out.write(reinterpret_cast<const char*>(frame.palette.data()+3*pixel),3);
 check(bool(out),"frame evidence write");
}
void key(SDL_Keycode code,Uint32 type=SDL_KEYDOWN,Uint8 repeat=0){SDL_Event e{};e.type=type;e.key.keysym.sym=code;e.key.repeat=repeat;check(SDL_PushEvent(&e)==1,"continuous SDL input");}
void pressKey(SDL_Keycode code){key(code);key(code,SDL_KEYUP);}
int child(const std::optional<fs::path> &game,const fs::path &dir,unsigned seed,const std::string &role){
 Harness h(game);auto s=h.services(seed);Oracle oracle(h,s,seed);const auto path=dir/(std::to_string(seed)+".mmsave");
 const bool producer=role=="producer",fresh=role=="fresh";const bool resume=!producer&&!fresh;
 unsigned preparations=0;const auto prepare=s.prepareCombat;s.prepareCombat=[&](auto &w,auto &p,auto &c,auto &b){++preparations;return prepare(w,p,c,b);};
 bool observed=false;const auto observe=s.observeGameplay;
 s.observeGameplay=[&](auto &w,auto &e,const auto &p,auto &c,const auto &f){observe(w,e,p,c,f);
  if(resume){oracle.live(w,p,c,f);check(h.flow->completed()&&preparations==0,"restored authority before first visible frame");}
  else {sameParty(oracle.initial,p);check(w.sessionState().actors().empty()&&!w.sessionState().combatAccounted(),"independent fresh preparation");}
  observed=true;
 };
 unsigned stage=0,commands=0,idles=0,stable=0;std::optional<std::uint64_t> lastInput;
 std::vector<SDL_Keycode> preparation;
 if(seed==56)preparation={SDLK_i,SDLK_F6,SDLK_RIGHT,SDLK_RIGHT,SDLK_2,SDLK_t,SDLK_F1,SDLK_RETURN,SDLK_F1,SDLK_2,SDLK_e,SDLK_i};
 preparation.insert(preparation.begin(),SDLK_F9);preparation.push_back(SDLK_RETURN);preparation.push_back(SDLK_PERIOD);
 unsigned prep=0;Bytes original=resume?diskBytes(path):Bytes{};unsigned mapBefore=0,mobBefore=0;std::uint64_t oldEntry=0;
 s.show=[&](const auto &first,const auto &handler,const auto &escape,const auto &idle,const auto &status){
  check(observed,"startup owners observed before SDL");
  image(dir/(std::to_string(seed)+"-"+role+"-first.ppm"),first);
  const auto drivenIdle=[&]()->std::optional<IndexedFrame>{
   h.now+=100;auto frame=idle();if(++idles>900)throw std::runtime_error("bounded completed SDL driver timed out");
   const auto ticket=handler.displayedInput();if(ticket!=lastInput){lastInput=ticket;stable=0;return frame;}
   if(++stable<2)return frame;stable=0;
   if(fresh){
    if(!stage++){pressKey(SDLK_RETURN);return frame;}
    check(h.phase()==Phase::Approach&&h.world->sessionState().actors().size()==27,"fresh Begin creates original actors");
    auto expected=oracle.actors;auto &a=expected[5];a.x=13;a.y=2;a.hp=20;a.lifecycle=XeenActorLifecycle::Present;a.activated=true;
    sameActors(expected,h.world->sessionState().actors());check(!h.world->sessionState().combatAccounted(),"fresh no completion/accounting");
    SDL_Event quit{};quit.type=SDL_QUIT;SDL_PushEvent(&quit);return frame;
   }
   if(producer&&!h.flow->completed()){
    if(prep<preparation.size())pressKey(preparation[prep++]);
    else if(h.phase()==Phase::PlayerReady){pressKey(commands++<6?SDLK_b:SDLK_SPACE);}
    else {check(!h.flow->encounter()->terminal(),"continuous producer unexpected failure");pressKey(SDLK_F9);}
    return frame;
   }
   if(stage==0){oracle.live(h);image(dir/(std::to_string(seed)+"-"+role+"-completed.ppm"),h.flow->frame());check(producer?commands==(seed==1?15u:8u):preparations==0,"production control command/restore count");
    h.world->discardMapCache();if(h.assets)h.assets->discardSpriteCache();frame=h.flow->refresh(true);oracle.live(h);++stage;return frame;}
   if(producer){
    if(stage++==1){check(h.saves==0,"unsafe F9 performed no stage work");pressKey(SDLK_F9);return frame;}
    oracle.disk(path);check(status().find("Saved")!=std::string::npos,"continuous production F9 status");
    SDL_Event quit{};quit.type=SDL_QUIT;SDL_PushEvent(&quit);return frame;
   }
   switch(stage++){
    case 1:pressKey(SDLK_i);break;
    case 2:check(h.flow->inventoryOpen(),"SDL completed I inspection");pressKey(SDLK_F5);break;
    case 3:pressKey(SDLK_RIGHT);break;
    case 4:pressKey(SDLK_2);break;
    case 5:image(dir/(std::to_string(seed)+"-"+role+"-inspection.ppm"),h.flow->frame());pressKey(SDLK_e);pressKey(SDLK_t);pressKey(SDLK_RETURN);pressKey(SDLK_F9);pressKey(SDLK_r);break;
    case 6:oracle.live(h);check(h.saves==0&&h.world->completedEntryGeneration()==0&&diskBytes(path)==original,"read-only inspection/refusal boundary");pressKey(SDLK_i);break;
    case 7:mapBefore=h.mapCalls;mobBefore=h.mobCalls;oldEntry=h.world->completedEntryGeneration();
     key(SDLK_r);key(SDLK_r,SDL_KEYDOWN,1);key(SDLK_r,SDL_KEYUP);key(SDLK_r);break;
    case 8:check(h.world->completedEntryGeneration()==oldEntry+1&&h.mapCalls>mapBefore&&h.mobCalls>mobBefore,"one true re-entry per poll batch");
     oracle.live(h);key(SDLK_r);break;
    case 9:check(h.world->completedEntryGeneration()==1,"held R cannot repeat");key(SDLK_r,SDL_KEYUP);break;
    case 10:pressKey(SDLK_r);break;
    case 11:image(dir/(std::to_string(seed)+"-"+role+"-revisited.ppm"),h.flow->frame());check(h.world->completedEntryGeneration()==2,"fresh R repeats true entry");oracle.live(h);
     check(diskBytes(path)==original,"inspection and true entry leave disk unchanged");
     h.world->discardMapCache();if(h.assets)h.assets->discardSpriteCache();frame=h.flow->refresh(true);break;
    case 12:oracle.live(h);pressKey(SDLK_F9);break;
    default:oracle.disk(path);{SDL_Event quit{};quit.type=SDL_QUIT;SDL_PushEvent(&quit);}break;
   }
   return frame;
  };
  return SdlWindow().showInteractive(first,"MMModern - Map completed acceptance",handler,escape,drivenIdle,status);
 };
 const auto result=Application().playGameplay(s,XeenActorApproach::kEntry,fresh?std::optional<fs::path>{}:path,resume,
  resume?XeenEncounterEntry::Ordinary:XeenEncounterEntry::Diagnostic27);
 check(result==0,"continuous Application/Flow/SDL child");
 std::cout<<"ACCEPT "<<seed<<' '<<role<<" PID "<<GetCurrentProcessId()<<" all owners/items/conditions/supplements/context/flags/27 actors; save stages="<<h.saves<<'\n';return 0;
}
void lostUpload(){
 Harness h;auto s=h.services();bool armed=false,injected=false,observed=false;const auto compose=s.composeEncounter;
 s.composeEncounter=[&](auto &w,const auto &p,const auto &c,auto phase,auto actor){
  if(armed&&!injected){injected=true;throw std::runtime_error("recover then lose returned frame");}
  return compose(w,p,c,phase,actor);};
 s.show=[&](const auto &,const auto &handler,const auto &escape,const auto &idle,const auto &status){
  complete(h,handler,idle);unsigned calls=0;
  const auto missing=[&]()->std::optional<IndexedFrame>{
   if(++calls>3)throw std::runtime_error("missing upload was not rejected");
   armed=true;h.flow->refresh(true);return std::nullopt; // Intentionally lose the new texture upload.
  };
  auto continued = handler;
  const auto previousCycle = h.cycle;
  continued.beginCycle = [&](std::uint64_t cycle) { handler.beginCycle(previousCycle + cycle); };
  const bool ok=SdlWindow().showInteractive(h.flow->frame(),"MMModern - Map missing handoff",continued,escape,missing,status);
  check(!ok&&injected&&!XeenSaveState::canCapture(*h.party,*h.camera,*h.world),"actual SDL must not release a composed but unuploaded recovery");
  observed=true;return false;
 };
 check(Application().playGameplay(s,XeenActorApproach::kEntry,{},false,XeenEncounterEntry::Diagnostic27)==4&&observed,"lost SDL upload lifecycle");
}
void processes(const fs::path &exe,const std::optional<fs::path> &game,const fs::path &dir){
 std::ofstream evidence(dir/"processes.log");
 for(unsigned seed:{1u,56u}){
  DWORD previous=0;
  for(const std::string role:{"producer","consumer","repeat","fresh"}){
   const auto name=std::to_string(seed)+"-"+role;
   const std::vector<std::wstring> args{L"--child",game?game->wstring():L"-",dir.wstring(),std::to_wstring(seed),fs::path(role).wstring()};
   const auto r=child_test::launch(exe,args,dir/(name+".log"));
   evidence<<"PID "<<r.pid<<" exit "<<r.exit<<" role "<<name<<'\n'<<r.output<<std::flush;
   if(r.exit)std::cerr<<r.output;
   check(r.exit==0&&r.pid!=previous,"normal distinct producer/consumer process lifetime");previous=r.pid;
  }
  if(game){
   const auto cli=exe.parent_path()/"mmodern.exe",save=dir/(std::to_string(seed)+".mmsave");const auto before=diskBytes(save);
   const auto r=child_test::launch(cli,{L"--load-game",game->wstring(),save.wstring()},dir/(std::to_string(seed)+"-cli-load.log"),true,true);
   evidence<<"EXECUTABLE PID "<<r.pid<<" exit "<<r.exit<<'\n'<<r.output<<std::flush;
   check(r.exit==0&&r.output.find("Resumed ")!=std::string::npos&&r.output.find("VictoryQuiescent")!=std::string::npos,"real executable v3 mode/inspection/normal exit");
   check(diskBytes(save)==before,"real executable load/inspection/exit changed save");
  }
 }
 if(game){
  const auto cli=exe.parent_path()/"mmodern.exe";
  for(unsigned fault=0;fault<4;++fault){
   const auto path=dir/("invalid-"+std::to_string(fault)+".mmsave");
   if(fault==1){auto saved=XeenSaveFile::read(dir/"56.mmsave");saved.resources.clouds.crc32^=1;XeenSaveFile::write(path,saved);}
   if(fault==2){auto saved=XeenSaveFile::read(dir/"56.mmsave");saved.characters[1].conditions[0]=1;XeenSaveFile::write(path,saved);}
   if(fault==3){std::ofstream out(path);out<<"invalid completed save";}
   const auto r=child_test::launch(cli,{L"--load-game",game->wstring(),path.wstring()},dir/("cli-invalid-"+std::to_string(fault)+".log"));
   evidence<<"EXECUTABLE INVALID PID "<<r.pid<<" exit "<<r.exit<<'\n'<<r.output<<std::flush;
   check(r.exit==3&&r.output.find("Resumed ")==std::string::npos,"real executable invalid load must fail before gameplay");
  }
  for(bool seed:{false,true}){
   const auto target=dir/(seed?"fresh-seed-target.mmsave":"fresh-target.mmsave");
   std::vector<std::wstring> args{L"--encounter-27"};if(seed){args.push_back(L"--combat-seed");args.push_back(L"56");}
   args.insert(args.end(),{game->wstring(),L"--save-file",target.wstring()});
   const auto r=child_test::launch(cli,args,dir/(seed?"cli-fresh-seed.log":"cli-fresh.log"),true,true);
   evidence<<"EXECUTABLE FRESH PID "<<r.pid<<" exit "<<r.exit<<'\n'<<r.output<<std::flush;
   check(r.exit==0&&!fs::exists(target),"real executable accepts target; no automatic save on exit");
  }
 }
}
}
int main(int argc,char **argv){try{
 const auto exe=fs::absolute(fs::u8path(argv[0]));
 if(argc==6&&std::string(argv[1])=="--child")return child(std::string(argv[2])=="-"?std::optional<fs::path>{}:fs::u8path(argv[2]),fs::u8path(argv[3]),std::stoul(argv[4]),argv[5]);
 if(argc==4&&std::string(argv[1])=="--oracle"){
  Harness h(fs::u8path(argv[2]));auto s=h.services(56);Oracle oracle(h,s,56);oracle.disk(fs::u8path(argv[3]));
  std::cout<<"PASS physical seed-56 save oracle: all 30 owners, 144 item bytes each, supplements/XP, conditions/HP/SP, context, flags, completion and wire fields.\n";return 0;
 }
 const bool original=argc==4&&std::string(argv[1])=="original";
 const auto root=original?fs::absolute(fs::u8path(argv[3])):fs::current_path()/"completed-gameplay-tests";
 fs::create_directories(root);const auto dir=root/("run-"+std::to_string(GetCurrentProcessId())+"-"+std::to_string(GetTickCount64()));check(fs::create_directory(dir),"new evidence directory");
 if(original||argc==2&&std::string(argv[1])=="sdl"){processes(exe,original?std::optional<fs::path>{fs::absolute(fs::u8path(argv[2]))}:std::optional<fs::path>{},dir);if(!original)lostUpload();}
 else {check(argc==1,"completed test arguments");directLifecycle(dir);saveCallbacks(dir);presentationFailures(dir);reentryFailures(dir);ordinarySourceGuards(dir);disposedRecovery();nestedSourceAndLifetime(dir);inspectionAndHandoff(dir);}
 std::cout<<"Completed production evidence passed: "<<dir.u8string()<<'\n';return 0;
}catch(const std::exception &e){std::cerr<<e.what()<<'\n';return 1;}}
