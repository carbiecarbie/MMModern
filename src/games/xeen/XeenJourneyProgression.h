#ifndef MMODERN_XEEN_JOURNEY_PROGRESSION_H
#define MMODERN_XEEN_JOURNEY_PROGRESSION_H
#include "games/xeen/XeenActor.h"
#include "games/xeen/XeenParty.h"
#include "games/xeen/XeenCombatRules.h"
#include <set>
#include <stdexcept>
namespace mmodern {
// Detached lethal candidate. No target selection, owner binding or replay authority.
struct XeenJourneyLethal {
	XeenActor actor;
	std::array<std::uint32_t,6> experience{};
	std::set<XeenMonsterIdentity> accounted;
};
inline XeenJourneyLethal xeenPrepareJourneyLethal(const XeenActor &actor,
		const std::array<const XeenCharacter *,6> &characters, const std::array<XeenCombatInputs,6> &inputs,
		const std::set<XeenMonsterIdentity> &accounted, unsigned participantMask) {
	if (participantMask>0x3f) throw std::invalid_argument("Invalid Journey XP participation mask");
	if (accounted.count(actor.id) || actor.lifecycle != XeenActorLifecycle::Present ||
		actor.status != XeenActorStatus::Physical || !actor.statistics || actor.hp <= 0)
		throw std::invalid_argument("Journey lethal identity is unavailable");
	unsigned eligible = 0;
	for (unsigned i=0;i<6;++i) if ((participantMask&(1u<<i)) && characters[i] && xeenCombatXpEligible(characters[i]->worstCondition())) ++eligible;
	if (!eligible) throw std::invalid_argument("Journey lethal publication has no XP recipient");
	XeenJourneyLethal result{actor,{},accounted};
	for (unsigned i = 0; i < 6; ++i) {
		if (!characters[i]) throw std::invalid_argument("Journey lethal owner is missing");
		result.experience[i] = (participantMask&(1u<<i)) && xeenCombatXpEligible(characters[i]->worstCondition()) ?
			xeenCombatExperience(actor.statistics->experience(),eligible,characters[i]->permanentLevel,inputs[i].experience) : inputs[i].experience;
	}
	result.accounted.insert(actor.id);
	result.actor.hp = 0; result.actor.x = result.actor.y = -128;
	result.actor.activated = false; result.actor.lifecycle = XeenActorLifecycle::Defeated;
	return result;
}
}
#endif
