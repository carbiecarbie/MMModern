#include "XeenProbeFired.h"
#include "platform/sdl/SdlWindow.h"
#include "platform/sdl/XeenMainScreenInput.h"

#define SDL_MAIN_HANDLED
#include <SDL.h>

#include <atomic>
#include <chrono>
#include <iostream>
#include <stdexcept>
#include <thread>
#include <variant>

using namespace mmodern;

// Deterministic millisecond collision at the production SDL boundary.
static bool fixedTicks=false;
extern "C" Uint32 __real_SDL_GetTicks();
extern "C" Uint32 __wrap_SDL_GetTicks(){if(fixedTicks)probe_fired::hit("SDL_GetTicks");return fixedTicks?100:__real_SDL_GetTicks();}

namespace {

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
  case 6:key(SDLK_w,SDL_KEYDOWN,99);key(SDLK_w,SDL_KEYUP,99);key(SDLK_RIGHT);key(SDLK_RIGHT,SDL_KEYUP);break;
  default:check(accepted==3);{SDL_Event e{};e.type=SDL_QUIT;SDL_PushEvent(&e);}break;
  }
  return {};
 });
 fixedTicks=false;check(ok&&accepted==3&&queuedOld);
}

// Fake readiness/clock, real production event queue and presentation loop.
struct QueueHarness {
 MainScreen screen=MainScreen::None;
 bool redraw=false,ready=false,queueable=true;std::uint64_t context=1,epoch=1,presented=0;
 unsigned stage=0,presentations=0,lastDelivery=0;std::vector<char> delivered;
 std::function<void()> duringPresentation;
 static void key(SDL_Keycode code,Uint32 type=SDL_KEYDOWN,Uint8 repeat=0,Uint32 stamp=99){
  SDL_Event event{};event.type=type;event.key.keysym.sym=code;event.key.repeat=repeat;event.key.timestamp=stamp;
  if(SDL_PeepEvents(&event,1,SDL_ADDEVENT,0,0)!=1)throw std::runtime_error("queue fixture key");
 }
 static void tap(SDL_Keycode code,Uint32 stamp=99){key(code,SDL_KEYDOWN,0,stamp);key(code,SDL_KEYUP,0,stamp);}
 static void click(int x,int y,Uint8 button=SDL_BUTTON_LEFT){
  SDL_Event event{};event.type=SDL_MOUSEBUTTONDOWN;event.button.button=button;event.button.x=x;event.button.y=y;
  if(SDL_PeepEvents(&event,1,SDL_ADDEVENT,0,0)!=1)throw std::runtime_error("queue fixture logical click");
 }
 static void quit(){SDL_Event event{};event.type=SDL_QUIT;SDL_PushEvent(&event);}
 void check(bool value,const char *message){if(!value)throw std::runtime_error(std::string(message)+" stage="+std::to_string(stage));}
 void run(const char *name,const std::function<void(QueueHarness &)> &script){
  IndexedFrame frame;frame.width=320;frame.height=200;frame.pixels.resize(64000);
  SdlWindow::FrameUpdateHandler handler=[](const auto &)->std::optional<IndexedFrame>{throw std::runtime_error("unversioned queued action");};
  handler.protectAllKeys=true;handler.displayedInput=[&]{return std::optional<std::uint64_t>{epoch};};
  handler.inputContext=[&](const auto &){return InputContext{context,queueable,ready && presented==epoch,screen};};
  handler.framePresented=[&](const auto &){presented=epoch;++presentations;const auto hook=duringPresentation;if(hook)hook();};
  handler.withPresentedInput=[&](const PlayerAction &action,auto token,const auto &)->std::optional<IndexedFrame>{
   if(token!=epoch){check(!queueable,"queued key reached a stale frame");return {};}
   const bool immediate=std::holds_alternative<SaveGameAction>(action) || std::holds_alternative<CancelInteractionAction>(action);
   check(!queueable || immediate || (ready && lastDelivery!=presentations),"drained twice or while busy");lastDelivery=presentations;
   char kind='?';if(const auto *nav=std::get_if<NavigationAction>(&action))kind=*nav==NavigationAction::MoveForward?'W':*nav==NavigationAction::MoveBackward?'S':*nav==NavigationAction::TurnLeft?'A':'D';
   else if(std::holds_alternative<InteractionAction>(action))kind=' ';
   else if(std::holds_alternative<BlockAction>(action))kind='B';
   else if(std::holds_alternative<ShootAction>(action))kind='F';
   else if(std::holds_alternative<RevisitCompletedAction>(action))kind='R';
   else if(std::holds_alternative<SaveGameAction>(action))kind='9';
   else if(std::holds_alternative<CancelInteractionAction>(action))kind='E';
   else if(std::holds_alternative<UnsupportedMainScreenAction>(action))kind='U';
   delivered.push_back(kind);++epoch;return frame;
  };
  fixedTicks=true;
  const bool ok=SdlWindow().showInteractive(frame,name,handler,[]{return true;},[&]()->std::optional<IndexedFrame>{
   check(stage<60,"queue test timeout");redraw=false;script(*this);++stage;return redraw?std::optional<IndexedFrame>{frame}:std::nullopt;
  });
  fixedTicks=false;check(ok,"queue show failed");std::cout<<name<<" passed\n";
 }
};
void mouseQueuePolicies(){
 QueueHarness h;h.screen=MainScreen::Combat;
 h.run("mouse-key-shared-FIFO-five/enemy-turn-once",[](auto &h){
  if(h.stage==0){h.click(290,80);h.tap(SDLK_b);h.click(261,149);h.tap(SDLK_r);h.click(12,151);h.click(290,80);}
  else if(h.stage<4)h.check(h.delivered.empty(),"busy mouse action drained");
  else if(h.stage==4)h.ready=true;
  else if(h.stage==11){h.check(h.delivered==std::vector<char>{' ','B','W','R','U'},"mixed FIFO/overflow/unsupported action");h.quit();}
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
 ignore.run("right-click-and-letterbox-ignored",[](auto &h){if(h.stage==0){h.click(100,50,SDL_BUTTON_RIGHT);h.click(-1,75);h.click(320,75);}
  else if(h.stage==3){h.check(h.delivered.empty(),"right/outside click action");h.quit();}});
}

void mouseHitAreas(){
 struct Area {int l,t,r,b;PlayerAction exploration,combat;};
 const auto u=[](const char *s)->PlayerAction{return UnsupportedMainScreenAction{s};};
 const std::vector<Area> areas={
  {235,75,259,95,ShootAction{},u("Quick Fight")},{260,75,284,95,CastSpellAction{},CastSpellAction{}},
  {286,75,310,95,u("Rest"),InteractionAction{}},{235,96,259,116,u("Bash"),u("Use")},
  {260,96,284,116,u("Dismiss"),RevisitCompletedAction{}},{286,96,310,116,u("View Quests"),BlockAction{}},
  {235,117,259,137,u("Map"),u("Quick Fight Options")},{260,117,284,137,u("Info"),u("Info")},
  {286,117,310,137,u("Quick Ref"),u("Quick Ref")},{109,137,122,147,u("Control panel"),u("Control panel")},
  {235,148,259,168,NavigationAction::TurnLeft,NavigationAction::TurnLeft},
  {260,148,284,168,NavigationAction::MoveForward,NavigationAction::MoveForward},
  {286,148,310,168,NavigationAction::TurnRight,NavigationAction::TurnRight},
  {235,169,259,189,u("Strafe"),u("Strafe")},{260,169,284,189,NavigationAction::MoveBackward,NavigationAction::MoveBackward},
  {286,169,310,189,u("Strafe"),u("Strafe")},
  {10,150,42,182,u("Character sheet"),u("Character sheet")},{45,150,77,182,u("Character sheet"),u("Character sheet")},
  {81,150,113,182,u("Character sheet"),u("Character sheet")},{117,150,149,182,u("Character sheet"),u("Character sheet")},
  {153,150,185,182,u("Character sheet"),u("Character sheet")},{189,150,221,182,u("Character sheet"),u("Character sheet")}
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
  for(unsigned row=0;row<3;++row)for(const int x:{239,311})for(const int y:{27+int(row)*10,36+int(row)*10}){
   const auto p=logical(x,y);const auto action=xeenMainScreenClick(p.first,p.second,MainScreen::Combat);
   check(action && std::get<SelectInventorySlotAction>(*action).slot==row);
   check(!xeenMainScreenClick(p.first,p.second,MainScreen::Exploration));
  }
  for(const auto point:std::vector<std::pair<int,int>>{{238,27},{312,27},{239,26},{239,57}}){
   const auto p=logical(point.first,point.second);check(!xeenMainScreenClick(p.first,p.second,MainScreen::Combat));
  }
  for(const auto point:std::vector<std::pair<int,int>>{{8,8},{223,8},{8,139},{223,139}}){
   const auto p=logical(point.first,point.second);check(equal(xeenMainScreenClick(p.first,p.second,MainScreen::Exploration),InteractionAction{}));
  }
  for(const auto point:std::vector<std::pair<int,int>>{{7,8},{8,7},{224,139},{8,140}}){
   const auto p=logical(point.first,point.second);check(!xeenMainScreenClick(p.first,p.second,MainScreen::Exploration));
  }
  SDL_DestroyRenderer(renderer);SDL_DestroyWindow(window);SDL_Quit();
 }
 std::cout<<"Main-screen hit areas and SDL native scaling passed\n";
}
void boundedQueuePolicies(){
 QueueHarness{}.run("FIFO/bound-five/overflow/one-per-ready-frame",[](auto &h){
  if(h.stage==0){for(auto code:{SDLK_w,SDLK_a,SDLK_s,SDLK_d,SDLK_SPACE,SDLK_b})h.tap(code);}
  else if(h.stage==1){h.check(h.delivered.empty(),"busy consumed queue");h.ready=true;}
  else if(h.stage<=6){h.check(h.delivered.size()==h.stage-1,"FIFO drain count");if(h.stage==6){h.check(h.delivered==std::vector<char>{'W','A','S','D',' '},"FIFO or overflow");h.quit();}}
 });
 QueueHarness{}.run("five-physical-W-edges",[](auto &h){
  if(h.stage==0){for(unsigned n=0;n<6;++n)h.tap(SDLK_w);}
  else if(h.stage==1)h.ready=true;
  else if(h.stage==7){h.check(h.delivered==std::vector<char>(5,'W'),"fresh physical edges were coalesced or overflow accepted");h.quit();}
 });
 QueueHarness{}.run("movement-repeat/single-pending-repeat/physical-edges/held-release",[](auto &h){
  if(h.stage==0){h.key(SDLK_w);for(unsigned n=0;n<20;++n)h.key(SDLK_w,SDL_KEYDOWN,1);h.key(SDLK_w);h.key(SDLK_w,SDL_KEYUP);h.tap(SDLK_a);h.tap(SDLK_s);h.tap(SDLK_d);h.tap(SDLK_b);}
  else if(h.stage==1)h.ready=true;
  else if(h.stage==7){h.check(h.delivered==std::vector<char>{'W','W','A','S','D'},"repeat backlog or physical bound");h.quit();}
 });
 QueueHarness{}.run("Space-B-F-R-repeat-ignored",[](auto &h){
  if(h.stage==0){for(auto code:{SDLK_SPACE,SDLK_b,SDLK_f,SDLK_r}){h.key(code);h.key(code,SDL_KEYDOWN,1);h.key(code,SDL_KEYUP);}}
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
  if(h.stage==0){h.tap(SDLK_w);h.tap(SDLK_F9,100);}
  else if(h.stage==1){h.check(h.delivered==std::vector<char>{'9'},"F9 buffered");h.tap(SDLK_ESCAPE,100);}
  else if(h.stage==2){h.ready=true;h.tap(SDLK_w);h.key(SDLK_ESCAPE,SDL_KEYDOWN,1,100);}
  else if(h.stage==5){h.check(h.delivered==std::vector<char>{'9','E','W'},"Escape flush/F9 replay");h.quit();}
 });
 QueueHarness{}.run("focus-loss-clears-held-and-queue",[](auto &h){
  if(h.stage==0){h.key(SDLK_w);SDL_Event event{};event.type=SDL_WINDOWEVENT;event.window.event=SDL_WINDOWEVENT_FOCUS_LOST;SDL_PushEvent(&event);h.key(SDLK_w,SDL_KEYDOWN,1);h.tap(SDLK_w);}
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
  if(h.stage==0){h.duringPresentation=[&h]{h.duringPresentation={};h.tap(SDLK_w);};h.tap(SDLK_w);h.ready=true;}
  else if(h.stage==5){h.check(h.delivered==std::vector<char>{'W','W'},"redraw key lost");h.quit();}
 });
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
 if(argc>1){mouseHitAreas();mouseQueuePolicies();return 0;}
 semanticBoundaryKeys();
 boundedQueuePolicies();
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
			pushKey(finished, SDLK_f, 0);
            pushKey(finished, SDLK_f, 1);
			pushKey(finished, SDLK_f, 0, SDL_KEYUP);
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
			pushKey(finished, SDLK_w, 0);
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
			pushKey(finished, SDLK_ESCAPE, 0);
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
			else if (std::holds_alternative<InspectInventoryAction>(action))
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
	if (!result || interactions != 2 || navigation != 2 || acknowledgments != 1 ||
			yes != 1 || no != 1 || selections != 6 || cancellations != 1 || inspections != 1 || slots != 9 || transfers != 1 || equipment != 1 || uses != 1 || shots != 1 || casts != 1) {
		std::cerr << "Space dispatch/repeat filtering failed: interactions="
			<< interactions << " navigation=" << navigation
			<< " acknowledgments=" << acknowledgments << " yes=" << yes
			<< " no=" << no << '\n';
		return 1;
	}
	std::cout << "SDL action dispatch and key-repeat filtering OK\n";
	return 0;
}
