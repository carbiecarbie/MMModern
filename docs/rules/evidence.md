# Evidence and unresolved questions

This chapter records what supports the [rules reference](README.md), how to
revisit the original-program findings, and where the evidence stops. It is a
research reference, not milestone acceptance or authorization to change code.

## Exact baseline

| Artifact | Identity |
| --- | --- |
| MMModern | `9cc4def8c5c0253899c094efbe5b4d7e4513b22c`, branch `m61-part-b`; inspected 2026-10-12 |
| Pinned ScummVM | `6814ee9ba54582f5b5adcffab49efbbd8f589edd`; separate checkout verified at that revision |
| Original | Installed English World of Xeen CD `WORLD/XEEN.DAT`, 353,511 bytes |
| Original SHA-256 | `8bf78b4850d5973cbb063c7dd96b9aaef4e94791d748334187eef74acf528d27` |
| Original local path | `F:\Games\gog\Might and Magic 4-5\WORLD\XEEN.DAT` |
| Analysis | Read-only original input, REA with Ghidra 12.1.4; bounded 16-bit instruction checks using Capstone |

The MZ header occupies `0x56c0` bytes; the load module has 331,303 bytes.
For an unrelocated module segment and offset, the file position is
`0x56c0 + segment * 16 + offset`. The REA import uses load segment `0x1000`,
so its linear address for a module byte is `file offset + 0xa940`.
The original data segment is `0x481d`, whose **file base is `0x4d890`**.
Do not confuse its module-relative base with a file offset.

All hexadecimal locations below are **original file offsets** unless explicitly
described as character fields or data-segment offsets. Ranges written `[a,b)`
include `a` and exclude `b`. These locations identify this exact executable;
other releases need their own hash, relocation and calibration.

## Method and limits

The analysis first established the load image and address mapping, then followed
dispatch entries, callers, arguments, conditional branches and writes. Known
behavior supplied calibration: First Aid's 6 HP, its global ID 26 and 1 SP cost;
previously established Suppress Poison refunds; retained item-use observations;
and the separate item-category layouts. A plausible function name alone was
never treated as proof.

Ghidra's inferred item-dispatch function body includes unrelated, noncontiguous
regions. Its aggregate decompilation is unsuitable evidence for that dispatcher.
The conclusions below instead use the bounded original bytes and 16-bit
control flow, checked against the verified image. Branches must be followed:
linear decoding past a return/jump can falsely attribute a neighboring handler
to an item power. Static agreement establishes the stated code path, not that
every possible surrounding game state has been observed in DOSBox.

The cost comparison covers every entry, rather than a random sample. The item
mapping comparison covers all 73 dispatch entries, but semantic destinations
do not prove all downstream effects, refunds, sound, animation or combat
admission identical. The chapters preserve these boundaries explicitly.

