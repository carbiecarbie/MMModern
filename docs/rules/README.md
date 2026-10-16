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
| [Magic](magic.md) | All 76 spell costs, all 73 item powers, casting and cancellation, healing, damage, buffs and travel boundaries |
| [Items and equipment](items.md) | Item representation, equipment slots, prices, Smith transactions, charges, Discard, generation and treasure delivery |
| [Time, rest and conditions](time-and-conditions.md) | Calendar, eight-hour checks, per-action recovery, fatigue, temporary effects, Rest, food and bank interest |
| [World Events](world-events.md) | Wall items, cell walls and surfaces, physical Event replacement/deletion, Spawn and map-state lifetimes; If/SetVar/TakeOrGive value modes, rewards, lock picking and random draws |
| [Evidence and unresolved questions](evidence.md) | Exact original identity, REA evidence, calibration, original file offsets, limitations and the next questions to resolve |

The reading order for implementation is the relevant rule, its evidence label,
then the linked source or original offsets when details matter. A concise formula
does not replace a nearby exception about input range, rounding or draw order.

## How to interpret evidence

| Label | What it establishes |
| --- | --- |
| DOS play | A retained maintainer observation in normal original-game play; only the described case is established |
| DOS static | A calibrated read-only analysis of the identified original program; a high-confidence code rule, not a claim of observed execution |
| DOS static (retained) | Earlier calibrated evidence reused from the project's records, with its offsets and provenance |
| Reference only | Verified behavior in the pinned ScummVM source; it may still differ from the original |
| Unknown | An explicit evidence gap; do not turn a hypothesis into an implementation rule |

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

Research baseline: MMModern `9cc4def8c5c0253899c094efbe5b4d7e4513b22c`,
ScummVM `6814ee9ba54582f5b5adcffab49efbbd8f589edd`, investigated on
2026-10-12. [Dependencies](../dependencies.md) owns the pinned dependency setup;
[project status](../project-status.md) owns playable coverage;
[original quirks](../original-quirks.md) owns oddities and deliberate differences.
This reference supplies reusable rules and evidence without duplicating milestone
acceptance or declaring production behavior changed.

The original program, raw byte reads, disassembly, decompiler output and analysis
projects stay outside the repository. The chapters contain descriptions,
derived formulas and source locations only. Pinned source links are stable
locators; they are not citations to a newer ScummVM revision.

The strongest new checks are item dispatch/cancellation, Resurrection cancellation,
Nature's Cure and monster treasure staging. Character/combat formulas and broader
spell effects also have a substantial reference inventory, with the DOS gaps
explicit. Full Event opcode semantics, every service, all terrain hazards,
monster AI, character creation, audio and frame-level interface timing are not
exhaustively covered. Darkside-only and World-linking behavior need their own
normal-play calibration before being treated as verified for those campaigns.
