#ifndef MMODERN_CORE_PLAYER_ACTION_H
#define MMODERN_CORE_PLAYER_ACTION_H

#include "core/NavigationAction.h"

#include <variant>
#include <cstddef>
#include <string>

namespace mmodern {

struct SaveGameAction {};
struct ControlPanelAction {};
struct QuickReferenceAction {};
struct QuickFightAction {};
struct QuickFightOptionsAction {};
struct InfoAction {};
struct WaitAction {};
struct AttackAction {};
struct ShootAction {};
struct CastSpellAction {};
struct BlockAction {};
struct BashAction {};
struct RestAction {};
struct RunAction {};
struct SelectCombatTargetAction { unsigned row; };
struct BeginEncounterAction {};
struct RevisitCompletedAction {};
struct InspectInventoryAction {};
struct SelectInventorySlotAction { std::size_t slot; };
struct TransferInventoryAction {};
struct EquipmentInventoryAction {};
struct UseItemAction {};
struct InteractionAction {};
struct AcknowledgeAction {};
struct YesAction {};
struct NoAction {};
struct SelectMemberAction { std::size_t partyIndex; };
struct CancelInteractionAction {};
// UI-only refusal; the label is a static main-screen feature name.
struct UnsupportedMainScreenAction { const char *label; };
// Dialog intent on the existing action path; no rules live in SDL.
struct DialogKeyAction { unsigned key; };
struct TextInputAction { std::string text; };

using PlayerAction = std::variant<NavigationAction, InteractionAction,
	AcknowledgeAction, YesAction, NoAction, SelectMemberAction, CancelInteractionAction, SaveGameAction, InspectInventoryAction,
	SelectInventorySlotAction, TransferInventoryAction, EquipmentInventoryAction, UseItemAction, WaitAction,
	AttackAction, ShootAction, CastSpellAction, BlockAction, RunAction, SelectCombatTargetAction, BeginEncounterAction, RevisitCompletedAction, UnsupportedMainScreenAction, DialogKeyAction, BashAction, RestAction, TextInputAction, ControlPanelAction, QuickReferenceAction, InfoAction, QuickFightAction, QuickFightOptionsAction>;

} // namespace mmodern

#endif
