#ifndef MMODERN_GAMES_XEEN_NAVIGATION_H
#define MMODERN_GAMES_XEEN_NAVIGATION_H

#include <cstdint>

namespace mmodern {

enum class XeenDirection : std::uint8_t {
	North = 0,
	East = 1,
	South = 2,
	West = 3
};

struct XeenCamera {
	std::uint16_t mapId = 0;
	int x = 0;
	int y = 0;
	XeenDirection direction = XeenDirection::North;
};

} // namespace mmodern

#endif
