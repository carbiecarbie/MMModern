#ifndef MMODERN_XEEN_COMBAT_RULES_H
#define MMODERN_XEEN_COMBAT_RULES_H
#include "games/xeen/XeenCharacter.h"
#include <limits>
#include <stdexcept>
namespace mmodern {
// Pure arithmetic controls, also usable with artificial overflow/predicate inputs.
// They do not grant admission, mutate XP or apply a prepared result.
inline bool xeenCombatXpEligible(XeenCondition c) noexcept {
	return c!=XeenCondition::Dead&&c!=XeenCondition::Stoned&&c!=XeenCondition::Eradicated;
}
inline std::uint32_t xeenCombatExperience(std::uint32_t base,unsigned eligible,int permanentLevel,std::uint32_t before) {
	if(!eligible||eligible>6||permanentLevel<0)throw std::invalid_argument("invalid combat XP operands");
	const auto after=std::uint64_t(before)+std::uint64_t(base/eligible)*(permanentLevel<15?2:1);
	if(after>std::numeric_limits<std::uint32_t>::max())throw std::overflow_error("combat XP overflow");
	return static_cast<std::uint32_t>(after);
}
inline unsigned xeenCombatAttackCount(XeenCharacterClass c,unsigned level) {
	constexpr unsigned divisors[]{5,6,6,7,8,6,5,4,7,6};
	const auto i=static_cast<unsigned>(c);if(i>=10)throw std::invalid_argument("invalid combat class");
	return level/divisors[i]+1;
}
}
#endif
