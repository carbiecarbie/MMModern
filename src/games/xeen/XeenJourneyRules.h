#ifndef MMODERN_XEEN_JOURNEY_RULES_H
#define MMODERN_XEEN_JOURNEY_RULES_H
#include "games/xeen/XeenParty.h"
namespace mmodern {
// Side-effect-free predicates over current values, never initial CHR equality.
void xeenValidateJourneyParty(const XeenPartyState &);
void xeenValidateJourneyMelee(const XeenPartyState &);
}
#endif
