#ifndef MMODERN_GAMES_XEEN_MOVEMENT_H
#define MMODERN_GAMES_XEEN_MOVEMENT_H

#include "core/NavigationAction.h"
#include "games/xeen/XeenNavigation.h"
#include "games/xeen/XeenMap.h"
#include <bitset>

namespace mmodern {

class XeenWorld;
struct XeenPartyState;

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
	// The party's ordinary traversal capabilities. Other abilities require
	// their actual consumers rather than silently changing this predicate.
	struct Capabilities {
		bool swimming = false, walkOnWater = false, mountaineer = false, pathfinder = false;
	};
	static Capabilities capabilities(const XeenPartyState &);
	static XeenMovementResult outdoorDestination(const XeenMapGeometry &geometry,
		const XeenMapCell &cell, Capabilities capabilities, XeenSide side);
	static XeenMovementResult localOutdoor(const XeenMap &map, int fromX, int fromY,
		int toX, int toY, Capabilities capabilities);
	static std::bitset<256> component(const XeenMap &map, int anchorX, int anchorY,
		Capabilities capabilities);
	// Indoor front wall drawn as a door (types 4 and 13, the pin's _wo[107]);
	// an admitted forward move through it requests FX47 (DAT 0x0cf93..0x0cfb7).
	static bool doorAhead(XeenWorld &world, const XeenCamera &camera);
	XeenMovementResult apply(XeenWorld &world, XeenCamera &camera,
		NavigationAction action, Capabilities capabilities = {false,false,false,false}) const;
};

} // namespace mmodern

#endif
