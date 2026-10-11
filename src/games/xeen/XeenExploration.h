#ifndef MMODERN_XEEN_EXPLORATION_H
#define MMODERN_XEEN_EXPLORATION_H
#include "games/xeen/XeenMapIdentity.h"
#include <array>
namespace mmodern {
// Physical DAT tile and y*16+x bits. Resource stepped bits remain immutable.
struct XeenExploredTile {
 XeenMapIdentity mapId;
 std::array<std::uint8_t,32> cells{};
 friend bool operator==(const XeenExploredTile &a,const XeenExploredTile &b) {
  return a.mapId==b.mapId && a.cells==b.cells;
 }
 friend bool operator!=(const XeenExploredTile &a,const XeenExploredTile &b) {return !(a==b);}
};
}
#endif
