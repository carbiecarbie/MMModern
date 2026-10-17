// Adapted from the ScummVM developers' TavernLocation::doOptions and
// BankLocation::depositWithdrawl, GPL-3.0-or-later,
// revision 6814ee9ba54582f5b5adcffab49efbbd8f589edd.
#include "games/xeen/XeenFoodBank.h"
#include <limits>
#include <stdexcept>

namespace mmodern {
XeenFoodResult xeenBuyFood(std::size_t activeMembers, std::uint16_t food,
        std::uint32_t gold, bool yes) {
    XeenFoodResult result{XeenFoodOutcome::Cancelled, food, gold};
    if (!yes) return result;
    if (activeMembers > std::numeric_limits<std::uint16_t>::max() / 15u)
        throw std::invalid_argument("Food offer exceeds party food storage");
    const auto limit = static_cast<std::uint16_t>(activeMembers * 15u);
    if (food >= limit) result.outcome = XeenFoodOutcome::Full;
    else if (gold < 10u) result.outcome = XeenFoodOutcome::InsufficientGold;
    else {
        result.outcome = XeenFoodOutcome::Purchased;
        result.foodAfter = limit;
        result.goldAfter = gold - 10u;
    }
    return result;
}

std::uint8_t xeenTavernDrinkCondition(std::uint8_t before,unsigned roll) {
    if(roll<1 || roll>100)throw std::invalid_argument("Tavern Drink roll outside 1..100");
    return static_cast<std::uint8_t>(before+(roll<26));
}
std::size_t xeenTavernTipText(XeenMapIdentity map,std::uint8_t index) {
    if(map.side==XeenSide::Darkside)return (map.number==29?30u:map.number==31?40u:50u)+index;
    switch(static_cast<unsigned>(map.number)) {
    case 28:return 30u+index;case 30:return 45u+index;case 49:return 60u+index;
    default:throw std::invalid_argument("Tavern tips are not known for this map");
    }
}
std::uint8_t xeenTavernNextTip(XeenMapIdentity map,std::uint8_t index) noexcept {
    return static_cast<std::uint8_t>(static_cast<std::uint8_t>(index+1)%(map.side==XeenSide::Darkside?10:15));
}
std::size_t xeenTavernRumorText(XeenMapIdentity map,unsigned day) {
    if(map.side==XeenSide::Darkside)
        return (map.number==29?0u:map.number==31?10u:20u)+day%10;
    switch(static_cast<unsigned>(map.number)) {
    case 28:return day%10;case 30:return 10u+day%10;case 49:return 20u+day%10;
    default:throw std::invalid_argument("Tavern rumors are not known for this map");
    }
}
XeenBankResult xeenTransferBank(std::uint32_t source, std::uint32_t destination,
        std::uint64_t amount) noexcept {
    XeenBankResult result{XeenBankOutcome::NoTransfer, source, destination};
    if (!amount) return result;
    if (!source) result.outcome = XeenBankOutcome::EmptySource;
    else if (amount > source) result.outcome = XeenBankOutcome::Excess;
    else {
        result.outcome = XeenBankOutcome::Transferred;
        result.sourceAfter = source - static_cast<std::uint32_t>(amount);
        result.destinationAfter = static_cast<std::uint32_t>(std::uint64_t(destination) + amount);
    }
    return result;
}

std::uint64_t xeenBankAmount(std::string_view digits) {
    if (digits.size() > 10u)
        throw std::invalid_argument("Bank amount exceeds ten digits");
    std::uint64_t amount = 0;
    for (const char digit : digits) {
        if (digit < '0' || digit > '9')
            throw std::invalid_argument("Bank amount requires decimal digits");
        amount = amount * 10u + static_cast<unsigned>(digit - '0');
    }
    // DOSBox-confirmed: amounts beyond uint32 are excess, not wrapped amounts.
    // Retain them intact so xeenTransferBank can issue the original refusal.
    return amount;
}
}
