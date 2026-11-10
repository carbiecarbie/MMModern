// M115 Part A: game controller keys. Mapping and tracker units without SDL,
// then the production SDL loop driven by pushed SDL_CONTROLLER* events whose
// state comes entirely from the event payloads (no device is needed).
#include "SdlMouseTestSupport.h"
#include "core/PresentationClock.h"
#include "platform/sdl/SdlController.h"
#include "platform/sdl/SdlWindow.h"

#define SDL_MAIN_HANDLED
#include <SDL.h>

#include <algorithm>
#include <atomic>
#include <chrono>
#include <filesystem>
#include <fstream>
#include <functional>
#include <thread>
#include <iostream>
#include <stdexcept>
#include <string>
#include <variant>
#include <vector>

using namespace mmodern;

// Part B: the wait the loop asks SDL for, while a test records it.
static std::vector<int> waits;
static bool recordWaits = false;
static std::optional<std::pair<int, int>> *cursorTarget = nullptr;
static bool *cursorRecording = nullptr;
extern "C" int __real_SDL_RenderCopy(SDL_Renderer *, SDL_Texture *, const SDL_Rect *, const SDL_Rect *);
extern "C" int __wrap_SDL_RenderCopy(SDL_Renderer *renderer, SDL_Texture *texture, const SDL_Rect *source, const SDL_Rect *target) {
    // The test cursor is 2x2: its copies are the only 2x2 destinations.
    if (cursorRecording && *cursorRecording && target && target->w == 2 && target->h == 2) *cursorTarget = std::pair<int, int>{target->x, target->y};
    return __real_SDL_RenderCopy(renderer, texture, source, target);
}
// SDL's dummy video driver gives every window mouse focus; a test can report
// none, as when the mouse is outside the window.
static bool noMouseFocus = false;
extern "C" SDL_Window *__real_SDL_GetMouseFocus();
extern "C" SDL_Window *__wrap_SDL_GetMouseFocus() { return noMouseFocus ? nullptr : __real_SDL_GetMouseFocus(); }
extern "C" int __real_SDL_WaitEventTimeout(SDL_Event *, int);
extern "C" int __wrap_SDL_WaitEventTimeout(SDL_Event *event, int timeout) {
    if (!recordWaits) return __real_SDL_WaitEventTimeout(event, timeout);
    waits.push_back(timeout);
    return __real_SDL_WaitEventTimeout(event, std::min(timeout, 2));
}

