#ifndef MMODERN_GAMES_XEEN_XEEN_CHARACTER_RULES_H
#define MMODERN_GAMES_XEEN_XEEN_CHARACTER_RULES_H

#include "games/xeen/XeenCharacter.h"
#include "games/xeen/XeenCombatInputs.h"

#include <cstdint>

namespace mmodern {

struct XeenCharacterRulesContext {
	std::uint32_t currentYear = 0;
};

class XeenCharacterRules {
public:
	enum class PhysicalAttribute { Might = 0, Speed = 4, Accuracy = 5 };
	static int effectivePhysical(const XeenCharacter &, const XeenCombatInputs &, PhysicalAttribute,
		const XeenCharacterRulesContext &);
	static int physicalBonus(int);
	static int combatArmorClass(const XeenCharacter &, const XeenCombatInputs &, const XeenCharacterRulesContext &);
	// Preflight untrusted active-character values using the same calculations,
	// with checked intermediates. Does not normalize or mutate the character.
	static void validateForUse(const XeenCharacter &character,
		const XeenCharacterRulesContext &context);
	static int effectiveEndurance(const XeenCharacter &character,
		const XeenCharacterRulesContext &context);
	static int effectiveIntellect(const XeenCharacter &character,
		const XeenCharacterRulesContext &context);
	static int effectivePersonality(const XeenCharacter &character,
		const XeenCharacterRulesContext &context);

	static int maxHp(const XeenCharacter &character,
		const XeenCharacterRulesContext &context);
	static int maxSp(const XeenCharacter &character,
		const XeenCharacterRulesContext &context);
};

} // namespace mmodern

#endif
