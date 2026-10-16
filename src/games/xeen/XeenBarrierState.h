#ifndef MMODERN_XEEN_BARRIER_STATE_H
#define MMODERN_XEEN_BARRIER_STATE_H
#include "games/xeen/XeenMap.h"
namespace mmodern {
// Sparse session values bound to the original physical DAT cell, never edits
// to that resource. Doors and Events share this physical-cell owner.
struct XeenBarrierOverride {
	XeenMapIdentity tile;
	XeenMutable<std::uint8_t> cell=0, mask=0;
	XeenMutable<std::uint16_t> originalWord=0;
	XeenMutable<std::uint8_t> originalAttributes=0;
	XeenMutableArray<std::uint8_t,4> walls{};
	XeenMutable<bool> unlocked=false;
	XeenMutable<bool> surfaceChanged=false;
	XeenMutable<std::uint8_t> surface=0;
	// Outdoor neighbour DAT writes last only until the logical map is left.
	// Keep this layer separate so a previously retained tile is not overwritten.
	XeenMapIdentity visitMap;
	XeenMutable<std::uint8_t> visitMask=0;
	XeenMutableArray<std::uint8_t,4> visitWalls{};
	std::uint16_t word(bool includeVisit=true) const {
		std::uint16_t value=originalWord;
		for(unsigned d=0;d<4;++d) {
			const auto shift=12-4*d;
			if(mask&(1u<<d))value=(value&~(15u<<shift))|(unsigned(walls[d])<<shift);
			if(includeVisit && (visitMask&(1u<<d)))value=(value&~(15u<<shift))|(unsigned(visitWalls[d])<<shift);
		}
		return value;
	}
	std::uint8_t attributes() const {
		const unsigned value=originalAttributes|(unlocked?0x80:0);
		return surfaceChanged?(value&0xf8)|unsigned(surface):value;
	}
	friend bool operator==(const XeenBarrierOverride &a,const XeenBarrierOverride &b) {
		return a.tile==b.tile && a.cell==b.cell && a.mask==b.mask &&
			a.originalWord==b.originalWord && a.originalAttributes==b.originalAttributes &&
			a.walls==b.walls && a.unlocked==b.unlocked &&
			a.surfaceChanged==b.surfaceChanged && a.surface==b.surface &&
			a.visitMap==b.visitMap && a.visitMask==b.visitMask && a.visitWalls==b.visitWalls;
	}
};
}
#endif
