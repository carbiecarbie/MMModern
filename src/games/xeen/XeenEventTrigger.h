#ifndef MMODERN_GAMES_XEEN_XEEN_EVENT_TRIGGER_H
#define MMODERN_GAMES_XEEN_XEEN_EVENT_TRIGGER_H

#include "games/xeen/XeenMap.h"

#include <cstdint>
#include <optional>

namespace mmodern {

constexpr std::uint8_t kXeenAutomaticEventFlag = 0x10;
constexpr std::uint8_t kXeenGrateUnlockedFlag = 0x80;

bool hasAutomaticTrigger(const XeenMapGeometry &geometry, int x, int y);

// Returns the facing wall value when Clouds would dispatch grate/door handling
// before ordinary event lookup. MMModern does not implement that handling yet.
std::optional<std::uint8_t> unsupportedManualSpecialInteraction(
	const XeenMapGeometry &geometry, int x, int y, XeenDirection direction);

} // namespace mmodern

#endif
