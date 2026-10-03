#ifndef MMODERN_XEEN_TEMPLE_HEAL_H
#define MMODERN_XEEN_TEMPLE_HEAL_H
#include "games/xeen/XeenParty.h"
#include "games/xeen/XeenCharacterRules.h"
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
XeenTempleHealResult xeenQuoteTempleHeal(const XeenCharacter &,std::uint32_t gold,
    const XeenGameplayContext &);
XeenTempleHealCandidate xeenPrepareTempleHeal(const XeenPartyState &,std::uint8_t owner,
    const XeenGameplayContext &);
}
#endif
