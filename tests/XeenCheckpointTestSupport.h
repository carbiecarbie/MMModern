#ifndef MMODERN_CHECKPOINT_TEST_SUPPORT_H
#define MMODERN_CHECKPOINT_TEST_SUPPORT_H
#include "core/PlayerAction.h"
#include "games/xeen/XeenNavigation.h"
#include <vector>
#define SDL_MAIN_HANDLED
#include <SDL.h>

namespace checkpoint_test {
using namespace mmodern;
// Extracted checkpoint positions and normal input sequences from the original
// Phirna/WhoWill/Myra integration tests. Positioning does not certify travel.
inline const XeenCamera phirna{23, 8, 2, XeenDirection::North};
inline const XeenCamera whistle{20, 5, 14, XeenDirection::North};
inline const XeenCamera myra{23, 9, 11, XeenDirection::West};
struct Input { SDL_Keycode key; PlayerAction action; };
inline std::vector<Input> collection(const XeenCamera &position) {
 if (position.x == phirna.x && position.y == phirna.y)
  return {{SDLK_SPACE, InteractionAction{}}, {SDLK_y, YesAction{}}, {SDLK_RETURN, AcknowledgeAction{}}};
 if (position.mapId == whistle.mapId)
  return {{SDLK_SPACE, InteractionAction{}}, {SDLK_F1, SelectMemberAction{0}}, {SDLK_RETURN, AcknowledgeAction{}}};
 return {{SDLK_SPACE, InteractionAction{}}, {SDLK_RETURN, AcknowledgeAction{}}, {SDLK_RETURN, AcknowledgeAction{}}};
}
inline bool sameCamera(const XeenCamera &a, const XeenCamera &b) {
 return a.mapId == b.mapId && a.x == b.x && a.y == b.y && a.direction == b.direction;
}
}
#endif
