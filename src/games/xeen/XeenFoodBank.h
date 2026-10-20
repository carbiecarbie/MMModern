#ifndef MMODERN_XEEN_FOOD_BANK_H
#define MMODERN_XEEN_FOOD_BANK_H

#include <cstddef>
#include <cstdint>
#include <string_view>
#include <optional>
#include "games/xeen/XeenMapIdentity.h"
#include "games/xeen/XeenCharacter.h"

namespace mmodern {
enum class XeenSimpleServiceOutcome { Done, NoAction, InsufficientGold, HaveDrink, Drunk, SupportLimit };
enum class XeenFoodOutcome { Purchased, Full, InsufficientGold, Cancelled, SupportLimit };
struct XeenFoodResult {
    XeenFoodOutcome outcome;
    std::uint16_t foodAfter;
    std::uint32_t goldAfter;
};
struct XeenFoodOffer { unsigned perMember, gold, text; };
std::optional<XeenFoodOffer> xeenTavernFoodOffer(XeenMapIdentity) noexcept;
// A flat price fills the party-wide food counter; untabled maps refuse.
XeenFoodResult xeenBuyFood(std::size_t activeMembers, std::uint16_t food,
    std::uint32_t gold, bool yes, XeenMapIdentity map=XeenMapIdentity(28));

// DOS Tavern rules: 0x1b17b..1b192, 0x1b471..1b521 and 0x1b626..1b6ad.
std::uint8_t xeenTavernDrinkCondition(std::uint8_t before,unsigned roll);
std::size_t xeenTavernTipText(XeenMapIdentity,std::uint8_t index);
std::uint8_t xeenTavernNextTip(XeenMapIdentity,std::uint8_t index) noexcept;
std::size_t xeenTavernRumorText(XeenMapIdentity,unsigned day);

enum class XeenBankOutcome { Transferred, NoTransfer, EmptySource, Excess };
struct XeenBankResult {
    XeenBankOutcome outcome;
    std::uint32_t sourceAfter, destinationAfter;
};
// The same arithmetic applies to either currency and either transfer direction.
XeenBankResult xeenTransferBank(std::uint32_t source, std::uint32_t destination,
    std::uint64_t amount) noexcept;
// Empty input is zero. Invalid characters or more than ten digits are rejected.
std::uint64_t xeenBankAmount(std::string_view digits);
}
#endif