namespace {

using C = ControllerControl;
void check(bool value, const std::string &message) {
    if (!value) throw std::runtime_error(message);
}

std::shared_ptr<const DialogInput> dialogWith(std::vector<unsigned> keys, bool textEntry = false, bool anyKey = false,
        const char *keySet = nullptr) {
    auto dialog = std::make_shared<DialogInput>();
    dialog->keys = std::move(keys);
    dialog->textEntry = textEntry;
    dialog->stringEntry = textEntry;
    dialog->anyKey = anyKey;
    dialog->keySet = keySet;
    return dialog;
}
InputContext contextOf(MainScreen screen, std::shared_ptr<const DialogInput> dialog = {}) {
    InputContext context;
    context.mainScreen = screen;
    context.dialog = std::move(dialog);
    return context;
}

// A synthetic Russian program keyboard: a few letter positions and the key
// sets that alias them, as XeenDosText builds them from the Russian xeen.dat.
std::shared_ptr<const XeenKeyboard> russianKeyboard() {
    auto keyboard = std::make_shared<XeenKeyboard>();
    // Every letter position types a distinct Cyrillic code, so no Latin letter
    // can be typed, as in the Russian program's table.
    constexpr std::uint8_t letterScans[26]{0x1e,0x30,0x2e,0x20,0x12,0x21,0x22,0x23,0x17,0x24,0x25,0x26,0x32,
        0x31,0x18,0x19,0x10,0x13,0x1f,0x14,0x16,0x2f,0x11,0x2d,0x15,0x2c};
    const std::uint8_t pool[26]{0xa1,0xa3,0xa4,0xa5,0xa6,0xa7,0xa9,0xab,0xac,0xad,0xae,0xaf,0xe0,0xe3,0xe6,0xe7,0xe8,0xe9,
        0xed,0xee,0xef,0x80,0x81,0x82,0x83,0x84};
    for (unsigned i = 0; i < 26; ++i) keyboard->letters[letterScans[i]] = pool[i];
    keyboard->letters[0x1a] = 0xe5; // [ types х (strafe left).
    keyboard->letters[0x1b] = 0xea; // ] types ъ (strafe right).
    keyboard->letters[0x2e] = 0xe1; // C types с.
    keyboard->letters[0x1e] = 0xe4; // A types ф.
    keyboard->letters[0x1f] = 0xeb; // S types ы.
    keyboard->letters[0x30] = 0xa8; // B types и.
    keyboard->letters[0x13] = 0xaa; // R types к.
    keyboard->letters[0x32] = 0xec; // M types ь.
    keyboard->letters[0x21] = 0xa0; // F types а.
    keyboard->letters[0x31] = 0xe2; // N types т.
    keyboard->letters[0x20] = 0xa2; // D types в.
    keyboard->sets["MAIN"].aliases = {{0xe5, ControllerKey::StrafeLeft}, {0xea, ControllerKey::StrafeRight},
        {0xe1, 'c'}, {0xeb, 's'}, {0xa8, 'b'}, {0xaa, 'r'}, {0xec, 'm'}};
    keyboard->sets["COMBAT"].aliases = {{0xe4, 'a'}, {0xe1, 'c'}, {0xa0, 'f'}, {0xa8, 'b'}, {0xaa, 'r'}, {0xec, 'm'}};
    keyboard->sets["CONFIRM"].aliases = {{0xa2, 'y'}, {0xe2, 'n'}};
    return keyboard;
}

// Every key cell of the mapping table (decision 2), the Yes/No rule, the
// English and Russian key events and the strafe events.
void mappingTable() {
    const auto key = [](C control, const InputContext &context) {
        return xeenControllerKey(control, xeenControllerContext(context, nullptr));
    };
    const auto none = std::optional<unsigned>{};
    using K = std::optional<unsigned>;
    const auto exploration = contextOf(MainScreen::Exploration), combat = contextOf(MainScreen::Combat);
    struct Row { C control; K exploration, combat, dialog; };
    const auto dialog = contextOf(MainScreen::Exploration, dialogWith({InputKey::Enter, InputKey::Escape, InputKey::Up}));
    const std::vector<Row> rows{
        {C::Up, InputKey::Up, InputKey::Up, InputKey::Up}, {C::Down, InputKey::Down, InputKey::Down, InputKey::Down},
        {C::Left, InputKey::Left, InputKey::Left, InputKey::Left}, {C::Right, InputKey::Right, InputKey::Right, InputKey::Right},
        {C::LeftShoulder, ControllerKey::StrafeLeft, none, InputKey::PageUp},
        {C::RightShoulder, ControllerKey::StrafeRight, none, InputKey::PageDown},
        {C::A, InputKey::Space, 'a', InputKey::Enter}, {C::B, InputKey::Escape, InputKey::Escape, InputKey::Escape},
        {C::X, 'c', 'c', none}, {C::Y, 's', 'f', none}, {C::LeftTrigger, 'b', 'b', none}, {C::View, 'r', 'r', none},
        {C::LeftStick, 'w', 'w', none}, {C::RightStick, 'm', 'm', none}, {C::Menu, 9, 9, 9},
    };
    for (const auto &row : rows) {
        const auto name = std::to_string(unsigned(row.control));
        check(key(row.control, exploration) == row.exploration, "exploration cell " + name);
        check(key(row.control, combat) == row.combat, "combat cell " + name);
        check(key(row.control, dialog) == row.dialog, "dialog cell " + name);
        // The title and setup screens have no main screen: the dialog column.
        check(key(row.control, contextOf(MainScreen::None)) == row.dialog, "title cell " + name);
        check(key(row.control, contextOf(MainScreen::Combat, dialogWith({InputKey::Enter, InputKey::Escape}))) == row.dialog,
            "combat dialog cell " + name);
    }
    // Yes/No: a dialog taking both Y and N. The original prompts take Escape
    // and not Enter, so A is Y and B Escape; N only for a prompt without Escape.
    const auto yesNo = contextOf(MainScreen::Exploration, dialogWith({'y', 'n', InputKey::Escape}));
    check(key(C::A, yesNo) == K('y') && key(C::B, yesNo) == K(InputKey::Escape), "Yes/No prompt: A is Y, B is Escape");
    const auto bare = contextOf(MainScreen::None, dialogWith({'y', 'n'}));
    check(key(C::A, bare) == K('y') && key(C::B, bare) == K('n'), "Yes/No prompt without Escape: B is N");
    const auto withEnter = contextOf(MainScreen::None, dialogWith({'y', 'n', InputKey::Enter, InputKey::Escape}));
    check(key(C::A, withEnter) == K(InputKey::Enter), "a Yes/No prompt with Enter keeps Enter");
    const auto newOnly = contextOf(MainScreen::None, dialogWith({'n', 'c', InputKey::Escape}));
    check(key(C::A, newOnly) == K(InputKey::Enter) && key(C::B, newOnly) == K(InputKey::Escape), "N alone is never sent");
    const auto anyKey = contextOf(MainScreen::None, dialogWith({}, false, true));
    check(key(C::A, anyKey) == K(InputKey::Enter) && key(C::B, anyKey) == K(InputKey::Escape), "any-key wait");
    // Text entry: A and B only (Part A; the character cycle is Part B).
    const auto text = contextOf(MainScreen::None, dialogWith({InputKey::Enter, InputKey::Escape}, true));
    for (unsigned control = 0; control < kControllerControls; ++control) {
        const auto c = C(control);
        const auto expected = c == C::A ? K(InputKey::Enter) : c == C::B ? K(InputKey::Escape) : none;
        check(key(c, text) == expected, "text entry sends only A and B: " + std::to_string(control));
    }

    // English key events: the CD keys; strafe carries Ctrl itself, on a scancode of its own.
    const auto event = [](C control, const InputContext &context) {
        return xeenControllerKeyEvent(control, xeenControllerContext(context, nullptr));
    };
    check(event(C::Up, exploration) == ControllerKeyEvent{SDLK_UP, SDL_SCANCODE_UP}, "English Up");
    check(event(C::A, exploration) == ControllerKeyEvent{SDLK_SPACE, SDL_SCANCODE_SPACE}, "English Space");
    check(event(C::A, combat) == ControllerKeyEvent{SDLK_a, SDL_SCANCODE_A}, "English attack");
    check(event(C::Menu, combat) == ControllerKeyEvent{SDLK_TAB, SDL_SCANCODE_TAB}, "English Tab");
    check(event(C::LeftShoulder, dialog) == ControllerKeyEvent{SDLK_PAGEUP, SDL_SCANCODE_PAGEUP}, "English Page Up");
    check(event(C::B, yesNo) == ControllerKeyEvent{SDLK_ESCAPE, SDL_SCANCODE_ESCAPE}, "English Escape");
    check(event(C::A, yesNo) == ControllerKeyEvent{SDLK_y, SDL_SCANCODE_Y}, "English Y");
    const auto strafeLeft = event(C::LeftShoulder, exploration), strafeRight = event(C::RightShoulder, exploration);
    check(strafeLeft && strafeLeft->sym == SDLK_LEFT && (strafeLeft->mod & KMOD_CTRL) && !(strafeLeft->mod & ~KMOD_CTRL) &&
        strafeLeft->scancode != SDL_SCANCODE_LEFT, "English strafe left is Ctrl+Left on its own scancode");
    check(strafeRight && strafeRight->sym == SDLK_RIGHT && (strafeRight->mod & KMOD_CTRL) &&
        strafeRight->scancode != SDL_SCANCODE_RIGHT && strafeRight->scancode != strafeLeft->scancode,
        "English strafe right is Ctrl+Right on its own scancode");
    for (unsigned control = 0; control < kControllerControls; ++control)
        for (const auto &context : {exploration, combat, dialog, yesNo})
            if (const auto k = key(C(control), context))
                check(bool(xeenControllerKeyEvent(*k, xeenControllerContext(context, nullptr))), "English key without event");

    // Russian: the Russian program's key for the same button, through its key set.
    const auto keyboard = russianKeyboard();
    const auto ru = [&](C control, const InputContext &context) {
        return xeenControllerKeyEvent(control, xeenControllerContext(context, keyboard));
    };
    check(ru(C::LeftShoulder, exploration) == ControllerKeyEvent{SDLK_LEFTBRACKET, SDL_SCANCODE_LEFTBRACKET},
        "Russian strafe left is the х key, without Ctrl");
    check(ru(C::RightShoulder, exploration) == ControllerKeyEvent{SDLK_RIGHTBRACKET, SDL_SCANCODE_RIGHTBRACKET},
        "Russian strafe right is the ъ key");
    check(ru(C::X, exploration) == ControllerKeyEvent{SDLK_c, SDL_SCANCODE_C}, "Russian Cast by position");
    check(ru(C::A, combat) == ControllerKeyEvent{SDLK_a, SDL_SCANCODE_A}, "Russian attack through COMBAT");
    check(ru(C::Y, combat) == ControllerKeyEvent{SDLK_f, SDL_SCANCODE_F}, "Russian Quick Fight");
    check(ru(C::A, exploration) == ControllerKeyEvent{SDLK_SPACE, SDL_SCANCODE_SPACE}, "Russian Space");
    check(ru(C::Up, exploration) == ControllerKeyEvent{SDLK_UP, SDL_SCANCODE_UP}, "Russian arrows keep the CD keys");
    check(ru(C::Menu, exploration) == ControllerKeyEvent{SDLK_TAB, SDL_SCANCODE_TAB}, "Russian Tab");
    check(!ru(C::LeftStick, exploration) && !ru(C::LeftStick, combat), "W cannot be typed in Russian: nothing is sent");
    const auto confirm = contextOf(MainScreen::Exploration, dialogWith({'y', 'n', InputKey::Escape}, false, false, "CONFIRM"));
    check(ru(C::A, confirm) == ControllerKeyEvent{SDLK_d, SDL_SCANCODE_D} && ru(C::B, confirm) == ControllerKeyEvent{SDLK_ESCAPE, SDL_SCANCODE_ESCAPE},
        "Russian Yes through the dialog's key set");
    const auto noEscape = contextOf(MainScreen::None, dialogWith({'y', 'n'}, false, false, "CONFIRM"));
    check(ru(C::B, noEscape) == ControllerKeyEvent{SDLK_n, SDL_SCANCODE_N}, "Russian No through the dialog's key set");
    const auto ruText = contextOf(MainScreen::None, dialogWith({InputKey::Enter, InputKey::Escape}, true));
    check(ru(C::A, ruText) == ControllerKeyEvent{SDLK_RETURN, SDL_SCANCODE_RETURN} && !ru(C::Up, ruText) && !ru(C::Right, ruText),
        "a Russian text entry never receives an arrow");
    // A letter whose code no key set aliases cannot be typed.
    check(!xeenControllerKeyEvent('w', xeenControllerContext(exploration, keyboard)), "unaliased Latin letter");
    std::cout << "Controller mapping table passed\n";
}

std::vector<ControllerKeyChange> concat(std::vector<ControllerKeyChange> a, const std::vector<ControllerKeyChange> &b) {
    a.insert(a.end(), b.begin(), b.end());
    return a;
}
ControllerKeyChange make(ControllerKeyEvent key) { return {key, true}; }
ControllerKeyChange brk(ControllerKeyEvent key) { return {key, false}; }

void trackerUnits() {
    const auto exploration = xeenControllerContext(contextOf(MainScreen::Exploration), nullptr);
    const auto dialog = xeenControllerContext(contextOf(MainScreen::None, dialogWith({InputKey::Enter, InputKey::Escape})), nullptr);
    const ControllerKeyEvent up{SDLK_UP, SDL_SCANCODE_UP}, left{SDLK_LEFT, SDL_SCANCODE_LEFT}, space{SDLK_SPACE, SDL_SCANCODE_SPACE};
    using T = ControllerTracker;
    {   // D-pad, stick and two controllers merged: made once, released once.
        T t;
        check(t.button(1, SDL_CONTROLLER_BUTTON_DPAD_UP, true, exploration) == std::vector{make(up)}, "D-pad make");
        check(t.axis(1, SDL_CONTROLLER_AXIS_LEFTY, -30000, exploration).empty(), "stick on a held D-pad");
        check(t.button(2, SDL_CONTROLLER_BUTTON_DPAD_UP, true, exploration).empty(), "second controller on a held key");
        check(t.button(1, SDL_CONTROLLER_BUTTON_DPAD_UP, false, exploration).empty(), "first release keeps the key");
        check(t.axis(1, SDL_CONTROLLER_AXIS_LEFTY, 0, exploration).empty(), "stick release keeps the key");
        check(t.button(2, SDL_CONTROLLER_BUTTON_DPAD_UP, false, exploration) == std::vector{brk(up)}, "last release breaks");
        check(t.idle(), "merged tracker idle");
        check(t.button(1, SDL_CONTROLLER_BUTTON_DPAD_UP, false, exploration).empty(), "duplicate break ignored");
    }
    {   // A break releases the key its make sent, whatever the context is then.
        T t;
        check(t.button(1, SDL_CONTROLLER_BUTTON_A, true, exploration) == std::vector{make(space)}, "A is Space");
        check(t.button(1, SDL_CONTROLLER_BUTTON_A, false, dialog) == std::vector{brk(space)}, "break after a context change");
        check(t.button(1, SDL_CONTROLLER_BUTTON_A, true, dialog) == std::vector{make({SDLK_RETURN, SDL_SCANCODE_RETURN})}, "A is Enter now");
        check(t.button(1, SDL_CONTROLLER_BUTTON_A, false, std::nullopt).size() == 1, "break without a snapshot");
        check(t.button(1, SDL_CONTROLLER_BUTTON_X, true, std::nullopt).empty() &&
            t.button(1, SDL_CONTROLLER_BUTTON_X, false, exploration).empty(), "nothing is translated without a snapshot");
        check(t.button(1, SDL_CONTROLLER_BUTTON_X, true, dialog).empty() &&
            t.button(1, SDL_CONTROLLER_BUTTON_X, false, exploration).empty(), "a press with no key breaks nothing");
        check(t.button(1, SDL_CONTROLLER_BUTTON_GUIDE, true, exploration).empty(), "Guide is not mapped");
    }
    {   // Travel hysteresis: noise around half travel makes one press.
        T t;std::vector<ControllerKeyChange> all;
        for (int value : {-16000, -16500, -15800, -16700, -12000, -16400, -11000, -17000, -16384})
            all = concat(all, t.axis(1, SDL_CONTROLLER_AXIS_LEFTY, value, exploration));
        check(all == std::vector{make(up)}, "noisy stick at the threshold");
        check(t.axis(1, SDL_CONTROLLER_AXIS_LEFTY, -10000, exploration) == std::vector{brk(up)}, "release below a third");
    }
    {   // Direction hysteresis: a noisy diagonal makes one press.
        T t;std::vector<ControllerKeyChange> all;
        const int noise[] = {0, 900, -700, 1500, -1200, 400, -1500, 1100};
        for (unsigned i = 0; i < 64; ++i) {
            all = concat(all, t.axis(1, SDL_CONTROLLER_AXIS_LEFTX, -23170 + noise[i % 8], exploration));
            all = concat(all, t.axis(1, SDL_CONTROLLER_AXIS_LEFTY, -23170 - noise[(i + 3) % 8], exploration));
        }
        // X moved first, so the stick chose Left and kept it through the noise.
        check(all == std::vector{make(left)}, "noisy diagonal made one press");
        // The other axis must lead by the margin to change direction.
        check(t.axis(1, SDL_CONTROLLER_AXIS_LEFTX, -20000, exploration).empty(), "a lead within the margin keeps the direction");
        check(t.axis(1, SDL_CONTROLLER_AXIS_LEFTY, -27000, exploration) == std::vector{brk(left), make(up)},
            "a lead past the margin changes direction, break first");
        // A stick returning to the center passes through smaller values on both axes.
        std::vector<ControllerKeyChange> centering;
        for (int m : {16000, 9000, 3000, 0})
            centering = concat(concat(centering, t.axis(1, SDL_CONTROLLER_AXIS_LEFTX, -m, exploration)),
                t.axis(1, SDL_CONTROLLER_AXIS_LEFTY, -m, exploration));
        check(centering == std::vector{brk(up)} && t.idle(), "stick centered");
    }
    {   // LT is a button with the same hysteresis; RT and the right stick are Part B's pointer.
        T t;const ControllerKeyEvent bash{SDLK_b, SDL_SCANCODE_B};
        check(t.axis(1, SDL_CONTROLLER_AXIS_TRIGGERLEFT, 16000, exploration).empty(), "LT below half travel");
        check(t.axis(1, SDL_CONTROLLER_AXIS_TRIGGERLEFT, 17000, exploration) == std::vector{make(bash)}, "LT at half travel");
        check(t.axis(1, SDL_CONTROLLER_AXIS_TRIGGERLEFT, 12000, exploration).empty(), "LT above a third");
        check(t.axis(1, SDL_CONTROLLER_AXIS_TRIGGERLEFT, 9000, exploration) == std::vector{brk(bash)}, "LT released");
        check(t.axis(1, SDL_CONTROLLER_AXIS_TRIGGERRIGHT, 32767, exploration).empty() &&
            t.axis(1, SDL_CONTROLLER_AXIS_RIGHTX, 32767, exploration).empty(), "RT and right stick send no key in Part A");
    }
    {   // Removal and focus loss release what is held.
        T t;
        t.button(1, SDL_CONTROLLER_BUTTON_DPAD_UP, true, exploration);
        t.axis(1, SDL_CONTROLLER_AXIS_LEFTX, -30000, exploration);
        t.button(2, SDL_CONTROLLER_BUTTON_A, true, exploration);
        const auto removed = t.removed(1);
        check(removed.size() == 2 && !removed[0].down && !removed[1].down, "removal releases its keys");
        check(t.button(1, SDL_CONTROLLER_BUTTON_DPAD_UP, false, exploration).empty(), "late release of a removed controller");
        check(t.releaseAll() == std::vector{brk(space)} && t.idle(), "focus loss releases every key");
        check(t.button(2, SDL_CONTROLLER_BUTTON_A, false, exploration).empty(), "release after focus loss ignored");
        check(t.button(2, SDL_CONTROLLER_BUTTON_A, true, exploration) == std::vector{make(space)}, "a fresh press after focus loss");
    }
    {   // English strafe carries Ctrl on its own events; the last strafe break releases Left Ctrl.
        T t;
        const ControllerKeyEvent sl{SDLK_LEFT, SDL_SCANCODE_F13, KMOD_LCTRL}, sr{SDLK_RIGHT, SDL_SCANCODE_F14, KMOD_LCTRL};
        const ControllerKeyEvent ctrl{SDLK_LCTRL, SDL_SCANCODE_LCTRL, KMOD_NONE};
        check(t.button(1, SDL_CONTROLLER_BUTTON_LEFTSHOULDER, true, exploration) == std::vector{make(sl)}, "LB make");
        check(t.button(1, SDL_CONTROLLER_BUTTON_RIGHTSHOULDER, true, exploration) == std::vector{make(sr)}, "RB make");
        check(t.button(1, SDL_CONTROLLER_BUTTON_RIGHTSHOULDER, false, exploration) == std::vector{brk(sr)},
            "no Ctrl release while the other strafe is held");
        check(t.button(1, SDL_CONTROLLER_BUTTON_LEFTSHOULDER, false, exploration) == std::vector{brk(sl), brk(ctrl)},
            "the last strafe break releases Left Ctrl");
        t.button(1, SDL_CONTROLLER_BUTTON_LEFTSHOULDER, true, exploration);
        check(t.releaseAll() == std::vector{brk(sl), brk(ctrl)} && t.idle(), "focus loss releases Ctrl after the strafe");
        check(t.button(1, SDL_CONTROLLER_BUTTON_A, true, exploration) == std::vector{make(space)} &&
            t.button(1, SDL_CONTROLLER_BUTTON_A, false, exploration) == std::vector{brk(space)}, "other keys send no Ctrl release");
    }
    {   // Two controls on one physical key (possible in Russian across contexts) make and break it once.
        T t;
        auto shared = std::make_shared<XeenKeyboard>(*russianKeyboard());
        shared->sets["COMBAT"].aliases.insert(shared->sets["COMBAT"].aliases.begin(), {0xa8, 'c'}); // B key casts in combat.
        const auto ruCombat = xeenControllerContext(contextOf(MainScreen::Combat), shared);
        const auto ruExploration = xeenControllerContext(contextOf(MainScreen::Exploration), shared);
        const ControllerKeyEvent b{SDLK_b, SDL_SCANCODE_B};
        check(t.button(1, SDL_CONTROLLER_BUTTON_X, true, ruCombat) == std::vector{make(b)}, "X in combat is the B key");
        check(t.axis(1, SDL_CONTROLLER_AXIS_TRIGGERLEFT, 30000, ruExploration).empty(), "LT in exploration holds the same key");
        check(t.button(1, SDL_CONTROLLER_BUTTON_X, false, ruExploration).empty(), "the key stays made while LT holds it");
        check(t.axis(1, SDL_CONTROLLER_AXIS_TRIGGERLEFT, 0, ruExploration) == std::vector{brk(b)}, "one physical break");
    }
    std::cout << "Controller tracker passed\n";
}

// ---- Production SDL loop -------------------------------------------------

void push(SDL_Event event) { check(SDL_PushEvent(&event) == 1, "SDL_PushEvent failed"); }
void button(int which, SDL_GameControllerButton b, bool down) {
    SDL_Event e{};e.type = down ? SDL_CONTROLLERBUTTONDOWN : SDL_CONTROLLERBUTTONUP;
    e.cbutton.which = which;e.cbutton.button = Uint8(b);e.cbutton.state = down ? SDL_PRESSED : SDL_RELEASED;push(e);
}
void tapButton(SDL_GameControllerButton b, int which = 7) { button(which, b, true);button(which, b, false); }
void axis(int which, SDL_GameControllerAxis a, int value) {
    SDL_Event e{};e.type = SDL_CONTROLLERAXISMOTION;e.caxis.which = which;e.caxis.axis = Uint8(a);e.caxis.value = Sint16(value);push(e);
}
void removed(int which) { SDL_Event e{};e.type = SDL_CONTROLLERDEVICEREMOVED;e.cdevice.which = which;push(e); }
void focusLost() { SDL_Event e{};e.type = SDL_WINDOWEVENT;e.window.event = SDL_WINDOWEVENT_FOCUS_LOST;push(e); }
// Keyboard twins go through SDL_PushEvent too, so that both get SDL's tick.
void key(SDL_Keycode code, bool down, Uint16 mod = 0) {
    SDL_Event e{};e.type = down ? SDL_KEYDOWN : SDL_KEYUP;e.key.state = down ? SDL_PRESSED : SDL_RELEASED;
    e.key.keysym.sym = code;e.key.keysym.scancode = SDL_GetScancodeFromKey(code);e.key.keysym.mod = mod;push(e);
}
void tapKey(SDL_Keycode code) { key(code, true);key(code, false); }
void quit() { SDL_Event e{};e.type = SDL_QUIT;push(e); }

std::string label(const PlayerAction &action) {
    if (const auto *nav = std::get_if<NavigationAction>(&action))
        switch (*nav) {
        case NavigationAction::MoveForward: return "fwd";
        case NavigationAction::MoveBackward: return "back";
        case NavigationAction::TurnLeft: return "left";
        case NavigationAction::TurnRight: return "right";
        case NavigationAction::StrafeLeft: return "sl";
        case NavigationAction::StrafeRight: return "sr";
        }
    if (std::holds_alternative<InteractionAction>(action)) return "space";
    if (std::holds_alternative<AttackAction>(action)) return "attack";
    if (std::holds_alternative<CancelInteractionAction>(action)) return "esc";
    if (std::holds_alternative<CastSpellAction>(action)) return "cast";
    if (std::holds_alternative<ShootAction>(action)) return "shoot";
    if (std::holds_alternative<BashAction>(action)) return "bash";
    if (std::holds_alternative<BlockAction>(action)) return "block";
    if (std::holds_alternative<RestAction>(action)) return "rest";
    if (std::holds_alternative<RevisitCompletedAction>(action)) return "run";
    if (std::holds_alternative<WaitAction>(action)) return "wait";
    if (std::holds_alternative<MapAction>(action)) return "map";
    if (std::holds_alternative<ControlPanelAction>(action)) return "panel";
    if (std::holds_alternative<QuickFightAction>(action)) return "quickfight";
    if (std::holds_alternative<AcknowledgeAction>(action)) return "ack";
    if (std::holds_alternative<YesAction>(action)) return "yes";
    if (std::holds_alternative<NoAction>(action)) return "no";
    if (const auto *k = std::get_if<DialogKeyAction>(&action)) return "key:" + std::to_string(k->key);
    if (const auto *t = std::get_if<TextInputAction>(&action)) return "text:" + t->text;
    if (const auto *k = std::get_if<ExplorationKeyAction>(&action)) return "ex:" + std::to_string(k->key);
    return "?" + std::to_string(action.index());
}
std::string joined(const std::vector<std::string> &values) {
    std::string text;
    for (const auto &value : values) text += (text.empty() ? "" : ",") + value;
    return text;
}

// Fake readiness and clock, the real production event queue and loop.
struct Loop {
    MainScreen screen = MainScreen::Exploration;
    std::shared_ptr<const DialogInput> dialog;
    std::shared_ptr<const XeenKeyboard> keyboard;
    bool queueable = true, ready = true, redraw = false, retainEventKeys = false;
    bool contextPerAction = false; // As the title's name entry: a new context id at every key.
    bool newEntryOnEnter = false;  // Enter opens another entry of the same kind.
    bool cursor = false;           // Draw the original-style cursor (2x2).
    std::function<bool()> finished;
    std::function<void(const PlayerAction &)> observe;
    std::uint64_t context = 1, epoch = 1, presented = 0, clock = 0;
    std::optional<std::uint64_t> deadline; // After the clock: the loop's presentation deadline.
    std::string field; // A text entry's value, from the delivered actions.
    unsigned stage = 0;
    std::vector<std::string> delivered;
    std::vector<std::uint64_t> deliveredAt;
    std::function<void()> duringPresentation;
    std::function<void()> inIdle;
    void fail(const std::string &message) const {
        throw std::runtime_error(message + " stage=" + std::to_string(stage) + " delivered=" + joined(delivered));
    }
    void expect(bool value, const std::string &message) const { if (!value) fail(message); }
    void run(const char *name, const std::function<void(Loop &)> &script) {
        IndexedFrame frame;frame.width = 320;frame.height = 200;frame.pixels.resize(64000);
        SdlWindow::FrameUpdateHandler handler = [](const auto &) -> std::optional<IndexedFrame> {
            throw std::runtime_error("unversioned action");
        };
        handler.protectAllKeys = true;
        handler.keyboard = keyboard;
        handler.displayedInput = [&] { return std::optional<std::uint64_t>{epoch}; };
        handler.presentationClock = [&] { return clock; };
        if (deadline) handler.nextPresentationDeadline = [&] { return std::optional<std::uint64_t>{clock + *deadline}; };
        if (finished) handler.finished = finished;
        if (cursor) handler.cursorImage = [] { IndexedFrame image(2, 2, {1, 1, 1, 1});image.palette[3] = 255;return image; };
        handler.inputContext = [&](const auto &) {
            InputContext c;c.contextId = context;c.acceptsQueuedInput = queueable;c.readyForAction = ready && presented == epoch;
            c.mainScreen = screen;c.dialog = dialog;c.retainEventKeys = retainEventKeys;return c;
        };
        handler.framePresented = [&](const auto &) {
            presented = epoch;
            if (const auto hook = duringPresentation) hook();
        };
        handler.withPresentedInput = [&](const PlayerAction &action, auto token, const auto &) -> std::optional<IndexedFrame> {
            if (token != epoch) return {};
            delivered.push_back(label(action));deliveredAt.push_back(clock);
            if (const auto *t = std::get_if<TextInputAction>(&action)) field += t->text;
            if (const auto *k = std::get_if<DialogKeyAction>(&action); k && k->key == 8 && !field.empty()) field.pop_back();
            if (observe) observe(action);
            if (contextPerAction) ++context;
            if (const auto *k = std::get_if<DialogKeyAction>(&action); newEntryOnEnter && k && k->key == InputKey::Enter) {
                field.clear();++context;
            }
            ++epoch;return frame;
        };
        const bool ok = SdlWindow().showInteractive(frame, name, handler, [] { return true; }, [&]() -> std::optional<IndexedFrame> {
            expect(stage < 80, "controller loop test timeout");
            if (inIdle) inIdle();
            redraw = false;script(*this);++stage;
            return redraw ? std::optional<IndexedFrame>{frame} : std::nullopt;
        });
        expect(ok, "loop failed");
        std::cout << name << " passed\n";
    }
};

void optionOff() {
    SdlWindow::useControllerSetting(false);
    Loop loop;
    loop.inIdle = [&] {
        loop.expect(!SDL_WasInit(SDL_INIT_GAMECONTROLLER) && !SDL_WasInit(SDL_INIT_JOYSTICK), "option off started a controller subsystem");
        loop.expect(!SdlController::instance().enabled(), "option off enabled the controller");
    };
    loop.run("option-off-no-subsystem-no-action", [](Loop &l) {
        if (l.stage == 0) { tapButton(SDL_CONTROLLER_BUTTON_DPAD_UP);tapButton(SDL_CONTROLLER_BUTTON_A);axis(7, SDL_CONTROLLER_AXIS_LEFTY, -32000); }
        else if (l.stage == 4) { l.expect(l.delivered.empty(), "option off produced an action");quit(); }
    });
}

// A held D-pad walks the steps of the held key and none after release.
void heldDpadMatchesHeldKey() {
    std::vector<std::vector<std::uint64_t>> times;std::vector<std::vector<std::string>> actions;
    for (const bool controller : {false, true}) {
        Loop loop;
        loop.run(controller ? "held-dpad-walk" : "held-key-walk", [&](Loop &l) {
            const auto down = [&](bool d) { if (controller) button(7, SDL_CONTROLLER_BUTTON_DPAD_UP, d); else key(SDLK_UP, d); };
            if (l.stage == 0) down(true);
            else if (l.stage == 2) l.clock = 499999999;
            else if (l.stage == 4) l.clock = 500000000;
            else if (l.stage == 6) l.clock += 100000000;
            else if (l.stage == 8) l.clock += 100000000;
            else if (l.stage == 10) { down(false);l.clock += 2000000000; }
            else if (l.stage == 14) { times.push_back(l.deliveredAt);actions.push_back(l.delivered);quit(); }
        });
    }
    check(actions[0] == actions[1] && times[0] == times[1], "held D-pad differs from held key: key=" + joined(actions[0]) +
        " controller=" + joined(actions[1]));
    check(actions[1].size() >= 3, "held D-pad did not repeat: " + joined(actions[1]));
}

void strafeChords() {
    {   Loop loop;
        loop.run("forward-held-with-LB", [](Loop &l) {
            if (l.stage == 0) button(7, SDL_CONTROLLER_BUTTON_DPAD_UP, true);
            else if (l.stage == 2) tapButton(SDL_CONTROLLER_BUTTON_LEFTSHOULDER);
            else if (l.stage == 4) l.clock = 600000000;
            else if (l.stage == 6) { button(7, SDL_CONTROLLER_BUTTON_DPAD_UP, false);l.clock += 2000000000; }
            else if (l.stage == 9) {
                l.expect(l.delivered.size() >= 3 && l.delivered[0] == "fwd" && l.delivered[1] == "sl" && l.delivered[2] == "fwd",
                    "forward held with LB must strafe and keep walking forward");
                l.expect(std::count(l.delivered.begin(), l.delivered.end(), "sl") == 1, "LB repeated");
                quit();
            }
        });
    }
    {   Loop loop;
        loop.run("dpad-left-with-LB", [](Loop &l) {
            if (l.stage == 0) button(7, SDL_CONTROLLER_BUTTON_DPAD_LEFT, true);
            else if (l.stage == 2) button(7, SDL_CONTROLLER_BUTTON_LEFTSHOULDER, true);
            else if (l.stage == 4) button(7, SDL_CONTROLLER_BUTTON_DPAD_LEFT, false); // LB held, Left released.
            else if (l.stage == 6) l.clock = 600000000;
            else if (l.stage == 8) { button(7, SDL_CONTROLLER_BUTTON_LEFTSHOULDER, false);l.clock += 2000000000; }
            else if (l.stage == 11) {
                l.expect(l.delivered.size() >= 3 && l.delivered[0] == "left" && l.delivered[1] == "sl" && l.delivered[2] == "sl" &&
                    std::count(l.delivered.begin(), l.delivered.end(), "left") == 1,
                    "D-pad Left with LB: one turn, a strafe that keeps repeating after Left is released");
                quit();
            }
        });
    }
}

// A press made while a frame is presented is dropped as its key is.
void pressDuringPresentation() {
    std::vector<std::vector<std::string>> results;
    for (const bool controller : {false, true}) {
        Loop loop;loop.screen = MainScreen::None;loop.queueable = false;
        loop.dialog = dialogWith({InputKey::Enter, InputKey::Escape});
        loop.run(controller ? "controller-press-during-presentation" : "key-press-during-presentation", [&](Loop &l) {
            if (l.stage == 1) {
                ++l.epoch;l.redraw = true;
                l.duringPresentation = [&] { l.duringPresentation = {};if (controller) tapButton(SDL_CONTROLLER_BUTTON_A); else tapKey(SDLK_RETURN); };
            } else if (l.stage == 4) { l.expect(l.delivered.empty(), "stale press delivered");if (controller) tapButton(SDL_CONTROLLER_BUTTON_A); else tapKey(SDLK_RETURN); }
            else if (l.stage == 7) { results.push_back(l.delivered);quit(); }
        });
    }
    check(results[0] == results[1] && results[1] == std::vector<std::string>{"key:13"},
        "presentation fence differs: key=" + joined(results[0]) + " controller=" + joined(results[1]));
}

// With the option on the subsystem keeps SDL's queue between loops.
void queueSurvivesLoopEnd() {
    {   Loop first;
        first.run("queued-before-loop-end", [](Loop &l) {
            if (l.stage == 1) { quit();tapButton(SDL_CONTROLLER_BUTTON_A); }
        });
        first.expect(first.delivered.empty(), "the first loop delivered after its close");
    }
    Loop second;
    second.run("queued-for-next-loop", [](Loop &l) {
        if (l.stage == 3) { l.expect(l.delivered == std::vector<std::string>{"space"}, "queued controller key lost between loops");quit(); }
    });
}

void breakAfterContextChange() {
    Loop loop;
    loop.run("break-after-context-change", [](Loop &l) {
        if (l.stage == 0) button(7, SDL_CONTROLLER_BUTTON_A, true); // Space.
        else if (l.stage == 2) {
            l.expect(l.delivered == std::vector<std::string>{"space"}, "A held in exploration");
            ++l.context;l.queueable = false;l.screen = MainScreen::None;l.dialog = dialogWith({InputKey::Enter, InputKey::Escape});l.redraw = true;
        } else if (l.stage == 4) button(7, SDL_CONTROLLER_BUTTON_A, false); // Must break Space, not Enter.
        else if (l.stage == 6) { ++l.context;l.queueable = true;l.screen = MainScreen::Exploration;l.dialog = {};l.redraw = true; }
        else if (l.stage == 8) tapButton(SDL_CONTROLLER_BUTTON_A);
        else if (l.stage == 11) {
            l.expect(l.delivered == std::vector<std::string>{"space", "space"}, "Space stayed held after its break");quit();
        }
    });
}

void removalAndFocusLoss() {
    {   Loop loop;
        loop.run("removal-releases-held-key", [](Loop &l) {
            if (l.stage == 0) button(7, SDL_CONTROLLER_BUTTON_DPAD_UP, true);
            else if (l.stage == 2) removed(7);
            else if (l.stage == 4) l.clock = 3000000000;
            else if (l.stage == 7) {
                l.expect(l.delivered == std::vector<std::string>{"fwd"}, "a removed controller kept walking");
                tapButton(SDL_CONTROLLER_BUTTON_DPAD_UP, 8);
            } else if (l.stage == 10) { l.expect(l.delivered == std::vector<std::string>{"fwd", "fwd"}, "a new controller cannot walk");quit(); }
        });
    }
    {   Loop loop;
        loop.run("focus-loss-releases-held-key", [](Loop &l) {
            if (l.stage == 0) button(7, SDL_CONTROLLER_BUTTON_DPAD_UP, true);
            else if (l.stage == 2) focusLost();
            else if (l.stage == 4) l.clock = 3000000000;
            else if (l.stage == 6) {
                l.expect(l.delivered == std::vector<std::string>{"fwd"}, "focus loss kept walking");
                // Still holding the D-pad, which focus loss released as a keyboard
                // key is: the stick is a fresh press of the same key.
                SDL_Event e{};e.type = SDL_WINDOWEVENT;e.window.event = SDL_WINDOWEVENT_FOCUS_GAINED;push(e);
                axis(7, SDL_CONTROLLER_AXIS_LEFTY, -30000);
            } else if (l.stage == 9) {
                l.expect(l.delivered == std::vector<std::string>{"fwd", "fwd"}, "a press after focus loss was merged into a released key");
                axis(7, SDL_CONTROLLER_AXIS_LEFTY, 0);button(7, SDL_CONTROLLER_BUTTON_DPAD_UP, false);
            } else if (l.stage == 11) tapButton(SDL_CONTROLLER_BUTTON_DPAD_UP);
            else if (l.stage == 14) { l.expect(l.delivered == std::vector<std::string>{"fwd", "fwd", "fwd"}, "press after focus loss lost");quit(); }
        });
    }
}

void noisySticks() {
    Loop loop;loop.screen = MainScreen::None;loop.queueable = false;
    loop.dialog = dialogWith({InputKey::Up, InputKey::Left, InputKey::Escape});
    loop.run("noisy-stick-threshold-and-diagonal", [](Loop &l) {
        if (l.stage == 0) for (int value : {-16000, -16500, -15800, -16700, -12000, -16400, -11000, -17000})
            axis(7, SDL_CONTROLLER_AXIS_LEFTY, value);
        else if (l.stage == 3) { l.expect(l.delivered == std::vector<std::string>{"key:" + std::to_string(InputKey::Up)}, "threshold noise");axis(7, SDL_CONTROLLER_AXIS_LEFTY, 0); }
        else if (l.stage == 5) {
            const int noise[] = {0, 900, -700, 1500, -1200, 400, -1500, 1100};
            for (unsigned i = 0; i < 40; ++i) {
                axis(7, SDL_CONTROLLER_AXIS_LEFTX, -23170 + noise[i % 8]);axis(7, SDL_CONTROLLER_AXIS_LEFTY, -23170 - noise[(i + 5) % 8]);
            }
        } else if (l.stage == 8) {
            l.expect(l.delivered == std::vector<std::string>{"key:" + std::to_string(InputKey::Up), "key:" + std::to_string(InputKey::Left)},
                "diagonal noise made phantom presses or the wrong key");
            for (int m : {16000, 9000, 3000, 0}) { axis(7, SDL_CONTROLLER_AXIS_LEFTX, -m);axis(7, SDL_CONTROLLER_AXIS_LEFTY, -m); }
        }
        else if (l.stage == 10) { l.expect(l.delivered.size() == 2, "centering pressed");quit(); }
    });
}

void deadZoneBursts() {
    for (const bool anyKey : {false, true}) {
        Loop loop;
        if (anyKey) { loop.screen = MainScreen::None;loop.queueable = false;loop.dialog = dialogWith({}, false, true); }
        loop.run(anyKey ? "dead-zone-burst-any-key" : "dead-zone-burst-exploration", [](Loop &l) {
            if (l.stage == 0) for (int i = 0; i < 1000; ++i) {
                SDL_Event e{};e.type = SDL_JOYAXISMOTION;e.jaxis.which = 7;e.jaxis.axis = Uint8(i % 4);e.jaxis.value = Sint16((i % 7) * 300 - 900);push(e);
                axis(7, SDL_GameControllerAxis(i % 6), (i % 9) * 400 - 1600);
            } else if (l.stage == 4) { l.expect(l.delivered.empty(), "dead-zone motion made a pass");quit(); }
        });
    }
}

void yesNoAndCombat() {
    {   Loop loop;loop.screen = MainScreen::None;loop.queueable = false;
        loop.dialog = dialogWith({'y', 'n', InputKey::Escape});
        loop.run("yes-no-prompt", [](Loop &l) {
            if (l.stage == 0) tapButton(SDL_CONTROLLER_BUTTON_A);
            else if (l.stage == 3) tapButton(SDL_CONTROLLER_BUTTON_X); // Letters send nothing in dialogs.
            else if (l.stage == 6) tapButton(SDL_CONTROLLER_BUTTON_B);
            else if (l.stage == 9) { l.expect(l.delivered == std::vector<std::string>{"key:121", "key:27"}, "Yes/No rule");quit(); }
        });
    }
    {   Loop loop;loop.screen = MainScreen::Combat;
        loop.run("combat-keys", [](Loop &l) {
            if (l.stage == 0) tapButton(SDL_CONTROLLER_BUTTON_A);
            else if (l.stage == 2) tapButton(SDL_CONTROLLER_BUTTON_Y);
            else if (l.stage == 4) tapButton(SDL_CONTROLLER_BUTTON_LEFTSHOULDER);
            else if (l.stage == 6) tapButton(SDL_CONTROLLER_BUTTON_DPAD_UP); // Refused by combat, as the key is.
            else if (l.stage == 8) { axis(7, SDL_CONTROLLER_AXIS_TRIGGERLEFT, 30000);axis(7, SDL_CONTROLLER_AXIS_TRIGGERLEFT, 0); }
            else if (l.stage == 11) { l.expect(l.delivered == std::vector<std::string>{"attack", "quickfight", "fwd", "block"}, "combat column");quit(); }
        });
    }
}

void russianLoop() {
    Loop loop;loop.keyboard = russianKeyboard();
    loop.run("russian-controller-keys", [](Loop &l) {
        if (l.stage == 0) tapButton(SDL_CONTROLLER_BUTTON_LEFTSHOULDER);
        else if (l.stage == 2) tapButton(SDL_CONTROLLER_BUTTON_X);
        else if (l.stage == 4) tapButton(SDL_CONTROLLER_BUTTON_LEFTSTICK); // W: nothing in Russian.
        else if (l.stage == 6) { l.screen = MainScreen::Combat;l.redraw = true; } // A press resolves against the frame on screen.
        else if (l.stage == 8) tapButton(SDL_CONTROLLER_BUTTON_A);
        else if (l.stage == 11) { l.expect(l.delivered == std::vector<std::string>{"sl", "cast", "attack"}, "Russian controller keys");quit(); }
    });
}

} // namespace

