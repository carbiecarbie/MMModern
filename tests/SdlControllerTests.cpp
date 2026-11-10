// M115 Part A: game controller keys. Mapping and tracker units without SDL,
// then the production SDL loop driven by pushed SDL_CONTROLLER* events whose
// state comes entirely from the event payloads (no device is needed).
#include "core/PresentationClock.h"
#include "platform/sdl/SdlController.h"
#include "platform/sdl/SdlWindow.h"

#define SDL_MAIN_HANDLED
#include <SDL.h>

#include <functional>
#include <iostream>
#include <stdexcept>
#include <string>
#include <variant>
#include <vector>

using namespace mmodern;

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
    std::function<void(const PlayerAction &)> observe;
    std::uint64_t context = 1, epoch = 1, presented = 0, clock = 0;
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
            if (observe) observe(action);
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

int main(int argc, char **) {
    try {
        mappingTable();
        trackerUnits();
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
        SdlWindow::useControllerSetting(false);
        optionOff();
        std::cout << "SDL controller tests passed\n";
        return 0;
    } catch (const std::exception &error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
