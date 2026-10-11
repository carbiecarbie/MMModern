#ifndef MMODERN_XEEN_FOOD_BANK_H
#define MMODERN_XEEN_FOOD_BANK_H

#include <cstddef>
#include <cstdint>
#include <string_view>

namespace mmodern {
enum class XeenFoodOutcome { Purchased, Full, InsufficientGold, Cancelled };
struct XeenFoodResult {
    XeenFoodOutcome outcome;
    std::uint16_t foodAfter;
    std::uint32_t goldAfter;
};
// Vertigo's offer is a flat price for filling the party-wide food counter.
XeenFoodResult xeenBuyFood(std::size_t activeMembers, std::uint16_t food,
    std::uint32_t gold, bool yes);

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
