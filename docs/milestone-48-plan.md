# Milestone 48 plan - Generic presentation systems

**Tier B. Status: completed and accepted.** Part A `6387a0e`, Part B
`b0e4919`, review fixes `4d953c6`. The planned Part C (combat impact timing
and rotation, Tier A) moved to M49.

## Goal

Make scene, combat feedback and controls faithful, reusable engine systems
for **all Clouds content**, driven by original data, so new areas inherit
them without per-map or per-monster work. Presentation only: rules,
damage publication order, gameplay RNG, time, input-queue rules and the save
format are unchanged.

## Final scope

- **Scene (Part A).** Action-driven ground/sky flips and periodic water
  phase in both scene builders; MOB wall items and torches with their own
  frame cycles, direction, depth and occlusion; monster normal, looped
  (back-and-forth), attack/recovery, flying and effect presentation from
  each monster's data (ScummVM `Interface::perform/draw3d`,
  `InterfaceScene::animate3d/setIndoorsWallPics/setMonsterSprite`,
  `MonsterObjectData::synchronize`). `--render-map` shows actors and wall
  items for any map through World without starting combat. Generic
  presentation validation replaced `validateSlime`; combat admission
  (`validateAdmittedPoisonCombat`) is unchanged.
- **Combat visuals and controls (Part B).** Simultaneous data-driven
  projectiles in both projections with per-lane termination; original hit
  splats; portrait damage effects for every published injury, including
  off-camera attacks; four-frame healing effects for First Aid, the well and
  antidotes; Magic Arrow travel on the shared lane; the remaining combat
  party strip after Run with original highlight and target icon; original
  keys (S Shoot, A Attack, arrows move/turn; combat movement says "not
  supported yet"); the original `mouse.icn` cursor.

## Decisions

- **Presentation RNG.** Initial monster effect phases use a separate,
  unsaved presentation RNG instead of ScummVM's shared `getRandomNumber(7)`.
  It reproduces the observable random starting phases without touching the
  gameplay RNG; MMModern does not reproduce the original RNG sequence.
- **Archive resolution.** Scene resources follow the pinned `File::open`:
  the current or explicitly selected archive, then the registered INTRO
  search; DARK.CC is only ever an explicit selection (cross-side pictures),
  never a fallback.
- **Original-data gaps.** Clouds maps 106-108 reference `sewer.srf`, which
  exists only in DARK.CC while the pinned `Map::load` loads surfaces from the
  current archive. These three entries are the only allowed missing
  references; rendering those maps keeps an explicit missing-resource error.
- **Moved to M49 (Tier A):** applying damage/death/XP/drops at projectile
  impact instead of before the volley, and turning the party in combat
  (`Interface::doCombat`), both requiring combat authority changes.

## Results

- Data-driven tests over all Clouds content: 128 maps composed, every one
  of 1,422 expected MOB actors drawn, 464 wall records and 862 scenery frames
  exercised, zero missing references outside the allowlist; 29 ranged
  monsters and 7 projectile resources in concurrent lanes; all 64 combat
  strip masks through portraits, HP, effects, highlight, F-keys and clicks.
- Full CTest 152/152 (single job, lightweight runner); M44 scenario digests
  unchanged throughout.
- Independent review of Parts A and B (REVISE) found Magic Arrow without
  travel, missing off-camera portrait damage and a non-reference DARK.CC
  fallback; all fixed in `4d953c6`, with stronger inventory and strip tests.
- Maintainer play-test: scene animation, `--render-map` samples, volleys
  with several shooters, splats, portrait effects, Magic Arrow travel, the
  combat strip, keys and cursor passed. Known pre-existing gaps confirmed in
  play and moved to M49: monsters can vanish before the arrow arrives, and
  the Slime hits the whole party where the original (DOSBox) hits two
  members.
