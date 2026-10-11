# Milestone 59 plan - Vertigo content

**Tier A, in two parts. Status: approved; implementation not started.**
Baseline: `main = origin/main = 8ce2ccbdc5534ccc8bc682094267a0789d2c3235`;
HEAD and clean working tree verified on 2026-10-11. Planning only.

## Goal and classification

Finish the remaining city content of logical map 28 through the existing Event,
Service, World and Party owners from [M56](milestone-56-plan.md),
[M57](milestone-57-plan.md) and [M58](milestone-58-plan.md), following
[roadmap step 5](roadmap.md#near-term). Coordinates below identify evidence,
never runtime admission, per-cell behavior or a route manifest.

Upgrade the roadmap's tentative Tier B classification to **Tier A**: Event year
advance changes calendar/reset semantics; typed damage consumes shared combat
rules and potentially RNG; world flag 23 needs durable state absent from v10.
Part A supplies these foundations and jail/well; Part B completes city content
and the Guild interface. Both remain one Tier A milestone with independent plan
and implementation reviews, tests and maintainer acceptance. Part B alone is
content/UI work, but splitting it into another milestone would not remove A's
dependencies. This proposal does not authorize either implementation part.

## Read-only inventory

Decoded all **865 records, 7,504 bytes**, no exact duplicates, using existing
`build-rel/mmodern.exe --inspect-events "F:\Games\gog\Might and Magic 4-5" 28`.
Source was CD `GAME1.INS`/`GAME1.GOG`, initial Clouds `maze0028.evt`, not the
root floppy CC. Also inspected the CD initial MOB in memory using the existing
bridge's block/index rules: 143 objects; no sprite 16, 58 or 73. No original
resource was written or copied into the repository. Classification is static
script/dependency inspection against current decoder/interpreter/publication
code, not a claim that gameplay tests were run. Offsets are half-open bytes.

| Mechanic and complete family | Baseline classification and M59 disposition |
| --- | --- |
| Display cases: `(12,12)`, `(8,12)`, `(9,3)`, `(10,3)`, `(11,3)`, `(9,5)`, `(10,5)`, `(11,5)`; `[3025,4153)` | **In scope; refuses.** WhoWill, If3 action 102 (Thievery <= 0), GiveMulti 66/1 on success; failure grants mode 85/1, selects all, grants award 8, displays text, alters line 0 and teleports to `(27,11)`. Missing comparison/year/teleport admission blocks the dependent script. |
| Well `(14,17)`; `[834,957)` | **In scope; both branches refuse.** Game flag 9 selects AlterHed 0/0 + WhoWill + Damage 5/type 5, or AlterHed 1/1 + WhoWill + comparison/grant 78 + world flag 103/23. Unsupported heads, typed damage and grants must become generic. |
| Vern `(16,11)` East; `[648,715)` | **Already supported effects; in scope for verification/UI.** NPC mode 2, WhoWill verb 21, debit 34/25 and award 15/0 already supported. Membership is award 0 on the selected roster owner, not a new party-wide flag. |
| Guild `(20,13)`; `[30,37)` | **In scope; refuses.** DoTownEvent 2 is not admitted. Add original member/non-member lobby and character selection; Buy Spells and Spell Info remain **deferred to step 6**, visibly unsupported. |
| Monster crates `(6,25)`, `(6,26)`, `(8,26)`, `(8,25)`, `(8,23)`, `(8,22)`, `(9,23)`; `[1495,1950)` | **Already supported operations; in scope for combined verification.** MoveObj records 1..7, Spawn slot 2/type 0, AfterEvent; shared empty-search call `(99,100)`. Verify combined actor/object publication and normal combat, not a new spawn owner. |
| Item crates `(9,28)`, `(8,28)`, `(9,26)`, `(9,25)`, `(3,25)`; `[1950,2255)` | **Already working M57 family.** MoveObj + generated level-1 item + AfterEvent; retain repeat-search and capacity rules. |
| Empty crates `(7,2)`, `(7,6)`, `(5,10)`, `(5,14)`, `(6,16)`, `(6,17)`, `(6,18)`, `(3,26)`, `(3,28)`, `(4,28)`, `(5,25)`, `(5,26)`, `(25,13)`, `(27,15)`; `[2255,3025)` | **Already supported.** WhoWill, open-crate MoveObj, empty text and AfterEvent. Verify appearance/repeats within city coverage. |
| Rubbish `(1,12)`, `(1,10)`, `(3,11)`, `(3,13)`, `(3,9)`, `(4,12)` and item search `(3,12)`; `[1037,1295)` excluding sign `[1101,1108)` | **Already supported.** Empty searches and one GiveMulti 66/2; MOB sprite 7, not a locked chest. |
| Beds `(3,17)`, `(28,7)`, `(3,5)`, `(28,3)` give level-1 items; `(3,21)`, `(3,7)`, `(3,3)`, `(28,4)`, `(28,6)`, `(28,12)`, `(27,12)`, `(22,11)`, `(18,11)` give empty text; `[4153,4677)` | **Already supported.** MOB sprite 3, WhoWill/GiveMulti/AfterEvent; no MoveObj. Keep beds visually unchanged. |
| Locked-chest GiveMulti | **Deferred: absent in Vertigo CD MOB.** No selected sprite 16/58/73; crates, cases, beds and rubbish use 6/4/3/7. Do not implement the reference's lock/trap/XP branch merely because baseline diagnostics mention chests. |
| Signs and talk: Joe/shop `[455,514)`, `[715,834)`; citizens `[957,1037)`, `[1101,1108)`, `[1295,1449)`; door labels `[4677,4726)`, `[7190,7204)` | **Already supported**, including proof-dependent shop text. Preserve resource-driven text and directions. CD/voice/FX audio stays **deferred post-release**, without blocking supported text/effects. |
| Gunther `[51,455)`, teachers `[514,648)`, services `[0,30)`, `[37,51)`, trees `[4790,6688)` | **Already working M56-M58 mechanics.** All 26 trees use flags 232..255,57,58; retain supported later Gunther branches without adding the out-of-city quests that set their flags. |
| Exit `[6688,6754)` and helpers `(99,101)`, `(99,100)` `[1449,1495)`, `(100,100)` `[6754,7190)`, `(75,76)` `[7204,7504)` | **Already supported.** Empty text, actor reset and tree-cycle flags are dependencies, not city cells. |
| Trap teleports | **In scope through jail.** All eight 0x1F records belong to the display cases above. There is no separate trap-teleport family in this EVT. |
| Mirror `(14,10)` West; `[4726,4790)` | **Deferred post-release.** ConfirmWord and mirror destination lookup remain unsupported; do not publish its tree-reset prelude before refusal. Map-23 fountains and other mainland/post-release content are outside M59. |

## Part A - time, damage, jail and well

- Implement action 102 through the existing Thievery calculation and selected
  member. The case script compares against zero; do not invent a lockpick die
  roll or grant lockpicking XP. Success uses the shared treasure delivery rules.
- Implement give mode 85 as the reference's **direct year jump**, once per
  operation: increment year, reset yearly game flags 55/155/222/231, reset active
  members' temporary stats and party temporary effects, mark rested. Preserve
  day/minutes; do not simulate 100 days of condition ticks, stock or interest.
  Ages/maxima derive from the new year through existing rules. Audit guards and
  economy/calendar invariants so the next ordinary time advance remains exact.
- Failure then grants award 8 (Convicted Thief) to the active party, preserves
  script text/order and moves to `(27,11)`. Implement TeleportAndContinue's
  destination-script restart, facing/input handling and same-map behavior
  generically in World/Event/Flow. The party walks out using ordinary navigation.
- Decode AlterHed as the primary map's left/right head indicators at
  `(x % 16, y % 16)`; present them through the original interface. World owns
  this bounded head cache, preserving coordinate aliasing across the indoor
  megamap; do not use exploration's physical-tile keys or character conditions.
- Typed Damage uses existing combat resistance, mitigation, death/unconscious
  and portrait rules with the retained Journey RNG. Poison-type damage is not
  an instruction to apply the Poison condition. Stage any draws with the effect.
- Well before proof: 5 poison-type damage, no message (DOSBox CD, 2026-10-11).
  After proof: normal-maximum healing, never an above-maximum bonus, with
  "You feel refreshed". Retain action 78's exact max-HP comparison and the
  resource's alternate text branch; world flag 23 is written in script order.
  Each use affects only the member chosen in WhoWill (maintainer DOSBox CD,
  2026-10-11); no hardcoded well branch or party-wide effect.
- Extend existing staged publication to calendar, heads, HP, flags and camera:
  cancellation before commitment changes nothing; repeated acknowledgments,
  settlement retries and stale input cannot reapply, reroll or respawn. Keep
  saving disabled across transient Event and Service states and settle once.

## Part B - city searches and Vern/Guild

- Verify/fix combined generic mechanics for the inventory families, retaining
  original selection, text, item delivery, actor identity and repeat behavior.
  Searched crates become open crates; beds do not change; repeat searches say
  "Nothing here." (maintainer DOSBox CD evidence, 2026-10-10).
- Vern's original offer says "I'll put anyone on the guild roll for free.
  Dues will cost 25 gold"; charge 25 carried gold per selected character via
  F1-F6/portrait, atomically with award 0. No/Escape/insufficient funds do not
  charge. The script has no membership test: retain its repeat-payment behavior.
- Guild uses the current character's membership: non-members see "You have to
  be a member to shop here"; members see Buy Spells / Spell Info, both visibly
  "not supported yet". Reuse terminal service handoff, original art/DAT text,
  character switching and departure-day settlement. No spell catalogue work.

## Save decision and evidence

Propose **v11, schema 9, content 14**, replacing v10; no legacy reader. Add one
bounded generic world-flag state (including existing flag 16 and new flag 23)
and head-cache overrides for exact quiet-boundary restoration, removing the duplicate
special-purpose flag-16 owner. Preserve the established reference map-reload
lifetime of head data; serialization must not turn a cache effect into a
permanent map change. Awards, skills, year, temporary stats, actors, objects,
Event changes and RNG already persist. Part B retains v11. Any different save
design requires amending this plan before implementation.

Before source changes, preserve M44 audit binaries/evidence with
[`tools/m44-digest-audit.ps1`](../tools/m44-digest-audit.ps1) `-Step Preserve`;
afterward run `-Step Compare`, updating the dump's stable field visitor. Explain
all changed fields/bytes, require unchanged gameplay traces and exact native
reload/re-save, and obtain maintainer approval before replacing digest values.

Authority: [dependencies](dependencies.md), verified clean ScummVM pin
`6814ee9ba54582f5b5adcffab49efbbd8f589edd`: `scripts.cpp` cmdIf/ifProc,
cmdDamage/cmdAlterHed/cmdTeleport; `party.cpp` giveTake/resetYearlyBits/resetTemps;
`character.cpp` getThievery/guildMember; `locations.cpp` GuildLocation.
Maintainer DOS evidence takes precedence. Retained oddities are recorded in
[original quirks](original-quirks.md); add any newly settled discrepancy there.

## Risks, DOS questions and acceptance

- **Well recipient settled:** one use damages or heals only the WhoWill-selected
  member (maintainer DOSBox CD, 2026-10-11), matching the script and reference.
- Ordinary play comparisons should confirm jail day/time and temporary/reset
  effects, the already-above-max well branch, head-indicator lifetime after
  leaving/re-entering, and Vern's repeat payment. These are reference-defined
  checks, not reasons to reopen settled DOS evidence; contradictory observations
  go to the maintainer rather than silently changing scope.
- Rules tests: selected Thievery, year/age/reset semantics, typed damage and RNG,
  healing boundaries, world flags/heads, membership, exact/insufficient gold,
  cancellation and exactly-once publication including stale guards and retries.
- Generic current-format save round-trip covers all affected owners, both flags,
  heads, jail/year state and RNG with no replay; no separate process witness per
  feature. A few original-data runs prove case success/jail + ordinary exit,
  well before/after proof, Vern + mixed-membership Guild entry, one monster crate.
- Build and complete `build-rel` suite after each final production task through
  the [test-runner protocol](../tools/AGENTS.md); Debug closure runs fast tests
  and three M44 scenarios. No tests/builds are part of this planning task.
- Independent Tier A plan review before implementation and implementation review
  before closure; resolve findings. Maintainer Debug/DOSBox CD play-test and
  explicit acceptance are required before recording completed status.
