// Adapted from ScummVM TempleLocation::doOptions, GPL-3.0-or-later,
// revision 6814ee9ba54582f5b5adcffab49efbbd8f589edd.
#include "games/xeen/XeenTempleHeal.h"
#include "games/xeen/XeenJourneyRules.h"
#include <algorithm>
#include <limits>
#include <stdexcept>
namespace mmodern {
XeenTempleHealResult xeenQuoteTempleHeal(const XeenCharacter &c,std::uint32_t gold,
        const XeenGameplayContext &context) {
    XeenCharacterRules::validateForUse(c,{context.year});
    XeenTempleHealResult r;
    r.owner=c.rosterId;r.goldBefore=r.goldAfter=gold;
    r.hpBefore=r.hpAfter=c.currentHp;r.spBefore=r.spAfter=c.currentSp;
    r.maxHpBefore=r.maxHpAfter=XeenCharacterRules::maxHp(c,{context.year});
    const std::int64_t level=std::max<std::int64_t>(std::int64_t(c.permanentLevel)+c.temporaryLevel,0);
    std::int64_t price=(c.currentHp<r.maxHpBefore ? 10*level : 0);
    for(unsigned condition:{3u,4u,8u,12u})if(c.conditions[condition])price+=10*level;
    if(c.conditions[13])price+=100*level+50*std::int64_t(c.conditions[13]);
    if(price<0 || price>std::numeric_limits<std::uint32_t>::max())
        throw std::invalid_argument("Temple Heal price exceeds carried-gold storage");
    r.price=static_cast<std::uint32_t>(price);
    r.outcome=!r.price?XeenTempleHealOutcome::NoCharge:
        gold<r.price?XeenTempleHealOutcome::InsufficientGold:XeenTempleHealOutcome::Quoted;
    return r;
}
XeenTempleHealCandidate xeenPrepareTempleHeal(const XeenPartyState &party,std::uint8_t owner,
        const XeenGameplayContext &context,std::uint16_t content) {
    if(content!=14)throw std::invalid_argument("Temple Heal requires Journey content 14");
    xeenValidateJourneyParty(party,content);
    if(owner>=30 || !party.roster.combatInputs(owner) || !party.monsterTreasure)
        throw std::invalid_argument("Missing Temple Heal owner");
    bool active=false;
    for(auto id:party.party.activeRosterIds())if(id==owner)active=true;
    if(!active)throw std::invalid_argument("Temple Heal owner is inactive");
    XeenTempleHealCandidate out;
    out.character=party.roster.at(owner);out.inputs=*party.roster.combatInputs(owner);
    auto &r=out.result;
    r=xeenQuoteTempleHeal(out.character,party.monsterTreasure->gold,context);
    if(r.outcome!=XeenTempleHealOutcome::Quoted)return out;
    auto &c=out.character;auto &i=out.inputs;
    c.intellect.temporary=0;c.personality.temporary=0;c.endurance.temporary=0;
    c.temporaryLevel=0;
    i.might.temporary=0;i.speed.temporary=0;i.accuracy.temporary=0;i.temporaryAc=0;
    i.luck->temporary=0;i.resistances->coldTemporary=0;i.resistances->electricalTemporary=0;
    i.poisonResistance->temporary=0;
    // The reference assigns HP before clearing conditions. Disease can therefore
    // leave current HP below the final healthy maximum.
    r.maxHpAtAssignment=XeenCharacterRules::maxHp(c,{context.year});
    if(r.maxHpAtAssignment<0 || r.maxHpAtAssignment>std::numeric_limits<std::int16_t>::max())
        throw std::invalid_argument("Temple Heal HP exceeds signed storage");
    c.currentHp=r.hpAfter=static_cast<std::int16_t>(r.maxHpAtAssignment);
    for(unsigned condition=1;condition<=15;++condition)c.conditions[condition]=0;
    r.maxHpAfter=XeenCharacterRules::maxHp(c,{context.year});
    r.goldAfter=r.goldBefore-r.price;r.outcome=XeenTempleHealOutcome::Healed;
    return out;
}
}
