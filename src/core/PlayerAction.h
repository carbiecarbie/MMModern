#ifndef MMODERN_CORE_PLAYER_ACTION_H
#define MMODERN_CORE_PLAYER_ACTION_H

#include "core/NavigationAction.h"

#include <variant>

namespace mmodern {

struct InteractionAction {};

using PlayerAction = std::variant<NavigationAction, InteractionAction>;

} // namespace mmodern

#endif
