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
	void credit(std::uint8_t mode,std::uint32_t value) noexcept {if(mode==34)_gold+=value;else _gems+=value;}
	std::uint32_t gold() const noexcept {return _gold;}
	std::uint32_t gems() const noexcept {return _gems;}
	std::size_t size() const { return _size; }
	std::size_t overflow() const { return _overflow; }
	std::size_t invalid() const { return _invalid; }
	bool hasWork() const { return _size || _overflow || _invalid || _gold || _gems; }
	const XeenItem &at(std::size_t i) const { return _items.at(i); }
	XeenRewardCategory categoryAt(std::size_t i) const {return _categories.at(i);}
private:
	std::array<XeenItem, kTotalCapacity> _items{};
	std::array<XeenRewardCategory,kTotalCapacity> _categories{};
	std::array<std::size_t,4> _counts{};
	std::uint32_t _gold=0,_gems=0;
	std::size_t _size = 0, _overflow = 0, _invalid = 0;
};
enum class XeenRewardLoss { None, EmptyParty, NoEligibleMember, CategoryFull };
enum class XeenRewardDiscard { None, ExecutionError, Abandoned, PresentationFailure };
struct XeenRewardEntry {
	XeenItem item;
	std::optional<std::uint8_t> owner;
	XeenRewardLoss loss = XeenRewardLoss::None;
	XeenRewardCategory category=XeenRewardCategory::Miscellaneous;
};
struct XeenRewardReceipt {
	std::array<XeenRewardEntry, XeenPendingRewards::kTotalCapacity> entries{};
	std::uint32_t gold=0,gems=0;
	bool currencyLost=false;
	std::size_t count = 0, delivered = 0, lost = 0, overflow = 0, invalid = 0;
	std::size_t discarded = 0;
	XeenRewardDiscard discardReason = XeenRewardDiscard::None;
};
bool xeenInsertMiscellaneous(XeenCharacter &character, XeenItem item) noexcept;
bool xeenPacksGloballyFull(const XeenPartyState &party);
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