Original bytes, disassembly, decompiler output and analysis projects remain
outside the repository. Local evidence is retained under
`D:\MModern-re\rules-20261012\`: `rea-evidence.json`, `query.py`,
`spell-cost-comparison.txt`, `item-mapping.txt`, `monster-loot.txt` and bounded
instruction slices. `query.py` checks the exact original hash before reading.
These local paths describe provenance, not dependencies required to build
MMModern. Reproducing the original checks requires the owner's game installation.

Earlier evidence is retained under `D:\MModern-re\20261011-rea-ghidra\` and
the external `D:\MModern-re\ghidra-xeen-dat\WorldXeenDat.gpr` project.
Its authoritative summaries and calibration are in
[original quirks](../original-quirks.md) and
[M61 price evidence](../milestone-61-plan.md#original-program-price-evidence-and-confidence).

## Spell and item control flow

The following findings are **DOS static, high confidence**, confined to the
stated paths. They were independently checked against the instruction slices
and pinned source. Existing implementation tests are not substitutes for these
original-program checks.

| Finding | Original anchors | Calibration and boundary |
| --- | --- | --- |
| All 76 SP costs and all 76 gem costs agree with the pin | Signed-word SP table `[0x4f752,0x4f7ea)`; gem-byte table `[0x4f7ec,0x4f838)`; cost lookup `0x2f2de..0x2f2e9`, refund lookup `0x2f353..0x2f35f` | Comparison against `SPELL_COSTS` and `SPELL_GEM_COST` has zero differing entries. The two-byte gap is not a gem entry. This establishes the tables, not every Guild or cost-overflow rule. |
| Resurrection cancellation refunds its own cost | Global dispatch 58 at `0x33598` reaches `0x30bee`; target call supplies 58 at `0x30bf3`; cancel return `0x30bff..0x30c01` | First Aid supplies 26 and Suppress Poison supplies 68 to the same prompt/refund convention. DOS refunds 125 SP/20 gems; the pin supplies Raise Dead 56 and refunds 50 SP/10 gems. Resurrection's recovery effect remains a separate unverified question. |
| Nature's Cure heals 25 HP | Dispatch 49 at `0x33550` reaches `0x2fb9e`; target ID at `0x2fba3`; terminal refusals `0x2fbe0..0x2fbef`; heal argument 25 at `0x2fbf9`, helper `0x2ee3a` | First Aid passes 6 at `0x2f450` to the same helper. No separate condition cure exists in the traced handler; positive-HP Unconscious clearing belongs to healing. |
| Normal Item Use spends before dispatch, without learned-cast resource debit | Entry `0x313b7`, admission `0x313ce`; decrement `0x3145c`; caster pointer assignment `0x3146d..0x31470`; dispatch call `0x3149e`; exhausted removal `0x314a4..0x314c6`; monster response `0x314ce` | Retained DOS play establishes no time advance, failed Jump spending and last-charge removal. The item caller has no SP/gem debit. Combat restrictions and individual effect failures remain separate checks. |
| Escaping a First Aid item's target prompt grants the user 1 SP | Item dispatcher `0x39e4b`; power-4 branch `0x39e79` reaches First Aid `0x2f3f5`; ID 26 at `0x2f3fa`; prompt `0x2f0ba`; Escape branch `0x2f146`; refund `0x2f34b`, SP addition `0x2f386` | The caller set this user as caster and did not debit SP. Refund adds to caster field `0x158`, with no maximum-SP clamp or item-origin test; party gem addition at `0x2f38f..0x2f393` is zero for this spell. Charge and normal post-effect settlement remain. This does not establish other powers' refund IDs. |

**Implemented (M61 Part C):** First Aid item Escape grants the user 1 SP
without a maximum-SP clamp, retaining the spent charge. No other item power
gains this refund. Both this path and Misc staging were rechecked read-only
against the persistent Ghidra project and the same original SHA-256; bounded
instructions and query are retained in `D:\MModern-re\m61c-recheck\`.

### Item-power dispatch and variants

The 73-entry table `[0x3a114,0x3a1a6)` is indexed by `power - 1` from
dispatcher `0x39e4b`; its code-segment file base is `0x39ca0`.
The [power lookup](magic.md#item-power-lookup) agrees with the semantic targets
of all entries. Four routes illustrate why that does not mean byte-for-byte
equivalence with a learned spell:

- Power 21 reaches Wizard Eye at `0x2f8df`, inside the learned route.
- Power 33 reaches the special Frostbite handler at `0x2fc77`.
- Power 64 reaches item Incinerate `0x30b58`; learned Incinerate reaches
  `0x30f0d`. Both prepare 250 Fire damage against a single target, but the
  item supplies sound selector 13/projectile selector 2 and the learned route
  supplies 22/1. Their visible and audible presentation requires resource/play
  calibration; selector identity alone does not name an animation or sound.
- Power 55's combat branch at `0x3a058..0x3a075` calls the prohibited-use notice
  and then reaches Town Portal's handler. The pin returns after the notice.
  **Unknown:** whether the downstream DOS handler ultimately allows any travel
  in combat. This observation must not be rewritten as permission to teleport.

The pin's shared Incinerate handler therefore does not establish identical item
presentation. Its Town Portal control flow is also not a complete account of
the original item route. Full non-First-Aid cancellation and travel outcomes
remain open.

## Monster treasure staging

**DOS static, high confidence:** the original retains generated Misc monster
treasure in its own category. The pinned `Combat::attack2` Misc case instead
copies the temporary character's accessory record into staged accessories.
**Implemented (M61 Part C):** MMModern stages Misc in its own ten-slot array
and delivers it with the existing shared rules, preserving the generation draws
and their order. Pending Misc is retained in save envelope v12 (schema 9/content
14). Full-pack and recipient fallback rules are unchanged.

The original attack/death routine starts at `0x2bdf8`. Its generation-result
dispatch is at `0x2c3f1..0x2c401`. The table at `0x2c592` uses code-segment
file base `0x2bd30` and selects the following branches and data destinations:

| Generated category | Branch file offset | Treasure base, relative to DS |
| --- | --- | --- |
| 0: weapon | `0x2c402` | `0x99e8` |
| 1: armor | `0x2c457` | `0x99c0` |
| 2: accessory | `0x2c4a6` | `0x9998` |
| 3: Misc | `0x2c4f5` | `0x9970` |

The Misc branch scans ten four-byte slots, testing the container field for
emptiness. It reads generated character fields `0x113`, `0x112` and `0x114`
at file `0x2c509`, `0x2c51a` and `0x2c52b`, then writes power, container and
state to DS-relative `0x9971`, `0x9970` and `0x9972` at file `0x2c513`,
`0x2c524` and `0x2c535`. These are distinct from accessory staging.

The generator independently identifies this layout: category 3 is selected at
`0x25e38`; it writes the Misc container at `0x25f68`, power at `0x26370` and
state at `0x26378`, then returns the category at `0x2637d`.
This caller/generator agreement, together with the item-use layout, calibrates
the meaning of the fields and excludes an accessory/Misc labeling mistake.

The conclusion is deliberately about **generation and staging**. It does not
prove full-pack delivery, recipient eligibility, overflow handling or every
monster's resource-defined drop chance in DOS. Those need separate traces.

## REA record locators

The exported evidence bundle contains complete records. The stable IDs below
locate the relevant image, bounded reads and routine inspections; they are not
replacements for the control-flow interpretation above.

| Record | Evidence ID |
| --- | --- |
| Verified load image | `ev_ef399a290c4b7c75504981f121b45825eb0386c1b063506e371c1e9a54f5d8ff` |
| Resurrection instructions | `ev_be37f34557e6137b01871640d0436763d6b5b7534c3832938210451d6252c7a4` |
| Nature's Cure instructions | `ev_aae374dd20efa81db12498f505e7590bf2c03a1a88e5e4216bbc880508002567` |
| Refund instructions | `ev_197465b8f6dafd646483c20094ac5dbcddca29a7b04a89d890f1a1ef158bbc51` |
| Item-use caller bytes | `ev_f31b8ab9b94c617efcb72ddd337262a8b561f02371bf50cfaa90a689a5083645` |
| Item dispatch and map bytes | `ev_cdca8f91f7fa96ee3478931b5f8f1cf43ea6a2bfb85cb3cbd70a44d81514a682` |
| Target-dialog bytes | `ev_1230c42cf3898996c5c6df763e2dc44f14ea902cc88d79018ec25f89d2fc2f79` |
| Healing-helper bytes | `ev_2d8022d539f88b97d38fb1ede3ed1462128f5a0c95ea192d41f0f6ff9ac7a63e` |
| Complete cost-table bytes | `ev_4e599ced8b32116289df9e5171b5833a20d3fd222e759b8dd2cdfb19b272ccc5` |
| Treasure-category branch bytes | `ev_05335aaa0db1cc046e56d244169faf096e294362721b458eb532fda188afe7ad` |

## Highest-value remaining questions

| Area | Question to settle before extending a rule |
| --- | --- |
| Combat | Does DOS natural 20 apply damage before the ordinary hit check, with armor breakage visible to that check? Which broken weapons still contribute base damage or effectiveness? |
| Damage and targeting | Confirm the hatred-1 exception, ranged continuation, elemental multi-attack arithmetic, strict Run boundary and monster recovery boundary in DOS. |
| Items | Trace all other item-prompt refunds; Recharge, Enchant, Item to Gold, cursed refusals and high-tier generation. |
| Treasure | Trace full-pack delivery, currency loss and recipient fallback independently of the now-settled Misc staging branch. |
| Magic | Establish Resurrection's actual recovery, Holy Bonus's combat contribution, control-spell duration, Poison Volley arithmetic and high-tier overflow. |
| Travel | Settle Town Portal item behavior after its combat notice; complete Beacon, Teleport, Etherealize and terrain-arrival paths. |
| Time | Verify multi-day economy/latch behavior, script-mode dawn suppression and uncalibrated condition-byte edges. Current MMModern wrapping/saturation is not literal equivalence with the pin's live integer counters; see the time chapter. |
| Broader coverage | Event opcode catalogue, all services, AI/movement, character creation, Darkside/World transitions, audio and frame-level UI timing still need dedicated chapters and calibration. |

These are evidence gaps, not instructions to broaden an active milestone.
Normal DOS play or calibrated static analysis should settle reachable disputes;
only the condition-counter fallback explicitly authorized in
[AGENTS.md](../../AGENTS.md) bypasses an otherwise unverifiable case.
