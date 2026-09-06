#ifndef MMODERN_CORE_PLAYER_ACTION_H
#define MMODERN_CORE_PLAYER_ACTION_H

#include "core/NavigationAction.h"

#include <variant>

namespace mmodern {

struct InteractionAction {};
struct AcknowledgeAction {};
struct YesAction {};
struct NoAction {};

using PlayerAction = std::variant<NavigationAction, InteractionAction,
	AcknowledgeAction, YesAction, NoAction>;

} // namespace mmodern

#endif
