#include "games/xeen/XeenIndoorScene.h"

#include "games/xeen/XeenIndoorSceneTables.h"
#include "games/xeen/XeenMap.h"
#include "games/xeen/XeenWorld.h"

#include <algorithm>
#include <array>
#include <stdexcept>
#include <string>
#include <utility>

namespace mmodern {
namespace {

XeenDirection faceForShift(std::uint8_t shift) {
	switch (shift) {
	case 12: return XeenDirection::North;
	case 8:  return XeenDirection::East;
	case 4:  return XeenDirection::South;
	case 0:  return XeenDirection::West;
	default: throw std::runtime_error("deslocamento de parede interior invalido");
	}
}

const char *terrainPrefix(std::uint8_t wallKind) {
	static constexpr std::array<const char *, 6> kPrefixes = {
		"town", "cave", "towr", "cstl", "dung", "scfi"
	};
	if (wallKind >= kPrefixes.size())
		throw std::runtime_error("wallKind interior invalido");
	return kPrefixes[wallKind];
}

struct MazeBits {
	std::array<bool, 308> active{};
};

MazeBits buildMazeBits(const std::array<XeenIndoorWallSample,
		XeenIndoorScene::kWallSampleCount> &samples) {
	MazeBits bits;
	for (const auto &sample : samples) {
		if (!sample.wallValue)
			continue;
		const auto value = static_cast<std::size_t>(*sample.wallValue);
		if (value >= 16)
			throw std::runtime_error("nibble de parede interior invalido");
		const int bit = xeen_indoor_scene_tables::kMazeBitIndex[sample.queryIndex][value];
		if (bit >= 0)
			bits.active[static_cast<std::size_t>(bit)] = true;
	}
	return bits;
}

std::optional<int> frontFrame4(std::uint8_t value, bool preserveOriginalRightBug = false) {
	switch (value) {
	case 1: return 8;
	case 2: return 16;
	case 3: return 10;
	case 4: case 13: return 7;
	case 5: case 8: return 0;
	case 6: return 14;
	case 7: return preserveOriginalRightBug ? std::nullopt : std::optional<int>(15);
	case 9: return 9;
	case 10: return 11;
	case 11: return 6;
	case 12: return 1; // Static _overallFrame is zero in this milestone.
	case 14: return 12;
	case 15: return 13;
	default: return std::nullopt;
	}
}

std::optional<int> frontFrame3(std::uint8_t value) {
	switch (value) {
	case 1: return 25;
	case 2: return 33;
	case 3: return 27;
	case 4: case 13: return 24;
	case 5: case 8: return 17;
	case 6: return 31;
	case 7: return 32;
	case 9: return 26;
	case 10: return 30;
	case 11: return 23;
	case 12: return 18;
	case 14: return 28;
	case 15: return 29;
	default: return std::nullopt;
	}
}

std::optional<int> frontFrame2(std::uint8_t value) {
	switch (value) {
	case 1: return 8;
	case 2: return 16;
	case 3: return 10;
	case 4: case 13: return 7;
	case 5: case 8: return 0;
	case 6: return 14;
	case 7: return 15;
	case 9: return 9;
	case 10: return 13;
	case 11: return 6;
	case 12: return 1;
	case 14: return 11;
	case 15: return 12;
	default: return std::nullopt;
	}
}

} // namespace

std::array<XeenIndoorWallSample, XeenIndoorScene::kWallSampleCount>
XeenIndoorScene::sampleWalls(XeenWorld &world, const XeenCamera &camera) const {
	const XeenMap &map = world.map(camera.mapId);
	if (map.geometry.isOutdoors())
		throw std::runtime_error("XeenIndoorScene requer um mapa interior");
	if (map.identity() != camera.mapId)
		throw std::runtime_error("camera e mapa possuem IDs diferentes");
	if (camera.x < 0 || camera.x >= 16 || camera.y < 0 || camera.y >= 16)
		throw std::runtime_error("camera fora dos limites do mapa interior");

	const auto directionIndex = static_cast<std::size_t>(camera.direction);
	if (directionIndex >= xeen_indoor_scene_tables::kScreenPositioningX.size())
		throw std::runtime_error("direcao de camera invalida");

	std::array<XeenIndoorWallSample, kWallSampleCount> result{};
	for (std::size_t queryIndex = 0; queryIndex < result.size(); ++queryIndex) {
		XeenIndoorWallSample &wall = result[queryIndex];
		wall.queryIndex = queryIndex;
		wall.sourceMapId = camera.mapId;
		wall.sourceX = camera.x +
			xeen_indoor_scene_tables::kScreenPositioningX[directionIndex][queryIndex];
		wall.sourceY = camera.y +
			xeen_indoor_scene_tables::kScreenPositioningY[directionIndex][queryIndex];
		wall.sourceFace = faceForShift(
			xeen_indoor_scene_tables::kWallShifts[directionIndex][queryIndex]);

		const auto cell = world.sampleCell(camera.mapId, wall.sourceX, wall.sourceY);
		if (cell) {
			wall.sourceMapId = cell->mapId;
			wall.wallValue = wallAt(*cell->cell, wall.sourceFace);
		}
	}
	return result;
}

std::vector<XeenIndoorDrawCommand> XeenIndoorScene::build(
		XeenWorld &world, const XeenCamera &camera) const {
	const XeenMap &map = world.map(camera.mapId);
	if (map.geometry.isOutdoors())
		throw std::runtime_error("XeenIndoorScene requer um mapa interior");
	const std::string terrain = terrainPrefix(map.geometry.wallKind);
	const std::string sky = terrain + ".sky";
	const std::string ground = terrain + ".gnd";
	const std::string fwl1 = "f" + terrain + "1.fwl";
	const std::string fwl2 = "f" + terrain + "2.fwl";
	const std::string fwl3 = "f" + terrain + "3.fwl";
	const std::string fwl4 = "f" + terrain + "4.fwl";
	const std::string swl = "s" + terrain + ".swl";
	const auto samples = sampleWalls(world, camera);
	const MazeBits maze = buildMazeBits(samples);
	const auto &wo = maze.active;

	std::vector<XeenIndoorDrawCommand> commands;
	commands.reserve(48);
	auto add = [&](int order, const std::string &resource, int frame, int x, int y,
			bool flip, std::optional<std::size_t> sampleIndex = std::nullopt) {
		XeenIndoorDrawCommand command;
		command.originalOrder = order;
		command.resourceName = resource;
		command.frame = static_cast<std::size_t>(frame);
		command.x = x;
		command.y = y;
		command.sourceMapId = camera.mapId;
		command.sourceX = camera.x;
		command.sourceY = camera.y;
		command.sourceFace = camera.direction;
		command.options.scaleIndex = 0;
		command.options.horizontalFlip = flip;
		command.options.sceneClipped = true;
		command.options.bottomClipped = false;
		if (sampleIndex) {
			const auto &source = samples.at(*sampleIndex);
			command.sourceMapId = source.sourceMapId;
			command.sourceX = source.sourceX;
			command.sourceY = source.sourceY;
			command.sourceFace = source.sourceFace;
		}
		commands.push_back(std::move(command));
	};
	auto value = [&](std::size_t index) -> std::uint8_t {
		return samples[index].wallValue.value_or(0);
	};
	auto addFront = [&](bool visible, int order, const std::string &resource,
			std::optional<int> frame, int x, int y, bool flip, std::size_t sampleIndex) {
		if (visible && frame)
			add(order, resource, *frame, x, y, flip, sampleIndex);
	};
	auto addSide = [&](bool visible, int order, int x, int y, bool flip,
			std::size_t sampleIndex, int ordinaryFrame, int type2Frame) {
		if (!visible || value(sampleIndex) == 0)
			return;
		add(order, swl, value(sampleIndex) == 2 ? type2Frame : ordinaryFrame,
			x, y, flip, sampleIndex);
	};

	// Fixed entries 0, 1, 2 and 28 from IndoorDrawList.
	add(0, sky, 0, 8, 8, false);
	add(1, sky, 1, 8, 25, false);
	add(2, ground, 0, 8, 67, false);
	add(28, fwl1, 7, 8, 64, false);

	// Depth four side walls. Conditions are a direct translation of drawIndoors().
	addSide(!wo[27] && !wo[20] && !wo[23] && !wo[12] && !wo[8] && !wo[30],
		29, 32, 60, false, 36, 22, 46);
	addSide(!wo[27] && !wo[22] && !wo[17] && !wo[12] && !wo[8],
		30, 56, 60, false, 37, 20, 44);
	addSide(!wo[27] && !wo[22] && !wo[15] && !wo[2] && !wo[7],
		31, 80, 60, false, 38, 18, 42);
	addSide(!wo[27] && !wo[22] && !wo[15] && !wo[6],
		32, 104, 60, false, 39, 16, 40);
	addSide(!wo[27] && !wo[22] && !wo[15] && !wo[6],
		36, 120, 60, true, 40, 17, 41);
	addSide(!wo[27] && !wo[22] && !wo[15] && !wo[5] && !wo[9],
		35, 131, 60, true, 41, 19, 43);
	addSide(!wo[27] && !wo[22] && !wo[19] && !wo[14] && !wo[10],
		34, 144, 60, true, 42, 21, 45);
	addSide(!wo[27] && !wo[21] && !wo[24] && !wo[14] && !wo[10] && !wo[31],
		33, 152, 60, true, 43, 23, 47);

	// Depth four front walls.
	addFront(!wo[25] && !wo[28] && !wo[20] && !wo[11] && !wo[16] && !wo[30] && !wo[32],
		37, fwl4, frontFrame4(value(19)), 8, 60, false, 19);
	addFront(!wo[27] && !wo[20] && !wo[12] && !wo[23] && !wo[8] && !wo[30],
		38, fwl4, frontFrame4(value(21)), 32, 60, false, 21);
	addFront(!(wo[22] && wo[20]) && !(wo[22] && wo[23]) && !(wo[20] && wo[17]) &&
		!(wo[23] && wo[17]) && !wo[12] && !wo[8],
		39, fwl4, frontFrame4(value(23)), 56, 60, false, 23);
	addFront(!(wo[15] && wo[17]) && !(wo[15] && wo[12]) && !(wo[12] && wo[7]) &&
		!(wo[17] && wo[7]), 40, fwl4, frontFrame4(value(25)), 80, 60, false, 25);
	addFront(!wo[27] && !wo[22] && !wo[15],
		41, fwl4, frontFrame4(value(27)), 104, 60, false, 27);
	addFront(!(wo[15] && wo[19]) && !(wo[15] && wo[14]) && !(wo[14] && wo[9]) &&
		!(wo[19] && wo[9]), 42, fwl4, frontFrame4(value(29)), 128, 60, false, 29);
	addFront(!(wo[22] && wo[21]) && !(wo[22] && wo[24]) && !(wo[21] && wo[19]) &&
		!(wo[24] && wo[19]) && !wo[14] && !wo[10],
		43, fwl4, frontFrame4(value(31)), 152, 60, false, 31);
	addFront(!wo[27] && !wo[21] && !wo[14] && !wo[24] && !wo[10] && !wo[31],
		44, fwl4, frontFrame4(value(33)), 176, 60, false, 33);
	addFront(!wo[26] && !wo[29] && !wo[21] && !wo[13] && !wo[18] && !wo[31] && !wo[33],
		45, fwl4, frontFrame4(value(35), true), 200, 60, false, 35);

	// Depth three side walls.
	addSide(!wo[25] && !wo[28] && !wo[20] && !wo[11] && !wo[16] && !wo[30],
		71, 8, 58, false, 20, 14, 38);
	const bool block3F3L = (wo[28] && (wo[27] || wo[12] || wo[23] || wo[8])) ||
		(wo[25] && (wo[27] || wo[12] || wo[23] || wo[8])) ||
		(wo[17] && (wo[27] || wo[12] || wo[23] || wo[8])) || wo[20];
	addSide(!block3F3L, 72, 8, 55, false, 22, 12, 36);
	const bool block3F2L = (wo[22] && (wo[23] || wo[20])) ||
		(wo[17] && (wo[23] || wo[20]));
	addSide(!block3F2L, 73, 32, 52, false, 24, 10, 34);
	addSide(!wo[27] && !wo[22] && !wo[15], 74, 88, 52, false, 26, 8, 32);
	addSide(!wo[27] && !wo[22] && !wo[15], 75, 128, 52, true, 28, 9, 33);
	const bool block3F3R = (wo[22] && (wo[24] || wo[21])) ||
		(wo[19] && (wo[24] || wo[21])) || wo[14];
	addSide(!block3F3R, 76, 152, 52, true, 30, 11, 35);
	const bool block3F2R = (wo[29] && (wo[27] || wo[14] || wo[24] || wo[10])) ||
		(wo[26] && (wo[27] || wo[14] || wo[24] || wo[10])) ||
		(wo[13] && (wo[27] || wo[14] || wo[24] || wo[10])) ||
		(wo[19] && (wo[27] || wo[24] || wo[10])) || wo[21];
	addSide(!block3F2R, 77, 176, 55, true, 32, 13, 37);
	addSide(!wo[26] && !wo[29] && !wo[21] && !wo[13] && !wo[18] && !wo[31],
		78, 200, 58, true, 34, 15, 39);

	// Depth three front walls.
	addFront(!wo[25] && !wo[28] && !wo[20] && !wo[16],
		87, fwl3, frontFrame3(value(10)), -24, 52, false, 10);
	addFront(!block3F2L, 88, fwl3, frontFrame3(value(12)), 32, 52, false, 12);
	addFront(!wo[22] && !wo[27], 89, fwl3, frontFrame3(value(14)), 88, 52, false, 14);
	const bool blockFront3F1R = (wo[22] && (wo[24] || wo[21])) ||
		(wo[19] && (wo[24] || wo[21]));
	addFront(!blockFront3F1R, 90, fwl3, frontFrame3(value(16)), 144, 52, false, 16);
	addFront(!wo[26] && !wo[29] && !wo[21] && !wo[18],
		91, fwl3, frontFrame3(value(18)), 200, 52, false, 18);

	// Depth two side and front walls.
	// A centered depth-three front wall fully covers this farther side piece.
	addSide(!wo[25] && !wo[28] && !wo[20] && !wo[15],
		107, 8, 48, false, 11, 6, 30);
	addSide(!wo[27] && !wo[22], 108, 64, 40, false, 13, 4, 28);
	addSide(!wo[27] && !wo[22], 109, 144, 40, true, 15, 5, 29);
	addSide(!wo[26] && !wo[29] && !wo[21], 110, 200, 48, true, 17, 7, 31);
	const bool block2F1L = (wo[27] && (wo[25] || wo[28])) ||
		(wo[23] && (wo[25] || wo[28]));
	addFront(!block2F1L, 119, fwl3, frontFrame2(value(5)), -40, 40, false, 5);
	addFront(!wo[27], 120, fwl3, frontFrame2(value(7)), 64, 40, false, 7);
	const bool block2F1R = (wo[27] && (wo[26] || wo[29])) ||
		(wo[24] && (wo[26] || wo[29]));
	addFront(!block2F1R, 121, fwl3, frontFrame2(value(9)), 168, 40, false, 9);

	// Nearest side walls.
	addSide(!wo[27], 133, 32, 24, false, 6, 2, 26);
	addSide(!wo[27], 134, 168, 24, true, 8, 3, 27);

	// Nearest front wall pieces preserve the original resource switches.
	auto addNearLeftOrCenter = [&](bool visible, int order, int x,
			std::size_t sampleIndex) {
		if (!visible)
			return;
		const auto v = value(sampleIndex);
		if (v == 2)
			add(order, fwl2, 9, x, 24, false, sampleIndex);
		else if (v == 1 || v == 3 || v == 4 || v == 5 || v == 8 ||
				v == 9 || v == 12 || v == 13)
			add(order, fwl1, 0, x, 24, false, sampleIndex);
	};
	addNearLeftOrCenter(!wo[28], 143, -136, 0);
	addNearLeftOrCenter(!wo[29], 147, 200, 4);
	if (const auto v = value(2); v != 0) {
		const bool useFwl1 = v == 5 || v == 8 || v == 11 || v == 12;
		int frame = -1;
		switch (v) {
		case 1: case 12: frame = 1; break;
		case 2: frame = 9; break;
		case 3: frame = 3; break;
		case 4: case 5: case 8: case 13: frame = 0; break;
		case 6: frame = 7; break;
		case 7: frame = 8; break;
		case 9: frame = 2; break;
		case 10: case 11: frame = 6; break;
		case 14: frame = 4; break;
		case 15: frame = 5; break;
		default: break;
		}
		if (frame >= 0)
			add(145, useFwl1 ? fwl1 : fwl2, frame, 32, 24, false, 2);
	}

	// Side faces at the camera cell.
	addSide(true, 144, 8, 12, false, 1, 0, 24);
	addSide(true, 146, 200, 12, true, 3, 1, 25);

	std::sort(commands.begin(), commands.end(), [](const auto &left, const auto &right) {
		return left.originalOrder < right.originalOrder;
	});
	return commands;
}

} // namespace mmodern
