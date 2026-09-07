#include "app/XeenNavigationFlow.h"

#include "games/xeen/XeenWorld.h"

namespace mmodern {

XeenNavigationFlow::XeenNavigationFlow(XeenEventSystem &eventSystem) :
	_eventSystem(eventSystem) {
}

XeenAutomaticEventResult XeenNavigationFlow::processInitialEvent(
		XeenWorld &world, XeenPartyState &partyState, XeenCamera &camera,
		XeenGameFlags &gameFlags) {
	return _eventSystem.runAutomaticEvent(world, partyState, camera, gameFlags);
}

XeenNavigationFlowResult XeenNavigationFlow::processNavigationAction(
		XeenWorld &world, XeenPartyState &partyState, XeenCamera &camera,
		XeenGameFlags &gameFlags, NavigationAction action) {
	const XeenMovementResult movementResult = _movement.apply(world, camera, action);
	return {movementResult,
		_eventSystem.runAutomaticEvent(world, partyState, camera, gameFlags)};
}

XeenManualEventResult XeenNavigationFlow::processInteraction(
		XeenWorld &world, XeenPartyState &partyState, XeenCamera &camera,
		XeenGameFlags &gameFlags) {
	return _eventSystem.runManualEvent(world, partyState, camera, gameFlags);
}

} // namespace mmodern
