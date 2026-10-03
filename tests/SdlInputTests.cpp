#include "XeenProbeFired.h"
#include "platform/sdl/SdlWindow.h"

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
 bool redraw=false,ready=false,queueable=true;std::uint64_t context=1,epoch=1,presented=0;
 unsigned stage=0,presentations=0,lastDelivery=0;std::vector<char> delivered;
 std::function<void()> duringPresentation;
 static void key(SDL_Keycode code,Uint32 type=SDL_KEYDOWN,Uint8 repeat=0,Uint32 stamp=99){
  SDL_Event event{};event.type=type;event.key.keysym.sym=code;event.key.repeat=repeat;event.key.timestamp=stamp;
  if(SDL_PeepEvents(&event,1,SDL_ADDEVENT,0,0)!=1)throw std::runtime_error("queue fixture key");
 }
 static void tap(SDL_Keycode code,Uint32 stamp=99){key(code,SDL_KEYDOWN,0,stamp);key(code,SDL_KEYUP,0,stamp);}
 static void quit(){SDL_Event event{};event.type=SDL_QUIT;SDL_PushEvent(&event);}
 void check(bool value,const char *message){if(!value)throw std::runtime_error(std::string(message)+" stage="+std::to_string(stage));}
 void run(const char *name,const std::function<void(QueueHarness &)> &script){
  IndexedFrame frame{1,1,{0}};
  SdlWindow::FrameUpdateHandler handler=[](const auto &)->std::optional<IndexedFrame>{throw std::runtime_error("unversioned queued action");};
  handler.protectAllKeys=true;handler.displayedInput=[&]{return std::optional<std::uint64_t>{epoch};};
  handler.inputContext=[&](const auto &){return InputContext{context,queueable,ready && presented==epoch};};
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
   delivered.push_back(kind);++epoch;return frame;
  };
  fixedTicks=true;
  const bool ok=SdlWindow().showInteractive(frame,name,handler,[]{return true;},[&]()->std::optional<IndexedFrame>{
   check(stage<60,"queue test timeout");redraw=false;script(*this);++stage;return redraw?std::optional<IndexedFrame>{frame}:std::nullopt;
  });
  fixedTicks=false;check(ok,"queue show failed");std::cout<<name<<" passed\n";
 }
};
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

int main() {
	probe_fired::expect("SDL_GetTicks");
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
