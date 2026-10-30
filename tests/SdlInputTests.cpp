#include "SdlMouseTestSupport.h"
#include "XeenProbeFired.h"
#include "core/PresentationClock.h"
#include "platform/sdl/SdlWindow.h"
#include "platform/sdl/XeenMainScreenInput.h"
#include "games/xeen/XeenDialogView.h"

#define SDL_MAIN_HANDLED
#include <SDL.h>

#include <atomic>
#include <chrono>
#include <filesystem>
#include <iostream>
#include <stdexcept>
#include <thread>
#include <variant>

using namespace mmodern;

// Deterministic millisecond collision at the production SDL boundary.
static bool fixedTicks=false;
extern "C" Uint32 __real_SDL_GetTicks();
extern "C" Uint32 __wrap_SDL_GetTicks(){if(fixedTicks)probe_fired::hit("SDL_GetTicks");return fixedTicks?100:__real_SDL_GetTicks();}
static bool recordFeedback=false;
static std::uint32_t nativePixel=0;
static std::vector<unsigned> nativePresentSamples,feedbackDelays;
extern "C" void __real_SDL_Delay(Uint32);
extern "C" void __wrap_SDL_Delay(Uint32 ms){if(recordFeedback)feedbackDelays.push_back(ms);else __real_SDL_Delay(ms);}
extern "C" int __real_SDL_UpdateTexture(SDL_Texture *,const SDL_Rect *,const void *,int);
extern "C" int __wrap_SDL_UpdateTexture(SDL_Texture *texture,const SDL_Rect *rect,const void *pixels,int pitch){
 if(recordFeedback)nativePixel=*static_cast<const std::uint32_t *>(pixels);
 return __real_SDL_UpdateTexture(texture,rect,pixels,pitch);
}
extern "C" void __real_SDL_RenderPresent(SDL_Renderer *);
extern "C" void __wrap_SDL_RenderPresent(SDL_Renderer *renderer){
 if(recordFeedback)nativePresentSamples.push_back(nativePixel&255);
 __real_SDL_RenderPresent(renderer);
}

