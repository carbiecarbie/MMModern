# Milestone 23 - Static indoor objects and an original readable gravestone

**Milestone 23 is complete.** It delivered bounded static ordinary Clouds indoor
objects through the existing indoor composition and certified the original
Nightshadow gravestone interaction. This closed plan records the final rendering,
ownership, persistence and [acceptance](#final-acceptance) contracts. Current
capabilities are summarized in [project status](project-status.md).

## Goal and boundary

M23 renders static ordinary Clouds objects in indoor scenes using original
directional appearances, twelve placement positions, exact wall predicates and
ordered rasterization. Its original-data checkpoint is the gravestone at
**Clouds map 29 (Nightshadow), `(4,6)`, facing West**, identity
`{Clouds,29,14}`. The existing event system displays its original clue and
acknowledgment; M23 supplies the previously missing visual object.

This is an indoor rendering foundation and one bounded visible interaction. It
does not certify Nightshadow gameplay, a normal route, a playable town or dungeon,
connected-map indoor rendering, or the nighttime/combat behavior mentioned by
the clue. Support is table-driven and is not hard-coded to the checkpoint.

## Selected original checkpoint

The selected encounter was verified against a legally obtained World of Xeen
installation. Commercial resources remain external and unmodified.

| Field | Durable contract |
| --- | --- |
| Map | Clouds 29, Nightshadow; `maze0029.dat`, indoor town geometry |
| Object source | Initial Clouds `maze0029.mob` ordinary-object list |
| Original record / identity | Zero-based record 14 / `{Clouds,29,14}` |
| Position / object direction | `(4,6)` / West (3) |
| Table slot / resolved resource | 3 / 14; the slot is not a metadata index |
| Visual | `XEEN.CC/014.obj`, two directory frames; static RIP gravestone |
| Metadata | `DARK.CC/clouds.dat` entry 14 |
| Relative initial frames / limits / flips | `[0,1,0,1]` / `[0,1,0,1]` / `[0,0,0,1]` |
| Primary projection | Query 2, order 149, anchor `(-5,2)`, scale 0, scene and bottom clipping |
| Primary cell | Raw wall word `0x8088`; no automatic trigger |
| Text | `XEEN.CC/aaze0029.txt`, zero-based entry 6, including its leading line feed |
| Behavior | Original bottom-window clue, then acknowledgment |
| State effects | None: no removal, item, flag, party, camera or time mutation |

Camera N/E/S/W resolves frame/flip to **1/false, 0/false, 1/true,
0/false**. These are directional static appearances, not animation. Static
classification retains the existing `initial + 1 >= limit` rule, including a
zero limit.

The complete original event at the cell is:

| Original EVT record | Byte offset | Direction | Line | Opcode and operands |
| ---: | ---: | --- | ---: | --- |
| 65 | 583 | All (4) | 0 | `0x29 DisplayBottom`, text index 6 |
| 66 | 590 | All (4) | 1 | `0x09 If2`, Action 44, value 1, target line 2 |

No line 2 exists there. The existing adjacent-acknowledgment natural-completion
rule completes the event after two instructions. The original event accepts all
four facings; West is the useful front-view checkpoint, not a new facing rule.
Adjacent cells do not dispatch this event, and the sibling gravestone at `(4,10)`
has a distinct clue.

The target is an **ordinary object**, even though it stands against a wall.
`parseMob` keeps wall items in a separate list, and the original wall-item path
uses `.pic` resources, exact-facing admission and different placements and frames.
Reclassifying the gravestone would corrupt those boundaries. Wall items therefore
remain excluded.

Rejected encounter alternatives needed unsupported script effects such as
AfterEvent or MakeNothingHere, or added unnecessary NPC complexity. Their short
lesson is durable: M23 certifies a complete supported interaction, not only a
convenient prefix of an unsupported script.

## Architecture and ownership

M23 extends existing owners rather than creating parallel indoor systems:

- `XeenMapFormat`, `XeenMapLoader` and `XeenWorld::objectFile` retain immutable
  DAT/MOB data, coordinates, directions, table slots and original record indices.
- `XeenWorld` and `XeenSessionWorldState` remain authoritative for stable
  side/map/record identities and disabled object/event overlays.
- `XeenIndoorScene` shares its existing 44 wall samples and bit translation
  between geometry and ordinary-object admission, emitting typed geometry/object
  commands into one ordered stream.
- `XeenObjectVisualResolver` supplies the existing resource, directional frame,
  flip, static/animated classification and identity-bearing diagnostics.
- `CloudsMapComposer` draws the merged indoor stream. Object commands use
  `XeenAssetSource::drawObjectVisual` and the existing checked Xeen sprite path;
  there is no second decoder or projection renderer.
- `XeenEventSystem`, `XeenEventFlow` and `XeenEventPresenter` retain interaction,
  continuation, blocking, recomposition and presentation ownership. M23 adds no
  event command, input loop or presentation mode.
- `XeenSaveState` and the existing save format retain persistence ownership.
  Indoor commands, wall samples, projection choices, sprite caches and pixels
  are derived values.

Geometry-only construction remains available and reproduces the accepted wall
commands. Indoor object construction uses the current-map ordinary records from
the accepted 16x16 domain. Out-of-domain records are preserved but neither
wrapped, clamped, deleted nor drawn; supporting neighbor geometry would be a
separate capability.

## Projection and occlusion contract

### Position, anchor and scale

For camera direction `d`, an ordinary record matches a placement query when:

```text
object.x = camera.x + kScreenPositioningX[d][query]
object.y = camera.y + kScreenPositioningY[d][query]
```

Map Y increases north. F is forward distance and L/R is lateral distance from
the camera; query 2 is the camera's own cell, not one cell forward.

| Query | Relative cell | Draw order | Normal anchor X,Y | Resource 113 anchor X,Y | Scale index |
| ---: | --- | ---: | --- | --- | ---: |
| 2 | 0F | 149 | -5,2 | -35,-65 | 0 |
| 7 | 1F | 125 | -7,25 | -35,-6 | 7 |
| 5 | 1F1L | 126 | -112,25 | -142,-6 | 7 |
| 9 | 1F1R | 127 | 98,25 | 68,-6 | 7 |
| 14 | 2F | 97 | -8,50 | -35,36 | 12 |
| 12 | 2F1L | 98 | -65,50 | -95,36 | 12 |
| 16 | 2F1R | 99 | 49,50 | 19,36 | 12 |
| 27 | 3F | 55 | -9,58 | -35,54 | 14 |
| 25 | 3F1L | 56 | -34,58 | -62,54 | 14 |
| 29 | 3F1R | 57 | 16,58 | -14,54 | 14 |
| 23 | 3F2L | 58 | -58,58 | -98,54 | 14 |
| 31 | 3F2R | 59 | 40,58 | 16,54 | 14 |

The exceptional anchors are selected only by resolved **Clouds resource 113**,
never by MOB slot or original record index. Darkside resource 47 is outside this
scope. There is no ordinary-object row at four forward cells.

Anchors are checked-drawer inputs rather than final sprite top-left coordinates.
The existing rasterizer applies cell offsets, scaled centering, compressed
transparent spans and horizontal flip. Scale indices retain their original
16-bit masks: 0=`0xffff`, 7=`0xaeaa`, 12=`0x8888`, 14=`0x8080`.

Object direction selects relative metadata entry
`(cameraDirection + 4 - objectDirection) % 4`. A nonzero metadata flip performs
horizontal flip. Camera rotation does not add another placement flip, and
ordinary objects are not filtered by direction equality as wall items are.

### Wall predicates

Let `W[n]` be the boolean produced by the existing `kMazeBitIndex` translation
over the 44 sampled wall faces. Missing samples contribute no bits. Each
expression below means **blocked**; an in-domain placement is admitted only when
its expression is false.

| Query | Blocked expression |
| ---: | --- |
| 2 | false |
| 7 | W27 |
| 5 | (W27 & W25) \| (W27 & W28) \| (W23 & W25) \| (W23 & W28) |
| 9 | (W27 & W26) \| (W27 & W29) \| (W24 & W26) \| (W24 & W29) |
| 14 | W22 \| W27 |
| 12 | W27 \| (W22 & W23) \| (W22 & W20) \| (W23 & W17) \| (W20 & W17) |
| 16 | W27 \| (W22 & W24) \| (W22 & W21) \| (W24 & W19) \| (W21 & W19) |
| 27 | W27 \| W22 \| W15 |
| 25 | W27 \| W22 \| (W15 & W17) \| (W15 & W12) \| (W12 & W7) \| (W17 & W7) |
| 29 | W27 \| (W15 & W19) \| (W15 & W14) \| (W14 & W9) \| (W19 & W9) |
| 23 | W27 \| (W22 & W20) \| (W22 & W23) \| (W20 & W17) \| (W23 & W17) \| W12 \| W8 |
| 31 | W27 \| (W22 & W21) \| (W22 & W24) \| (W21 & W19) \| (W24 & W19) \| W14 \| W10 |

The query-29 asymmetry is deliberate: unlike query 25, it has no standalone W22
test. Nonzero wall nibbles are not interchangeable; the existing bit translation
determines which predicates they satisfy.

### Ordered rasterization and clipping

Admitted ordinary objects are merged with geometry in ascending original numeric
order. Objects at orders 55-59, 97-99 and 125-127 follow farther walls and precede
nearer walls. The same-cell order 149 follows near wall orders 143-147, so the
gravestone appears in front of its back wall. Later opaque wall spans overwrite
object pixels while transparent spans preserve them, allowing partial or complete
coverage without rectangle rejection or a second visibility system.

Every ordinary-object command enables scene clipping to the existing half-open
rectangle `[8,223) x [8,141)`. Query 2 also enables bottom clipping at 140; no
other placement does. Composition reconstructs a clean background first, so a
turn, removal, failed rebuild or cache discard cannot leave ghost pixels. Border,
indicator, party and interface overlays retain their later composition order.

## Static, identity and failure contracts

M23 supports **static directional ordinary appearances only**. Indoor resolution
omits the outdoor ordinary phase. Animated metadata remains an explicit
`UnsupportedAnimation` diagnostic and is not frozen at frame zero; indoor
composition reports no ordinary animation and starts no periodic redraw.

At each placement, the first eligible original ordinary record owns the position.
Base-disabled, unresolved, out-of-domain and session-disabled records are skipped
before visual resolution. Identity is side/map/original-record index; records are
never deduplicated by sprite, coordinate or visible slot. Disabling an earlier
eligible record allows the next eligible record to win.

Once a record owns a placement, an invalid direction, unavailable metadata or
unsupported animation retains that record's diagnostic and does not promote a
later record. Missing MOB means no ordinary objects. Malformed present MOB or
metadata and selected missing/malformed sprites retain existing checked failures.
Selected frames are validated on cache hits and misses. Failed composition does
not publish a partial frame or mutate gameplay owners.

Rendering never determines gameplay selection. An event can remain eligible when
its object is visually omitted; object-only disable and physical-cell event
disable remain distinct existing semantics.

## Interaction contract

At the Nightshadow checkpoint, production follows the existing path:

1. `XeenWorld` loads immutable DAT/MOB data and applies disabled overlays.
2. `XeenIndoorScene` resolves record 14 and emits its admitted object command;
   `CloudsMapComposer` draws it in the wall-ordered stream.
3. Space dispatches a manual interaction at physical camera `(29,4,6,West)`;
   the automatic-event bit is not required.
4. Existing selection chooses original record 14. The event presents text entry
   6 as a bottom-window message through Flow and Presenter.
5. Action 44 produces a pending acknowledgment. Gameplay remains blocked until
   Space or Enter responds, after which the missing adjacent target line ends
   execution naturally after two instructions.
6. The retained bottom message may remain until the next gameplay action clears
   and recomposes it. The gravestone remains visible, and repeating the interaction
   yields the same clue without accumulated mutation.

No click picking, visible-nearest selection, selected-object facing restriction,
new text, new opcode, read flag or presentation lifetime was introduced. Escape
retains its ordinary behavior and is not added as an acknowledgment key here.

## Persistence policy

M23 introduces **no persistent category, save-version change or migration**.
The original gravestone interaction writes no read marker and changes no world,
party, camera, flag, inventory or time state.

Existing v2 writing and v1/v2 reading are unchanged. `XeenWorld`'s disabled
object/event identities remain authoritative and are already serialized. When
such identities affect indoor objects, fresh owners and rebuilt map/object/sprite
caches derive the correct visibility without serializing placement, wall,
resolved-frame, command or pixel data. Cache reconstruction cannot resurrect a
removed object, including when another identity uses the same resource.

Save visual preflight uses the new derived indoor composition and existing
checked-failure path. A malformed selected sprite may fail preflight; it does not
cause a format bump, inferred repair or fresh-session fallback.

## Regression and original-data contract

Data-independent regression coverage establishes:

- A literal test-owned oracle for all twelve placements in all four camera
  directions, including exact source coordinates, order, anchors, scale and
  clipping for ordinary resources and resource 113. Record-index and table-slot
  negative controls prevent circular exceptional-resource logic.
- Independent wall-predicate cases, including conjunctions, wall nibbles and the
  query-29 asymmetry; geometry-only commands and map 33's wall-query vector remain
  stable.
- Discriminating synthetic raster bounds across every depth and both lateral
  sides, plus original order, partial and full coverage, two-cell sprites, flip,
  clipping and checked cache/resource failures.
- Stable original-record precedence and identity under base/session disable,
  shared resources, overlapping records, invalid visuals and separate/combined
  cache reconstruction. Indoor animation is rejected explicitly.
- A combined production Flow/composition lifecycle with a target and distinct
  shared-resource sibling: pending presentation, normal continuation through
  existing Remove, ghost-free recomposition, combined cache reconstruction,
  save, restore into fresh owners and continuing object/event suppression.
- Nightshadow target attribution, partial coverage, fully blocked view,
  All-facing interaction, sibling clue, exact no-mutation snapshot and distinct
  object-only versus event-disable semantics.

Original-data rendering uses the production command stream and attributes target
pixels by replaying the same composition without `{Clouds,29,14}`. The accepted
bounded views are:

| Camera (map 29) | Target contract |
| --- | --- |
| `(4,6)` West | Query 2, order 149, visible static primary view in front of its back wall |
| `(5,6)` West | Query 7, scale 7, visible; interaction does not dispatch from this adjacent cell |
| `(6,6)` West | Query 14, scale 12, visible |
| `(7,6)` West | Query 27, scale 14, visible |
| `(6,7)` West | Query 12, partially covered by a later wall while retaining target pixels |
| `(4,8)` South | Query 14 source is blocked by W27; no target command or contribution |

Map 33 geometry, Castle Basenji transfer, outdoor static and animated objects,
supported Remove paths, presentation and save/resume remain regression controls.
No original resource or generated original-data image is stored as a repository
fixture.

## Explicit non-goals

M23 excludes indoor ordinary animation, scripted appearance or movement, wall
items, monsters/combat, complete town or dungeon content, general indoor traversal,
neighbor/connected-map indoor rendering, lighting gameplay, doors, locks, grates,
traps, night/day effects, a general animation system or world clock, new mutation
opcodes, inventory/equipment expansion, Darkside gameplay and new save formats.
Rendering another static indoor object does not certify its script or region.

## Final acceptance

### Automated validation

- The complete build passed and the normal suite passed **61/61 CTest**.
- Required original-data indoor map/object, navigation, Remove and save/resume
  integration and regression checks passed against unmodified resources.
- Relevant headless SDL object, Remove and cross-process save/resume checks passed.

### Independent review

The independent reviewer found no production correctness or architecture defect.
The initial review identified two required regression-coverage gaps: circular
general-placement expectations and the absence of one combined indoor Remove
lifecycle. Both were corrected with the independent placement/raster oracle and
combined production Flow/save/restore test described above. Follow-up review
approved the corrected implementation candidate with no remaining P0, P1 or P2
findings.

### Maintainer physical SDL acceptance

On **2026-09-09**, the maintainer physically verified the required M23 SDL
checkpoint and controls:

- the Nightshadow RIP gravestone was visible and static at the primary view;
- the original clue and acknowledgment worked, with gameplay blocked while the
  acknowledgment was pending and repeat interaction causing no mutation;
- centered distance views showed coherent scaling;
- the lateral view showed partial wall occlusion and the blocked control hid the
  target completely; and
- the requested existing indoor regression controls remained functional.

This physical acceptance is distinct from automated/headless SDL evidence and
independent review. It certifies only M23's bounded checkpoint and controls, not
broader Nightshadow, town, dungeon, traversal or combat behavior.

M23 delivers the accepted static indoor-object foundation without extending
indoor animation, wall items, gameplay mutation categories or save state.
[Project status](project-status.md) owns the stable snapshot,
[history](project-history.md#m23---static-indoor-objects-and-first-visible-indoor-interaction)
records the concise result, and [roadmap](roadmap.md#current-planning-state)
records that no successor milestone is currently promoted.
