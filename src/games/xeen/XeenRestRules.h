#ifndef MMODERN_XEEN_REST_RULES_H
#define MMODERN_XEEN_REST_RULES_H
#include "games/xeen/XeenCombatRules.h"
namespace mmodern {
bool xeenRestDanger(const XeenConsequenceCharacters &,const XeenConsequenceInputs &,unsigned year);
bool xeenRestRangedWake(const XeenConsequenceCharacters &);
// Interface::rest recovery suffix; time and dream are separate visible steps.
struct XeenRestRecovery {
 XeenConsequenceCharacters characters;
 XeenConsequenceInputs inputs;
 XeenGameplayContext context;
 std::uint16_t food;
 unsigned consumed=0;
 bool starving=false;
 XeenRestRecovery(const XeenConsequenceCharacters &,const XeenConsequenceInputs &,
  const XeenGameplayContext &,std::uint16_t);
};
}
#endif
