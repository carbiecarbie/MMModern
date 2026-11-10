# Milestone 115 plan - Game controller support

**Tier A. Status: completed and accepted by the maintainer (2026-11-10)**,
after a play-test on a Steam Deck and on Windows with a physical controller.
Plan `36ab807`; Part A `f866278`, Part B `a7c9844`. First of the opt-in
extras of [later goal 6](roadmap.md#later-goals). Save v26, Journey schema
and content unchanged: nothing here is game state. With the option off, the
default, nothing changes.

## Scope

With `controller=on` in `window.ini` (the Windows launcher's box or the
Linux setup screen's row), a game controller plays the whole game: walking,
fighting, the original buttons, portraits, dialogs, services, text entry,
the title and the setup screen. Every press becomes a key or a click the
original already accepts. Out of scope: remapping inside MMModern, rumble,
gyro, controller pictures on the original screens, a focus highlight, more
than one player and Android itself.

## Evidence

The original has no joystick or gamepad support (DOS static, high; offsets
in [original quirks](original-quirks.md#dev-controller)), so controller
support is an approved deviation by nature. The pinned ScummVM plays Xeen
with a controller as a pointer with two click buttons.

## Maintainer decisions (2026-11-09)

1. **Navigation model: keys and a pointer, no focus highlight.** Buttons
   send the original keys by screen; the right stick moves the original
   pointer and RT clicks, for everything without a button. The original has
   no focus, and the arrow keys already mean something in several dialogs.
2. **Default mapping.** The table is in
   [controls](controls.md#game-controller). A answers Yes in Yes/No prompts
   and B is Escape or No; the letter buttons send nothing in dialogs. Text
   entry needs no new screen: Up and Down change the last character, Right
   starts a new one, Left deletes; the platform keyboards keep working
   beside it.
3. **Hot-plug, one player, no remapping.** Controllers can be connected and
   removed while playing; every connected one drives the same party. SDL's
   database and Steam Input adapt devices and let players rebind.
4. **The switch.** `controller=on` in `window.ini`, off by default; never on
   by itself when a controller is connected.

## Design decisions

1. **Owner.** `SdlController` (`src/platform/sdl`, process lifetime) holds
   SDL's controller subsystem only while the option is on.
2. **One entry point.** One event watch turns controller payloads into
   ordinary key and mouse events, so the loop's translation, queue,
   held-key repeat and stale-input fences apply unchanged. The loop
   publishes the input context it already samples; the watch never calls
   the handler. `SDL_SetEventFilter` was rejected: it discards the queue.
3. **Keys, not actions.** A button resolves to an internal key and then to
   the key of the selected language's program, through the same tables the
   keyboard uses.
4. **Held buttons repeat as held keys.** A break releases the key its make
   sent; D-pad, stick and controllers are merged; sticks and triggers have
   travel hysteresis and the left stick direction hysteresis; removal and
   focus loss release keys and the click; the last strafe release also
   releases Ctrl.
5. **The pointer.** Its position is kept for the process and moves on the
   presentation clock; the loop wakes faster only while the stick is
   outside its dead zone. Moving the mouse takes the pointer back, and RT
   never clicks where no pointer is drawn.
6. **Text entry.** Controller text inputs are sent one at a time, each right
   after the loop acquires a frame: the one place where the loop gained
   controller-specific sequencing. An entry ends on any other context or a
   delivered Enter or Escape.
7. **The switch.** `controller` is a field of `WindowPlacement`; the setup
   screen and the launcher change that one key and keep the others.
8. **Steam Input.** MMModern never calls Steam. A connected controller that
   is not pressed makes no key, no click and no pass. Binding one physical
   button to both a key and a controller button would act twice; the README
   says so.
9. **Android.** Everything is SDL2's game controller API and SDL events, so
   it carries over.
10. **Input after a refusal.** A controller press is a key or a click before
    any fence or handler sees it, so a refusal behaves the same from keys
    and from a controller; the open audit seam need not treat controllers
    separately.

## Results

- Independent reviews: the plan (two blocking design errors corrected
  before any code), Part A and Part B (approve after fixes, all applied:
  among them a loop freeze from an RT press at a loop's end, a dropped
  character deleting one the player typed, and presses leaking into the
  next entry).
- The complete suite passes on Windows and WSL Linux, 422 tests each, and
  GitHub Actions is green on both systems. Loop tests push controller
  events, and one runs on SDL's virtual joystick; an original-data test
  shows a refusal behaves the same from keys and from a controller.
- The AppImage's SDL2 reads controllers through its evdev driver, without
  udev; its build script asserts that driver.
- Play-test: every item passed and needed no fix. On the Steam Deck: Game
  Mode with a gamepad layout (walking, strafing, the pointer and RT, text
  entry, the Tavern, a sheet, the control panel), Game Mode with the M112
  keyboard layout with the option on and off, without double input, and
  Desktop Mode. On Windows: a physical controller with the launcher's box,
  unplugged while held and plugged in again, and the pointer passing
  between the mouse and the stick.
