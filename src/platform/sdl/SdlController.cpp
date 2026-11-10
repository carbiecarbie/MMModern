#include "platform/sdl/SdlController.h"

#define SDL_MAIN_HANDLED
#include <SDL.h>

#include <cstdlib>
#include <iostream>

namespace mmodern {
namespace {

std::optional<ControllerControl> buttonControl(int button) {
    using C = ControllerControl;
    switch (button) {
    case SDL_CONTROLLER_BUTTON_A: return C::A;
    case SDL_CONTROLLER_BUTTON_B: return C::B;
    case SDL_CONTROLLER_BUTTON_X: return C::X;
    case SDL_CONTROLLER_BUTTON_Y: return C::Y;
    case SDL_CONTROLLER_BUTTON_BACK: return C::View;
    case SDL_CONTROLLER_BUTTON_START: return C::Menu;
    case SDL_CONTROLLER_BUTTON_LEFTSTICK: return C::LeftStick;
    case SDL_CONTROLLER_BUTTON_RIGHTSTICK: return C::RightStick;
    case SDL_CONTROLLER_BUTTON_LEFTSHOULDER: return C::LeftShoulder;
    case SDL_CONTROLLER_BUTTON_RIGHTSHOULDER: return C::RightShoulder;
    case SDL_CONTROLLER_BUTTON_DPAD_UP: return C::Up;
    case SDL_CONTROLLER_BUTTON_DPAD_DOWN: return C::Down;
    case SDL_CONTROLLER_BUTTON_DPAD_LEFT: return C::Left;
    case SDL_CONTROLLER_BUTTON_DPAD_RIGHT: return C::Right;
    default: return std::nullopt; // Guide, paddles, touchpad, misc.
    }
}

// One direction at a time: keep the current one while it stays above the
// release travel and the other axis does not lead it by the margin.
std::optional<ControllerControl> stickDirection(int x, int y, std::optional<ControllerControl> current) {
    using C = ControllerControl;
    if (current) {
        const bool vertical = *current == C::Up || *current == C::Down;
        const int along = *current == C::Up ? -y : *current == C::Down ? y : *current == C::Left ? -x : x;
        const int across = std::abs(vertical ? x : y);
        if (along >= ControllerTracker::kRelease &&
            !(across >= ControllerTracker::kPress && across > along + ControllerTracker::kDirectionMargin))
            return current;
    }
    const int ax = std::abs(x), ay = std::abs(y);
    if (std::max(ax, ay) < ControllerTracker::kPress) return std::nullopt;
    if (ay >= ax) return y < 0 ? C::Up : C::Down;
    return x < 0 ? C::Left : C::Right;
}

} // namespace

void ControllerTracker::change(ControllerControl control, bool down, const std::optional<ControllerContext> &context,
        std::vector<ControllerKeyChange> &changes) {
    const auto index = unsigned(control);
    if (down) {
        if (holders_[index]++) return;
        // Resolved against the frame on screen when pressed; nothing without a snapshot.
        made_[index] = context ? xeenControllerKeyEvent(control, *context) : std::nullopt;
        if (made_[index] && !heldScancodes_[made_[index]->scancode]++) {
            changes.push_back({*made_[index], true});
            if (made_[index]->mod & KMOD_CTRL) ++heldCtrl_;
        }
        return;
    }
    if (!holders_[index] || --holders_[index]) return;
    if (const auto made = made_[index]) {
        made_[index].reset();
        auto held = heldScancodes_.find(made->scancode);
        if (held != heldScancodes_.end() && !--held->second) {
            heldScancodes_.erase(held);
            changes.push_back({*made, false});
            // The strafe carries Ctrl on its own events and sends no Ctrl make.
            // After the last one, release Ctrl as a keyboard would, so that the
            // loop's held modifiers do not keep it (never while the other
            // strafe is held: its repeat would become a turn).
            if ((made->mod & KMOD_CTRL) && heldCtrl_ && !--heldCtrl_)
                changes.push_back({ControllerKeyEvent{SDLK_LCTRL, SDL_SCANCODE_LCTRL, KMOD_NONE}, false});
        }
    }
}

std::vector<ControllerKeyChange> ControllerTracker::button(std::int32_t instance, int sdlButton, bool down,
        const std::optional<ControllerContext> &context) {
    std::vector<ControllerKeyChange> changes;
    const auto control = buttonControl(sdlButton);
    if (!control || sdlButton < 0 || sdlButton >= int(Instance{}.buttons.size())) return changes;
    auto &state = instances_[instance];
    if (state.buttons[sdlButton] == down) return changes;
    state.buttons[sdlButton] = down;
    change(*control, down, context, changes);
    return changes;
}

std::vector<ControllerKeyChange> ControllerTracker::axis(std::int32_t instance, int sdlAxis, int value,
        const std::optional<ControllerContext> &context) {
    std::vector<ControllerKeyChange> changes;
    if (sdlAxis != SDL_CONTROLLER_AXIS_LEFTX && sdlAxis != SDL_CONTROLLER_AXIS_LEFTY &&
        sdlAxis != SDL_CONTROLLER_AXIS_TRIGGERLEFT) return changes; // The right stick and RT belong to the pointer.
    auto &state = instances_[instance];
    if (sdlAxis == SDL_CONTROLLER_AXIS_TRIGGERLEFT) {
        const bool pressed = state.trigger ? value >= kRelease : value >= kPress;
        if (pressed != state.trigger) { state.trigger = pressed; change(ControllerControl::LeftTrigger, pressed, context, changes); }
        return changes;
    }
    (sdlAxis == SDL_CONTROLLER_AXIS_LEFTX ? state.x : state.y) = value;
    const auto direction = stickDirection(state.x, state.y, state.stick);
    if (direction == state.stick) return changes;
    const auto previous = state.stick;
    state.stick = direction;
    if (previous) change(*previous, false, context, changes); // Breaks before makes.
    if (direction) change(*direction, true, context, changes);
    return changes;
}

void ControllerTracker::release(Instance &state, std::vector<ControllerKeyChange> &changes) {
    for (int button = 0; button < int(state.buttons.size()); ++button)
        if (state.buttons[button]) {
            state.buttons[button] = false;
            if (const auto control = buttonControl(button)) change(*control, false, std::nullopt, changes);
        }
    if (state.stick) change(*state.stick, false, std::nullopt, changes);
    if (state.trigger) change(ControllerControl::LeftTrigger, false, std::nullopt, changes);
    state = {};
}

std::vector<ControllerKeyChange> ControllerTracker::removed(std::int32_t instance) {
    std::vector<ControllerKeyChange> changes;
    if (const auto found = instances_.find(instance); found != instances_.end()) {
        release(found->second, changes);
        instances_.erase(found);
    }
    return changes;
}

std::vector<ControllerKeyChange> ControllerTracker::releaseAll() {
    std::vector<ControllerKeyChange> changes;
    for (auto &[id, state] : instances_) release(state, changes);
    instances_.clear();
    return changes;
}

bool ControllerTracker::idle() const {
    for (const auto holders : holders_) if (holders) return false;
    return heldScancodes_.empty();
}

SdlController &SdlController::instance() {
    static SdlController controller;
    return controller;
}

void SdlController::configure(bool enabled) {
    if (enabled == enabled_.load() && (!enabled || SDL_WasInit(SDL_INIT_GAMECONTROLLER))) return;
    if (enabled) {
        if (startFailed_) return; // Reported once; not retried in this process.
        if (enabled_.load()) {
            // SDL_Quit elsewhere ended the subsystem, the watch and every
            // controller: forget them without closing.
            SDL_DelEventWatch(&SdlController::watch, this);
            open_.clear();
            std::lock_guard lock(mutex_);
            tracker_ = {};
            context_.reset();
        }
        if (SDL_InitSubSystem(SDL_INIT_GAMECONTROLLER) != 0) {
            std::cerr << "Cannot start game controller support: " << SDL_GetError() << '\n';
            startFailed_ = true;
            enabled_ = false;
            return;
        }
        SDL_AddEventWatch(&SdlController::watch, this);
        enabled_ = true;
        return;
    }
    SDL_DelEventWatch(&SdlController::watch, this);
    enabled_ = false;
    for (const auto &[id, controller] : open_) SDL_GameControllerClose(controller);
    open_.clear();
    std::vector<ControllerKeyChange> changes;
    {
        std::lock_guard lock(mutex_);
        changes = tracker_.releaseAll();
        context_.reset();
    }
    push(changes);
    SDL_QuitSubSystem(SDL_INIT_GAMECONTROLLER);
}

void SdlController::publish(std::optional<ControllerContext> context) {
    if (!enabled_.load()) return;
    std::lock_guard lock(mutex_);
    context_ = std::move(context);
}

std::optional<ControllerContext> SdlController::published() const {
    std::lock_guard lock(mutex_);
    return context_;
}

void SdlController::deviceEvent(const SDL_Event &event) {
    if (!enabled_.load()) return;
    if (event.type == SDL_CONTROLLERDEVICEADDED) {
        const int index = event.cdevice.which;
        const auto id = SDL_JoystickGetDeviceInstanceID(index);
        if (id < 0 || open_.count(id) || !SDL_IsGameController(index)) return;
        if (auto *controller = SDL_GameControllerOpen(index)) open_[SDL_JoystickInstanceID(SDL_GameControllerGetJoystick(controller))] = controller;
        else std::cerr << "Cannot open game controller: " << SDL_GetError() << '\n';
    } else if (event.type == SDL_CONTROLLERDEVICEREMOVED) {
        // The watch has already released what this controller held.
        if (const auto found = open_.find(event.cdevice.which); found != open_.end()) {
            SDL_GameControllerClose(found->second);
            open_.erase(found);
        }
    }
}

void SdlController::releaseAll() {
    if (!enabled_.load()) return;
    std::vector<ControllerKeyChange> changes;
    {
        std::lock_guard lock(mutex_);
        changes = tracker_.releaseAll();
    }
    // Pushed outside the lock: a watch running on another thread holds SDL's
    // watcher lock while it waits for this one.
    push(changes);
}

void SdlController::push(const std::vector<ControllerKeyChange> &changes) {
    for (const auto &change : changes) {
        SDL_Event key{};
        key.type = change.down ? SDL_KEYDOWN : SDL_KEYUP;
        key.key.state = change.down ? SDL_PRESSED : SDL_RELEASED;
        key.key.keysym.sym = change.key.sym;
        key.key.keysym.scancode = change.key.scancode;
        key.key.keysym.mod = change.key.mod;
        // SDL stamps it with the current tick, as it does the controller event.
        if (SDL_PushEvent(&key) != 1) std::cerr << "Cannot queue controller key: " << SDL_GetError() << '\n';
    }
}

// Runs inside SDL_PushEvent, under SDL's recursive watcher lock, on whatever
// thread pushed the event. Any other event type returns at once without
// touching state. The pushed keys enter the queue before the controller event
// itself, which the loop ignores like mouse motion.
int SdlController::watch(void *self, SDL_Event *event) {
    switch (event->type) {
    case SDL_CONTROLLERBUTTONDOWN: case SDL_CONTROLLERBUTTONUP:
    case SDL_CONTROLLERAXISMOTION: case SDL_CONTROLLERDEVICEREMOVED: break;
    default: return 1;
    }
    auto &controller = *static_cast<SdlController *>(self);
    std::lock_guard lock(controller.mutex_);
    std::vector<ControllerKeyChange> changes;
    if (event->type == SDL_CONTROLLERAXISMOTION)
        changes = controller.tracker_.axis(event->caxis.which, event->caxis.axis, event->caxis.value, controller.context_);
    else if (event->type == SDL_CONTROLLERDEVICEREMOVED)
        changes = controller.tracker_.removed(event->cdevice.which);
    else
        changes = controller.tracker_.button(event->cbutton.which, event->cbutton.button,
            event->type == SDL_CONTROLLERBUTTONDOWN, controller.context_);
    // Under the lock, so that makes and breaks keep their order; the loop
    // thread never holds this lock while it calls into SDL.
    controller.push(changes);
    return 1;
}

} // namespace mmodern
