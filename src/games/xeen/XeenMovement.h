#ifndef MMODERN_GAMES_XEEN_MOVEMENT_H
#define MMODERN_GAMES_XEEN_MOVEMENT_H

#include "core/NavigationAction.h"
#include "games/xeen/XeenNavigation.h"
#include "games/xeen/XeenMap.h"
#include <bitset>

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
	// The admitted ordinary Clouds capability value. Other abilities require
	// their actual consumers rather than silently changing this predicate.
	struct Capabilities {
		bool swimming = false, walkOnWater = false, mountaineer = false;
	};
	static XeenMovementResult outdoorDestination(const XeenMapGeometry &geometry,
		const XeenMapCell &cell, Capabilities capabilities);
	static XeenMovementResult localOutdoor(const XeenMap &map, int fromX, int fromY,
		int toX, int toY, Capabilities capabilities);
	static std::bitset<256> component(const XeenMap &map, int anchorX, int anchorY,
		Capabilities capabilities);
	XeenMovementResult apply(XeenWorld &world, XeenCamera &camera,
		NavigationAction action) const;
};

} // namespace mmodern

#endif