// Review S1: a strafe tap must not leave the loop's held modifiers at Ctrl,
// or a held letter in a retained Event context repeats as a control code.
void strafeReleasesCtrl() {
    Loop loop;loop.retainEventKeys = true;
    loop.run("strafe-tap-keeps-held-letter", [](Loop &l) {
        if (l.stage == 0) button(7, SDL_CONTROLLER_BUTTON_Y, true); // S: Shoot.
        else if (l.stage == 2) tapButton(SDL_CONTROLLER_BUTTON_LEFTSHOULDER);
        else if (l.stage == 4) l.clock = 600000000;
        else if (l.stage == 6) l.clock += 100000000;
        else if (l.stage == 8) { button(7, SDL_CONTROLLER_BUTTON_Y, false);l.clock += 2000000000; }
        else if (l.stage == 11) {
            l.expect(l.delivered == std::vector<std::string>{"shoot", "sl", "shoot", "shoot"}, "held Shoot repeated with Ctrl after a strafe");
            quit();
        }
    });
}

// Records the key events the controller pushes, as the loop receives them.
std::vector<std::pair<Uint32, SDL_Scancode>> pushedKeys;
int recordKeys(void *, SDL_Event *event) {
    if (event->type == SDL_KEYDOWN || event->type == SDL_KEYUP) pushedKeys.push_back({event->type, event->key.keysym.scancode});
    return 1;
}

