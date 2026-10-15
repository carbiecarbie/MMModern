#ifndef MMODERN_XEEN_MONSTER_FORMAT_H
#define MMODERN_XEEN_MONSTER_FORMAT_H

#include <array>
#include <cstdint>
#include <string>
#include <vector>
#include "games/xeen/XeenMutation.h"

namespace mmodern {

// ScummVM character.h, pinned 6814ee9b: record hatred is not damage breadth.
enum class XeenMonsterHatred : unsigned { Dwarf=12, Party=15, Nobody=16 };

// Opaque combat fields remain bytes, never unchecked enum/table indexes.
struct XeenMonsterRecord {
	XeenMutableArray<std::uint8_t, 60> raw{};
	std::string name() const;
	std::uint32_t experience() const;
	std::uint16_t baseHp() const;
	std::uint16_t strikes() const;
	std::uint16_t gold() const;
	std::uint8_t image() const { return raw[47]; }
	bool supportsApproach() const;
	bool supportsMovement() const;
	bool supportsGroundMovement() const;
	bool supportsRendering() const;
	// The legacy presentation restriction remains a mechanic admission predicate.
	bool supportsAdmittedMechanicsRendering() const;
	void validatePresentation() const;
	bool loopAnimation() const { return raw[48] != 0; }
	unsigned animationEffect() const { return raw[49]; }
	bool flying() const { return raw[46] != 0; }
	void validateCombat() const;
	// Supported attack semantics, independent of species/resource identity and
	// animation metadata. Call before selecting targets or consuming RNG.
	void validateAttackCapabilities() const;
	std::uint32_t fingerprint() const;
	unsigned armorClass() const { return raw[22]; }
	unsigned speed() const { return raw[23]; }
	unsigned attacks() const { return raw[24]; }
	unsigned preferredClass() const { return raw[25]; }
	unsigned hatred() const { return raw[25]; }
	unsigned damageType() const { return raw[29]; }
	// SpecialAttack byte 30: 0 none, 5 Poison, 7 Disease, 8 Insane, 9 Sleep,
	// 10 Curse Item, 11 (In Love, no effect), 12 Drain SP, 13 Curse, 14 Paralyze,
	// 15 Unconscious, 16 Confuse, 17 Break Weapon, 18 Weaken, 19 Eradicate,
	// 20 Aging, 21 Death, 22 Stone; 1-4 and 6 reach no handler in DOS.
	unsigned specialAttack() const { return raw[30]; }
	bool ranged() const { return raw[32] != 0; }
	unsigned damageDie() const { return raw[28]; }
	unsigned hitParameter() const { return raw[31]; }
	// MonsterStruct::synchronize order (pinned ScummVM map.cpp): type 33, then
	// fire, electrical, cold, poison, energy, magic and physical resistance.
	unsigned monsterType() const { return raw[33]; }
	unsigned fireResistance() const { return raw[34]; }
	unsigned electricalResistance() const { return raw[35]; }
	unsigned coldResistance() const { return raw[36]; }
	unsigned poisonResistance() const { return raw[37]; }
	unsigned energyResistance() const { return raw[38]; }
	unsigned magicResistance() const { return raw[39]; }
	unsigned physicalResistance() const { return raw[40]; }
	// Elemental category order of the weapon material tables: Fire, Electrical,
	// Cold, Poison, Energy, Magic (XeenItem::getElementalCategory).
	unsigned elementalResistance(unsigned category) const { return raw.at(34+category); }
};

class XeenMonsterFormat {
public:
	static constexpr std::size_t kRecordSize = 60, kMaximumBytes = 65535;
	static std::vector<XeenMonsterRecord> parse(const std::vector<std::uint8_t> &bytes);
};

} // namespace mmodern
#endif
