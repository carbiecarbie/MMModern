#ifndef MMODERN_CORE_PLAYER_ACTION_H
#define MMODERN_CORE_PLAYER_ACTION_H

#include "core/NavigationAction.h"

#include <variant>
#include <cstddef>

namespace mmodern {

struct InteractionAction {};
struct AcknowledgeAction {};
struct YesAction {};
struct NoAction {};
struct SelectMemberAction { std::size_t partyIndex; };
struct CancelInteractionAction {};

using PlayerAction = std::variant<NavigationAction, InteractionAction,
	AcknowledgeAction, YesAction, NoAction, SelectMemberAction, CancelInteractionAction>;

} // namespace mmodern

#endif
