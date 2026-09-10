#ifndef MMODERN_XEEN_ITEM_TRANSFER_H
#define MMODERN_XEEN_ITEM_TRANSFER_H
#include "games/xeen/XeenItemCatalog.h"
#include "games/xeen/XeenParty.h"

namespace mmodern {
// Category access is checked before any slot access. Null means invalid category.
XeenItemCategory *xeenInventoryItems(XeenCharacter &, XeenInventoryCategory) noexcept;
const XeenItemCategory *xeenInventoryItems(const XeenCharacter &, XeenInventoryCategory) noexcept;
bool xeenSameItem(XeenItem, XeenItem) noexcept;

enum class XeenTransferStatus {
	Success, EmptyParty, InvalidParticipant, InvalidOwner, SameOwner,
	InvalidCategory, InvalidSlot, EmptySource, StaleSelection, Cursed,
	DestinationFull, UnsafeRules
};
struct XeenTransferResult {
	XeenTransferStatus status = XeenTransferStatus::StaleSelection;
	std::uint8_t sourceOwner = 0, destinationOwner = 0, destinationSlot = 0;
	XeenItem item{};
};
// Synchronous preflight/publication only; the caller owns modal token validation.
// Preparation/allocation failures propagate before publication.
XeenTransferResult xeenTransferItem(XeenPartyState &, std::size_t source,
	std::size_t destination, XeenInventoryCategory, std::size_t slot);
const char *xeenTransferMessage(XeenTransferStatus) noexcept;
}
#endif