void optionOffWhileHeld() {
    {   Loop held;
        held.run("option-on-key-held", [](Loop &l) {
            if (l.stage == 0) button(7, SDL_CONTROLLER_BUTTON_DPAD_UP, true);
            else if (l.stage == 3) { l.expect(l.delivered == std::vector<std::string>{"fwd"}, "held key");quit(); }
        });
    }
    SdlWindow::useControllerSetting(false);
    pushedKeys.clear();SDL_AddEventWatch(&recordKeys, nullptr);
    {   Loop off;
        off.inIdle = [&] { off.expect(!SDL_WasInit(SDL_INIT_GAMECONTROLLER), "option off kept the subsystem"); };
        off.run("option-off-releases-held-key", [](Loop &l) { if (l.stage == 2) quit(); });
    }
    SDL_DelEventWatch(&recordKeys, nullptr);
    check(pushedKeys == std::vector<std::pair<Uint32, SDL_Scancode>>{{SDL_KEYUP, SDL_SCANCODE_UP}},
        "turning the option off did not release the held key");
    SdlWindow::useControllerSetting(true);
    {   Loop again;
        again.run("option-on-again-fresh-press", [](Loop &l) {
            if (l.stage == 0) tapButton(SDL_CONTROLLER_BUTTON_DPAD_UP);
            else if (l.stage == 3) { l.expect(l.delivered == std::vector<std::string>{"fwd"}, "a button held before the option went off stays dead");quit(); }
        });
    }
}

