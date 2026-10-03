#ifndef MMODERN_XEEN_JOURNEY_CONTENT_H
#define MMODERN_XEEN_JOURNEY_CONTENT_H
#include "games/xeen/XeenNavigation.h"
#include "games/xeen/XeenMovement.h"
#include <array>
namespace mmodern {
// Immutable admission policy, never a gameplay owner.
struct XeenJourneyContent {
	XeenCamera entry;
	std::array<unsigned,19> records;
	unsigned count;
	std::uint16_t day;
	XeenMovement::Capabilities traversal{};
	bool vertigoCell(int x, int y) const noexcept {
		return (x == 15 && y >= 0 && y <= 28) ||
			(x == 16 && y >= 1 && y <= 4) || (y == 4 && x >= 8 && x <= 14) ||
			(y == 7 && x >= 10 && x <= 14) || (x == 10 && y >= 8 && y <= 11);
	}
	bool eventCameraCell(int x, int y) const noexcept { return vertigoCell(x,y); }
	bool influences(unsigned record) const noexcept {
		for (unsigned i=0;i<count;++i) if (records[i]==record) return true;
		return false;
	}
};
inline const XeenJourneyContent &xeenJourneyContent() {
	static const XeenJourneyContent regional{{23,9,11,XeenDirection::West},
		{0,1,2,3,4,5,6,7,8,9,10,11,12,13,14,15,16,17,18},19,8};
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
