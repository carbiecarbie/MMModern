#ifndef MMODERN_PLATFORM_XEEN_CONTROLLER_INPUT_H
#define MMODERN_PLATFORM_XEEN_CONTROLLER_INPUT_H

#include "core/InputContext.h"
#include "core/XeenKeyboard.h"
#include "platform/sdl/XeenProgramKeys.h"
#include "platform/sdl/XeenRussianKeys.h"

#include <SDL_keycode.h>

#include <algorithm>
#include <cstdint>
#include <memory>
#include <optional>
#include <string>

namespace mmodern {

// M115 (approved deviation dev-controller): the original has no controller
// support, so every controller press becomes a key the original accepts.
// This header is the mapping only: (control, context snapshot) to an internal
// key, then to the physical key event of the active keyboard. It reads no
// device and no SDL state.

// The controller's controls as the mapping table names them (Xbox names; SDL
// maps other controllers to the same controls).
enum class ControllerControl : unsigned {
    Up, Down, Left, Right, A, B, X, Y, LeftShoulder, RightShoulder, LeftTrigger,
    View, LeftStick, RightStick, Menu, Count
};
inline constexpr unsigned kControllerControls = unsigned(ControllerControl::Count);

// What the controller needs of the input context of the frame on screen. The
// loop publishes it where it already samples the context; the controller never
// calls the handler's inputContext itself.
struct ControllerContext {
    MainScreen screen = MainScreen::None;
    bool dialog = false;
    bool textEntry = false;
    // The dialog takes Enter (or any key) / Escape; a Yes/No prompt takes Y and N.
    bool acceptsEnter = false, acceptsEscape = false, yesNo = false;
    // The Russian program key set the loop translates this context with
    // (xeenContextKeySet, as SdlWindow's contextKey); empty translates nothing.
    std::string keySet;
    // The Russian program's keyboard, or null for the CD keyboard.
    std::shared_ptr<const XeenKeyboard> keyboard;
};

inline ControllerContext xeenControllerContext(const InputContext &context,
        std::shared_ptr<const XeenKeyboard> keyboard) {
    ControllerContext result;
    result.screen = context.mainScreen;
    result.keyboard = std::move(keyboard);
    if (const auto &dialog = context.dialog) {
        const auto accepts = [&](unsigned key) {
            return std::find(dialog->keys.begin(), dialog->keys.end(), key) != dialog->keys.end();
        };
        result.dialog = true;
        result.textEntry = dialog->textEntry;
        result.acceptsEnter = dialog->anyKey || accepts(InputKey::Enter);
        result.acceptsEscape = dialog->anyKey || accepts(InputKey::Escape);
        result.yesNo = accepts('y') && accepts('n');
    } else {
        // Without a dialog the loop itself maps Enter and Escape.
        result.acceptsEnter = result.acceptsEscape = true;
    }
    if (const auto *set = xeenContextKeySet(context)) result.keySet = set;
    return result;
}

// Internal keys (core/DialogInput.h InputKey codes and lowercase ASCII), plus
// the strafe codes that Ctrl+Left/Right give in the CD program.
namespace ControllerKey {
inline constexpr unsigned StrafeLeft = 0xb0, StrafeRight = 0xb1, Tab = 9;
}

// The mapping table of docs/milestone-115-plan.md, decision 2 (Part A keys;
// the right stick and RT are the pointer and its click).
inline std::optional<unsigned> xeenControllerKey(ControllerControl control, const ControllerContext &context) {
    using C = ControllerControl;
    if (context.dialog || context.screen == MainScreen::None) {
        // Text entry takes A and B only; letter buttons are other commands in
        // dialogs (C creates a character in the Inn) and send nothing.
        if (context.textEntry)
            return control == C::A ? std::optional<unsigned>(InputKey::Enter) :
                control == C::B ? std::optional<unsigned>(InputKey::Escape) : std::nullopt;
        switch (control) {
        case C::Up: return InputKey::Up;
        case C::Down: return InputKey::Down;
        case C::Left: return InputKey::Left;
        case C::Right: return InputKey::Right;
        case C::LeftShoulder: return InputKey::PageUp;
        case C::RightShoulder: return InputKey::PageDown;
        // A Yes/No prompt without Enter takes A as Y, one without Escape B as N.
        case C::A: return !context.acceptsEnter && context.yesNo ? 'y' : InputKey::Enter;
        case C::B: return !context.acceptsEscape && context.yesNo ? 'n' : InputKey::Escape;
        case C::Menu: return ControllerKey::Tab;
        default: return std::nullopt;
        }
    }
    const bool combat = context.screen == MainScreen::Combat;
    switch (control) {
    case C::Up: return InputKey::Up; // Refused in combat, as on the keyboard.
    case C::Down: return InputKey::Down;
    case C::Left: return InputKey::Left;
    case C::Right: return InputKey::Right;
    case C::LeftShoulder: return combat ? std::nullopt : std::optional<unsigned>(ControllerKey::StrafeLeft);
    case C::RightShoulder: return combat ? std::nullopt : std::optional<unsigned>(ControllerKey::StrafeRight);
    case C::A: return combat ? 'a' : InputKey::Space;
    case C::B: return InputKey::Escape;
    case C::X: return 'c';
    case C::Y: return combat ? 'f' : 's';
    case C::LeftTrigger: return 'b';
    case C::View: return 'r';
    case C::LeftStick: return 'w';
    case C::RightStick: return 'm';
    case C::Menu: return ControllerKey::Tab;
    default: return std::nullopt;
    }
}

// The key event a controller sends: the event carries its modifiers itself and
// is never accompanied by a modifier key event.
struct ControllerKeyEvent {
    SDL_Keycode sym = SDLK_UNKNOWN;
    SDL_Scancode scancode = SDL_SCANCODE_UNKNOWN;
    std::uint16_t mod = KMOD_NONE;
    bool operator==(const ControllerKeyEvent &other) const {
        return sym == other.sym && scancode == other.scancode && mod == other.mod;
    }
    bool operator!=(const ControllerKeyEvent &other) const { return !(*this == other); }
};

namespace xeen_controller_input {
struct NamedKey { SDL_Keycode sym; SDL_Scancode scancode; unsigned key; };
// Keys outside the printable block: both programs keep the CD's codes.
inline constexpr NamedKey kNamedKeys[] {
    {SDLK_UP, SDL_SCANCODE_UP, InputKey::Up}, {SDLK_DOWN, SDL_SCANCODE_DOWN, InputKey::Down},
    {SDLK_LEFT, SDL_SCANCODE_LEFT, InputKey::Left}, {SDLK_RIGHT, SDL_SCANCODE_RIGHT, InputKey::Right},
    {SDLK_PAGEUP, SDL_SCANCODE_PAGEUP, InputKey::PageUp}, {SDLK_PAGEDOWN, SDL_SCANCODE_PAGEDOWN, InputKey::PageDown},
    {SDLK_RETURN, SDL_SCANCODE_RETURN, InputKey::Enter}, {SDLK_ESCAPE, SDL_SCANCODE_ESCAPE, InputKey::Escape},
    {SDLK_TAB, SDL_SCANCODE_TAB, ControllerKey::Tab},
};
// The US keycode of a printable position (a USB usage, as SDL scancodes are),
// so that keysym and scancode agree as they do on a US keyboard.
inline SDL_Keycode printableSymbol(unsigned usage) {
    if (usage >= 4 && usage <= 29) return SDL_Keycode(SDLK_a + (usage - 4));
    if (usage >= 30 && usage <= 38) return SDL_Keycode(SDLK_1 + (usage - 30));
    switch (usage) {
    case 39: return SDLK_0; case 44: return SDLK_SPACE; case 45: return SDLK_MINUS; case 46: return SDLK_EQUALS;
    case 47: return SDLK_LEFTBRACKET; case 48: return SDLK_RIGHTBRACKET; case 49: return SDLK_BACKSLASH;
    case 51: return SDLK_SEMICOLON; case 52: return SDLK_QUOTE; case 53: return SDLK_BACKQUOTE;
    case 54: return SDLK_COMMA; case 55: return SDLK_PERIOD; case 56: return SDLK_SLASH;
    default: return SDLK_UNKNOWN;
    }
}
} // namespace xeen_controller_input

// The physical key that produces `key` in this context, as SdlWindow
// translates keys: the CD key in English; in Russian the key whose Russian
// program code the context's key set maps to `key`. A key that cannot be
// typed in Russian sends nothing, as on the Russian keyboard.
inline std::optional<ControllerKeyEvent> xeenControllerKeyEvent(unsigned key, const ControllerContext &context) {
    using namespace xeen_controller_input;
    if (!context.keyboard) {
        // English strafing is Ctrl+Left/Right. Its own scancode keeps it apart
        // from a held D-pad Left or Right in the loop's per-key state.
        if (key == ControllerKey::StrafeLeft) return ControllerKeyEvent{SDLK_LEFT, SDL_SCANCODE_F13, KMOD_LCTRL};
        if (key == ControllerKey::StrafeRight) return ControllerKeyEvent{SDLK_RIGHT, SDL_SCANCODE_F14, KMOD_LCTRL};
        if (key == InputKey::Space) return ControllerKeyEvent{SDLK_SPACE, SDL_SCANCODE_SPACE};
        if (key >= 'a' && key <= 'z')
            return ControllerKeyEvent{SDL_Keycode(SDLK_a + (key - 'a')), SDL_Scancode(SDL_SCANCODE_A + (key - 'a'))};
        for (const auto &named : kNamedKeys) if (named.key == key) return ControllerKeyEvent{named.sym, named.scancode};
        return std::nullopt;
    }
    const auto &keyboard = *context.keyboard;
    const auto translate = [&](unsigned code) { return context.keySet.empty() ? code : keyboard.key(context.keySet, code); };
    for (const auto &named : kNamedKeys)
        if (translate(named.key) == key) return ControllerKeyEvent{named.sym, named.scancode};
    // Printable keys by position, unshifted first; Shift is carried by the
    // event itself. The Russian Ctrl codes are never needed.
    for (const bool shift : {false, true}) {
        for (unsigned usage = 4; usage <= 56; ++usage) {
            if (usage >= 40 && usage <= 43) continue; // Enter, Escape, Backspace, Tab.
            const auto code = xeenRussianKeyCode(keyboard, usage, shift, false);
            if (!code) continue;
            const auto internal = xeenProgramInputKey(code);
            if (internal && translate(internal) == key)
                return ControllerKeyEvent{printableSymbol(usage), SDL_Scancode(usage),
                    std::uint16_t(shift ? KMOD_LSHIFT : KMOD_NONE)};
        }
    }
    return std::nullopt;
}

inline std::optional<ControllerKeyEvent> xeenControllerKeyEvent(ControllerControl control, const ControllerContext &context) {
    const auto key = xeenControllerKey(control, context);
    return key ? xeenControllerKeyEvent(*key, context) : std::nullopt;
}

} // namespace mmodern

#endif
