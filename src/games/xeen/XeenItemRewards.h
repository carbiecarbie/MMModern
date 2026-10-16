#ifndef MMODERN_XEEN_ITEM_REWARDS_H
#define MMODERN_XEEN_ITEM_REWARDS_H
#include "games/xeen/XeenParty.h"

namespace mmodern {
enum class XeenRewardCategory : std::uint8_t { Weapons, Armor, Accessories, Miscellaneous };
XeenItemCategory &xeenRewardItems(XeenCharacter &,XeenRewardCategory) noexcept;
bool xeenInsertReward(XeenCharacter &,XeenItem,XeenRewardCategory) noexcept;
enum class XeenRewardEnqueue { Accepted, InvalidEmpty, IgnoredOverflow };
// Typed category records accept every opaque nonzero ID, independent of
// the narrower future opcode domain. Empty input never occupies a queue slot.
class XeenPendingRewards {
public:
	static constexpr std::size_t kCapacity = 10;
	static constexpr std::size_t kTotalCapacity = 4*kCapacity;
	XeenRewardEnqueue enqueue(XeenItem item) noexcept;
	XeenRewardEnqueue enqueue(XeenItem,XeenRewardCategory) noexcept;
	XeenRewardEnqueue enqueueQuest(std::uint8_t id) noexcept {
		const auto result=enqueue({0,id,0,0},XeenRewardCategory::Weapons);
		if(result==XeenRewardEnqueue::Accepted)_quests[_size-1]=true;
		return result;
	}
	void credit(std::uint8_t mode,std::uint32_t value) noexcept {if(mode==34)_gold+=value;else _gems+=value;}
	std::uint32_t gold() const noexcept {return _gold;}
	std::uint32_t gems() const noexcept {return _gems;}
	std::size_t size() const { return _size; }
	std::size_t count(XeenRewardCategory category) const { return _counts.at(static_cast<unsigned>(category)); }
	std::size_t overflow() const { return _overflow; }
	std::size_t invalid() const { return _invalid; }
	bool hasWork() const { return _size || _overflow || _invalid || _gold || _gems; }
	const XeenItem &at(std::size_t i) const { return _items.at(i); }
	XeenRewardCategory categoryAt(std::size_t i) const {return _categories.at(i);}
	bool questAt(std::size_t i) const {return _quests.at(i);}
private:
	std::array<XeenItem, kTotalCapacity> _items{};
	std::array<XeenRewardCategory,kTotalCapacity> _categories{};
	std::array<bool,kTotalCapacity> _quests{};
	std::array<std::size_t,4> _counts{};
	std::uint32_t _gold=0,_gems=0;
	std::size_t _size = 0, _overflow = 0, _invalid = 0;
};
enum class XeenRewardLoss { None, EmptyParty, GloballyFull };
enum class XeenRewardDiscard { None, ExecutionError, Abandoned, PresentationFailure };
struct XeenRewardEntry {
	XeenItem item;
	std::optional<std::uint8_t> owner;
	XeenRewardLoss loss = XeenRewardLoss::None;
	XeenRewardCategory category=XeenRewardCategory::Miscellaneous;
	bool quest=false;
};
struct XeenRewardReceipt {
	std::array<XeenRewardEntry, XeenPendingRewards::kTotalCapacity> entries{};
	std::uint32_t gold=0,gems=0;
	std::size_t count = 0, delivered = 0, lost = 0, overflow = 0, invalid = 0;
	std::size_t discarded = 0;
	XeenRewardDiscard discardReason = XeenRewardDiscard::None;
};
bool xeenInsertMiscellaneous(XeenCharacter &character, XeenItem item) noexcept;
bool xeenPacksGloballyFull(const XeenPartyState &party);
bool xeenPacksGloballyFull(XeenCharacter *const *owners, std::size_t count) noexcept;
// Shared DOS delivery. Returns active index; updates item to the delivered bytes.
std::optional<std::size_t> xeenDeliverRewardItem(XeenCharacter *const *owners,
	std::size_t count, XeenItem &item, XeenRewardCategory category,
	std::optional<std::size_t> preferred = {}, bool sort = true, bool quest = false) noexcept;
// Retained display obligations, ordered by the original category scans. Preparing
// a receipt does not transfer items, credit currency or clear staged treasure.
XeenRewardReceipt xeenPrepareRewardReceipt(const XeenPendingRewards &) noexcept;
void xeenDeliverRewardEntry(XeenRewardReceipt &,std::size_t,XeenPartyState &,
	std::optional<std::size_t> preferred = {}, bool sort = true);
void xeenSortRewardEntry(const XeenRewardEntry &,XeenPartyState &);
void xeenCreditRewardCurrency(XeenPendingRewards &,const XeenRewardReceipt &,XeenPartyState &) noexcept;
// Fixed result storage and no callbacks/formatting in the mutation segment.
XeenRewardReceipt xeenDeliverRewards(XeenPendingRewards &pending,
	XeenPartyState &party, std::optional<std::size_t> preferred);
void xeenDiscardRewards(XeenPendingRewards &pending, XeenRewardReceipt &receipt,
	XeenRewardDiscard reason) noexcept;
std::string xeenRewardReceiptText(const XeenRewardReceipt &receipt, const XeenRoster &roster);
std::string xeenInventoryInspection(const XeenPartyState &party);
std::string xeenInventorySummary(const XeenPartyState &party);
}
#endif