void nestedLoopRestoresSnapshot() {
    Loop outer;
    bool checked = false;
    outer.observe = [&](const PlayerAction &) {
        if (checked) return;
        checked = true;
        Loop inner;inner.screen = MainScreen::None;inner.queueable = false;inner.dialog = dialogWith({InputKey::Enter});
        inner.run("nested-loop", [](Loop &l) {
            if (l.stage == 1) {
                const auto snapshot = SdlController::instance().published();
                l.expect(snapshot && snapshot->dialog && snapshot->screen == MainScreen::None, "nested loop did not publish its own context");
                quit();
            }
        });
        const auto restored = SdlController::instance().published();
        check(restored && !restored->dialog && restored->screen == MainScreen::Exploration, "nested loop did not restore its caller's snapshot");
    };
    outer.run("nested-loop-caller", [&](Loop &l) {
        if (l.stage == 0) tapButton(SDL_CONTROLLER_BUTTON_A);
        else if (l.stage == 4) {
            l.expect(checked && l.delivered == std::vector<std::string>{"space"}, "outer loop action");
            const auto snapshot = SdlController::instance().published();
            l.expect(snapshot && snapshot->screen == MainScreen::Exploration, "outer snapshot after the nested loop");
            quit();
        }
    });
    check(!SdlController::instance().published(), "the outermost loop did not withdraw its snapshot");
}

