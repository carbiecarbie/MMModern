#ifndef MMODERN_XEEN_TERRAIN_ARRIVAL_H
#define MMODERN_XEEN_TERRAIN_ARRIVAL_H
#include "games/xeen/XeenTransitionResult.h"
#include <cstdint>
namespace mmodern {
enum class XeenDamageType : std::uint8_t;
struct XeenTerrainArrival {
 XeenTransitionResult transition;
 unsigned surface=0, minutes=0, damage=0;
 XeenDamageType type=static_cast<XeenDamageType>(0);
 bool falling=false, dead=false;
 explicit operator bool() const {return bool(transition);}
};
}
#endif
