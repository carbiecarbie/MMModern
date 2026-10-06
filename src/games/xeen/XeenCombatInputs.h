#ifndef MMODERN_XEEN_COMBAT_INPUTS_H
#define MMODERN_XEEN_COMBAT_INPUTS_H
#include "games/xeen/XeenCharacter.h"
#include <array>
#include "games/xeen/XeenMutation.h"
namespace mmodern {
struct XeenCombatResistances {
	XeenMutable<std::uint8_t> coldPermanent = 0, coldTemporary = 0, electricalPermanent = 0, electricalTemporary = 0;
	XeenMutable<std::uint8_t> firePermanent=0, fireTemporary=0, energyPermanent=0, energyTemporary=0, magicPermanent=0, magicTemporary=0;
	friend bool operator==(const XeenCombatResistances &a, const XeenCombatResistances &b) noexcept {
		return a.coldPermanent == b.coldPermanent && a.coldTemporary == b.coldTemporary &&
			a.electricalPermanent == b.electricalPermanent && a.electricalTemporary == b.electricalTemporary &&
			a.firePermanent==b.firePermanent && a.fireTemporary==b.fireTemporary && a.energyPermanent==b.energyPermanent &&
			a.energyTemporary==b.energyTemporary && a.magicPermanent==b.magicPermanent && a.magicTemporary==b.magicTemporary;
	}
	friend bool operator!=(const XeenCombatResistances &a, const XeenCombatResistances &b) noexcept { return !(a == b); }
};
// Roster-owned live CHR supplements, persisted by the current Journey format.
// HP/items remain in characters.
struct XeenCombatInputs {
	XeenAttributeValue might, speed, accuracy;
	XeenMutable<int> temporaryAc = 0;
	XeenMutable<std::uint32_t> experience = 0;
	// Explicit successor input; legacy supplement presence does not supply Luck.
	XeenMutableOptional<XeenAttributeValue> luck;
	XeenMutableOptional<XeenCombatResistances> resistances;
	// Original CHR bytes 317/318. Required by the current Journey.
	XeenMutableOptional<XeenAttributeValue> poisonResistance;
};
inline constexpr std::array<std::uint8_t,6> kXeenCombatOwners{0,18,14,11,1,6};
}
#endif
