#ifndef MMODERN_PLATFORM_SDL_CONTROLLER_H
#define MMODERN_PLATFORM_SDL_CONTROLLER_H

#include "core/PlayerAction.h"
#include "platform/sdl/XeenControllerInput.h"

#include <array>
#include <atomic>
#include <cstdint>
#include <deque>
#include <map>
#include <mutex>
#include <optional>
#include <utility>
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

// M115 Part B: a D-pad/left-stick direction, A or B pressed in a text entry,
// with the context it was pressed in; SdlController types it.
struct ControllerTextPress {
    ControllerControl control;
    ControllerContext context;
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
    // Part B. Text-entry presses since the last call, in order.
    std::vector<ControllerTextPress> takeTextPresses();
    // RT, merged over controllers like a control: true for its press, false
    // for its release (also on removal and focus loss), in order.
    std::vector<bool> takeClicks();
    // The right stick of the controller deflected most (raw SDL values).
    std::pair<int, int> pointerAxes() const;

private:
    // Per-controller sources: SDL's buttons, the left stick's direction, LT,
    // and (Part B) RT and the right stick.
    struct Instance {
        std::array<bool, 32> buttons{};
        std::optional<ControllerControl> stick;
        int x = 0, y = 0;
        bool trigger = false;
        bool click = false;
        int pointerX = 0, pointerY = 0;
    };
    void click(bool down);
    void change(ControllerControl control, bool down, const std::optional<ControllerContext> &context,
        std::vector<ControllerKeyChange> &changes);
    void release(Instance &instance, std::vector<ControllerKeyChange> &changes);
    std::map<std::int32_t, Instance> instances_;
    std::array<unsigned, kControllerControls> holders_{};
    std::array<std::optional<ControllerKeyEvent>, kControllerControls> made_{};
    std::map<int, unsigned> heldScancodes_;
    unsigned heldCtrl_ = 0; // Made keys carrying Ctrl (English strafe).
    unsigned clickHolders_ = 0;
    std::vector<ControllerTextPress> textPresses_;
    std::vector<bool> clicks_;
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
    // Focus loss: pushes the break of every key the controllers hold, releases
    // RT and drops the text entry's pending input.
    void releaseAll();

    // Part B, the pointer: RT presses and releases for the loop to push as
    // mouse button events at its pointer, and the right stick's deflection.
    std::vector<bool> takeClicks();
    std::pair<int, int> pointerAxes() const;
    // The pointer the right stick moves, kept for the whole process so that it
    // keeps its place, its owner and a pushed RT press across windows (loop
    // thread only).
    struct Pointer {
        bool stick = false;      // The stick owns it: drawn and clicked at x, y.
        bool known = false;      // x, y were set; otherwise the frame center.
        double x = 0, y = 0;     // Frame coordinates.
        bool down = false;       // An RT press was pushed and not yet released.
        std::pair<int, int> press{0, 0};
    };
    Pointer &pointer() { return pointer_; }
    // Part B, text entry (decision 6). The loop reports each delivered action
    // and each acquired frame (after publishing its context). Every controller
    // text input (character, Backspace, Enter, Escape) is held here and sent
    // one at a time, each right after an acquisition: one input per acquired
    // frame. The strict entries answer an input synchronously, so the frame
    // acquired next is the one answering it; an input not delivered by then
    // was dropped by a fence, and the controller forgets the field. The entry
    // lasts while every acquired context is a text entry of the same kind
    // (the title's name takes a new context id at every key); any other
    // context, a delivered Enter or Escape, focus loss or turning the option
    // off ends it and drops what it still had to type.
    void delivered(const PlayerAction &action);
    void acquired();

private:
    SdlController() = default;
    static int watch(void *self, SDL_Event *event);
    void push(const std::vector<ControllerKeyChange> &changes);
    static void pushEvents(const std::vector<SDL_Event> &events);
    // Under the mutex: one text-entry press.
    void type(const ControllerTextPress &press);
    struct TextInput {
        enum class Kind { Backspace, Character, Key } kind;
        unsigned char character = 0;
        ControllerKeyEvent key;
    };
    struct TextEntry {
        ControllerContext context;
        // The last character the controller added in this entry, while Up and
        // Down still change it.
        std::optional<unsigned char> added;
        std::deque<TextInput> pending;
        enum class Flight { None, Sent, Delivered } flight = Flight::None;
        std::uint64_t sentAt = 0;
    };
    std::optional<TextEntry> text_;
    Pointer pointer_;
    std::uint64_t acquisitions_ = 0;
    std::vector<bool> clicks_;
    std::atomic<bool> enabled_{false};
    bool startFailed_ = false;
    mutable std::mutex mutex_;
    ControllerTracker tracker_;
    std::optional<ControllerContext> context_;
    std::map<std::int32_t, _SDL_GameController *> open_; // Loop thread only.
};

} // namespace mmodern

#endif
