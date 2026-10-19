#ifndef MMODERN_XEEN_JOURNEY_CONTENT_H
#define MMODERN_XEEN_JOURNEY_CONTENT_H
#include "games/xeen/XeenNavigation.h"
#include "games/xeen/XeenMovement.h"
#include <array>
namespace mmodern {
// Reference coordinate capacity. World resolves tile presence and passage.
inline bool xeenIndoorCoordinate(int x,int y) noexcept { return x>=0 && x<32 && y>=0 && y<32; }
// Admitted mainland area: the component reachable from the entry with water
// open, independent of the party's current abilities. Movement arrival,
// save restore and same-map spell destinations share it.
inline constexpr XeenMovement::Capabilities kXeenAdmittedAreaTraversal{true,true,false};
// Immutable Journey setup, never a gameplay owner.
struct XeenJourneyContent {
	XeenCamera entry;
	std::uint16_t day;
	XeenMovement::Capabilities traversal{};

};
inline const XeenJourneyContent &xeenJourneyContent() {
	static const XeenJourneyContent regional{{23,9,11,XeenDirection::West},
		8};
	return regional;
}
struct XeenJourneyRandomState {
	XeenMutable<std::uint8_t> algorithm=1;
	XeenMutable<std::uint32_t> state=1;
	XeenMutable<std::uint64_t> count=0;
	friend bool operator==(const XeenJourneyRandomState &a,const XeenJourneyRandomState &b) {
		return a.algorithm==b.algorithm && a.state==b.state && a.count==b.count;
	}
	friend bool operator!=(const XeenJourneyRandomState &a,const XeenJourneyRandomState &b) { return !(a==b); }
};
}
#endif
