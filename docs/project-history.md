# MMModern - Project History

One short paragraph per completed milestone, with the closing commit. This is
not the authority for current state: see [project status](project-status.md) and
the [roadmap](roadmap.md). Plans of milestones 15-43 are kept in
[docs/archive](archive/) as a historical record; their limits and entry modes
describe their time, not the present.

**M1-M12 - Pre-public foundation.** Original-resource loading, outdoor and indoor
rendering, navigation, collision, party loading and the first Event support.
Surviving history does not assign these to individual milestones.

**M13 - Public baseline** (`c12d04d`). First public codebase: Event decoding and
execution, automatic Events, conditions, teleports and game flags.

**M14 - Manual interaction and original text** (`275a0ca`). Space interaction,
original Event text, resumable dialogs and Yes/No, original fonts and windows,
and the SDL gameplay loop; Castle Basenji connected dialogue to teleport.

**M15 - Session-owned world mutations** (`742b109`). Stable object and Event
identity and Remove as session overlays that survive cache reconstruction.
[Plan](archive/milestone-15-plan.md).

**M16 - Outdoor objects and visual Remove** (`484c8ce`). Static outdoor objects
in the scene with immediate refresh when an object is removed.
[Plan](archive/milestone-16-plan.md).

**M17 - Quest items and Phirna** (`b59f39e`). Party-owned quest-item counters and
the original Phirna Root collection. [Plan](archive/milestone-17-plan.md).

**M18 - WhoWill and Bone Whistle** (`32e6879`). Character selection and the
Bone Whistle collection with cancel and retry. [Plan](archive/milestone-18-plan.md).

**M19 - NPC dialogue and Myra's request** (`59d3016`). Original NPC portraits and
paginated speech, quest flags and Myra's request. [Plan](archive/milestone-19-plan.md).

**M20 - Save and resume** (`a449bb7`). Versioned snapshots restored into the
existing owners, F9 on Windows and resume at startup, with resource checks.
[Plan](archive/milestone-20-plan.md).

**M21 - Myra's exchange and item rewards** (`bb524c0`). Full item records,
deterministic rewards and the complete Myra request, Phirna and return loop with
exact restart. [Plan](archive/milestone-21-plan.md).

**M22 - Outdoor object animation** (`7f8a7d5`). Ordinary outdoor objects animate
while stationary, with independent cosmetic timing. [Plan](archive/milestone-22-plan.md).

**M23 - Indoor objects** (`a6b0826`). Static indoor objects with original
directional art, wall occlusion and the Nightshadow gravestone interaction.
[Plan](archive/milestone-23-plan.md).

**M24 - Inventory and transfer** (`2c9ef4f`). Item catalog from original names,
inventory inspection and character-to-character transfer.
[Plan](archive/milestone-24-plan.md).

**M25 - Equipment** (`b78398b`). Equip and remove with class, conflict and
capacity rules and truthful feedback. [Plan](archive/milestone-25-plan.md).

**M26 - Actor approach** (`49c2adf`). Original outdoor monster identities, wake-up,
approach and engagement through the Flow and SDL path.
[Plan](archive/milestone-26-plan.md).

**M27 - Playable combat** (`f63bc3d`). Attack/Block combat with original
appearance, injury, armor breakage, XP and once-only defeat.
[Plan](archive/milestone-27-plan.md).

**M28 - Durable encounter completion** (`d867b50`). A finished encounter becomes
saveable and restores in a new process without replay.
[Plan](archive/milestone-28-plan.md).

**M29 - Mutable Journey** (`45efc11`). The first Journey: play continues after
combat on the same owners, with save and restart. [Plan](archive/milestone-29-plan.md).

**M30 - Expedition** (`dfacac7`). Grouped Skeleton and Zombie combat, Disease and
world-owned random continuation. [Plan](archive/milestone-30-plan.md).

