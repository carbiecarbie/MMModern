#ifndef MMODERN_TESTS_XEEN_INDOOR_OBJECT_TEST_ORACLE_H
#define MMODERN_TESTS_XEEN_INDOOR_OBJECT_TEST_ORACLE_H

#include "games/xeen/XeenNavigation.h"

#include <array>

namespace indoor_object_test {

// Test-owned transcription of the Milestone 23 placement contract. This table
// deliberately has no dependency on XeenIndoorSceneTables.h.
struct PlacementSpec {
	int query;
	int forward;
	int lateral; // Positive is camera-left; negative is camera-right.
	int order;
	int normalX;
	int normalY;
	int resource113X;
	int resource113Y;
	int scale;
	bool bottomClipped;
};

inline constexpr std::array<PlacementSpec, 12> kPlacementSpecs = {{
	{ 2, 0,  0, 149,   -5,  2,  -35, -65,  0, true },
	{ 7, 1,  0, 125,   -7, 25,  -35,  -6,  7, false},
	{ 5, 1,  1, 126, -112, 25, -142,  -6,  7, false},
	{ 9, 1, -1, 127,   98, 25,   68,  -6,  7, false},
	{14, 2,  0,  97,   -8, 50,  -35,  36, 12, false},
	{12, 2,  1,  98,  -65, 50,  -95,  36, 12, false},
	{16, 2, -1,  99,   49, 50,   19,  36, 12, false},
	{27, 3,  0,  55,   -9, 58,  -35,  54, 14, false},
	{25, 3,  1,  56,  -34, 58,  -62,  54, 14, false},
	{29, 3, -1,  57,   16, 58,  -14,  54, 14, false},
	{23, 3,  2,  58,  -58, 58,  -98,  54, 14, false},
	{31, 3, -2,  59,   40, 58,   16,  54, 14, false}
}};

struct SourceCell {
	int x;
	int y;
};

// Independent camera transform for the project convention that map Y grows
// northward. It uses the literal forward/lateral fields above, not the
// production 44-query screen-positioning table.
inline constexpr SourceCell expectedSource(const mmodern::XeenCamera &camera,
		const PlacementSpec &placement) {
	int forwardX = 0, forwardY = 0, leftX = 0, leftY = 0;
	switch (camera.direction) {
	case mmodern::XeenDirection::North:
		forwardY = 1; leftX = -1; break;
	case mmodern::XeenDirection::East:
		forwardX = 1; leftY = 1; break;
	case mmodern::XeenDirection::South:
		forwardY = -1; leftX = 1; break;
	case mmodern::XeenDirection::West:
		forwardX = -1; leftY = -1; break;
	}
	return {camera.x + placement.forward * forwardX + placement.lateral * leftX,
		camera.y + placement.forward * forwardY + placement.lateral * leftY};
}

inline constexpr const PlacementSpec *specForQuery(int query) {
	for (const auto &placement : kPlacementSpecs)
		if (placement.query == query) return &placement;
	return nullptr;
}

} // namespace indoor_object_test

#endif
