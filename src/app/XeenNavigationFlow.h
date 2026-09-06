#ifndef MMODERN_APP_XEEN_NAVIGATION_FLOW_H
#define MMODERN_APP_XEEN_NAVIGATION_FLOW_H

#include "core/NavigationAction.h"
#include "games/xeen/XeenEventSystem.h"
#include "games/xeen/XeenMovement.h"

namespace mmodern {

class XeenWorld;

struct XeenNavigationFlowResult {
	XeenMovementResult movementResult = XeenMovementResult::Moved;
	XeenAutomaticEventResult automaticEvent;
};

class XeenNavigationFlow {
public:
	explicit XeenNavigationFlow(XeenEventSystem &eventSystem);

	XeenAutomaticEventResult processInitialEvent(XeenWorld &world,
		const XeenPartyState &partyState, XeenCamera &camera,
		XeenGameFlags &gameFlags);

	XeenNavigationFlowResult processNavigationAction(XeenWorld &world,
		const XeenPartyState &partyState, XeenCamera &camera,
		XeenGameFlags &gameFlags, NavigationAction action);

	XeenManualEventResult processInteraction(XeenWorld &world,
		const XeenPartyState &partyState, XeenCamera &camera,
		XeenGameFlags &gameFlags);

private:
	XeenEventSystem &_eventSystem;
	XeenMovement _movement;
};

} // namespace mmodern

#endif
