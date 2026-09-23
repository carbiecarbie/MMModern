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
extern "C" Uint32 __wrap_SDL_GetTicks(){return fixedTicks?100:__real_SDL_GetTicks();}

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
 semanticBoundaryKeys();
	std::atomic<bool> finished{false};
	std::atomic<int> interactions{0};
	std::atomic<int> navigation{0};
	std::atomic<int> acknowledgments{0};
	std::atomic<int> yes{0};
	std::atomic<int> no{0};
	std::atomic<int> selections{0};
	std::atomic<int> cancellations{0};
	std::atomic<int> inspections{0};
	std::atomic<int> slots{0}, transfers{0}, equipment{0}, uses{0}, shots{0};
	std::atomic<bool> canCancel{true};
	std::exception_ptr senderError;
	std::thread sender([&] {
		try {
			pushKey(finished, SDLK_f, 0);
            pushKey(finished, SDLK_f, 1);
            pushKey(finished, SDLK_f, 0, SDL_KEYUP);
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
			yes != 1 || no != 1 || selections != 6 || cancellations != 1 || inspections != 1 || slots != 9 || transfers != 1 || equipment != 1 || uses != 1 || shots != 1) {
		std::cerr << "Space dispatch/repeat filtering failed: interactions="
			<< interactions << " navigation=" << navigation
			<< " acknowledgments=" << acknowledgments << " yes=" << yes
			<< " no=" << no << '\n';
		return 1;
	}
	std::cout << "SDL action dispatch and key-repeat filtering OK\n";
	return 0;
}
