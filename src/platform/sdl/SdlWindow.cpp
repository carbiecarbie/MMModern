#include "platform/sdl/SdlWindow.h"

#define SDL_MAIN_HANDLED
#include <SDL.h>

#include <cstdint>
#include <exception>
#include <iostream>
#include <optional>
#include <vector>

namespace mmodern {
namespace {

bool uploadFrame(SDL_Texture *texture, const IndexedFrame &frame,
		int expectedWidth, int expectedHeight, std::vector<std::uint32_t> &pixels) {
	if (!frame.isValid() || frame.width != expectedWidth || frame.height != expectedHeight) {
		std::cerr << "Framebuffer indexado invalido ou com dimensoes alteradas.\n";
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
		std::cerr << "SDL_UpdateTexture falhou: " << SDL_GetError() << '\n';
		return false;
	}
	return true;
}

std::optional<PlayerAction> playerAction(const SDL_KeyboardEvent &key) {
	switch (key.keysym.sym) {
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
	if (!initialFrame.isValid()) {
		std::cerr << "Framebuffer indexado invalido.\n";
		return false;
	}

	SDL_SetMainReady();
	if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_EVENTS) != 0) {
		std::cerr << "SDL_Init falhou: " << SDL_GetError() << '\n';
		return false;
	}

	SDL_SetHint(SDL_HINT_RENDER_SCALE_QUALITY, "0");
	SDL_Window *window = SDL_CreateWindow(title.c_str(), SDL_WINDOWPOS_CENTERED,
		SDL_WINDOWPOS_CENTERED, 960, 600,
		SDL_WINDOW_RESIZABLE | SDL_WINDOW_ALLOW_HIGHDPI);
	if (!window) {
		std::cerr << "SDL_CreateWindow falhou: " << SDL_GetError() << '\n';
		SDL_Quit();
		return false;
	}
	SDL_SetWindowMinimumSize(window, initialFrame.width, initialFrame.height);

	SDL_Renderer *renderer = SDL_CreateRenderer(window, -1,
		SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);
	if (!renderer)
		renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_SOFTWARE);
	if (!renderer) {
		std::cerr << "SDL_CreateRenderer falhou: " << SDL_GetError() << '\n';
		SDL_DestroyWindow(window);
		SDL_Quit();
		return false;
	}

	if (SDL_RenderSetLogicalSize(renderer, initialFrame.width, initialFrame.height) != 0 ||
			SDL_RenderSetIntegerScale(renderer, SDL_TRUE) != 0) {
		std::cerr << "Nao foi possivel configurar a escala SDL: " << SDL_GetError() << '\n';
		SDL_DestroyRenderer(renderer);
		SDL_DestroyWindow(window);
		SDL_Quit();
		return false;
	}

	SDL_Texture *texture = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_ARGB8888,
		SDL_TEXTUREACCESS_STREAMING, initialFrame.width, initialFrame.height);
	if (!texture) {
		std::cerr << "SDL_CreateTexture falhou: " << SDL_GetError() << '\n';
		SDL_DestroyRenderer(renderer);
		SDL_DestroyWindow(window);
		SDL_Quit();
		return false;
	}

#if SDL_VERSION_ATLEAST(2, 0, 12)
	SDL_SetTextureScaleMode(texture, SDL_ScaleModeNearest);
#endif
	SDL_SetTextureBlendMode(texture, SDL_BLENDMODE_NONE);
	std::vector<std::uint32_t> pixels;
	bool success = uploadFrame(texture, initialFrame, initialFrame.width,
		initialFrame.height, pixels);
	bool running = success;
	while (running) {
		SDL_Event event;
		if (SDL_WaitEventTimeout(&event, 16)) {
			do {
				if (event.type == SDL_QUIT) {
					running = false;
				} else if (event.type == SDL_KEYDOWN) {
					if (event.key.repeat != 0) continue;
					if (event.key.keysym.sym == SDLK_ESCAPE &&
							!(handler && canCancelInteraction && canCancelInteraction())) {
						running = false;
					} else if (event.key.repeat == 0 && handler) {
						const auto action = playerAction(event.key);
						if (action) {
							try {
								const auto nextFrame = handler(*action);
								if (nextFrame && !uploadFrame(texture, *nextFrame,
										initialFrame.width, initialFrame.height, pixels)) {
									success = false;
									running = false;
								}
							} catch (const std::exception &error) {
								std::cerr << "Falha ao atualizar a cena: " << error.what() << '\n';
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
				if (nextFrame && !uploadFrame(texture, *nextFrame,
						initialFrame.width, initialFrame.height, pixels)) {
					success = false;
					break;
				}
			} catch (const std::exception &error) {
				std::cerr << "Falha ao atualizar apresentacao: " << error.what() << '\n';
				success = false;
				break;
			}
		}

		if (status) SDL_SetWindowTitle(window, status().c_str());
		SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
		SDL_RenderClear(renderer);
		if (SDL_RenderCopy(renderer, texture, nullptr, nullptr) != 0) {
			std::cerr << "SDL_RenderCopy falhou: " << SDL_GetError() << '\n';
			success = false;
			break;
		}
		SDL_RenderPresent(renderer);
	}

	SDL_DestroyTexture(texture);
	SDL_DestroyRenderer(renderer);
	SDL_DestroyWindow(window);
	SDL_Quit();
	return success;
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