**M31 - First connected slice** (`6faf45b`). Bone Whistle collection inside the
expedition with accumulated consequences saved across restart.
[Plan](archive/milestone-31-plan.md).

**M32 - Regional Journey** (`5e15e1d`). Map-23 mainland exploration with all 19
original actors and the automatic sign. [Plan](archive/milestone-32-plan.md).

**M33 - Mainland combat** (`ac13999`). Orc, Snake and Toad combat, enemy ranged
attacks, player Shoot, Poison, Sleep, XP, gold and generated equipment.
[Plan](archive/milestone-33-plan.md).

**M34 - Run and disengagement** (`b200171`). Per-member Run, casualties,
relocation to `(10,12)` and re-engagement with wounded survivors.
[Plan](archive/milestone-34-plan.md).

**M35 - Myra quest and recovery** (`58356a6`). The quest loop across mainland
travel, the selected well and the delivered antidote.
[Plan](archive/milestone-35-plan.md).

**M36 - Exploration casting** (`f8ba96e`). Learned spell books and First Aid and
Awaken in exploration. [Plan](archive/milestone-36-plan.md).

**M37 - Vertigo entry** (`891b2a0`). Entrance, first town cells, Slime combat and
the exit with the original monster reset. [Plan](archive/milestone-37-plan.md).

**M38 - Ironworks Armor Repair** (`79c31a8`). Repair for carried gold and the
one-day visit departure. [Plan](archive/milestone-38-plan.md).

**M39 - Combat casting** (`0760a6b`). Magic Arrow, First Aid and Awaken in combat.
[Plan](archive/milestone-39-plan.md).

**M40 - Service days** (`7d22ac0`). Repeated visits across days, merchant stock
generation, bank balances and interest. [Plan](archive/milestone-40-plan.md).

**M41 - Training** (`0c9877e`). Permanent levels from earned XP and gold, with
per-member service days. [Plan](archive/milestone-41-plan.md).

**M42 - Equipment purchase** (`b3b3679`). Buying plain Weapons and Armor from
generated stock with depletion. [Plan](archive/milestone-42-plan.md).

**M43 - Temple Heal and resurrection** (`c26a26b`). A 49-cell Vertigo route to the
Temple with original prices and a two-day paid departure.
[Plan](archive/milestone-43-plan.md).

**M44 - Simplification** (`830af7e`). One save format and one Journey
configuration; legacy contracts, save versions and diagnostic modes removed;
tests labelled `fast`/`process` with digest-checked original-data scenarios;
closed plans archived and docs rewritten. Game behavior unchanged.
[Plan](milestone-44-plan.md).

**M45 - Reliable input and play stability**. Buffered keyboard input in
exploration and combat (up to five presses, applied when the game is ready,
flushed on context changes), replacing silent dropping; integrity guards
admit maps and object files loaded on demand, ending "Stale encounter frame"
crashes near map edges. [Plan](milestone-45-plan.md).

**M46 - Clickable main screen** (`646ea1e`). Left clicks on the original
main-screen buttons, combat buttons, targets and 3D view drive the existing
actions through the M45 input path; unimplemented buttons say so.
[Plan](milestone-46-plan.md).

**M47 - Original dialogs** (`02214e1` and the Part B commit). The original
character sheet and items dialog replace the project inventory, and the
Ironworks, Training and Temple use the original location screens and
dialogs, by mouse or key; dialog text comes from a pinned build-time
generator. [Plan](milestone-47-plan.md).

**M48 - Generic presentation systems** (`6387a0e`, `b0e4919`, `4d953c6`).
Terrain, sky, water, wall items and monster animation from original data on
every Clouds map; simultaneous missiles, splats, portrait damage/healing
effects, Magic Arrow travel, the combat strip, original keys and cursor.
Presentation only; combat impact timing and rotation moved to M49.
[Plan](milestone-48-plan.md).

