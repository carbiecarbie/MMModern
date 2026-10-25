# Milestone 89 plan - Clouds scripted kills and map rules

**Tier B. Status: completed and accepted by the maintainer (2026-10-25)**
after the play-test. Plan `67b15da`, implementation `3182b40` and the
closure commit. Roadmap
[step 15](roadmap.md#next-arc-finishing-the-games-systems), after
[M88](milestone-88-plan.md). Save v20, schema 9 and content 14 unchanged.

## Scope and results

The Clouds rules that live in the original program rather than in the
Events now work as in DOS, so the main quest reaches the Sixth Mirror and
three side quests can finish:

- **Lord Xeen.** Only the Xeen Slayer Sword hurts him, every melee hit gets
  +100, and spells, Poison Volley and Shoot do nothing. His death gives the
  Scepter through the ordinary treasure screen, sets his defeat flag, ends
  the Lord Xeen quest and gives every active member the "Defeated Lord Xeen"
  award, dead members included.
- **Scripted kills.** Killing the Clan King completes Gunther's dwarf quest,
  and killing Count Draco makes the Nightshadow well grant ten levels.
- **Ice Trolls** heal fully at each new combat round.
- **Map rules.** Leaving the lake map with its three Water Dragons dead
  completes Medin's quest; flag 25 drains the three caves; nine completed
  quests hide their neighbouring-map objects; Newcastle shows the castle
  once its walls are bought.
- **Sixth Mirror.** Its text runs as in DOS and the ending then says
  "Clouds ending: not supported yet" (step 18).

The rules, offsets and ScummVM differences are in
[original quirks](original-quirks.md#sv-slayer-sword-exact-id) and
[combat rules](rules/combat.md#lord-xeen-and-scripted-death-hooks).

## Key decisions

- **Evidence.** The completion audit's DOS static findings, rechecked range
  by range by the implementer and the independent review, and the
  maintainer's DOSBox run of the ending (Scepter to the first member even
  when dead; only the sword hurts Lord Xeen).
- **Exact Slayer Sword.** Only weapon 34 bypasses physical resistance
  (`0x2beb6`), not 34 and above as the reference has it.
- **RNG.** Shoot against Lord Xeen takes DOS's zero-damage exit and no
  longer draws the monster's save (`0x2be33` jumps past `0x2c037`); the
  maintainer approved this and any draw a DOS rule of the plan requires.
- **Map 50.** The audit's stepped-map fill on map 50 is Darkside only in DOS
  (`0xf7ee` skips it on Clouds); it was removed from the scope and waits
  for the Darkside arc.
- **Object table.** DOS applies only the first matching entry (an else-if
  chain); on map 15 flag 66 suppresses the flag-69 entry, kept as an
  original bug.
- **No saved state.** The death rule publishes with the XP on all three
  death paths (melee, spells and Poison Volley, Shoot outside combat); the
  map rules derive from saved flags at load. M44 digests unchanged.
- **Opcode 30** decodes as a known operation that ends the Event with a
  notice, so the Event preflight no longer refuses the whole Sixth Mirror.

## Results

Complete suite 297/297 (run `20261025-001800-99d87e16`), M44 digests
unchanged, census: opcode 30 moved from unsupported to partial. The
independent implementation review (pre-authorized) approved after one fix
round (map 50, the else-if chain, Shoot and spell kill tests, cleanups).
The maintainer play-tested the presets `m89-lord-xeen`, `m89-clan-king`,
`m89-draco`, `m89-dragons` and `m89-trolls`
([development guide](development.md)).
