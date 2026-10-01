#include "games/xeen/XeenTraining.h"
#include "games/xeen/XeenJourneyRules.h"
#include "games/xeen/XeenCombatRules.h"
#include <limits>
#include <stdexcept>
namespace mmodern {
namespace {
constexpr std::array<std::uint32_t,10> xpBases{1500,2000,2000,1500,2000,1000,1500,1500,1500,2000};
std::uint32_t nextXp(const XeenCharacter &c) {
    return xpBases[static_cast<unsigned>(c.characterClass)]*(std::uint32_t(1)<<(c.permanentLevel-1));
}
}
void xeenValidateTrainingSource(const std::vector<std::uint8_t> &bytes) {
    if(bytes.size()!=30*XeenCharacter::kSerializedSize)
        throw std::invalid_argument("Training requires thirty complete original characters");
    for(unsigned owner=0;owner<30;++owner)
        for(unsigned offset:{312u,320u,322u})
            if(bytes[owner*XeenCharacter::kSerializedSize+offset])
                throw std::invalid_argument("Unsupported omitted temporary resistance in Training source");
}
XeenTrainingResult xeenQuoteTraining(const XeenCharacter &c,const XeenCombatInputs &i,std::uint32_t gold,
        const XeenGameplayContext &context) {
    XeenCharacterRules::validateForUse(c,{context.year});
    const int level=c.permanentLevel;
    const unsigned cls=static_cast<unsigned>(c.characterClass);
    if(cls>9 || level<1 || level>255)throw std::invalid_argument("Invalid Training class or permanent level");
    XeenTrainingResult r;r.owner=c.rosterId;r.levelBefore=r.levelAfter=level;
    r.xpBefore=r.xpAfter=i.experience;r.goldBefore=r.goldAfter=gold;
    r.hpBefore=r.hpAfter=c.currentHp;r.spBefore=r.spAfter=c.currentSp;
    r.maxHpBefore=r.maxHpAfter=XeenCharacterRules::maxHp(c,{context.year});
    r.maxSpBefore=r.maxSpAfter=XeenCharacterRules::maxSp(c,{context.year});
    if(level>=10){r.outcome=XeenTrainingOutcome::Cap;return r;}
    const std::uint32_t next=nextXp(c);
    const std::uint32_t previous=level==1?0:next/2;
    const auto current=static_cast<std::uint32_t>(std::uint64_t(previous)+r.xpBefore);
    r.missing=current>=next?0:next-current;r.cost=10u*level*level;
    r.outcome=r.missing?XeenTrainingOutcome::MissingExperience:!c.canAct()?XeenTrainingOutcome::CannotAct:
        gold<r.cost?XeenTrainingOutcome::InsufficientGold:XeenTrainingOutcome::Quoted;
    return r;
}
XeenTrainingCandidate xeenPrepareTraining(const XeenPartyState &party,std::uint8_t owner,const XeenGameplayContext &context,
        std::uint16_t content) {
    if(content!=12 && content!=13)throw std::invalid_argument("Unsupported Training content");
    xeenValidateJourneyParty(party,content);
    if(owner>=30 || !party.roster.combatInputs(owner) || !party.monsterTreasure)
        throw std::invalid_argument("Missing Training owner");
    XeenTrainingCandidate candidate;
    auto &r=candidate.result;
    r=xeenQuoteTraining(party.roster.at(owner),*party.roster.combatInputs(owner),party.monsterTreasure->gold,context);
    std::bitset<30> seen;
    for(auto id:party.party.activeRosterIds()) {
        if(id>=30)throw std::invalid_argument("Invalid Training reset population");
        if(seen.test(id))continue;
        seen.set(id);
        if(candidate.count==6)throw std::invalid_argument("Training reset population exceeds active capacity");
        auto &c=candidate.characters[candidate.count];c=party.roster.at(id);
        auto &i=candidate.inputs[candidate.count++];i=*party.roster.combatInputs(id);
        if(r.outcome!=XeenTrainingOutcome::Quoted)continue;
        c.temporaryLevel=0;c.intellect.temporary=0;c.personality.temporary=0;c.endurance.temporary=0;
        i.might.temporary=0;i.speed.temporary=0;i.accuracy.temporary=0;i.temporaryAc=0;
        i.luck->temporary=0;i.resistances->coldTemporary=0;i.resistances->electricalTemporary=0;
        i.poisonResistance->temporary=0;
        if(id==owner) {
            const auto next=nextXp(c);
            const auto debit=r.levelBefore==1?next:next/2;
            if(r.xpBefore<debit)throw std::logic_error("Eligible Training XP cannot cover its debit");
            i.experience=r.xpAfter=r.xpBefore-debit;c.permanentLevel=r.levelAfter=r.levelBefore+1;
        }
        XeenCharacterRules::validateForUse(c,{context.year});
        for(auto attr:{XeenCharacterRules::PhysicalAttribute::Might,XeenCharacterRules::PhysicalAttribute::Speed,
                XeenCharacterRules::PhysicalAttribute::Accuracy})
            (void)XeenCharacterRules::effectivePhysical(c,i,attr,{context.year});
        (void)XeenCharacterRules::combatArmorClass(c,i,{context.year});
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
