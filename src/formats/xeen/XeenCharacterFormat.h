#ifndef MMODERN_FORMATS_XEEN_XEEN_CHARACTER_FORMAT_H
#define MMODERN_FORMATS_XEEN_XEEN_CHARACTER_FORMAT_H

#include "games/xeen/XeenParty.h"

#include <array>
#include <cstdint>
#include <vector>

namespace mmodern {

class XeenCharacterFormat {
public:
	struct PartyHeader {
		std::uint8_t firstCount = 0;
		std::uint8_t effectiveCount = 0;
		std::array<int, XeenParty::kSerializedMemberSlots> rosterIds{};
	};

	static XeenRoster parseRoster(const std::vector<std::uint8_t> &bytes);
	// Parsing produces inert values, never installs encounter authority.
	static XeenCombatInputs parseCombatInputs(const std::vector<std::uint8_t> &bytes, std::size_t owner, bool includeLuck = false);
	static PartyHeader parsePartyHeader(const std::vector<std::uint8_t> &bytes);
};

} // namespace mmodern

#endif
