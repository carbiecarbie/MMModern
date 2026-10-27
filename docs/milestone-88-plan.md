# Milestone 88 plan - Create and Delete at the Inn

**Tier A. Status: completed and accepted by the maintainer (2026-10-24)**
after the play-test. Plan `e862107`, Part 0 `b54f893` (with the plan
amendments), Part A `eba589d`, Part B `d8168c4` and the closure commit.
Roadmap [step 14](roadmap.md#next-arc-finishing-the-games-systems), second
of its two milestones, after [M87](milestone-87-plan.md). Save v20; Journey
schema 9 and content 14 unchanged.

## Scope and results

As in DOS, the Inn's Create opens the original creation screen: the dice
roll the seven attributes, retrying until a class qualifies, the classes
they allow light up, attributes can be exchanged, and Up and Down choose a
free roster slot with its portrait, race and sex. Create asks for a name and
writes the whole record as DOS initializes it, lodged in the Inn's town.
Delete removes a listed character after confirmation, unless it carries the
Xeen Slayer Sword. Every return from Create and every confirmed Delete saves
the game, also with an empty party, and loading such a save reopens the
Inn. A save loaded into the Inn from the title opens it without music until
Exit. The rules are in [Create and Delete](rules/party.md#create-and-delete).

| Part | Commit | Result |
| --- | --- | --- |
| 0 - DOS evidence | `b54f893` | Free slots, rolls and retry, classes, screen and input, the created and deleted records, both saves with an empty party; the maintainer's DOSBox checks; the Inn song flag |
| A - save v20 and Delete | `eba589d` | Empty Inn party in save v20, whole-record operations in the Encounter/Inn owner, Delete, empty-party Load, the Inn song flag, version-only digest audit |
| B - Create | `d8168c4` | Creation rules and screen, rolls through the shared range helper, return saves, Part A review deferrals, preset `m88-roster` |
| C - closure | This change | Coverage checklist, status, history, roadmap, README, controls |

## Key decisions

- **Tier A with save v20** (maintainer, 2026-10-24): DOS saves an empty
  party inside the Inn after Delete and every Create return, which v19
  refused. Only the validator's meaning changed: the four M44 digests differ
  only in the version byte and were replaced after a counterfactual audit
  the maintainer approved.
- **Whole-record publication** stays in the existing Encounter/Inn owner:
  complete character and supplement successors are validated against the
  Journey and save-format rules before publication; no new owner.
- **DOSBox checks before Part A** (maintainer): five of the six ran, the
  globally full roster was skipped, and all agreed with the static rules.
  They also showed that a title Load into the Inn plays no song.
- **The Inn song after a title Load joined Part A** (maintainer): DOS's flag
  `DS:8d28` is set by the title menu and the Inn's song start and cleared by
  the Inn routine's exit. MMModern keeps it in the running program, never
  saved ([`sv-inn-title-load-song`](original-quirks.md#sv-inn-title-load-song)).
- **Range draw for the rolls** (maintainer, after the Part B review): Create
  and Events share one helper, MMModern's unbiased range draw. DOS's
  one-value 15-bit conversion is not used, and DOS clock mixing stays
  deferred ([`dos-create-reroll-choice`](original-quirks.md#dos-create-reroll-choice)).
- **Approved deviation** [`dev-inn-save-recovery`](original-quirks.md#dev-inn-save-recovery)
  now also covers Delete and the Create return: a failed write shows the
  error and retries without repeating the change or the rolls.
- Differences from the pin are recorded as `sv-create-class-selection`,
  `sv-create-dice-bounds`,
  `sv-create-current-spell`, `sv-create-delete-beacon-clear`,
  `sv-inn-delete-list-page` and `sv-inn-title-load-song`; DOS oddities kept
  as `dos-create-reroll-choice` and `dos-inn-empty-snapshot-count`.

## Validation

- Independent plan review (approve with changes, all applied), then one
  independent implementation review session for Parts A and B and their fix
  rounds, each approved.
- Complete suite: 289 tests after Part A (the four M44 scenarios rerun after
  rebuilding a stale tool, accepted by the review), 290/290 after Part B and
  again after its fix round; Part B left the M44 digests unchanged.
- The maintainer play-tested Create and Delete with `m87-inn` and
  `m88-roster`, using fresh v20 saves, and found no defect.
- Open evidence: the globally full roster with fewer than 24 characters
  listed locally rests on the static rule; during fades MMModern keeps the
  outgoing image and swaps it at black, within the documented timing.
