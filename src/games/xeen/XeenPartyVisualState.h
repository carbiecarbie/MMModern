#ifndef MMODERN_GAMES_XEEN_XEEN_PARTY_VISUAL_STATE_H
#define MMODERN_GAMES_XEEN_XEEN_PARTY_VISUAL_STATE_H

#include "games/xeen/XeenCharacterRules.h"

#include <cstddef>

namespace mmodern {

class XeenPartyVisualState {
public:
	static std::size_t selectHpFrame(int currentHp, int maxHp);
	static std::size_t hpFrame(const XeenCharacter &character,
		const XeenCharacterRulesContext &context);
};

} // namespace mmodern

#endif
