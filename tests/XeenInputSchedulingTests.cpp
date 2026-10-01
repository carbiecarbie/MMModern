// Synthetic restored checkpoints, original resources, production Application/SDL.
// The earned M41 process witness remains a separate test.
#include "XeenTrainingTestSupport.h"
#include "app/Application.h"
#include "app/XeenGameplayServices.h"
#include "games/xeen/CloudsMapComposer.h"
#define SDL_MAIN_HANDLED
#include <SDL.h>
#include <thread>
#include <atomic>
#include <chrono>
#include <iostream>
using namespace training_test;
namespace {
std::function<void()> uploadHook,copyHook;
void key(SDL_Keycode code,Uint32 type=SDL_KEYDOWN,Uint8 repeat=0) {
 SDL_Event e{};e.type=type;e.key.keysym.sym=code;e.key.keysym.scancode=SDL_GetScancodeFromKey(code);e.key.repeat=repeat;
 check(SDL_PushEvent(&e)==1,"scheduling key enqueue");
}
void tap(SDL_Keycode code) {key(code);key(code,SDL_KEYUP);}
void quit(){SDL_Event e{};e.type=SDL_QUIT;SDL_PushEvent(&e);}
}
extern "C" int __real_SDL_UpdateTexture(SDL_Texture *,const SDL_Rect *,const void *,int);
extern "C" int __wrap_SDL_UpdateTexture(SDL_Texture *t,const SDL_Rect *r,const void *p,int pitch){if(uploadHook)uploadHook();return __real_SDL_UpdateTexture(t,r,p,pitch);}
extern "C" int __real_SDL_RenderCopy(SDL_Renderer *,SDL_Texture *,const SDL_Rect *,const SDL_Rect *);
extern "C" int __wrap_SDL_RenderCopy(SDL_Renderer *r,SDL_Texture *t,const SDL_Rect *s,const SDL_Rect *d){if(copyHook)copyHook();return __real_SDL_RenderCopy(r,t,s,d);}
namespace {
struct Harness {
 Inputs &in;XeenEventFlow *flow=nullptr;XeenWorld *world=nullptr;const XeenPartyState *party=nullptr;XeenCamera *camera=nullptr;
 std::function<void()> composeHook;
 IndexedFrame::Presentation observedOrigin;
 std::uint64_t liveCompositionMicros=0,maxLiveCompositionMicros=0;
 explicit Harness(Inputs &i):in(i){}
 XeenGameplayServices services() {
  XeenGameplayServices s{in.resources(),[]{return XeenGameFlags{};},in.mapLoader(),in.objectLoader(),[&](auto id){return in.texts.load(id);},in.font,
   [](auto &,const auto &,const auto &,auto){return XeenEventFlow::Composition{frame(),false};},{},
   [&](auto &f,const auto &){flow=&f;f.drawTrainingArt=[&](auto &b){in.assets.drawTraining(b);};},{},
   [&](auto &w,auto &,const auto &p,auto &c,const auto &){world=&w;party=&p;camera=&c;}};
  s.composeEncounter=[&](auto &w,const auto &p,const auto &c,auto phase,auto actor){
   const bool live=flow && observedOrigin && flow->acceptsInputFrame(observedOrigin);
   const auto started=std::chrono::steady_clock::now();
   if(composeHook)composeHook();XeenEventFlow::Composition result;
   result.frame=CloudsMapComposer().compose(in.assets,w,p,c,{610},nullptr,phase,&result.containsOrdinaryAnimation,actor);
   if(live){
    check(flow->acceptsInputFrame(observedOrigin),"composition retired its acquired input origin");
    const auto elapsed=static_cast<std::uint64_t>(std::chrono::duration_cast<std::chrono::microseconds>(std::chrono::steady_clock::now()-started).count());
    liveCompositionMicros+=elapsed;maxLiveCompositionMicros=std::max(maxLiveCompositionMicros,elapsed);
   }
   return result;
  };
  return s;
 }
 int run(XeenGameplayServices &s,const XeenSaveSnapshot &source,const std::string &name) {
  const auto path=std::filesystem::absolute("scheduling-"+name+".mmsave");
  XeenSaveFile::write(path,source);
  const auto result=Application().playGameplay(s,source.camera,path,true);
  std::filesystem::remove(path);return result;
 }
};
void stress(Inputs &in) {
 Harness h(in);auto s=h.services();auto source=in.service();source.camera={28,10,10,XeenDirection::North};
 unsigned cosmetic=0;std::array<unsigned,4> counts{};unsigned dispatches=0;
 s.show=[&](const auto &first,const auto &handler,const auto &escape,const auto &idle,const auto &status){
  auto native=handler;std::thread sender;std::atomic<bool> stop=false,done=false,settled=false;std::atomic<unsigned> sent=0;
  struct Join {std::thread &t;std::atomic<bool> &stop;~Join(){stop=true;if(t.joinable())t.join();}} join{sender,stop};
  auto previous=first.presentation();auto epoch=handler.displayedInput();
  native.framePresented=[&](const auto &f){handler.framePresented(f);
   h.observedOrigin=f;
   if(f!=previous && handler.displayedInput()==epoch)++cosmetic;
   previous=f;epoch=handler.displayedInput();
   if(!sender.joinable())sender=std::thread([&]{
    for(unsigned n=0;n<80 && !stop;++n){
     // Separate ordinary human taps; offsets walk the 100 ms cosmetic cadence.
     std::this_thread::sleep_for(std::chrono::milliseconds(173+(n%5)*13));
     // Wait only for a preceding gameplay action's mandatory settlement.
     // Cosmetic composition/acquisition never clears this readiness flag.
     while(!settled && !stop)std::this_thread::sleep_for(std::chrono::milliseconds(1));
     if(stop)break;
     const auto code=n<40?(n%2?SDLK_s:SDLK_w):n<60?SDLK_a:SDLK_d;
     tap(code);++sent;
    }
    std::this_thread::sleep_for(std::chrono::milliseconds(600));done=true;
   });
  };
  native.withPresentedInput=[&](const auto &action,auto input,const auto &origin){
   settled=false;
   check(handler.acceptsInputFrame(origin),"stress origin was retired");
   const auto before=*h.camera;
   auto result=handler.withPresentedInput(action,input,origin);
   const auto *nav=std::get_if<NavigationAction>(&action);check(nav,"unexpected stress action");
   check(h.camera->x!=before.x || h.camera->y!=before.y || h.camera->direction!=before.direction,"stress dispatched without navigation publication");
   unsigned index=*nav==NavigationAction::MoveForward?0:*nav==NavigationAction::TurnLeft?1:*nav==NavigationAction::MoveBackward?2:3;
   ++counts[index];++dispatches;return result;
  };
  const bool ok=SdlWindow().showInteractive(first,"Real clock scheduling",native,escape,[&](){
   // framePresented and completeInputHandoff both precede SDL's strict queue
   // fence. Observe readiness in a later loop's idle callback, after that fence,
   // only while the exact acquired origin accepts input. A dispatched action
   // clears readiness; purely cosmetic work retains its live predecessor.
   if(h.flow->canSave() && h.observedOrigin && handler.acceptsInputFrame(h.observedOrigin))settled=true;
   if(done)quit();return idle();
  },status);
  check(sent==80,"stress sender incomplete");return ok;
 };
 check(h.run(s,source,"stress")==0,"real clock application failed");
 std::cout<<"REAL CLOCK W="<<counts[0]<<"/20 A="<<counts[1]<<"/20 S="<<counts[2]<<"/20 D="<<counts[3]<<"/20 cosmetics="<<cosmetic<<" total="<<dispatches<<"/80\n";
 std::cout<<"LIVE COSMETIC COMPOSITION total-us="<<h.liveCompositionMicros<<" maximum-us="<<h.maxLiveCompositionMicros<<" unavailable-boundary-samples=0\n";
 check(cosmetic>20 && counts==std::array<unsigned,4>{20,20,20,20},"real clock lost or duplicated a legitimate tap");
}
void services(Inputs &in) {
 for(unsigned mode=0;mode<5;++mode)for(unsigned timing:{1u,3u,4u}) {
  Harness h(in);auto s=h.services();auto source=in.service();std::uint64_t now=0;unsigned cycles=0,dispatches=0,loops=0;
  s.clock=[&]{return now;};bool armed=false,injected=false;IndexedFrame::Presentation a,b;
  const auto code=mode==0?SDLK_SPACE:mode==1?SDLK_F2:SDLK_RETURN;
  const auto inject=[&]{if(armed && !injected){injected=true;tap(code);}};
  h.composeHook=[&]{if(timing==1)inject();};copyHook=[&]{if(timing==3)inject();};
  s.show=[&](const auto &first,const auto &handler,const auto &escape,const auto &idle,const auto &status){
   const auto present=[&]{handler.framePresented(h.flow->frame().presentation());handler.completeInputHandoff(h.flow->frame().presentation());};
   const auto act=[&](PlayerAction action){handler.beginCycle(++cycles);handler.withPresentedInput(action,*handler.displayedInput(),h.flow->frame().presentation());present();};
   const auto prepare=[&]{for(unsigned n=0;n<1000;++n){handler.beginCycle(++cycles);if(idle()){present();return;}}check(false,"service setup did not settle");};
   present();
   if(mode){act(InteractionAction{});prepare();check(XeenTrainingTestAccess::menu(*h.flow),"service menu setup");}
   if(mode>=2)act(SelectMemberAction{1});
   if(mode>=3){act(AcknowledgeAction{});check(XeenTrainingTestAccess::quote(*h.flow),"quote setup");}
   if(mode==4){act(AcknowledgeAction{});prepare();}
   auto native=handler;native.beginCycle=[&](auto){handler.beginCycle(++cycles);};
   native.completeInputHandoff=[&](const auto &f){handler.completeInputHandoff(f);if(armed && f==b && timing==4)inject();};
   native.withPresentedInput=[&](const auto &action,auto epoch,const auto &origin){
    check(origin==(timing==4?b:a),"service response origin changed");++dispatches;return handler.withPresentedInput(action,epoch,origin);
   };
   const auto ok=SdlWindow().showInteractive(h.flow->frame(),"Service cosmetic tap",native,escape,[&]()->std::optional<IndexedFrame>{
    check(++loops<15,"service cosmetic tap lost");
    if(dispatches){check(dispatches==1,"service tap duplicated");
   if(mode==0 && !XeenTrainingTestAccess::menu(*h.flow))return idle();
   if(mode==0)check(h.world->sessionState().journeyActivity()==XeenJourneyActivity::Service,"Space not accepted");
   if(mode==1)check(XeenTrainingTestAccess::selected(*h.flow)==1,"F2 selection not accepted");
   if(mode==2)check(XeenTrainingTestAccess::quote(*h.flow),"menu Enter not accepted");
   if(mode==3)check(!XeenTrainingTestAccess::quote(*h.flow),"quote Enter not accepted");
   if(mode==4)check(XeenTrainingTestAccess::menu(*h.flow),"result Enter not accepted");
quit();return {};}
    if(!armed){armed=true;a=h.flow->frame().presentation();now+=100;
     auto frame=mode?h.flow->refresh(true):*idle();b=frame.presentation();check(a!=b && handler.acceptsInputFrame(a),"service cosmetic lost A");return frame;}
    return idle();
   },status);
   return ok;
  };
  check(h.run(s,source,"service")==0,"service scheduling failed");copyHook={};
  std::cout<<"SERVICE mode="<<mode<<" timing="<<timing<<" tap=1 dispatch="<<dispatches<<" PASS\n";
 }
}

void boundaries(Inputs &in) {
 for(unsigned timing=0;timing<7;++timing) {
  Harness h(in);auto s=h.services();auto source=in.service();source.camera={28,10,10,XeenDirection::North};
  bool armed=false,injected=false,acquired=false;unsigned dispatches=0,loops=0,cosmetics=0;std::uint64_t now=0;
  s.clock=[&]{return now;};IndexedFrame::Presentation a,b;std::optional<std::uint64_t> epoch;std::optional<XeenEncounterFlow::Ticket> ticket;
  const auto inject=[&]{if(!armed || injected)return;injected=true;key(SDLK_d);key(SDLK_d,SDL_KEYDOWN,1);key(SDLK_d);key(SDLK_d,SDL_KEYUP);};
  h.composeHook=[&]{if(armed && !injected && timing==1){check(h.flow->acceptsInputFrame(a),"A unavailable during composition");inject();}};
  copyHook=[&]{if(armed && timing==2)inject();};
  s.show=[&](const auto &first,const auto &handler,const auto &escape,const auto &idle,const auto &status){
   auto native=handler;
   native.framePresented=[&](const auto &f){
    handler.framePresented(f);
    if(armed && f!=a && handler.displayedInput()==epoch){
     b=f;++cosmetics;check(handler.acceptsInputFrame(a) && !handler.acceptsInputFrame(b),"cosmetic acquisition lost A or prematurely opened B");
     check(h.flow->encounter()->current(*ticket),"cosmetic consumed semantic/boundary authority");
     if(timing==3)inject();
     if(timing==6){quit();inject();}
    }
   };
   native.completeInputHandoff=[&](const auto &f){handler.completeInputHandoff(f);if(!armed || f!=b)return;acquired=true;
    check(!handler.acceptsInputFrame(a) && handler.acceptsInputFrame(f),"handoff did not replace exact origin");
    const auto before=*h.camera;handler.withPresentedInput(NavigationAction::TurnRight,*epoch,a);
    check(h.camera->direction==before.direction,"old A replay authorized B");
    if(timing==4 || (timing==5 && cosmetics>=2))inject();
    if(timing==5 && cosmetics<2){a=f;acquired=false;ticket=h.flow->encounter()->ticket();}
   };
   native.withPresentedInput=[&](const auto &action,auto input,const auto &origin){
    check(origin==(acquired?b:a),"response reinterpreted as another concrete frame");++dispatches;
    return handler.withPresentedInput(action,input,origin);
   };
   return SdlWindow().showInteractive(first,"Deterministic cosmetic boundary",native,escape,[&]()->std::optional<IndexedFrame>{
    check(++loops<20,"deterministic tap lost");
    if(dispatches){check(dispatches==1 && h.camera->direction==XeenDirection::East,"duplicate boundary action");quit();return {};}
    if(!armed){armed=true;a=first.presentation();epoch=handler.displayedInput();ticket=h.flow->encounter()->ticket();if(timing==0){inject();return {};}}
    now+=100;return idle();
   },status);
  };
  check(h.run(s,source,"boundary")==0,"boundary Application failed");
  if(timing==6)check(injected && dispatches==0,"key overtook queued close during cosmetic acquisition");
  uploadHook={};copyHook={};std::cout<<(timing==6?"CLOSE ORDER ":"BOUNDARY ")<<timing<<" tap=1 dispatch="<<dispatches<<" repeats=0 PASS\n";
 }
}
}
int main(int argc,char **argv){try{
 check(argc==2,"usage: input-scheduling <installation>");SDL_setenv("SDL_VIDEODRIVER","dummy",1);SDL_setenv("SDL_RENDER_DRIVER","software",1);
 const auto installation=XeenInstallationDetector().detect(argv[1]);check(bool(installation),"original installation absent");Inputs in(*installation);
 boundaries(in);services(in);stress(in);return 0;
}catch(const std::exception &e){uploadHook={};copyHook={};std::cerr<<e.what()<<'\n';return 1;}}
