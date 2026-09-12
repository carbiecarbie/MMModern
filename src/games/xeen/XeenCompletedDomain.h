#ifndef MMODERN_XEEN_COMPLETED_DOMAIN_H
#define MMODERN_XEEN_COMPLETED_DOMAIN_H
#include "games/xeen/XeenWorld.h"
namespace mmodern {
void xeenValidateCompletedParty(const XeenPartyState &, const XeenPartyState &,
	const std::array<XeenCombatInputs, 6> &, const XeenMonsterRecord &);
void xeenApplyCompletedOverlay(std::vector<XeenActor> &, XeenMonsterIdentity);
XeenCompletedEncounterAuthority xeenCompletedPreimage(const XeenWorld &,
	const XeenPartyState &, const XeenCamera &);
}
#endif
