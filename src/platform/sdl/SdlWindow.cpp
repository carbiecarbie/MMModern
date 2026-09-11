#include "platform/sdl/SdlWindow.h"

#define SDL_MAIN_HANDLED
#include <SDL.h>

#include <cstdint>
#include <exception>
#include <iostream>
#include <optional>
#include <vector>
#include <limits>
#include <stdexcept>

namespace mmodern {
namespace {

bool uploadFrame(SDL_Texture *texture, const IndexedFrame &frame,
		int expectedWidth, int expectedHeight, std::vector<std::uint32_t> &pixels) {
	if (!frame.isValid() || frame.width != expectedWidth || frame.height != expectedHeight) {
		std::cerr << "Invalid indexed framebuffer or changed dimensions.\n";
		return false;
	}
	pixels.resize(frame.pixels.size());
	for (std::size_t idx = 0; idx < frame.pixels.size(); ++idx) {
		const std::size_t paletteOffset = static_cast<std::size_t>(frame.pixels[idx]) * 3;
		pixels[idx] = 0xff000000U |
			(static_cast<std::uint32_t>(frame.palette[paletteOffset]) << 16) |
			(static_cast<std::uint32_t>(frame.palette[paletteOffset + 1]) << 8) |
			frame.palette[paletteOffset + 2];
	}
	if (SDL_UpdateTexture(texture, nullptr, pixels.data(),
			frame.width * static_cast<int>(sizeof(std::uint32_t))) != 0) {
		std::cerr << "SDL_UpdateTexture failed: " << SDL_GetError() << '\n';
		return false;
	}
	return true;
}

std::optional<PlayerAction> playerAction(const SDL_KeyboardEvent &key) {
	switch (key.keysym.sym) {
	case SDLK_PERIOD: return WaitAction{};
	case SDLK_b: return BlockAction{};
	case SDLK_F9: return SaveGameAction{};
	case SDLK_i: return InspectInventoryAction{};
	case SDLK_t: return TransferInventoryAction{};
	case SDLK_e: return EquipmentInventoryAction{};
	case SDLK_1: case SDLK_2: case SDLK_3: case SDLK_4: case SDLK_5:
	case SDLK_6: case SDLK_7: case SDLK_8: case SDLK_9:
		return SelectInventorySlotAction{static_cast<std::size_t>(key.keysym.sym - SDLK_1)};
	case SDLK_ESCAPE:
		return CancelInteractionAction{};
	case SDLK_F1:
	case SDLK_F2:
	case SDLK_F3:
	case SDLK_F4:
	case SDLK_F5:
	case SDLK_F6:
		return SelectMemberAction{static_cast<std::size_t>(key.keysym.sym - SDLK_F1)};
	case SDLK_a:
	case SDLK_LEFT:
		return NavigationAction::TurnLeft;
	case SDLK_d:
	case SDLK_RIGHT:
		return NavigationAction::TurnRight;
	case SDLK_w:
	case SDLK_UP:
		return NavigationAction::MoveForward;
	case SDLK_s:
	case SDLK_DOWN:
		return NavigationAction::MoveBackward;
	case SDLK_SPACE:
		return InteractionAction{};
	case SDLK_RETURN:
	case SDLK_KP_ENTER:
		return AcknowledgeAction{};
	case SDLK_y:
		return YesAction{};
	case SDLK_n:
		return NoAction{};
	default:
		return std::nullopt;
	}
}

bool showLoop(const IndexedFrame &initialFrame, const std::string &title,
		const SdlWindow::FrameUpdateHandler &handler,
		const std::function<bool()> &canCancelInteraction = {},
		const SdlWindow::IdleFrameHandler &idle = {},
		const std::function<std::string()> &status = {}) {
	bool success = false;
	struct CloseNotification {
		const SdlWindow::FrameUpdateHandler &handler;
		bool &success;
		~CloseNotification() {
			try { if (!success && handler.failed) handler.failed(); } catch (...) {}
			try { if (handler.closed) handler.closed(); } catch (...) {}
		}
	} close{handler, success};
	try {
	if (!initialFrame.isValid()) {
		std::cerr << "Invalid indexed framebuffer.\n";
		return false;
	}

	SDL_SetMainReady();
	if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_EVENTS) != 0) {
		std::cerr << "SDL_Init failed: " << SDL_GetError() << '\n';
		return false;
	}

	struct Resources {
		SDL_Window *window = nullptr;
		SDL_Renderer *renderer = nullptr;
		SDL_Texture *texture = nullptr;
		~Resources() {
			if (texture) SDL_DestroyTexture(texture);
			if (renderer) SDL_DestroyRenderer(renderer);
			if (window) SDL_DestroyWindow(window);
			SDL_Quit();
		}
	} resources;
	SDL_SetHint(SDL_HINT_RENDER_SCALE_QUALITY, "0");
	SDL_Window *window = resources.window = SDL_CreateWindow(title.c_str(), SDL_WINDOWPOS_CENTERED,
		SDL_WINDOWPOS_CENTERED, 960, 600,
		SDL_WINDOW_RESIZABLE | SDL_WINDOW_ALLOW_HIGHDPI);
	if (!window) {
		std::cerr << "SDL_CreateWindow failed: " << SDL_GetError() << '\n';
		return false;
	}
	SDL_SetWindowMinimumSize(window, initialFrame.width, initialFrame.height);

	SDL_Renderer *renderer = resources.renderer = SDL_CreateRenderer(window, -1,
		SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);
	if (!renderer)
		renderer = resources.renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_SOFTWARE);
	if (!renderer) {
		std::cerr << "SDL_CreateRenderer failed: " << SDL_GetError() << '\n';
		return false;
	}

