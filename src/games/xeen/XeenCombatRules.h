#ifndef MMODERN_XEEN_COMBAT_RULES_H
#define MMODERN_XEEN_COMBAT_RULES_H
#include "games/xeen/XeenCombat.h"
#include "games/xeen/XeenCharacterRules.h"
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
// Explicit value inputs from the party owner, never inferred from a missing
// field. Order is fire, electrical, cold, poison as in giveCharDamage.
struct XeenDamageProtection {
	std::array<int,4> resistances{};
	int powerShield=0;
};
// Detached giveCharDamage continuation. Damage carries between selected
// members exactly as in the reference; publication remains with the caller.
struct XeenTypedDamageCandidate {
	XeenConsequenceCharacters characters;
	XeenCombatResult result;
	bool deferInjury=false, injuryReady=false, injuryAcknowledged=false, injuryApplied=false;
	std::uint8_t impactOwner=0;
	unsigned portraitFrame=0;
	XeenTypedDamageCandidate(const XeenConsequenceCharacters &,const XeenConsequenceInputs &,
		int damage,XeenDamageType,unsigned year,unsigned memberMask,const XeenDamageProtection &);
	bool service(XeenConsequenceDraw &);
private:
	enum class Step { Begin, Save, Injury, Next, Done } step=Step::Begin;
	XeenConsequenceInputs inputs;
	XeenDamageProtection protection;
	XeenDamageType type;
	unsigned year,mask,target=0;
	int damage,beforeAc=0;
};
// One reference targeting pass, independent of damage/ability admission.
// A zero mask means the reference defeat path has no able target.
struct XeenMonsterTargetCandidate {
    unsigned mask=0;
    XeenMonsterTargetCandidate(const XeenConsequenceCharacters &,unsigned hatred,unsigned participants);
    bool service(XeenConsequenceDraw &);
private:
    std::array<unsigned,6> participants{},eligible{};
    unsigned count=0,eligibleCount=0;
    unsigned preferred=6;
    enum class Step { Target, Fallback, Done } step=Step::Target;
};
// Detached learned Magic Arrow. Same world-owned cursor and bounded draw service.
struct XeenMagicArrowCandidate {
    int damage=0;
    bool resisted=false;
    XeenMagicArrowCandidate(std::int64_t permanentLevel, std::int64_t temporaryLevel,
        unsigned resistance, unsigned resourceId);
    bool service(XeenConsequenceDraw &);
private:
    enum class Step { Resistance, Save, Done };
    Step step;
    unsigned levelBound, resistance, resourceId, saveBound;
};
struct XeenRunCandidate {
	unsigned roll = 0;
	bool success = false;
	explicit XeenRunCandidate(int signedThreshold);
	bool service(XeenConsequenceDraw &);
private:
	int threshold;
};
struct XeenEnemyAttackCandidate {
	XeenConsequenceCharacters characters;
	XeenCombatResult result;
 // Live opportunity owners opt into one acknowledged portrait per injury.
 bool deferInjury=false, injuryReady=false, injuryAcknowledged=false, injuryApplied=false;
 std::uint8_t impactOwner=0;
	XeenEnemyAttackCandidate(const XeenConsequenceCharacters &, const XeenConsequenceInputs &,
		const XeenMonsterRecord &, unsigned year, unsigned participantMask, const std::array<bool,6> &blocked = {});
	bool service(XeenConsequenceDraw &);
	// Call only after publishing/retaining the completed attack. Both melee and
	// ranged sources use this same reference count loop and updated characters.
	bool nextAttack();
private:
	enum class Step { Target, Begin, Roll, Dice, Special, Injury, Parameter,
		PoisonSaveInitial, PoisonSaveRepeat, Next, Done };
	Step step = Step::Target, afterInjury = Step::Next;
	XeenConsequenceInputs inputs;
	XeenMonsterRecord monster;
	unsigned year;
	std::array<unsigned,6> participants{};
	unsigned participantCount = 0, participantCursor = 0, attackOrdinal=0;
	std::optional<XeenMonsterTargetCandidate> selection;
	std::array<bool,6> blocked;
	int target = -1, roll = 0, damage = 0, beforeDamageAc = 0;
	unsigned dice = 0;
	bool allParty;
	bool poison=false;
	bool noTarget=false;
};
struct XeenPhysicalPlayerCandidate {
	int damage = 0;
	bool hit = false;
	XeenPhysicalPlayerCandidate(const XeenCharacter &, const XeenCombatInputs &,
		const XeenMonsterRecord &, unsigned monsterType, unsigned year, bool shoot,
		XeenDifficulty difficulty = XeenDifficulty::Adventurer);
	bool service(XeenConsequenceDraw &);
private:
	enum class Step { Weapon, Hit, Save, Done };
	Step step = Step::Weapon;
	XeenCharacter character;
	XeenMonsterRecord monster;
	unsigned monsterType, attacks, slot = 0, dice = 0, sides = 0;
	int baseHit, hitTotal, might, weapon = 0, accumulated = 0;
	bool shoot, adventurer;
};
enum class XeenTimeMode { Interactive, Sleeping, Script, Interactive7 };
enum class XeenTimeCall { Change, Add };
class XeenMerchantStockCandidate;
void xeenResetCharacterTemps(XeenCharacter &,XeenCombatInputs &);
void xeenResetPartyTemps(XeenGameplayContext &);
struct XeenConditionTimeCandidate {
	XeenGameplayContext context;
	XeenConsequenceCharacters characters;
	XeenConsequenceInputs inputs;
	std::optional<XeenServiceEconomy> economy;
	bool needsRest=false, resetTemps=false;
	XeenConditionTimeCandidate(const XeenGameplayContext &, std::uint64_t charge,
		const XeenConsequenceCharacters &, const XeenConsequenceInputs &,
		const XeenServiceEconomy *economy=nullptr,XeenTimeMode mode=XeenTimeMode::Interactive,
		XeenTimeCall call=XeenTimeCall::Change);
	bool service(XeenConsequenceDraw &);
private:
	enum class Step { Stats, Poison, Electrical, Disease, Cold, Remaining, Economy, Dawn, Confused, Physical, Paralyzed, Done };
	Step step;
	XeenGameplayContext ending;
	XeenTimeMode mode;
	XeenTimeCall call;
	std::shared_ptr<XeenMerchantStockCandidate> stock;
	unsigned owner = 0;
};
}
#endif
