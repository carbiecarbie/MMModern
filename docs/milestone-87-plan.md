# Milestone 87 plan - The Inn and a party of one to six

**Tier A. Status: completed and accepted by the maintainer (2026-10-24)**
after the play-test. Plan `f77720f`, Part 0 `4ed3a15` (with the plan
amendment), Part A `dfa3788`, Part B `4182202` and the closure commit.
Roadmap [step 14](roadmap.md#next-arc-finishing-the-games-systems), first of
its two milestones, after [M86](milestone-86-plan.md). Save v19; Journey
schema 9 and content 14 unchanged.

## Scope and results

As in DOS, Sign In at the Vertigo, Rivercity or Shangri-La tavern lodges the
party, adds a day, saves the game and opens the original Inn with its song.
The Inn lists the characters lodged in that town; members are added up to
six and removed, and Exit saves again and returns the party to the tavern
door, reloading the map. Loading a save made at Sign In reopens the Inn.
Dismiss works in exploration, and the next save lodges the dismissed member
in Vertigo. The game plays with any one to six roster characters in any
order, in every rule that counts members. Create and Delete say "not
supported yet" until M88. The rules are in [party rules](rules/party.md).

| Part | Commit | Result |
| --- | --- | --- |
| 0 - DOS evidence | `4ed3a15` | Roster, Inn list, Sign In and its saves, Load into the Inn, return, Inn input, Dismiss and its deferred lodging, consumer bounds, the count-dependent target draw |
| A - membership and save | `dfa3788` | Counted six lanes, membership validation, DOS target draws, Dismiss, full roster records (CHR byte 339 unknown, 340-341 lodging map), save v19 with an Inn state |
| B - the Inn | `4182202` | Sign In, the Inn screen and input, both saves, Load into the Inn, return, scrolls at one BIOS tick with input skip (death scroll too), presets `m87-inn`, `m87-three`, `m87-shangrila` |
| C - closure | This change | Coverage checklist, status, history, roadmap, README, controls |

## Key decisions

- **Step 14 in two milestones** (maintainer, 2026-10-23): M87 holds the core
  risk (membership, every six-lane consumer, the save layout); M88 manages
  roster slots, Create and Delete. Delete moved with Create because without
  Create a deletion could leave an empty Inn with Exit refused.
- **Sign In saves as in DOS** (maintainer, after the first Part 0 stop): the
  save records the party inside the Inn, as DOS does with map 0; MMModern
  keeps the camera at the tavern door with an Inn flag instead.
- **Monster target draws as in DOS** (maintainer, after the second Part 0
  stop): none for one participant, `R(1,6)` then `R(0,4)` unless 6 for six.
  Six-member draw sequences changed, so the four M44 digests were replaced
  after a three-step audit (old targeting and v18 reproduce the old digests;
  the first divergence is a six-participant draw; v19 adds only new fields),
  approved by the maintainer on 2026-10-24.
- **Lodging layout** (maintainer, during Part A): DOS writes the lodging map
  as a word at CHR `+0x154`; MMModern and the pinned reference had read it
  one byte early, a recorded ScummVM difference.
- **Dismiss's tail** is the only read of an inactive lane: it lives in the
  running program, and every save lodges it in Vertigo and clears it.
- **WhoWill** survives Dismiss and Exchange, as in DOS, and is bounded at
  delivery (independent review).
- **Approved deviation** [`dev-inn-save-recovery`](original-quirks.md#dev-inn-save-recovery)
  (maintainer, 2026-10-24): a failed Inn save shows the error and retries
  instead of quitting.
- **The death scroll joined Part B** (maintainer, 2026-10-24): it shares the
  DOS scroll routine, so it got the same input skip and stall rule.

## Validation

- Independent plan review (two rounds), then one independent implementation
  review per part, each with its fix rounds applied and re-checked.
- Complete suite: 285/285 after Part A, 289/289 after Part B's last fix
  round; M44 digests unchanged by Part B.
- The maintainer play-tested the presets; the one defect found, a less fluid
  Inn scroll, was fixed to DOS's one-step-per-tick cadence and confirmed.
- Open evidence: Poison Volley's missile count with fewer than six members
  ([party rules](rules/party.md)); DOS skips scrolls when `[0x94]` is nonzero
  (`0xad12..0xad2a`), and since [combat rules](rules/combat.md) name
  `ds:0x94` the archive selector, this may concern Darkside only
  (unverified); and the runtime checks in
  [M87 evidence](rules/evidence.md#m87-runtime-checks).
