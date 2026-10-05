#ifndef MMODERN_XEEN_BARRIER_STATE_H
#define MMODERN_XEEN_BARRIER_STATE_H
#include "games/xeen/XeenMap.h"
namespace mmodern {
// Sparse session values bound to the original physical DAT cell, never edits
// to that resource. Only selected wall faces and the unlocked bit may change.
struct XeenBarrierOverride {
	XeenMapIdentity tile;
	XeenMutable<std::uint8_t> cell=0, mask=0;
	XeenMutable<std::uint16_t> originalWord=0;
	XeenMutable<std::uint8_t> originalAttributes=0;
	XeenMutableArray<std::uint8_t,4> walls{};
	XeenMutable<bool> unlocked=false;
	friend bool operator==(const XeenBarrierOverride &a,const XeenBarrierOverride &b) {
		return a.tile==b.tile && a.cell==b.cell && a.mask==b.mask &&
			a.originalWord==b.originalWord && a.originalAttributes==b.originalAttributes &&
			a.walls==b.walls && a.unlocked==b.unlocked;
	}
};
}
#endif
