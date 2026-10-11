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
