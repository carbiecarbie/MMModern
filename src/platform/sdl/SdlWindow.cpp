#include "platform/sdl/SdlWindow.h"
#include "platform/sdl/XeenMainScreenInput.h"

#define SDL_MAIN_HANDLED
#include <SDL.h>

#include <cstdint>
#include <array>
#include <deque>
#include <algorithm>
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

std::optional<PlayerAction> playerAction(const SDL_KeyboardEvent &key, MainScreen screen) {
    if((key.keysym.mod & KMOD_CTRL) && (key.keysym.sym==SDLK_LEFT || key.keysym.sym==SDLK_RIGHT))
        return UnsupportedMainScreenAction{"Strafe"};
    if((key.keysym.mod & KMOD_CTRL) && key.keysym.sym==SDLK_DOWN)
        return UnsupportedMainScreenAction{"Turn around"};
    if((key.keysym.mod & KMOD_CTRL) && key.keysym.sym==SDLK_UP)return std::nullopt;
	switch (key.keysym.sym) {
	case SDLK_PERIOD: return WaitAction{};
	case SDLK_b: return BlockAction{};
	case SDLK_f: return screen==MainScreen::Combat ? std::optional<PlayerAction>{UnsupportedMainScreenAction{"Quick Fight"}} : std::nullopt;
	case SDLK_s: return screen==MainScreen::Combat ? std::nullopt : std::optional<PlayerAction>{ShootAction{}};
	case SDLK_a: return screen==MainScreen::Combat ? std::optional<PlayerAction>{AttackAction{}} : std::nullopt;
	case SDLK_c: return CastSpellAction{};
	case SDLK_r: return RevisitCompletedAction{};
	case SDLK_F9: return SaveGameAction{};
	case SDLK_i: return UnsupportedMainScreenAction{"Info"};
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
		return xeenMainScreenMemberKey(InputKey::F1+key.keysym.sym-SDLK_F1);
	case SDLK_LEFT:
		return NavigationAction::TurnLeft;
	case SDLK_RIGHT:
		return NavigationAction::TurnRight;
	case SDLK_UP:
		return NavigationAction::MoveForward;
	case SDLK_DOWN:
		return NavigationAction::MoveBackward;
	case SDLK_SPACE:
		return screen==MainScreen::Combat ? std::nullopt : std::optional<PlayerAction>{InteractionAction{}};
	case SDLK_RETURN:
	case SDLK_KP_ENTER:
		return screen==MainScreen::Combat ? std::nullopt : std::optional<PlayerAction>{AcknowledgeAction{}};
	case SDLK_y:
		return YesAction{};
	case SDLK_n:
		return NoAction{};
	default:
		return std::nullopt;
	}
}

