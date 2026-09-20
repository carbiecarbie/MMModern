#ifndef MMODERN_XEEN_COMBAT_RULES_H
#define MMODERN_XEEN_COMBAT_RULES_H
#include "games/xeen/XeenCombat.h"
#include <limits>
#include <stdexcept>
namespace mmodern {
// These predicates intentionally differ from action eligibility. Sleep is a
// targetable, nonterminal condition; terminal conditions remain XP-ineligible.
inline bool xeenCombatTargetable(const XeenCharacter &c) noexcept {
	const auto condition = c.worstCondition();
	return condition < XeenCondition::Paralyzed || condition == XeenCondition::Good;
}
struct XeenWeaponDice { unsigned count, sides; };
XeenWeaponDice xeenOrdinaryWeaponDice(unsigned id);
// Shared checked HP consequences after wake/special changes are prepared.
void xeenApplyPhysicalInjury(XeenCharacter &, int damage, unsigned year);
// Pure immutable-resource admission shared with completed restore.
void xeenValidateInitialCombatParty(const XeenPartyState &, const std::vector<std::uint8_t> &,
	const std::array<XeenCombatInputs, 6> &);
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
// Detached continuation helpers: callers retain authority and own publication.
// A service budget counts rejected conversions as well as accepted draws.
struct XeenConsequenceDraw {
	XeenCombatRandom &random;
	unsigned remaining = 64;
	std::function<void()> check;
	std::optional<unsigned> draw(unsigned lo, unsigned hi);
};
using XeenConsequenceCharacters = std::array<XeenCharacter,6>;
using XeenConsequenceInputs = std::array<XeenCombatInputs,6>;
struct XeenEnemyAttackCandidate {
	XeenConsequenceCharacters characters;
	XeenCombatResult result;
	XeenEnemyAttackCandidate(const XeenConsequenceCharacters &, const XeenConsequenceInputs &,
		const XeenMonsterRecord &, unsigned year, const std::array<bool,6> &blocked = {});
	bool service(XeenConsequenceDraw &);
private:
	enum class Step { Target, Fallback, Begin, Roll, Dice, Special, Injury, Parameter, Next, Done };
	Step step = Step::Target, afterInjury = Step::Next;
	XeenConsequenceInputs inputs;
	XeenMonsterRecord monster;
	unsigned year;
	std::array<bool,6> blocked;
	int target = -1, roll = 0, damage = 0, beforeDamageAc = 0;
	unsigned dice = 0;
	bool allParty;
};
struct XeenPhysicalPlayerCandidate {
	int damage = 0;
	bool hit = false;
	XeenPhysicalPlayerCandidate(const XeenCharacter &, const XeenCombatInputs &,
		const XeenMonsterRecord &, unsigned monsterType, unsigned year, bool shoot);
	bool service(XeenConsequenceDraw &);
private:
	enum class Step { Weapon, Hit, Save, Done };
	Step step = Step::Weapon;
	XeenCharacter character;
	XeenMonsterRecord monster;
	unsigned monsterType, attacks, slot = 0, dice = 0, sides = 0;
	int baseHit, hitTotal, might, weapon = 0, accumulated = 0;
	bool shoot;
};
struct XeenConditionTimeCandidate {
	XeenGameplayContext context;
	XeenConsequenceCharacters characters;
	XeenConditionTimeCandidate(const XeenGameplayContext &, unsigned charge,
		const XeenConsequenceCharacters &, const XeenConsequenceInputs &);
	bool service(XeenConsequenceDraw &);
private:
	enum class Step { Stats, Poison, Electrical, Disease, Cold, Death, Done };
	Step step = Step::Done;
	XeenConsequenceInputs inputs;
	unsigned owner = 0;
};
}
#endif
