#include "games/xeen/XeenTraining.h"
#include "games/xeen/XeenJourneyRules.h"
#include "games/xeen/XeenCombatRules.h"
#include <limits>
#include <stdexcept>
namespace mmodern {
unsigned xeenTrainingCap(XeenMapIdentity map) {
    if(map.side==XeenSide::Darkside)return map.number==29?30:map.number==31?50:map.number==37?200:100;
    return map.number==28?10:map.number==30?15:20;
}
void xeenValidateTrainingSource(const std::vector<std::uint8_t> &bytes) {
    if(bytes.size()!=30*XeenCharacter::kSerializedSize)
        throw std::invalid_argument("Training requires thirty complete original characters");
    // Every resistance pair is now a saved live supplement.
}
XeenTrainingResult xeenQuoteTraining(const XeenCharacter &c,const XeenCombatInputs &i,std::uint32_t gold,
        const XeenGameplayContext &context,XeenMapIdentity map) {
    XeenCharacterRules::validateForUse(c,{context.year});
    const int level=c.permanentLevel;
    const unsigned cls=static_cast<unsigned>(c.characterClass);
    if(cls>9 || level<1 || level>255)throw std::invalid_argument("Invalid Training class or permanent level");
    XeenTrainingResult r;r.owner=c.rosterId;r.levelBefore=r.levelAfter=level;
    r.xpBefore=r.xpAfter=i.experience;r.goldBefore=r.goldAfter=gold;
    r.hpBefore=r.hpAfter=c.currentHp;r.spBefore=r.spAfter=c.currentSp;
    r.maxHpBefore=r.maxHpAfter=XeenCharacterRules::maxHp(c,{context.year});
    r.maxSpBefore=r.maxSpAfter=XeenCharacterRules::maxSp(c,{context.year});
    if(level>=xeenTrainingCap(map)){r.outcome=XeenTrainingOutcome::Cap;return r;}
    // DAT 0x1c8d1: the sheet's experience-to-next-level helper.
    r.missing=XeenCharacterRules::experienceToNextLevel(c,&i);r.cost=10u*level*level;
    r.outcome=r.missing?XeenTrainingOutcome::MissingExperience:!c.canAct()?XeenTrainingOutcome::CannotAct:
        gold<r.cost?XeenTrainingOutcome::InsufficientGold:XeenTrainingOutcome::Quoted;
    return r;
}
XeenTrainingCandidate xeenPrepareTraining(const XeenPartyState &party,std::uint8_t owner,const XeenGameplayContext &context,XeenMapIdentity map) {

    xeenValidateJourneyParty(party);
    if(owner>=30 || !party.roster.combatInputs(owner) || !party.purse)
        throw std::invalid_argument("Missing Training owner");
    XeenTrainingCandidate candidate;candidate.context=context;
    auto &r=candidate.result;
    r=xeenQuoteTraining(party.roster.at(owner),*party.roster.combatInputs(owner),party.purse->gold,context,map);
    if(r.outcome==XeenTrainingOutcome::Quoted)xeenResetPartyTemps(candidate.context);
    std::bitset<30> seen;
    for(auto id:party.party.activeRosterIds()) {
        if(id>=30)throw std::invalid_argument("Invalid Training reset population");
        if(seen.test(id))continue;
        seen.set(id);
        if(candidate.count==6)throw std::invalid_argument("Training reset population exceeds active capacity");
        auto &c=candidate.characters[candidate.count];c=party.roster.at(id);
        auto &i=candidate.inputs[candidate.count++];i=*party.roster.combatInputs(id);
        if(r.outcome!=XeenTrainingOutcome::Quoted)continue;
        xeenResetCharacterTemps(c,i);
        if(id==owner) {
            // DAT 0x1ac05..0x1ac36: debit the next threshold minus the current one.
            const auto debit=XeenCharacterRules::nextExperienceLevel(c)-(XeenCharacterRules::currentExperience(c,&i)-std::uint32_t(i.experience));
            if(r.xpBefore<debit)throw std::logic_error("Eligible Training XP cannot cover its debit");
            i.experience=r.xpAfter=r.xpBefore-debit;c.permanentLevel=r.levelAfter=r.levelBefore+1;
        }
        XeenCharacterRules::validateForUse(c,{context.year});
        for(auto attr:{XeenCharacterRules::PhysicalAttribute::Might,XeenCharacterRules::PhysicalAttribute::Speed,
                XeenCharacterRules::PhysicalAttribute::Accuracy})
            (void)XeenCharacterRules::effectivePhysical(c,i,attr,{context.year});
        (void)XeenCharacterRules::combatArmorClass(c,i,{context.year,context.effects[XeenPartyEffect::Bless]});
        (void)xeenCombatAttackCount(c.characterClass,c.currentLevel());
        if(id==owner) {
            r.maxHpAfter=XeenCharacterRules::maxHp(c,{context.year});r.maxSpAfter=XeenCharacterRules::maxSp(c,{context.year});
            if(r.maxHpAfter<0 || r.maxHpAfter>std::numeric_limits<std::int16_t>::max() ||
                r.maxSpAfter<0 || r.maxSpAfter>std::numeric_limits<std::int16_t>::max())
                throw std::invalid_argument("Training refill exceeds signed current-stat storage");
            c.currentHp=r.hpAfter=static_cast<std::int16_t>(r.maxHpAfter);
            c.currentSp=r.spAfter=static_cast<std::int16_t>(r.maxSpAfter);
        }
    }
    if(!seen.test(owner))throw std::invalid_argument("Training selected owner is inactive");
    if(r.outcome==XeenTrainingOutcome::Quoted){r.goldAfter=r.goldBefore-r.cost;r.outcome=XeenTrainingOutcome::Trained;}
    return candidate;
}
}
