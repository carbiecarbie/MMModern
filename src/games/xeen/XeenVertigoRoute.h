#ifndef MMODERN_XEEN_VERTIGO_ROUTE_H
#define MMODERN_XEEN_VERTIGO_ROUTE_H
#include "games/xeen/XeenEventFile.h"
namespace mmodern {
// Immutable original instruction graph required by the admitted transition.
void xeenValidateVertigoRoute(const XeenEventFile &mainland, const XeenEventFile &city);
}
#endif