**M49 - Combat rules fidelity** (`804fc5a`). Monsters choose targets and attack counts
from their original data for every record (the Slime now hits two members,
not the whole party); missile damage, death and rewards apply on arrival;
the party can turn during combat. Two M44 scenarios were re-routed off the
old defect and all three digests regenerated with every changed byte
explained. [Plan](milestone-49-plan.md).

**M50 - Whole Vertigo** (`2cabd9a`, `ff91d0a`, `73739e7`). The whole town is
loaded from the original resources: certification and actor freezes removed,
all 46 monsters active, Events dispatched generically through guarded
publication, original Bash and grate/door unlocking with typed trap damage
and Thievery, indoor Shoot and enemy ranged attacks; save v5.
[Plan](milestone-50-plan.md).

**M51 - Rest, food and daily time** (`b8856c1`, `55633c1`, `cffb2c3`).
Original Rest from the button or R with food, recovery, the dream and monster
interruption; shared daily time with the original eight-hour condition
schedule, dawn, night sky and year rollover replaced the old calendar limits;
food read from the correct offset; save v6. [Plan](milestone-51-plan.md).

**M52 - Normal start in Vertigo** (`08178fe`, `a830248`). A new game starts
from the original initialization: the six level-1 characters in Vertigo at
`(18,4)`, day 1, 800 gold and 90 food, with all town actors in their original
state. Adventurer and Warrior both work. The command line temporarily replaces
the original title menu and difficulty dialog; the prepared Journey remains a
test mode. [Plan](milestone-52-plan.md).

**M53 - CD edition data** (`69b82d0`, `2bbe975`, `37101c4`). The World of
Xeen CD talkie edition became the only reference data: archives read from the
GOG disc images or a CD copy, DOS interface text from `WORLD/XEEN.DAT`, floppy
fixed checks replaced by structural ones, and CD speech handled as deferred
audio. All accepted behavior was revalidated: the three M44 routes play
identically on CD data. [Plan](milestone-53-plan.md).

**M54 - Original title menu and saves** (`4ab4762`, `11e8c81`, `a760e69`).
The game opens on the original CD title screen with Credits and Other Options;
New picks a slot, name and difficulty; the in-game control panel saves, saves
as, loads and quits as in DOS, with ten slots under Local AppData and the
original pulsing text cursor. Save v7 adds the slot name. M52's temporary
command-line entry is retired. [Plan](milestone-54-plan.md).

**M55 - Interface gaps** (`9d51e42`, `9e4df22`, `ef7ec84`). The party can
strafe, Quick Reference shows the party summary and Info the date, time and
active effects, Exchange swaps two members and combat, Shoot, Rest, casting,
barriers and treasure follow the new order, and Quick Fight runs each
member's configured action with the original Options dialog, all by mouse and
keyboard. Save v8 stores each character's quick action and remembered spell.
[Plan](milestone-55-plan.md).

**M56 - Food and bank** (`cf4c9a9`). The Tavern refills party food for the
original flat price, with full packs free; the Bank deposits and withdraws
gold or gems through the original screens and ten-digit amount entry.
Supported Event preludes run and publish once before terminal town services;
one-day departures apply scheduled interest to final balances without
rerolling stock. Save v8 and all M44 digests are unchanged. Maintainer play-test
accepted. [Plan](milestone-56-plan.md).

**M57 - Generic Event effects** (`94007d0` and `d1f5459`). Events
can move and turn objects, award XP, awards, flags, gold and gems, and give
treasure through the same item generation and random draws as merchants. Joe's
proof and Gunther's reward work from the original scripts, searched crates
open and each tree pays once; the well refuses safely until it is implemented.
Save v9 stores all characters' awards and moved objects. Two display bugs
found in play-testing were fixed. Maintainer play-test accepted.
[Plan](milestone-57-plan.md).

