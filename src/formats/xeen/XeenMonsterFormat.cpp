#include "formats/xeen/XeenMonsterFormat.h"

#include <algorithm>
#include <stdexcept>

namespace mmodern {
namespace {
std::uint16_t word(const XeenMonsterRecord &r, std::size_t p) {
	return static_cast<std::uint16_t>(r.raw[p] | (std::uint16_t(r.raw[p + 1]) << 8));
}
}

std::string XeenMonsterRecord::name() const {
	const auto end = std::find(raw.begin(), raw.begin() + 15, 0);
	return std::string(raw.begin(), end);
}
std::uint32_t XeenMonsterRecord::experience() const {
	return word(*this, 16) | (std::uint32_t(word(*this, 18)) << 16);
}
std::uint16_t XeenMonsterRecord::baseHp() const { return word(*this, 20); }
std::uint16_t XeenMonsterRecord::strikes() const { return word(*this, 26); }
std::uint16_t XeenMonsterRecord::gold() const { return word(*this, 42); }
bool XeenMonsterRecord::supportsApproach() const {
	return baseHp() != 0 && raw[30] == 0 && raw[32] == 0 && raw[46] == 0 &&
		raw[47] != 255 && raw[48] == 0 && raw[49] == 0;
}

std::vector<XeenMonsterRecord> XeenMonsterFormat::parse(const std::vector<std::uint8_t> &bytes) {
	if (bytes.empty() || bytes.size() > kMaximumBytes || bytes.size() % kRecordSize)
		throw std::invalid_argument("malformed DARK.CC/xeen.mon: expected complete 60-byte records");
	std::vector<XeenMonsterRecord> records(bytes.size() / kRecordSize);
	for (std::size_t i = 0; i < records.size(); ++i)
		std::copy_n(bytes.begin() + i * kRecordSize, kRecordSize, records[i].raw.begin());
	return records;
}
void XeenMonsterRecord::validateCombat() const {
	if (!supportsApproach() || baseHp()!=20 || experience()!=250 || armorClass()!=5 || speed()!=10 ||
		attacks()!=1 || preferredClass()!=3 || strikes()!=2 || damageDie()!=6 || raw[29]!=0 ||
		hitParameter()!=4 || raw[33]!=4 || physicalResistance()!=50 || gold()!=0 || raw[44]!=0 || raw[45]!=0 || image()!=8)
		throw std::invalid_argument("monster resource is outside the Diagnostic27 combat profile");
	for (unsigned i=34;i<=40;++i) if (raw[i]>100)
		throw std::invalid_argument("monster resistance percentage out of bounds");
}
} // namespace mmodern
