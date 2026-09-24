#ifndef MMODERN_XEEN_COMBAT_INPUTS_H
#define MMODERN_XEEN_COMBAT_INPUTS_H
#include "games/xeen/XeenCharacter.h"
#include <array>
#include "games/xeen/XeenMutation.h"
namespace mmodern {
struct XeenCombatResistances {
	XeenMutable<std::uint8_t> coldPermanent = 0, coldTemporary = 0, electricalPermanent = 0, electricalTemporary = 0;
	friend bool operator==(const XeenCombatResistances &a, const XeenCombatResistances &b) noexcept {
		return a.coldPermanent == b.coldPermanent && a.coldTemporary == b.coldTemporary &&
			a.electricalPermanent == b.electricalPermanent && a.electricalTemporary == b.electricalTemporary;
	}
	friend bool operator!=(const XeenCombatResistances &a, const XeenCombatResistances &b) noexcept { return !(a == b); }
};
// CHR-only inputs absent from ordinary persistence. HP/items remain in characters.
struct XeenCombatInputs {
	XeenAttributeValue might, speed, accuracy;
	XeenMutable<int> temporaryAc = 0;
	XeenMutable<std::uint32_t> experience = 0;
	// Explicit successor input; legacy supplement presence does not supply Luck.
	XeenMutableOptional<XeenAttributeValue> luck;
	XeenMutableOptional<XeenCombatResistances> resistances;
	// Original CHR bytes 317/318. Presence is specific to Journey content 8.
	XeenMutableOptional<XeenAttributeValue> poisonResistance;
};
inline constexpr std::array<std::uint8_t,6> kXeenCombatOwners{0,18,14,11,1,6};
}
#endif