// Review S2: SDL's own path from a joystick to controller events, hot-plug
// open and close, on SDL's virtual joystick (no hardware).
int virtualIndex(SDL_JoystickID id) {
    for (int index = 0; index < SDL_NumJoysticks(); ++index)
        if (SDL_JoystickGetDeviceInstanceID(index) == id) return index;
    return -1;
}
void virtualController() {
    // The dummy video driver never gives the window keyboard focus, and SDL
    // drops joystick presses without it; a test-only hint lets them through.
    SDL_SetHint(SDL_HINT_JOYSTICK_ALLOW_BACKGROUND_EVENTS, "1");
    SDL_JoystickID id = -1;bool unsupported = false;
    const auto attach = [&] {
        SDL_VirtualJoystickDesc desc{};
        desc.version = SDL_VIRTUAL_JOYSTICK_DESC_VERSION;desc.type = SDL_JOYSTICK_TYPE_GAMECONTROLLER;
        desc.naxes = SDL_CONTROLLER_AXIS_MAX;desc.nbuttons = SDL_CONTROLLER_BUTTON_DPAD_RIGHT + 1;desc.name = "MMModern test controller";
        const int index = SDL_JoystickAttachVirtualEx(&desc);
        if (index < 0) { unsupported = true;return; }
        id = SDL_JoystickGetDeviceInstanceID(index);
    };
    Loop loop;
    loop.run("virtual-controller-hot-plug", [&](Loop &l) {
        if (unsupported) { if (l.stage < 100) { quit();l.stage = 100; } return; }
        if (l.stage == 0) attach();
        else if (l.stage == 3) {
            l.expect(SDL_IsGameController(virtualIndex(id)) && SDL_GameControllerFromInstanceID(id), "the loop did not open the attached controller");
            l.expect(SDL_JoystickSetVirtualButton(SDL_JoystickFromInstanceID(id), SDL_CONTROLLER_BUTTON_DPAD_UP, 1) == 0, "virtual press");
        } else if (l.stage == 6) {
            l.expect(l.delivered == std::vector<std::string>{"fwd"}, "a virtual D-pad press did not walk one step");
            l.expect(SDL_JoystickDetachVirtual(virtualIndex(id)) == 0, "virtual detach");
            l.clock = 3000000000;
        } else if (l.stage == 9) {
            l.expect(l.delivered == std::vector<std::string>{"fwd"}, "a detached controller kept walking");
            l.expect(!SDL_GameControllerFromInstanceID(id), "the loop did not close the removed controller");
            attach();
        } else if (l.stage == 12) {
            l.expect(SDL_GameControllerFromInstanceID(id), "the reattached controller was not opened");
            quit();
        }
    });
    if (unsupported) { std::cout << "SDL virtual joystick unavailable in this SDL2 build: virtual controller test skipped\n";return; }
    SdlWindow::useControllerSetting(false);
    Loop off;
    off.inIdle = [&] { off.expect(!SDL_WasInit(SDL_INIT_GAMECONTROLLER) && !SDL_WasInit(SDL_INIT_JOYSTICK), "option off kept the subsystem"); };
    off.run("virtual-controller-option-off", [](Loop &l) { if (l.stage == 2) quit(); });
    SdlWindow::useControllerSetting(true);
    SDL_SetHint(SDL_HINT_JOYSTICK_ALLOW_BACKGROUND_EVENTS, nullptr);
}


// ---- Part B: text entry (decision 6) ------------------------------------

void textEntryUnits() {
    ControllerContext english;english.textEntry = english.stringEntry = true;
    const auto letters = xeenControllerCharacters(english);
    check(letters.size() == 37 && letters.front() == 'A' && letters[25] == 'Z' && letters[26] == ' ' &&
        letters[27] == '0' && letters.back() == '9', "English string entries cycle A-Z, space, 0-9");
    ControllerContext number;number.textEntry = true;
    const auto digits = xeenControllerCharacters(number);
    check(digits.size() == 10 && digits.front() == '0' && digits.back() == '9', "number entries cycle digits");
    check(xeenControllerStep(letters, std::nullopt, true) == 'A' && xeenControllerStep(letters, std::nullopt, false) == '9',
        "Up adds the first character, Down the last");
    check(xeenControllerStep(letters, 'Z', true) == ' ' && xeenControllerStep(letters, '9', true) == 'A' &&
        xeenControllerStep(letters, 'A', false) == '9', "the cycle wraps");
    const auto typedA = xeenControllerTypedKey('A', english);
    check(typedA && typedA->key == ControllerKeyEvent{SDLK_a, SDL_SCANCODE_A} && typedA->text == "A", "English A: key down and text");
    const auto typed0 = xeenControllerTypedKey('0', number);
    check(typed0 && typed0->key == ControllerKeyEvent{SDLK_0, SDL_SCANCODE_0} && typed0->text == "0", "English 0");
    ControllerContext russian = english;russian.keyboard = russianKeyboard();
    const auto cyrillic = xeenControllerCharacters(russian);
    check(cyrillic.size() > 11 && std::is_sorted(cyrillic.begin(), cyrillic.end() - 11) && cyrillic.front() >= 0x80 &&
        cyrillic[cyrillic.size() - 11] == ' ', "Russian entries cycle the Russian program's letters, space, digits");
    const auto typedC = xeenControllerTypedKey(0xe1, russian);
    check(typedC && typedC->key == ControllerKeyEvent{SDLK_c, SDL_SCANCODE_C} && typedC->text.empty(),
        "a Russian letter is its key, without text input");
    for (const auto c : cyrillic) check(bool(xeenControllerTypedKey(c, russian)), "every Russian cycle character is typable");
    // The setup screen's path line is free text: the controller sends its keys.
    auto path = std::make_shared<DialogInput>();path->textEntry = true;path->freeText = true;
    path->keys = {InputKey::Enter, InputKey::Escape, InputKey::Up, InputKey::Down};
    const auto pathContext = xeenControllerContext(contextOf(MainScreen::None, path), nullptr);
    check(!pathContext.textEntry && xeenControllerKey(C::Up, pathContext) == std::optional<unsigned>(InputKey::Up),
        "the path line has no character cycle");
    // The tracker hands text-entry presses to SdlController instead of keys.
    ControllerTracker t;
    const auto entry = xeenControllerContext(contextOf(MainScreen::None, dialogWith({InputKey::Enter, InputKey::Escape, 8}, true)), nullptr);
    check(t.button(1, SDL_CONTROLLER_BUTTON_DPAD_UP, true, entry).empty() && t.axis(1, SDL_CONTROLLER_AXIS_LEFTY, -30000, entry).empty() &&
        t.button(1, SDL_CONTROLLER_BUTTON_A, true, entry).empty(), "text entry presses make no key");
    const auto presses = t.takeTextPresses();
    check(presses.size() == 2 && presses[0].control == C::Up && presses[1].control == C::A, "D-pad and stick merged; A typed in order");
    t.button(1, SDL_CONTROLLER_BUTTON_DPAD_UP, false, entry);t.axis(1, SDL_CONTROLLER_AXIS_LEFTY, 0, entry);
    check(t.button(1, SDL_CONTROLLER_BUTTON_A, false, entry).empty() && t.takeTextPresses().empty(), "breaks type nothing");
    // RT is merged like a control; the right stick is read, not translated.
    check(t.axis(1, SDL_CONTROLLER_AXIS_TRIGGERRIGHT, 30000, entry).empty() && t.axis(2, SDL_CONTROLLER_AXIS_TRIGGERRIGHT, 30000, entry).empty(),
        "RT sends no key");
    t.axis(1, SDL_CONTROLLER_AXIS_TRIGGERRIGHT, 12000, entry); // Above a third: still held.
    t.axis(1, SDL_CONTROLLER_AXIS_TRIGGERRIGHT, 0, entry);
    check(t.takeClicks() == std::vector<bool>{true}, "two controllers press once");
    t.removed(2);
    check(t.takeClicks() == std::vector<bool>{false}, "removal releases RT");
    t.axis(3, SDL_CONTROLLER_AXIS_RIGHTX, -20000, entry);t.axis(4, SDL_CONTROLLER_AXIS_RIGHTY, 30000, entry);
    check(t.pointerAxes() == std::pair<int, int>{0, 30000}, "the most deflected right stick moves the pointer");
    std::cout << "Controller text entry and pointer units passed" << std::endl;
}

std::string bytes(std::initializer_list<unsigned char> characters) { return std::string(characters.begin(), characters.end()); }

