#ifndef MMODERN_XEEN_INVENTORY_VIEW_H
#define MMODERN_XEEN_INVENTORY_VIEW_H
#include "games/xeen/XeenItemTransfer.h"
#include "games/xeen/XeenEquipment.h"
#include "games/xeen/XeenTextRenderer.h"

namespace mmodern {
enum class XeenInventoryMode { Closed, Browse, ChooseDestination, Confirm };
// Selection facts only, owned by Flow. No retained character or category copy.
struct XeenInventorySelection {
	XeenInventoryMode mode = XeenInventoryMode::Closed;
	std::size_t source = 0;
	std::optional<std::uint8_t> sourceOwner;
	XeenInventoryCategory category = XeenInventoryCategory::Weapons;
	std::optional<std::size_t> slot;
	XeenItem record{};
	std::optional<std::size_t> destination;
	std::optional<std::uint8_t> destinationOwner;
};
// Disposable layout, useful for checking that complete mandatory fields fit.
struct XeenInventoryLine { XeenTextRect bounds; std::string text; };
std::vector<XeenInventoryLine> xeenInventoryLayout(const XeenFontFormat &,
	const XeenItemCatalog &, const XeenPartyState &, const XeenInventorySelection &, const char *feedback,
	const XeenEquipmentResult *equipmentResult = nullptr, bool combatPreparation = false, bool readOnly = false);
IndexedFrame drawXeenInventory(const IndexedFrame &, const XeenFontFormat &,
	const XeenItemCatalog &, const XeenPartyState &, const XeenInventorySelection &, const char *feedback,
	const XeenEquipmentResult *equipmentResult = nullptr, bool combatPreparation = false, bool readOnly = false);
}
#endif
