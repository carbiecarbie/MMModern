# Milestone 65 - Coverage census

**Tier B. Status: approved; implementation not started.**

## Goal

Measure, from the original data, every mechanic the game uses and whether
MMModern supports it, so the next arc can finish the game's systems as a closed
list ordered by impact, instead of discovering gaps map by map in play. The
census reads the World of Xeen CD data at a GOG root (Clouds and Darkside,
reported separately) and changes no gameplay, save format or timing.

## Tasks

1. **Census tool.** A developer tool (a new `--inspect-*` mode or a small
   separate target, implementer's choice) that loads every map, monster, item
   and spell resource and counts, for each mechanic: how many times the game
   uses it, on which maps, and its support status. Support status comes from
   the engine's own decoders and support checks wherever they exist, never from
   a second hand-written list. Where support depends on runtime context, report
   it as "partial" with the reason the engine gives.
2. **Dimensions.**
   - Event scripts: every instruction by opcode and by the operand that changes
     its meaning (TakeOrGive and If modes, NPC and Display modes, town service
     actions, teleports, spawns and similar).
   - Monsters: attack types, special attacks, spell casting and other abilities,
     with the maps whose encounter tables use each monster.
   - Spells: all of them, with who uses each (party, monsters, items, Events).
   - Items: Misc powers and enchantments, with where they can be obtained when
     the data says so.
   - Map features: terrain and surface effects, wall items and objects with
     behavior, and town services per town.
   - Screens: the original dialogs and buttons, as a checklist taken from the
     pinned ScummVM dialog set, since they are not enumerable from the data.
3. **Per-map readiness.** For each map, the number of unsupported and partial
   mechanics it uses, so "which areas would already play correctly" becomes a
   list.
4. **Report.** A generated `docs/coverage.md` with totals first ("N of M event
   operations supported"), then each dimension sorted by how many places an
   unsupported mechanic blocks. Register it in `docs/AGENTS.md`; it is
   regenerated whenever support changes.
5. **Guard test.** A CTest that regenerates the census from the original data
   and fails when `docs/coverage.md` is stale, so every milestone that adds
   support updates the numbers in the same change.

## Boundaries

- Read-only over original data; nothing original is copied into the
  repository beyond counts, identifiers and names already shown in game text.
- If exposing an engine support check needs a production refactor, keep
  behavior identical and run the complete `build-rel` suite; the M44 audit must
  show zero differences.
- No implementation of missing mechanics in this milestone.

## Acceptance

- Tests for the census on small synthetic inputs plus the original-data guard.
- The maintainer reviews `docs/coverage.md` and the per-map readiness list.
- Closure proposes the next arc in [the roadmap](roadmap.md): systems ordered
  by the census (spells, combat Equip/Use, missing screens and the rest), with
  the test-suite cleanup placed where the maintainer chooses.
