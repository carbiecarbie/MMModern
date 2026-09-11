#ifndef MMODERN_XEEN_COMBAT_INPUTS_H
#define MMODERN_XEEN_COMBAT_INPUTS_H
#include "games/xeen/XeenCharacter.h"
#include <array>
namespace mmodern {
// CHR-only inputs absent from ordinary persistence. HP/items remain in characters.
struct XeenCombatInputs {
	XeenAttributeValue might, speed, accuracy;
	int temporaryAc = 0;
	std::uint32_t experience = 0;
};
inline constexpr std::array<std::uint8_t,6> kXeenCombatOwners{0,18,14,11,1,6};
}
#endif
