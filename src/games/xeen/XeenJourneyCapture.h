#ifndef MMODERN_XEEN_JOURNEY_CAPTURE_H
#define MMODERN_XEEN_JOURNEY_CAPTURE_H
#include "games/xeen/XeenCombat.h"
#include "games/xeen/XeenRestoreGuard.h"
#include "games/xeen/XeenJourneyRules.h"
namespace mmodern {
// Flow alone owns this lifetime. World holds a weak reference, never a raw
// coordinator or callback. No countdown is copied: queries read the actual state.
class XeenJourneyCapture {
	friend class XeenEncounterFlow;
	friend class XeenSaveState;
	friend class XeenWorld;
	const XeenWorld *w = nullptr;
	const XeenPartyState *p = nullptr;
	const XeenCamera *c = nullptr;
	const XeenEncounterState *state = nullptr;
	const XeenCombatBoundary *boundary = nullptr;
	const bool *busy = nullptr;
	const std::shared_ptr<XeenRestoreGuard> *preimage = nullptr;
	std::uint64_t generation = 0;
	bool closed = false;
	std::vector<XeenActor> admittedActors;
	XeenJourneyCapture() = default;
	XeenJourneyCapture(const XeenWorld &w, const XeenPartyState &p, const XeenCamera &c,
		const XeenEncounterState &state, const XeenCombatBoundary &boundary, const bool &busy,
		const std::shared_ptr<XeenRestoreGuard> &preimage) { bind(w,p,c,state,boundary,busy,preimage); }
	void bind(const XeenWorld &world, const XeenPartyState &party, const XeenCamera &camera,
		const XeenEncounterState &coordination, const XeenCombatBoundary &external, const bool &work,
		const std::shared_ptr<XeenRestoreGuard> &guard) noexcept {
		w=&world; p=&party; c=&camera; state=&coordination; boundary=&external; busy=&work; preimage=&guard;
	}
	bool current(const XeenPartyState &party, const XeenCamera &camera) const noexcept {
		if (closed || &party != p || &camera != c || !preimage || !*preimage || !(*preimage)->ownersAlive()) return false;
		// Unavailable coordination is not an integrity observation. In particular,
		// combat publications and legitimate leases need not match the quiet preimage.
		if (*busy || generation != boundary->generation() || !boundary->quiet() ||
			w->sessionState().journeyActivity() != XeenJourneyActivity::Quiet ||
			state->pending() != 0 || state->phase() != XeenEncounterPhase::Exploring ||
			state->reason() != XeenEncounterStop::None || !XeenActorApproach::authoritative(*w,*p,*c,*state)) return false;
		// Reuse the retained guard's monotonic failure latch. Flow checks this same
		// guard before any later publication; equal bytes cannot revive its ticket.
		try { (*preimage)->check(); } catch (...) { return false; }
		const auto &actors = w->sessionState().actors();
		if (actors.size() != 27 || admittedActors.size() != 27) return false;
		for (unsigned i = 0; i < 27; ++i)
			if (!xeenJourneyContent(w->sessionState().journeyContract()).influences(i) && !xeen_state::sameActor(actors[i],admittedActors[i])) return false;
		for (auto id:w->sessionState().accountedMonsters())
			if (id.mapId!=XeenMapIdentity(20) || !xeenJourneyContent(w->sessionState().journeyContract()).influences(id.recordIndex)) return false;
		return true;
	}
};

// Unpublished-to-published handoff, created only by SaveState. It carries
// detached resource values and a destination-bound preimage, never End authority.
struct XeenJourneyRestoration {
	std::shared_ptr<XeenJourneyCapture> capture;
	std::shared_ptr<XeenRestoreGuard> guard;
	std::vector<XeenMonsterRecord> statistics;
	XeenEventFile events;
};
}
#endif