void textEntryLoop(bool russian, bool retained, bool contextPerAction = false) {
    Loop loop;loop.screen = MainScreen::None;loop.queueable = false;loop.retainEventKeys = retained;
    loop.contextPerAction = contextPerAction;loop.newEntryOnEnter = true;
    if (russian) loop.keyboard = russianKeyboard();
    loop.dialog = retained ? dialogWith({8, 127, InputKey::Escape, InputKey::Enter}, true) :
        dialogWith({InputKey::Escape, InputKey::Enter, 8, InputKey::Left}, true);
    ControllerContext context;context.textEntry = context.stringEntry = true;context.keyboard = loop.keyboard;
    const auto characters = xeenControllerCharacters(context);
    // Event word entries take letters from key events, which the CD program lowercases.
    const auto shown = [&](unsigned char c) { return !russian && retained && c >= 'A' && c <= 'Z' ? (unsigned char)(c + 32) : c; };
    pushedKeys.clear();SDL_AddEventWatch(&recordKeys, nullptr);
    const std::string name = std::string(russian ? "russian" : "english") + (retained ? "-event-word" : "-strict") +
        (contextPerAction ? "-new-context-per-key" : "") + "-text-entry";
    loop.run(name.c_str(), [&](Loop &l) {
        if (l.stage == 0) tapButton(SDL_CONTROLLER_BUTTON_DPAD_UP);
        else if (l.stage == 4) {
            l.expect(l.field == bytes({shown(characters[0])}), "the first Up adds the first character");
            tapButton(SDL_CONTROLLER_BUTTON_DPAD_UP);
        } else if (l.stage == 8) {
            l.expect(l.field == bytes({shown(characters[1])}), "Up changes the character the controller added");
            tapButton(SDL_CONTROLLER_BUTTON_DPAD_UP);tapButton(SDL_CONTROLLER_BUTTON_DPAD_UP); // A fast second press.
        } else if (l.stage == 14) {
            l.expect(l.field == bytes({shown(characters[3])}), "a fast second press was lost or doubled");
            tapButton(SDL_CONTROLLER_BUTTON_DPAD_RIGHT);axis(7, SDL_CONTROLLER_AXIS_LEFTY, 30000);axis(7, SDL_CONTROLLER_AXIS_LEFTY, 0);
        } else if (l.stage == 20) {
            l.expect(l.field == bytes({shown(characters[3]), shown(characters.back())}), "Right then Down (stick) adds the last character");
            tapButton(SDL_CONTROLLER_BUTTON_DPAD_LEFT);
        } else if (l.stage == 24) {
            l.expect(l.field == bytes({shown(characters[3])}), "Left deletes one character");
            tapButton(SDL_CONTROLLER_BUTTON_A);tapButton(SDL_CONTROLLER_BUTTON_DPAD_UP); // Up in the same batch as A.
        } else if (l.stage == 28) {
            l.expect(!l.delivered.empty() && l.delivered.back() == "key:13", "A confirms");
            l.expect(l.field.empty(), "a press after A reached the next entry"); // A opened another entry.
            l.expect(std::count(l.delivered.begin(), l.delivered.end(), "key:8") == 3, "Backspaces doubled or lost");
        } else if (l.stage == 30) tapButton(SDL_CONTROLLER_BUTTON_DPAD_UP);
        else if (l.stage == 34) {
            l.expect(l.field == bytes({shown(characters[0])}) && std::count(l.delivered.begin(), l.delivered.end(), "key:8") == 3,
                "a new entry continued the last one's character");
            quit();
        }
    });
    SDL_DelEventWatch(&recordKeys, nullptr);
    for (const auto &[type, scancode] : pushedKeys)
        check(scancode != SDL_SCANCODE_UP && scancode != SDL_SCANCODE_DOWN && scancode != SDL_SCANCODE_LEFT &&
            scancode != SDL_SCANCODE_RIGHT, "a text entry received a raw arrow");
}

void numberAndPathEntries() {
    {   Loop loop;loop.screen = MainScreen::None;loop.queueable = false;
        loop.dialog = dialogWith({InputKey::Enter, InputKey::Escape, 8, '0', '1', '2', '3', '4', '5', '6', '7', '8', '9'}, true);
        auto number = std::make_shared<DialogInput>(*loop.dialog);number->stringEntry = false;loop.dialog = number;
        loop.run("number-entry-digits", [](Loop &l) {
            if (l.stage == 0) tapButton(SDL_CONTROLLER_BUTTON_DPAD_UP);
            else if (l.stage == 4) tapButton(SDL_CONTROLLER_BUTTON_DPAD_UP);
            else if (l.stage == 9) {
                // The digit key acts; its text input then belongs to a stale frame, as on a keyboard.
                l.expect(l.delivered == std::vector<std::string>{"key:48", "key:8", "key:49"}, "number entry cycles digits");
                quit();
            }
        });
    }
    {   Loop loop;loop.screen = MainScreen::None;loop.queueable = false;
        auto path = std::make_shared<DialogInput>();path->textEntry = true;path->freeText = true;
        path->keys = {8, 22, InputKey::Enter, InputKey::Escape, InputKey::Up, InputKey::Down, InputKey::Left, InputKey::Right};
        loop.dialog = path;
        loop.run("setup-path-line-no-cycle", [](Loop &l) {
            if (l.stage == 0) tapButton(SDL_CONTROLLER_BUTTON_DPAD_UP);
            else if (l.stage == 3) {
                l.expect(l.delivered == std::vector<std::string>{"key:" + std::to_string(InputKey::Up)}, "the path line moved the focus");
                quit();
            }
        });
    }
}

// ---- Part B: the pointer (decision 5) -------------------------------------

void warp(int x, int y, PictureAspect aspect) {
    SDL_MouseButtonEvent at{};
    sdl_mouse_test::position(at, x, y, aspect);
    SDL_WarpMouseInWindow(sdl_mouse_test::window(), at.x, at.y);
}
void mouseClick(int x, int y, PictureAspect aspect) {
    SDL_Event e{};e.type = SDL_MOUSEBUTTONDOWN;e.button.button = SDL_BUTTON_LEFT;e.button.state = SDL_PRESSED;
    sdl_mouse_test::position(e.button, x, y, aspect);push(e);
    e.type = SDL_MOUSEBUTTONUP;e.button.state = SDL_RELEASED;push(e);
}
void rtTap() { axis(7, SDL_CONTROLLER_AXIS_TRIGGERRIGHT, 32767);axis(7, SDL_CONTROLLER_AXIS_TRIGGERRIGHT, 0); }

void pointerClicks(PictureAspect aspect) {
    const auto directory = std::filesystem::temp_directory_path() / "mmodern-m115-pointer";
    std::filesystem::create_directories(directory);
    const auto settings = directory / "window.ini";
    { std::ofstream out(settings); out << (aspect == PictureAspect::Square ? "aspect=square" : "aspect=4:3") << std::endl; }
    SdlWindow::useAspectSetting(settings);
    Loop loop;
    loop.run(aspect == PictureAspect::Square ? "stick-pointer-click-square" : "stick-pointer-click-4:3", [&](Loop &l) {
        if (l.stage == 0) warp(160, 100, aspect);
        else if (l.stage == 2) axis(7, SDL_CONTROLLER_AXIS_RIGHTX, 32767); // Takes the pointer at (160,100).
        else if (l.stage == 3) l.clock += 560000000;                        // 200 px/s: x 272.
        else if (l.stage == 4) axis(7, SDL_CONTROLLER_AXIS_RIGHTX, 0);
        else if (l.stage == 5) axis(7, SDL_CONTROLLER_AXIS_RIGHTY, -32767);
        else if (l.stage == 6) l.clock += 75000000;                         // y 85: on Cast.
        else if (l.stage == 7) { axis(7, SDL_CONTROLLER_AXIS_RIGHTY, 0);l.expect(l.delivered.empty(), "moving the pointer made a pass"); }
        else if (l.stage == 8) rtTap();
        else if (l.stage == 12) { l.expect(l.delivered == std::vector<std::string>{"cast"}, "RT did not click Cast at the pointer");mouseClick(272, 85, aspect); }
        else if (l.stage == 16) { l.expect(l.delivered == std::vector<std::string>{"cast", "cast"}, "the mouse hit another button");warp(247, 85, aspect); }
        else if (l.stage == 18) rtTap();
        else if (l.stage == 22) {
            l.expect(l.delivered == std::vector<std::string>{"cast", "cast", "shoot"}, "the mouse did not take the pointer back");
            quit();
        }
    });
    SdlWindow::useAspectSetting(std::nullopt);
    std::error_code ignored;std::filesystem::remove_all(directory, ignored);
}

void pointerWaitCap() {
    Loop loop;loop.deadline = 500000000;
    loop.run("pointer-wait-cap", [](Loop &l) {
        if (l.stage == 0) { axis(7, SDL_CONTROLLER_AXIS_RIGHTX, 4000);waits.clear();recordWaits = true; } // Inside the dead zone.
        else if (l.stage == 3) {
            l.expect(!waits.empty() && *std::min_element(waits.begin(), waits.end()) >= 400, "the dead zone capped the wait");
            waits.clear();axis(7, SDL_CONTROLLER_AXIS_RIGHTX, 30000);
        } else if (l.stage == 5) {
            l.expect(!waits.empty() && *std::max_element(waits.begin(), waits.end()) <= 8, "a moving stick did not cap the wait");
            recordWaits = false;axis(7, SDL_CONTROLLER_AXIS_RIGHTX, 0);
            l.expect(l.delivered.empty(), "moving the pointer made a pass");
            quit();
        }
    });
}

std::vector<Uint32> mouseEvents;
std::atomic<bool> recordingMouse{false};
int recordMouse(void *, SDL_Event *event) {
    if (!recordingMouse) return 1; // Not the focus loss of the window closing.
    if (event->type == SDL_MOUSEBUTTONDOWN || event->type == SDL_MOUSEBUTTONUP ||
        (event->type == SDL_WINDOWEVENT && event->window.event == SDL_WINDOWEVENT_FOCUS_LOST)) mouseEvents.push_back(event->type);
    return 1;
}