unsigned inputKey(SDL_Keycode key) {
    if(key>=SDLK_F1 && key<=SDLK_F6) return InputKey::F1+key-SDLK_F1;
    switch(key) {
    case SDLK_UP: case SDLK_KP_8: return InputKey::Up;
    case SDLK_DOWN: case SDLK_KP_2: return InputKey::Down;
    case SDLK_LEFT: case SDLK_KP_4: return InputKey::Left;
    case SDLK_RIGHT: case SDLK_KP_6: return InputKey::Right;
    case SDLK_KP_ENTER: return InputKey::Enter;
    default: return static_cast<unsigned>(key);
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
        SDL_Texture *cursor = nullptr;
        int priorCursor=SDL_ENABLE;
		~Resources() {
            if(cursor) { SDL_DestroyTexture(cursor); SDL_ShowCursor(priorCursor); }
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
    int cursorWidth=0,cursorHeight=0;
    if(handler.cursorImage) {
        const auto cursor=handler.cursorImage();
        if(!cursor.isValid())throw std::runtime_error("Invalid original cursor");
        cursorWidth=cursor.width;cursorHeight=cursor.height;
        resources.cursor=SDL_CreateTexture(renderer,SDL_PIXELFORMAT_ARGB8888,SDL_TEXTUREACCESS_STATIC,cursorWidth,cursorHeight);
        if(!resources.cursor)throw std::runtime_error("Cannot create original cursor texture");
        std::vector<std::uint32_t> cursorPixels(cursor.pixels.size());
        for(std::size_t i=0;i<cursorPixels.size();++i) {
            const auto c=cursor.pixels[i];cursorPixels[i]=c ? 0xff000000u | (std::uint32_t(cursor.palette[3*c])<<16) | (std::uint32_t(cursor.palette[3*c+1])<<8) | cursor.palette[3*c+2] : 0;
        }
        if(SDL_UpdateTexture(resources.cursor,nullptr,cursorPixels.data(),cursorWidth*4)!=0)throw std::runtime_error("Cannot upload original cursor");
        SDL_SetTextureBlendMode(resources.cursor,SDL_BLENDMODE_BLEND);
        resources.priorCursor=SDL_ShowCursor(SDL_QUERY);SDL_ShowCursor(SDL_DISABLE);
    }
    const auto drawCursor=[&] {
        if(!resources.cursor || SDL_GetMouseFocus()!=window)return;
        int x,y;SDL_GetMouseState(&x,&y);float logicalX,logicalY;
        SDL_RenderWindowToLogical(renderer,x,y,&logicalX,&logicalY);
        if(logicalX<0 || logicalY<0 || logicalX>=initialFrame.width || logicalY>=initialFrame.height)return;
        SDL_Rect destination{int(logicalX),int(logicalY),cursorWidth,cursorHeight};
        if(SDL_RenderCopy(renderer,resources.cursor,nullptr,&destination)!=0)throw std::runtime_error("Original cursor presentation failed");
    };
	std::vector<std::uint32_t> pixels;
	if (handler.frameCurrent && !handler.frameCurrent()) throw std::runtime_error("Stale initial frame handoff");
	IndexedFrame::Presentation uploadedFrame, presentedFrame;
	std::optional<std::uint64_t> uploadedInput;
	bool uploaded = false;
	IndexedFrame displayContent;
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
		displayContent=content;
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
		drawCursor();SDL_RenderPresent(renderer);
		if (handler.frameCurrent && !handler.frameCurrent()) throw std::runtime_error("Stale initial upload");
		// Sample pending OS input while this handoff is still unacquired. Never
		// pump between successful acquisition and the queue fence below: that
		// would collect and retire a genuinely fresh press under the new frame.
		SDL_PumpEvents();
		if (handler.framePresented) handler.framePresented(uploadedFrame);
		if (handler.completeInputHandoff) handler.completeInputHandoff(uploadedFrame);
		presentedFrame = uploadedFrame;
		uploadedInput = handler.displayedInput ? handler.displayedInput() : std::nullopt;
	}
	std::uint64_t cycle = 0;
	bool spaceDown = false, blockDown = false, revisitDown = false, inspectDown = false;
	std::array<bool,SDL_NUM_SCANCODES> journeyKeys{};
	std::uint32_t readyAt = SDL_GetTicks();
    struct PendingAction { PlayerAction action; std::uint64_t context; bool repeat; std::optional<InputButton> button; };
    std::deque<PendingAction> pendingActions;
    bool actionUsed = false;
    const auto contextFor = [&](const IndexedFrame::Presentation &origin) {
        return handler.inputContext ? handler.inputContext(origin) : InputContext{};
    };
    auto acquiredContext = contextFor(presentedFrame);
    const auto synchronizeQueue = [&](const InputContext &context) {
        if (!context.acceptsQueuedInput || (!pendingActions.empty() && pendingActions.front().context != context.contextId))
            pendingActions.clear();
    };
    const auto retireQueuedKeys = [&] {
        // The strict path retains the pre-acquisition timestamp fence.
        if (contextFor(presentedFrame).acceptsQueuedInput) return;
        SDL_FilterEvents([](void *context, SDL_Event *queued) -> int {
            if (queued->type == SDL_KEYDOWN)
                queued->key.timestamp = *static_cast<std::uint32_t *>(context) - 1;
            if (queued->type == SDL_MOUSEBUTTONDOWN)
                queued->button.timestamp = *static_cast<std::uint32_t *>(context) - 1;
            return 1;
        }, &readyAt);
    };
    retireQueuedKeys();
    std::optional<std::uint64_t> displayedInput = uploaded && accepts(uploadedFrame) && handler.displayedInput ? handler.displayedInput() : std::nullopt;
    const auto inputCurrent = [&](const IndexedFrame::Presentation &origin) {
        return handler.acceptsInputFrame ? handler.acceptsInputFrame(origin) :
            uploaded && uploadedFrame == origin && accepts(origin);
    };
    const auto feedback = [&](const InputButton &button,const IndexedFrame::Presentation &origin) {
        if(handler.drawButton && inputCurrent(origin)) {
            // ButtonContainer::checkEvents shows frame | 1, waits two
            // presentation ticks, then restores before returning the key.
            // No gameplay callback, idle update, input pump, semantic upload or
            // frame acquisition occurs during this native display effect.
            auto pressed=displayContent;
            handler.drawButton(pressed,button);
            if(!inputCurrent(origin) || (handler.frameCurrent && !handler.frameCurrent()))
                throw std::runtime_error("Stale button feedback origin");
            const auto presentButton=[&](const IndexedFrame &frame) {
                if(!uploadFrame(texture,frame,initialFrame.width,initialFrame.height,pixels))
                    throw std::runtime_error("Button feedback upload failed");
                SDL_SetRenderDrawColor(renderer,0,0,0,255);SDL_RenderClear(renderer);
                if(SDL_RenderCopy(renderer,texture,nullptr,nullptr)!=0)
                    throw std::runtime_error("Button feedback presentation failed");
                drawCursor();SDL_RenderPresent(renderer);
            };
            presentButton(pressed);
            SDL_Delay(kButtonFeedbackMilliseconds);
            presentButton(displayContent);
            if(!inputCurrent(origin) || (handler.frameCurrent && !handler.frameCurrent()))
                throw std::runtime_error("Stale button feedback restoration");
        }
    };
    const auto deliver = [&](const PlayerAction &action, const std::optional<std::uint64_t> &input,
            const IndexedFrame::Presentation &origin,const std::optional<InputButton> &button=std::nullopt) {
        try {
            actionUsed = true;
            if(button) feedback(*button,origin);
            const auto nextFrame = input && handler.withPresentedInput ?
                handler.withPresentedInput(action,*input,origin) :
                input && handler.withDisplayedInput ? handler.withDisplayedInput(action,*input) : handler(action);
            if (handler.frameCurrent && !handler.frameCurrent()) throw std::runtime_error("Stale gameplay frame handoff");
            if (nextFrame && !upload(*nextFrame)) { success = false; running = false; }
        } catch (const std::exception &error) {
            std::cerr << "Scene update failed: " << error.what() << '\n';
            success = false; running = false;
        }
    };
    const auto dispatchEvent = [&](const SDL_Event &event, const std::optional<std::uint64_t> &batchInput,
            const IndexedFrame::Presentation &batchFrame, const InputContext &batchContext) {
        if (event.type == SDL_QUIT) {
            pendingActions.clear(); running = false;
        } else if (event.type == SDL_WINDOWEVENT && event.window.event == SDL_WINDOWEVENT_FOCUS_LOST) {
            journeyKeys.fill(false);
            spaceDown = blockDown = revisitDown = inspectDown = false;
            pendingActions.clear();
        } else if (event.type == SDL_KEYUP) {
            const auto scan = SDL_GetScancodeFromKey(event.key.keysym.sym);
            if (scan > SDL_SCANCODE_UNKNOWN && scan < SDL_NUM_SCANCODES) journeyKeys[scan] = false;
            if (event.key.keysym.sym == SDLK_SPACE) spaceDown = false;
            if (event.key.keysym.sym == SDLK_b) blockDown = false;
            if (event.key.keysym.sym == SDLK_r) revisitDown = false;
            if (event.key.keysym.sym == SDLK_i) inspectDown = false;
        } else if (event.type == SDL_KEYDOWN || event.type == SDL_MOUSEBUTTONDOWN) {
            const bool mouse = event.type == SDL_MOUSEBUTTONDOWN;
            const auto context = contextFor(batchFrame);
            synchronizeQueue(context);
            if (mouse && (event.button.button != SDL_BUTTON_LEFT || (!context.dialog && context.mainScreen == MainScreen::None) ||
                initialFrame.width != 320 || initialFrame.height != 200)) return;
            // SDL_RenderSetLogicalSize transforms native mouse events to framebuffer
            // coordinates, including integer scaling, letterboxing and high DPI.
            // Do not scale a second time. Out-of-viewport coordinates are rejected.
            const auto action = context.dialog ? (mouse ? context.dialog->click(event.button.x,event.button.y) :
                context.dialog->key(inputKey(event.key.keysym.sym))) :
                mouse ? xeenMainScreenClick(event.button.x,event.button.y,context.mainScreen) : playerAction(event.key,context.mainScreen);
            auto button=context.dialog ?
                (action && std::get_if<DialogKeyAction>(&*action) ? context.dialog->button(std::get<DialogKeyAction>(*action).key) : std::nullopt) :
                mouse ? xeenMainScreenButtonAt(event.button.x,event.button.y,context.mainScreen) :
                    xeenMainScreenKeyButton(action,inputKey(event.key.keysym.sym),context.mainScreen);
            if(!mouse && !context.dialog && (event.key.keysym.mod & KMOD_CTRL)) {
                button=event.key.keysym.sym==SDLK_LEFT ? xeenMainScreenButtonAt(235,169,context.mainScreen) :
                    event.key.keysym.sym==SDLK_RIGHT ? xeenMainScreenButtonAt(286,169,context.mainScreen) : std::nullopt;
            }
            if (!mouse && event.key.keysym.sym == SDLK_ESCAPE && !event.key.repeat) pendingActions.clear();
            const bool movement = action && std::holds_alternative<NavigationAction>(*action);
            const auto *slot = action ? std::get_if<SelectInventorySlotAction>(&*action) : nullptr;
            const bool queueKey = action && (movement || std::holds_alternative<AttackAction>(*action) || std::holds_alternative<InteractionAction>(*action) ||
                std::holds_alternative<BlockAction>(*action) || std::holds_alternative<ShootAction>(*action) ||
                std::holds_alternative<RevisitCompletedAction>(*action) || std::holds_alternative<WaitAction>(*action) ||
                std::holds_alternative<CastSpellAction>(*action) || std::holds_alternative<SelectMemberAction>(*action) ||
                std::holds_alternative<UnsupportedMainScreenAction>(*action) || (slot && slot->slot < 3));
            const bool queueable = context.acceptsQueuedInput && queueKey;
            const auto scan = mouse ? SDL_SCANCODE_UNKNOWN : SDL_GetScancodeFromKey(event.key.keysym.sym);
            if (queueable) {
                // A context transition within this event batch cannot relabel old keys.
                if (context.contextId != batchContext.contextId) return;
                if (mouse) {
                    if (pendingActions.size() < 5) pendingActions.push_back({*action,context.contextId,false,button});
                    return;
                }
                if (scan <= SDL_SCANCODE_UNKNOWN || scan >= SDL_NUM_SCANCODES) return;
                const bool held = journeyKeys[scan];
                if (!event.key.repeat) journeyKeys[scan] = true;
                if (event.key.repeat) {
                    if (!movement || !held || std::any_of(pendingActions.begin(),pendingActions.end(),
                        [](const auto &entry) { return entry.repeat; })) return;
                } else if (held) return;
                if (pendingActions.size() < 5) pendingActions.push_back({*action,context.contextId,event.key.repeat != 0,button});
                return;
            }
            if(context.dialog && (!context.readyForAction || !inputCurrent(batchFrame) || context.contextId!=batchContext.contextId)) return;
            if(mouse) {
                if(context.dialog && context.readyForAction && inputCurrent(batchFrame) &&
                    context.contextId==batchContext.contextId && static_cast<std::int32_t>(event.button.timestamp-readyAt)>=0 && action)
                    deliver(*action,batchInput,batchFrame,button);
                return;
            }
            if (event.key.repeat != 0) return;
            if ((handler.protectAllKeys || context.dialog) && (action || (button && handler.drawButton))) {
                if (scan <= SDL_SCANCODE_UNKNOWN || scan >= SDL_NUM_SCANCODES) return;
                const bool held = journeyKeys[scan]; journeyKeys[scan] = true;
                if (held || static_cast<std::int32_t>(event.key.timestamp-readyAt) < 0) return;
            } else if (batchInput && (event.key.keysym.sym == SDLK_SPACE || event.key.keysym.sym == SDLK_b ||
                event.key.keysym.sym == SDLK_r || event.key.keysym.sym == SDLK_i)) {
                auto &down = event.key.keysym.sym == SDLK_SPACE ? spaceDown : event.key.keysym.sym == SDLK_b ? blockDown :
                    event.key.keysym.sym == SDLK_r ? revisitDown : inspectDown;
                const bool held = down; down = true;
                if (held || static_cast<std::int32_t>(event.key.timestamp-readyAt) < 0) return;
            }
            if (handler.withPresentedInput && static_cast<std::int32_t>(event.key.timestamp-readyAt) < 0) return;
            if (handler.protectAllKeys && event.key.keysym.sym == SDLK_ESCAPE &&
                (batchInput != (handler.displayedInput ? handler.displayedInput() : std::nullopt) || !inputCurrent(batchFrame))) return;
            if (event.key.keysym.sym == SDLK_ESCAPE && !(handler && canCancelInteraction && canCancelInteraction())) {
                running = false;
            } else if (handler && action && ((!handler.acceptsFrame && !handler.withPresentedInput) || inputCurrent(batchFrame))) {
                deliver(*action,batchInput,batchFrame,button);
            } else if(!action && button && handler.drawButton && context.readyForAction && inputCurrent(batchFrame)) {
                // Cosmetic response only: no PlayerAction is created or queued.
                try {feedback(*button,batchFrame);}
                catch(const std::exception &error) {
                    std::cerr<<"Button feedback failed: "<<error.what()<<'\n';success=false;running=false;
                }
            }
        }
    };
	while (running) {
		try {
			if (cycle == std::numeric_limits<std::uint64_t>::max()) throw std::overflow_error("SDL loop cycle overflow");
			if (handler.beginCycle) handler.beginCycle(++cycle);
		} catch (...) { success = false; break; }
		SDL_Event event;
		const auto batchInput = displayedInput;
		const auto batchFrame = presentedFrame;
		const auto batchContext = contextFor(batchFrame);
		synchronizeQueue(batchContext);
		if (SDL_WaitEventTimeout(&event, 16)) {
			do {
				dispatchEvent(event,batchInput,batchFrame,batchContext);
			} while (running && SDL_PollEvent(&event));
		}
		if (!running) break;
        const auto drainContext = contextFor(presentedFrame);
        synchronizeQueue(drainContext);
        if (!actionUsed && handler && !pendingActions.empty() && drainContext.acceptsQueuedInput &&
            drainContext.readyForAction && inputCurrent(presentedFrame)) {
            const auto action = pendingActions.front().action;
            const auto button = pendingActions.front().button;
            pendingActions.pop_front(); // A ready refusal is consumed, never retried.
            deliver(action,displayedInput,presentedFrame,button);
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
		drawCursor();SDL_RenderPresent(renderer);
		if (handler.frameCurrent && !handler.frameCurrent()) throw std::runtime_error("Stale presented frame");
		if (uploadedInput != (handler.displayedInput ? handler.displayedInput() : std::nullopt))
			throw std::runtime_error("Current frame was not uploaded");
		SDL_PumpEvents();
		const auto acquired = uploadedFrame;
		if (handler.framePresented) handler.framePresented(acquired);
		const bool cosmetic = acquired != presentedFrame && handler.acceptsInputFrame &&
			handler.completeInputHandoff && handler.acceptsInputFrame(presentedFrame);
		if (cosmetic) {
			// The queue cut is the input handoff's linearization point. B is fully
			// acquired; the bounded existing batch still belongs to live A. Never
			// pump or relabel it as B. Keys arriving after this cut belong to B.
			struct Batch { std::vector<SDL_Event> events; bool failed = false; } pending;
			SDL_FilterEvents([](void *context, SDL_Event *queued) -> int {
				auto &batch = *static_cast<Batch *>(context);
				try { batch.events.push_back(*queued); } catch (...) { batch.failed = true; }
				return 0;
			}, &pending);
			if (pending.failed) throw std::runtime_error("Cannot retain acquired input batch");
			// Preserve non-key event order too: a queued close precedes later keys.
			for (const auto &event : pending.events) {
				if (!running) break;
				dispatchEvent(event,displayedInput,presentedFrame,acquiredContext);
			}
			if (!running) break;
			// An A response can supersede B. Its semantic successor must pass
			// the ordinary strict pre-acquisition fence before accepting input.
			if (uploadedFrame != acquired || !accepts(acquired)) continue;
		}
		if (handler.completeInputHandoff) handler.completeInputHandoff(acquired);
		const bool concreteChanged = presentedFrame != uploadedFrame;
		presentedFrame = uploadedFrame;
		uploadedInput = handler.displayedInput ? handler.displayedInput() : std::nullopt;
		const auto nextInput = handler.displayedInput ? handler.displayedInput() : std::nullopt;
		if (!cosmetic && (concreteChanged || nextInput != displayedInput)) { readyAt = SDL_GetTicks(); retireQueuedKeys(); }
        const auto nextContext = contextFor(presentedFrame);
        synchronizeQueue(nextContext);
        if (handler.inputContext && acquiredContext.contextId != nextContext.contextId) {
            // clearEvents boundary: retain releases and non-input events in order.
            SDL_FilterEvents([](void *, SDL_Event *queued) -> int {
                return queued->type != SDL_KEYDOWN && queued->type != SDL_MOUSEBUTTONDOWN;
            },nullptr);
        }
        acquiredContext = nextContext;
        actionUsed = false;
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
