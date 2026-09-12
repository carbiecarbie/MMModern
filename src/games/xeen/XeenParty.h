#ifndef MMODERN_GAMES_XEEN_XEEN_PARTY_H
#define MMODERN_GAMES_XEEN_XEEN_PARTY_H

#include "games/xeen/XeenCharacter.h"
#include "games/xeen/XeenGameplayContext.h"
#include "games/xeen/XeenCombatInputs.h"
#include "games/xeen/XeenOwnerIdentity.h"
#include "games/xeen/XeenGameplayBorrow.h"

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
	XeenRoster() = default;
	XeenRoster(const XeenRoster &);
	XeenRoster(XeenRoster &&);
	XeenRoster &operator=(const XeenRoster &);
	XeenRoster &operator=(XeenRoster &&);
	void swap(XeenRoster &);
	friend void swap(XeenRoster &a, XeenRoster &b) { a.swap(b); }
	bool combatMarked() const noexcept { return _combatMarked; }
	const std::optional<XeenCombatInputs> &combatInputs(std::size_t owner) const { return _combatInputs.at(owner); }

	const XeenCharacter &at(std::size_t rosterId) const;
	XeenCharacter &at(std::size_t rosterId);
	const std::array<XeenCharacter, kCharacterCount> &characters() const { return _characters; }

private:
	friend class XeenCombat;
	friend class XeenActorApproach;
	friend class XeenSaveState;
	friend class XeenRestoreGuard;
	friend class XeenWorld;
	XeenGameplayBorrowOwner _gameplayBorrow;
	friend struct XeenPartyState;
	const std::uint64_t _incarnation = xeenNextOwnerIdentity();
	std::uint64_t _replacement = 0;
	static void requireOrdinary(const XeenRoster &);
	void swapOrdinary(XeenRoster &) noexcept;
	bool _combatMarked = false;
	std::array<std::optional<XeenCombatInputs>, kCharacterCount> _combatInputs{};
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
	XeenPartyState() = default;
	XeenPartyState(const XeenPartyState &);
	XeenPartyState(XeenPartyState &&);
	XeenPartyState &operator=(const XeenPartyState &);
	XeenPartyState &operator=(XeenPartyState &&);
	void swap(XeenPartyState &);
	friend void swap(XeenPartyState &a, XeenPartyState &b) { a.swap(b); }
	// Only explicit encounter preparation installs this; ordinary loading/restoration does not.
	std::optional<XeenGameplayContext> encounterContext;
	XeenRoster roster;
	XeenParty party;
	XeenCloudsQuestItems questItems;
	XeenCloudsQuestFlags questFlags;
	std::uint8_t firstSerializedCount = 0;
	std::uint8_t effectiveSerializedCount = 0;
	std::vector<std::string> diagnostics;
private:
	friend class XeenSaveState;
	friend class XeenRestoreGuard;
	friend class XeenWorld;
	XeenGameplayBorrowOwner _gameplayBorrow;
	const std::uint64_t _incarnation = xeenNextOwnerIdentity();
	std::uint64_t _replacement = 0;
	void swapOrdinary(XeenPartyState &) noexcept;
	// SaveState alone checks fresh destination and an unpublished completed candidate.
	void publishCompleted(XeenPartyState &) noexcept;
};

} // namespace mmodern

// Explicit std::swap must perform the same two-owner preflight as ADL swap.
namespace std {
template<> inline void swap(mmodern::XeenRoster &a, mmodern::XeenRoster &b) { a.swap(b); }
template<> inline void swap(mmodern::XeenPartyState &a, mmodern::XeenPartyState &b) { a.swap(b); }
}

#endif
