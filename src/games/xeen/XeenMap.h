#ifndef MMODERN_GAMES_XEEN_MAP_H
#define MMODERN_GAMES_XEEN_MAP_H

#include "games/xeen/XeenNavigation.h"

#include <array>
#include <cstddef>
#include <cstdint>
#include <variant>
#include <vector>

namespace mmodern {

struct XeenIndoorWalls {
	std::array<std::uint8_t, 4> walls{}; // North, east, south, west.
};

struct XeenOutdoorLayers {
	std::uint8_t surface = 0, middle = 0, top = 0, overlay = 0;
};

struct XeenMapCell {
	std::uint16_t rawWord = 0;
	std::uint8_t rawAttributes = 0, surfaceIndex = 0, flags = 0;
	std::variant<XeenIndoorWalls, XeenOutdoorLayers> geometry;
	bool seen = false, stepped = false;
};

struct XeenMapGeometry {
	static constexpr std::size_t kWidth = 16, kHeight = 16;
	std::uint16_t id = 0, flags = 0, flags2 = 0;
	std::array<std::uint16_t, 4> neighbors{}; // North, east, south, west; zero = none.
	std::array<XeenMapCell, kWidth * kHeight> cells{}; // y * 16 + x; Y increases north.
	std::array<std::uint8_t, 16> wallTypes{}, surfaceTypes{};
	std::uint8_t floorType = 0, runX = 0, runY = 0;
	// Preserve metadata without implementing its gameplay effects.
	std::array<int, 8> difficulties{};
	std::uint8_t trapDamage = 0, wallKind = 0, tavernTips = 0;

	bool isOutdoors() const { return (flags2 & 0x8000) != 0; }
};

struct XeenMapEntity {
	int x = 0, y = 0; // Signed bytes, including -128 for disabled records.
	std::uint8_t tableIndex = 0, direction = 0;
	int resourceId = -1; // Sprite ID for objects/wall items; monster type ID otherwise.

	bool isDisabled() const { return x == -128 || y == -128; }
	bool hasResource() const { return resourceId >= 0; }
	bool isActive() const { return !isDisabled() && hasResource(); }
};

struct XeenMapEntities {
	// Raw tables retain FF slots. Only monster/wall item lookup compacts those slots.
	std::array<std::uint8_t, 16> objectTable{}, monsterTable{}, wallItemTable{};
	std::vector<XeenMapEntity> objects, monsters, wallItems;
};

struct XeenEventInstruction {
	std::uint8_t x = 0, y = 0, direction = 0, line = 0, opcode = 0;
	std::vector<std::uint8_t> parameters; // Opaque; no opcode execution.
};

struct XeenMap {
	XeenMapGeometry geometry;
	XeenMapEntities entities;
	std::vector<XeenEventInstruction> instructions;
};

inline std::uint8_t wallAt(const XeenMapCell &cell, XeenDirection direction) {
	const auto &walls = std::get<XeenIndoorWalls>(cell.geometry).walls;
	return walls.at(static_cast<std::size_t>(direction));
}

} // namespace mmodern

#endif
