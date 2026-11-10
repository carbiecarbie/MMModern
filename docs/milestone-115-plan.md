# Milestone 115 plan - Game controller support

**Tier A. Status: planned (not started).** The maintainer decided the four
[open questions](#maintainer-decisions-2026-11-09) on 2026-11-09.
First of the opt-in extras of [later goal 6](roadmap.md#later-goals).
Baseline: `547aa51`, equal to `origin/main`. Save v26, Journey schema and
content unchanged: nothing here is game state. With the option off, which
is the default, nothing changes.

## Goal and scope

A player who turns the option on plays the whole game with a game
controller: walking, fighting, the original buttons, portraits, dialogs,
services, text entry, the title and the Linux setup screen. Every controller
press becomes a key or a click the original game already accepts, so the
rules, timing and screens stay those of the original.

Out of scope: remapping inside MMModern, rumble, gyro, controller button
pictures on the original screens, a focus highlight, more than one player,
Android itself, and any change to keyboard or mouse behavior.

## Evidence (checked 2026-11-09)

- **The original has no joystick or gamepad support** (DOS static, high).
  `WORLD/XEEN.DAT` never loads the game port address `0x201` into a
  register and has no joystick text; its only BIOS INT 15h calls are the
  extended-memory moves at file offsets `0x402af` and `0x40300` (AH `87h`),
  not the joystick service (AH `84h`). The scan is calibrated on the input
  code it does find: the mouse driver calls (INT 33h at `0x7ca5` and
  `0x4373d..0x43985`) and the keyboard port read at `0x455c2`.
  `WORLD/XEEN.EXE` is the same (INT 15h AH `87h` at `0x14f33`, `0x14f84`;
  INT 33h at `0x4ccd`, `0x183c3..0x1860b`), `WORLD/INSTALL.EXE` asks for
  sound only, and the 29 bytes of `XEEN.CFG` are all sound fields
  ([audio rules](rules/audio.md)). There is no DOS baseline to reproduce;
  controller support is a deviation by nature. Scan and output:
  `D:\Projetos\MModern\MModern-re\m115-joystick\`.
- **Pinned ScummVM reference.** The Xeen engine reads keys and the mouse
  only (`engines/mm/xeen/events.cpp`). `engines/mm/metaengine.cpp:178..198`
  maps `JOY_A` to a left click and `JOY_B` to a right click, and ScummVM's
  own backend moves the pointer with a stick. So the reference plays Xeen
  with a controller as a pointer with two click buttons.
- **MMModern today.** All input enters through `SdlWindow::showLoop`
  (`src/platform/sdl/SdlWindow.cpp`), which starts SDL with video and
  events only. Keys go through the DOS translation (`dosCode`, `inputKey`,
  the Russian keyboard) to a `PlayerAction`; clicks go through
  `DialogInput::click` or `xeenMainScreenClick`. The same loop owns the
  pending queue (5 or 10 slots), the held-key repeat model of M63, the
  keyboard and mouse clears, the context and frame fences, and the
  timestamp fence that retires presses made before a new frame is acquired.
  Five `SDL_FilterEvents` sites apply those fences to queued key, text and
  mouse events by event type.

## Why Tier A

The milestone adds an input source to the loop that owns input timing and
the stale-input fences, and changes when that loop wakes while a stick
moves the pointer. The design below is built so that none of that timing
changes, with one stated exception (the text-entry follow-up of design
decision 6), and that claim is exactly what an independent review has to
check. It adds no game owner and no rule, and touches no save, RNG or game
clock.

## Maintainer decisions (2026-11-09)

The maintainer accepted the four recommendations below as written.

**1. Navigation model.** The options were (a) a pointer moved by a stick,
clicking the original buttons, as the reference does; (b) focus navigation,
a highlight that the D-pad moves between buttons, portraits and choices;
(c) a mix.
*Decided: a mix without focus navigation.* On every screen the
controller buttons send the original keys (table below) and the right stick
moves the original mouse pointer, with a click button for everything that
has no key on the controller: portraits, item rows, combat targets, service
letters, the title menu. Reasons: the original has no focus, so a highlight
would be a new picture on original screens; the arrow keys already mean
something in several dialogs (the sheet, lists, Create, the Inn), so a
D-pad focus would take them away; the pointer is one mechanism for every
screen instead of one per dialog; it is what the M112 layout already
play-tested through Steam Input; and on Android a touch is the same click.
Focus navigation can be a later opt-in if play shows the pointer is too
slow somewhere.

**2. Default mapping.** *Decided* (names as on an Xbox controller; SDL maps
other controllers by position):

| Control | Exploration | Combat | Dialogs, services, title, setup screen |
| --- | --- | --- | --- |
| D-pad, left stick | Up/Down: forward, back; Left/Right: turn | Left/Right: turn; Up/Down are refused as on the keyboard | The dialog's arrow keys |
| LB / RB | Strafe left / right | - | Page Up / Page Down |
| A | Space: interact | A: attack | Enter; Y in a Yes/No prompt without Enter |
| B | Escape | Escape | Escape; N in a prompt without Escape |
| X | C: cast | C: cast | - |
| Y | S: shoot | F: Quick Fight | - |
| LT | B: bash | B: block | - |
| View (Back) | R: rest | R: run | - |
| Left stick click | W: wait | W | - |
| Right stick click | M: map | M | - |
| Menu (Start) | Tab: control panel | Tab | Tab |
| Right stick | Move the pointer | Move the pointer | Move the pointer |
| RT | Click at the pointer | Click | Click |

A Yes/No prompt is a dialog that accepts both Y and N; the original ones
accept Escape and not Enter, so A is Y and B is Escape there. N alone is
another command in Cast (New) and Quick Fight Options (Next) and is never
sent. As on the keyboard, B in exploration completes a pass, and Tab or any
key the control panel does not use closes it.
The letter buttons send nothing in dialogs, where the same letters are
other commands (C creates a character in the Inn). Everything else (Info,
Quick Reference, Quests, Dismiss, Use, Quick Fight Options, F1-F6, digits,
service letters) is a click on its original button.
*Text entry* (names, save names, Event words, Bank and spell numbers): Up
and Down change the last character through A-Z, space and 0-9 (digits only
in number entry), Right adds a character, Left deletes, A confirms and B
cancels. It sends the same typed characters and Backspace as a keyboard, so
no new screen is drawn ([design decision 6](#design-decisions) gives the
exact rules). Steam's keyboard (it opens by itself in Game Mode,
as M114 found) and later Android's keyboard keep working beside it. With
Russian selected the same scheme runs over the Russian program's letters.
A answers Yes in Yes/No prompts, with B as Escape or No, as the table says.

**3. Hot-plug, several controllers, remapping.** *Decided:* controllers
can be connected and removed while playing; removing one releases whatever
it held, and without any controller the game is exactly as with the option
off. Every connected controller drives the same single
player, with no selection screen. Remapping is not in this milestone: SDL's
controller database and Steam Input already adapt devices and let players
rebind, and a remapping screen would be a new project menu. A later
milestone can add it if players ask.

**4. The switch.** The README promises that extras are off by default.
*Decided:* `controller=on` in `window.ini` (host settings, never
game state, like `aspect=square`), default off, with a row on the Linux
setup screen and a checkbox in the Windows launcher. Under Proton the file
is edited by hand, as for `aspect`. The alternative, turning on by itself
when a controller is connected, was rejected: on a Deck Steam is
expected to show the game a virtual controller even with the keyboard
layout of M112 (to verify on the device).

## Design decisions

1. **Owner.** A new `SdlController` in `src/platform/sdl`, alive for the
   whole process like the window placement memory, holds SDL's controller
   subsystem, opens and closes game controllers and turns their events into
   key and mouse events. Reason for a new owner: device lifetime and
   hot-plug belong to no existing one, and `SdlWindow` is already large.
   Reason for the process lifetime: starting the subsystem with each loop,
   as video is, would reopen the devices and report held buttons again in
   the next loop; holding it also keeps the event queue alive between
   loops, as the audio device does today. Devices are opened and closed
   from the loop, never inside an SDL callback. The mapping is a pure
   header beside `XeenMainScreenInput.h` (controller button and input
   context to an internal key), unit-tested without SDL devices. No linked
   ScummVM portion is used: its keymapper needs ScummVM's backend, event
   manager, configuration and GUI, which MMModern does not run. No
   `PlayerAction`, flow or game owner changes.
2. **One entry point: an event watch that adds keys and clicks.** With the
   option on, `SdlController` registers one SDL event watch, once. It does
   not use `SDL_SetEventFilter`, which discards the whole queue every time
   it is set (SDL 2.32.10 `SDL_events.c:1212`) and would lose type-ahead,
   key releases and close requests at each loop boundary. When a controller
   event passes, the watch pushes the equivalent `SDL_KEYDOWN`/`SDL_KEYUP`,
   `SDL_TEXTINPUT` or `SDL_MOUSEBUTTONDOWN`/`UP`. From there the press is
   an ordinary key or click: same translation, same queue slots, same
   clears, same context, frame and timestamp fences, same five
   `SDL_FilterEvents` sites, unchanged. Checked in SDL 2.32.10:
   `SDL_PushEvent` runs watchers under a recursive lock, stamps the pushed
   event with the current tick, and SDL itself pushes from inside a
   watcher. The controller's own events stay in the queue: the raw
   `SDL_JOY*` events, which SDL needs in order to build controller events
   and which cannot be disabled or dropped, and the `SDL_CONTROLLER*`
   events. The loop has no case for them, exactly as for mouse motion
   today: they wake it and make no pass. The watch acts only on controller
   event types; for any other type, on any thread, it returns at once.
   - *Context.* The watch never calls the handler's `inputContext`, which
     has side effects and can throw. Each loop publishes a snapshot of the
     input context to `SdlController` where it already samples it (at
     start, per batch, after a delivery, at acquisition) and withdraws it
     when it ends; nested loops restore their caller's, like the Russian
     keyboard. Without a snapshot nothing is translated.
   - *Resolution.* A button resolves against the snapshot of the frame on
     screen when it is pressed. Where the loop admits type-ahead across a
     boundary (a dialog closing into exploration), the resolved key acts as
     that key typed on a keyboard at the same moment: Enter is an unmatched
     key with a pass, Space interacts and waits.
3. **Keys, not actions.** A button resolves to an internal key, then to the
   physical key that produces it: the CD key in English, the Russian
   program's key for the same button through `XeenKeyboard` in Russian. A
   button whose key cannot be typed in Russian sends nothing, as on the
   Russian keyboard.
   - Each make remembers the key event it sent, and its break releases
     that same key even if the context changed in between; otherwise the
     loop would keep the key as held and ignore it from then on.
   - D-pad, stick and every connected controller are merged first: a key
     is made once and released once.
   - English strafing is Ctrl+Left/Right. The pushed event carries the
     Ctrl modifier itself, with no Ctrl key event (a Ctrl make re-resolves
     every held key, turning a held forward into an unmatched key), and
     uses a scancode of its own so that it cannot collide with a held
     D-pad Left or Right.
4. **Held buttons repeat as held keys.** The controller sends one make and
   one break per press and never repeats by itself; `SdlWindow`'s existing
   model repeats where it does for keys (queued contexts: 500 ms, then the
   nominal 10.9 Hz, movement as M63; dialogs take fresh presses only).
   Sticks and triggers become buttons with hysteresis on travel (press at
   half travel, release at about a third). The left stick gives one
   direction at a time and keeps it until the other axis leads by a margin,
   so neither a stick resting near a threshold nor one held near a diagonal
   can make phantom steps. Losing focus or a controller releases every key
   and click it holds. SDL reports a button that was pressed while the
   window was unfocused when focus returns; it then counts as a press.
5. **The pointer.** `SdlWindow` keeps one pointer position in frame
   coordinates, set by whichever device moved it last. This is new
   handling on both sides: today the loop ignores `SDL_MOUSEMOTION` and
   reads the OS mouse position and mouse focus when it draws. Both drawing
   paths (4:3 and square) draw the kept position, and the mouse-focus test
   no longer hides a pointer the stick owns; `hideCursor` still hides it.
   The stick moves the pointer by its deflection on the loop's presentation
   clock (`presentationClock`, which tests replace), and only while the
   stick is outside its dead zone does the loop cap its wait to a few
   milliseconds. The click is a mouse press and release pushed at the kept
   position, converted with `frameToWindow` and carrying the window's id
   (square mode rescales mouse events only for its own window), so
   `ignoreMouse`, button feedback until release and the hit tables apply
   unchanged. While a mouse button is down the loop neither drains input
   nor runs idle work, so a removed controller or lost focus must push the
   release.
6. **Text entry.**
   - A typed character is sent as a keyboard sends it: key down, text
     input, key up. Event word entries take their text from key downs, the
     other English entries from text input, Russian from key downs.
   - Up and Down act on the last character the controller added in this
     entry; otherwise they add one. Right, Left, keyboard typing or a new
     entry end that state, since the controller cannot read the field.
   - Changing a character is two inputs, Backspace and the new character.
     Name, save-name and number entries are strict: the loop rightly drops
     a second input that arrives before the frame answering the first is
     acquired. `SdlController` therefore holds the new character as a
     one-slot follow-up, which the loop releases right after it acquires
     that frame, and drops if the context changed. This is the one place
     where the loop gains controller-specific sequencing.
   - String entries cycle A-Z, space and 0-9 (the Russian program's
     letters in Russian); number entries cycle digits. The setup screen's
     path line has no cycle: its arrows move the focus, and the folder is
     chosen from the browser's rows. Part B adds that distinction to
     `DialogInput` if nothing existing gives it. Raw arrows are never sent
     into a Russian string entry, where they type characters.
   - Left deletes one character per press; a controller has no key repeat.
   - If play shows the follow-up is not good enough, the alternative (Up
     and Down choose the character that Right adds) needs a drawn candidate
     and therefore a maintainer decision.
7. **The switch.** `controller` becomes a field of `WindowPlacement`, so
   that saving the window size does not erase a hand-edited line. The Linux
   setup screen and the Windows launcher, neither of which writes
   `window.ini` today, change that one key and keep the others. Tests turn
   the option on without a file, as `useAspectSetting` does for the shape.
8. **Steam Input.** MMModern never calls Steam and sets no Steam hint. A
   controller that is merely connected makes no key, no click and no pass;
   only presses act. Option off: the M112 layout works as today. Option on:
   the player uses Steam's Gamepad template, and may still bind the Deck's
   back buttons and trackpads to keys and the mouse, since that path is
   unchanged. Binding one physical button to both a key and a controller
   button would act twice; the README says so.
9. **Android.** Everything is SDL2's game controller API and SDL events,
   with no Windows, Linux or Steam code, so it carries over. Touch stays
   SDL's mouse click; the pointer is simply not needed there.
10. **Input after a refusal** (open seam of the 2026-11-07 audit). No
    interaction: a controller press is a key or a click before any fence or
    handler sees it, so whatever the next input does after a refusal, it
    does the same from a controller. M115 neither settles nor blocks that
    audit, and the audit need not treat controllers separately. One test
    pins this: a refusal scenario driven by controller and by keys gives
    the same actions and final state.

## Parts

- **Part A - keys.** The `controller` field in `window.ini`;
  `SdlController` with hot-plug, hysteresis, the event watch and the
  context snapshot; the mapping header with English and Russian
  resolution; exploration, combat and dialog keys. Check that the
  AppImage's SDL2 build has controller support (its package list has
  `libudev1` but may lack the development package), and amend the pinned
  list if not. In the same change: `original-quirks.md` gets the approved
  deviation `dev-controller` with the DOS finding above,
  `scummvm-differences.md` its item, and the root `AGENTS.md`, which still
  says quality-of-life options "are not part of current work", is amended
  with wording the maintainer approves.
- **Part B - pointer, text entry, switch UI.** The stick pointer and click;
  text entry with its follow-up; the setup screen row and the launcher
  checkbox; `controls.md` (including that B in exploration completes a
  pass) and the README (Steam Deck sections, Planned extras).
- Independent implementation review, complete suite on Windows and WSL
  Linux, then the play-test.

## Risks

- **A fence missed.** Any path where a controller press skips a fence
  would admit stale input. Mitigation: decision 2 leaves no second path
  except the stated follow-up, and the equivalence tests compare
  controller and key runs action by action.
- **Phantom steps** from stick noise, a diagonal, or a D-pad and stick
  held together: decisions 3 and 4, tested with noisy axis sequences.
- **A stuck key or a frozen loop** from a lost break or a lost click
  release: decisions 3 and 5, tested on removal, focus loss and a context
  change while held.
- **The text-entry follow-up** is new sequencing in the loop. It is one
  slot, released at one existing point, and has its own tests; decision 6
  names the fallback.
- **Double input on a Deck** (hypotheses to verify on the device): with the
  M112 layout Steam's virtual controller is connected but silent; in
  Desktop Mode SDL's own Steam Deck driver may open the built-in controls
  while Steam's desktop layout sends keys and mouse. If the second holds,
  the option's text says to use it in Game Mode with the Gamepad template.
- **Processor use.** With a joystick present SDL's wait polls every
  millisecond instead of sleeping in the system. Loop timing is unchanged;
  the play-test looks at the Deck's load once.
- **Keyboard and controller holding the same key**: releasing one releases
  the key. Accepted; the two devices are not meant to be mixed on one key.
- **Scope**: if Russian text entry or the launcher checkbox proves larger
  than expected, the plan is amended instead of silently narrowed.

## Acceptance

- Option off: no controller subsystem is started and scripted controller
  events produce no action; every existing test passes unchanged.
- Unit tests cover every cell of the mapping table, the Yes/No rule, the
  Russian resolution, the strafe events and the text-entry cycle.
- SDL loop tests push `SDL_CONTROLLER*` events, with all state taken from
  the event payloads (no device needed), and show under the synthetic
  clock:
  - a held D-pad walks the same steps as the held key and adds none after
    release; forward held with LB, and D-pad Left with LB, each do what
    the two keys do;
  - a press made while a frame is being presented is dropped as its key
    is. The test creates that order for real, because a pushed event gets
    SDL's own tick, not the test's;
  - events queued before a loop ends are still there for the next loop
    with the option on;
  - a break after a context change releases its key; removal and focus
    loss release keys and the click, and idle work resumes;
  - noisy sticks at a threshold and near a diagonal make one press;
  - a burst of `SDL_JOYAXISMOTION` inside the dead zone makes no pass,
    also in an any-key dialog, as the mouse-motion test shows today;
  - a pointer click hits the same button as a mouse click in both picture
    shapes, and the mouse takes the pointer back;
  - a character is changed in a strict entry (Create's name) and in an
    Event word entry, in English and in Russian; a fast second press is
    neither lost nor doubled, and a Russian entry never receives a raw
    arrow.
- One original-data scenario replayed with controller input reaches the
  same approved digest as its keyboard run, and the refusal test of
  decision 10 passes.
- The complete suite passes on Windows and WSL Linux; the independent
  implementation review has no open finding.
- One play-test session, only for what devices and feel decide: on a Steam
  Deck in Game Mode with the Gamepad template (walk, fight, a service, name
  a character, save, processor load); the M112 layout with the option off
  and on (no double input); Desktop Mode; on Windows with a controller,
  unplugging and replugging while walking; pointer speed, stick thresholds
  and how changing a character feels. No DOSBox comparison saves: no item
  compares behavior with DOS.