// RT held: the loop waits for the release without idle work; removing the
// controller or losing focus pushes the release.
void pointerReleaseOnLoss(bool focus) {
    auto &pointer = SdlController::instance().pointer();
    pointer.stick = true;pointer.known = true; // RT clicks at the stick's pointer.
    Loop loop;
    std::atomic<bool> finished{false};
    std::vector<std::chrono::steady_clock::time_point> stages;
    std::thread helper;
    mouseEvents.clear();recordingMouse = true;SDL_AddEventWatch(&recordMouse, nullptr);
    loop.run(focus ? "rt-held-focus-loss" : "rt-held-removal", [&](Loop &l) {
        stages.push_back(std::chrono::steady_clock::now());
        if (l.stage == 0) {
            axis(7, SDL_CONTROLLER_AXIS_TRIGGERRIGHT, 32767);
            helper = std::thread([&] {
                std::this_thread::sleep_for(std::chrono::milliseconds(300));
                if (focus) focusLost(); else removed(7);
                for (int i = 0; i < 30 && !finished; ++i) std::this_thread::sleep_for(std::chrono::milliseconds(100));
                if (!finished) quit(); // Never released: end the loop so that the test fails.
            });
        } else if (l.stage == 5) { recordingMouse = false;finished = true;quit(); }
    });
    finished = true;helper.join();
    SDL_DelEventWatch(&recordMouse, nullptr);
    check(stages.size() == 6, "idle work did not resume after the loss");
    std::chrono::steady_clock::duration longest{};
    for (std::size_t i = 1; i < stages.size(); ++i) longest = std::max(longest, stages[i] - stages[i - 1]);
    check(longest >= std::chrono::milliseconds(250), "idle work ran while RT held the mouse button");
    const std::vector<Uint32> expected = focus ?
        std::vector<Uint32>{SDL_MOUSEBUTTONDOWN, SDL_WINDOWEVENT, SDL_MOUSEBUTTONUP} : std::vector<Uint32>{SDL_MOUSEBUTTONDOWN, SDL_MOUSEBUTTONUP};
    check(mouseEvents == expected, "the controller did not push the release");
    std::cout << (focus ? "RT focus-loss release passed" : "RT removal release passed") << std::endl;
}


// ---- Part B fix round ------------------------------------------------------

// S1: an RT press taken in the batch that finishes a loop is left to the next
// loop, whose RT release then ends the hold: no frozen loop.
void rtAcrossLoops() {
    auto &pointer = SdlController::instance().pointer();
    pointer.stick = true;pointer.known = true;pointer.x = 160;pointer.y = 100;
    {   Loop first;bool finishing = false;
        first.finished = [&] { return finishing; };
        first.run("rt-press-as-a-loop-finishes", [&](Loop &l) {
            if (l.stage == 1) { axis(7, SDL_CONTROLLER_AXIS_TRIGGERRIGHT, 32767);finishing = true; }
            else if (l.stage > 3) quit();
        });
    }
    Loop second;
    std::atomic<bool> finished{false};
    std::thread helper;
    mouseEvents.clear();recordingMouse = true;SDL_AddEventWatch(&recordMouse, nullptr);
    second.run("rt-release-in-the-next-loop", [&](Loop &l) {
        if (l.stage == 0)
            helper = std::thread([&] {
                std::this_thread::sleep_for(std::chrono::milliseconds(300));
                axis(7, SDL_CONTROLLER_AXIS_TRIGGERRIGHT, 0);
                for (int i = 0; i < 30 && !finished; ++i) std::this_thread::sleep_for(std::chrono::milliseconds(100));
                if (!finished) quit(); // Frozen: end the loop so that the test fails.
            });
        else if (l.stage == 5) { recordingMouse = false;finished = true;quit(); }
    });
    finished = true;helper.join();
    SDL_DelEventWatch(&recordMouse, nullptr);
    check(second.stage >= 6, "the next loop froze on the RT press");
    check(!mouseEvents.empty() && mouseEvents.back() == SDL_MOUSEBUTTONUP, "the RT release was not pushed");
    std::cout << "RT across loops passed" << std::endl;
}

// N3: the pointer keeps its place and owner across loops.
void pointerAcrossLoops() {
    auto &pointer = SdlController::instance().pointer();
    pointer.stick = false;pointer.known = false;
    {   Loop first;
        first.run("pointer-moved-in-one-loop", [](Loop &l) {
            if (l.stage == 0) warp(160, 100, PictureAspect::FourThree);
            else if (l.stage == 2) axis(7, SDL_CONTROLLER_AXIS_RIGHTX, 32767); // Takes the pointer at (160,100).
            else if (l.stage == 3) l.clock += 560000000;
            else if (l.stage == 4) axis(7, SDL_CONTROLLER_AXIS_RIGHTX, 0);
            else if (l.stage == 5) axis(7, SDL_CONTROLLER_AXIS_RIGHTY, -32767);
            else if (l.stage == 6) l.clock += 75000000;
            else if (l.stage == 7) { axis(7, SDL_CONTROLLER_AXIS_RIGHTY, 0);quit(); }
        });
    }
    Loop second;
    second.run("pointer-kept-in-the-next-loop", [](Loop &l) {
        if (l.stage == 0) rtTap();
        else if (l.stage == 4) { l.expect(l.delivered == std::vector<std::string>{"cast"}, "the pointer did not keep its place");quit(); }
    });
}

// N4: RT never clicks where no pointer is drawn; it shows the stick's pointer.
void rtWithoutPointer() {
    auto &pointer = SdlController::instance().pointer();
    pointer.stick = false;pointer.known = false;
    Loop loop;
    noMouseFocus = true; // The mouse is not over the window.
    loop.run("rt-shows-the-pointer-first", [&](Loop &l) {
        if (l.stage == 2) { pointer.stick = false;rtTap(); }
        else if (l.stage == 6) {
            l.expect(l.delivered.empty(), "RT clicked where no pointer is drawn");
            l.expect(pointer.stick && pointer.x == 160 && pointer.y == 100, "RT did not show the pointer at the frame center");
            rtTap();
        } else if (l.stage == 10) { l.expect(l.delivered == std::vector<std::string>{"space"}, "the next RT did not click at the pointer");quit(); }
    });
    noMouseFocus = false;
}

// N11: RT in a strict dialog acts on release, as a mouse click does.
void rtStrictDialog() {
    auto &pointer = SdlController::instance().pointer();
    pointer.stick = true;pointer.known = true;pointer.x = 15;pointer.y = 15;
    Loop loop;loop.screen = MainScreen::None;loop.queueable = false;
    DialogInput input;input.hits.push_back({10, 10, 30, 30, 'e'});input.keys = {'e', InputKey::Escape};
    loop.dialog = std::make_shared<const DialogInput>(std::move(input));
    loop.run("rt-in-a-strict-dialog", [](Loop &l) {
        if (l.stage == 0) rtTap();
        else if (l.stage == 4) { l.expect(l.delivered == std::vector<std::string>{"key:101"}, "RT in a strict dialog");quit(); }
    });
}

// N11: the cursor is drawn at the kept position while the stick owns it.
std::optional<std::pair<int, int>> cursorAt;
bool recordCursor = false;
void cursorAtKeptPosition() {
    const auto directory = std::filesystem::temp_directory_path() / "mmodern-m115-cursor";
    std::filesystem::create_directories(directory);
    const auto settings = directory / "window.ini";
    { std::ofstream out(settings); out << "aspect=square" << std::endl; }
    SdlWindow::useAspectSetting(settings); // Square: the cursor is drawn at frame scale 1.
    auto &pointer = SdlController::instance().pointer();
    pointer.stick = true;pointer.known = true;pointer.x = 100.7;pointer.y = 50.2;
    Loop loop;loop.cursor = true;
    cursorAt.reset();recordCursor = true;
    loop.run("cursor-at-the-kept-position", [&](Loop &l) {
        if (l.stage == 2) {
            l.expect(cursorAt == std::pair<int, int>{100, 50}, "the cursor is not drawn at the kept position");
            warp(30, 40, PictureAspect::Square);cursorAt.reset(); // The mouse moves.
        } else if (l.stage == 5) {
            l.expect(!pointer.stick && cursorAt == std::pair<int, int>{30, 40}, "the mouse did not take the cursor back");
            quit();
        }
    });
    recordCursor = false;
    SdlWindow::useAspectSetting(std::nullopt);
    std::error_code ignored;std::filesystem::remove_all(directory, ignored);
}

// S2: a controller character dropped by a fence leaves the field unknown: the
// next Up adds, and no Backspace deletes a character the player typed.
void droppedCharacter() {
    Loop loop;loop.screen = MainScreen::None;loop.queueable = false;loop.ready = false;
    loop.dialog = dialogWith({InputKey::Escape, InputKey::Enter, 8, InputKey::Left}, true);
    loop.run("dropped-controller-character", [](Loop &l) {
        if (l.stage == 0) tapButton(SDL_CONTROLLER_BUTTON_DPAD_UP); // The entry is not ready yet.
        else if (l.stage == 4) { l.expect(l.delivered.empty(), "a character reached a busy entry");l.ready = true; }
        else if (l.stage == 5) tapButton(SDL_CONTROLLER_BUTTON_DPAD_UP);
        else if (l.stage == 9) {
            l.expect(l.delivered == std::vector<std::string>{"text:A"}, "a Backspace followed a dropped character");
            quit();
        }
    });
}

int main(int argc, char **) {
    try {
        mappingTable();
        trackerUnits();
        textEntryUnits();
        if (argc > 1) return 0;
        optionOff();
        SdlWindow::useControllerSetting(true);
        heldDpadMatchesHeldKey();
        strafeChords();
        pressDuringPresentation();
        breakAfterContextChange();
        removalAndFocusLoss();
        noisySticks();
        deadZoneBursts();
        yesNoAndCombat();
        russianLoop();
        queueSurvivesLoopEnd();
        strafeReleasesCtrl();
        optionOffWhileHeld();
        nestedLoopRestoresSnapshot();
        virtualController();
        for (const bool russian : {false, true})
            for (const bool retained : {false, true}) textEntryLoop(russian, retained);
        textEntryLoop(false, false, true);
        numberAndPathEntries();
        pointerClicks(PictureAspect::FourThree);
        pointerClicks(PictureAspect::Square);
        pointerWaitCap();
        pointerReleaseOnLoss(false);
        pointerReleaseOnLoss(true);
        cursorTarget = &cursorAt;cursorRecording = &recordCursor;
        rtAcrossLoops();
        pointerAcrossLoops();
        rtWithoutPointer();
        rtStrictDialog();
        cursorAtKeptPosition();
        droppedCharacter();
        SdlWindow::useControllerSetting(false);
        optionOff();
        std::cout << "SDL controller tests passed\n";
        return 0;
    } catch (const std::exception &error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
