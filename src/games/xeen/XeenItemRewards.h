#ifndef MMODERN_XEEN_ITEM_REWARDS_H
#define MMODERN_XEEN_ITEM_REWARDS_H
#include "games/xeen/XeenParty.h"

namespace mmodern {
XeenItemCategory &xeenRewardItems(XeenCharacter &,XeenRewardCategory) noexcept;
bool xeenInsertReward(XeenCharacter &,XeenItem,XeenRewardCategory) noexcept;
enum class XeenRewardLoss { None, EmptyParty, GloballyFull };
enum class XeenRewardDiscard { None, ExecutionError, Abandoned, PresentationFailure };
struct XeenRewardEntry {
	XeenItem item;
	std::optional<std::uint8_t> owner;
	XeenRewardLoss loss = XeenRewardLoss::None;
	XeenRewardCategory category=XeenRewardCategory::Miscellaneous;
	bool quest=false;
};
// One delivery's obligations, prepared from the staging in the original category
// and slot order. Event and monster treasure share it (DOS delivery 0x24128).
struct XeenRewardReceipt {
	std::array<XeenRewardEntry, 4*XeenTreasureStaging::kSlots> entries{};
	std::uint32_t gold=0,gems=0;
	std::size_t count = 0, delivered = 0, lost = 0;
	std::size_t discarded = 0;
	XeenRewardDiscard discardReason = XeenRewardDiscard::None;
};
enum class XeenRewardPhase { Running, Warning, Blank, Found, Receipt, Completed };
// A delivery in progress: Warning is the initial full-pack notice, Blank the
// one-tick hold after an item's transfer, Found its ten-tick found line and
// Receipt the final acknowledgment that credits the currency.
struct XeenTreasureDelivery {
	XeenRewardReceipt receipt;
	XeenRewardPhase phase = XeenRewardPhase::Running;
	std::size_t cursor = 0;
};
bool xeenInsertMiscellaneous(XeenCharacter &character, XeenItem item) noexcept;
bool xeenPacksGloballyFull(const XeenPartyState &party);
bool xeenPacksGloballyFull(XeenCharacter *const *owners, std::size_t count) noexcept;
// Shared DOS delivery. Returns active index; updates item to the delivered bytes.
std::optional<std::size_t> xeenDeliverRewardItem(XeenCharacter *const *owners,
	std::size_t count, XeenItem &item, XeenRewardCategory category,
	std::optional<std::size_t> preferred = {}, bool sort = true, bool quest = false) noexcept;
// Event quest items stage as raw weapon IDs 82..116 (0x22883); their counts are
// granted first, so delivery only names a recipient (0x245f8..0x24610).
bool xeenTreasureQuest(const XeenItem &, XeenRewardCategory) noexcept;
// Preparing a receipt does not transfer items, credit currency or clear the staging.
XeenRewardReceipt xeenPrepareRewardReceipt(const XeenTreasureStaging &) noexcept;
void xeenDeliverRewardEntry(XeenRewardReceipt &,std::size_t,XeenPartyState &,
	std::optional<std::size_t> preferred = {}, bool sort = true);
void xeenSortRewardEntry(const XeenRewardEntry &,XeenPartyState &);
// Starts a delivery: the full-pack warning, or the first item transferred to its
// recipient. Each advance completes one presentation step; Receipt follows the
// last item. The caller credits and empties the staging on its acknowledgment.
void xeenBeginTreasureDelivery(XeenTreasureDelivery &,const XeenTreasureStaging &,XeenPartyState &,
	std::optional<std::size_t> preferred = {});
void xeenAdvanceTreasureDelivery(XeenTreasureDelivery &,XeenPartyState &,
	std::optional<std::size_t> preferred = {});
// Counts the staged Event items an abandoned run never delivered.
void xeenDiscardRewards(XeenRewardReceipt &receipt,const XeenTreasureStaging &staged,
	const XeenTreasureStaging &base,XeenRewardDiscard reason) noexcept;
std::string xeenRewardReceiptText(const XeenRewardReceipt &receipt, const XeenRoster &roster);
std::string xeenInventoryInspection(const XeenPartyState &party);
std::string xeenInventorySummary(const XeenPartyState &party);
}
#endif
