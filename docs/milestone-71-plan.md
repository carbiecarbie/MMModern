# Milestone 71 - Original item generation

**Tier A. Status: completed and accepted by the maintainer (2026-10-16)**
after independent plan and implementation reviews and a maintainer play-test.
Plan `697385f`; implementation in this closure's parent commit. Baseline
`e035c5c`. Grew out of a makeItem maintenance task whose new random sequence
broke stock- and combat-dependent tests.

## Scope

Every supported item source follows the original makeItem draws through one
generator: shops (initial stock and restock), Event Give/GiveMulti mode 66,
display cases and crates, and level-1 monster drops. Tests broken by the new
random sequence are repaired without weakening them or growing the suite.

Out of scope, moved to roadmap step 9 (monster treasure, changes the save):
drop levels 2 to 6, drops for every monster, and accessory staging.

## Results

- **makeItem, as DOS** (`WORLD/XEEN.DAT` `0x25ca8..0x26382`, helper
  `0x45b9a` inclusive min..max): category `R(1,100)`; subcategory `R(1,100)`,
  or `R(1,80)` at levels 6-7; level 7 draws its material selector before the
  forced row; weapon effectiveness `R(1,20)==10`; Misc charges `R(1,8)+2`
  (3..10). The pinned ScummVM draws from 0 and stores 1..8 charges.
- **Monster drops**: DOS passes the record's drop field (`+0x2d`) to makeItem
  after the chance draw (`0x2c36c..0x2c3ec`). The duplicated level-1 generator
  in `XeenMonsterDropCandidate` is gone; chance, accounting, staging and
  delivery are unchanged.
- **Saves**: v13 / schema 9 / content 14 unchanged. Stored stock and pending
  Misc accept charges 1..10, so v0.2.0 saves load and re-save exactly; only
  new generation follows 3..10.
- **Tests**: offers selected by property, combat witnesses re-routed through
  ordinary play, generation checked against an independent oracle over all
  legal category/subcategory pairs at levels 1..7, each rewritten assertion
  mapped to a mutation it catches; still 223 CTest targets.

## Validation

- M44: all four scenarios compared; every changed field explained (stock from
  the corrected draws, then ordinary combat and route changes in Services and
  Temple that keep their paid Buy, Fix, Training, Heal and resurrection). The
  maintainer approved the four new digests.
- Complete `build-rel` suite through the runner outside the Codex sandbox,
  with the failed-only reruns the protocol allows; fast suite 185/185.
- Independent implementation review: APPROVE. It found that the M60 spell-kill
  test never reaches item generation (comment corrected; a gameplay test of a
  successful drop is still missing) and that Temple Heal pricing differs from
  DOS, a pre-existing gap left for its own maintenance task.
- Maintainer play-test: new-game shop stock, Misc charges and a v0.2.0 save.
