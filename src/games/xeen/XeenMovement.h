#ifndef MMODERN_GAMES_XEEN_MOVEMENT_H
#define MMODERN_GAMES_XEEN_MOVEMENT_H

#include "core/NavigationAction.h"
#include "games/xeen/XeenNavigation.h"

namespace mmodern {

class XeenWorld;

enum class XeenMovementResult {
	Turned,
	Moved,
	BlockedByMapBoundary,
	BlockedByWall,
	BlockedByTerrain,
	BlockedBySurface
};

class XeenMovement {
public:
	XeenMovementResult apply(XeenWorld &world, XeenCamera &camera,
		NavigationAction action) const;
};

} // namespace mmodern

#endif
