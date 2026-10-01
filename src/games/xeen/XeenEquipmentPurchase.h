#ifndef MMODERN_XEEN_EQUIPMENT_PURCHASE_H
#define MMODERN_XEEN_EQUIPMENT_PURCHASE_H
#include "games/xeen/XeenItemTransfer.h"
#include <optional>
#include <type_traits>
namespace mmodern {
enum class XeenEquipmentPurchaseOutcome {
    Quoted, Purchased, Empty, Unsupported, DestinationFull, InsufficientGold,
    InvalidParticipant, InvalidCategory, InvalidSlot, Cancelled
};
// Fixed detached facts only. Recorded operation/reservation numbers confer no
// authority; the Smith Flow binds them to owner incarnations and concrete frames.
struct XeenEquipmentPurchaseResult {
    XeenEquipmentPurchaseOutcome outcome=XeenEquipmentPurchaseOutcome::Unsupported;
    XeenInventoryCategory category=XeenInventoryCategory::Weapons;
    std::uint8_t side=0,shop=0,member=0,owner=0,offerSlot=0,recipientSlot=0;
    std::array<std::uint8_t,6> activeRosterIds{};
    XeenItem offer{};
    XeenItemCategory recipientBefore{},recipientAfter{},stockBefore{},stockAfter{};
    std::uint32_t price=0,goldBefore=0,goldAfter=0,shortfall=0;
    // Flow fills these checked identities before installing a quote/result.
    // Pure preparation grants no operation or reservation identity itself.
    std::uint64_t quotedOperation=0,quotedReservation=0,publishedReservation=0;
};
struct XeenEquipmentPurchaseCandidate {
    XeenEquipmentPurchaseResult result;
    XeenServiceEconomy economyBefore,economyAfter;
};
static_assert(std::is_nothrow_copy_assignable_v<XeenEquipmentPurchaseResult> &&
    std::is_nothrow_copy_constructible_v<XeenEquipmentPurchaseResult> &&
    std::is_nothrow_copy_assignable_v<XeenEquipmentPurchaseCandidate>);
// Plain ordinary equipment only. Displaying a raw record does not admit it.
bool xeenSupportedEquipmentOffer(unsigned side,unsigned shop,XeenInventoryCategory,const XeenItem &) noexcept;
std::optional<std::uint32_t> xeenEquipmentPurchasePrice(XeenInventoryCategory,const XeenItem &) noexcept;
XeenEquipmentPurchaseResult xeenQuoteEquipmentPurchase(const XeenPartyState &,std::size_t member,
    XeenInventoryCategory,std::size_t slot,std::uint16_t content=13,unsigned side=0,unsigned shop=0);
XeenEquipmentPurchaseCandidate xeenPrepareEquipmentPurchase(const XeenPartyState &,std::size_t member,
    XeenInventoryCategory,std::size_t slot,std::uint16_t content=13,unsigned side=0,unsigned shop=0);
// Exact authorized economy delta, independent of snapshot existence proof.
// Neither bank nor any other shop/category/side may change. expected binds the
// selected physical source; an identical item elsewhere is not a substitute.
void xeenValidateEquipmentPurchaseEconomyDelta(const XeenServiceEconomy &before,
    const XeenServiceEconomy &after,XeenInventoryCategory,std::size_t slot,const XeenItem &expected);
}
#endif
