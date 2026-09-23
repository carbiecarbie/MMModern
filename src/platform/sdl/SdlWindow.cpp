#include "platform/sdl/SdlWindow.h"

#define SDL_MAIN_HANDLED
#include <SDL.h>

#include <cstdint>
#include <array>
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
	case SDLK_f: return ShootAction{};
	case SDLK_c: return CastSpellAction{};
	case SDLK_r: return RevisitCompletedAction{};
	case SDLK_F9: return SaveGameAction{};
	case SDLK_i: return InspectInventoryAction{};
	case SDLK_t: return TransferInventoryAction{};
	case SDLK_e: return EquipmentInventoryAction{};
	case SDLK_u: return UseItemAction{};
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

bool showLoop(const IndexedFrame &suppliedInitial, const std::string &title,
		const SdlWindow::FrameUpdateHandler &handler,
		const std::function<bool()> &canCancelInteraction = {},
		const SdlWindow::IdleFrameHandler &idle = {},
		const std::function<std::string()> &status = {}) {
	const auto initialBinding = suppliedInitial.presentation();
	const auto &initialFrame = initialBinding ? *initialBinding : suppliedInitial;
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
	IndexedFrame::Presentation uploadedFrame, presentedFrame;
	std::optional<std::uint64_t> uploadedInput;
	bool uploaded = false;
	const auto accepts = [&](const IndexedFrame::Presentation &frame) {
		return !handler.acceptsFrame || handler.acceptsFrame(frame);
	};
	// All three upload paths use the content and identity supplied together.
	// Stale/wrong-owner frames are ignored without releasing the live boundary.
	const auto upload = [&](const IndexedFrame &supplied) {
		const auto bound = supplied.presentation();
		if (!accepts(bound)) return true;
		const auto &content = bound ? *bound : supplied;
		if (!uploadFrame(texture, content, initialFrame.width, initialFrame.height, pixels)) return false;
		uploadedFrame = bound;
		uploaded = true;
		uploadedInput = handler.displayedInput ? handler.displayedInput() : std::nullopt;
		return true;
	};
	success = upload(suppliedInitial);
	bool running = success;
	if (running && uploaded && accepts(uploadedFrame)) {
		SDL_SetRenderDrawColor(renderer,0,0,0,255);
		SDL_RenderClear(renderer);
		if (SDL_RenderCopy(renderer,texture,nullptr,nullptr) != 0) { success=false; return false; }
		SDL_RenderPresent(renderer);
		if (handler.frameCurrent && !handler.frameCurrent()) throw std::runtime_error("Stale initial upload");
		if (handler.framePresented) handler.framePresented(uploadedFrame);
		presentedFrame = uploadedFrame;
		uploadedInput = handler.displayedInput ? handler.displayedInput() : std::nullopt;
	}
	std::uint64_t cycle = 0;
	bool spaceDown = false, blockDown = false, revisitDown = false, inspectDown = false;
	std::array<bool,SDL_NUM_SCANCODES> journeyKeys{};
	std::uint32_t readyAt = SDL_GetTicks();
	const auto retireQueuedKeys = [&] {
		// Fence keys already sampled before a semantic presentation boundary.
		// Preserve queue order/key-up processing and allow genuinely new keys
		// sampled in the same millisecond as the newly presented frame.
		SDL_PumpEvents();
		SDL_FilterEvents([](void *context, SDL_Event *queued) -> int {
			if (queued->type == SDL_KEYDOWN)
				queued->key.timestamp = *static_cast<std::uint32_t *>(context) - 1;
			return 1;
		}, &readyAt);
	};
	retireQueuedKeys();
	std::optional<std::uint64_t> displayedInput = uploaded && accepts(uploadedFrame) && handler.displayedInput ? handler.displayedInput() : std::nullopt;
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
					const auto scan = SDL_GetScancodeFromKey(event.key.keysym.sym);
					if (scan > SDL_SCANCODE_UNKNOWN && scan < SDL_NUM_SCANCODES) journeyKeys[scan] = false;
					if (event.key.keysym.sym == SDLK_SPACE) spaceDown = false;
					if (event.key.keysym.sym == SDLK_b) blockDown = false;
					if (event.key.keysym.sym == SDLK_r) revisitDown = false;
					if (event.key.keysym.sym == SDLK_i) inspectDown = false;
				} else if (event.type == SDL_KEYDOWN) {
					if (event.key.repeat != 0) continue;
					if (handler.protectAllKeys && playerAction(event.key)) {
						const auto scan = SDL_GetScancodeFromKey(event.key.keysym.sym);
						if (scan <= SDL_SCANCODE_UNKNOWN || scan >= SDL_NUM_SCANCODES) continue;
						const bool held = journeyKeys[scan]; journeyKeys[scan] = true;
						if (held || static_cast<std::int32_t>(event.key.timestamp-readyAt) < 0) continue;
					} else if (batchInput && (event.key.keysym.sym == SDLK_SPACE || event.key.keysym.sym == SDLK_b ||
						event.key.keysym.sym == SDLK_r || event.key.keysym.sym == SDLK_i)) {
						auto &down = event.key.keysym.sym == SDLK_SPACE ? spaceDown : event.key.keysym.sym == SDLK_b ? blockDown :
							event.key.keysym.sym == SDLK_r ? revisitDown : inspectDown;
						const bool held = down; down = true;
						if (held || static_cast<std::int32_t>(event.key.timestamp-readyAt) < 0) continue;
					}
					// Exit must obey the same semantic presentation fence as gameplay.
					if (handler.protectAllKeys && event.key.keysym.sym == SDLK_ESCAPE &&
						(batchInput != (handler.displayedInput ? handler.displayedInput() : std::nullopt) ||
						 !uploaded || !accepts(uploadedFrame) || uploadedFrame != presentedFrame)) continue;
					if (event.key.keysym.sym == SDLK_ESCAPE &&
							!(handler && canCancelInteraction && canCancelInteraction())) {
						running = false;
					} else if (event.key.repeat == 0 && handler) {
						const auto action = playerAction(event.key);
						if (action && (!handler.acceptsFrame || (uploaded && accepts(uploadedFrame)))) {
							try {
								const auto nextFrame = batchInput && handler.withDisplayedInput ?
									handler.withDisplayedInput(*action,*batchInput) : handler(*action);
								if (handler.frameCurrent && !handler.frameCurrent()) throw std::runtime_error("Stale gameplay frame handoff");
								if (nextFrame && !upload(*nextFrame)) {
									success = false; running = false;
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
				if (nextFrame && !upload(*nextFrame)) {
					success = false; break;
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
		if (!uploaded || !accepts(uploadedFrame)) continue;
		SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
		SDL_RenderClear(renderer);
		if (SDL_RenderCopy(renderer, texture, nullptr, nullptr) != 0) {
			std::cerr << "SDL_RenderCopy failed: " << SDL_GetError() << '\n';
			success = false;
			break;
		}
		SDL_RenderPresent(renderer);
		if (handler.frameCurrent && !handler.frameCurrent()) throw std::runtime_error("Stale presented frame");
		if (uploadedInput != (handler.displayedInput ? handler.displayedInput() : std::nullopt))
			throw std::runtime_error("Current frame was not uploaded");
		if (handler.framePresented) handler.framePresented(uploadedFrame);
		presentedFrame = uploadedFrame;
		uploadedInput = handler.displayedInput ? handler.displayedInput() : std::nullopt;
		const auto nextInput = handler.displayedInput ? handler.displayedInput() : std::nullopt;
		if (nextInput != displayedInput) { readyAt = SDL_GetTicks(); retireQueuedKeys(); }
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
