#ifndef MMODERN_XEEN_TRAINING_H
#define MMODERN_XEEN_TRAINING_H
#include "games/xeen/XeenParty.h"
#include "games/xeen/XeenCharacterRules.h"
#include <bitset>
namespace mmodern {
enum class XeenTrainingOutcome { Quoted, Trained, Cap, MissingExperience, CannotAct, InsufficientGold, Capacity };
enum class XeenTrainingBoundary { BeforeReservation, StockComplete, BankPrepared, BeforeAdmission, AfterAdmission,
    Quote, BeforeLevel, LevelPublished, BeforeDeparture, DeparturePublished, Return,
    BeforeEventSettlement, AfterEventSettlement };
struct XeenTrainingResult {
    XeenTrainingOutcome outcome=XeenTrainingOutcome::Capacity;
    std::uint8_t owner=0;
    int levelBefore=0,levelAfter=0;
    std::uint32_t xpBefore=0,xpAfter=0,missing=0,cost=0,goldBefore=0,goldAfter=0;
    int maxHpBefore=0,maxSpBefore=0,maxHpAfter=0,maxSpAfter=0;
    std::int16_t hpBefore=0,spBefore=0,hpAfter=0,spAfter=0;
};
// Detached values only; Flow retains all preimages and publication authority.
struct XeenTrainingCandidate {
    XeenTrainingResult result;
    std::array<XeenCharacter,6> characters;
    std::array<XeenCombatInputs,6> inputs;
    unsigned count=0;
};
XeenTrainingResult xeenQuoteTraining(const XeenCharacter &,const XeenCombatInputs &,std::uint32_t gold,
    const XeenGameplayContext &);
XeenTrainingCandidate xeenPrepareTraining(const XeenPartyState &,std::uint8_t owner,const XeenGameplayContext &,
    std::uint16_t content);
void xeenValidateTrainingSource(const std::vector<std::uint8_t> &);
}
#endif
