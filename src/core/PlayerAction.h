#ifndef MMODERN_CORE_PLAYER_ACTION_H
#define MMODERN_CORE_PLAYER_ACTION_H

#include "core/NavigationAction.h"

#include <variant>
#include <cstddef>

namespace mmodern {

struct SaveGameAction {};
struct WaitAction {};
struct AttackAction {};
struct BlockAction {};
struct BeginEncounterAction {};
struct InspectInventoryAction {};
struct SelectInventorySlotAction { std::size_t slot; };
struct TransferInventoryAction {};
struct EquipmentInventoryAction {};
struct InteractionAction {};
struct AcknowledgeAction {};
struct YesAction {};
struct NoAction {};
struct SelectMemberAction { std::size_t partyIndex; };
struct CancelInteractionAction {};

using PlayerAction = std::variant<NavigationAction, InteractionAction,
	AcknowledgeAction, YesAction, NoAction, SelectMemberAction, CancelInteractionAction, SaveGameAction, InspectInventoryAction,
	SelectInventorySlotAction, TransferInventoryAction, EquipmentInventoryAction, WaitAction,
	AttackAction, BlockAction, BeginEncounterAction>;

} // namespace mmodern

#endif
