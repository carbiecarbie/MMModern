#ifndef MMODERN_XEEN_TEMPLE_HEAL_H
#define MMODERN_XEEN_TEMPLE_HEAL_H
#include "games/xeen/XeenParty.h"
#include "games/xeen/XeenCharacterRules.h"
#include "games/xeen/XeenMapIdentity.h"
namespace mmodern {
enum class XeenTempleHealOutcome { Quoted, Healed, NoCharge, InsufficientGold, SupportLimit, HpSupportLimit };
struct XeenTempleHealResult {
    XeenTempleHealOutcome outcome=XeenTempleHealOutcome::NoCharge;
    std::uint8_t owner=0;
    std::uint32_t price=0,goldBefore=0,goldAfter=0;
    std::int16_t hpBefore=0,hpAfter=0,spBefore=0,spAfter=0;
    int maxHpBefore=0,maxHpAtAssignment=0,maxHpAfter=0;
};
struct XeenTempleHealCandidate {
    XeenTempleHealResult result;
    XeenCharacter character;
    XeenCombatInputs inputs;
};
// Per-temple additions selected by map (DOS XEEN.DAT 0x1c9f5..0x1cab3). Only
// heal and resurrect/eradicate enter the Heal price; uncurse and donation are
// recorded for the other Temple options.
struct XeenTempleSurcharges {
    std::int16_t heal=0,resurrect=0,eradicate=0,uncurse=0,donation=0;
};
// Throws for a Clouds map without a temple entry: DOS keeps the previous
// temple's values there, which MMModern does not model.
XeenTempleSurcharges xeenTempleSurcharges(XeenMapIdentity);
// Donation overwrites these spell effects, rather than taking their maximum.
bool xeenTempleDonationBlessing(XeenGameplayContext &,std::uint8_t &counter);
std::uint32_t xeenTempleUncurseCost(const XeenCharacter &,XeenMapIdentity temple=XeenMapIdentity(28));
void xeenTempleClearCurse(XeenCharacter &) noexcept;
XeenTempleHealResult xeenQuoteTempleHeal(const XeenCharacter &,std::uint32_t gold,
    const XeenGameplayContext &,XeenMapIdentity temple);
XeenTempleHealCandidate xeenPrepareTempleHeal(const XeenPartyState &,std::uint8_t owner,
    const XeenGameplayContext &,XeenMapIdentity temple);
}
#endif
