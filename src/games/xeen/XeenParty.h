#ifndef MMODERN_GAMES_XEEN_XEEN_PARTY_H
#define MMODERN_GAMES_XEEN_XEEN_PARTY_H
#include "games/xeen/XeenSoundOptions.h"

#include "games/xeen/XeenCharacter.h"
#include "games/xeen/XeenGameplayContext.h"
#include "games/xeen/XeenCombatInputs.h"
#include "games/xeen/XeenOwnerIdentity.h"
#include "games/xeen/XeenGameplayBorrow.h"
#include "games/xeen/XeenMonsterTreasure.h"
#include "games/xeen/XeenServiceEconomy.h"

#include <array>
#include <cstddef>
#include <cstdint>
#include <optional>
#include <stdexcept>
#include <string>
#include <vector>

namespace mmodern {

// DOS clears the entire CHR, with all supplemental presence retained.
XeenCharacter xeenDeletedCharacter(std::uint8_t owner);
XeenCombatInputs xeenDeletedCombatInputs();

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
	const XeenMutableOptional<XeenCombatInputs> &combatInputs(std::size_t owner) const { return _combatInputs.at(owner); }

	const XeenCharacter &at(std::size_t rosterId) const;
	XeenCharacter &at(std::size_t rosterId);
	const std::array<XeenCharacter, kCharacterCount> &characters() const { return _characters; }

private:
	friend class XeenCombat;
	friend class XeenActorApproach;
	friend class XeenEncounterFlow;
	friend class XeenEventFlow;
	friend class XeenSaveState;
	friend class XeenRestoreGuard;
	friend class XeenWorld;
	friend struct XeenEventValueAccess;
	XeenGameplayBorrowOwner _gameplayBorrow;
	friend struct XeenPartyState;
	const std::uint64_t _incarnation = xeenNextOwnerIdentity();
	std::uint64_t _replacement = 0;
	static void requireOrdinary(const XeenRoster &);
	void swapOrdinary(XeenRoster &) noexcept;
	bool _combatMarked = false;
	std::array<XeenMutableOptional<XeenCombatInputs>, kCharacterCount> _combatInputs{};
	std::array<XeenCharacter, kCharacterCount> _characters{};
};

class XeenParty {
public:
	// Membership and the unsaved Dismiss tail. PartyState owns other party data.
	XeenParty()=default;
	XeenParty(const XeenParty &)=default;
	XeenParty(XeenParty &&other) noexcept : _activeRosterIds(std::move(other._activeRosterIds)), _dismissedRosterIds(std::move(other._dismissedRosterIds)) { XeenMutationWatch::write(&other); }
	XeenParty &operator=(const XeenParty &other) { XeenMutationWatch::write(this);_activeRosterIds=other._activeRosterIds;_dismissedRosterIds=other._dismissedRosterIds;return *this; }
	XeenParty &operator=(XeenParty &&other) noexcept { XeenMutationWatch::write(this);XeenMutationWatch::write(&other);_activeRosterIds=std::move(other._activeRosterIds);_dismissedRosterIds=std::move(other._dismissedRosterIds);return *this; }
	static constexpr std::size_t kSerializedMemberSlots = 8;
	static constexpr std::size_t kMaximumVisibleMembers = 6;
	// Restore exact active order, including the loader's supported duplicates.
	static XeenParty fromRosterIds(std::vector<std::uint8_t> ids);

	const XeenMutableVector<std::uint8_t> &activeRosterIds() const { return _activeRosterIds; }
    const XeenMutableVector<std::uint8_t> &dismissedRosterIds() const {return _dismissedRosterIds;}
    XeenParty dismissed(std::size_t index) const {
        if(size()<=1 || index>=size())throw std::invalid_argument("Invalid Dismiss selection");
        XeenParty next(*this);next._dismissedRosterIds.push_back(next._activeRosterIds.at(index));
        next._activeRosterIds.erase(next._activeRosterIds.begin()+index);return next;
    }
    XeenParty reordered(std::vector<std::uint8_t> ids) const {
        auto next=fromRosterIds(std::move(ids));next._dismissedRosterIds=_dismissedRosterIds;return next;
    }
	std::size_t size() const { return _activeRosterIds.size(); }
	XeenPartyOrder activeOrder() const {

		XeenPartyOrder order(size());for(unsigned n=0;n<size();++n)order[n]=_activeRosterIds[n];return order;
	}
	const XeenCharacter &member(const XeenRoster &roster, std::size_t partyIndex) const;

private:
	friend class XeenPartyLoader;
    friend class XeenSaveState;
    XeenMutableVector<std::uint8_t> _dismissedRosterIds;
	XeenMutableVector<std::uint8_t> _activeRosterIds;
};

