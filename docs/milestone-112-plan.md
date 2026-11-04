# Milestone 112 plan - Steam Deck through Proton

**Tier B. Status: completed and accepted by the maintainer (2026-11-04)**,
after a play-test on a Steam Deck. Plan `c8fb586`; player log fix found
during the play-test `9161d61`. First half of
[later goal 3](roadmap.md#later-goals): the native Linux build, its CI job
and its package follow as their own milestone (Tier A). Save v26 unchanged;
no gameplay change.

## Scope

The Windows release package plays on a Steam Deck through Proton, started
from Game Mode, by Steam Input, trackpad and touch, and a README section
tells players how to set it up. Controller support, a launcher that runs
under Proton, sound setup and the Russian language on the Deck were out of
scope.

## Key decisions

- Steam starts `mmodern.exe "<game folder>"` directly with the folder in
  `Z:\...` form; the WPF launcher does not run under Wine.
- The game keeps keyboard and mouse input: the README gives a Steam Input
  layout built on Steam's Keyboard (WASD) and Mouse template.
- The default 4:3 picture stays (1066x800 with side borders on the Deck);
  `aspect=square` fills the 1280x800 screen.
- The play-test had no DOSBox comparison saves, because no item compared
  behavior with DOS.

## Results

- Under Wine 10 (WSL) the package reported usable data, started the title
  with the game folder in `Z:\...` and Linux forms, played a new game, saved,
  quit and reloaded at the same position, and wrote its log in the prefix;
  1280x800 frames for both aspect settings were inspected. No code fix was
  needed for Proton.
- On the Deck the maintainer played the title, a new game, walking and a
  save with the README layout; the log showed usable data and no notices.
  Sleep and resume and `aspect=square` were not played; the maintainer
  accepted the milestone on the main result.
- The Deck log showed that every exploration input wrote the developer
  notice and the whole Journey inspection, about 100 MB per hour, on every
  platform. Sessions from the original title now keep that report out of the
  player log (`9161d61`, complete suite 416/416).
