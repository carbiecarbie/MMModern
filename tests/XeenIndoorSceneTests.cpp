#include "games/xeen/XeenIndoorScene.h"
#include "games/xeen/XeenIndoorSceneTables.h"
#include "games/xeen/XeenWorld.h"

#include <algorithm>
#include <array>
#include <cstdlib>
#include <iostream>
#include <map>
#include <stdexcept>
#include <utility>

namespace {

void require(bool condition, const char *message) {
	if (!condition)
		throw std::runtime_error(message);
}

mmodern::XeenMap indoorFixture(mmodern::XeenMapIdentity id = 33) {
	mmodern::XeenMap map;
	map.geometry.id = id.number;
	map.side = id.side;
	map.geometry.neighbors = {115, 116, 117, 118};
	for (auto &cell : map.geometry.cells)
		cell.geometry = mmodern::XeenIndoorWalls{{1, 2, 3, 4}};
	return map;
}

mmodern::XeenMap emptyIndoorFixture(std::uint8_t wallKind = 1) {
	mmodern::XeenMap map;
	map.geometry.id = 33;
	map.geometry.wallKind = wallKind;
	for (auto &cell : map.geometry.cells)
		cell.geometry = mmodern::XeenIndoorWalls{};
	return map;
}

mmodern::XeenWorld worldWith(mmodern::XeenMap map) {
	return mmodern::XeenWorld([map = std::move(map)](mmodern::XeenMapIdentity id) {
		if (id != map.geometry.id)
			throw std::runtime_error("unexpected map load");
		return map;
	});
}

void setQueryWall(mmodern::XeenMap &map, const mmodern::XeenCamera &camera,
		std::size_t queryIndex, std::uint8_t value) {
	const auto direction = static_cast<std::size_t>(camera.direction);
	const int x = camera.x +
		mmodern::xeen_indoor_scene_tables::kScreenPositioningX[direction][queryIndex];
	const int y = camera.y +
		mmodern::xeen_indoor_scene_tables::kScreenPositioningY[direction][queryIndex];
	require(x >= 0 && x < 16 && y >= 0 && y < 16, "synthetic query outside fixture");
	const auto shift = mmodern::xeen_indoor_scene_tables::kWallShifts[direction][queryIndex];
	const std::size_t face = shift == 12 ? 0 : shift == 8 ? 1 : shift == 4 ? 2 : 3;
	std::get<mmodern::XeenIndoorWalls>(
		map.geometry.cells[static_cast<std::size_t>(y) * 16 + x].geometry).walls[face] = value;
}

const mmodern::XeenIndoorDrawCommand *byOrder(
		const std::vector<mmodern::XeenIndoorDrawCommand> &commands, int order) {
	const auto found = std::find_if(commands.begin(), commands.end(),
		[order](const auto &command) { return command.originalOrder == order; });
	return found == commands.end() ? nullptr : &*found;
}

void checkDirection(mmodern::XeenDirection direction) {
	int loads = 0;
	mmodern::XeenWorld world([&](mmodern::XeenMapIdentity id) {
		++loads;
		if (id != 33)
			throw std::runtime_error("consulta interior tentou carregar vizinho");
		return indoorFixture();
	});
	const mmodern::XeenCamera camera{33, 8, 8, direction};
	const auto samples = mmodern::XeenIndoorScene().sampleWalls(world, camera);
	require(samples.size() == 44, "numero de consultas interiores incorreto");
	for (std::size_t i = 0; i < samples.size(); ++i) {
		const auto &sample = samples[i];
		require(sample.queryIndex == i, "indice original da consulta nao foi preservado");
		require(sample.sourceMapId == 33 && sample.wallValue.has_value(),
			"consulta central deveria permanecer no mapa interior");
		const unsigned expectedWall = static_cast<unsigned>(sample.sourceFace) + 1;
		require(*sample.wallValue == expectedWall,
			"face direcional nao selecionou o nibble correspondente");
	}
	require(loads == 1 && world.cachedMapCount() == 1,
		"coleta interior recarregou mapa ou carregou vizinho");
}

void testFourDirectionsAndRotatedOffsets() {
	for (const auto direction : {mmodern::XeenDirection::North,
			mmodern::XeenDirection::East, mmodern::XeenDirection::South,
			mmodern::XeenDirection::West})
		checkDirection(direction);

	mmodern::XeenWorld world([](mmodern::XeenMapIdentity) { return indoorFixture(); });
	const auto north = mmodern::XeenIndoorScene().sampleWalls(world,
		{33, 8, 8, mmodern::XeenDirection::North});
	const auto east = mmodern::XeenIndoorScene().sampleWalls(world,
		{33, 8, 8, mmodern::XeenDirection::East});
	const auto south = mmodern::XeenIndoorScene().sampleWalls(world,
		{33, 8, 8, mmodern::XeenDirection::South});
	const auto west = mmodern::XeenIndoorScene().sampleWalls(world,
		{33, 8, 8, mmodern::XeenDirection::West});
	require(north[0].sourceX == 7 && north[0].sourceY == 8 &&
		north[0].sourceFace == mmodern::XeenDirection::North,
		"consulta North 0 divergiu do offset/face original");
	require(north[3].sourceX == 8 && north[3].sourceY == 8 &&
		north[3].sourceFace == mmodern::XeenDirection::East,
		"consulta North 3 divergiu do offset/face original");
	require(north[43].sourceX == 11 && north[43].sourceY == 12 &&
		north[43].sourceFace == mmodern::XeenDirection::East,
		"consulta North 43 divergiu do offset/face original");
	for (std::size_t i = 0; i < north.size(); ++i) {
		const int dx = north[i].sourceX - 8;
		const int dy = north[i].sourceY - 8;
		require(east[i].sourceX - 8 == dy && east[i].sourceY - 8 == -dx,
			"offset East nao e rotacao de North");
		require(south[i].sourceX - 8 == -dx && south[i].sourceY - 8 == -dy,
			"offset South nao e rotacao de North");
		require(west[i].sourceX - 8 == -dy && west[i].sourceY - 8 == dx,
			"offset West nao e rotacao de North");
	}
}

void testBordersDoNotReadOrLoadNeighbors() {
	const std::array<mmodern::XeenCamera, 4> cameras = {{
		{33, 8, 15, mmodern::XeenDirection::North},
		{33, 15, 8, mmodern::XeenDirection::East},
		{33, 8, 0, mmodern::XeenDirection::South},
		{33, 0, 8, mmodern::XeenDirection::West}
	}};
	for (const auto &camera : cameras) {
		std::map<mmodern::XeenMapIdentity, int> loads;
		mmodern::XeenWorld world([&](mmodern::XeenMapIdentity id) {
			++loads[id];
			if (id != 33)
				throw std::runtime_error("borda interior carregou mapa vizinho");
			return indoorFixture();
		});
		const auto samples = mmodern::XeenIndoorScene().sampleWalls(world, camera);
		std::size_t absent = 0;
		for (const auto &sample : samples) {
			if (!sample.wallValue) {
				++absent;
				require(sample.sourceX < 0 || sample.sourceX > 15 ||
					sample.sourceY < 0 || sample.sourceY > 15,
					"consulta local valida perdeu a parede");
			}
		}
		require(absent > 0, "camera na borda deveria produzir consultas ausentes");
		require(loads.size() == 1 && loads[33] == 1 && world.cachedMapCount() == 1,
			"camera na borda carregou vizinho declarado");
	}
}

void testRejectsOutdoorAndInvalidCamera() {
	mmodern::XeenMap outdoor;
	outdoor.geometry.id = 1;
	outdoor.geometry.flags2 = 0x8000;
	for (auto &cell : outdoor.geometry.cells)
		cell.geometry = mmodern::XeenOutdoorLayers{};
	mmodern::XeenWorld outdoorWorld([&](mmodern::XeenMapIdentity) { return outdoor; });
	bool rejected = false;
	try {
		(void)mmodern::XeenIndoorScene().sampleWalls(outdoorWorld,
			{1, 8, 8, mmodern::XeenDirection::North});
	} catch (const std::runtime_error &) {
		rejected = true;
	}
	require(rejected, "cena interior aceitou mapa exterior");

	mmodern::XeenWorld indoorWorld([](mmodern::XeenMapIdentity) { return indoorFixture(); });
	rejected = false;
	try {
		(void)mmodern::XeenIndoorScene().sampleWalls(indoorWorld,
			{33, -1, 8, mmodern::XeenDirection::North});
	} catch (const std::runtime_error &) {
		rejected = true;
	}
	require(rejected, "cena interior aceitou camera fora do mapa");
}

void testFixedSceneAndTerrainResources() {
	const std::array<const char *, 6> prefixes = {
		"town", "cave", "towr", "cstl", "dung", "scfi"
	};
	for (std::size_t kind = 0; kind < prefixes.size(); ++kind) {
		auto world = worldWith(emptyIndoorFixture(static_cast<std::uint8_t>(kind)));
		const auto commands = mmodern::XeenIndoorScene().build(world,
			{33, 8, 8, mmodern::XeenDirection::North});
		require(commands.size() == 4, "empty interior should contain four fixed commands");
		require(commands[0].originalOrder == 0 &&
			commands[0].resourceName == std::string(prefixes[kind]) + ".sky" &&
			commands[0].frame == 0 && commands[0].x == 8 && commands[0].y == 8,
			"top sky command mismatch");
		require(commands[1].originalOrder == 1 && commands[1].frame == 1 &&
			commands[1].x == 8 && commands[1].y == 25,
			"bottom sky command mismatch");
		require(commands[2].originalOrder == 2 &&
			commands[2].resourceName == std::string(prefixes[kind]) + ".gnd" &&
			commands[2].frame == 0 && commands[2].x == 8 && commands[2].y == 67,
			"ground command mismatch");
		require(commands[3].originalOrder == 28 &&
			commands[3].resourceName == "f" + std::string(prefixes[kind]) + "1.fwl" &&
			commands[3].frame == 7 && commands[3].x == 8 && commands[3].y == 64,
			"horizon command mismatch");
	}

	auto invalidWorld = worldWith(emptyIndoorFixture(6));
	bool rejected = false;
	try {
		(void)mmodern::XeenIndoorScene().build(invalidWorld,
			{33, 8, 8, mmodern::XeenDirection::North});
	} catch (const std::runtime_error &) {
		rejected = true;
	}
	require(rejected, "invalid wallKind was accepted");
}

void testStaticWallCommandsAndFrames() {
	const mmodern::XeenCamera camera{33, 8, 8, mmodern::XeenDirection::North};
	auto map = emptyIndoorFixture();
	setQueryWall(map, camera, 2, 2);  // Nearest front-right, fwl2 variation.
	setQueryWall(map, camera, 14, 8); // Intermediate front.
	setQueryWall(map, camera, 22, 8); // Left side.
	setQueryWall(map, camera, 30, 8); // Right side.
	setQueryWall(map, camera, 3, 8);  // Side at camera, flipped.
	map.entities.objects.push_back({8, 8, 0, 0, 1});
	map.entities.monsters.push_back({8, 8, 0, 0, 1});
	auto world = worldWith(std::move(map));
	const auto commands = mmodern::XeenIndoorScene().build(world, camera);

	const auto *near = byOrder(commands, 145);
	const auto *middle = byOrder(commands, 89);
	const auto *left = byOrder(commands, 72);
	const auto *right = byOrder(commands, 76);
	const auto *cameraRight = byOrder(commands, 146);
	require(near && near->resourceName == "fcave2.fwl" && near->frame == 9,
		"nearest front wall/resource variation mismatch");
	require(middle && middle->resourceName == "fcave3.fwl" && middle->frame == 17,
		"intermediate front wall mismatch");
	require(left && left->resourceName == "scave.swl" && !left->options.horizontalFlip,
		"left side wall mismatch");
	require(right && right->resourceName == "scave.swl" && right->options.horizontalFlip,
		"right side wall/flip mismatch");
	require(cameraRight && cameraRight->frame == 1 &&
		cameraRight->options.horizontalFlip, "camera right side wall mismatch");
	require(std::is_sorted(commands.begin(), commands.end(), [](const auto &a, const auto &b) {
		return a.originalOrder < b.originalOrder;
	}), "indoor commands are not in original draw order");
	require(std::all_of(commands.begin(), commands.end(), [](const auto &command) {
		return command.options.scaleIndex == 0 && command.options.sceneClipped &&
			!command.options.bottomClipped;
	}), "static indoor draw options mismatch");
	require(std::none_of(commands.begin(), commands.end(), [](const auto &command) {
		return command.resourceName.find("object") != std::string::npos ||
			command.resourceName.find("monster") != std::string::npos;
	}), "entities leaked into static indoor commands");

	auto farMap = emptyIndoorFixture();
	setQueryWall(farMap, camera, 27, 1);
	auto farWorld = worldWith(std::move(farMap));
	const auto farCommands = mmodern::XeenIndoorScene().build(farWorld, camera);
	const auto *far = byOrder(farCommands, 41);
	require(far && far->resourceName == "fcave4.fwl" && far->frame == 8,
		"distant front wall mismatch");

	auto differentMap = emptyIndoorFixture();
	setQueryWall(differentMap, camera, 14, 1);
	auto differentWorld = worldWith(std::move(differentMap));
	const auto different = mmodern::XeenIndoorScene().build(differentWorld, camera);
	require(byOrder(different, 89) && byOrder(different, 89)->frame == 25,
		"different wall nibble did not select a different frame");
}

void testOcclusionDirectionsAndBorders() {
	const mmodern::XeenCamera northCamera{33, 8, 8, mmodern::XeenDirection::North};
	auto blockedMap = emptyIndoorFixture();
	setQueryWall(blockedMap, northCamera, 7, 8);  // Near centered front.
	setQueryWall(blockedMap, northCamera, 14, 8); // Farther centered front.
	auto blockedWorld = worldWith(std::move(blockedMap));
	const auto blocked = mmodern::XeenIndoorScene().build(blockedWorld, northCamera);
	require(byOrder(blocked, 120) && !byOrder(blocked, 89),
		"near wall did not occlude farther centered wall");

	for (const auto direction : {mmodern::XeenDirection::North,
			mmodern::XeenDirection::East, mmodern::XeenDirection::South,
			mmodern::XeenDirection::West}) {
		const mmodern::XeenCamera camera{33, 8, 8, direction};
		auto map = emptyIndoorFixture();
		setQueryWall(map, camera, 14, 8);
		auto world = worldWith(std::move(map));
		const auto commands = mmodern::XeenIndoorScene().build(world, camera);
		const auto *front = byOrder(commands, 89);
		require(front && front->sourceFace == direction,
			"front wall face did not rotate with camera");
	}

	const std::array<mmodern::XeenCamera, 4> edges = {{
		{33, 8, 15, mmodern::XeenDirection::North},
		{33, 15, 8, mmodern::XeenDirection::East},
		{33, 8, 0, mmodern::XeenDirection::South},
		{33, 0, 8, mmodern::XeenDirection::West}
	}};
	for (const auto &camera : edges) {
		int loads = 0;
		mmodern::XeenWorld world([&](mmodern::XeenMapIdentity id) {
			++loads;
			if (id != 33)
				throw std::runtime_error("indoor build loaded neighbor");
			return emptyIndoorFixture();
		});
		const auto commands = mmodern::XeenIndoorScene().build(world, camera);
		require(commands.size() >= 4 && loads == 1 && world.cachedMapCount() == 1,
			"indoor edge build failed or loaded neighbor");
	}
}

} // namespace

int main() {
	try {
		testFourDirectionsAndRotatedOffsets();
		testBordersDoNotReadOrLoadNeighbors();
		testRejectsOutdoorAndInvalidCamera();
		testFixedSceneAndTerrainResources();
		testStaticWallCommandsAndFrames();
		testOcclusionDirectionsAndBorders();
		std::cout << "Xeen indoor directional wall sampling tests passed\n";
		return EXIT_SUCCESS;
	} catch (const std::exception &error) {
		std::cerr << "Xeen indoor scene tests failed: " << error.what() << '\n';
		return EXIT_FAILURE;
	}
}
