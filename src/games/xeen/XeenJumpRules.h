#ifndef MMODERN_XEEN_JUMP_RULES_H
#define MMODERN_XEEN_JUMP_RULES_H
#include "games/xeen/XeenNavigation.h"
namespace mmodern {
class XeenWorld;
enum class XeenJumpAdmission { Blocked, Unsupported, Ready };
struct XeenJumpPreflight {
    XeenJumpAdmission admission;
    XeenCamera destination;
};
// Read-only map preflight; no movement, arrival, time, costs or random draws.
XeenJumpPreflight xeenJumpPreflight(XeenWorld &,const XeenCamera &);
}
#endif
