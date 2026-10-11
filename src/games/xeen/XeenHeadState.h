#ifndef MMODERN_XEEN_HEAD_STATE_H
#define MMODERN_XEEN_HEAD_STATE_H
#include "games/xeen/XeenNavigation.h"
#include <array>
#include <optional>
namespace mmodern {
struct XeenHeadIndicators {
 XeenMutable<std::uint8_t> left=0, right=0;
 friend bool operator==(XeenHeadIndicators a,XeenHeadIndicators b) {return a.left==b.left && a.right==b.right;}
 friend bool operator!=(XeenHeadIndicators a,XeenHeadIndicators b) {return !(a==b);}
};
// Only the current primary-map HED may be overridden. A real map load ends it.
struct XeenHeadCache {
 XeenMapIdentity mapId;
 std::array<XeenMutableOptional<XeenHeadIndicators>,256> overrides{};
 friend bool operator==(const XeenHeadCache &a,const XeenHeadCache &b) {return a.mapId==b.mapId && a.overrides==b.overrides;}
 friend bool operator!=(const XeenHeadCache &a,const XeenHeadCache &b) {return !(a==b);}
};
}
#endif
