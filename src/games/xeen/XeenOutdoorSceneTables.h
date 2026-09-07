#ifndef MMODERN_GAMES_XEEN_OUTDOOR_SCENE_TABLES_H
#define MMODERN_GAMES_XEEN_OUTDOOR_SCENE_TABLES_H

#include <array>

namespace mmodern::xeen_scene_tables {

// Minimal rendering subset transcribed from ScummVM's Xeen constants and
// OutdoorDrawList. These are data tables, not precomputed Area A1 results.
inline constexpr std::array<int, 48> kSouthX = {
	1,0,0,0,-1,1,0,0,0,-1,2,1,1,0,0,0,-1,-1,-2,4,3,3,2,2,
	1,1,0,0,0,-1,-1,-2,-2,-3,-3,-4,3,2,1,0,0,-1,-2,-3,4,-4,0,0
};
inline constexpr std::array<int, 48> kSouthY = {
	0,0,0,0,0,-1,-1,-1,-1,-1,-2,-2,-2,-2,-2,-2,-2,-2,-2,
	-3,-3,-3,-3,-3,-3,-3,-3,-3,-3,-3,-3,-3,-3,-3,-3,-3,
	-4,-4,-4,-4,-4,-4,-4,-4,-4,-4,0,-1
};

inline constexpr std::array<const char *, 16> kSurfaceNames = {
	"water.srf", "dirt.srf", "grass.srf", "snow.srf", "swamp.srf",
	"lava.srf", "desert.srf", "road.srf", "dwater.srf", "tflr.srf",
	"sky.srf", "croad.srf", "sewer.srf", "cloud.srf", "scortch.srf",
	"space.srf"
};
inline constexpr std::array<const char *, 16> kOutdoorWallNames = {
	"", "mount", "ltree", "dtree", "grass", "snotree", "dsnotree",
	"snomnt", "dedltree", "mount", "lavamnt", "palm", "dmount",
	"dedltree", "dedltree", "dedltree"
};

inline constexpr std::array<int, 25> kDrawNumbers = {
	36,37,38,43,42,41,39,20,22,24,33,31,29,26,10,11,18,16,13,5,9,6,0,4,1
};
inline constexpr std::array<int, 25> kGroundFrames = {
	18,19,20,24,23,22,21,11,12,13,17,16,15,14,6,7,10,9,8,3,5,4,0,2,1
};

struct Placement {
	int order;
	int frame;
	int x;
	int y;
	int scale;
	bool flipped;
	bool bottomClipped;
	bool enlarge;
};

inline constexpr std::array<Placement, 25> kGroundPlacements = {{
	{3,0,8,67,0,false,false,false}, {4,0,38,67,0,false,false,false},
	{5,0,84,67,0,false,false,false}, {6,0,134,67,0,false,false,false},
	{7,0,117,67,0,false,false,false}, {8,0,117,67,0,false,false,false},
	{9,0,103,67,0,false,false,false}, {10,0,8,73,0,false,false,false},
	{11,0,8,73,0,false,false,false}, {12,0,30,73,0,false,false,false},
	{13,0,181,73,0,false,false,false}, {14,0,154,73,0,false,false,false},
	{15,0,129,73,0,false,false,false}, {16,0,87,73,0,false,false,false},
	{17,0,8,81,0,false,false,false}, {18,0,8,81,0,false,false,false},
	{19,0,202,81,0,false,false,false}, {20,0,145,81,0,false,false,false},
	{21,0,63,81,0,false,false,false}, {22,0,8,93,0,false,false,false},
	{23,0,169,93,0,false,false,false}, {24,0,31,93,0,false,false,false},
	{25,0,8,109,0,false,false,false}, {26,0,201,109,0,false,false,false},
	{27,0,8,109,0,false,false,false}
}};

struct TerrainPlacement {
	int sample;
	Placement placement;
};

struct ObjectPlacement {
	int sample, order, scale;
	std::array<int, 2> x, y; // Normal / Clouds resource 113.
};

// Pinned ScummVM 6814ee9b: setOutdoorsObjects / OUTDOOR_OBJECT_X /
// MAP_OBJECT_Y. Ordered by depth and lateral position, not table column.
inline constexpr std::array<ObjectPlacement, 12> kObjectPlacements = {{
	{2,111,0,{-5,-35},{2,-65}},
	{5,88,7,{-112,-142},{25,-6}}, {7,87,7,{-7,-35},{25,-6}},
	{9,89,7,{98,68},{25,-6}},
	{12,67,12,{-77,-95},{50,36}}, {14,66,12,{-8,-35},{50,36}},
	{16,68,12,{61,19},{50,36}},
	{23,40,14,{-74,-98},{58,54}}, {25,38,14,{-43,-62},{58,54}},
	{27,37,14,{-9,-35},{58,54}}, {29,39,14,{25,-24},{58,54}},
	{31,41,14,{56,16},{58,54}}
}};

inline constexpr std::array<TerrainPlacement, 25> kTerrainPlacements = {{
	{44,{28,1,-64,61,14,false,false,false}}, {36,{29,1,-40,61,14,false,false,false}},
	{37,{30,1,-16,61,14,false,false,false}}, {38,{31,1,8,61,14,false,false,false}},
	{45,{32,1,128,61,14,true,false,false}}, {43,{33,1,104,61,14,true,false,false}},
	{42,{34,1,80,61,14,true,false,false}}, {41,{35,1,56,61,14,true,false,false}},
	{39,{36,1,32,61,14,false,false,false}},
	{22,{61,2,-11,54,8,false,false,false}}, {24,{62,1,-21,54,11,false,false,false}},
	{31,{63,2,165,54,8,true,false,false}}, {29,{64,1,86,54,11,true,false,false}},
	{26,{65,1,33,54,11,false,false,false}},
	{11,{84,2,8,40,0,false,false,false}}, {16,{85,2,146,40,0,true,false,false}},
	{13,{86,1,32,40,6,false,false,false}},
	{5,{103,0,8,24,0,false,false,false}}, {9,{104,0,169,24,0,true,false,false}},
	{7,{105,1,32,24,0,false,false,false}}, {0,{106,0,-23,40,0,false,false,false}},
	{4,{107,0,200,40,0,true,false,false}},
	{1,{108,0,8,47,0,false,false,false}}, {1,{109,0,169,47,0,true,false,false}},
	{1,{110,1,-56,-4,0,false,true,true}}
}};

} // namespace mmodern::xeen_scene_tables

#endif
