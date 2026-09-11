#ifndef MMODERN_GAMES_XEEN_XEEN_PARTY_H
#define MMODERN_GAMES_XEEN_XEEN_PARTY_H

#include "games/xeen/XeenCharacter.h"
#include "games/xeen/XeenGameplayContext.h"

#include <array>
#include <cstddef>
#include <cstdint>
#include <optional>
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
	// Restore exact active order, including the loader's supported duplicates.
	static XeenParty fromRosterIds(std::vector<std::uint8_t> ids);

	const std::vector<std::uint8_t> &activeRosterIds() const { return _activeRosterIds; }
	std::size_t size() const { return _activeRosterIds.size(); }
	const XeenCharacter &member(const XeenRoster &roster, std::size_t partyIndex) const;

private:
	friend class XeenPartyLoader;
	std::vector<std::uint8_t> _activeRosterIds;
};

// Party-owned Clouds quest counters, independent of character inventories.
class XeenCloudsQuestItems {
public:
	static constexpr std::size_t kCount = 35;
	static constexpr std::int64_t kFirstItemId = 82;
	using Counts = std::array<std::uint32_t, kCount>;

	XeenCloudsQuestItems() = default;
	explicit XeenCloudsQuestItems(Counts counts) : _counts(counts) {}
	static std::optional<std::size_t> indexForItemId(std::int64_t itemId);
	std::uint32_t at(std::size_t index) const { return _counts.at(index); }
	// Bounded access; returns false on overflow without changing the counter.
	bool increment(std::size_t index);
	// Bounded access; returns false at zero without changing the counter.
	bool decrement(std::size_t index);
	const Counts &counts() const { return _counts; }

private:
	Counts _counts{};
};

// Clouds request state, separate from transactional game flags and item counts.
class XeenCloudsQuestFlags {
public:
	static constexpr std::size_t kCount = 30;
	using Values = std::array<bool, kCount>;
	XeenCloudsQuestFlags() = default;
	explicit XeenCloudsQuestFlags(Values values) : _values(values) {}
	static bool validIndex(std::int64_t index);
	bool isSet(std::int64_t index) const;
	void set(std::int64_t index);
	void clear(std::int64_t index);
	const Values &values() const { return _values; }
private:
	static std::size_t checkedIndex(std::int64_t index);
	Values _values{};
};

struct XeenPartyState {
	// Only explicit encounter preparation installs this; ordinary loading/restoration does not.
	std::optional<XeenGameplayContext> encounterContext;
	XeenRoster roster;
	XeenParty party;
	XeenCloudsQuestItems questItems;
	XeenCloudsQuestFlags questFlags;
	std::uint8_t firstSerializedCount = 0;
	std::uint8_t effectiveSerializedCount = 0;
	std::vector<std::string> diagnostics;
};

} // namespace mmodern

#endif
