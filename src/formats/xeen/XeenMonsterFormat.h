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
	// Existing poison combat admission is intentionally unchanged. Presentation
	// uses validatePresentation and sprite validation, never this fingerprint.
	void validateAdmittedPoisonCombat() const;
	std::uint32_t fingerprint() const;
	unsigned armorClass() const { return raw[22]; }
	unsigned speed() const { return raw[23]; }
	unsigned attacks() const { return raw[24]; }
	unsigned preferredClass() const { return raw[25]; }
	unsigned hatred() const { return raw[25]; }
	unsigned damageType() const { return raw[29]; }
	unsigned damageDie() const { return raw[28]; }
	unsigned hitParameter() const { return raw[31]; }
	unsigned magicResistance() const { return raw[39]; }
	unsigned physicalResistance() const { return raw[40]; }
};

class XeenMonsterFormat {
public:
	static constexpr std::size_t kRecordSize = 60, kMaximumBytes = 65535;
	static std::vector<XeenMonsterRecord> parse(const std::vector<std::uint8_t> &bytes);
};

} // namespace mmodern
#endif
