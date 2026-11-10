#ifndef MMODERN_PLATFORM_SDL_CONTROLLER_H
#define MMODERN_PLATFORM_SDL_CONTROLLER_H

#include "platform/sdl/XeenControllerInput.h"

#include <array>
#include <atomic>
#include <cstdint>
#include <map>
#include <mutex>
#include <optional>
#include <vector>

union SDL_Event;
struct _SDL_GameController;

namespace mmodern {

// One key make or break a controller sends.
struct ControllerKeyChange {
    ControllerKeyEvent key;
    bool down = false;
    bool operator==(const ControllerKeyChange &other) const { return key == other.key && down == other.down; }
};

// M115: turns controller payloads (instance id, button, axis value) into key
// makes and breaks. All state comes from the event payloads, never from a
// device. Pure: no SDL calls.
// - Sticks and triggers become buttons with travel hysteresis; the left stick
//   gives one direction at a time and keeps it until the other axis leads by
//   a margin.
// - The D-pad, the left stick and every controller are merged per control:
//   a control is made once and released once.
// - A make resolves against the published context; its break releases the
//   same key event, whatever the context is then. Two controls holding the
//   same physical key make and break it once.
class ControllerTracker {
public:
    // Half travel presses, about a third releases (SDL axes span +-32767).
    static constexpr int kPress = 16384, kRelease = 10923;
    // The other stick axis must lead by this much to change direction.
    static constexpr int kDirectionMargin = 6554;

    std::vector<ControllerKeyChange> button(std::int32_t instance, int sdlButton, bool down,
        const std::optional<ControllerContext> &context);
    std::vector<ControllerKeyChange> axis(std::int32_t instance, int sdlAxis, int value,
        const std::optional<ControllerContext> &context);
    // A removed controller releases whatever it held.
    std::vector<ControllerKeyChange> removed(std::int32_t instance);
    // Focus loss: every held key is released and every controller forgotten.
    // Breaking the last key that carried Ctrl also releases Left Ctrl.
    std::vector<ControllerKeyChange> releaseAll();
    bool idle() const;

private:
    // Per-controller sources: SDL's buttons, the left stick's direction and LT.
    struct Instance {
        std::array<bool, 32> buttons{};
        std::optional<ControllerControl> stick;
        int x = 0, y = 0;
        bool trigger = false;
    };
    void change(ControllerControl control, bool down, const std::optional<ControllerContext> &context,
        std::vector<ControllerKeyChange> &changes);
    void release(Instance &instance, std::vector<ControllerKeyChange> &changes);
    std::map<std::int32_t, Instance> instances_;
    std::array<unsigned, kControllerControls> holders_{};
    std::array<std::optional<ControllerKeyEvent>, kControllerControls> made_{};
    std::map<int, unsigned> heldScancodes_;
    unsigned heldCtrl_ = 0; // Made keys carrying Ctrl (English strafe).
};

// M115: the process-lifetime owner of SDL's game controller subsystem, held
// only while the option is on. It registers one SDL event watch, which turns
// controller events into ordinary SDL_KEYDOWN/SDL_KEYUP events pushed into the
// queue, so that SdlWindow's translation, queue, repeat model and fences apply
// unchanged. It never uses SDL_SetEventFilter, which flushes the queue.
class SdlController {
public:
    static SdlController &instance();
    // Called by each loop after SDL video and events are started. Starting
    // pushes SDL_CONTROLLERDEVICEADDED for attached controllers; stopping
    // closes them and quits the subsystem. Off and never started: no SDL call.
    void configure(bool enabled);
    bool enabled() const { return enabled_.load(); }
    // The loop's context snapshot; nullopt withdraws it. Nothing is translated
    // without one.
    void publish(std::optional<ControllerContext> context);
    std::optional<ControllerContext> published() const;
    // Loop thread: opens and closes controllers on SDL_CONTROLLERDEVICEADDED
    // and SDL_CONTROLLERDEVICEREMOVED (never inside an SDL callback).
    void deviceEvent(const SDL_Event &event);
    // Focus loss: pushes the break of every key the controllers hold.
    void releaseAll();

private:
    SdlController() = default;
    static int watch(void *self, SDL_Event *event);
    void push(const std::vector<ControllerKeyChange> &changes);
    std::atomic<bool> enabled_{false};
    bool startFailed_ = false;
    mutable std::mutex mutex_;
    ControllerTracker tracker_;
    std::optional<ControllerContext> context_;
    std::map<std::int32_t, _SDL_GameController *> open_; // Loop thread only.
};

} // namespace mmodern

#endif
