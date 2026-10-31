// Adapted from ScummVM TempleLocation::createLocationText and doOptions, GPL-3.0-or-later,
// revision 6814ee9ba54582f5b5adcffab49efbbd8f589edd.
#include "games/xeen/XeenTemplePriceState.h"
#include "games/xeen/XeenJourneyRules.h"
#include <algorithm>
#include <limits>
#include <stdexcept>
namespace mmodern {
XeenTempleSurcharges XeenTemplePriceState::prepare(XeenMapIdentity map) const {
    if(map.side==XeenSide::Clouds && map.number!=28 && map.number!=30 && map.number!=49 && map.number!=73)
        return _row;
    return xeenTempleSurcharges(map);
}
void XeenTemplePriceState::checkPublication(std::uint64_t base) const {
    if(base!=_revision)throw std::logic_error("Temple price preparation is stale");
    if(_revision==std::numeric_limits<std::uint64_t>::max())
        throw std::overflow_error("Temple price revision exhausted");
}
void XeenTemplePriceState::checkVisit(const XeenTempleSurcharges &row,std::uint64_t revision) const {
    if(revision!=_revision || !(row==_row))throw std::logic_error("Temple visit prices changed");
}
XeenTempleSurcharges xeenTempleSurcharges(XeenMapIdentity temple) {
    // DOS XEEN.DAT 0x1c9f5..0x1cab3. Clouds maps go through the switch at
    // 0x1ca30 (table 0x1ce87); Darkside maps compare 29/31/37 and fall back to
    // the most expensive temple. Matches ScummVM createLocationText.
    const XeenTempleSurcharges vertigo{0,0,0,0,10},low{10,50,500,100,25},
        middle{20,100,1000,200,50},high{100,500,5000,300,100};
    if(temple.side==XeenSide::Darkside)
        return temple.number==29?vertigo:temple.number==31?low:temple.number==37?middle:high;
    if(temple.side==XeenSide::Clouds)switch(static_cast<std::uint16_t>(temple.number)) {
        case 28:return vertigo;
        case 30:return low;
        case 49:return high;
        case 73:return middle;
    }
    throw std::invalid_argument("Temple surcharges are not known for this map");
}
bool xeenTempleDonationBlessing(XeenGameplayContext &context,std::uint8_t &counter) {
    // DOS 0x1aee4..1af28: the counter is local to a visit/character selection.
    counter=static_cast<std::uint8_t>((counter+1)%10);
    if(counter!=context.day%10)return false;
    context.effects[XeenPartyEffect::Clairvoyance]=1;context.lightAndResistances[0]=1;
    const auto amount=static_cast<std::uint8_t>(counter?counter:10);
    for(auto i:{XeenPartyEffect::Bless,XeenPartyEffect::PowerShield,XeenPartyEffect::HolyBonus,XeenPartyEffect::Heroism})context.effects[i]=amount;
    return true;
}
std::uint32_t xeenTempleUncurseCost(const XeenCharacter &c,XeenMapIdentity temple) {
    return xeenTempleUncurseCost(c,xeenTempleSurcharges(temple));
}
std::uint32_t xeenTempleUncurseCost(const XeenCharacter &c,const XeenTempleSurcharges &row) {
    bool cursed=c.conditions[0]!=0;
    for(const auto *items:{&c.weapons,&c.armor,&c.accessories,&c.miscellaneous})
        for(const auto &item:*items)cursed|=(item.state&0x40)!=0;
    return cursed?c.currentLevel()*20+row.uncurse:0;
}
void xeenTempleClearCurse(XeenCharacter &c) noexcept {
    // DOS 0x1afa2..1b01b clears the state bit in all 36 slots, even empty ones.
    c.conditions[0]=0;
    for(auto *items:{&c.weapons,&c.armor,&c.accessories,&c.miscellaneous})
        for(auto &item:*items)item.state&=0xbf;
}
XeenTempleHealResult xeenQuoteTempleHeal(const XeenCharacter &c,std::uint32_t gold,
        const XeenGameplayContext &context,XeenMapIdentity temple) {
    return xeenQuoteTempleHeal(c,gold,context,xeenTempleSurcharges(temple));
}
XeenTempleHealResult xeenQuoteTempleHeal(const XeenCharacter &c,std::uint32_t gold,
        const XeenGameplayContext &context,const XeenTempleSurcharges &surcharge) {
    XeenCharacterRules::validateForUse(c,{context.year});
    XeenTempleHealResult r;
    r.owner=c.rosterId;r.goldBefore=r.goldAfter=gold;
    r.hpBefore=r.hpAfter=c.currentHp;r.spBefore=r.spAfter=c.currentSp;
    r.maxHpBefore=r.maxHpAfter=XeenCharacterRules::maxHp(c,{context.year});
    // DOS XEEN.DAT 0x1c9ce..0x1cd3e; totals are 32-bit there.
    const std::int64_t level=std::max<std::int64_t>(std::int64_t(c.permanentLevel)+c.temporaryLevel,0);
    std::int64_t price=0;
    // 0x1cadb compares current and maximum HP unsigned (jbe), so negative HP
    // pays nothing for this term. ScummVM compares signed; DOS is followed.
    if(static_cast<std::uint16_t>(c.currentHp)<static_cast<std::uint16_t>(r.maxHpBefore))
        price+=10*level+surcharge.heal;
    // 0x1cb08..0x1cb40: Heart Broken through Unconscious, 10*L each.
    for(unsigned condition=1;condition<=12;++condition)if(c.conditions[condition])price+=10*level;
    // Dead 0x1cb4e and Stoned 0x1cb93 use the same formula and surcharge.
    for(unsigned condition:{13u,14u})if(c.conditions[condition])
        price+=100*level+50*std::int64_t(c.conditions[condition])+surcharge.resurrect;
    // 0x1cbd8 multiplies the Eradicated counter by 500 in 16 bits and
    // sign-extends it; counters above 65 wrap (unreachable in ordinary play).
    if(c.conditions[15])price+=1000*level+
        static_cast<std::int16_t>(static_cast<std::uint16_t>(500u*c.conditions[15]))+surcharge.eradicate;
    if(price<0 || price>std::numeric_limits<std::uint32_t>::max())
        throw std::invalid_argument("Temple Heal price is outside carried-gold range");
    r.price=static_cast<std::uint32_t>(price);
    r.outcome=!r.price?XeenTempleHealOutcome::NoCharge:
        gold<r.price?XeenTempleHealOutcome::InsufficientGold:XeenTempleHealOutcome::Quoted;
    return r;
}
XeenTempleHealCandidate xeenPrepareTempleHeal(const XeenPartyState &party,std::uint8_t owner,
        const XeenGameplayContext &context,XeenMapIdentity temple) {

    return xeenPrepareTempleHeal(party,owner,context,xeenTempleSurcharges(temple));
}
XeenTempleHealCandidate xeenPrepareTempleHeal(const XeenPartyState &party,std::uint8_t owner,
        const XeenGameplayContext &context,const XeenTempleSurcharges &surcharge) {
    xeenValidateJourneyParty(party);
    if(owner>=30 || !party.roster.combatInputs(owner) || !party.purse)
        throw std::invalid_argument("Missing Temple Heal owner");
    bool active=false;
    for(auto id:party.party.activeRosterIds())if(id==owner)active=true;
    if(!active)throw std::invalid_argument("Temple Heal owner is inactive");
    XeenTempleHealCandidate out;
    out.character=party.roster.at(owner);out.inputs=*party.roster.combatInputs(owner);
    auto &r=out.result;
    r=xeenQuoteTempleHeal(out.character,party.purse->gold,context,surcharge);
    if(r.outcome!=XeenTempleHealOutcome::Quoted)return out;
    auto &c=out.character;auto &i=out.inputs;
    c.intellect.temporary=0;c.personality.temporary=0;c.endurance.temporary=0;
    c.temporaryLevel=0;
    i.might.temporary=0;i.speed.temporary=0;i.accuracy.temporary=0;i.temporaryAc=0;
    i.luck->temporary=0;i.resistances->coldTemporary=0;i.resistances->electricalTemporary=0;
    i.resistances->fireTemporary=0;i.resistances->energyTemporary=0;i.resistances->magicTemporary=0;
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
