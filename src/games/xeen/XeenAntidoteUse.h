#ifndef MMODERN_XEEN_ANTIDOTE_USE_H
#define MMODERN_XEEN_ANTIDOTE_USE_H
#include "games/xeen/XeenItemTransfer.h"

namespace mmodern {
struct XeenAntidoteResult {
	std::uint8_t source=0;
	std::optional<std::uint8_t> target;
	std::uint8_t poisonBefore=0, poisonAfter=0;
	std::uint8_t spentCharge=0;
	bool exhausted=false;
};
// Detached byte/condition rules; the Journey coordinator owns admission and publication.
class XeenAntidoteUse {
public:
	static bool eligible(XeenItem item) noexcept;
	static XeenItem debit(XeenItem item);
	static XeenAntidoteResult effect(std::uint8_t source, std::optional<std::uint8_t> target,
		XeenCharacter *character, std::uint8_t spentCharge, bool exhausted);
	static void settle(XeenItemCategory &items, std::size_t slot, bool exhausted);
};
}
#endif
