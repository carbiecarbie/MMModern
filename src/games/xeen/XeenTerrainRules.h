#ifndef MMODERN_XEEN_TERRAIN_RULES_H
#define MMODERN_XEEN_TERRAIN_RULES_H
#include "games/xeen/XeenCombatRules.h"
#include "games/xeen/XeenTerrainArrival.h"
namespace mmodern {
// DOS doStepCode 0x0c924..0x0ca48. IDs are resolved surface types,
// independent of the different indoor/outdoor cell-slot encodings.
XeenTerrainArrival xeenTerrainArrival(unsigned surface,bool outdoor,bool navigator,
 bool levitating,const XeenCamera &);
XeenCamera xeenTerrainFallDestination(const XeenCamera &);
// Retained consequences owned by the caller's existing action publication.
// DOS giveCharDamage restores the initial damage for each member (0x23de0),
// so each member gets a fresh single-target typed-damage candidate.
struct XeenTerrainArrivalCandidate {
 XeenTerrainArrival rule;
 XeenConsequenceCharacters characters;
 XeenConsequenceInputs inputs;
 XeenGameplayContext context;
 std::optional<XeenServiceEconomy> economy;
 bool needsRest=false, resetTemps=false;
 XeenTerrainArrivalCandidate(const XeenTerrainArrival &,const XeenConsequenceCharacters &,
  const XeenConsequenceInputs &,const XeenGameplayContext &,const XeenServiceEconomy *);
 bool service(XeenConsequenceDraw &);
private:
 std::optional<XeenConditionTimeCandidate> time;
 std::optional<XeenTypedDamageCandidate> injury;
 unsigned member=0;
 bool timeDone=false;
};
}
#endif