// Party-owned Clouds quest counters, independent of character inventories.
class XeenCloudsQuestItems {
public:
	static constexpr std::size_t kCount = 35;
	static constexpr std::int64_t kFirstItemId = 82;
	using Counts = std::array<std::uint32_t, kCount>;

	XeenCloudsQuestItems() = default;
	XeenCloudsQuestItems(const XeenCloudsQuestItems &)=default;
	XeenCloudsQuestItems &operator=(const XeenCloudsQuestItems &other) noexcept { XeenMutationWatch::write(this);_counts=other._counts;return *this; }
	explicit XeenCloudsQuestItems(Counts counts) : _counts(counts) {}
	static std::optional<std::size_t> indexForItemId(std::int64_t itemId);
	std::uint32_t at(std::size_t index) const { return _counts.at(index); }
	// Bounded access; returns false on overflow without changing the counter.
	bool increment(std::size_t index);
	// Bounded access; returns false at zero without changing the counter.
	bool decrement(std::size_t index);
	// Event quest-counter rule: the original counter is one byte, incremented by
	// give 21 (0x22967..0x2296e) and GiveEnchanted (0x289c2..0x289d1) and
	// decremented by take 21 (0x22339..0x22340) without bounds, so 255 + 1 = 0
	// and 0 - 1 = 255. The stored width is unchanged.
	static std::uint32_t afterEventGrant(std::uint32_t count) noexcept { return static_cast<std::uint8_t>(count + 1); }
	static std::uint32_t afterEventTake(std::uint32_t count) noexcept { return static_cast<std::uint8_t>(count - 1); }
	void eventGrant(std::size_t index);
	void eventTake(std::size_t index);
	const XeenMutableArray<std::uint32_t,kCount> &counts() const { return _counts; }

private:
	XeenMutableArray<std::uint32_t,kCount> _counts{};
};

// Clouds request state, separate from transactional game flags and item counts.
class XeenCloudsQuestFlags {
public:
	static constexpr std::size_t kCount = 30;
	using Values = std::array<bool, kCount>;
	XeenCloudsQuestFlags() = default;
	XeenCloudsQuestFlags(const XeenCloudsQuestFlags &)=default;
	XeenCloudsQuestFlags &operator=(const XeenCloudsQuestFlags &other) noexcept { XeenMutationWatch::write(this);_values=other._values;return *this; }
	explicit XeenCloudsQuestFlags(Values values) : _values(values) {}
	static bool validIndex(std::int64_t index);
	bool isSet(std::int64_t index) const;
	void set(std::int64_t index);
	void clear(std::int64_t index);
	const XeenMutableArray<bool,kCount> &values() const { return _values; }
private:
	static std::size_t checkedIndex(std::int64_t index);
	XeenMutableArray<bool,kCount> _values{};
};

// The original bounded world-flag block, shared by every Event use.
struct XeenWorldFlags {
 static constexpr std::size_t kCount = 128;
 XeenMutableArray<bool,kCount> values{};
 friend bool operator==(const XeenWorldFlags &a, const XeenWorldFlags &b) noexcept { return a.values == b.values; }
 friend bool operator!=(const XeenWorldFlags &a, const XeenWorldFlags &b) noexcept { return !(a == b); }
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
	XeenMutableOptional<XeenGameplayContext> encounterContext;
	// Installed with the party by the fresh Journey and restore; saved.
	XeenMutableOptional<XeenPurse> purse;
    XeenMutable<std::uint32_t> playTimeSeconds=0;
    XeenMutable<bool> cloudsCompleted=false;
	// The fresh Journey lifecycle installs merchant stock/shared bank.
	XeenMutableOptional<XeenServiceEconomy> serviceEconomy;
	XeenRoster roster;
	XeenParty party;
	XeenCloudsQuestItems questItems;
	XeenCloudsQuestFlags questFlags;
	XeenMutableOptional<XeenWorldFlags> worldFlags;
	XeenMutable<std::uint8_t> firstSerializedCount = 0;
	XeenMutable<std::uint8_t> effectiveSerializedCount = 0;
	XeenMutableDiagnostics diagnostics;
	XeenMutable<bool> insideInn=false; // Envelope flag; camera is the prior map's tavern door.
	XeenMutable<std::uint16_t> food = 0;
	XeenMutable<XeenSoundOptions> soundOptions;
private:
	friend class XeenPartyLoader;
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
