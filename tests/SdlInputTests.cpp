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

namespace {

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
	std::atomic<bool> finished{false};
	std::atomic<int> interactions{0};
	std::atomic<int> navigation{0};
	std::exception_ptr senderError;
	std::thread sender([&] {
		try {
			pushKey(finished, SDLK_SPACE, 0);
			pushKey(finished, SDLK_SPACE, 1);
			pushKey(finished, SDLK_SPACE, 0, SDL_KEYUP);
			pushKey(finished, SDLK_SPACE, 0);
			pushKey(finished, SDLK_w, 0);
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
			if (std::holds_alternative<InteractionAction>(action))
				++interactions;
			else
				++navigation;
			return std::nullopt;
		});
	finished = true;
	sender.join();
	if (senderError)
		std::rethrow_exception(senderError);
	if (!result || interactions != 2 || navigation != 1) {
		std::cerr << "Space dispatch/repeat filtering failed: interactions="
			<< interactions << " navigation=" << navigation << '\n';
		return 1;
	}
	std::cout << "SDL Space dispatch and key-repeat filtering OK\n";
	return 0;
}
