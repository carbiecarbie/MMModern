#ifndef MMODERN_GAMES_XEEN_XEEN_EVENT_TRIGGER_H
#define MMODERN_GAMES_XEEN_XEEN_EVENT_TRIGGER_H

#include "games/xeen/XeenMap.h"

#include <cstdint>

namespace mmodern {

constexpr std::uint8_t kXeenAutomaticEventFlag = 0x10;

bool hasAutomaticTrigger(const XeenMapGeometry &geometry, int x, int y);

} // namespace mmodern

#endif
