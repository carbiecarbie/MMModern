// Test-executable-only linker wrapper. The real main, Application startup,
// resources, EventFlow, F9 target transaction and SDL loop execute unchanged.
// This adapter observes borrowed owners and queues physical SDL keys only.
#include "XeenJourneyGameplayTestSupport.h"
#include "XeenRestoreReplayProbe.h"
#define SDL_MAIN_HANDLED
#include <SDL.h>
#include <deque>
#include <cstdlib>
using namespace journey_gameplay_test;
namespace fs=std::filesystem;
#define PLAY_SYMBOL "_ZNK7mmodern11Application12playGameplayERKNS_20XeenGameplayServicesENS_10XeenCameraERKSt8optionalINSt10filesystem7__cxx114pathEEbNS_18XeenEncounterEntryES5_IjE"
extern "C" int realPlay(const Application *,const XeenGameplayServices &,XeenCamera,const std::optional<fs::path> &,bool,XeenEncounterEntry,std::optional<std::uint32_t>) asm("__real_" PLAY_SYMBOL);
extern "C" int wrappedPlay(const Application *,const XeenGameplayServices &,XeenCamera,const std::optional<fs::path> &,bool,XeenEncounterEntry,std::optional<std::uint32_t>) asm("__wrap_" PLAY_SYMBOL);
namespace {
void key(SDL_Keycode code, Uint32 type=SDL_KEYDOWN, Uint8 repeat=0) {
 SDL_Event e{};e.type=type;e.key.keysym.sym=code;e.key.keysym.scancode=SDL_GetScancodeFromKey(code);
 e.key.repeat=repeat;e.key.timestamp=SDL_GetTicks()+1;check(SDL_PushEvent(&e)==1,"witness key enqueue");
}
void press(SDL_Keycode code){key(code);key(code,SDL_KEYUP);}
Bytes bytes(const fs::path &p){return completed_test::diskBytes(p);}
void diskOracle(const JourneyOracle &oracle,const fs::path &path) {
 auto expected=oracle.expected;auto &j=*expected.journey;
 const auto &a=oracle.actors[5];j.actors={{a.id,a.x,a.y,a.hp,a.activated,a.lifecycle,a.status,a.lifecycle==XeenActorLifecycle::Defeated}};
 const auto actual=bytes(path);
 check(actual==XeenSaveFormat::encode(expected),"independent literal full wire oracle");
 const auto b=actual.size()-1060;
 check(actual[8]==4&&actual[b]==3&&actual[b+1]==1&&actual[b+3]==1&&actual[b+5]==1&&actual[b+39]==30,"literal v4 suffix header");
 for(unsigned i=0;i<30;++i)check(actual[b+40+i*33]==i,"literal v4 owner order");
 check(actual[b+1030]==56&&actual[b+1031]==0&&actual[b+1037]==27&&actual[b+1039]==1,"literal seed and actor counts");
}
}
extern "C" int wrappedPlay(const Application *app,const XeenGameplayServices &original,XeenCamera camera,
 const std::optional<fs::path> &target,bool resume,XeenEncounterEntry entry,std::optional<std::uint32_t> seed) {
 try {
  const char *modeValue=std::getenv("MMODERN_JOURNEY_WITNESS");check(modeValue,"witness mode required");const std::string mode=modeValue;
  Harness h;auto services=original;JourneyOracle oracle(original);
  if(mode=="consumer"||mode=="final") {
   oracle.ring(true,true);oracle.victory();oracle.ring(false,mode=="consumer");
   oracle.expected.camera=mode=="consumer"?XeenCamera{20,14,2,XeenDirection::North}:XeenCamera{20,13,2,XeenDirection::West};
   oracle.expected.journey->context->minutes=mode=="consumer"?512:522;oracle.expected.journey->context->ctr24=mode=="consumer"?5:7;
  }
  if(mode=="moved-load") {
   oracle.expected.camera={20,14,1,XeenDirection::East};oracle.expected.journey->context->minutes=490;oracle.expected.journey->context->ctr24=2;
   oracle.actors[5].x=13;oracle.actors[5].y=1;
  }
  if(resume) { check(target.has_value(),"load target");diskOracle(oracle,*target);++replay_test::depth; }
  const auto configure=services.configureFlow;
  services.configureFlow=[&](auto &flow,const auto &c){if(configure)configure(flow,c);h.flow=&flow;};
  services.observeGameplay=[&](auto &w,auto &e,const auto &p,auto &c,const auto &f){h.world=&w;h.eventSystem=&e;h.party=&p;h.camera=&c;h.flags=&f;
   oracle.state(h);check(!h.flow->inventoryOpen()&&!h.flow->encounter()->combat(),"exact startup before input");};
  if(!std::getenv("MMODERN_JOURNEY_REAL_CLOCK"))services.clock=[&]{return h.now;};
  services.observeSaveStage=[&](auto stage){
   ++h.saves;
   if(stage==XeenGameplayServices::SaveStage::Preflight)++replay_test::depth;
   if(stage==XeenGameplayServices::SaveStage::Write){--replay_test::depth;check(replay_test::unexpected==0,"CLI F9 detached preflight has zero gameplay/RNG replay");}
  };
  services.show=[&](const auto &first,const auto &handler,const auto &escape,const auto &idle,const auto &status){
   if(resume){check(replay_test::unexpected==0,"CLI load has zero gameplay/RNG replay");--replay_test::depth;}
   std::deque<SDL_Keycode> queue;
   const auto append=[&](std::initializer_list<SDL_Keycode> keys){queue.insert(queue.end(),keys.begin(),keys.end());};
   const auto transfer=[&](bool back){append({SDLK_i,back?SDLK_F1:SDLK_F6,SDLK_RIGHT,SDLK_RIGHT,SDLK_2,SDLK_t,
    back?SDLK_F6:SDLK_F1,SDLK_RETURN,back?SDLK_F6:SDLK_F1,SDLK_2,SDLK_e,SDLK_i});};
   unsigned stage=0,commands=0,loops=0;bool done=false;std::uint64_t stale=0;
   auto input=handler;
   const auto drive=[&]{
    check(++loops<1500,"bounded SDL witness");
    if(done)return;
    if(!queue.empty()){const auto code=queue.front();queue.pop_front();press(code);return;}
    if(stage==0){oracle.state(h);stale=*handler.displayedInput();
     if(mode=="producer") {transfer(false);stage=1;}
     else if(mode=="consumer") {append({SDLK_i,SDLK_F6,SDLK_RIGHT,SDLK_RIGHT,SDLK_2,SDLK_e,SDLK_i,SDLK_LEFT,SDLK_UP});stage=5;}
     else if(mode=="moved") {append({SDLK_RIGHT,SDLK_UP});stage=6;}
     else if(mode=="moved-load") {press(SDLK_PERIOD);stage=7;}
     else {stage=9;}
     return;
    }
    if(stage==1){oracle.ring(true,true);oracle.state(h);check(!h.flow->encounter()->combat(),"no coordinator during actual SDL inventory");press(SDLK_PERIOD);stage=2;return;}
    if(stage==2){
     if(h.flow->encounter()->combat()) {
      check(!h.flow->encounter()->terminal(),"unexpected connected terminal");
      if(h.flow->encounter()->combat()->phase()==Phase::PlayerReady) {
       press(commands++<6?SDLK_b:SDLK_SPACE);
       // Same fixed poll batch: stale F9 and held/repeated command may not work.
       key(SDLK_F9);key(SDLK_F9,SDL_KEYUP);key(SDLK_SPACE,SDL_KEYDOWN,1);
      }
      return;
     }
     check(commands==8,"literal connected eight command sequence");oracle.victory();oracle.state(h);
     check(h.saves==0,"no stale combat F9 providers");
     handler.withDisplayedInput(SaveGameAction{},stale);check(h.saves==0,"stale returned F9 Application gate");
     append({SDLK_RIGHT,SDLK_UP,SDLK_LEFT,SDLK_UP});stage=3;return;
    }
    if(stage==3){if(!h.flow->encounter()->journeyQuiet())return;
     oracle.expected.camera={20,14,2,XeenDirection::North};oracle.expected.journey->context->minutes=512;oracle.expected.journey->context->ctr24=5;oracle.state(h);
     transfer(true);stage=4;return;
    }
    if(stage==4){oracle.ring(false,true);oracle.state(h);stage=9;return;}
    if(stage==5){if(!h.flow->encounter()->journeyQuiet())return;
     oracle.ring(false,false);oracle.expected.camera={20,13,2,XeenDirection::West};oracle.expected.journey->context->minutes=522;oracle.expected.journey->context->ctr24=7;oracle.state(h);stage=9;return;}
    if(stage==6){if(!h.flow->encounter()->journeyQuiet())return;
     oracle.expected.camera={20,14,1,XeenDirection::East};oracle.expected.journey->context->minutes=490;oracle.expected.journey->context->ctr24=2;
     oracle.actors[5].x=13;oracle.actors[5].y=1;oracle.state(h);stage=9;return;}
    if(stage==7){check(h.flow->encounter()->combat()&&h.party->encounterContext->minutes==500&&h.party->encounterContext->ctr24==3,"moved-anchor loaded engagement literal oracle");
     check(h.world->sessionState().actors()[5].x==14&&h.world->sessionState().actors()[5].y==1,"moved-anchor continuation uses saved actor");
     done=true;SDL_Event quit{};quit.type=SDL_QUIT;SDL_PushEvent(&quit);return;}
    if(stage==9){oracle.state(h);press(SDLK_F9);stage=10;return;}
    if(stage==10){check(h.saves==3&&status().find("Saved")!=std::string::npos,"actual CLI/SDL/F9 transaction");diskOracle(oracle,*target);
     done=true;SDL_Event quit{};quit.type=SDL_QUIT;SDL_PushEvent(&quit);}
   };
   unsigned stable=0,total=0;
   auto timedIdle=[&] {
    check(++total<1500,"bounded SDL loop");
    const auto *combat=h.flow->encounter()->combat();
    if((combat&&combat->pending()!=Work::None)||h.flow->encounter()->state().pending())h.now+=100;
    auto frame=idle();
    if(frame)stable=0;
    else if(++stable>=2){stable=0;drive();}
    return frame;
   };
   const bool success=original.show(first,input,escape,timedIdle,status);
   check(success&&done,"actual SDL witness completed and exited");
   std::cout<<"CLI SDL witness "<<mode<<" passed; exact all-owner/actor/context/wire oracle\n";return success;
  };
  return realPlay(app,services,camera,target,resume,entry,seed);
 }catch(const std::exception &e){std::cerr<<"Witness failed: "<<e.what()<<'\n';return 8;}
}
