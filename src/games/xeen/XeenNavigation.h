#ifndef MMODERN_GAMES_XEEN_NAVIGATION_H
#define MMODERN_GAMES_XEEN_NAVIGATION_H

#include "games/xeen/XeenMapIdentity.h"
#include "games/xeen/XeenGameplayBorrow.h"
#include "games/xeen/XeenMutation.h"

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
	XeenMutable<int> x = 0;
	XeenMutable<int> y = 0;
	XeenMutable<XeenDirection> direction = XeenDirection::North;
	XeenGameplayBorrowOwner gameplayBorrow{};
};

} // namespace mmodern

#endif
