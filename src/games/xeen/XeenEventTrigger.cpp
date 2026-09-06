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

std::optional<std::uint8_t> unsupportedManualSpecialInteraction(
		const XeenMapGeometry &geometry, int x, int y, XeenDirection direction) {
	if (geometry.isOutdoors() || x < 0 || y < 0 ||
			x >= static_cast<int>(XeenMapGeometry::kWidth) ||
			y >= static_cast<int>(XeenMapGeometry::kHeight))
		return std::nullopt;
	const unsigned directionIndex = static_cast<unsigned>(direction);
	if (directionIndex > static_cast<unsigned>(XeenDirection::West))
		return std::nullopt;
	const XeenMapCell &cell = geometry.cells[static_cast<std::size_t>(y) *
		XeenMapGeometry::kWidth + static_cast<std::size_t>(x)];
	const std::uint8_t wall = wallAt(cell, direction);
	if (wall == 1 || wall == 6 || wall == 9 ||
			(wall == 13 && (cell.rawAttributes & kXeenGrateUnlockedFlag) != 0))
		return wall;
	return std::nullopt;
}

} // namespace mmodern
