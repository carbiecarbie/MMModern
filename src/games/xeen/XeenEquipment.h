#ifndef MMODERN_XEEN_EQUIPMENT_H
#define MMODERN_XEEN_EQUIPMENT_H

#include "games/xeen/XeenItemCatalog.h"
#include "games/xeen/XeenParty.h"
#include <optional>
#include <type_traits>

namespace mmodern {

enum class XeenEquipmentOperation { Equip, Remove };
enum class XeenEquipmentStatus {
	Success, NoChange, EmptyParty, InvalidParticipant, InvalidOwner,
	InvalidOperation, InvalidCategory, InvalidSlot, EmptySource,
	UnsupportedItem, NotProficient, Conflict, RingLimit, MedalLimit,
	Cursed, UnsafeRules
};

struct XeenEquipmentPosition {
	XeenInventoryCategory category;
	std::size_t physicalSlot;
};
struct XeenEquipmentValues {
	int intellect, personality, endurance, maxHp, maxSp;
};
struct XeenEquipmentChange {
	XeenEquipmentValues before, after;
};
// Fixed facts only. Optionals distinguish unavailable facts from valid zeroes.
struct XeenEquipmentResult {
	XeenEquipmentStatus status = XeenEquipmentStatus::InvalidOperation;
	std::optional<XeenEquipmentOperation> operation;
	std::optional<std::uint8_t> owner;
	std::optional<XeenEquipmentPosition> selection;
	std::optional<XeenItem> beforeItem, afterItem;
	std::optional<XeenEquipmentPosition> conflict;
	std::optional<std::size_t> matchingFrameCount;
	std::optional<XeenEquipmentChange> modeled;
};
static_assert(std::is_nothrow_copy_constructible_v<XeenEquipmentResult>);
static_assert(std::is_nothrow_move_constructible_v<XeenEquipmentResult>);
static_assert(std::is_nothrow_copy_assignable_v<XeenEquipmentResult>);
static_assert(std::is_nothrow_move_assignable_v<XeenEquipmentResult>);

// Resolves current membership synchronously; no selection token or presentation.
// Allocation failures can propagate during preparation, before the sole live store.
XeenEquipmentResult xeenSetEquipment(XeenPartyState &, std::size_t activeIndex,
	XeenInventoryCategory, std::size_t physicalSlot, XeenEquipmentOperation);

} // namespace mmodern
#endif