namespace {

void cutsceneControlQuit() {
 for(unsigned mode=0;mode<4;++mode) { // Intro, ending, DAT title, DAT gameplay.
 const bool cutscene=mode<2;
 IndexedFrame frame{1,1,{0}};unsigned cycles=0,delivered=0;bool closed=false;
 SdlWindow::FrameUpdateHandler h=[&](const PlayerAction &a)->std::optional<IndexedFrame>{
  const auto key=std::get_if<DialogKeyAction>(&a);const auto event=std::get_if<ExplorationKeyAction>(&a);
  if((!key || key->key!=17) && (!event || event->key!=17))throw std::runtime_error("DAT Ctrl-Q no longer retains its control code");
  ++delivered;return {};
 };
 h.protectAllKeys=true;h.ignoreMouse=true;h.closed=[&]{closed=true;};
 h.cutsceneControlQuit=cutscene;
 h.inputContext=[mode](const auto &){InputContext c;c.readyForAction=true;
  if(mode!=3){auto d=std::make_shared<DialogInput>();d->anyKey=true;d->retainKey=true;c.dialog=d;}
  else {c.mainScreen=MainScreen::Exploration;c.retainEventKeys=true;}return c;};
 const auto ok=SdlWindow().showInteractive(frame,"Scoped EXE Ctrl-Q",h,{},[&]()->std::optional<IndexedFrame>{
  ++cycles;SDL_Event event{};
  if(cycles==1){event.type=SDL_KEYDOWN;event.key.keysym.sym=SDLK_q;event.key.keysym.mod=KMOD_CTRL;}
  else {if(cutscene || cycles>2)throw std::runtime_error("Ctrl-Q did not quit the EXE presentation");event.type=SDL_QUIT;}
  SDL_PushEvent(&event);return {};
 });
 if(!ok || !closed || (cutscene && (cycles!=1 || delivered)) || (!cutscene && (cycles!=2 || delivered!=1)))throw std::runtime_error("Ctrl-Q escaped its EXE presentation scope");
 }
}
void semanticBoundaryKeys() {
 IndexedFrame frame;frame.width=frame.height=1;frame.pixels={0};
 unsigned accepted=0,stage=0;std::uint64_t epoch=1;bool queuedOld=false;
 const auto key=[](SDL_Keycode code,Uint32 type=SDL_KEYDOWN,Uint32 stamp=100,Uint8 repeat=0){
  SDL_Event e{};e.type=type;e.key.keysym.sym=code;e.key.timestamp=stamp;e.key.repeat=repeat;
  if(SDL_PeepEvents(&e,1,SDL_ADDEVENT,0,0)!=1)throw std::runtime_error("boundary key queue");
 };
 const auto check=[&](bool v){if(!v)throw std::runtime_error("SDL equal-tick boundary stage="+std::to_string(stage)+" accepted="+std::to_string(accepted));};
 SdlWindow::FrameUpdateHandler handler=[](const PlayerAction &)->std::optional<IndexedFrame>{throw std::runtime_error("unversioned protected dispatch");};
 handler.protectAllKeys=true;handler.displayedInput=[&]{return std::optional<std::uint64_t>{epoch};};
 handler.withDisplayedInput=[&](const PlayerAction &,std::uint64_t supplied)->std::optional<IndexedFrame>{if(supplied!=epoch)return {}; ++accepted;++epoch;return frame;};
 handler.framePresented=[&](const auto &){if(epoch==2&&!queuedOld){queuedOld=true;key(SDLK_UP);key(SDLK_UP,SDL_KEYUP);}};
 fixedTicks=true;
 const bool ok=SdlWindow().showInteractive(frame,"SDL semantic boundary",handler,{},[&]()->std::optional<IndexedFrame>{
  switch(stage++){
  case 0:key(SDLK_RIGHT);key(SDLK_RIGHT,SDL_KEYUP);break;
  case 1:check(accepted==1);break; // First command still awaits presentation.
  case 2:check(accepted==1);key(SDLK_RIGHT);key(SDLK_LEFT);key(SDLK_LEFT,SDL_KEYUP);break;
  case 3:check(accepted==2);break;
  case 4:key(SDLK_RIGHT);key(SDLK_RIGHT,SDL_KEYDOWN,100,1);break;
  case 5:check(accepted==2);key(SDLK_RIGHT,SDL_KEYUP);break;
  case 6:key(SDLK_UP,SDL_KEYDOWN,99);key(SDLK_UP,SDL_KEYUP,99);key(SDLK_RIGHT);key(SDLK_RIGHT,SDL_KEYUP);break;
  default:check(accepted==3);{SDL_Event e{};e.type=SDL_QUIT;SDL_PushEvent(&e);}break;
  }
  return {};
 });
 fixedTicks=false;check(ok&&accepted==3&&queuedOld);
}

// Fake readiness/clock, real production event queue and presentation loop.
struct QueueHarness {
 MainScreen screen=MainScreen::None;
 std::shared_ptr<const DialogInput> dialog;
 bool feedbackEnabled=false;std::vector<unsigned> pressedFrames;
 bool redraw=false,ready=false,queueable=true,restAvailable=false,navigationDrawPending=false,actionDrawPending=false;std::uint64_t context=1,epoch=1,presented=0;
 unsigned stage=0,presentations=0,lastDelivery=0;std::vector<char> delivered;
 std::uint64_t clock=0,clockStep=100000000,drawDeadline=0;
 bool drawPaced=false;std::vector<std::uint64_t> deliveredAt;
 std::function<void()> duringPresentation;
 std::function<void(const PlayerAction &)> observeAction;
 bool retainEventKeys=false;std::uint64_t keyboardClear=0;
 static void key(SDL_Keycode code,Uint32 type=SDL_KEYDOWN,Uint8 repeat=0,Uint32 stamp=99,Uint16 modifiers=0){
  SDL_Event event{};event.type=type;event.key.keysym.sym=code;event.key.keysym.mod=modifiers;event.key.repeat=repeat;event.key.timestamp=stamp;
  if(SDL_PeepEvents(&event,1,SDL_ADDEVENT,0,0)!=1)throw std::runtime_error("queue fixture key");
 }
 static void tap(SDL_Keycode code,Uint32 stamp=99){key(code,SDL_KEYDOWN,0,stamp);key(code,SDL_KEYUP,0,stamp);}
 static PictureAspect mouseAspect;
 static void click(int x,int y,Uint8 button=SDL_BUTTON_LEFT,Uint32 stamp=99){
  SDL_Event event{};event.type=SDL_MOUSEBUTTONDOWN;event.button.button=button;
  sdl_mouse_test::position(event.button,x,y,mouseAspect);event.button.timestamp=stamp;
  if(SDL_PushEvent(&event)!=1)throw std::runtime_error("queue fixture native click");
  event.type=SDL_MOUSEBUTTONUP;
  sdl_mouse_test::position(event.button,x,y,mouseAspect);
  if(SDL_PushEvent(&event)!=1)throw std::runtime_error("queue fixture native release");
 }
 static void quit(){SDL_Event event{};event.type=SDL_QUIT;SDL_PushEvent(&event);}
 void check(bool value,const char *message){if(!value)throw std::runtime_error(std::string(message)+" stage="+std::to_string(stage)+" actions="+std::string(delivered.begin(),delivered.end()));}
 void run(const char *name,const std::function<void(QueueHarness &)> &script){
  IndexedFrame frame;frame.width=320;frame.height=200;frame.pixels.resize(64000);
  for(unsigned i=0;i<256;++i)frame.palette[i*3+2]=i;
  SdlWindow::FrameUpdateHandler handler=[](const auto &)->std::optional<IndexedFrame>{throw std::runtime_error("unversioned queued action");};
  handler.protectAllKeys=true;handler.displayedInput=[&]{return std::optional<std::uint64_t>{epoch};};
  handler.presentationClock=[&]{return clock;};
  handler.beginCycle=[&](auto){clock+=clockStep;};
  handler.inputContext=[&](const auto &){return InputContext{context,queueable,ready && clock>=drawDeadline && presented==epoch,screen,dialog,false,restAvailable && presented==epoch,false,navigationDrawPending,keyboardClear,retainEventKeys,6,0,actionDrawPending};};
  handler.framePresented=[&](const auto &){presented=epoch;++presentations;const auto hook=duringPresentation;if(hook)hook();};
  if(feedbackEnabled)handler.drawButton=[&](auto &pressed,const InputButton &button){
   pressedFrames.push_back(button.pressedFrame());pressed.pixels[0]=button.pressedFrame();
  };
  handler.withPresentedInput=[&](const PlayerAction &action,auto token,const auto &)->std::optional<IndexedFrame>{
   if(token!=epoch){check(!queueable,"queued key reached a stale frame");return {};}
   const bool immediate=std::holds_alternative<SaveGameAction>(action) || std::holds_alternative<CancelInteractionAction>(action);
   check(!queueable || immediate || ((ready || (restAvailable && std::holds_alternative<RestAction>(action))) && lastDelivery!=presentations),"drained twice or while busy");lastDelivery=presentations;
   if(feedbackEnabled)check(!nativePresentSamples.empty() && nativePresentSamples.back()==0,"button not restored before action dispatch");
   char kind='?';if(const auto *nav=std::get_if<NavigationAction>(&action))kind=*nav==NavigationAction::MoveForward?'W':*nav==NavigationAction::MoveBackward?'S':*nav==NavigationAction::TurnLeft?'A':*nav==NavigationAction::TurnRight?'D':*nav==NavigationAction::StrafeLeft?'L':'T';
   else if(std::holds_alternative<InteractionAction>(action))kind=' ';
   else if(std::holds_alternative<AttackAction>(action))kind='X';
   else if(std::holds_alternative<BlockAction>(action))kind='B';
   else if(std::holds_alternative<ShootAction>(action))kind='F';
   else if(std::holds_alternative<RevisitCompletedAction>(action))kind='R';
   else if(std::holds_alternative<RestAction>(action))kind='R';
   else if(std::holds_alternative<SaveGameAction>(action))kind='9';
   else if(std::holds_alternative<CancelInteractionAction>(action))kind='E';
   else if(std::holds_alternative<QuickFightAction>(action))kind='F';
   else if(std::holds_alternative<QuickFightOptionsAction>(action))kind='O';
   else if(std::holds_alternative<UseItemAction>(action))kind='u';
   else if(std::holds_alternative<CastSpellAction>(action))kind='C';
   else if(std::holds_alternative<UnsupportedMainScreenAction>(action))kind='U';
   else if(std::holds_alternative<ControlPanelAction>(action))kind='P';
   else if(std::holds_alternative<InfoAction>(action))kind='I';
   else if(std::holds_alternative<WaitAction>(action))kind='.';
   else if(std::holds_alternative<DismissAction>(action))kind='D';
   else if(std::holds_alternative<QuickReferenceAction>(action))kind='Q';
   else if(std::holds_alternative<MapAction>(action))kind='M';
   else if(std::holds_alternative<QuestsAction>(action))kind='V';
   else if(std::holds_alternative<ToggleAutomapAction>(action))kind='=';
   else if(std::holds_alternative<AcknowledgeAction>(action))kind='K';
   else if(std::holds_alternative<TextInputAction>(action))kind='t';
   else if(std::holds_alternative<SelectMemberAction>(action))kind='P';
   else if(const auto *key=std::get_if<DialogKeyAction>(&action))kind=key->key==InputKey::Escape?'E':char(key->key);
   check(!drawPaced || clock>=drawDeadline,"action bypassed draw cadence");
   if(observeAction)observeAction(action);
   delivered.push_back(kind);deliveredAt.push_back(clock);
   if(drawPaced)drawDeadline=PresentationClock::after(clock,2);
   ++epoch;return frame;
  };
  fixedTicks=true;
  const bool ok=SdlWindow().showInteractive(frame,name,handler,[]{return true;},[&]()->std::optional<IndexedFrame>{
   check(stage<60,"queue test timeout");redraw=false;script(*this);++stage;return redraw?std::optional<IndexedFrame>{frame}:std::nullopt;
  });
  fixedTicks=false;check(ok,"queue show failed");std::cout<<name<<" passed\n";
 }
};
PictureAspect QueueHarness::mouseAspect = PictureAspect::FourThree;
// Part 0's physical-key table exercised through the production SDL loop.
void dosKeyTable() {
 struct Case {SDL_Keycode key;Uint16 mod;std::optional<PlayerAction> expected;};
 const std::vector<Case> cases{
  {SDLK_w,0,WaitAction{}},{SDLK_PERIOD,0,ExplorationKeyAction{'.'}},
  {SDLK_KP_4,0,NavigationAction::TurnLeft},{SDLK_KP_6,KMOD_NUM,NavigationAction::TurnRight},
  {SDLK_LEFT,KMOD_CTRL,NavigationAction::StrafeLeft},{SDLK_RIGHT,KMOD_CTRL,NavigationAction::StrafeRight},
  {SDLK_UP,KMOD_CTRL,ExplorationKeyAction{0xb2}},{SDLK_DOWN,KMOD_CTRL,ExplorationKeyAction{0xb3}},
  {SDLK_i,KMOD_CTRL,ControlPanelAction{}},{SDLK_i,Uint16(KMOD_CTRL|KMOD_SHIFT),ControlPanelAction{}},
  {SDLK_BACKQUOTE,KMOD_CTRL,InteractionAction{}},{SDLK_w,KMOD_ALT,WaitAction{}},
  {SDLK_F7,0,ExplorationKeyAction{InputKey::F1+6}},{SDLK_F8,0,ExplorationKeyAction{InputKey::F1+7}},
  {SDLK_F9,0,SaveGameAction{}},{SDLK_F10,0,{}},{SDLK_F10,KMOD_SHIFT,{}},
  {SDLK_F10,KMOD_ALT,{}},{SDLK_F10,KMOD_CTRL,ExplorationKeyAction{0x92}},
  {SDLK_F11,0,{}},{SDLK_F12,0,{}},{SDLK_DELETE,0,{}},{SDLK_KP_PERIOD,0,{}},
  {SDLK_u,0,ExplorationKeyAction{'u'}},{SDLK_1,0,ExplorationKeyAction{'1'}},
  {SDLK_RETURN,0,ExplorationKeyAction{13}},{SDLK_BACKSPACE,0,ExplorationKeyAction{8}},
  {SDLK_EQUALS,KMOD_SHIFT,ExplorationKeyAction{'+'}},{SDLK_MINUS,KMOD_SHIFT,ExplorationKeyAction{'-'}},
  {SDLK_INSERT,0,ExplorationKeyAction{0xe0}},{SDLK_HOME,0,ExplorationKeyAction{InputKey::Home}},
  {SDLK_END,0,ExplorationKeyAction{InputKey::End}},{SDLK_PAGEUP,0,ExplorationKeyAction{InputKey::PageUp}},
  {SDLK_PAGEDOWN,0,ExplorationKeyAction{InputKey::PageDown}},
  {SDLK_CAPSLOCK,0,ExplorationKeyAction{0xfc}},{SDLK_SCROLLLOCK,0,ExplorationKeyAction{0xfa}},
  {SDLK_LSHIFT,0,{}},{SDLK_LCTRL,0,{}},{SDLK_LALT,0,{}},{SDLK_RGUI,0,{}},
  {SDLK_LEFTBRACKET,KMOD_CTRL,CancelInteractionAction{}}
 };
 for(std::size_t begin=0;begin<cases.size();begin+=20) {
  QueueHarness h;h.ready=true;h.screen=MainScreen::Exploration;
  std::size_t current=begin,accepted=0;
  h.observeAction=[&](const PlayerAction &actual) {
   const auto &expected=cases[current].expected;
   h.check(expected && expected->index()==actual.index(),("DOS key type at case "+std::to_string(current)).c_str());
   if(const auto *nav=std::get_if<NavigationAction>(&actual))h.check(*nav==std::get<NavigationAction>(*expected),"DOS navigation translation");
   if(const auto *key=std::get_if<ExplorationKeyAction>(&actual))h.check(key->key==std::get<ExplorationKeyAction>(*expected).key,"DOS unmatched code");
   ++accepted;
  };
  h.run("DOS physical translation",[&](auto &h) {
   if(h.stage%2==0) {
    current=begin+h.stage/2;
    if(current>=cases.size() || current>=begin+20){h.quit();return;}
    const auto &c=cases[current];h.key(c.key,SDL_KEYDOWN,0,100,c.mod);h.key(c.key,SDL_KEYUP,0,100,c.mod);accepted=0;
   } else h.check(accepted==unsigned(bool(cases[current].expected)),("DOS pass/no-code at case "+std::to_string(current)).c_str());
  });
 }
 QueueHarness locks;locks.ready=true;locks.screen=MainScreen::Exploration;
 locks.run("NumLock make has a code; its release clears pending makes",[](auto &h){
  if(h.stage==0)h.key(SDLK_NUMLOCKCLEAR,SDL_KEYDOWN,0,100);
  else if(h.stage==1){h.check(h.delivered==std::vector<char>{'?'},"NumLock make missing");h.ready=false;h.tap(SDLK_PERIOD);h.key(SDLK_NUMLOCKCLEAR,SDL_KEYUP);}
  else if(h.stage==2)h.ready=true;
  else if(h.stage==4){h.check(h.delivered.size()==1,"NumLock release made a pass or retained pending makes");h.quit();}
 });
}
void dosRepeatsAndClears() {
 for(const auto key:{SDLK_w,SDLK_PERIOD}) {
  QueueHarness h;h.screen=MainScreen::Exploration;h.ready=true;h.clockStep=0;
  h.run("DOS mapped and unmatched typematic",[key](auto &h){
   if(h.stage==0)h.key(key,SDL_KEYDOWN,0,100);
   else if(h.stage==1){h.check(h.delivered.size()==1,"first make missing");h.clock=499999999;}
   else if(h.stage==2){h.check(h.delivered.size()==1,"repeat before 500ms");h.clock=500000000;}
   else if(h.stage==3){h.check(h.delivered.size()==2,"held non-navigation key did not repeat");h.clock+=1000000000;}
   else if(h.stage==4){h.check(h.delivered.size()==3,"late repeat caught up or missing");h.key(key,SDL_KEYUP,0,100);}
   else if(h.stage==5){h.clock+=1000000000;}
   else if(h.stage==6){h.check(h.delivered.size()==3,"release generated a pass");h.quit();}
  });
 }
 for(bool clear:{false,true}) {
  QueueHarness h;h.screen=MainScreen::Exploration;h.retainEventKeys=true;
  h.run(clear?"DOS explicit clear drops queued makes":"DOS non-modal Event preserves queued taps",[clear](auto &h){
   if(h.stage==0){h.tap(SDLK_PERIOD);h.tap(SDLK_w);}
   else if(h.stage==1){++h.context;if(clear)++h.keyboardClear;h.redraw=true;}
   else if(h.stage==2){++h.context;h.ready=true;h.redraw=true;}
   else if(h.stage==6){h.check(h.delivered==(clear?std::vector<char>{}:std::vector<char>{'?','.'}),"DOS queued-script/clear-site rule");h.quit();}
  });
 }
 // Host events and modifiers never enter the gameplay queue.
 QueueHarness host;host.ready=true;host.screen=MainScreen::Exploration;
 host.run("host events have no DOS pass",[](auto &h){
  if(h.stage==0){h.key(SDLK_RETURN,SDL_KEYDOWN,0,100,KMOD_ALT);h.key(SDLK_RETURN,SDL_KEYUP);
   SDL_Event e{};e.type=SDL_WINDOWEVENT;e.window.event=SDL_WINDOWEVENT_SIZE_CHANGED;SDL_PushEvent(&e);}
  else if(h.stage==2){h.check(h.delivered.empty(),"host action made a pass");h.quit();}
 });
}
void retainedEventRegressions() {
 for(const auto code:{SDLK_ESCAPE,SDLK_F9}) {
  QueueHarness h;h.screen=MainScreen::Exploration;h.retainEventKeys=true;h.clockStep=0;
  h.run("Escape/F9 retain makes and repeat",[code](auto &h){
   if(h.stage==0)h.key(code,SDL_KEYDOWN,0,100);
   else if(h.stage==1){h.check(h.delivered.empty(),"Escape/F9 bypassed Event readiness");h.ready=true;}
   else if(h.stage==2){h.check(h.delivered.size()==1,"Escape/F9 make lost");h.clock=500000000;}
   else if(h.stage==3){h.check(h.delivered.size()==2,"Escape/F9 failed to repeat");h.key(code,SDL_KEYUP,0,100);}
   else if(h.stage==5){h.check(h.delivered.size()==2,"Escape/F9 release made a pass");h.quit();}
  });
 }
 QueueHarness held;held.screen=MainScreen::Exploration;held.retainEventKeys=true;held.ready=true;held.clockStep=0;
 held.run("cleared native makes retain physical hold and future repeat",[](auto &h){
  if(h.stage==0){h.key(SDLK_PERIOD);h.redraw=true;h.duringPresentation=[&h]{h.duringPresentation={};++h.keyboardClear;};}
  else if(h.stage==1){h.check(h.delivered.empty(),"cleared native make delivered");h.clock=499999999;}
  else if(h.stage==2){h.check(h.delivered.empty(),"cleared physical hold repeated early");h.clock=500000000;}
  else if(h.stage==3){h.check(h.delivered==std::vector<char>{'?'},"cleared hold lost its future repeat");h.key(SDLK_PERIOD,SDL_KEYUP);}
  else if(h.stage==5){h.check(h.delivered.size()==1,"release replayed a cleared make");h.quit();}
 });
 QueueHarness mouse;mouse.screen=MainScreen::Exploration;mouse.retainEventKeys=true;
 mouse.run("NumLock keyboard clear preserves mouse",[](auto &h){
  if(h.stage==0){h.click(235,75);h.key(SDLK_NUMLOCKCLEAR);h.key(SDLK_NUMLOCKCLEAR,SDL_KEYUP);}
  else if(h.stage==1)h.ready=true;
  else if(h.stage==4){h.check(h.delivered==std::vector<char>{'F'},"NumLock cleared a mouse click");h.quit();}
 });
 QueueHarness afterLock;afterLock.screen=MainScreen::Exploration;afterLock.retainEventKeys=true;afterLock.ready=true;
 afterLock.run("NumLock release precedes a later make in the same native batch",[](auto &h){
  if(h.stage==0){h.key(SDLK_NUMLOCKCLEAR,SDL_KEYUP);h.tap(SDLK_w);}
  else if(h.stage==3){h.check(h.delivered==std::vector<char>{'.'},"NumLock release discarded the later W tap");h.quit();}
 });
 for(bool textPrompt:{false,true}) {
  QueueHarness h;h.retainEventKeys=true;h.queueable=false;h.ready=true;h.clockStep=0;
  DialogInput input;input.textEntry=textPrompt;input.anyKey=!textPrompt;input.keys={8,13,27};
  h.dialog=std::make_shared<const DialogInput>(input);
  h.run("modifier release preserves held modal text/unmatched repeats",[textPrompt](auto &h){
   if(h.stage==0)h.key(SDLK_z,SDL_KEYDOWN,0,100,KMOD_SHIFT);
   else if(h.stage==1){h.check(h.delivered.size()==1,"modal initial make lost");h.key(SDLK_LSHIFT,SDL_KEYUP,0,100,KMOD_SHIFT);h.clock=500000000;}
   else if(h.stage==2){h.check(h.delivered==std::vector<char>(2,textPrompt?'t':'K'),"modifier release erased the held modal press");h.key(SDLK_z,SDL_KEYUP);}
   else if(h.stage==4){h.check(h.delivered.size()==2,"modal key release made another input");h.quit();}
  });
 }
 QueueHarness fifo;fifo.retainEventKeys=true;fifo.queueable=false;
 DialogInput wait;wait.anyKey=true;fifo.dialog=std::make_shared<const DialogInput>(wait);
 fifo.run("modal Event uses the ten-key FIFO",[](auto &h){
  if(h.stage==0)for(unsigned n=0;n<11;++n)h.tap(SDLK_z);
  else if(h.stage==1)h.ready=true;
  else if(h.stage==14){h.check(h.delivered==std::vector<char>(10,'K'),"modal FIFO must retain ten and drop the eleventh make");h.quit();}
 });
 QueueHarness text;text.screen=MainScreen::Exploration;text.retainEventKeys=true;
 text.run("retained letters enter an Event text prompt",[](auto &h){
  if(h.stage==0){h.tap(SDLK_a);h.tap(SDLK_b);h.tap(SDLK_RETURN);}
  else if(h.stage==1){DialogInput input;input.textEntry=true;input.keys={8,13,27};
   h.dialog=std::make_shared<const DialogInput>(input);h.screen=MainScreen::None;h.queueable=false;++h.context;h.ready=true;h.redraw=true;}
  else if(h.stage==7){h.check(h.delivered==std::vector<char>{'t','t',13},"buffered text or modal completion was lost");h.quit();}
 });
}
void eitherMouseButton() {
 struct Target {MainScreen screen;int x,y;std::shared_ptr<const DialogInput> dialog;};
 DialogInput button;button.keys={'y',InputKey::Escape};button.hits={{10,10,30,30,'y'}};
 DialogInput text;text.textEntry=true;text.keys={8,13,27,127};
 const std::vector<Target> targets{{MainScreen::Exploration,235,75,{}},{MainScreen::Exploration,10,150,{}},
  {MainScreen::Exploration,8,8,{}},{MainScreen::Combat,239,27,{}},
  {MainScreen::None,10,10,std::make_shared<const DialogInput>(button)},
  {MainScreen::None,10,10,std::make_shared<const DialogInput>(text)}};
 for(const auto &t:targets) {
  QueueHarness h;h.ready=true;h.screen=t.screen;h.dialog=t.dialog;h.queueable=!bool(t.dialog);
  std::vector<std::size_t> actions;h.observeAction=[&](const auto &a){actions.push_back(a.index());};
  h.run("either mouse button follows coordinates",[&](auto &h){
   if(h.stage==0)h.click(t.x,t.y,SDL_BUTTON_LEFT,100);
   else if(h.stage==2)h.click(t.x,t.y,SDL_BUTTON_RIGHT,100);
   else if(h.stage==4){h.check(t.dialog && t.dialog->textEntry?actions.empty():actions.size()==2 && actions[0]==actions[1],"DOS right/left parity");
    if(t.dialog && t.dialog->textEntry){h.tap(SDLK_DELETE,100);h.tap(SDLK_F11,100);h.tap(SDLK_F12,100);} }
   else if(h.stage==6){if(t.dialog && t.dialog->textEntry)h.check(actions.empty(),"no-code key reached text input");h.quit();}
  });
 }
}
void wordInputBoundaries() {
 QueueHarness h;h.queueable=false;h.ready=true;h.screen=MainScreen::None;
 DialogInput input;input.textEntry=true;input.keys={8,13,27};h.dialog=std::make_shared<const DialogInput>(input);
 h.run("word-input-context-and-queue-boundaries",[](auto &h) {
  const auto type=[](Uint32 stamp) {SDL_Event event{};event.type=SDL_TEXTINPUT;event.text.timestamp=stamp;
   event.text.text[0]='w';event.text.text[1]=0;SDL_PeepEvents(&event,1,SDL_ADDEVENT,0,0);};
  if(h.stage==0)type(99); // Older than the acquired prompt.
  else if(h.stage==1) {h.check(h.delivered.empty(),"stale text entered prompt");type(100);h.tap(SDLK_UP,100);h.tap(SDLK_b,100);}
  else if(h.stage==2) {h.check(h.delivered==std::vector<char>{'t'},"typed letters leaked to gameplay");h.click(60,170,SDL_BUTTON_LEFT,100);}
  else if(h.stage==3) {h.check(h.delivered==std::vector<char>{'t'},"left click submitted text");h.click(60,170,SDL_BUTTON_RIGHT,100);}
  else if(h.stage==4) {h.check(h.delivered==std::vector<char>{'t'},"Part 0: right click must leave text unchanged");
   h.dialog.reset();h.queueable=true;h.screen=MainScreen::Exploration;++h.context;type(100);h.redraw=true;}
  else if(h.stage==5) {h.check(h.delivered==std::vector<char>{'t'},"retired prompt text leaked into exploration");h.tap(SDLK_UP,100);}
  else if(h.stage==7) {h.check(h.delivered==std::vector<char>{'t','W'},"exploration did not regain input");h.quit();}
 });
}
void inputWaitCompletesBeforeAnotherDraw() {
 for(const auto [code,expected]:std::vector<std::pair<SDL_Keycode,char>>{
     {SDLK_UP,'W'},{SDLK_w,'.'},{SDLK_PERIOD,'?'},{SDLK_F9,'9'},{SDLK_ESCAPE,'E'}})
 for(bool held:{false,true}) {
  QueueHarness h;h.screen=MainScreen::Exploration;h.retainEventKeys=true;
  h.navigationDrawPending=code==SDLK_UP;h.actionDrawPending=code!=SDLK_UP;
  h.duringPresentation=[&] {if(h.presentations==3){h.ready=true;h.navigationDrawPending=h.actionDrawPending=false;}};
  h.run("input-draw-boundary-poll",[held,code,expected](auto &h){
   if(h.stage==0){if(held)h.key(code);else h.tap(code);}
   else {
    h.check(h.delivered==std::vector<char>{expected},"another semantic idle draw ran while input awaited only draw completion");
    h.key(code,SDL_KEYUP);h.quit();
   }
  });
 }
}
void pressedButtonFeedback(){
 const auto verify=[](const QueueHarness &h,const std::vector<unsigned> &expected){
  if(h.pressedFrames!=expected || !feedbackDelays.empty())
   throw std::runtime_error("pressed sprite/immediate restoration mismatch");
  std::vector<unsigned> seen;
  for(std::size_t i=0;i<nativePresentSamples.size();++i)if(nativePresentSamples[i]){
   seen.push_back(nativePresentSamples[i]);
   if(i+1==nativePresentSamples.size() || nativePresentSamples[i+1]!=0)throw std::runtime_error("native pressed pixels not restored");
  }
  if(seen!=expected || h.presentations+2*expected.size()!=nativePresentSamples.size())
   throw std::runtime_error("feedback acquired a semantic frame or failed native display");
 };
 QueueHarness main;main.screen=MainScreen::Exploration;main.ready=true;main.feedbackEnabled=true;
 recordFeedback=true;nativePresentSamples.clear();feedbackDelays.clear();
 main.run("main-key-mouse-pressed/restore/FIFO-unchanged",[](auto &h){
  if(h.stage==0)h.tap(SDLK_UP,100);
  else if(h.stage==1)h.click(235,75,SDL_BUTTON_LEFT,100);
  else if(h.stage==2)h.click(8,8,SDL_BUTTON_LEFT,100); // Viewport Space has no sprite.
  else if(h.stage==3)h.tap(SDLK_q,100);
  else if(h.stage==4){h.check(h.delivered==std::vector<char>{'W','F',' ','Q'},"Quick Reference shortcut missing");h.click(235,169,SDL_BUTTON_LEFT,100);}
  else if(h.stage==5)h.tap(SDLK_i,100);
  else if(h.stage==6){h.check(h.delivered==std::vector<char>{'W','F',' ','Q','L','I'},"feedback changed command trace");h.quit();}
 });
 recordFeedback=false;verify(main,{23,1,17,27,15});
 QueueHarness dialog;dialog.queueable=false;dialog.ready=true;dialog.feedbackEnabled=true;
 DialogInput input;input.hits.push_back({10,10,30,30,'e',InputButton{"items.icn",0,10,10}});input.keys={'e'};
 dialog.dialog=std::make_shared<const DialogInput>(std::move(input));
 recordFeedback=true;nativePresentSamples.clear();feedbackDelays.clear();
 dialog.run("dialog-key-mouse-pressed/ignored-input-no-feedback",[](auto &h){
  if(h.stage==0)h.tap(SDLK_e,100);
  else if(h.stage==1)h.check(h.delivered==std::vector<char>{'e'},"keyboard feedback action"); // Successor awaits acquisition.
  else if(h.stage==2)h.click(10,10,SDL_BUTTON_LEFT,100);
  else if(h.stage==3)h.check(h.delivered==std::vector<char>{'e','e'},"mouse feedback action");
  else if(h.stage==4){h.tap(SDLK_e,99);h.key(SDLK_e,SDL_KEYDOWN,1,100);h.click(10,10,SDL_BUTTON_RIGHT,100);h.click(30,10,SDL_BUTTON_LEFT,100);}
  else if(h.stage==5){h.check(h.delivered==std::vector<char>{'e','e','e'},"Part 0: right press selects the same dialog button");h.quit();}
 });
 recordFeedback=false;verify(dialog,{1,1,1});
 for(auto screen:{MainScreen::Exploration,MainScreen::Combat}){
  for(unsigned index=0;index<16;++index){
   constexpr int columns[]{235,260,286};const int x=index==9?109:columns[index<9?index%3:(index-10)%3];
   const int y=index<9?75+21*(index/3):index==9?137:index<13?148:169;
   const auto button=xeenMainScreenButtonAt(x,y,screen);
   if(!button || button->frame!=index*2 || button->pressedFrame()!=index*2+1 ||
    std::string(button->resource)!=(screen==MainScreen::Combat?"combat.icn":"main.icn"))throw std::runtime_error("main button sprite table");
  }
 }
}
void journalKeys() {
 QueueHarness main;main.ready=true;main.screen=MainScreen::Exploration;
 main.run("main-Quests-key-and-mouse",[](auto &h){
  if(h.stage==0)h.tap(SDLK_v,100);
  else if(h.stage==2)h.click(286,96,SDL_BUTTON_LEFT,100);
  else if(h.stage==4){h.check(h.delivered==std::vector<char>{'V','V'},"main Quests V/mouse route");h.quit();}
 });
 for(bool awards:{false,true}) {
  QueueHarness h;h.queueable=false;h.ready=true;
  h.dialog=std::make_shared<const DialogInput>(xeenJournalInput(awards));
  const std::vector<SDL_Keycode> keys=awards?
   std::vector<SDL_Keycode>{SDLK_UP,SDLK_KP_8,SDLK_DOWN,SDLK_KP_2,SDLK_u,SDLK_d,SDLK_F1,SDLK_HOME,SDLK_ESCAPE}:
   std::vector<SDL_Keycode>{SDLK_UP,SDLK_KP_8,SDLK_DOWN,SDLK_KP_2,SDLK_HOME,SDLK_END,SDLK_PAGEUP,SDLK_PAGEDOWN,SDLK_u,SDLK_d,SDLK_F1,SDLK_ESCAPE};
  h.run("journal-DOS-scroll-keys",[&](auto &h){
   // A strict dialog needs the returned frame acquired before another fresh key.
   if(h.stage<keys.size()*2) {if(h.stage%2==0)h.tap(keys[h.stage/2],100);}
   else {
    std::vector<char> expected{char(InputKey::Up),char(InputKey::Up),char(InputKey::Down),char(InputKey::Down)};
    if(!awards)for(auto k:{InputKey::Home,InputKey::End,InputKey::PageUp,InputKey::PageDown})expected.push_back(char(k));
    expected.push_back('E');
    std::string detail="journal SDL keypad/scroll/ignored keys awards="+std::to_string(awards)+" actual:";
    for(auto k:h.delivered)detail+=" "+std::to_string(static_cast<unsigned char>(k));
    detail+=" expected:";for(auto k:expected)detail+=" "+std::to_string(static_cast<unsigned char>(k));
    h.check(h.delivered==expected,detail.c_str());h.quit();
   }
  });
 }
}
void strictDialogPolicies(){
 QueueHarness h;h.queueable=false;h.ready=true;
 DialogInput input;input.hits.push_back({10,10,30,30,'e'});input.keys={'e',InputKey::Escape};
 h.dialog=std::make_shared<const DialogInput>(std::move(input));
 h.run("dialog-fresh-key-mouse/no-queue/stale-frame/right-outside",[](auto &h){
  if(h.stage==0){h.click(30,15,SDL_BUTTON_LEFT,100);h.tap(SDLK_e,99);h.tap(SDLK_UP,100);}
  else if(h.stage==1){h.check(h.delivered.empty(),"stale/outside dialog input");h.key(SDLK_e,SDL_KEYDOWN,0,100);h.key(SDLK_e,SDL_KEYDOWN,1,100);h.key(SDLK_e,SDL_KEYDOWN,0,100);}
  else if(h.stage==2){h.check(h.delivered==std::vector<char>{'e'},"dialog repeat or duplicate");h.key(SDLK_e,SDL_KEYDOWN,0,100);}
  else if(h.stage==3){h.check(h.delivered.size()==1,"held dialog key");h.key(SDLK_e,SDL_KEYUP,0,100);h.ready=false;h.tap(SDLK_e,100);h.click(15,15,SDL_BUTTON_LEFT,100);}
  else if(h.stage==4){h.check(h.delivered.size()==1,"busy dialog consumed input");h.ready=true;}
  else if(h.stage==5){h.check(h.delivered.size()==1,"busy dialog queued input");h.click(15,15,SDL_BUTTON_LEFT,100);}
  else if(h.stage==6){h.check(h.delivered==std::vector<char>{'e','e'},"fresh mouse differs from key");++h.context;h.redraw=true;h.duringPresentation=[] {QueueHarness::tap(SDLK_e,100);QueueHarness::click(15,15,SDL_BUTTON_LEFT,100);};}
  else if(h.stage==7){h.duringPresentation={};h.check(h.delivered.size()==2,"input from superseded dialog frame");}
  else if(h.stage==8){h.check(h.delivered.size()==2,"stale dialog input replayed");h.tap(SDLK_ESCAPE,100);}
  else if(h.stage==9){h.check(h.delivered==std::vector<char>{'e','e','E'},"dialog Escape response");h.quit();}
 });
}
void mouseQueuePolicies(){
 for(bool mouse:{false,true}) {
  QueueHarness rest;rest.screen=MainScreen::Exploration;rest.restAvailable=true;
  rest.run(mouse?"pending-countdown-mouse-Rest":"pending-countdown-key-Rest",[mouse](auto &h){
   if(h.stage==0){if(mouse)h.click(290,80);else h.tap(SDLK_r);h.tap(SDLK_UP);}
   else if(h.stage==1){h.check(h.delivered==std::vector<char>{'R'},"Rest waited for ordinary countdown readiness");h.queueable=false;h.restAvailable=false;++h.context;}
   else if(h.stage==3){h.check(h.delivered==std::vector<char>{'R'},"Countdown Rest leaked a queued movement");h.quit();}
  });
 }
 QueueHarness h;h.screen=MainScreen::Combat;
 h.run("mouse-key-shared-FIFO-five/enemy-turn-once",[](auto &h){
  if(h.stage==0){h.click(290,80);h.tap(SDLK_b);h.click(261,149);h.tap(SDLK_r);h.click(12,151);h.click(290,80);}
  else if(h.stage<4)h.check(h.delivered.empty(),"busy mouse action drained");
  else if(h.stage==4)h.ready=true;
  else if(h.stage==11){h.check(h.delivered==std::vector<char>{'X','B','W','R','P'},"mixed FIFO/overflow/unsupported action");h.quit();}
 });
 for(const bool panel:{false,true}){
  QueueHarness h;h.screen=MainScreen::Exploration;
  h.run(panel?"mouse-panel-flush":"mouse-combat-transition-flush",[panel](auto &h){
   if(h.stage==0){h.click(261,149);h.tap(SDLK_SPACE);}
   else if(h.stage==1){++h.context;h.ready=true;if(panel){h.queueable=false;h.screen=MainScreen::None;}}
   else if(h.stage==5){h.check(h.delivered.empty(),"mouse context flush");h.quit();}
  });
 }
 for(const auto *name:{"mouse-inventory-ignored","mouse-services-ignored","mouse-dialog-ignored","mouse-casting-ignored"}){
  QueueHarness h;h.ready=true;h.queueable=false;
  h.run(name,[](auto &h){if(h.stage==0){h.click(270,80);h.click(12,151);h.click(100,50,SDL_BUTTON_RIGHT);}
   else if(h.stage==3){h.check(h.delivered.empty(),"strict mouse action");h.quit();}});
 }
 QueueHarness ignore;ignore.screen=MainScreen::Exploration;ignore.ready=true;
 ignore.run("right-click-view-and-letterbox-ignored",[](auto &h){if(h.stage==0){SDL_SetWindowSize(sdl_mouse_test::window(),1000,690);h.click(100,50,SDL_BUTTON_RIGHT);h.click(-1,75);h.click(320,75);}
  else if(h.stage==3){h.check(h.delivered==std::vector<char>{' '},"Part 0: right view click equals left; outside clicks ignored");h.quit();}});
}

void mouseHitAreas(){
 struct Area {int l,t,r,b;PlayerAction exploration,combat;};
 const auto u=[](const char *s)->PlayerAction{return UnsupportedMainScreenAction{s};};
 const std::vector<Area> areas={
  {235,75,259,95,ShootAction{},QuickFightAction{}},{260,75,284,95,CastSpellAction{},CastSpellAction{}},
  {286,75,310,95,RestAction{},AttackAction{}},{235,96,259,116,BashAction{},UseItemAction{}},
  {260,96,284,116,DismissAction{},RevisitCompletedAction{}},{286,96,310,116,QuestsAction{},BlockAction{}},
  {235,117,259,137,MapAction{},QuickFightOptionsAction{}},{260,117,284,137,InfoAction{},InfoAction{}},
  {286,117,310,137,QuickReferenceAction{},QuickReferenceAction{}},
  {235,148,259,168,NavigationAction::TurnLeft,NavigationAction::TurnLeft},
  {260,148,284,168,NavigationAction::MoveForward,NavigationAction::MoveForward},
  {286,148,310,168,NavigationAction::TurnRight,NavigationAction::TurnRight},
  {235,169,259,189,NavigationAction::StrafeLeft,NavigationAction::StrafeLeft},{260,169,284,189,NavigationAction::MoveBackward,NavigationAction::MoveBackward},
  {286,169,310,189,NavigationAction::StrafeRight,NavigationAction::StrafeRight},
  {10,150,42,182,SelectMemberAction{0},SelectMemberAction{0}},{45,150,77,182,SelectMemberAction{1},SelectMemberAction{1}},
  {81,150,113,182,SelectMemberAction{2},SelectMemberAction{2}},{117,150,149,182,SelectMemberAction{3},SelectMemberAction{3}},
  {153,150,185,182,SelectMemberAction{4},SelectMemberAction{4}},{189,150,221,182,SelectMemberAction{5},SelectMemberAction{5}}
 };
 const auto equal=[](const std::optional<PlayerAction> &actual,const PlayerAction &expected){
  if(!actual || actual->index()!=expected.index())return false;
  if(const auto *v=std::get_if<NavigationAction>(&expected))return *v==std::get<NavigationAction>(*actual);
  if(const auto *v=std::get_if<UnsupportedMainScreenAction>(&expected))return std::string(v->label)==std::get<UnsupportedMainScreenAction>(*actual).label;
  return true;
 };
 const auto check=[](bool value){if(!value)throw std::runtime_error("main-screen hit area/SDL scaling");};
 // Exercise SDL's real event filter at several scales, with centered letterboxes.
 for(const auto size:std::vector<std::pair<int,int>>{{320,200},{640,400},{960,600},{1000,700},{500,350}}){
  check(SDL_Init(SDL_INIT_VIDEO|SDL_INIT_EVENTS)==0);
  auto *window=SDL_CreateWindow("mouse scales",0,0,size.first,size.second,0);check(window);
  auto *renderer=SDL_CreateRenderer(window,-1,SDL_RENDERER_SOFTWARE);check(renderer);
  check(SDL_RenderSetLogicalSize(renderer,320,200)==0 && SDL_RenderSetIntegerScale(renderer,SDL_TRUE)==0);
  const int scale=std::min(size.first/320,size.second/200),ox=(size.first-320*scale)/2,oy=(size.second-200*scale)/2;
  const auto logical=[&](int x,int y){
   float cursorX,cursorY;SDL_RenderWindowToLogical(renderer,ox+x*scale,oy+y*scale,&cursorX,&cursorY);
   check(int(cursorX)==x && int(cursorY)==y); // Cursor and clicks share 320x200 scaling.
   SDL_Event event{};event.type=SDL_MOUSEBUTTONDOWN;event.button.windowID=SDL_GetWindowID(window);event.button.button=SDL_BUTTON_LEFT;
   event.button.x=ox+x*scale;event.button.y=oy+y*scale;check(SDL_PushEvent(&event)==1);
   if(SDL_PeepEvents(&event,1,SDL_GETEVENT,SDL_MOUSEBUTTONDOWN,SDL_MOUSEBUTTONDOWN)==1)
    return std::pair<int,int>{event.button.x,event.button.y};
   throw std::runtime_error("SDL scaled click absent");
  };
  for(const auto &area:areas)for(auto screen:{MainScreen::Exploration,MainScreen::Combat}){
   const auto &expected=screen==MainScreen::Combat?area.combat:area.exploration;
   for(const auto point:std::vector<std::pair<int,int>>{{area.l,area.t},{area.r-1,area.t},{area.l,area.b-1},{area.r-1,area.b-1}}){
    const auto p=logical(point.first,point.second);check(p==point);check(equal(xeenMainScreenClick(p.first,p.second,screen),expected));
    check(!xeenMainScreenClick(p.first,p.second,MainScreen::None));
   }
   for(const auto point:std::vector<std::pair<int,int>>{{area.l-1,area.t},{area.r,area.t},{area.l,area.t-1},{area.l,area.b}}){
    const auto p=logical(point.first,point.second);check(p==point);
    check(!equal(xeenMainScreenClick(p.first,p.second,screen),expected));
   }
  }
  for(auto screen:{MainScreen::Exploration,MainScreen::Combat})for(int x:{109,121})for(int y:{137,140,141,146}) {
   const auto p=logical(x,y);const auto click=xeenMainScreenClick(p.first,p.second,screen);
   if(screen==MainScreen::Exploration && y<=140)check(click && std::get<InteractionAction>(*click).openPanel);
   else check(click && std::holds_alternative<ControlPanelAction>(*click));
  }
  for(unsigned row=0;row<3;++row)for(const int x:{239,311})for(const int y:{27+int(row)*10,36+int(row)*10}){
   const auto p=logical(x,y);const auto action=xeenMainScreenClick(p.first,p.second,MainScreen::Combat);
   check(action && std::get<SelectInventorySlotAction>(*action).slot==row);
   if(x<308)check(equal(xeenMainScreenClick(p.first,p.second,MainScreen::Exploration),ToggleAutomapAction{}));
   else check(!xeenMainScreenClick(p.first,p.second,MainScreen::Exploration));
  }
  for(const auto point:std::vector<std::pair<int,int>>{{238,27},{312,27},{239,26},{239,57}}){
   const auto p=logical(point.first,point.second);check(!xeenMainScreenClick(p.first,p.second,MainScreen::Combat));
  }
  for(const auto point:std::vector<std::pair<int,int>>{{8,8},{224,8},{8,135},{224,135}}){
   const auto p=logical(point.first,point.second);check(equal(xeenMainScreenClick(p.first,p.second,MainScreen::Exploration),InteractionAction{}));
  }
  for(const auto point:std::vector<std::pair<int,int>>{{7,8},{8,7},{225,135},{8,136},{8,140}}){
   const auto p=logical(point.first,point.second);check(!xeenMainScreenClick(p.first,p.second,MainScreen::Exploration));
  }
  SDL_DestroyRenderer(renderer);SDL_DestroyWindow(window);SDL_Quit();
 }
 std::cout<<"Main-screen hit areas and SDL native scaling passed\n";
}
void quickFightKeys() {
 QueueHarness h;h.ready=true;h.screen=MainScreen::Combat;
 h.run("Quick-Fight-and-Options-key-mouse-parity",[](auto &h){
  if(h.stage==0)h.tap(SDLK_f,100);
  else if(h.stage==2)h.click(236,76,SDL_BUTTON_LEFT,100);
  else if(h.stage==4)h.tap(SDLK_o,100);
  else if(h.stage==6)h.click(236,118,SDL_BUTTON_LEFT,100);
  else if(h.stage==8){h.check(h.delivered==std::vector<char>{'F','F','O','O'},"Quick Fight/Options parity");h.quit();}
 });
 QueueHarness modal;modal.ready=true;modal.screen=MainScreen::Combat;
 modal.run("Options-held-opener-stale-Next-fresh-Exit",[](auto &h){
  if(h.stage==0)h.key(SDLK_o,SDL_KEYDOWN,0,100);
  else if(h.stage==2){h.dialog=std::make_shared<const DialogInput>(xeenQuickFightInput());h.queueable=false;++h.context;h.redraw=true;}
  else if(h.stage==3){h.key(SDLK_o,SDL_KEYDOWN,1,100);h.key(SDLK_o,SDL_KEYDOWN,0,100);}
  else if(h.stage==4){h.check(h.delivered==std::vector<char>{'O'},"held Options leaked into dialog");h.key(SDLK_o,SDL_KEYUP,0,100);h.tap(SDLK_n,100);}
  else if(h.stage==6)h.click(245,112,SDL_BUTTON_LEFT,100);
  else if(h.stage==8)h.tap(SDLK_RETURN,100);
  else if(h.stage==10){h.check(h.delivered==std::vector<char>{'O','n','n',char(InputKey::Enter)},"Options fresh Next/Return parity");h.quit();}
 });
}
// Every combat button click reaches the flow as its key does; Use (U, the
// potion button) opens the combat Use dialog through the same action.
void combatButtonClicks() {
 QueueHarness h;h.ready=true;h.screen=MainScreen::Combat;
 h.run("combat-button-clicks-and-Use-key-parity",[](auto &h){
  constexpr std::pair<int,int> buttons[]{{236,76},{261,76},{287,76},{236,97},{261,97},{287,97},{236,118},{261,118},{287,118},{110,138}};
  if(h.stage==0)h.tap(SDLK_u,100);
  else if(h.stage>=2 && h.stage<22 && h.stage%2==0)h.click(buttons[(h.stage-2)/2].first,buttons[(h.stage-2)/2].second,SDL_BUTTON_LEFT,100);
  else if(h.stage==24){h.check(h.delivered==std::vector<char>{'u','F','C','X','u','R','B','O','I','Q','P'},"Combat button click lost");h.quit();}
 });
}
void callerEscapeAndAcknowledgment() {
 QueueHarness yes;yes.ready=true;yes.queueable=false;
 DialogInput confirm;confirm.keys={'y','n',InputKey::Escape};
 confirm.hits={{235,75,259,95,'y'},{260,75,284,95,'n'}};
 yes.dialog=std::make_shared<const DialogInput>(confirm);
 yes.run("YesNo-right-Escape-modifier-unrelated-routing",[](auto &h) {
  if(h.stage==0){h.tap(SDLK_LCTRL,100);h.tap(SDLK_RSHIFT,100);h.tap(SDLK_LALT,100);h.tap(SDLK_LGUI,100);h.tap(SDLK_CAPSLOCK,100);h.tap(SDLK_z,100);h.tap(SDLK_RETURN,100);}
  else if(h.stage==1){h.check(h.delivered.empty(),"modifier/unrelated key answered YesNo");h.click(0,0,SDL_BUTTON_RIGHT,100);}
  else if(h.stage==3)h.tap(SDLK_ESCAPE,100);
  else if(h.stage==5){
   SDL_Event event{};event.type=SDL_MOUSEBUTTONDOWN;event.button.button=SDL_BUTTON_RIGHT;
   sdl_mouse_test::position(event.button,235,75);event.button.timestamp=100;
   if(SDL_PushEvent(&event)!=1)throw std::runtime_error("right chord enqueue");
   event.button.button=SDL_BUTTON_LEFT;
   if(SDL_PushEvent(&event)!=1)throw std::runtime_error("left chord enqueue");
   event.type=SDL_MOUSEBUTTONUP;
   if(SDL_PushEvent(&event)!=1)throw std::runtime_error("left chord release");
   event.button.button=SDL_BUTTON_RIGHT;
   if(SDL_PushEvent(&event)!=1)throw std::runtime_error("right chord release");
  }
  else if(h.stage==7){h.check(h.delivered==std::vector<char>{'E','y'},"Part 0: outside right ignored, Escape cancels, either button on Yes selects Yes");h.quit();}
 });
 QueueHarness generic;generic.ready=true;generic.queueable=false;
 DialogInput wait;wait.anyKey=wait.anyClick=wait.keyMouseWait=true;
 generic.dialog=std::make_shared<const DialogInput>(wait);
 generic.run("generic-ordinary-key-either-mouse-and-Escape",[](auto &h) {
  if(h.stage==0){h.tap(SDLK_RALT,100);}
  else if(h.stage==1){h.check(h.delivered.empty(),"modifier acknowledged generic wait");h.tap(SDLK_z,100);}
  else if(h.stage==3)h.click(0,0,SDL_BUTTON_LEFT,100);
  else if(h.stage==5)h.click(319,199,SDL_BUTTON_RIGHT,100);
  else if(h.stage==7)h.tap(SDLK_ESCAPE,100);
  else if(h.stage==9){h.check(h.delivered==std::vector<char>{'K','K','K','K'},"generic wait meanings");h.quit();}
 });
}
void strafeAndSummaryKeys() {
 QueueHarness h;h.ready=true;h.screen=MainScreen::Exploration;
 h.run("strafe-keypad-ctrl-mouse-and-summary-keys",[](auto &h){
  if(h.stage==0)h.tap(SDLK_KP_4,100);
  else if(h.stage==2)h.tap(SDLK_KP_6,100);
  else if(h.stage==4){h.key(SDLK_LEFT,SDL_KEYDOWN,0,100,KMOD_CTRL);h.key(SDLK_LEFT,SDL_KEYUP,0,100);}
  else if(h.stage==6){h.key(SDLK_RIGHT,SDL_KEYDOWN,0,100,KMOD_CTRL);h.key(SDLK_RIGHT,SDL_KEYUP,0,100);}
  else if(h.stage==8)h.click(235,169,SDL_BUTTON_LEFT,100);
  else if(h.stage==10)h.click(286,169,SDL_BUTTON_LEFT,100);
  else if(h.stage==12)h.tap(SDLK_q,100);
  else if(h.stage==14)h.tap(SDLK_i,100);
  else if(h.stage==16)h.tap(SDLK_m,100);
  else if(h.stage==18)h.click(235,117,SDL_BUTTON_LEFT,100);
  else if(h.stage==20)h.tap(SDLK_EQUALS,100);
  else if(h.stage==22)h.tap(SDLK_KP_EQUALS,100);
  else if(h.stage==24)h.click(236,11,SDL_BUTTON_LEFT,100);
  else if(h.stage==26){h.check(h.delivered==std::vector<char>{'A','D','L','T','L','T','Q','I','M','M','=','=','='},("strafe, summaries and Map keyboard/mouse parity: "+std::string(h.delivered.begin(),h.delivered.end())).c_str());h.quit();}
 });
 QueueHarness modal;modal.ready=true;modal.screen=MainScreen::Exploration;
 modal.run("summary-held-opener-and-fresh-dismissal",[](auto &h){
  if(h.stage==0)h.key(SDLK_q,SDL_KEYDOWN,0,100);
  else if(h.stage==2) {
   h.check(h.delivered==std::vector<char>{'Q'},"summary opener missing");
   DialogInput input;input.anyKey=input.anyClick=input.keyMouseWait=true;h.dialog=std::make_shared<const DialogInput>(std::move(input));
   h.queueable=false;++h.context;h.redraw=true;
  } else if(h.stage==3){h.key(SDLK_q,SDL_KEYDOWN,1,100);h.key(SDLK_q,SDL_KEYDOWN,0,100);h.tap(SDLK_LSHIFT,100);}
  else if(h.stage==4){h.check(h.delivered==std::vector<char>{'Q'},"held opener dismissed summary");h.key(SDLK_q,SDL_KEYUP,0,100);h.tap(SDLK_q,100);}
  else if(h.stage==6){h.check(h.delivered==std::vector<char>{'Q','K'},"fresh summary dismissal missing");h.quit();}
 });
 QueueHarness right;right.ready=true;right.queueable=false;
 DialogInput wait;wait.anyKey=wait.anyClick=wait.keyMouseWait=true;right.dialog=std::make_shared<const DialogInput>(std::move(wait));
 right.run("summary-right-click-dismissal",[](auto &h){
  if(h.stage==0)h.click(0,0,SDL_BUTTON_RIGHT,100);
  else if(h.stage==2){h.check(h.delivered==std::vector<char>{'K'},"summary right click did not dismiss");h.quit();}
 });
}
void boundedQueuePolicies(){
 {QueueHarness h;h.screen=MainScreen::Exploration;h.run("Tab/gem queue original panel",[](auto &h){
  if(h.stage==0){h.tap(SDLK_TAB);h.click(114,141);}
  else if(h.stage==1)h.ready=true;
  else if(h.stage==4){h.check(h.delivered==std::vector<char>{'P','P'},"Tab/gem panel mapping");h.quit();}
 });}
 {QueueHarness h;h.screen=MainScreen::Exploration;h.run("modified arrows strafe or retain their refusal",[](auto &h){
  if(h.stage==0)for(auto code:{SDLK_LEFT,SDLK_RIGHT,SDLK_DOWN,SDLK_UP}){h.key(code,SDL_KEYDOWN,0,99,KMOD_CTRL);h.key(code,SDL_KEYUP);}
  else if(h.stage==1)h.ready=true;
  else if(h.stage==6){h.check(h.delivered==std::vector<char>{'L','T','?','?'},"Modified arrow strafe/refusal mapping");h.quit();}
 });}
 QueueHarness{}.run("DOS W waits; A/F unmatched; Dismiss and Shoot",[](auto &h){
  if(h.stage==0)for(auto code:{SDLK_w,SDLK_a,SDLK_d,SDLK_f,SDLK_s})h.tap(code);
  else if(h.stage==1)h.ready=true;
  else if(h.stage==7){h.check(h.delivered==std::vector<char>{'.','D','F'},"Part 0 W Wait, D Dismiss, S Shoot; no movement aliases");h.quit();}
 });
 {QueueHarness h;h.screen=MainScreen::Combat;h.run("original combat A/ignored D-S-Space-Enter",[](auto &h){
  if(h.stage==0)for(auto code:{SDLK_d,SDLK_s,SDLK_SPACE,SDLK_RETURN,SDLK_a}){h.tap(code);h.key(code,SDL_KEYDOWN,1);h.key(code,SDL_KEYUP);}
  else if(h.stage==1)h.ready=true;
  else if(h.stage==4){h.check(h.delivered==std::vector<char>{'X'},"Combat D stays unmatched; Attack context or repeated action");h.quit();}
 });}
 QueueHarness{}.run("FIFO/bound-five/overflow/one-per-ready-frame",[](auto &h){
  if(h.stage==0){for(auto code:{SDLK_UP,SDLK_LEFT,SDLK_DOWN,SDLK_RIGHT,SDLK_SPACE,SDLK_b})h.tap(code);}
  else if(h.stage==1){h.check(h.delivered.empty(),"busy consumed queue");h.ready=true;}
  else if(h.stage<=6){h.check(h.delivered.size()==h.stage-1,"FIFO drain count");if(h.stage==6){h.check(h.delivered==std::vector<char>{'W','A','S','D',' '},"FIFO or overflow");h.quit();}}
 });
 QueueHarness{}.run("six-distinct-taps-no-loss",[](auto &h){
  if(h.stage==0){for(unsigned n=0;n<6;++n)h.tap(SDLK_UP);}
  else if(h.stage==1)h.ready=true;
  else if(h.stage==7){h.check(h.delivered==std::vector<char>(6,'W'),"fresh physical edges were coalesced or overflow accepted");h.quit();}
 });
 QueueHarness{}.run("movement-repeat/single-pending-repeat/physical-edges/held-release",[](auto &h){
  if(h.stage==0){h.key(SDLK_UP);for(unsigned n=0;n<20;++n)h.key(SDLK_UP,SDL_KEYDOWN,1);h.key(SDLK_UP);h.key(SDLK_UP,SDL_KEYUP);h.tap(SDLK_LEFT);h.tap(SDLK_DOWN);h.tap(SDLK_RIGHT);h.tap(SDLK_b);}
  else if(h.stage==1)h.ready=true;
  else if(h.stage==7){h.check(h.delivered==std::vector<char>{'W','A','S','D','B'},"repeat backlog or physical bound");h.quit();}
 });
 QueueHarness{}.run("Space-B-F-R-repeat-ignored",[](auto &h){
  if(h.stage==0){for(auto code:{SDLK_SPACE,SDLK_b,SDLK_s,SDLK_r}){h.key(code);h.key(code,SDL_KEYDOWN,1);h.key(code,SDL_KEYUP);}}
  else if(h.stage==1)h.ready=true;
  else if(h.stage==6){h.check(h.delivered==std::vector<char>{' ','B','F','R'},"command auto repeat");h.quit();}
 });
 for(const auto *name:{"enemy-turn-Space-survives","ATT-projectile-animation-survives","automatic-work-survives","handoff-survives"})QueueHarness{}.run(name,[](auto &h){
  h.redraw=true;
  if(h.stage==0)h.tap(SDLK_SPACE);
  else if(h.stage<6)h.check(h.delivered.empty(),"busy frame consumed Space");
  else if(h.stage==6)h.ready=true;
  else if(h.stage==10){h.check(h.delivered==std::vector<char>{' '},"Space lost/retried after ready refusal");h.quit();}
 });
 QueueHarness{}.run("Escape-clears/F9-immediate-never-replayed",[](auto &h){
  if(h.stage==0){h.tap(SDLK_UP);h.tap(SDLK_F9,100);}
  else if(h.stage==1){h.check(h.delivered==std::vector<char>{'9'},"F9 buffered");h.tap(SDLK_ESCAPE,100);}
  else if(h.stage==2){h.ready=true;h.tap(SDLK_UP);h.key(SDLK_ESCAPE,SDL_KEYDOWN,1,100);}
  else if(h.stage==5){h.check(h.delivered==std::vector<char>{'9','E','W'},"Escape flush/F9 replay");h.quit();}
 });
 QueueHarness{}.run("focus-loss-clears-held-and-queue",[](auto &h){
  if(h.stage==0){h.key(SDLK_UP);SDL_Event event{};event.type=SDL_WINDOWEVENT;event.window.event=SDL_WINDOWEVENT_FOCUS_LOST;SDL_PushEvent(&event);h.key(SDLK_UP,SDL_KEYDOWN,1);h.tap(SDLK_UP);}
  else if(h.stage==1)h.ready=true;
  else if(h.stage==4){h.check(h.delivered==std::vector<char>{'W'},"focus loss stranded held or replayed action");h.quit();}
 });
 for(const auto *name:{"combat-end-flush","dialog-open-flush","map-transition-flush","save-failure-flush"})QueueHarness{}.run(name,[](auto &h){
  if(h.stage==0)h.tap(SDLK_SPACE);
  else if(h.stage==1){++h.context;h.ready=true;}
  else if(h.stage==4){h.check(h.delivered.empty(),"old context replay");h.quit();}
 });
 for(const auto *name:{"service-strict","dialog-strict","reward-strict","inventory-strict","casting-strict"}){
  QueueHarness h;h.queueable=false;h.ready=true;h.run(name,[](auto &h){
   if(h.stage==0)h.tap(SDLK_SPACE); // Pre-ready timestamp stays discarded.
   else if(h.stage==1){h.check(h.delivered.empty(),"strict timestamp bypass");h.tap(SDLK_SPACE,100);}
   else if(h.stage==4){h.check(h.delivered==std::vector<char>{' '},"strict edge changed");h.quit();}
  });
 }
 QueueHarness{}.run("movement-redraw/same-generation-input-buffered",[](auto &h){
  if(h.stage==0){h.duringPresentation=[&h]{h.duringPresentation={};h.tap(SDLK_UP);};h.tap(SDLK_UP);h.ready=true;}
  else if(h.stage==5){h.check(h.delivered==std::vector<char>{'W','W'},"redraw key lost");h.quit();}
 });
}

void navigationObligations() {
 QueueHarness{}.run("eight-fresh-taps-behind-unacquired-frame",[](auto &h) {
  if(h.stage==0) {
   ++h.epoch;h.redraw=true;
   h.duringPresentation=[&h] {h.duringPresentation={};for(unsigned i=0;i<8;++i)h.tap(SDLK_UP);};
  } else if(h.stage==2)h.ready=true;
  else if(h.stage==11){h.check(h.delivered==std::vector<char>(8,'W'),"unacquired frame lost a distinct tap");h.quit();}
 });
 QueueHarness{}.run("held-demand-release-batch-preserves-distinct-taps",[](auto &h) {
  if(h.stage==0)h.key(SDLK_UP);
  else if(h.stage==1)h.ready=true;
  else if(h.stage==2)h.clock+=500000000;
  else if(h.stage==3) {
   h.check(h.delivered==std::vector<char>{'W','W'},"held demand must repeat without OS makes");
   for(unsigned i=0;i<30;++i)h.key(SDLK_UP,SDL_KEYDOWN,1);
   h.key(SDLK_UP,SDL_KEYUP);h.tap(SDLK_LEFT);h.tap(SDLK_UP);
  } else if(h.stage==7){h.check(h.delivered==std::vector<char>{'W','W','A','W'},"release retained repeats or discarded fresh taps");h.quit();}
 });
 {QueueHarness h;h.screen=MainScreen::Exploration;
 h.run("held-turn-modifier-changes-and-release",[](auto &h) {
  if(h.stage==0)h.key(SDLK_LEFT);
  else if(h.stage==1)h.ready=true;
  else if(h.stage==2){h.clock+=500000000;h.key(SDLK_LCTRL,SDL_KEYDOWN,0,100,KMOD_LCTRL);}
  else if(h.stage==3)h.key(SDLK_LCTRL,SDL_KEYUP,0,100,0);
  else if(h.stage==4)h.key(SDLK_LEFT,SDL_KEYUP);
  else if(h.stage==6){h.check(h.delivered==std::vector<char>{'A','L','A'},"held physical key lost modifier/release identity");h.quit();}
 });}
 QueueHarness{}.run("physical-break-survives-keysym-change",[](auto &h) {
  const auto key=[](SDL_Keycode symbol,SDL_Scancode scan,Uint32 type,Uint8 repeat=0) {
   SDL_Event e{};e.type=type;e.key.keysym.sym=symbol;e.key.keysym.scancode=scan;e.key.repeat=repeat;e.key.timestamp=100;
   if(SDL_PeepEvents(&e,1,SDL_ADDEVENT,0,0)!=1)throw std::runtime_error("physical-key fixture enqueue");
  };
  if(h.stage==0)key(SDLK_LEFT,SDL_SCANCODE_KP_4,SDL_KEYDOWN);
  else if(h.stage==1)h.ready=true;
  else if(h.stage==2)h.clock+=500000000;
  else if(h.stage==3) {
   key(SDLK_KP_4,SDL_SCANCODE_KP_4,SDL_KEYUP);
   for(unsigned n=0;n<20;++n)key(SDLK_LEFT,SDL_SCANCODE_KP_4,SDL_KEYDOWN,1);
   key(SDLK_UP,SDL_SCANCODE_KP_8,SDL_KEYDOWN);key(SDLK_KP_8,SDL_SCANCODE_KP_8,SDL_KEYUP);
  } else if(h.stage==7){h.check(h.delivered==std::vector<char>{'A','A','W'},"keysym change stranded held physical key");h.quit();}
 });

}

void navigationTypematic() {
 for(const auto key:std::vector<std::pair<SDL_Keycode,Uint16>>{{SDLK_UP,0},{SDLK_LEFT,0},{SDLK_RIGHT,0},{SDLK_LEFT,KMOD_LCTRL}})
  for(bool drawPaced:{false,true}) {
   QueueHarness h;h.screen=MainScreen::Exploration;h.ready=true;h.clockStep=0;h.drawPaced=drawPaced;
   h.run("navigation-initial-delay-rate-draw-cap-and-release",[&](auto &h) {
    const auto third=drawPaced?PresentationClock::boundary(11):591666667ULL;
    if(h.stage==0)h.key(key.first,SDL_KEYDOWN,0,100,key.second);
    else if(h.stage==1){
     h.check(h.delivered.size()==1 && h.deliveredAt[0]==0,"first press must act without typematic delay");
     h.clock=499999999;
     for(unsigned i=0;i<20;++i)h.key(key.first,SDL_KEYDOWN,1,100,key.second);
    } else if(h.stage==2){h.check(h.delivered.size()==1,"OS repeats or held state bypassed initial delay");h.clock=500000000;}
    else if(h.stage==3){h.check(h.delivered.size()==2 && h.deliveredAt[1]==500000000,"hold did not repeat at initial deadline");h.clock=third-1;}
    else if(h.stage==4){h.check(h.delivered.size()==2,"typematic period or draw floor bypassed");h.clock=third;}
    else if(h.stage==5){h.check(h.delivered.size()==3 && h.deliveredAt[2]==third,"repeat rate must be capped by drawing");h.clock+=1000000000;}
    else if(h.stage==6)h.check(h.delivered.size()==4,"late wake replayed repeats");
    else if(h.stage==7){
     h.check(h.delivered.size()==4,"same-time observation repeated an action");
     h.key(key.first,SDL_KEYUP,0,100,key.second);h.clock+=2000000000;
    } else if(h.stage==8){h.check(h.delivered.size()==4,"release retained typematic demand");for(unsigned i=0;i<8;++i)h.tap(SDLK_UP);}
    else if(h.stage<17)h.clock+=110000000;
    else if(h.stage==17){h.check(h.delivered.size()==12,"distinct taps lost or gained actions after hold release");h.quit();}
   });
  }
}

void pushKey(std::atomic<bool> &finished, SDL_Keycode key, std::uint8_t repeat,
		std::uint32_t type = SDL_KEYDOWN) {
	for (int attempt = 0; attempt < 100 && !finished; ++attempt) {
		std::this_thread::sleep_for(std::chrono::milliseconds(10));
		SDL_Event event{};
		event.type = type;
		event.key.state = type == SDL_KEYUP ? SDL_RELEASED : SDL_PRESSED;
		event.key.repeat = repeat;
		event.key.keysym.sym = key;
		if (SDL_PushEvent(&event) == 1)
			return;
	}
	throw std::runtime_error("could not push SDL test event");
}

} // namespace

