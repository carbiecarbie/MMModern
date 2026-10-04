#ifndef MMODERN_XEEN_INVENTORY_STATE_H
#define MMODERN_XEEN_INVENTORY_STATE_H
#include "games/xeen/XeenItemTransfer.h"
#include "games/xeen/XeenEquipment.h"

namespace mmodern {
enum class XeenInventoryMode { Closed, Browse, UseTarget };
// Selection facts only, owned by Flow. No retained character or category copy.
struct XeenInventorySelection {
	XeenInventoryMode mode = XeenInventoryMode::Closed;
	std::size_t source = 0;
	std::optional<std::uint8_t> sourceOwner;
	XeenInventoryCategory category = XeenInventoryCategory::Weapons;
	std::optional<std::size_t> slot;
	XeenItem record{};
};
}
#endif
