#ifndef MMODERN_TESTS_XEEN_MONSTER_TEST_ACCESS_H
#define MMODERN_TESTS_XEEN_MONSTER_TEST_ACCESS_H
#include "games/xeen/XeenJourneyCapture.h"
#include "games/xeen/XeenRestoreGuard.h"
namespace mmodern {
// Synthetic fixture control, never route evidence. Decision 8 (M69) rebuilds
// monster HP and wake state on every map and game load, so a save can never
// carry a wound; a fixture wounds or wakes the live monster after restore,
// before the Journey flow adopts the restored world. The restoration preimage
// adopts exactly this change.
struct XeenMonsterTestAccess {
	// Inject a control at a quiet boundary; use the existing owner preimage.
	template<class EncounterFlow>
	static void status(EncounterFlow &flow, XeenWorld &world, XeenMapIdentity map, std::size_t index, XeenActorStatus status) {
		auto &guard=*flow._journeyPreimage;guard.check();
		world._sessionState.mutableMonsters(map).at(index).status=status;
		guard.s._mapStates=world._sessionState._mapStates;guard.adoptMutationBoundary();
	}
	static void set(XeenWorld &world, XeenMapIdentity map, std::size_t index, std::int32_t hp, bool activated,
		XeenActorStatus status=XeenActorStatus::Physical) {
		if(!world._journeyRestoration || !world._journeyRestoration->guard)
			throw std::logic_error("Live monster fixture needs an unconsumed Journey restoration");
		auto &actor=world._sessionState.mutableMonsters(map).at(index);
		actor.hp=hp;actor.activated=activated;actor.status=status;
		auto &guard=*world._journeyRestoration->guard;
		guard.s._mapStates=world._sessionState._mapStates;guard.adoptMutationBoundary();
	}
	// Synthetic state on a map the fixture never visited (decision 2 controls),
	// adopted by the unconsumed restoration preimage like set().
	static void removeObject(XeenWorld &world, XeenObjectIdentity id) {
		if(!world._journeyRestoration || !world._journeyRestoration->guard)
			throw std::logic_error("Live map fixture needs an unconsumed Journey restoration");
		world._sessionState._mapStates.state(id.mapId).removedObjects.insert(id);
		auto &guard=*world._journeyRestoration->guard;
		guard.s._mapStates=world._sessionState._mapStates;guard.adoptMutationBoundary();
	}
	// The same control in a running application, at a quiet boundary before
	// combat: the live Journey preimage adopts exactly this change.
	template<class EventFlow>
	static void setLive(EventFlow &flow, XeenWorld &world, XeenMapIdentity map, std::size_t index, std::int32_t hp, bool activated) {
		auto &guard=*flow._encounter->_journeyPreimage;
		guard.check();
		auto &actor=world._sessionState.mutableMonsters(map).at(index);
		actor.hp=hp;actor.activated=activated;
		guard.s._mapStates=world._sessionState._mapStates;guard.adoptMutationBoundary();
	}
	// Running-program pending treasure at a quiet boundary: the live Journey
	// preimage adopts exactly this publication, like status() and setLive().
	static void stageTreasure(XeenRestoreGuard &guard, XeenWorld &world, const XeenTreasureStaging &value) {
		guard.check();world.treasureStaging().publish(value);
		guard.staging=value;guard.stagingRevision=world.treasureStaging().revision();guard.adoptMutationBoundary();
	}
	template<class EncounterFlow>
	static void stageTreasure(EncounterFlow &flow, XeenWorld &world, const XeenTreasureStaging &value) {
		stageTreasure(*flow._journeyPreimage, world, value);
	}
	template<class EventFlow>
	static void stageTreasureLive(EventFlow &flow, XeenWorld &world, const XeenTreasureStaging &value) {
		stageTreasure(*flow._encounter->_journeyPreimage, world, value);
	}
};
}
#endif
