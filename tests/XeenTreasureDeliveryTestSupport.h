#ifndef MMODERN_TREASURE_DELIVERY_TEST_SUPPORT_H
#define MMODERN_TREASURE_DELIVERY_TEST_SUPPORT_H
#include "games/xeen/XeenItemRewards.h"
namespace mmodern {
// Drives the production delivery (DOS 0x24128) through every presentation step
// without presenting it, then credits the currency as the final acknowledgment does.
inline XeenRewardReceipt xeenTestDeliverTreasure(const XeenTreasureStaging &staged, XeenPartyState &party,
	std::optional<std::size_t> preferred = {}) {
	XeenTreasureDelivery delivery;
	xeenBeginTreasureDelivery(delivery, staged, party, preferred);
	while (delivery.phase != XeenRewardPhase::Receipt) xeenAdvanceTreasureDelivery(delivery, party, preferred);
	if (party.purse) *party.purse = xeenCreditTreasure(*party.purse, staged);
	return delivery.receipt;
}
}
#endif
