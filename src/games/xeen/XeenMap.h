#ifndef MMODERN_GAMES_XEEN_MAP_H
#define MMODERN_GAMES_XEEN_MAP_H

#include "games/xeen/XeenNavigation.h"

#include <array>
#include <cstddef>
#include <cstdint>
#include <string>
#include <variant>
#include <vector>

namespace mmodern {

struct XeenIndoorWalls {
	XeenMutableArray<std::uint8_t, 4> walls{}; // North, east, south, west.
};

struct XeenOutdoorLayers {
	XeenMutable<std::uint8_t> surface = 0, middle = 0, top = 0, overlay = 0;
};

struct XeenMapCell {
	XeenMutable<std::uint16_t> rawWord = 0;
	XeenMutable<std::uint8_t> rawAttributes = 0, surfaceIndex = 0, flags = 0;
	XeenMutableVariant<XeenIndoorWalls, XeenOutdoorLayers> geometry;
	XeenMutable<bool> seen = false, stepped = false;
};

struct XeenMapGeometry {
	static constexpr std::size_t kWidth = 16, kHeight = 16;
	XeenMutable<std::uint16_t> id = 0, flags = 0, flags2 = 0;
	XeenMutableArray<std::uint16_t, 4> neighbors{}; // North, east, south, west; zero = none.
	std::array<XeenMapCell, kWidth * kHeight> cells{}; // y * 16 + x; Y increases north.
	XeenMutableArray<std::uint8_t, 16> wallTypes{}, surfaceTypes{};
	XeenMutable<std::uint8_t> floorType = 0, runX = 0, runY = 0;
	// Preserve metadata without implementing its gameplay effects.
	XeenMutableArray<int, 8> difficulties{};
	XeenMutable<std::uint8_t> trapDamage = 0, wallKind = 0, tavernTips = 0;

	bool isOutdoors() const { return (flags2 & 0x8000) != 0; }
};

struct XeenMapEntity {
	XeenMutable<int> x = 0, y = 0; // Signed bytes, including -128 for disabled records.
	XeenMutable<std::uint8_t> tableIndex = 0, direction = 0;
	XeenMutable<int> resourceId = -1; // Sprite ID for objects/wall items; monster type ID otherwise.

	bool isDisabled() const { return x == -128 || y == -128; }
	bool hasResource() const { return resourceId >= 0; }
	bool isActive() const { return !isDisabled() && hasResource(); }
};

struct XeenMapEntities {
	// Raw tables retain FF slots. Only monster/wall item lookup compacts those slots.
	XeenMutableArray<std::uint8_t, 16> objectTable{}, monsterTable{}, wallItemTable{};
	XeenMutableVector<XeenMapEntity> objects, monsters, wallItems;
};

// Immutable per-map MOB payload; separate from geometry and session mutations.
struct XeenObjectFile {
	XeenMapIdentity mapId;
	XeenMutableString resourceName;
	XeenMutable<bool> resourcePresent = false;
	XeenMapEntities entities;
};

struct XeenEventInstruction {
	XeenMutable<std::uint8_t> x = 0, y = 0, direction = 0, line = 0, opcode = 0;
	XeenMutableVector<std::uint8_t> parameters; // Opaque; no opcode execution.
};

struct XeenMap {
	XeenMutable<XeenSide> side = XeenSide::Clouds; // Loading context, not serialized geometry.
	XeenMapGeometry geometry;
	XeenMapEntities entities;
	XeenMutableVector<XeenEventInstruction> instructions;
	XeenMapIdentity identity() const { return {side, geometry.id}; }
};

inline std::uint8_t wallAt(const XeenMapCell &cell, XeenDirection direction) {
	const auto &walls = xeenGet<XeenIndoorWalls>(cell.geometry).walls;
	return walls.at(static_cast<std::size_t>(direction));
}

} // namespace mmodern

#endif