**M58 - Skills and mapping** (`a39ec95`). Each character has one skill state
from the original data, used by the sheet and the rules. Mylo and Rialdo teach
Cartography and Pathfinder through the original scripts, including Rialdo's
lower actual price and paid repeat lessons. Cartography records explored cells
for the original automap and Map. Save v10 stores skills and exploration; a
reusable digest audit tool now checks save-format changes.
[Plan](milestone-58-plan.md).

**M59 - Vertigo content** (Part A `384eb13`, `5166cab`; Part B `4d39d79`). Display cases use
Thievery for treasure or a one-year jail penalty with ordinary walk-out; the
well damages or heals only the selected member. Generic city searches preserve
crate/bed appearance, actor identity and repeats. Vern charges membership per
character, and Guild entry uses that character's membership with original
art/text and deferred spell actions. Skill, award and XP grants show the
original portrait sparkle. Save v11 stores generic world flags and head-cache
overrides; Part A's explained M44 digest update was approved, and Part B and the
sparkle correction left the digests unchanged. Both parts and the corrected
feedback were accepted after maintainer play-testing on 2026-10-11.
[Plan](milestone-59-plan.md).

**M60 - Magic** (`e7e7a62`, `6bc4779`). The original Cast dialog and New list
replace the project screens, and the Guild sells and describes Vertigo's 18
spells to members at the original prices. Eight new spells work through shared
projectile and casting rules: Wizard Eye, Shrapmetal, Energy Blast, Jump, Cure
Wounds, Flying Fist, Revitalize and Pain. Original program analysis settled
spell resistance and Pain. Jump refuses unimplemented terrain hazards. Save v11
unchanged. [Plan](milestone-60-plan.md).

**M61 - Items for the loot** (`5e0542c`, `9cc4def`, `dc056ae`, `cbebb10`). The
Ironworks buys, sells, identifies and fixes every item category at prices taken
from the original program, and charged items cast their supported effects from
the character sheet without spending time. Analysis of the original program
corrected the reference: monsters' miscellaneous drops are delivered, full packs
keep gold and gems, and treasure delivery follows the original recipient and
replacement rules. A game rules reference (`docs/rules`) records the evidence.
Save v12 stores pending miscellaneous treasure. [Plan](milestone-61-plan.md).

**M62 - Faithful screens and animation** (`5cc0079`, `41237e1`, `27f344a`). The
developer overlay and log-style combat text left normal play. Event, NPC,
WhoWill, combat-panel and treasure screens use the original windows, text and
input, the six shopkeepers animate, scenery animations include their final
frame, and the title and credits follow DOS. Timing-dependent parts are left
for M63. [Plan](milestone-62-plan.md).

**M63 - Original presentation clock and movement pace** (`4f20b52`,
`5c40ffe`). One clock in original BIOS ticks drives every wait, and the party
walks about nine steps per second as in DOS, keeping every tap but adding no
step after a held key is released (approved deviation). NPC speech, shop
voices, treasure, sparkles, Shoot, the dream, the title and party death follow
the original sequences and timing; death returns to the title instead of
DEFEAT. Myra's original speaking draws changed the Mainland M44 digest
(approved). [Plan](milestone-63-plan.md).

**M64 - Vertigo closure and first public release** (`c080eb1`, `080b05f`,
`5545471`). The maintainer's full play-through fixes: one sparkle per member
per Event run, as in DOS, original item names in equip refusals and a plain
window title. The first public releases followed: v0.1.0, a Windows zip with
a player README and the 0.x save policy, and v0.1.1, whose launcher finds and
remembers the GOG folder. [Plan](milestone-64-plan.md).

**M65 - Coverage census** (`1248d83`). A read-only tool counts every mechanic
the World of Xeen data uses and asks the engine whether it is supported, with
a real game context; the generated coverage report lists totals, the largest
gaps and per-map readiness, and a calibration test pins Vertigo to its known
gaps. It found a real gap: equipped material or enchanted items are refused
before combat and Rest. [Plan](milestone-65-plan.md).
