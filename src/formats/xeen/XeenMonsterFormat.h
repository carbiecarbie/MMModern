#ifndef MMODERN_XEEN_MONSTER_FORMAT_H
#define MMODERN_XEEN_MONSTER_FORMAT_H

#include <array>
#include <cstdint>
#include <string>
#include <vector>

namespace mmodern {

// Opaque combat fields remain bytes, never unchecked enum/table indexes.
struct XeenMonsterRecord {
	std::array<std::uint8_t, 60> raw{};
	std::string name() const;
	std::uint32_t experience() const;
	std::uint16_t baseHp() const;
	std::uint16_t strikes() const;
	std::uint16_t gold() const;
	std::uint8_t image() const { return raw[47]; }
	bool supportsApproach() const;
	void validateCombat() const;
	unsigned armorClass() const { return raw[22]; }
	unsigned speed() const { return raw[23]; }
	unsigned attacks() const { return raw[24]; }
	unsigned preferredClass() const { return raw[25]; }
	unsigned damageDie() const { return raw[28]; }
	unsigned hitParameter() const { return raw[31]; }
	unsigned physicalResistance() const { return raw[40]; }
};

class XeenMonsterFormat {
public:
	static constexpr std::size_t kRecordSize = 60, kMaximumBytes = 65535;
	static std::vector<XeenMonsterRecord> parse(const std::vector<std::uint8_t> &bytes);
};

} // namespace mmodern
#endif
