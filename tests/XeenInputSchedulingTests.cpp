// Synthetic restored checkpoints, original resources, production Application/SDL.
// The earned M41 process witness remains a separate test.
#include "XeenProbeFired.h"
#include "XeenTrainingTestSupport.h"
#include "app/Application.h"
#include "app/XeenGameplayServices.h"
#include "games/xeen/CloudsMapComposer.h"
#include "games/xeen/CloudsUiComposer.h"
#include "platform/sdl/XeenMainScreenInput.h"
#define SDL_MAIN_HANDLED
#include <SDL.h>
#include <thread>
#include <atomic>
#include <chrono>
#include <iostream>
#include <cstdlib>
using namespace training_test;
namespace {
std::function<void()> uploadHook,copyHook;
void key(SDL_Keycode code,Uint32 type=SDL_KEYDOWN,Uint8 repeat=0) {
 SDL_Event e{};e.type=type;e.key.keysym.sym=code;e.key.keysym.scancode=SDL_GetScancodeFromKey(code);e.key.repeat=repeat;
 check(SDL_PushEvent(&e)==1,"scheduling key enqueue");
}
void tap(SDL_Keycode code) {key(code);key(code,SDL_KEYUP);}
// Native window coordinates; SDL's renderer filters these to logical pixels.
void click(int x,int y,Uint8 button=SDL_BUTTON_LEFT) {
 SDL_Event e{};e.type=SDL_MOUSEBUTTONDOWN;e.button.button=button;e.button.windowID=SDL_GetWindowID(SDL_GetWindowFromID(1));
 e.button.x=x*3;e.button.y=y*3;
 check(SDL_PushEvent(&e)==1,"scheduling mouse enqueue");
}
void quit(){SDL_Event e{};e.type=SDL_QUIT;SDL_PushEvent(&e);}
void preview(const IndexedFrame &frame,const char *name) {
 const auto *directory=std::getenv("MMODERN_MOUSE_PREVIEW");if(!directory)return;
 auto *surface=SDL_CreateRGBSurfaceWithFormat(0,320,200,32,SDL_PIXELFORMAT_ARGB8888);check(surface,"preview surface");
 for(unsigned y=0;y<200;++y)for(unsigned x=0;x<320;++x){const auto p=frame.pixels[y*320+x]*3;
  reinterpret_cast<Uint32 *>(static_cast<Uint8 *>(surface->pixels)+y*surface->pitch)[x]=
   SDL_MapRGB(surface->format,frame.palette[p],frame.palette[p+1],frame.palette[p+2]);}
 const auto path=std::filesystem::path(directory)/(std::string(name)+".bmp");
 const int result=SDL_SaveBMP(surface,path.string().c_str());SDL_FreeSurface(surface);check(result==0,"preview output");
}
}
extern "C" int __real_SDL_UpdateTexture(SDL_Texture *,const SDL_Rect *,const void *,int);
extern "C" int __wrap_SDL_UpdateTexture(SDL_Texture *t,const SDL_Rect *r,const void *p,int pitch){probe_fired::hit("SDL_UpdateTexture");if(uploadHook)uploadHook();return __real_SDL_UpdateTexture(t,r,p,pitch);}
extern "C" int __real_SDL_RenderCopy(SDL_Renderer *,SDL_Texture *,const SDL_Rect *,const SDL_Rect *);
extern "C" int __wrap_SDL_RenderCopy(SDL_Renderer *r,SDL_Texture *t,const SDL_Rect *s,const SDL_Rect *d){probe_fired::hit("SDL_RenderCopy");if(copyHook)copyHook();return __real_SDL_RenderCopy(r,t,s,d);}
namespace {
struct Harness {
 Inputs &in;XeenEventFlow *flow=nullptr;XeenWorld *world=nullptr;const XeenPartyState *party=nullptr;const XeenGameFlags *flags=nullptr;XeenCamera *camera=nullptr;
 std::function<void()> composeHook;
 IndexedFrame::Presentation observedOrigin;
 std::uint64_t liveCompositionMicros=0,maxLiveCompositionMicros=0;
 explicit Harness(Inputs &i):in(i){}
 XeenGameplayServices services() {
  XeenGameplayServices s{in.resources(),[]{return XeenGameFlags{};},in.mapLoader(),in.objectLoader(),[&](auto id){return in.texts.load(id);},in.font,
   [](auto &,const auto &,const auto &,auto){return XeenEventFlow::Composition{frame(),false};},{},
   [&](auto &f,const auto &){flow=&f;f.drawTrainingArt=[&](auto &b){in.assets.drawTraining(b);};
    f.drawCombatButtons=[&](auto &b){CloudsUiComposer().drawCombatButtons(in.assets,b);};},{},
   [&](auto &w,auto &,const auto &p,auto &c,const auto &g){world=&w;party=&p;camera=&c;flags=&g;}};
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
  s.validateEncounterSprite=[&](auto image){in.assets.validateNormalMonster(image);};
  s.validateCombatSprite=[&](auto image){in.assets.validateAttackMonster(image);};
  return s;
 }
 int run(XeenGameplayServices &s,const XeenSaveSnapshot &source,const std::string &name) {
  const auto path=std::filesystem::absolute("scheduling-"+name+".mmsave");
  XeenSaveFile::write(path,source);
  const auto result=Application().playGameplay(s,source.camera,path,true);
  std::filesystem::remove(path);return result;
 }
};
void movementRedraw(Inputs &in) {
 Harness h(in);auto s=h.services();auto source=in.service();source.camera={28,10,9,XeenDirection::North};
 std::uint64_t now=0;unsigned dispatches=0,loops=0;bool sent=false,injected=false;
 s.clock=[&]{return now;};
 h.composeHook=[&]{if(dispatches==1 && !injected){injected=true;tap(SDLK_UP);}};
 s.show=[&](const auto &first,const auto &handler,const auto &escape,const auto &idle,const auto &status){
  auto native=handler;
  native.withPresentedInput=[&](const auto &action,auto input,const auto &origin){
   check(handler.inputContext(origin).readyForAction && handler.acceptsInputFrame(origin) && h.flow->journeyInputCurrent(input),"movement drained before Flow authority/readiness");
   const auto before=*h.camera;const auto oldResult=h.flow->encounter()->actionResult().revision;
   ++dispatches;auto next=handler.withPresentedInput(action,input,origin);
   check(h.camera->y==before.y+1 && h.camera->x==before.x,"delivered movement silently refused by handle/frameCopy");
   check(h.flow->encounter()->actionResult().revision>oldResult,"movement lacked action publication");
   return next;
  };
  return SdlWindow().showInteractive(first,"W during movement redraw",native,escape,[&]()->std::optional<IndexedFrame>{
   check(++loops<100,"movement redraw key did not drain");now+=100;
   if(!sent){sent=true;tap(SDLK_UP);}
   if(dispatches==2){check(injected && h.camera->y==11,"movement redraw final position");quit();return {};}
   return idle();
  },status);
 };
 check(h.run(s,source,"movement-redraw")==0,"W redraw Application failed");
 std::cout<<"MOVEMENT REDRAW W=2 delivered=2 moved=2 silent-handle-refusals=0 PASS\n";
}

void wallRefusal(Inputs &in) {
 Harness h(in);auto s=h.services();auto source=in.service();unsigned dispatches=0,loops=0;std::uint64_t now=0;s.clock=[&]{return now;};
 s.show=[&](const auto &first,const auto &handler,const auto &escape,const auto &idle,const auto &status){
  auto native=handler;native.withPresentedInput=[&](const auto &action,auto input,const auto &origin){
   check(handler.inputContext(origin).readyForAction,"wall action drained before ready");
   const auto before=*h.camera;++dispatches;auto next=handler.withPresentedInput(action,input,origin);
   check(h.camera->x==before.x && h.camera->y==before.y,"wall refusal moved party");return next;
  };
  return SdlWindow().showInteractive(first,"Ready wall refusal",native,escape,[&]()->std::optional<IndexedFrame>{
   now+=100;if(++loops==1)tap(SDLK_UP);
   if(loops==8){check(dispatches==1,"wall refusal was lost or retried");quit();return {};}
   return idle();
  },status);
 };
 check(h.run(s,source,"wall")==0,"wall refusal Application failed");
 std::cout<<"READY WALL REFUSAL delivered=1 retried=0 PASS\n";
}

void contextAuthority(Inputs &in) {
 auto source=in.service();source.camera={28,10,9,XeenDirection::North};Fixture f(in,source,true);
 const auto initial=f.flow->inputContext(f.flow->frame().presentation());
 check(initial.acceptsQueuedInput && initial.readyForAction,"Quiet exploration context not ready");
 const auto old=f.flow->frame().presentation();f.flow->beginCycle(++f.cycle);
 const auto next=f.flow->handle(NavigationAction::MoveForward,f.flow->displayedInput(),old);
 auto pending=f.flow->inputContext(next.presentation());
 check(pending.contextId==initial.contextId && !pending.readyForAction,"movement changed context or bypassed handoff");
 check(!f.flow->inputContext(old).readyForAction,"retired movement origin marked ready");f.present(next);
 auto moved=f.flow->inputContext(next.presentation());check(moved.contextId==initial.contextId,"movement failed to retain context");
 const auto settleReady=[&]{for(unsigned n=0;n<100 && !f.flow->inputContext(f.flow->frame().presentation()).readyForAction;++n){f.now+=100;f.flow->beginCycle(++f.cycle);if(const auto frame=f.flow->updatePresentation())f.present(*frame);}check(f.flow->inputContext(f.flow->frame().presentation()).readyForAction,"regional work never became ready");};
 // A current, presented movement result can still own automatic actor work.
 check(!moved.readyForAction && f.flow->journeyInputCurrent(f.flow->displayedInput()),"automatic actor-work readiness window missing");
 settleReady();check(f.flow->inputContext(f.flow->frame().presentation()).contextId==initial.contextId,"actor work changed exploration context");
 f.act(InspectInventoryAction{});auto inventory=f.flow->inputContext(f.flow->frame().presentation());
 check(!inventory.acceptsQueuedInput && inventory.contextId!=moved.contextId,"inventory did not flush queue context");
 f.act(CancelInteractionAction{});auto closed=f.flow->inputContext(f.flow->frame().presentation());
 check(closed.acceptsQueuedInput && closed.contextId!=inventory.contextId,"inventory close did not change context");
 f.act(CastSpellAction{});auto casting=f.flow->inputContext(f.flow->frame().presentation());
 check(!casting.acceptsQueuedInput && casting.contextId!=closed.contextId,"casting UI accepts queue");
 f.act(CancelInteractionAction{});auto canceled=f.flow->inputContext(f.flow->frame().presentation());
 check(canceled.acceptsQueuedInput && canceled.contextId!=casting.contextId,"casting close did not flush context");
 f.act(NavigationAction::MoveForward);settleReady();f.act(InteractionAction{});auto service=f.flow->inputContext(f.flow->frame().presentation());
 check(!service.acceptsQueuedInput && service.contextId!=canceled.contextId,"Training/dialog context accepts queue");
 std::cout<<"FLOW CONTEXT movement/handoff/inventory/casting/Training PASS\n";
}
void combatQueue(Inputs &in,bool mouse=false) {
 Harness h(in);auto s=h.services();const auto source=in.base();std::uint64_t now=0,cycles=0;unsigned loops=0,dispatches=0,busyFrames=0;bool sent=false;
 s.clock=[&]{return now;};
 s.show=[&](const auto &first,const auto &handler,const auto &escape,const auto &idle,const auto &status){
  const auto present=[&](const auto &frame){handler.framePresented(frame.presentation());handler.completeInputHandoff(frame.presentation());};
  const auto act=[&](PlayerAction action){handler.beginCycle(++cycles);const auto next=handler.withPresentedInput(action,*handler.displayedInput(),h.flow->frame().presentation());if(next)present(*next);};
  const auto tick=[&]{now+=100;handler.beginCycle(++cycles);if(auto next=idle())present(*next);};
  const auto quiet=[&]{for(unsigned n=0;n<500 && !h.flow->canSave();++n)tick();check(h.flow->canSave(),"combat queue prefix Quiet bound");};
  present(first);act(NavigationAction::MoveForward);quiet();act(ShootAction{});quiet();act(NavigationAction::MoveForward);
  for(unsigned n=0;n<500;++n){
   const auto *combat=h.flow->encounter()->combat();
   if(combat && combat->phase()==XeenCombatPhase::PlayerReady){
    const auto ticket=combat->ticket();XeenRestoreGuard guard(*h.world,*h.party,*h.camera,*h.flags);
    const auto sky=h.world->scenePresentation().sky;std::string notice;
    h.flow->reportText=[&](const auto &text){notice=text;};
    for(auto movement:{NavigationAction::TurnLeft,NavigationAction::TurnRight,NavigationAction::MoveForward,NavigationAction::MoveBackward}) {
     act(movement);check(notice=="Combat movement: not supported yet"&&combat->current(ticket)&&guard.current()&&h.world->scenePresentation().sky==sky,"Combat movement notice changed state");
    }
    h.flow->reportText={};
    act(BlockAction{});if(h.flow->encounter()->combat()->phase()==XeenCombatPhase::PendingEnemy)break;
   }
   else tick();
  }
  check(h.flow->encounter()->combat() && h.flow->encounter()->combat()->phase()==XeenCombatPhase::PendingEnemy,"enemy-turn queue prefix absent");
  const auto context=handler.inputContext(h.flow->frame().presentation());check(context.acceptsQueuedInput && !context.readyForAction,"enemy turn marked ready/strict");
  auto native=handler;native.beginCycle=[&](auto){handler.beginCycle(++cycles);};
  if(mouse)native.framePresented=[&](const auto &frame){handler.framePresented(frame);preview(*frame,"combat");};
  native.withPresentedInput=[&](const auto &action,auto input,const auto &origin){
   const auto ready=handler.inputContext(origin);check(ready.contextId==context.contextId && ready.readyForAction,"A lost combat context or drained busy");
   check(std::holds_alternative<AttackAction>(action),"combat queue action changed");
   const auto generation=h.flow->encounter()->combat()->result().generation;
   if(mouse) {
    const auto ticket=h.flow->encounter()->combat()->ticket();
    // Refusal is presentation only, including in combat: no turn or RNG work.
    auto refusal=handler.withPresentedInput(UnsupportedMainScreenAction{"Quick Fight"},input,origin);
    check(refusal && h.flow->encounter()->combat()->current(ticket),"unsupported combat button consumed turn");
    present(*refusal);
    ++dispatches;auto next=handler.withPresentedInput(action,*handler.displayedInput(),h.flow->frame().presentation());
    check(h.flow->encounter()->combat()->result().generation!=generation,"ready click silently refused");return next;
   }
   ++dispatches;auto next=handler.withPresentedInput(action,input,origin);
   check(h.flow->encounter()->combat()->result().generation!=generation,"ready A silently refused");return next;
  };
  return SdlWindow().showInteractive(h.flow->frame(),"A during enemy turns",native,escape,[&]()->std::optional<IndexedFrame>{
   check(++loops<200,"enemy-turn A never drained");
   if(dispatches){check(dispatches==1 && busyFrames>=2,"A duplicated or busy witness missing");quit();return {};}
   if(!sent){sent=true;if(mouse)click(290,80);else tap(SDLK_a);}
   if(!handler.inputContext(h.flow->frame().presentation()).readyForAction)++busyFrames;
   now+=100;return idle();
  },status);
 };
 check(h.run(s,source,"combat-queue")==0,"combat queue Application failed");
 std::cout<<"COMBAT QUEUE "<<(mouse?"mouse":"A")<<"=1 attacks=1 busy-frames="<<busyFrames<<" PASS\n";
}

void originalDialogs(Inputs &in) {
 Harness h(in);auto s=h.services();auto source=in.service();source.camera={28,10,9,XeenDirection::North};
 const auto catalog=loadXeenItemCatalog(in.assets).catalog;s.catalog=&catalog;
 unsigned delivered=0,loops=0;bool queued=false;std::uint64_t now=0;s.clock=[&]{return now;};
 const auto configure=s.configureFlow;
 s.configureFlow=[&](auto &flow,const auto &camera){configure(flow,camera);flow.drawDialogSprite=[&](auto &frame,const char *name,unsigned id,int x,int y){in.assets.drawDialogSprite(frame,name,id,x,y);};};
 s.show=[&](const auto &first,const auto &handler,const auto &escape,const auto &idle,const auto &status){
  auto native=handler;
  std::function<void()> pending;
  const auto buttonDraw=native.drawButton;
  native.drawButton=[&](auto &frame,const InputButton &button){
   check(bool(buttonDraw),"original dialog pressed sprite callback absent");buttonDraw(frame,button);
   if(std::string(button.resource)=="view.icn" && button.frame==40)preview(frame,"sheet-button-pressed");
   if(std::string(button.resource)=="items.icn" && button.frame==6)preview(frame,"items-button-pressed");
  };
  native.beginCycle=[&](auto cycle){handler.beginCycle(cycle);if(pending){auto send=std::move(pending);pending={};try{send();}catch(const std::exception &e){std::cerr<<e.what()<<'\n';throw;}}};
  native.withPresentedInput=[&](const auto &action,auto input,const auto &origin){
   check(handler.inputContext(origin).readyForAction,"original dialog input while unready");
   ++delivered;queued=false;return handler.withPresentedInput(action,input,origin);
  };
  native.framePresented=[&](const auto &frame){handler.framePresented(frame);if(delivered==1)preview(*frame,"character-sheet");if(delivered==5)preview(*frame,"original-items");if(delivered==19)preview(*frame,"misc-title");};
  return SdlWindow().showInteractive(first,"Original sheet and items",native,escape,[&]()->std::optional<IndexedFrame>{
   check(++loops<200,("original dialogs timeout at input "+std::to_string(delivered)).c_str());now+=25;
   if(!queued){queued=true;pending=[&]{switch(delivered){
    case 0:tap(SDLK_F1);break;
    case 1:click(10,24);break; // Might popup.
    case 2:click(0,0);break; // Any click closes the stat popup.
    case 3:tap(SDLK_F2);break;
    case 4:click(286,12);break; // Items button.
    case 5:click(8,20);break; // Select equipped first weapon.
    case 6:click(182,109);break; // Remove.
    case 7:tap(SDLK_1);break;
    case 8:tap(SDLK_e);break; // Equip in one step.
    case 9:tap(SDLK_1);break;
    case 10:click(10,150);break; // Immediate transfer to Arturius.
    case 11:tap(SDLK_F1);break; // Switch after the selection is consumed.
    case 12:tap(SDLK_2);break;
    case 13:tap(SDLK_F2);break; // Transfer back.
    case 14:tap(SDLK_F2);break;
    case 15:tap(SDLK_1);break;
    case 16:tap(SDLK_e);break;
    case 17:tap(SDLK_F3);break;
    case 18:click(114,109);break; // Misc title for Badger.
    case 19:click(284,109);break; // Back to sheet.
    case 20:tap(SDLK_ESCAPE);break;
    default:check(!h.flow->inventoryOpen(),"original dialogs failed to close");
     check(h.flow->canSave(),"dialog close did not release save boundary");
     save_test::sameSnapshot(source,XeenSaveState::capture(in.signature,*h.party,*h.camera,*h.flags,*h.world));quit();return;
   }};}
   return idle();
  },status);
 };
 check(h.run(s,source,"original-dialogs")==0 && delivered==21,"original sheet/item native cycle");
 std::cout<<"ORIGINAL DIALOGS mouse/key sheet, popup, equip/remove and immediate transfer PASS\n";
}

void mouseJourney(Inputs &in) {
 Harness h(in);auto s=h.services();auto source=in.service();source.camera={28,10,9,XeenDirection::North};
 std::uint64_t now=0;unsigned loops=0,dispatches=0;s.clock=[&]{return now;};
 s.show=[&](const auto &first,const auto &handler,const auto &escape,const auto &idle,const auto &status){
  auto native=handler;
  std::function<void()> pending;unsigned scripted=0;
  native.beginCycle=[&](auto cycle){handler.beginCycle(cycle);if(pending){auto send=std::move(pending);pending={};send();}};
  native.framePresented=[&](const auto &frame){handler.framePresented(frame);preview(*frame,"exploration");};
  native.withPresentedInput=[&](const auto &action,auto input,const auto &origin){
   ++dispatches;check(handler.inputContext(origin).readyForAction,"mouse navigation drained while busy");
   return handler.withPresentedInput(action,input,origin);
  };
  return SdlWindow().showInteractive(first,"Main-screen mouse walk",native,escape,[&]()->std::optional<IndexedFrame>{
   check(++loops<100,"mouse walk timeout");now+=100;
   if(scripted==0){++scripted;pending=[&]{click(290,80);click(12,151);};}
   if(dispatches==2 && scripted==1){++scripted;check(h.flow->inventoryOpen(),"portrait sheet absent");pending=[&]{tap(SDLK_ESCAPE);};}
   if(dispatches==3 && scripted==2){++scripted;pending=[&]{click(261,149);click(290,149);click(100,50);};}
   if(dispatches==6){check(h.camera->x==10 && h.camera->y==10 && h.camera->direction==XeenDirection::East,"mouse walk result");quit();return {};}
   return idle();
  },status);
 };
 check(h.run(s,source,"mouse-walk")==0 && dispatches==6,"mouse Journey failed");
 std::cout<<"MOUSE WALK unsupported/portrait/forward/turn/viewport delivered exactly once PASS\n";
}

void mouseNotices(Inputs &in) {
 auto source=in.service();source.camera={28,10,9,XeenDirection::North};Fixture f(in,source,true);
 std::string reported;f.flow->reportText=[&](const auto &text){reported=text;};
 const auto before=XeenSaveFormat::encode(f.snapshot());
 for(const auto label:{"Rest","Bash","Dismiss","View Quests","Map","Info","Quick Ref","Control panel","Strafe"}) {
  const auto pixels=f.flow->frame().pixels;
  f.act(UnsupportedMainScreenAction{label});
  check(reported==std::string(label)+": not supported yet","unsupported notice missing");
  check(f.flow->frame().pixels!=pixels,"unsupported notice invisible");
  check(XeenSaveFormat::encode(f.snapshot())==before,"unsupported button changed saved gameplay/RNG/time");
 }
 // Actual Flow context classification, rather than only SDL's fake contexts.
 f.act(InspectInventoryAction{});check(f.flow->inputContext(f.flow->frame().presentation()).mainScreen==MainScreen::None,"inventory exposes main screen");
 f.act(CancelInteractionAction{});f.act(CastSpellAction{});
 check(f.flow->inputContext(f.flow->frame().presentation()).mainScreen==MainScreen::None,"casting exposes main screen");
 std::cout<<"MAIN SCREEN unsupported notices visible; saved gameplay byte-identical PASS\n";
}

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
     const auto code=n<40?(n%2?SDLK_DOWN:SDLK_UP):n<60?SDLK_LEFT:SDLK_RIGHT;
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
  const auto code=mode==0?SDLK_SPACE:mode==1?SDLK_F2:mode==2?SDLK_t:mode==3?SDLK_F3:SDLK_ESCAPE;
  const auto inject=[&]{if(armed && !injected){injected=true;tap(code);}};
  h.composeHook=[&]{if(timing==1)inject();};copyHook=[&]{if(timing==3)inject();};
  s.show=[&](const auto &first,const auto &handler,const auto &escape,const auto &idle,const auto &status){
   const auto present=[&]{handler.framePresented(h.flow->frame().presentation());handler.completeInputHandoff(h.flow->frame().presentation());};
   const auto act=[&](PlayerAction action){handler.beginCycle(++cycles);handler.withPresentedInput(action,*handler.displayedInput(),h.flow->frame().presentation());present();};
   const auto prepare=[&]{for(unsigned n=0;n<1000;++n){handler.beginCycle(++cycles);if(idle()){present();return;}}check(false,"service setup did not settle");};
   present();
   if(mode){act(InteractionAction{});prepare();check(XeenTrainingTestAccess::menu(*h.flow),"service menu setup");}
   if(mode>=2)act(SelectMemberAction{1});
   if(mode>=3){act(DialogKeyAction{'t'});prepare();}
   check(handler.inputContext(h.flow->frame().presentation()).acceptsQueuedInput==(mode==0),"service context queue acceptance");
   auto native=handler;native.beginCycle=[&](auto){handler.beginCycle(++cycles);};
   native.completeInputHandoff=[&](const auto &f){handler.completeInputHandoff(f);if(armed && f==b && timing==4)inject();};
   native.withPresentedInput=[&](const auto &action,auto epoch,const auto &origin){
    check(handler.acceptsInputFrame(origin) && (mode==0 || origin==(timing==4?b:a)),"service response origin changed");++dispatches;return handler.withPresentedInput(action,epoch,origin);
   };
   const auto ok=SdlWindow().showInteractive(h.flow->frame(),"Service cosmetic tap",native,escape,[&]()->std::optional<IndexedFrame>{
    check(++loops<15,"service cosmetic tap lost");
    if(dispatches){check(dispatches==1,"service tap duplicated");
   if(mode==0 && !XeenTrainingTestAccess::menu(*h.flow))return idle();
   if(mode==0)check(h.world->sessionState().journeyActivity()==XeenJourneyActivity::Service,"Space not accepted");
   if(mode==1)check(XeenTrainingTestAccess::selected(*h.flow)==1,"F2 selection not accepted");
   if(mode==2 && !XeenTrainingTestAccess::menu(*h.flow))return idle();
   if(mode==2)check(h.party->roster.at(18).permanentLevel==4,"T not accepted");
   if(mode==3)check(XeenTrainingTestAccess::selected(*h.flow)==2,"F3 not accepted");
   if(mode==4 && !h.flow->canSave())return idle();
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
  const auto inject=[&]{if(!armed || injected)return;injected=true;key(SDLK_RIGHT);key(SDLK_RIGHT,SDL_KEYDOWN,1);key(SDLK_RIGHT);key(SDLK_RIGHT,SDL_KEYUP);};
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
    check(handler.acceptsInputFrame(origin) && input==*handler.displayedInput(),"queued response used a stale concrete frame");++dispatches;
    return handler.withPresentedInput(action,input,origin);
   };
   return SdlWindow().showInteractive(first,"Deterministic cosmetic boundary",native,escape,[&]()->std::optional<IndexedFrame>{
    check(++loops<20,"deterministic tap lost");
    if(dispatches==2){check(h.camera->direction==XeenDirection::South,"physical edge/repeat did not turn exactly twice");quit();return {};}
    check(dispatches<=2,"duplicate boundary action");
    if(!armed){armed=true;a=first.presentation();epoch=handler.displayedInput();ticket=h.flow->encounter()->ticket();if(timing==0){inject();return {};}}
    now+=100;return idle();
   },status);
  };
  check(h.run(s,source,"boundary")==0,"boundary Application failed");
  if(timing==6)check(injected && dispatches==0,"key overtook queued close during cosmetic acquisition");
  uploadHook={};copyHook={};std::cout<<(timing==6?"CLOSE ORDER ":"BOUNDARY ")<<timing<<" tap=1 dispatch="<<dispatches<<" movement-repeat=1 held-duplicate=0 PASS\n";
 }
}
}
int main(int argc,char **argv){probe_fired::expect("SDL_UpdateTexture");probe_fired::expect("SDL_RenderCopy");try{
 check(argc==2 || argc==3,"usage: input-scheduling <installation> [mouse]");SDL_setenv("SDL_VIDEODRIVER","dummy",1);SDL_setenv("SDL_RENDER_DRIVER","software",1);
 const auto installation=XeenInstallationDetector().detect(argv[1]);check(bool(installation),"original installation absent");Inputs in(*installation);
 if(argc==3){mouseNotices(in);combatQueue(in,true);originalDialogs(in);mouseJourney(in);return 0;}
 contextAuthority(in);wallRefusal(in);combatQueue(in);movementRedraw(in);boundaries(in);services(in);stress(in);return 0;
}catch(const std::exception &e){uploadHook={};copyHook={};std::cerr<<e.what()<<'\n';return 1;}}
