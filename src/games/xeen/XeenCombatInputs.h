#ifndef MMODERN_XEEN_COMBAT_INPUTS_H
#define MMODERN_XEEN_COMBAT_INPUTS_H
#include "games/xeen/XeenCharacter.h"
#include <array>
namespace mmodern {
struct XeenCombatResistances {
	std::uint8_t coldPermanent = 0, coldTemporary = 0, electricalPermanent = 0, electricalTemporary = 0;
	friend bool operator==(const XeenCombatResistances &a, const XeenCombatResistances &b) noexcept {
		return a.coldPermanent == b.coldPermanent && a.coldTemporary == b.coldTemporary &&
			a.electricalPermanent == b.electricalPermanent && a.electricalTemporary == b.electricalTemporary;
	}
	friend bool operator!=(const XeenCombatResistances &a, const XeenCombatResistances &b) noexcept { return !(a == b); }
};
// CHR-only inputs absent from ordinary persistence. HP/items remain in characters.
struct XeenCombatInputs {
	XeenAttributeValue might, speed, accuracy;
	int temporaryAc = 0;
	std::uint32_t experience = 0;
	// Explicit successor input; legacy supplement presence does not supply Luck.
	std::optional<XeenAttributeValue> luck;
	std::optional<XeenCombatResistances> resistances;
};
inline constexpr std::array<std::uint8_t,6> kXeenCombatOwners{0,18,14,11,1,6};
}
#endif
