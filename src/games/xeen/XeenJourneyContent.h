#ifndef MMODERN_XEEN_JOURNEY_CONTENT_H
#define MMODERN_XEEN_JOURNEY_CONTENT_H
#include "formats/xeen/XeenMonsterFormat.h"
#include "games/xeen/XeenNavigation.h"
#include <array>
#include <stdexcept>
namespace mmodern {
struct XeenJourneyActorAdmission {
	unsigned record, resourceId, profileImage;
	int spawnX, spawnY, minX, maxX, minY, maxY, hp;
	bool contains(int x, int y) const noexcept {
		return x >= minX && x <= maxX && y >= minY && y <= maxY;
	}
	void validateStatistics(const XeenMonsterRecord &statistics) const {
		statistics.validateCombat();
		if (statistics.image() != profileImage)
			throw std::invalid_argument("Journey monster statistics do not match the content descriptor");
	}
};
// Immutable admission policy, never a gameplay owner. Legacy selection is explicit.
struct XeenJourneyContent {
	std::uint16_t contract;
	XeenCamera entry;
	std::array<unsigned,4> records;
	unsigned count;
	std::uint16_t day;
	bool deferredObjective;
	bool contains(int x, int y) const noexcept {
		return contract == 1 ? x >= 13 && x <= 14 && y >= 1 && y <= 2 : x >= 0 && x <= 5 && y == 14;
	}
	bool influences(unsigned record) const noexcept {
		for (unsigned i=0;i<count;++i) if (records[i]==record) return true;
		return false;
	}
	bool movementContains(int x, int y) const noexcept {
		return contract == 1 ? contains(x,y) : x >= 0 && x <= 8 && y >= 13 && y <= 15;
	}
	XeenJourneyActorAdmission actor(unsigned record) const {
		if (!influences(record)) throw std::invalid_argument("Unadmitted Journey actor identity");
		if (contract == 1) return {5,8,8,13,2,13,14,1,2,20};
		if (record == 9) return {9,8,8,6,14,0,6,14,14,20};
		if (record == 25) return {25,9,9,1,13,0,5,13,14,30};
		return {record,9,9,8,15,0,8,14,15,30};
	}
	bool blockedTerrain(int x, int y) const noexcept {
		return contract == 2 && y == 15 && (x == 5 || x == 7);
	}
};
inline const XeenJourneyContent &xeenJourneyContent(std::uint16_t contract) {
	static const XeenJourneyContent skeleton{1,{20,13,1,XeenDirection::North},{5,0,0,0},1,1,false};
	static const XeenJourneyContent expedition{2,{20,0,14,XeenDirection::East},{9,17,18,25},4,8,true};
	if (contract == 1) return skeleton;
	if (contract == 2) return expedition;
	throw std::invalid_argument("Unsupported Journey content contract");
}
struct XeenJourneyRandomState {
	std::uint8_t algorithm=1;
	std::uint32_t state=1;
	std::uint64_t count=0;
	friend bool operator==(const XeenJourneyRandomState &a,const XeenJourneyRandomState &b) {
		return a.algorithm==b.algorithm && a.state==b.state && a.count==b.count;
	}
	friend bool operator!=(const XeenJourneyRandomState &a,const XeenJourneyRandomState &b) { return !(a==b); }
};
}
#endif
