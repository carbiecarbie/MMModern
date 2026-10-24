# World of Xeen rules reference

This reference explains game mechanics for faithful implementation, with formulas,
operation order, random draws, examples and evidence. It covers the installed
English World of Xeen CD edition used by MMModern. It is not a claim that every
rule is known, that all editions behave identically, or that every listed feature
is implemented.

## Find a rule

| Chapter | Consult it for |
| --- | --- |
| [Combat and character capacity](combat.md) | Attributes, HP/SP, armor class, hit and damage, targeting, saves, initiative, Run, experience and monster treasure |
| [The Inn, membership and party size](party.md) | Town roster lists, Sign In and its saves, Create rolls/input/record initialization, Delete and empty-party saves, Inn return, Dismiss and deferred lodging, shared state and active-count consumers |
| [Magic](magic.md) | All 76 spell costs, all 73 item powers, casting and cancellation, healing, damage, buffs and travel boundaries |
| [Items and equipment](items.md) | Item representation, equipment slots, prices, Smith transactions, charges, Discard, generation and treasure delivery |
| [Time, rest and conditions](time-and-conditions.md) | Bash, calendar, eight-hour checks, per-action recovery, fatigue, temporary effects, Rest, outdoor movement skills, terrain arrival and falls, food, bank interest, Training, Tavern options and Temple prices/effects |
| [World Events](world-events.md) | Wall items, cell walls and surfaces, physical Event replacement/deletion, Spawn, the Battle Master and Warzone, and map-state lifetimes; If/SetVar/TakeOrGive value modes, rewards, lock picking, Gold Veins and random draws; teleports, mirrors and typed passwords; entrance scenes; the Quests and Awards screens |
| [Audio](audio.md) | CD promus commands, stereo, ticks/fades, song selection, saved options, fixed-layout VOC playback and waits, CD addressing/lifecycle, Text off, mix balance, output recovery and remaining gaps |
| [Evidence and unresolved questions](evidence.md) | Exact original identity, REA evidence, calibration, original file offsets, limitations and the next questions to resolve |

The reading order for implementation is the relevant rule, its evidence label,
then the linked source or original offsets when details matter. A concise formula
does not replace a nearby exception about input range, rounding or draw order.

## How to interpret evidence

| Label | What it establishes |
| --- | --- |
| DOS play | A retained maintainer observation in normal original-game play, including DOSBox screenshots, video, DRO and audio captures and measurements taken from them; only the described case is established, and a DOSBox capture shows the emulator's output rather than DOS hardware |
| DOS static | A calibrated read-only analysis of the identified original program (also written calibrated DOS static, DOS-confirmed static or, for port accesses, I/O audit); a high-confidence code rule, not a claim of observed execution |
| DOS static (retained) | Earlier calibrated evidence reused from the project's records, with its offsets and provenance |
| Resource inspection | A read of the installed original data files (also DOS resource read, resource audit or resource census); it establishes their contents; how the program uses them needs DOS static as well |
| Emulator source | Published device documentation or DOSBox source at a named release (also installed-emulator source); it establishes what the device or emulator does with the program's requests, not the program's own behavior |
| Reference only | Verified behavior in the pinned ScummVM source; it may still differ from the original. A reference comparison contrasts it with the stated DOS evidence |
| Inference | A conclusion drawn from the evidence it names, at its stated confidence; neither an observation nor a code path |
| Maintainer decision | A choice the maintainer approved, such as a bound, a fallback or an approved deviation; it sets MMModern's behavior and is not evidence about the original |
| Unknown | An explicit evidence gap (also Unresolved); do not turn a hypothesis into an implementation rule |

A label may name its confidence and scope ("high for the table", "medium for
owner mapping"); a medium-confidence part is not settled DOS evidence. Joined
labels such as "DOS play + DOS static" or "DOS static/resource" give each
source, and "Mixed" means the section states the label of each part.

The authority order remains [AGENTS.md](../../AGENTS.md): original DOS evidence
overrides the pinned reference. Reference-only documentation does not independently
authorize a deviation, a new feature or a milestone. The special fallback for
unreachable condition-counter edges remains exactly as defined there.

`R(a,b)` and `U(a,b)` both denote one inclusive bounded random draw. They describe
the call contract, not proof of the DOS generator's statistical uniformity or
bit-for-bit equivalence with ScummVM. Keep draw count and order even when a draw
does not affect visible damage. Division of nonnegative integers truncates down;
signed arithmetic and overflow require the specific evidence stated at the rule.

## Baseline and responsibilities

Chapters state their own check dates and any local baseline; pinned source
links refer to ScummVM `6814ee9ba54582f5b5adcffab49efbbd8f589edd`.
[Dependencies](../dependencies.md) owns the pinned dependency setup;
[project status](../project-status.md) owns playable coverage;
[original quirks](../original-quirks.md) owns oddities and deliberate differences.
This reference supplies reusable rules and evidence without duplicating milestone
acceptance or declaring production behavior changed.

The original program, raw byte reads, disassembly, decompiler output and analysis
projects stay outside the repository. The chapters contain descriptions,
derived formulas and source locations only. Pinned source links are stable
locators; they are not citations to a newer ScummVM revision.

Event opcodes beyond the World Events chapter, every service, monster AI,
character creation and frame-level interface timing are not exhaustively
covered. Darkside-only and World-linking behavior need their own normal-play
calibration before being treated as verified for those campaigns.
