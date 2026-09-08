# MMModern - Project History

This is a concise summary of major completed milestones, not the authority for
current state or detailed validation. Read [project status](project-status.md)
for today's capabilities and boundaries, [roadmap](roadmap.md) for future
direction, and the linked milestone plans for detailed decisions and evidence.
Historical limitations in those plans describe their recorded boundaries.

## M1-M12 - Pre-public foundation

- By the M13 public baseline, the project had established original-resource
  loading, outdoor/indoor rendering, navigation/collision, party loading and
  the initial event foundation.
- Detailed M1-M12 milestone history is not reconstructed where surviving
  repository evidence is insufficient; these capabilities are not assigned to
  individual early milestones without evidence.

## M13 - Public baseline and event execution

- Established the initial public MMModern codebase and first public repository.
- Provided functional Xeen event decoding/execution, automatic events,
  conditions, calls/returns, teleports and game flags alongside the existing
  rendering, navigation and party foundation.
- Included automated tests and manual runtime validation. No dedicated M13 plan
  survives in the current documentation tree.

## M14 - Manual interaction and original text

- Added Space interaction from the current cell/facing independently of the
  automatic-event trigger gate, plus original event-text resource lookup.
- Added resumable display, acknowledgment and Yes/No execution with camera/flag
  transaction semantics across suspension.
- Integrated original fonts, labels/windows and paginated text into gameplay's
  indexed framebuffer and SDL input loop; Castle Basenji connected original
  dialogue choices to teleport presentation. No dedicated M14 plan survives in
  the current documentation tree; dependency validation also records its
  integration controls in [dependencies.md](dependencies.md).

## M15 - Session-owned world mutations

- Established side-aware map identity and stable original object/event record
  identities independent of disposable caches.
- Implemented Remove as immediate session-owned object/event overlays, retaining
  original metadata and logical/physical execution semantics.
- Established same-session reconstruction and fresh-session isolation. See the
  [Milestone 15 plan](milestone-15-plan.md).

## M16 - Static outdoor objects and visual Remove

- Resolved supported Clouds static visuals using World of Xeen metadata and
  integrated objects with outdoor terrain direction, scale, clipping and order.
- Added immediate scene refresh for Remove and presentation rebasing that retains
  valid text/response state through world changes and cache reconstruction.
- Kept selection and session ownership independent of rendering. See the
  [Milestone 16 plan](milestone-16-plan.md).

## M17 - Quest-item counters and Phirna collection

- Added party-owned Clouds quest-item counters, original loading, possession
  comparisons and bounded immediate grants.
- Completed original local Phirna harvesting: one Root, visible Remove, retained
  success text and repeat prevention, with No/already-owned controls.
- Preserved ownership/removal across same-session reconstruction and restored
  initial state in fresh sessions. See the [Milestone 17 plan](milestone-17-plan.md).

## M18 - WhoWill and Bone Whistle collection

- Added resumable WhoWill selection, eligibility feedback and cancellation,
  with temporary character context used by current-SP comparisons.
- Completed the original local Bone Whistle selection/acknowledgment/grant/Remove
  flow, including cancellation/retry and repeat prevention.
- Validated collection through session reconstruction and fresh-session controls.
  See the [Milestone 18 plan](milestone-18-plan.md).

## M19 - NPC dialogue and Myra's request

- Added Clouds NPC mode-1 presentation with original portraits, bounded
  speech/rest animation, positioned titles and paginated acknowledgment.
- Added party-owned Clouds quest flags, original loading and bounded immediate
  set; completed Myra's no-Root request and recorded quest flag 2.
- Established request/revisit behavior while retaining an explicit unsupported
  return-consumption boundary. See the [Milestone 19 plan](milestone-19-plan.md).

## M20 - Save and resume supported Clouds progress

- Added versioned value snapshots and validated restoration into existing owners
  for camera, modeled party/roster, independent flags/counters and world removals.
- Added local Windows F9 saving and startup resume, stable-boundary refusal,
  safe handled-failure replacement and resource compatibility checks.
- Accepted Phirna, Bone Whistle, Myra request and cumulative multi-map progress
  across process restarts, with fresh-session controls. See the
  [Milestone 20 plan](milestone-20-plan.md) for format and acceptance evidence.

## Maintaining this history

Normally add one short section with 2-4 lasting-result bullets per completed
milestone and link its plan. Keep stage chronology, acceptance matrices and
validation artifacts in that plan; do not duplicate current-state authority here.
Earlier M13/M14 status records remain available in repository history.
