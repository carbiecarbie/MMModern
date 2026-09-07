#ifndef MMODERN_GAMES_XEEN_NAVIGATION_H
#define MMODERN_GAMES_XEEN_NAVIGATION_H

#include "games/xeen/XeenMapIdentity.h"

#include <cstdint>

namespace mmodern {

enum class XeenDirection : std::uint8_t {
	North = 0,
	East = 1,
	South = 2,
	West = 3
};

struct XeenCamera {
	XeenMapIdentity mapId = 0;
	int x = 0;
	int y = 0;
	XeenDirection direction = XeenDirection::North;
};

} // namespace mmodern

#endif
