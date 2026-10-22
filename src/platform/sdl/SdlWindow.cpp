#include "platform/sdl/SdlWindow.h"
#include "platform/sdl/SdlAudioDevice.h"
#include "platform/sdl/SdlWindowPlacement.h"
#include "platform/sdl/XeenMainScreenInput.h"
#include "core/PresentationClock.h"

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

// Maintainer-approved IBM enhanced-keyboard fallback (F3 2Bh): 500 ms,
// then (8 + 3) * 2 / 240 seconds. Independent of host OS repeat settings.
constexpr std::uint64_t navigationRepeatDelay = 500000000;
constexpr std::uint64_t navigationRepeatPeriod = (11000000000ULL + 119) / 120;

// Host window presentation shared by the title and gameplay windows.
struct PlacementMemory {
	WindowPlacement placement, saved;
	std::optional<std::filesystem::path> file;
	bool loaded = false;
};
PlacementMemory &placementMemory() {
	static PlacementMemory memory;
	return memory;
}
void savePlacement() {
	auto &memory = placementMemory();
	if (!memory.file || memory.placement == memory.saved) return;
	if (writeWindowPlacement(*memory.file, memory.placement)) memory.saved = memory.placement;
	else std::cerr << "Cannot save window settings: " << memory.file->u8string() << '\n';
}
// Fullscreen, maximized and minimized sizes are not the windowed size.
void recordWindowedSize(SDL_Window *window) {
	if (SDL_GetWindowFlags(window) & (SDL_WINDOW_FULLSCREEN | SDL_WINDOW_MAXIMIZED | SDL_WINDOW_MINIMIZED)) return;
	int width = 0, height = 0;
	SDL_GetWindowSize(window, &width, &height);
	if (width > 0 && height > 0) {
		placementMemory().placement.width = width;
		placementMemory().placement.height = height;
	}
}

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
        return key.keysym.sym==SDLK_LEFT?NavigationAction::StrafeLeft:NavigationAction::StrafeRight;
    if((key.keysym.mod & KMOD_CTRL) && key.keysym.sym==SDLK_DOWN)
        return UnsupportedMainScreenAction{"Turn around"};
    if((key.keysym.mod & KMOD_CTRL) && key.keysym.sym==SDLK_UP)return std::nullopt;
	switch (key.keysym.sym) {
	case SDLK_PERIOD: return WaitAction{};
	case SDLK_b: return screen==MainScreen::Exploration ? PlayerAction{BashAction{}} : PlayerAction{BlockAction{}};
	case SDLK_o: return screen==MainScreen::Combat ? std::optional<PlayerAction>{QuickFightOptionsAction{}} : std::nullopt;
	case SDLK_f: return screen==MainScreen::Combat ? std::optional<PlayerAction>{QuickFightAction{}} : std::nullopt;
	case SDLK_s: return screen==MainScreen::Combat ? std::nullopt : std::optional<PlayerAction>{ShootAction{}};
	case SDLK_a: return screen==MainScreen::Combat ? std::optional<PlayerAction>{AttackAction{}} : std::nullopt;
	case SDLK_c: return CastSpellAction{};
	case SDLK_r: return screen==MainScreen::Exploration ? PlayerAction{RestAction{}} : PlayerAction{RevisitCompletedAction{}};
	case SDLK_F9: return SaveGameAction{};
	case SDLK_TAB: return ControlPanelAction{};
	case SDLK_i: return InfoAction{};
	case SDLK_q: return QuickReferenceAction{};
	case SDLK_m: return MapAction{};
    case SDLK_v: return screen==MainScreen::Exploration ? std::optional<PlayerAction>{QuestsAction{}} : std::nullopt;
	case SDLK_EQUALS: case SDLK_KP_EQUALS: return ToggleAutomapAction{};
	case SDLK_KP_4: return NavigationAction::StrafeLeft;
	case SDLK_KP_6: return NavigationAction::StrafeRight;
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
SDL_Scancode physicalScan(const SDL_KeyboardEvent &key) {
    // A modifier/layout change can change keysym between make and break.
    // Synthetic clients may omit scancode; native SDL always supplies it.
    return key.keysym.scancode>SDL_SCANCODE_UNKNOWN && key.keysym.scancode<SDL_NUM_SCANCODES ?
        key.keysym.scancode:SDL_GetScancodeFromKey(key.keysym.sym);
}

unsigned inputKey(SDL_Keycode key) {
    if(key>=SDLK_F1 && key<=SDLK_F6) return InputKey::F1+key-SDLK_F1;
    switch(key) {
    case SDLK_UP: case SDLK_KP_8: return InputKey::Up;
    case SDLK_DOWN: case SDLK_KP_2: return InputKey::Down;
    case SDLK_LEFT: case SDLK_KP_4: return InputKey::Left;
    case SDLK_RIGHT: case SDLK_KP_6: return InputKey::Right;
    case SDLK_KP_ENTER: return InputKey::Enter;
    case SDLK_HOME: case SDLK_KP_7: return InputKey::Home;
    case SDLK_END: case SDLK_KP_1: return InputKey::End;
    case SDLK_PAGEUP: case SDLK_KP_9: return InputKey::PageUp;
    case SDLK_PAGEDOWN: case SDLK_KP_3: return InputKey::PageDown;
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
#ifdef SDL_HINT_WINDOWS_DPI_AWARENESS
	// Windows would otherwise blur-stretch the window on displays scaled above 100%.
	SDL_SetHint(SDL_HINT_WINDOWS_DPI_AWARENESS, "permonitorv2");
#endif
	if (SDL_InitSubSystem(SDL_INIT_VIDEO | SDL_INIT_EVENTS) != 0) {
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
			SDL_QuitSubSystem(SDL_INIT_VIDEO | SDL_INIT_EVENTS);
		}
	} resources;
	SDL_SetHint(SDL_HINT_RENDER_SCALE_QUALITY, "0");
	auto &placement = placementMemory();
	if (placement.file && !placement.loaded) placement.placement = placement.saved = readWindowPlacement(*placement.file);
	placement.loaded = true;
	std::pair<int, int> windowed{initialFrame.width * 3, initialFrame.height * 3};
	SDL_Rect usable{};
	if (SDL_GetDisplayUsableBounds(0, &usable) == 0)
		windowed = windowedSize(placement.placement, usable.w, usable.h, initialFrame.width, initialFrame.height);
	SDL_Window *window = resources.window = SDL_CreateWindow(title.c_str(), SDL_WINDOWPOS_CENTERED,
		SDL_WINDOWPOS_CENTERED, windowed.first, windowed.second,
		SDL_WINDOW_RESIZABLE | SDL_WINDOW_ALLOW_HIGHDPI |
		(placement.placement.fullscreen ? SDL_WINDOW_FULLSCREEN_DESKTOP : 0));
	if (!window) {
		std::cerr << "SDL_CreateWindow failed: " << SDL_GetError() << '\n';
		return false;
	}
	// Runs before Resources destroys the window.
	struct PlacementRecord {
		SDL_Window *window;
		~PlacementRecord() { try { recordWindowedSize(window); savePlacement(); } catch (...) {} }
	} placementRecord{window};
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
        if(handler.inputContext && handler.inputContext({}).hideCursor)return;
        if(!resources.cursor || SDL_GetMouseFocus()!=window)return;
        int x,y;SDL_GetMouseState(&x,&y);float logicalX,logicalY;
        SDL_RenderWindowToLogical(renderer,x,y,&logicalX,&logicalY);
        if(logicalX<0 || logicalY<0 || logicalX>=initialFrame.width || logicalY>=initialFrame.height)return;
        SDL_Rect destination{int(logicalX),int(logicalY),cursorWidth,cursorHeight};
        if(SDL_RenderCopy(renderer,resources.cursor,nullptr,&destination)!=0)throw std::runtime_error("Original cursor presentation failed");
    };
    // Borderless desktop fullscreen keeps the display mode; SDL's logical size
    // re-centers the integer-scaled frame and maps the mouse in either mode.
    const auto toggleFullscreen=[&] {
        const bool fullscreen=!(SDL_GetWindowFlags(window)&SDL_WINDOW_FULLSCREEN);
        if(fullscreen)recordWindowedSize(window);
        if(SDL_SetWindowFullscreen(window,fullscreen?SDL_WINDOW_FULLSCREEN_DESKTOP:0)!=0) {
            std::cerr<<"Cannot change fullscreen mode: "<<SDL_GetError()<<'\n';return;
        }
        placementMemory().placement.fullscreen=fullscreen;savePlacement();
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
    unsigned mouseButtons=0;
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
    struct PendingAction { PlayerAction action; std::uint64_t context; bool repeat; std::optional<InputButton> button; int scan=0; std::uint64_t press=0; unsigned mouse=0; };
    std::deque<PendingAction> pendingActions;
    struct HeldNavigation {
        std::optional<PlayerAction> action;
        std::optional<InputButton> button;
        std::uint64_t context=0, press=0;
        SDL_Keycode key=0;
        bool delivered=false;
        std::uint64_t repeatAt=0;
    };
    std::array<HeldNavigation,SDL_NUM_SCANCODES> heldNavigation{};
    std::uint64_t pressIdentity=0,lastRepeat=0,heldContext=0;
    Uint16 heldModifiers=0;
    struct StrictMouse {PlayerAction action;std::optional<std::uint64_t> input;IndexedFrame::Presentation frame;std::uint64_t context;};
    std::optional<StrictMouse> strictMouse;
    std::optional<InputButton> mouseFeedback;
    bool actionUsed = false;
    const auto contextFor = [&](const IndexedFrame::Presentation &origin) {
        return handler.inputContext ? handler.inputContext(origin) : InputContext{};
    };
    const auto navigationNow = [&] {
        return handler.presentationClock ? handler.presentationClock() : SdlWindow::presentationNanoseconds();
    };
    auto acquiredContext = contextFor(presentedFrame);
    const auto synchronizeQueue = [&](const InputContext &context) {
        if (!context.acceptsQueuedInput || heldContext!=context.contextId ||
            (!pendingActions.empty() && pendingActions.front().context != context.contextId)) {
            pendingActions.clear();heldNavigation.fill({});lastRepeat=0;
        }
        heldContext=context.contextId;
    };
    const auto retireQueuedKeys = [&] {
        // The strict path retains the pre-acquisition timestamp fence.
        if (contextFor(presentedFrame).acceptsQueuedInput) return;
        SDL_FilterEvents([](void *context, SDL_Event *queued) -> int {
            if (queued->type == SDL_KEYDOWN)
                queued->key.timestamp = *static_cast<std::uint32_t *>(context) - 1;
            if (queued->type == SDL_MOUSEBUTTONDOWN)
                queued->button.timestamp = *static_cast<std::uint32_t *>(context) - 1;
            if (queued->type == SDL_TEXTINPUT)
                queued->text.timestamp = *static_cast<std::uint32_t *>(context) - 1;
            return 1;
        }, &readyAt);
    };
    retireQueuedKeys();
    std::optional<std::uint64_t> displayedInput = uploaded && accepts(uploadedFrame) && handler.displayedInput ? handler.displayedInput() : std::nullopt;
    const auto inputCurrent = [&](const IndexedFrame::Presentation &origin) {
        return handler.acceptsInputFrame ? handler.acceptsInputFrame(origin) :
            uploaded && uploadedFrame == origin && accepts(origin);
    };
    const auto restoreMouseFeedback = [&] {
        if(!mouseFeedback)return;
        if(!uploadFrame(texture,displayContent,initialFrame.width,initialFrame.height,pixels))throw std::runtime_error("Mouse feedback restoration failed");
        SDL_SetRenderDrawColor(renderer,0,0,0,255);SDL_RenderClear(renderer);
        if(SDL_RenderCopy(renderer,texture,nullptr,nullptr)!=0)throw std::runtime_error("Mouse feedback restoration failed");
        drawCursor();SDL_RenderPresent(renderer);mouseFeedback.reset();
    };
    const auto feedback = [&](const InputButton &button,const IndexedFrame::Presentation &origin,bool restore=true) {
        if(handler.drawButton && inputCurrent(origin)) {
            // DOS shows frame | 1 and restores immediately for keyboard input.
            // Mouse feedback retains the native texture until physical release.
            // No gameplay callback, idle update, semantic upload or frame
            // acquisition occurs during this native display effect.
            auto pressed=displayContent;
            handler.drawButton(pressed,button);
            if(!inputCurrent(origin) || (handler.frameCurrent && !handler.frameCurrent()))
                throw std::runtime_error("Stale button feedback origin");
            const auto presentButtonTexture=[&] {
                SDL_SetRenderDrawColor(renderer,0,0,0,255);SDL_RenderClear(renderer);
                if(SDL_RenderCopy(renderer,texture,nullptr,nullptr)!=0)
                    throw std::runtime_error("Button feedback presentation failed");
                drawCursor();SDL_RenderPresent(renderer);
            };
            const auto presentButton=[&](const IndexedFrame &frame) {
                if(!uploadFrame(texture,frame,initialFrame.width,initialFrame.height,pixels))
                    throw std::runtime_error("Button feedback upload failed");
                presentButtonTexture();
            };
            presentButton(pressed);
            // Keyboard feedback has no explicit dwell (DAT 0x0e1ca..0x0e2b1).
            // Mouse release is handled below before entering gameplay.
            if(restore)presentButton(displayContent);else mouseFeedback=button;
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
        if(event.type==SDL_AUDIODEVICEREMOVED && !event.adevice.iscapture) {
            SdlAudioDevice::removed(event.adevice.which);return;
        }
        // Host window control, as in DOSBox: the game never receives Alt+Enter,
        // including its repeats; a plain Enter is unaffected.
        if(event.type==SDL_KEYDOWN && fullscreenToggleKey(event.key.keysym.sym,event.key.keysym.mod)) {
            if(!event.key.repeat)toggleFullscreen();
            return;
        }
        if(event.type==SDL_WINDOWEVENT && event.window.event==SDL_WINDOWEVENT_SIZE_CHANGED)recordWindowedSize(window);
        if(handler.finished && handler.finished())return;
        if (event.type == SDL_QUIT) {
            pendingActions.clear(); running = false;
        } else if (event.type == SDL_WINDOWEVENT && event.window.event == SDL_WINDOWEVENT_FOCUS_LOST) {
            journeyKeys.fill(false);heldNavigation.fill({});
            heldModifiers=0;
            spaceDown = blockDown = revisitDown = inspectDown = false;
            mouseButtons=0;strictMouse.reset();restoreMouseFeedback();
            pendingActions.clear();
        } else if(event.type==SDL_MOUSEBUTTONUP) {
            mouseButtons&=~SDL_BUTTON(event.button.button);
            if(!mouseButtons)restoreMouseFeedback();
        } else if (event.type == SDL_KEYUP) {
            const auto scan = physicalScan(event.key);
            if (scan > SDL_SCANCODE_UNKNOWN && scan < SDL_NUM_SCANCODES) {
                journeyKeys[scan] = false;heldNavigation[scan]={};
            }
            if(event.key.keysym.sym==SDLK_LCTRL || event.key.keysym.sym==SDLK_RCTRL ||
                event.key.keysym.sym==SDLK_LSHIFT || event.key.keysym.sym==SDLK_RSHIFT ||
                event.key.keysym.sym==SDLK_LALT || event.key.keysym.sym==SDLK_RALT) {
                heldModifiers=event.key.keysym.mod;
                if(event.key.keysym.sym==SDLK_LCTRL || event.key.keysym.sym==SDLK_RCTRL)heldModifiers&=~(event.key.keysym.sym==SDLK_LCTRL?KMOD_LCTRL:KMOD_RCTRL);
                const auto context=contextFor(batchFrame);
                for(auto &held:heldNavigation)if(held.press) {
                    SDL_KeyboardEvent key{};key.keysym.sym=held.key;key.keysym.mod=heldModifiers;
                    auto resolved=playerAction(key,context.mainScreen);
                    held.action=resolved && std::holds_alternative<NavigationAction>(*resolved)?resolved:std::nullopt;
                    held.button=xeenMainScreenKeyButton(held.action,inputKey(held.key),context.mainScreen);
                }
            }
            if (event.key.keysym.sym == SDLK_SPACE) spaceDown = false;
            if (event.key.keysym.sym == SDLK_b) blockDown = false;
            if (event.key.keysym.sym == SDLK_r) revisitDown = false;
            if (event.key.keysym.sym == SDLK_i) inspectDown = false;
        } else if(event.type==SDL_TEXTINPUT) {
            const auto context=contextFor(batchFrame);
            if(context.dialog && context.dialog->textEntry && context.readyForAction && inputCurrent(batchFrame) &&
                context.contextId==batchContext.contextId && static_cast<std::int32_t>(event.text.timestamp-readyAt)>=0)
                deliver(TextInputAction{event.text.text},batchInput,batchFrame,{});
        } else if (event.type == SDL_KEYDOWN || event.type == SDL_MOUSEBUTTONDOWN) {
            const bool mouse = event.type == SDL_MOUSEBUTTONDOWN;
            const auto context = contextFor(batchFrame);
            synchronizeQueue(context);
            if(mouse)mouseButtons|=SDL_BUTTON(event.button.button);
            // DOS polls the button status: right, including both buttons,
            // takes precedence over left. Retain event state for injected
            // input and include the native status for a simultaneous chord.
            const bool rightMouse=mouse && ((mouseButtons | SDL_GetMouseState(nullptr,nullptr))&SDL_BUTTON_RMASK);
            if(mouse && strictMouse && !rightMouse)return;
            if(!mouse) switch(event.key.keysym.sym) {
            case SDLK_LCTRL: case SDLK_RCTRL: case SDLK_LALT: case SDLK_RALT:
            case SDLK_LSHIFT: case SDLK_RSHIFT: case SDLK_LGUI: case SDLK_RGUI:
            case SDLK_CAPSLOCK: case SDLK_NUMLOCKCLEAR: case SDLK_SCROLLLOCK:
                heldModifiers=event.key.keysym.mod;
                if(event.key.keysym.sym==SDLK_LCTRL || event.key.keysym.sym==SDLK_RCTRL)heldModifiers|=event.key.keysym.sym==SDLK_LCTRL?KMOD_LCTRL:KMOD_RCTRL;
                for(auto &held:heldNavigation)if(held.press) {
                    SDL_KeyboardEvent key{};key.keysym.sym=held.key;key.keysym.mod=heldModifiers;
                    auto resolved=playerAction(key,context.mainScreen);
                    held.action=resolved && std::holds_alternative<NavigationAction>(*resolved)?resolved:std::nullopt;
                    held.button=xeenMainScreenKeyButton(held.action,inputKey(held.key),context.mainScreen);
                }
                return;
            default:break;
            }
            if (mouse && ((event.button.button != SDL_BUTTON_LEFT && event.button.button!=SDL_BUTTON_RIGHT) || (!context.dialog && context.mainScreen == MainScreen::None) ||
                initialFrame.width != 320 || initialFrame.height != 200 ||
                event.button.x<0 || event.button.y<0 || event.button.x>=320 || event.button.y>=200)) return;
            // SDL_RenderSetLogicalSize transforms native mouse events to framebuffer
            // coordinates, including integer scaling, letterboxing and high DPI.
            // Do not scale a second time. Out-of-viewport coordinates are rejected.
            const auto action = context.dialog ? (mouse ? (rightMouse?context.dialog->rightClick():context.dialog->click(event.button.x,event.button.y)) :
                context.dialog->key(inputKey(event.key.keysym.sym))) :
                mouse ? (rightMouse ? std::optional<PlayerAction>{CancelInteractionAction{}} : xeenMainScreenClick(event.button.x,event.button.y,context.mainScreen)) : playerAction(event.key,context.mainScreen);
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
                std::holds_alternative<BlockAction>(*action) || std::holds_alternative<BashAction>(*action) || std::holds_alternative<ShootAction>(*action) ||
                std::holds_alternative<RestAction>(*action) || std::holds_alternative<ControlPanelAction>(*action) ||
                std::holds_alternative<QuickReferenceAction>(*action) || std::holds_alternative<InfoAction>(*action) ||
                std::holds_alternative<MapAction>(*action) || std::holds_alternative<QuestsAction>(*action) || std::holds_alternative<ToggleAutomapAction>(*action) ||
                std::holds_alternative<RevisitCompletedAction>(*action) || std::holds_alternative<WaitAction>(*action) ||
                std::holds_alternative<QuickFightAction>(*action) || std::holds_alternative<QuickFightOptionsAction>(*action) ||
                std::holds_alternative<CastSpellAction>(*action) || std::holds_alternative<SelectMemberAction>(*action) ||
                (context.mainScreen==MainScreen::Combat && std::holds_alternative<UseItemAction>(*action)) ||
                std::holds_alternative<UnsupportedMainScreenAction>(*action) ||
                (context.dialog && (std::holds_alternative<DialogKeyAction>(*action) ||
                    std::holds_alternative<AcknowledgeAction>(*action) || std::holds_alternative<CancelInteractionAction>(*action))) ||
                (slot && slot->slot < 3));
            const bool queueable = context.acceptsQueuedInput && queueKey;
            const auto scan = mouse ? SDL_SCANCODE_UNKNOWN : physicalScan(event.key);
            if (queueable) {
                // A context transition within this event batch cannot relabel old keys.
                if (context.contextId != batchContext.contextId) return;
                if (mouse) {
                    if (pendingActions.size() < 5) {
                        if(button)feedback(*button,batchFrame,false);
                        pendingActions.push_back({*action,context.contextId,false,{},0,0,SDL_BUTTON(event.button.button)});
                    }
                    return;
                }
                if (scan <= SDL_SCANCODE_UNKNOWN || scan >= SDL_NUM_SCANCODES) return;
                const bool held = journeyKeys[scan];
                if (event.key.repeat) {
                    // OS repeats are observations, never accumulated movement.
                    if(held && heldNavigation[scan].press) {
                        heldNavigation[scan].action=movement?action:std::nullopt;
                        heldNavigation[scan].button=button;
                        heldNavigation[scan].key=event.key.keysym.sym;
                    }
                    return;
                }
                if(held)return;
                journeyKeys[scan]=true;
                if(movement) {
                    if(pressIdentity==std::numeric_limits<std::uint64_t>::max())throw std::overflow_error("Navigation press identity exhausted");
                    const auto press=++pressIdentity;
                    heldNavigation[scan]={action,button,context.contextId,press,event.key.keysym.sym,false,
                        PresentationClock::checkedAdd(navigationNow(),navigationRepeatDelay)};
                    // Approved option b: fresh taps are ordered obligations, without
                    // the five-entry drop. Held demand is stored separately.
                    pendingActions.push_back({*action,context.contextId,false,button,int(scan),press});
                } else if(pendingActions.size()<5)pendingActions.push_back({*action,context.contextId,false,button});
                return;
            }
            if(context.dialog && (!context.readyForAction || !inputCurrent(batchFrame) || context.contextId!=batchContext.contextId)) return;
            if(mouse) {
                if(rightMouse && !context.dialog && context.mainScreen==MainScreen::Combat && handler && canCancelInteraction && canCancelInteraction() &&
                    context.readyForAction && inputCurrent(batchFrame) && context.contextId==batchContext.contextId &&
                    static_cast<std::int32_t>(event.button.timestamp-readyAt)>=0 && action)
                    strictMouse=StrictMouse{*action,batchInput,batchFrame,context.contextId};
                if(context.dialog && context.readyForAction && inputCurrent(batchFrame) &&
                    context.contextId==batchContext.contextId && static_cast<std::int32_t>(event.button.timestamp-readyAt)>=0 && action)
                {
                    if(button)feedback(*button,batchFrame,false);
                    strictMouse=StrictMouse{*action,batchInput,batchFrame,context.contextId};
                }
                return;
            }
            const bool textBackspace=context.dialog && context.dialog->textEntry && event.key.keysym.sym==SDLK_BACKSPACE;
            if (event.key.repeat != 0 && !textBackspace) return;
            if ((handler.protectAllKeys || context.dialog) && (action || (button && handler.drawButton))) {
                if (scan <= SDL_SCANCODE_UNKNOWN || scan >= SDL_NUM_SCANCODES) return;
                const bool held = journeyKeys[scan]; journeyKeys[scan] = true;
                if ((held && !textBackspace) || (!held && event.key.repeat) || static_cast<std::int32_t>(event.key.timestamp-readyAt) < 0) return;
            } else if (batchInput && (event.key.keysym.sym == SDLK_SPACE || event.key.keysym.sym == SDLK_b ||
                event.key.keysym.sym == SDLK_r || event.key.keysym.sym == SDLK_i)) {
                auto &down = event.key.keysym.sym == SDLK_SPACE ? spaceDown : event.key.keysym.sym == SDLK_b ? blockDown :
                    event.key.keysym.sym == SDLK_r ? revisitDown : inspectDown;
                const bool held = down; down = true;
                if (held || static_cast<std::int32_t>(event.key.timestamp-readyAt) < 0) return;
            }
            if (handler.withPresentedInput && static_cast<std::int32_t>(event.key.timestamp-readyAt) < 0) return;
            if(mouseButtons)return; // Common mouse input returns only after release.
            if (handler.protectAllKeys && event.key.keysym.sym == SDLK_ESCAPE &&
                (batchInput != (handler.displayedInput ? handler.displayedInput() : std::nullopt) || !inputCurrent(batchFrame))) return;
            if (!context.dialog && event.key.keysym.sym == SDLK_ESCAPE && !(handler && canCancelInteraction && canCancelInteraction())) {
                // Nothing to cancel: the original main screen ignores Escape (no
                // Escape entry in the pinned Interface::perform). The game ends
                // through the control panel's Quit, Alt+F4 or the close button.
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
    const auto releaseMouseAction=[&] {
        if(!strictMouse || mouseButtons)return;
        const auto next=*strictMouse;strictMouse.reset();
        const auto context=contextFor(presentedFrame);
        if(context.contextId==next.context && context.readyForAction && inputCurrent(next.frame))
            deliver(next.action,next.input,next.frame,{});
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
		if(batchContext.dialog && batchContext.dialog->textEntry)SDL_StartTextInput();else SDL_StopTextInput();
		synchronizeQueue(batchContext);
        const auto actionReady=[&](const PlayerAction &a,const InputContext &c) {
            return c.readyForAction || (std::holds_alternative<NavigationAction>(a) && c.readyForNavigation) ||
                (c.restAvailable && std::holds_alternative<RestAction>(a));
        };
        bool queuedReady=!pendingActions.empty() && actionReady(pendingActions.front().action,batchContext) &&
            !(pendingActions.front().mouse & mouseButtons);
        std::optional<std::uint64_t> nextRepeat;
        if(pendingActions.empty())for(const auto &held:heldNavigation)
            if(held.action && held.delivered && held.context==batchContext.contextId && actionReady(*held.action,batchContext))
                nextRepeat=nextRepeat?std::min(*nextRepeat,held.repeatAt):held.repeatAt;
        const auto repeatSample=nextRepeat?navigationNow():0;
        const bool repeatReady=nextRepeat && repeatSample>=*nextRepeat;
        const auto waitSample=SdlWindow::presentationNanoseconds();
        int wait=int(PresentationClock::ceilMilliseconds(PresentationClock::after(waitSample,1)-waitSample));
        if(mouseButtons)wait=10;
        else if(queuedReady || repeatReady)wait=0;
        else if(handler.nextPresentationDeadline) {
            const auto deadline=handler.nextPresentationDeadline();
            if(deadline) {
                const auto now=handler.presentationClock?handler.presentationClock():SdlWindow::presentationNanoseconds();
                wait=*deadline<=now?0:int(std::min<std::uint64_t>(1000,PresentationClock::ceilMilliseconds(*deadline-now)));
            }
        }
        if(nextRepeat && *nextRepeat>repeatSample && !mouseButtons)
            wait=std::min(wait,int(std::min<std::uint64_t>(1000,PresentationClock::ceilMilliseconds(*nextRepeat-repeatSample))));
        if (SDL_WaitEventTimeout(&event, wait)) {
			do {
				dispatchEvent(event,batchInput,batchFrame,batchContext);
			} while (running && SDL_PollEvent(&event));
		}
		if (!running) break;
        if(handler.finished && handler.finished())break;
        const auto drainContext = contextFor(presentedFrame);
        synchronizeQueue(drainContext);
        releaseMouseAction();
        const bool hasHeldRepeat=std::any_of(heldNavigation.begin(),heldNavigation.end(),[&](const auto &held) {
            return held.action && held.delivered && held.context==drainContext.contextId;
        });
        const auto drainRepeatNow=hasHeldRepeat?navigationNow():0;
        if (!mouseButtons && !actionUsed && handler && !pendingActions.empty() && drainContext.acceptsQueuedInput &&
            actionReady(pendingActions.front().action,drainContext) && inputCurrent(presentedFrame) &&
            !(pendingActions.front().mouse & mouseButtons)) {
            const auto next=pendingActions.front();
            pendingActions.pop_front(); // A ready refusal is consumed, never retried.
            if(next.scan && heldNavigation[next.scan].press==next.press) {
                auto &held=heldNavigation[next.scan];held.delivered=true;
                held.repeatAt=std::max(held.repeatAt,PresentationClock::checkedAdd(navigationNow(),navigationRepeatPeriod));
            }
            deliver(next.action,displayedInput,presentedFrame,next.button);
        } else if(hasHeldRepeat && !mouseButtons && !actionUsed && handler && pendingActions.empty() && drainContext.acceptsQueuedInput && inputCurrent(presentedFrame)) {
            HeldNavigation *selected=nullptr,*first=nullptr;
            const auto now=drainRepeatNow;
            for(auto &held:heldNavigation)if(held.action && held.delivered && held.context==drainContext.contextId &&
                now>=held.repeatAt && actionReady(*held.action,drainContext)) {
                if(!first || held.press<first->press)first=&held;
                if(held.press>lastRepeat && (!selected || held.press<selected->press))selected=&held;
            }
            if(!selected)selected=first;
            if(selected){
                lastRepeat=selected->press;
                // Reset at dispatch, not after fallible feedback/work; late wakes
                // supply one demand and never accumulate typematic catch-up.
                selected->repeatAt=PresentationClock::checkedAdd(now,navigationRepeatPeriod);
                deliver(*selected->action,displayedInput,presentedFrame,selected->button);
            }
        }
        if (!running) break;
        const bool navigationWaitingForDraw=drainContext.navigationDrawPending &&
            ((!pendingActions.empty() && std::holds_alternative<NavigationAction>(pendingActions.front().action)) ||
             (pendingActions.empty() && std::any_of(heldNavigation.begin(),heldNavigation.end(),[&](const auto &held) {
                 return held.action && held.delivered && held.context==drainContext.contextId && drainRepeatNow>=held.repeatAt;
             })));
        // draw3d completes before polling input (DAT 0x0d1c3..0x0d1c9). If the
        // clock crosses its threshold after admission was sampled, return to
        // that poll before allowing idle to start a second draw. Retained work
        // never sets this flag and continues through its existing idle owner.
		if (idle && !mouseButtons && !navigationWaitingForDraw) {
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
        if(mouseButtons)continue; // Release wait transports cursor pixels only.
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
            if(!running)break; // A queued close cancels the retained response.
            // A mouse action from this bounded A batch returns on release,
            // before A is retired. Holding it keeps the acquired B handoff
            // pending; never relabel the press as B or run semantic idle work.
            releaseMouseAction();
			if (!running) break;
            if(mouseButtons)continue;
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
            // clearEvents boundary: retain releases, non-input events and the
            // host fullscreen toggle in order.
            SDL_FilterEvents([](void *, SDL_Event *queued) -> int {
                if(queued->type==SDL_KEYDOWN)return fullscreenToggleKey(queued->key.keysym.sym,queued->key.keysym.mod);
                return queued->type != SDL_MOUSEBUTTONDOWN;
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

std::uint64_t SdlWindow::presentationNanoseconds() {
    const auto frequency=SDL_GetPerformanceFrequency(),counter=SDL_GetPerformanceCounter();
    if(!frequency)throw std::runtime_error("SDL monotonic clock unavailable");
    const auto seconds=counter/frequency,remainder=counter%frequency;
    return seconds*1000000000+(remainder*1000000000)/frequency;
}

void SdlWindow::rememberPlacement(std::optional<std::filesystem::path> file) {
	auto &memory = placementMemory();
	memory.file = std::move(file);
	memory.loaded = false;
}

bool SdlWindow::show(const IndexedFrame &frame, const std::string &title) const {
	return showLoop(frame, title, FrameUpdateHandler{});
}

bool SdlWindow::showInteractive(const IndexedFrame &frame, const std::string &title,
		const FrameUpdateHandler &handler, const std::function<bool()> &handlesEscape,
		const IdleFrameHandler &idle, const std::function<std::string()> &status) const {
	return showLoop(frame, title, handler, handlesEscape, idle, status);
}

} // namespace mmodern
