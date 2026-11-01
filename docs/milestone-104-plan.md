# Milestone 104 plan - Mixed World of Xeen maps

**Tier A. Status: completed and accepted by the maintainer (2026-11-01)**
after the play-test. Plan `06ddf1d`, DOS check (Part 0) `0484419`, loading
and saving the mixed maps (Part A) `dc88cd2`, the ways in and out (Part B)
`0c91498`, and the closure commit. Step 3 of the
[Darkside arc](roadmap.md#darkside-arc). Save v23 -> v24; Journey schema 9
and content 14 unchanged.

## Scope

A World of Xeen party carrying the Darkside keys enters Darkstone Tower,
Dragon Tower, the Dungeon of Death and the Southern Sphinx from their Clouds
sites and reaches the Dragon Clouds, as on the CD. The sixteen mixed maps
(Darkside 113-128) load with the resources, monsters, music and side rules
DOS uses for each, FlipWorld (opcode 59) sends the party back to the side DOS
chooses, and Town Portal, Lloyd's Beacon, Teleport, Jump, falls, saves and
loads work from every one of them.

Out of scope: opcodes 57 and 58 and the endings (step 4), the three waiting
Tier A roadmap items, DOS save import and any further mechanic these maps
use, which keeps its census row and "not supported yet" notice.

## Key decisions

- **DOS check first** (Part 0, calibrated static analysis of
  `WORLD/XEEN.DAT`; [evidence](rules/evidence.md#mixed-world-of-xeen-maps-m104-part-0),
  [rules](original-quirks.md#mixed-world-map-rules-established-from-dos)).
  DOS agrees with the reference on every per-map selector, the Sphinx's
  Clouds scenery and fixed song, map 124's draw, map 128's hidden horizon
  and day sky, the falls and Town Portal. It differs in two pending-side
  rules: a same-map Beacon Return overwrites the pending side, and Save first
  makes it active.
- **Independent selectors per resource family, not one substituted side.**
  Each map identity resolves to separate monster table, monster art, object
  art, wall pictures, scenery, animation table, missing-cell slot, horizon,
  day sky, music and Town Portal choices (`xeenMapResources`). Map archive,
  Events, text and every gameplay rule (flags, quests, combat, voices,
  terrain, services) stay Darkside on all sixteen maps. Species 91 keeps its
  Clouds art. No new owner; nothing derived is saved.
- **Save v24, no new field.** Validation widens: the Sphinx (125-127)
  requires an absent song byte, the drawing maps a saved song; every saved
  map's actors rebind to their own monster table, so a history holding both
  tables restores. Saves from 0.13.0 and older (v23) are refused with the
  usual message.
- **FlipWorld as DOS does it.** It writes only the world's pending load
  side, staged and published with the existing detached candidates. The next
  different-map teleport, boundary, reload or fall uses it; a same-number
  teleport does not reload. Town Portal, every accepted Beacon Return and the
  keyed entrances overwrite it, and the Inn roster reads it. Saving while a
  pending side differs from the camera's says "not supported yet" and keeps
  playing ([approved deviation](original-quirks.md#dev-pending-side-save));
  no original Event leaves such a state.
- **Entrance keys.** The M80 rule treating Clouds keys 35 and above as
  absent is removed: the four sites (Clouds maps 2, 7, 13, 19; quest items
  51, 46, 45, 50) play their keyed outcome from the shared counters.

## Results

- All sixteen maps load through production admission; a parameterized
  original-data test checks every selector, restriction and boundary, the
  music draw (zero or one, with the next Journey draw) and exact restore.
- Live-input tests cover the four keyed entrances and their FlipWorld
  returns, Town Portal names and arrivals from both selectors, Beacon,
  Teleport, Jump, terrain and scripted falls (flag 118) and the 128 exit.
- M44 digests: Part A changed only the envelope version byte (replacement
  hashes approved); Part B left them unchanged.
- Complete suite 393/393 after Part A and 394/394 after Part B; independent
  reviews of the plan and of both parts.
- The runtime-refusal sweep leaves three Events outside this scope, each with
  a census row and a safe notice: a world flag on map 116 and two DisplayLarge
  instructions on map 121.
- The maintainer play-test passed every item: the Sphinx entrance and return,
  the look and music of maps 113, 117, 121, 124 and 125, Save and Load in the
  Sphinx, and Town Portal from 125 and 128.
