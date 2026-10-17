#include "games/xeen/XeenFoodBank.h"
#include <iostream>
#include <limits>
#include <stdexcept>
#include <string_view>

using namespace mmodern;
namespace {
void check(bool value, const char *message) {
    if (!value) throw std::runtime_error(message);
}
void foodResult(const XeenFoodResult &result, XeenFoodOutcome outcome,
        std::uint16_t food, std::uint32_t gold) {
    check(result.outcome == outcome && result.foodAfter == food && result.goldAfter == gold,
        "Food outcome or balances differ");
}
void bankResult(const XeenBankResult &result, XeenBankOutcome outcome,
        std::uint32_t source, std::uint32_t destination) {
    check(result.outcome == outcome && result.sourceAfter == source && result.destinationAfter == destination,
        "Bank outcome or balances differ");
}
}
int main() {
    try {
        // DOS Drink succeeds on 1..25 of 1..100; its unsigned condition byte wraps.
        for(unsigned roll=1;roll<=100;++roll)
            check(xeenTavernDrinkCondition(8,roll)==(roll<26?9:8),"Drink threshold differs");
        check(xeenTavernDrinkCondition(255,25)==0 && xeenTavernDrinkCondition(255,26)==255,"Drink byte wrap differs");
        const XeenMapIdentity vertigo(28),nightshadow(30),rivercity(49),dark(XeenSide::Darkside,31);
        check(xeenTavernTipText(vertigo,14)==44 && xeenTavernTipText(nightshadow,0)==45 &&
            xeenTavernTipText(rivercity,14)==74 && xeenTavernTipText(dark,9)==49,"Tip text ranges differ");
        check(xeenTavernNextTip(vertigo,13)==14 && xeenTavernNextTip(vertigo,14)==0 &&
            xeenTavernNextTip(dark,9)==0 && xeenTavernNextTip(vertigo,255)==0,"Tip modulo/byte wrap differs");
        check(xeenTavernRumorText(vertigo,18)==8 && xeenTavernRumorText(nightshadow,18)==18 &&
            xeenTavernRumorText(rivercity,18)==28 && xeenTavernRumorText(dark,118)==18,"Rumor day selection differs");
        bool unknownTavern=false;
        try {static_cast<void>(xeenTavernRumorText(45,8));}catch(const std::invalid_argument &){unknownTavern=true;}
        check(unknownTavern,"Untabled Clouds Tavern must not invent a rumor base");
        constexpr auto maximum = std::numeric_limits<std::uint32_t>::max();
        for (std::size_t members = 1; members <= 6; ++members) {
            const auto limit = static_cast<std::uint16_t>(15u * members);
            for (std::uint16_t food : {std::uint16_t(0), std::uint16_t(limit - 1)}) {
                foodResult(xeenBuyFood(members, food, 10, true), XeenFoodOutcome::Purchased, limit, 0);
                foodResult(xeenBuyFood(members, food, maximum, true), XeenFoodOutcome::Purchased, limit, maximum - 10);
                foodResult(xeenBuyFood(members, food, 9, true), XeenFoodOutcome::InsufficientGold, food, 9);
                foodResult(xeenBuyFood(members, food, 10, false), XeenFoodOutcome::Cancelled, food, 10);
            }
            for (std::uint16_t food : {limit, std::uint16_t(limit + 1), std::uint16_t(65535)})
                for (std::uint32_t gold : {0u, 9u, 10u, maximum}) {
                    foodResult(xeenBuyFood(members, food, gold, true), XeenFoodOutcome::Full, food, gold);
                    foodResult(xeenBuyFood(members, food, gold, false), XeenFoodOutcome::Cancelled, food, gold);
                }
        }
        foodResult(xeenBuyFood(0, 0, 10, true), XeenFoodOutcome::Full, 0, 10);
        // Party sizes outside the food storage domain cannot silently truncate.
        bool invalidSize = false;
        try { xeenBuyFood(std::numeric_limits<std::size_t>::max(), 0, 10, true); }
        catch (const std::invalid_argument &) { invalidSize = true; }
        check(invalidSize, "Food limit overflow admitted");

        bankResult(xeenTransferBank(0, 8, 0), XeenBankOutcome::NoTransfer, 0, 8);
        bankResult(xeenTransferBank(9, 8, 0), XeenBankOutcome::NoTransfer, 9, 8);
        bankResult(xeenTransferBank(0, 8, 1), XeenBankOutcome::EmptySource, 0, 8);
        bankResult(xeenTransferBank(9, 8, 10), XeenBankOutcome::Excess, 9, 8);
        bankResult(xeenTransferBank(9, 8, 9), XeenBankOutcome::Transferred, 0, 17);
        bankResult(xeenTransferBank(9, 8, 4), XeenBankOutcome::Transferred, 5, 12);
        bankResult(xeenTransferBank(maximum, 0, maximum), XeenBankOutcome::Transferred, 0, maximum);
        bankResult(xeenTransferBank(1, maximum, 1), XeenBankOutcome::Transferred, 0, 0);
        bankResult(xeenTransferBank(maximum, maximum, maximum), XeenBankOutcome::Transferred, 0, maximum - 1);
        for (const auto amount : {4294967296ull, 4294967306ull, 9999999999ull,
                std::numeric_limits<unsigned long long>::max()})
            bankResult(xeenTransferBank(maximum, 17, amount), XeenBankOutcome::Excess, maximum, 17);

        check(xeenBankAmount("") == 0 && xeenBankAmount("0000000000") == 0 &&
            xeenBankAmount("0000000042") == 42 && xeenBankAmount("4294967295") == maximum &&
            xeenBankAmount("4294967296") == 4294967296ull &&
            xeenBankAmount("4294967306") == 4294967306ull &&
            xeenBankAmount("9999999999") == 9999999999ull, "Bank decimal parsing differs");
        for (std::string_view text : {"-1", "+1", " 1", "1 ", "1a", "1.0", "1\n", "00000000000"}) {
            bool refused = false;
            try { xeenBankAmount(text); }
            catch (const std::invalid_argument &) { refused = true; }
            check(refused, "Invalid bank amount admitted");
        }
        bankResult(xeenTransferBank(100, 7, xeenBankAmount("4294967296")), XeenBankOutcome::Excess, 100, 7);
        bankResult(xeenTransferBank(100, 7, xeenBankAmount("4294967306")), XeenBankOutcome::Excess, 100, 7);
        std::cout << "Food purchase and bank amount/arithmetic rules passed\n";
        return 0;
    } catch (const std::exception &error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
