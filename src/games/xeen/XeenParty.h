#ifndef MMODERN_GAMES_XEEN_XEEN_PARTY_H
#define MMODERN_GAMES_XEEN_XEEN_PARTY_H

#include "games/xeen/XeenCharacter.h"

#include <array>
#include <cstddef>
#include <cstdint>
#include <string>
#include <vector>

namespace mmodern {

class XeenRoster {
public:
	static constexpr std::size_t kCharacterCount = 30;

	const XeenCharacter &at(std::size_t rosterId) const;
	XeenCharacter &at(std::size_t rosterId);
	const std::array<XeenCharacter, kCharacterCount> &characters() const { return _characters; }

private:
	std::array<XeenCharacter, kCharacterCount> _characters{};
};

class XeenParty {
public:
	static constexpr std::size_t kSerializedMemberSlots = 8;
	static constexpr std::size_t kMaximumVisibleMembers = 6;

	const std::vector<std::uint8_t> &activeRosterIds() const { return _activeRosterIds; }
	std::size_t size() const { return _activeRosterIds.size(); }
	const XeenCharacter &member(const XeenRoster &roster, std::size_t partyIndex) const;

private:
	friend class XeenPartyLoader;
	std::vector<std::uint8_t> _activeRosterIds;
};

struct XeenPartyState {
	XeenRoster roster;
	XeenParty party;
	std::uint8_t firstSerializedCount = 0;
	std::uint8_t effectiveSerializedCount = 0;
	std::vector<std::string> diagnostics;
};

} // namespace mmodern

#endif