int main(int argc,char **) {
	probe_fired::expect("SDL_GetTicks");
 if(argc>1){journalKeys();mouseHitAreas();mouseQueuePolicies();strictDialogPolicies();pressedButtonFeedback();strafeAndSummaryKeys();quickFightKeys();combatButtonClicks();callerEscapeAndAcknowledgment();
  const auto directory=std::filesystem::temp_directory_path() / ("mmodern-square-mouse-"+std::to_string(std::chrono::steady_clock::now().time_since_epoch().count()));
  const auto settings=directory/"window.ini";
  if(!writeWindowPlacement(settings,{false,960,600,PictureAspect::Square}))throw std::runtime_error("square fixture settings");
  SdlWindow::rememberPlacement(settings);QueueHarness::mouseAspect=PictureAspect::Square;
  combatButtonClicks();
  QueueHarness::mouseAspect=PictureAspect::FourThree;SdlWindow::rememberPlacement(std::nullopt);
  std::error_code ignored;std::filesystem::remove_all(directory,ignored);return 0;}
 dosKeyTable();dosRepeatsAndClears();retainedEventRegressions();eitherMouseButton();
 semanticBoundaryKeys();cutsceneControlQuit();
 boundedQueuePolicies();
 navigationObligations();
 navigationTypematic();
 wordInputBoundaries();inputWaitCompletesBeforeAnotherDraw();
	std::atomic<bool> finished{false};
	std::atomic<int> interactions{0};
	std::atomic<int> navigation{0};
	std::atomic<int> acknowledgments{0};
	std::atomic<int> yes{0};
	std::atomic<int> no{0};
	std::atomic<int> selections{0};
	std::atomic<int> cancellations{0};
	std::atomic<int> inspections{0};
	std::atomic<int> slots{0}, transfers{0}, equipment{0}, uses{0}, shots{0}, casts{0};
	std::atomic<bool> canCancel{true};
	std::exception_ptr senderError;
	std::thread sender([&] {
		try {
			pushKey(finished, SDLK_s, 0);
            pushKey(finished, SDLK_s, 1);
			pushKey(finished, SDLK_s, 0, SDL_KEYUP);
			pushKey(finished, SDLK_c, 0);
			pushKey(finished, SDLK_c, 1);
			pushKey(finished, SDLK_c, 0, SDL_KEYUP);
			pushKey(finished, SDLK_SPACE, 0);
			pushKey(finished, SDLK_i, 0);
			pushKey(finished, SDLK_i, 1);
			pushKey(finished, SDLK_t, 0);
			pushKey(finished, SDLK_t, 1);
			pushKey(finished, SDLK_t, 0, SDL_KEYUP);
			pushKey(finished, SDLK_e, 0);
			pushKey(finished, SDLK_e, 1);
			pushKey(finished, SDLK_e, 0, SDL_KEYUP);
			pushKey(finished, SDLK_u, 0);
			pushKey(finished, SDLK_u, 1);
			pushKey(finished, SDLK_u, 0, SDL_KEYUP);
			for (int i=0;i<9;++i) {
				pushKey(finished, SDLK_1+i, 0);
				pushKey(finished, SDLK_1+i, 1);
				pushKey(finished, SDLK_1+i, 0, SDL_KEYUP);
			}
			pushKey(finished, SDLK_SPACE, 1);
			pushKey(finished, SDLK_SPACE, 0, SDL_KEYUP);
			pushKey(finished, SDLK_SPACE, 0);
			pushKey(finished, SDLK_UP, 0);
			pushKey(finished, SDLK_RETURN, 0);
			pushKey(finished, SDLK_y, 1);
			pushKey(finished, SDLK_y, 0);
			pushKey(finished, SDLK_n, 0);
			for (int i=0; i<6; ++i) {
				pushKey(finished, SDLK_F1+i, 0);
				pushKey(finished, SDLK_F1+i, 1);
			}
			pushKey(finished, SDLK_ESCAPE, 0);
			pushKey(finished, SDLK_ESCAPE, 1);
			pushKey(finished, SDLK_RIGHT, 0); // proves held Escape did not exit
			pushKey(finished, SDLK_ESCAPE, 0, SDL_KEYUP);
			pushKey(finished, SDLK_ESCAPE, 0); // nothing to cancel: ignored, never quits
			pushKey(finished, SDLK_ESCAPE, 0, SDL_KEYUP);
			pushKey(finished, SDLK_LEFT, 0); // proves that Escape did not exit
			for (int attempt = 0; attempt < 100 && !finished; ++attempt) {
				std::this_thread::sleep_for(std::chrono::milliseconds(10));
				SDL_Event quit{};
				quit.type = SDL_QUIT;
				if (SDL_PushEvent(&quit) == 1) break;
			}
		} catch (...) {
			senderError = std::current_exception();
		}
	});

	IndexedFrame frame;
	frame.width = 1;
	frame.height = 1;
	frame.pixels = {0};
	const bool result = SdlWindow().showInteractive(frame, "MMModern input test",
		[&](const PlayerAction &action) -> std::optional<IndexedFrame> {
			if (std::holds_alternative<ShootAction>(action)) ++shots;
			else if (std::holds_alternative<CastSpellAction>(action)) ++casts;
            else if (std::holds_alternative<InteractionAction>(action))
				++interactions;
			else if (std::holds_alternative<NavigationAction>(action))
				++navigation;
			else if (std::holds_alternative<AcknowledgeAction>(action))
				++acknowledgments;
			else if (std::holds_alternative<YesAction>(action))
				++yes;
			else if (std::holds_alternative<NoAction>(action))
				++no;
			else if (std::holds_alternative<InfoAction>(action))
				++inspections;
			else if (std::holds_alternative<TransferInventoryAction>(action)) ++transfers;
			else if (std::holds_alternative<EquipmentInventoryAction>(action)) ++equipment;
			else if (std::holds_alternative<UseItemAction>(action)) ++uses;
			else if (const auto *slot=std::get_if<SelectInventorySlotAction>(&action)) {
				if (slot->slot!=static_cast<std::size_t>(slots++)) throw std::runtime_error("1-9 slot mapping");
			}
			else if (const auto *selection=std::get_if<SelectMemberAction>(&action)) {
				if (selection->partyIndex!=static_cast<std::size_t>(selections.load()))
					throw std::runtime_error("F1-F6 active index mapping");
				++selections;
			} else if (std::holds_alternative<CancelInteractionAction>(action)) {
				++cancellations;
				canCancel=false;
			}
			return std::nullopt;
		}, [&] { return canCancel.load(); });
	finished = true;
	sender.join();
	if (senderError)
		std::rethrow_exception(senderError);
	if (!result || interactions != 2 || navigation != 3 || acknowledgments != 1 ||
			yes != 1 || no != 1 || selections != 6 || cancellations != 1 || inspections != 1 || slots != 9 || transfers != 0 || equipment != 0 || uses != 1 || shots != 1 || casts != 1) {
		std::cerr << "Space dispatch/repeat filtering failed: interactions="
			<< interactions << " navigation=" << navigation
			<< " acknowledgments=" << acknowledgments << " yes=" << yes
			<< " no=" << no << '\n';
		return 1;
	}
	std::cout << "SDL action dispatch and key-repeat filtering OK\n";
	return 0;
}
