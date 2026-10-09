#ifndef MMODERN_XEEN_JOURNEY_CAPTURE_H
#define MMODERN_XEEN_JOURNEY_CAPTURE_H
#include "games/xeen/XeenCombat.h"
#include "games/xeen/XeenRestoreGuard.h"
#include "games/xeen/XeenJourneyRules.h"
#include "games/xeen/XeenIndoorScene.h"
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
	const bool *needsRestNotice = nullptr;
	const std::shared_ptr<XeenRestoreGuard> *preimage = nullptr;
	std::uint64_t generation = 0;
	bool closed = false;
	bool initializationPending = false;
	std::vector<XeenActor> admittedActors;
	XeenJourneyCapture() = default;
	XeenJourneyCapture(const XeenWorld &w, const XeenPartyState &p, const XeenCamera &c,
		const XeenEncounterState &state, const XeenCombatBoundary &boundary, const bool &busy,
		const std::shared_ptr<XeenRestoreGuard> &preimage,const bool *notice=nullptr) { bind(w,p,c,state,boundary,busy,preimage,notice); }
	void bind(const XeenWorld &world, const XeenPartyState &party, const XeenCamera &camera,
		const XeenEncounterState &coordination, const XeenCombatBoundary &external, const bool &work,
		const std::shared_ptr<XeenRestoreGuard> &guard,const bool *notice=nullptr) noexcept {
		w=&world; p=&party; c=&camera; state=&coordination; boundary=&external; busy=&work; preimage=&guard;needsRestNotice=notice;
	}
	bool current(const XeenPartyState &party, const XeenCamera &camera, bool initializing=false) const noexcept {
		if (closed || &party != p || &camera != c || !preimage || !*preimage || !(*preimage)->ownersAlive()) return false;
		// Unavailable coordination is not an integrity observation. In particular,
		// combat publications and legitimate leases need not match the quiet preimage.
		if (*busy || (needsRestNotice && *needsRestNotice) || generation != boundary->generation() || !boundary->quiet() ||
			(initializing ? (!initializationPending || w->sessionState().journeyActivity()!=XeenJourneyActivity::Presentation) :
			 w->sessionState().journeyActivity()!=XeenJourneyActivity::Quiet) ||
			state->pending() != 0 || state->phase() != XeenEncounterPhase::Exploring ||
			state->reason() != XeenEncounterStop::None || !XeenActorApproach::authoritative(*w,*p,*c,*state)) return false;
		// Reuse the retained guard's monotonic failure latch. Flow checks this same
		// guard before any later publication; equal bytes cannot revive its ticket.
		try { (*preimage)->check(); } catch (...) { return false; }
		const auto &actors = w->sessionState().actors();
		const auto expected=admittedActors.size();
		if (!expected || expected>XeenActorApproach::kCapacity || actors.size() != expected) return false;
		for (auto id:w->sessionState().accountedMonsters()) {
			if(!w->sessionState().hasRegionalActors(id.mapId)) return false;
			const auto owned=w->sessionState().regionalActors(id.mapId);
			if(id.recordIndex>=owned.size() || owned[id.recordIndex].lifecycle!=XeenActorLifecycle::Defeated) return false;
		}
		if (c->mapId==XeenMapIdentity(28)) {
			if (!w->sessionState().hasRegionalActors(28)) return false;
			try {
				const auto &city=w->sessionState().regionalActors(28);
				const auto view=XeenIndoorScene().classifyActors(*const_cast<XeenWorld *>(w),*c,city);
				if (view.engaged()) return false;
				for(unsigned i=0;i<city.size();++i) if(view.activation[i] && !city[i].activated) return false;
				if(p->monsterTreasure && p->monsterTreasure->ready()) {
					bool selected=false;for(const auto &slot:view.slots)selected=selected || bool(slot);
					if(!selected)return false;
				}
			} catch (...) { return false; }
			return true;
		}
		if(p->monsterTreasure && p->monsterTreasure->ready()) {
			const auto view=XeenActorApproach::classify(actors,*c);bool selected=false;
			for(const auto &v:view.slots) selected=selected || bool(v);if(!selected) return false;
		}
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
	std::function<XeenLearnedSpellNames()> learnedNamesProvider;
};
}
#endif
