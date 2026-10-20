# Milestone 78 plan - Opening areas

**Tier A. Status: completed and accepted by the maintainer (2026-10-20)**
after independent plan and implementation reviews and a maintainer
play-test. Plan `612a4ac`, DOS check (Part 0) `a0872f1`, traversal and
lighting rules (Part A) `48a2784`, opening, travel and save (Part B)
`4d38594`. Step 8 of the
[systems arc](roadmap.md#next-arc-finishing-the-games-systems), after
[M77](milestone-77-plan.md).

## Scope

Open the whole of Clouds beyond Vertigo and F3, with actors, Events and every
travel path; Mountaineer and Pathfinder outdoor traversal; the original dark
view and light drain. Other towns' services (step 9), entrance scenes and the
Arena (10), expanded treasure (11), audio, the Inn and Darkside stayed out:
they keep a clear "not supported yet" notice without changing state, time or
random draws.

## Key decisions

- **DOS check first** (Part 0, static analysis of `WORLD/XEEN.DAT`):
  Mountaineer and Pathfinder each need two members with the skill, dead or
  unconscious members included. Pathfinder lets the party cross outdoor wall
  types 3 and 6, a DOS rule the reference omits
  ([difference](original-quirks.md#sv-pathfinder-movement)). Dark maps
  (flags2 `0x4000`) are completely black unless light remains; Clouds has
  no light-drain cells, so drain is proven with synthetic fixtures. Evidence
  is in [magic](rules/magic.md#darkness-and-light-timing) and
  [evidence](rules/evidence.md).
- **Admission from the data**: a Clouds map is open when its original
  resources and coordinates load. Primary maps (`mazeNNNN`, below 100) are
  destinations; `mazexNNN` tiles are only physical parts of a larger indoor
  map, which keeps its logical identity and per-tile state. There is no map
  list, component whitelist or census gate; developer fixtures and the
  prepared F3 start stay separate from admission.
- **Travel**: outdoor border crossings prepare camera, monsters, Events,
  terrain, time and random draws on a detached copy and publish them
  together. Event teleports and stairs, the mirror, Town Portal's five towns,
  Lloyd's Beacon, Jump, Teleport, Etherealize, Time Distortion, Run, Mr
  Wizard and sky, cloud and scripted falls share one destination check and
  keep their original costs, facing and monster reloads. The special Vertigo
  entrance and exit became ordinary Event transitions.
- **Refusals before effects**: other towns' services, `DoTownEvent` 6 and
  8-12 and an outdoor AlterCellFlag aimed at a neighbouring map refuse
  before any write, time or draw, including at a Continue destination. The
  World CD protection check is a no-op on every map
  ([quirk](original-quirks.md#world-cd-protection-events), reference only).
- **Save v16**: only the envelope version changed; schema 9, content 14 and
  the byte layout stayed. Restore no longer requires F3's state or the
  regional manifest and binds the camera map's Events and text without
  replaying arrivals or draining light. v15 saves are rejected clearly.

## Results

- All 85 Clouds primary maps open; the census lists no destination refusal
  and no unresolved in-scope Event (supported Clouds Event rows 480 to 537).
- The M44 audit for Part B changed only the version byte in all 14
  checkpoints, with identical traces, reloads and resumes; the maintainer
  approved the four v16 digests. Part A replaced the services digest for the
  Pathfinder detour at map 23 `(3,2)`, also approved.
- Independent review findings (border presentation lifetime, a detached
  draw-budget yield, empty tables for an absent optional MOB, the Journey-only
  naming bound and stronger Flow-level tests) were fixed, followed by a
  focused recheck. Complete suite 250/250.
- The maintainer play-tested borders and terrain, the five towns, Dwarf Mine
  1 in the dark with Light, falls, the travel families, save and load in new
  areas, v15 rejection, indoor Run and Mr Wizard from outside Vertigo. The
  `m78-playtest` and `m78-one-holder` developer presets provide that party.
