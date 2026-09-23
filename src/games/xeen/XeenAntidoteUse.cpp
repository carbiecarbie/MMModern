#include "games/xeen/XeenAntidoteUse.h"
#include "games/xeen/XeenPartyLoader.h"
#include <stdexcept>

namespace mmodern {
bool XeenAntidoteUse::eligible(XeenItem item) noexcept {
	return item.material==10 && item.id==37 && (item.state & 0xc0)==0 && (item.state & 0x3f)!=0;
}
XeenItem XeenAntidoteUse::debit(XeenItem item) {
	if (!eligible(item)) throw std::invalid_argument("Antidote item is ineligible");
	--item.state;
	return item;
}
XeenAntidoteResult XeenAntidoteUse::effect(std::uint8_t source, std::optional<std::uint8_t> target,
		XeenCharacter *character, std::uint8_t spentCharge, bool exhausted) {
	XeenAntidoteResult result{source,target,0,0,spentCharge,exhausted};
	if (target) {
		if (!character) throw std::invalid_argument("Missing antidote target");
		result.poisonBefore=character->conditions[3];
		character->conditions[3]=0;
		if (character->currentHp>0 && !character->conditions[13] && !character->conditions[14] && !character->conditions[15])
			character->conditions[12]=0;
		result.poisonAfter=character->conditions[3];
	}
	return result;
}
void XeenAntidoteUse::settle(XeenItemCategory &items, std::size_t slot, bool exhausted) {
	if (slot>=items.size()) throw std::invalid_argument("Antidote slot is invalid");
	if (exhausted) { items[slot]={}; xeenCompactItems(items); }
}
}