	if (SDL_RenderSetLogicalSize(renderer, initialFrame.width, initialFrame.height) != 0 ||
			SDL_RenderSetIntegerScale(renderer, SDL_TRUE) != 0) {
		std::cerr << "Cannot configure SDL scaling: " << SDL_GetError() << '\n';
		return false;
	}

	SDL_Texture *texture = resources.texture = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_ARGB8888,
		SDL_TEXTUREACCESS_STREAMING, initialFrame.width, initialFrame.height);
	if (!texture) {
		std::cerr << "SDL_CreateTexture failed: " << SDL_GetError() << '\n';
		return false;
	}

#if SDL_VERSION_ATLEAST(2, 0, 12)
	SDL_SetTextureScaleMode(texture, SDL_ScaleModeNearest);
#endif
	SDL_SetTextureBlendMode(texture, SDL_BLENDMODE_NONE);
	std::vector<std::uint32_t> pixels;
	if (handler.frameCurrent && !handler.frameCurrent()) throw std::runtime_error("Stale initial frame handoff");
	success = uploadFrame(texture, initialFrame, initialFrame.width,
		initialFrame.height, pixels);
	bool running = success;
	if (running) {
		SDL_SetRenderDrawColor(renderer,0,0,0,255);
		SDL_RenderClear(renderer);
		if (SDL_RenderCopy(renderer,texture,nullptr,nullptr) != 0) { success=false; return false; }
		SDL_RenderPresent(renderer);
	}
	std::uint64_t cycle = 0;
	bool spaceDown = false, blockDown = false;
	std::uint32_t readyAt = SDL_GetTicks();
	std::optional<std::uint64_t> displayedInput = handler.displayedInput ? handler.displayedInput() : std::nullopt;
	while (running) {
		try {
			if (cycle == std::numeric_limits<std::uint64_t>::max()) throw std::overflow_error("SDL loop cycle overflow");
			if (handler.beginCycle) handler.beginCycle(++cycle);
		} catch (...) { success = false; break; }
		SDL_Event event;
		const auto batchInput = displayedInput;
		if (SDL_WaitEventTimeout(&event, 16)) {
			do {
				if (event.type == SDL_QUIT) {
					running = false;
				} else if (event.type == SDL_KEYUP) {
					if (event.key.keysym.sym == SDLK_SPACE) spaceDown = false;
					if (event.key.keysym.sym == SDLK_b) blockDown = false;
				} else if (event.type == SDL_KEYDOWN) {
					if (event.key.repeat != 0) continue;
					if (batchInput && (event.key.keysym.sym == SDLK_SPACE || event.key.keysym.sym == SDLK_b)) {
						auto &down = event.key.keysym.sym == SDLK_SPACE ? spaceDown : blockDown;
						const bool held = down; down = true;
						if (held || static_cast<std::int32_t>(event.key.timestamp-readyAt) <= 0) continue;
					}
					if (event.key.keysym.sym == SDLK_ESCAPE &&
							!(handler && canCancelInteraction && canCancelInteraction())) {
						running = false;
					} else if (event.key.repeat == 0 && handler) {
						const auto action = playerAction(event.key);
						if (action) {
							try {
								const auto nextFrame = batchInput && handler.withDisplayedInput ?
									handler.withDisplayedInput(*action,*batchInput) : handler(*action);
								if (handler.frameCurrent && !handler.frameCurrent()) throw std::runtime_error("Stale gameplay frame handoff");
								if (nextFrame && !uploadFrame(texture, *nextFrame,
										initialFrame.width, initialFrame.height, pixels)) {
									success = false;
									running = false;
								}
							} catch (const std::exception &error) {
								std::cerr << "Scene update failed: " << error.what() << '\n';
								success = false;
								running = false;
							}
						}
					}
				}
			} while (running && SDL_PollEvent(&event));
		}
		if (!running) break;
		if (idle) {
			try {
				const auto nextFrame = idle();
				if (handler.frameCurrent && !handler.frameCurrent()) throw std::runtime_error("Stale idle frame handoff");
				if (nextFrame && !uploadFrame(texture, *nextFrame,
						initialFrame.width, initialFrame.height, pixels)) {
					success = false;
					break;
				}
			} catch (const std::exception &error) {
				std::cerr << "Presentation update failed: " << error.what() << '\n';
				success = false;
				break;
			}
		}

		try {
			if (status) {
				const auto title = status();
				if (handler.frameCurrent && !handler.frameCurrent()) throw std::runtime_error("Stale status callback");
				SDL_SetWindowTitle(window, title.c_str());
			}
		} catch (...) { success = false; break; }
		SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
		SDL_RenderClear(renderer);
		if (SDL_RenderCopy(renderer, texture, nullptr, nullptr) != 0) {
			std::cerr << "SDL_RenderCopy failed: " << SDL_GetError() << '\n';
			success = false;
			break;
		}
		SDL_RenderPresent(renderer);
		const auto nextInput = handler.displayedInput ? handler.displayedInput() : std::nullopt;
		if (nextInput != displayedInput) readyAt = SDL_GetTicks();
		displayedInput = nextInput;
	}

	return success;
	} catch (const std::exception &error) {
		success = false;
		std::cerr << "SDL presentation failed: " << error.what() << '\n';
		return false;
	} catch (...) { success = false; return false; }
}

} // namespace

bool SdlWindow::show(const IndexedFrame &frame, const std::string &title) const {
	return showLoop(frame, title, FrameUpdateHandler{});
}

bool SdlWindow::showInteractive(const IndexedFrame &frame, const std::string &title,
		const FrameUpdateHandler &handler, const std::function<bool()> &handlesEscape,
		const IdleFrameHandler &idle, const std::function<std::string()> &status) const {
	return showLoop(frame, title, handler, handlesEscape, idle, status);
}

} // namespace mmodern
