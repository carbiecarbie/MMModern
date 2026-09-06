#include "games/xeen/XeenPartyVisualState.h"

namespace mmodern {

std::size_t XeenPartyVisualState::selectHpFrame(int currentHp, int maxHp) {
	if (currentHp < 1)
		return 4;
	else if (currentHp > maxHp)
		return 3;
	else if (currentHp == maxHp)
		return 0;
	else if (currentHp < (maxHp / 4))
		return 2;
	else
		return 1;
}

std::size_t XeenPartyVisualState::hpFrame(const XeenCharacter &character,
		const XeenCharacterRulesContext &context) {
	return selectHpFrame(character.currentHp,
		XeenCharacterRules::maxHp(character, context));
}

} // namespace mmodern
