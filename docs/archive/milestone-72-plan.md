# Milestone 72 - Terrain and simple services

**Tier A. Status: completed and accepted by the maintainer (2026-10-17)**
after independent plan and implementation reviews and a maintainer
play-test. Plan `e21f398`, amended twice by the maintainer; implementation in
this closure's parent commit. Step 5 of the
[systems arc](../roadmap.md#next-arc-finishing-the-games-systems).

## Scope

Rest and arrival on every Clouds surface as the original allows, and Drink,
Tip, Rumors, Donation and Uncurse in the Vertigo Tavern and Temple. Planned as
Tier B; the maintainer reclassified it as Tier A (2026-10-17) when Tip turned
out to change saved per-map state.

Out of scope: Sign In, which opens the original inn party manager (moved to
roadmap step 8); the entrance scenes (`DoTownEvent` 8 to 12, step 10); new
areas; audio.

## Results

Evidence: calibrated static analysis of `WORLD/XEEN.DAT` (offsets in
[original quirks](../original-quirks.md) and the rules chapters), pinned ScummVM
`6814ee9b` where DOS was not checked.

- **Surfaces.** One arrival owner (`0x0c924..0x0ca5f`) called from every
  original site: steps, turns and waits, Bash pass-through, ordinary Space,
  successful casts and Shoot (Jump included), and completed Rest. Lava deals
  100 fire damage to each member with the original saving draws; desert adds
  170 minutes outdoors unless someone has Navigator; sky and cloud make the
  party fall to the original destination table and take 100 physical damage
  (Levitate protects only on cloud); space kills the party. Blocked or
  cancelled actions and item Use do not apply arrival.
- **Rest** has no surface refusal in the original: it runs on every surface
  and applies arrival after recovery (Rest clears Levitate, so Rest on cloud
  falls).
- **Space scheduling** (amendment, 2026-10-17): a completed manual Event or an
  Event-free Space charges 1 minute indoors or 10 outdoors, gives the monsters
  their opportunity, then applies arrival (`0x0d468..0x0d482`); terminal
  services, handled doors and grates, and blocked Bash skip it. MMModern had
  omitted this since before M44.
- **Tavern.** Drink (1 gold, 25% chance of Drunk), Tip (needs a drink this
  visit; cycles 15 Clouds tips with DOS modulo, where the pin divides),
  Rumors (free, one per day).
- **Temple.** Donation (counter per visit; the matching day grants
  Clairvoyance, light and four bonuses), Uncurse (20 x level plus surcharge;
  clears the Cursed condition and the curse bit of every carried item), and
  one farewell day per paid Heal or Uncurse plus one on leaving, as in DOS.
- **Save v14** (schema 9, content 14): the per-map Tavern tip index joins the
  generic map state; saves from v13 and older are rejected.

## Validation

- Census: no unsupported Rest-surface row in Clouds; five Services rows
  supported, Sign In deferred; arrival rows classified.
- M44: all four digests replaced with the maintainer's approval; every field
  explained (time from Space scheduling and farewell days, one extra monster
  opportunity on the mainland route, the new tip-state byte).
- Complete `build-rel` suite through the runner, with test-only reruns;
  fast suite 185/185.
- Independent reviews: plan (with both amendments) and implementation inside
  the implementing session, then an external review of Space scheduling and
  save v14: APPROVE, no findings.
- Maintainer play-test: Tavern and Temple services with save and load, lava
  and Rest on lava through the developer fixture, Space passing time.
