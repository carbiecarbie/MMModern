// Instrumented child of the real Application/CLI, driven through its SDL input handler.
#include "app/Application.h"
#include "app/XeenGameplayServices.h"
#include "app/XeenEventFlow.h"
#include "platform/XeenSaveFile.h"
#include "formats/xeen/XeenSaveFormat.h"
#include "games/xeen/XeenIndoorScene.h"
#include "XeenRestoreReplayProbe.h"
#define SDL_MAIN_HANDLED
#include <SDL.h>
#include <cstdlib>
#include <iostream>
#include <stdexcept>
#include <filesystem>

using namespace mmodern;
namespace fs=std::filesystem;
#define PLAY_SYMBOL "_ZNK7mmodern11Application12playGameplayERKNS_20XeenGameplayServicesENS_10XeenCameraERKSt8optionalINSt10filesystem7__cxx114pathEEbNS_18XeenEncounterEntryES5_IjES5_ItE"
extern "C" int realPlay(const Application *,const XeenGameplayServices &,XeenCamera,const std::optional<fs::path> &,bool,XeenEncounterEntry,std::optional<std::uint32_t>,std::optional<std::uint16_t>) asm("__real_" PLAY_SYMBOL);
extern "C" int wrappedPlay(const Application *,const XeenGameplayServices &,XeenCamera,const std::optional<fs::path> &,bool,XeenEncounterEntry,std::optional<std::uint32_t>,std::optional<std::uint16_t>) asm("__wrap_" PLAY_SYMBOL);
namespace {
void check(bool ok,const char *message){if(!ok)throw std::runtime_error(message);}
}
extern "C" int wrappedPlay(const Application *app,const XeenGameplayServices &original,XeenCamera camera,
 const std::optional<fs::path> &target,bool resume,XeenEncounterEntry entry,
 std::optional<std::uint32_t> seed,std::optional<std::uint16_t> contract) {
 try {
  replay_test::journeyInitializations=replay_test::journeyConstructions=0;
  replay_test::actions=replay_test::pulses=replay_test::retirements=0;
  replay_test::commands=replay_test::draws=0;
  replay_test::constructions=replay_test::services=replay_test::preparations=0;
  replay_test::timePreparations=replay_test::eventExecutions=0;
  replay_test::transfers=replay_test::equipmentChanges=0;
  // Explicit legacy-entry control: retain 8/8 coverage after production selects 8/9.
  // M38 has a separate fresh-production CLI witness.
  if (!resume && contract==9) contract=8;
  check(target.has_value(),"M37 witness needs save file");
  check(resume || (entry==XeenEncounterEntry::Journey && contract==8),"M37 CLI content 8 expected");
  auto services=original;
  XeenEventFlow *flow=nullptr;XeenWorld *world=nullptr;
  const XeenPartyState *party=nullptr;const XeenCamera *position=nullptr;const XeenGameFlags *flags=nullptr;
  std::uint64_t cycle=0,now=0;
  unsigned saveStages=0;
  const std::string stage=std::getenv("MMODERN_M37_STAGE")?std::getenv("MMODERN_M37_STAGE"):"fresh";
  bool manifestProbe=false;
  bool manifestMismatch=true;
  if(stage=="manifest-mismatch") {
   services.resources.vertigoManifest=[&](auto &w,const auto &e,const auto &mon) {
    manifestProbe=true;auto altered=mon;if(manifestMismatch)altered.at(2).raw.at(20)^=1;
    original.resources.vertigoManifest(w,e,altered);
   };
  }
  services.clock=[&]{return now;};
  bool cityComposeFailed=false;
  bool candidateProbe=false;
  bool visualProbe=false;
  const auto originalCompose=original.composeEncounter;
  services.composeEncounter=[&](auto &w,const auto &p,const auto &c,auto phase,auto actor){
   if(stage=="fresh" && c.mapId==XeenMapIdentity(28) && !visualProbe) {
    // Derived-view control over the real admitted logical world, not a route shortcut.
    for(const auto &cell:std::array<std::array<int,2>,11>{{{15,0},{15,1},{15,2},{15,3},{15,4},
      {16,1},{16,2},{16,3},{16,4},{14,4},{13,4}}})
    for(unsigned direction=0;direction<4;++direction) for(bool night:{false,true}) {
     const XeenCamera view{28,cell[0],cell[1],static_cast<XeenDirection>(direction)};
     const auto sampled=w.sampleCell(28,view.x,view.y);
     check(sampled && !(sampled->cell->flags&8),"admitted original street unexpectedly has a ceiling flag");
     const auto commands=XeenIndoorScene().build(w,view,nullptr,nullptr,phase,{},night);
     for(unsigned i=0;i<2;++i)check(commands[i].geometry().resourceName==(night?"night.sky":"sky.sky") &&
      commands[i].geometry().frame==i,"logical Vertigo route sky mismatch across seam/facings");
     for(const auto &command:commands)if(command.actor())check(command.drawOptions().slimePalettePhase==-1,
      "admitted Slime was recolored during route composition");
    }
    XeenCamera westBoundary{28,13,4,XeenDirection::West};
    check(XeenMovement().apply(w,westBoundary,NavigationAction::MoveForward)==XeenMovementResult::BlockedByMapBoundary &&
     westBoundary.x==13 && westBoundary.y==4,"M38 widened the legacy 8/8 west boundary");
    visualProbe=true;
   }
   if(resume && c.mapId==XeenMapIdentity(28) && !candidateProbe && stage.rfind("restore-candidate-",0)==0) {
    candidateProbe=true;
    auto &actor=const_cast<XeenActor &>(w.sessionState().regionalActors(28).at(35));
    ++actor.hp;
    if(stage=="restore-candidate-aba") --actor.hp;
   }
   if(c.mapId==XeenMapIdentity(28) && !candidateProbe && stage.rfind("candidate-",0)==0) {
    candidateProbe=true;
    if(stage=="candidate-cache-rebuild") {
     // The entrance question's Event stack has already returned. Invoke the
     // retained providers through a real destination cache miss on delayed Yes.
     w.discardMapCache();w.map(28);w.objectFile(28);
    } else if(stage=="candidate-aba-container") {
     auto &state=const_cast<XeenSessionWorldState &>(w.sessionState());
     const auto before=state;state=XeenSessionWorldState{};state=before;
    } else if(stage=="candidate-aba-party") {
     auto &item=const_cast<XeenPartyState &>(p).roster.at(0).weapons[0].state;
     const auto before=std::uint8_t(item);item^=1;item=before;
    } else if(stage=="candidate-aba-camera") {
     auto &camera=const_cast<XeenCamera &>(c);++camera.x;--camera.x;
    } else if(stage=="candidate-aba-flags") {
     auto &live=const_cast<XeenGameFlags &>(*flags);const bool before=live.isSet(9);
     if(before){live.clear(9);live.set(9);}else{live.set(9);live.clear(9);}
    } else if(stage=="candidate-aba-overlay") {
     const auto before=w.sessionState();w.disableObject({28,0});
     const_cast<XeenSessionWorldState &>(w.sessionState())=before;
    } else if(stage=="candidate-aba-rng") {
     auto &random=const_cast<XeenMutableOptional<XeenJourneyRandomState> &>(w.sessionState().journeyRandom());
     ++random->count;--random->count;
    } else if(stage=="candidate-aba-treasure") {
     auto &gold=const_cast<XeenPartyState &>(p).monsterTreasure->gold;++gold;--gold;
    } else {
     auto &actor=const_cast<XeenActor &>(w.sessionState().regionalActors(28).at(35));
     ++actor.hp;
     if(stage=="candidate-aba") --actor.hp;
    }
   }
   if(stage=="fail-city-compose" && c.mapId==XeenMapIdentity(28) && !cityComposeFailed) {
    cityComposeFailed=true;throw std::runtime_error("M37 injected destination composition failure");
   }
   return originalCompose(w,p,c,phase,actor);
  };
  services.observeSaveStage=[&](auto){++saveStages;};
  const auto observe=original.observeGameplay;
  services.observeGameplay=[&](auto &w,auto &e,const auto &p,auto &c,const auto &f){
   if(observe)observe(w,e,p,c,f);world=&w;party=&p;position=&c;flags=&f;
  };
  const auto configure=original.configureFlow;
  services.configureFlow=[&](auto &f,const auto &c){if(configure)configure(f,c);flow=&f;};
  services.show=[&](const IndexedFrame &first,const auto &handler,const auto &escape,const auto &idle,const auto &status) {
   check(flow && world && party && position && flags && first.isValid(),"M37 production owners absent");
   const auto present=[&]{handler.framePresented(flow->frame().presentation());};
   const auto input=[&](PlayerAction action){present();handler.beginCycle(++cycle);const auto token=handler.displayedInput();check(bool(token),"M37 input token absent");handler.withDisplayedInput(action,*token);present();};
   const auto pulse=[&]{now+=100;handler.beginCycle(++cycle);idle();present();};
   const auto deniedSave=[&]{
    check(!flow->canSave(),"M37 mandatory Event work exposed Quiet save");
    const auto before=XeenSaveFormat::encode(XeenSaveFile::read(*target));
    input(SaveGameAction{});
    check(!flow->canSave() && XeenSaveFormat::encode(XeenSaveFile::read(*target))==before,
     "M37 F9 changed disk during mandatory Event work");
   };
   bool combatSaveChecked=false;
   const auto settle=[&]{for(unsigned n=0;n<30000;++n){
   if(world->sessionState().journeyActivity()!=XeenJourneyActivity::Quiet)
    check(!flow->canSave(),"M37 mandatory settlement exposed Quiet save");
   if(flow->encounter()->combat()) {
     if(!combatSaveChecked && fs::exists(*target)) {deniedSave();combatSaveChecked=true;}
     const auto *combat=flow->encounter()->combat();
     if(combat->phase()==XeenCombatPhase::Failed || combat->phase()==XeenCombatPhase::Defeat || combat->phase()==XeenCombatPhase::SupportStopped)
      throw std::runtime_error("M37 combat stopped");
     if(combat->phase()==XeenCombatPhase::PlayerReady)input(AttackAction{});else pulse();
     continue;
    }
    if(world->sessionState().journeyActivity()==XeenJourneyActivity::Event && flow->blocksGameplay()) {input(AcknowledgeAction{});continue;}
    if(flow->encounter()->state().phase()==XeenEncounterPhase::SupportStopped)throw std::runtime_error("M37 approach stopped");
    if(flow->canSave())return;
    pulse();
   }throw std::runtime_error("M37 settlement bound");};
   const auto nav=[&](NavigationAction action){check(flow->canSave(),"M37 navigation requires Quiet");input(action);settle();};
   const auto checkpoint=[&](char label){
    const unsigned before=saveStages;
    auto sdl=handler;sdl.closed={};sdl.failed={};
    bool queued=false;unsigned loops=0;
    sdl.beginCycle=[&](std::uint64_t){handler.beginCycle(++cycle);
     if(!queued){queued=true;SDL_Event key{};key.type=SDL_KEYDOWN;
      key.key.keysym.sym=SDLK_F9;key.key.keysym.scancode=SDL_SCANCODE_F9;
      key.key.timestamp=SDL_GetTicks()+1;
      check(SDL_PushEvent(&key)==1,"M37 SDL F9 enqueue");}
    };
    const auto sdlIdle=[&]()->std::optional<IndexedFrame>{
     check(++loops<100,"M37 SDL F9 loop bound");
     auto frame=idle();
     if(saveStages==before+3){SDL_Event quit{};quit.type=SDL_QUIT;
      check(SDL_PushEvent(&quit)==1,"M37 SDL quit enqueue");}
     return frame;
    };
    check(original.show(flow->frame(),sdl,escape,sdlIdle,status) && queued &&
     saveStages==before+3 && status().find("Saved")!=std::string::npos,
     "M37 SDL F9 checkpoint failed");
    const auto copy=target->parent_path()/(target->stem().string()+"-"+label+".mmsave");
    fs::copy_file(*target,copy,fs::copy_options::overwrite_existing);
    std::cout<<"M37 CHECKPOINT "<<label<<" bytes="<<XeenSaveFormat::encode(XeenSaveFile::read(copy)).size()<<'\n';
   };
   const auto enterFromA=[&]{
    nav(NavigationAction::MoveForward);check(position->x==10&&position->y==13,"M37 entrance cell");
    const auto sourceFrame=flow->frame(),sourceToken=handler.displayedInput();
    check(bool(sourceToken),"M37 source frame input identity");
    input(InteractionAction{});deniedSave();
    input(NoAction{});settle();check(position->mapId==XeenMapIdentity(23),"M37 entrance No camera");
    input(InteractionAction{});input(YesAction{});settle();
    check(position->mapId==XeenMapIdentity(28)&&position->x==15&&position->y==0,"M37 entrance arrival");
    const auto before=XeenSaveFormat::encode(XeenSaveState::capture(original.resources.signature,*party,*position,*flags,*world));
    check(!flow->acceptsFrame(sourceFrame.presentation()),"M37 source frame survived destination publication");
    bool staleFrame=false;
    try {handler.framePresented(sourceFrame.presentation());}catch(const std::exception &){staleFrame=true;}
    check(staleFrame,"M37 stale concrete frame was presented");
    try {handler.withDisplayedInput(NavigationAction::MoveForward,*sourceToken);}catch(const std::exception &){}
    check(before==XeenSaveFormat::encode(XeenSaveState::capture(original.resources.signature,*party,*position,*flags,*world)),
     "M37 stale source callback commanded destination");
   };
   const auto toB=[&](unsigned selected){
    nav(NavigationAction::MoveForward);nav(NavigationAction::TurnRight);nav(NavigationAction::MoveForward);
    nav(NavigationAction::TurnLeft);nav(NavigationAction::MoveForward);
    check(position->x==16&&position->y==2&&world->sessionState().regionalActors(28).at(selected).lifecycle==XeenActorLifecycle::Defeated,"M37 selected Slime defeat");
   };
   const auto fromBToC=[&](unsigned expectedCount=52){
    nav(NavigationAction::MoveForward);nav(NavigationAction::MoveForward);
    nav(NavigationAction::TurnLeft);nav(NavigationAction::MoveForward);
    nav(NavigationAction::MoveForward);nav(NavigationAction::MoveForward);
    check(position->x==13&&position->y==4&&position->direction==XeenDirection::West,"M37 Ironworks exterior");
    nav(NavigationAction::MoveBackward);nav(NavigationAction::MoveBackward);nav(NavigationAction::TurnLeft);
    nav(NavigationAction::MoveForward);nav(NavigationAction::MoveForward);
    nav(NavigationAction::MoveForward);nav(NavigationAction::MoveForward);
    check(position->x==15&&position->y==0&&position->direction==XeenDirection::South,"M37 exit cell");
    input(InteractionAction{});deniedSave();input(NoAction{});settle();
    check(position->mapId==XeenMapIdentity(28),"M37 exit No camera");
    input(InteractionAction{});deniedSave();input(YesAction{});settle();
    check(position->mapId==XeenMapIdentity(23)&&position->x==10&&position->y==12&&position->direction==XeenDirection::South,"M37 mainland return");
    check(world->sessionState().regionalActors(28).size()==expectedCount,"M37 flag-9 reset predicate");
   };
   const auto fromCToD=[&]{
    nav(NavigationAction::TurnLeft);nav(NavigationAction::TurnLeft);nav(NavigationAction::MoveForward);
    input(InteractionAction{});input(YesAction{});settle();
    check(position->mapId==XeenMapIdentity(28)&&position->x==15&&position->y==0,"M37 revisit entrance");
    toB(36);
   };
   present();
   if(resume) {
    check(!replay_test::journeyInitializations && !replay_test::journeyConstructions &&
     !replay_test::actions && !replay_test::pulses && !replay_test::commands &&
     !replay_test::draws && !replay_test::retirements && !replay_test::constructions &&
     !replay_test::services && !replay_test::preparations &&
     !replay_test::timePreparations && !replay_test::eventExecutions &&
     !replay_test::transfers && !replay_test::equipmentChanges,
     "M37 fresh restore replayed initialization, Event, combat, time or RNG");
    const auto saved=XeenSaveFile::read(*target),live=XeenSaveState::capture(original.resources.signature,*party,*position,*flags,*world);
    check(XeenSaveFormat::encode(saved)==XeenSaveFormat::encode(live),"M37 fresh restore changed saved bytes");
    std::cout<<"M37 RESTORE BEFORE INPUT "<<position->mapId.number<<','<<position->x<<','<<position->y<<'\n';
    check(stage=="A"||stage=="B"||stage=="C"||stage=="D"||stage=="B9","M37 resume checkpoint stage");
    if(stage=="B9") {
     check(flags->isSet(9) && world->sessionState().regionalActors(28).size()==46,
      "M37 artificial flag-9-true fixture");
     fromBToC(46);
     input(SaveGameAction{});check(status().find("Saved")!=std::string::npos,"M37 flag-9-true save");
     std::cout<<"M37 FLAG9 SKIP RESET passed\n";
     return true;
    }
    if(stage=="A") {enterFromA();toB(35);}
    if(stage=="A"||stage=="B") {fromBToC();fromCToD();}
    if(stage=="C")fromCToD();
    fromBToC();
    input(SaveGameAction{});check(status().find("Saved")!=std::string::npos,"M37 resumed final F9");
   } else {
    check(position->mapId==XeenMapIdentity(23) && position->x==9 && position->y==11 && position->direction==XeenDirection::West,"M37 mainland origin");
    nav(NavigationAction::TurnLeft);nav(NavigationAction::TurnLeft);nav(NavigationAction::MoveForward);
    nav(NavigationAction::TurnLeft);nav(NavigationAction::MoveForward);
    check(position->x==10 && position->y==12 && position->direction==XeenDirection::North,"M37 checkpoint A camera");
    checkpoint('A');
    if(stage=="manifest-mismatch") {
     nav(NavigationAction::MoveForward);
     try {input(InteractionAction{});}catch(const std::exception &){}
     check(manifestProbe && position->mapId==XeenMapIdentity(23) &&
      !world->sessionState().hasRegionalActors(28) && !flow->canSave(),
      "M37 immutable admission mismatch did not latch integrity failure");
     manifestMismatch=false;
     try {input(InteractionAction{});}catch(const std::exception &){}
     check(!flow->canSave() && !world->sessionState().hasRegionalActors(28),"M37 matching retry cleared integrity latch");
     std::cout<<"M37 IMMUTABLE INTEGRITY REJECTION passed\n";
     return true;
    }
    if(stage.rfind("candidate-",0)==0 && stage!="candidate-cache-rebuild") {
     nav(NavigationAction::MoveForward);
     const auto before=XeenSaveState::capture(original.resources.signature,*party,*position,*flags,*world);
     input(InteractionAction{});
     try {input(YesAction{});}catch(const std::exception &){}
     check(candidateProbe && position->mapId==XeenMapIdentity(23) &&
      !world->sessionState().hasRegionalActors(28) && !flow->canSave() &&
      flags->values()==before.gameFlags && world->sessionState().journeyRandom()==before.journey->random &&
      party->encounterContext==before.journey->context,
      "M37 candidate callback mutation escaped integrity guard");
     std::cout<<"M37 CANDIDATE INTEGRITY REJECTION passed\n";
     return true;
    }
    if(stage=="fail-city-compose") {
     const auto before=XeenSaveFile::read(*target);
     nav(NavigationAction::MoveForward);
     const auto atEntrance=XeenSaveState::capture(original.resources.signature,*party,*position,*flags,*world);
     input(InteractionAction{});input(YesAction{});
     check(cityComposeFailed && position->mapId==XeenMapIdentity(23) &&
      !world->sessionState().hasRegionalActors(28) &&
      flags->values()==before.gameFlags &&
      party->encounterContext->minutes==atEntrance.journey->context->minutes &&
      world->sessionState().journeyRandom()==atEntrance.journey->random &&
      XeenSaveFormat::encode(XeenSaveFile::read(*target))==XeenSaveFormat::encode(before),
      "M37 failed destination composition published partial transition");
     const auto &retained=world->sessionState().actors();
     check(retained.size()==atEntrance.journey->actors.size(),"M37 failed transition changed mainland actor count");
     for(unsigned i=0;i<retained.size();++i){const auto &live=retained[i];const auto &saved=atEntrance.journey->actors[i];
      check(live.id==saved.id && live.x==saved.x && live.y==saved.y && live.hp==saved.hp &&
       live.activated==saved.activated && live.lifecycle==saved.lifecycle &&
       bool(world->sessionState().accountedMonsters().count(live.id))==saved.accounted,
       "M37 failed transition changed inactive mainland actors");}
     settle();
     check(flow->canSave(),"M37 composition failure retained transition authority");
     input(SaveGameAction{});check(status().find("Saved")!=std::string::npos,"M37 recovered composition failure cannot save");
     input(InteractionAction{});input(YesAction{});settle();
     check(position->mapId==XeenMapIdentity(28) && flow->canSave(),"M37 recovered second transition failed");
     std::cout<<"M37 DESTINATION COMPOSE FAILURE atomic passed; recovery, F9 and retry passed\n";
     return true;
    }
    enterFromA();
    nav(NavigationAction::MoveForward);
    nav(NavigationAction::TurnRight);
    nav(NavigationAction::MoveForward);
    nav(NavigationAction::TurnLeft);
    nav(NavigationAction::MoveForward);
    check(position->x==16 && position->y==2,"M37 checkpoint B camera");
    std::cout<<"M37 SLIME 35 life="<<unsigned(world->sessionState().regionalActors(28).at(35).lifecycle)<<" hp="<<world->sessionState().regionalActors(28).at(35).hp<<'\n';
    checkpoint('B');
    check(XeenSaveFile::read(*target).camera.mapId==XeenMapIdentity(28),"M37 checkpoint B F9 camera");
    nav(NavigationAction::MoveForward);nav(NavigationAction::MoveForward);
    nav(NavigationAction::TurnLeft);nav(NavigationAction::MoveForward);
    nav(NavigationAction::MoveForward);nav(NavigationAction::MoveForward);
    check(position->x==13 && position->y==4 && position->direction==XeenDirection::West,"M37 door landmark camera");
    nav(NavigationAction::MoveBackward);nav(NavigationAction::MoveBackward);
    nav(NavigationAction::TurnLeft);
    nav(NavigationAction::MoveForward);nav(NavigationAction::MoveForward);
    nav(NavigationAction::MoveForward);nav(NavigationAction::MoveForward);
    check(position->x==15 && position->y==0 && position->direction==XeenDirection::South,"M37 city exit cell");
    input(InteractionAction{});check(!flow->canSave(),"M37 exit No question");input(NoAction{});settle();
    check(position->mapId==XeenMapIdentity(28),"M37 exit No retained city");
    input(InteractionAction{});check(!flow->canSave(),"M37 exit Yes question");input(YesAction{});settle();
    check(position->mapId==XeenMapIdentity(23) && position->x==10 && position->y==12 && position->direction==XeenDirection::South,"M37 original mainland return");
    check(world->sessionState().regionalActors(28).size()==52,"M37 original reset slot count");
    checkpoint('C');
    check(XeenSaveFile::read(*target).camera.mapId==XeenMapIdentity(23) && XeenSaveFile::read(*target).journey->vertigoActors->size()==52,"M37 checkpoint C F9 state");
    nav(NavigationAction::TurnLeft);nav(NavigationAction::TurnLeft);nav(NavigationAction::MoveForward);
    check(position->x==10 && position->y==13,"M37 revisit mainland entrance");
    input(InteractionAction{});input(YesAction{});settle();
    check(position->mapId==XeenMapIdentity(28) && position->x==15 && position->y==0,"M37 revisit entrance");
    nav(NavigationAction::MoveForward);nav(NavigationAction::TurnRight);nav(NavigationAction::MoveForward);
    nav(NavigationAction::TurnLeft);nav(NavigationAction::MoveForward);
    check(position->x==16 && position->y==2 && world->sessionState().regionalActors(28).at(36).lifecycle==XeenActorLifecycle::Defeated,"M37 reset entrance Slime 36");
    checkpoint('D');
    check(XeenSaveFile::read(*target).camera.mapId==XeenMapIdentity(28) && XeenSaveFile::read(*target).journey->vertigoActors->size()==52,"M37 checkpoint D F9 state");
    fromBToC();input(SaveGameAction{});check(status().find("Saved")!=std::string::npos,"M37 uninterrupted final F9");
   }
   std::cout<<"M37 CLI SMOKE "<<position->mapId.number<<','<<position->x<<','<<position->y<<" passed\n";
   return true;
  };
  return realPlay(app,services,camera,target,resume,entry,seed,contract);
 } catch(const std::exception &e){std::cerr<<"M37 CLI witness: "<<e.what()<<'\n';return 8;}
}
