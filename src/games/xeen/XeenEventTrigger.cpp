#include "games/xeen/XeenEventTrigger.h"

#include <cstddef>

namespace mmodern {

bool hasAutomaticTrigger(const XeenMapGeometry &geometry, int x, int y) {
	if (x < 0 || y < 0 || x >= static_cast<int>(XeenMapGeometry::kWidth) ||
			y >= static_cast<int>(XeenMapGeometry::kHeight))
		return false;
	const std::size_t index = static_cast<std::size_t>(y) *
		XeenMapGeometry::kWidth + static_cast<std::size_t>(x);
	return (geometry.cells[index].rawAttributes & kXeenAutomaticEventFlag) != 0;
}

} // namespace mmodern
