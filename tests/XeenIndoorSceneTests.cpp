#include "games/xeen/XeenIndoorScene.h"
#include "games/xeen/XeenIndoorSceneTables.h"
#include "games/xeen/XeenWorld.h"
#include "XeenIndoorObjectTestOracle.h"

#include <algorithm>
#include <array>
#include <cstdlib>
#include <iostream>
#include <map>
#include <stdexcept>
#include <utility>
#include <vector>

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

mmodern::XeenObjectFile objectFile(mmodern::XeenMapIdentity id,
		std::vector<mmodern::XeenMapEntity> records) {
	mmodern::XeenObjectFile file;
	file.mapId = id;
	file.resourceName = "synthetic.mob";
	file.resourcePresent = true;
	file.entities.objects = std::move(records);
	return file;
}

mmodern::XeenWorld worldWith(mmodern::XeenMap map, mmodern::XeenObjectFile objects) {
	return mmodern::XeenWorld(
		[map = std::move(map)](mmodern::XeenMapIdentity id) {
			if (id != map.identity())
				throw std::runtime_error("unexpected map load");
			return map;
		},
		[objects = std::move(objects)](mmodern::XeenMapIdentity id) {
			if (id != objects.mapId)
				throw std::runtime_error("unexpected object load");
			return objects;
		});
}

mmodern::XeenObjectVisualResolver objectResolver() {
	std::vector<std::uint8_t> bytes(1452);
	for (const int resource : {111, 113}) {
		for (std::size_t relative = 0; relative < 4; ++relative) {
			bytes[static_cast<std::size_t>(resource) * 12 + relative] =
				static_cast<std::uint8_t>(relative);
			bytes[static_cast<std::size_t>(resource) * 12 + 4 + relative] =
				static_cast<std::uint8_t>(relative % 2);
			bytes[static_cast<std::size_t>(resource) * 12 + 8 + relative] =
				static_cast<std::uint8_t>(relative + 1);
		}
	}
	for (std::size_t relative = 0; relative < 4; ++relative)
		bytes[110 * 12 + 8 + relative] = 3;
	return mmodern::XeenObjectVisualResolver(
		mmodern::XeenCloudsVisualMetadata::parse(bytes));
}

std::vector<const mmodern::XeenIndoorDrawCommand *> objectCommands(
		const std::vector<mmodern::XeenIndoorDrawCommand> &commands) {
	std::vector<const mmodern::XeenIndoorDrawCommand *> result;
	for (const auto &command : commands)
		if (command.object()) result.push_back(&command);
	return result;
}

const mmodern::XeenIndoorDrawCommand *objectByIdentity(
		const std::vector<mmodern::XeenIndoorDrawCommand> &commands,
		mmodern::XeenObjectIdentity identity) {
	const auto found = std::find_if(commands.begin(), commands.end(), [&](const auto &command) {
		return command.object() && command.object()->visual.identity == identity;
	});
	return found == commands.end() ? nullptr : &*found;
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

void setMazeBit(mmodern::XeenMap &map, const mmodern::XeenCamera &camera, int bit) {
	struct Source { int bit; std::size_t query; std::uint8_t value; };
	static constexpr Source sources[] = {
		{27,2,5}, {25,0,5}, {28,1,1}, {23,6,1}, {26,4,5}, {29,3,1},
		{24,8,1}, {22,7,5}, {20,5,5}, {17,13,1}, {21,9,5}, {19,15,1},
		{15,14,5}, {12,12,5}, {7,26,1}, {9,28,1}, {8,24,1},
		{14,16,5}, {10,30,1}
	};
	const auto found = std::find_if(std::begin(sources), std::end(sources),
		[bit](const Source &source) { return source.bit == bit; });
	require(found != std::end(sources), "missing raw-wall source for maze bit");
	setQueryWall(map, camera, found->query, found->value);
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
			commands[0].geometry().resourceName == std::string(prefixes[kind]) + ".sky" &&
			commands[0].geometry().frame == 0 && commands[0].x == 8 && commands[0].y == 8,
			"top sky command mismatch");
		require(commands[1].originalOrder == 1 && commands[1].geometry().frame == 1 &&
			commands[1].x == 8 && commands[1].y == 25,
			"bottom sky command mismatch");
		require(commands[2].originalOrder == 2 &&
			commands[2].geometry().resourceName == std::string(prefixes[kind]) + ".gnd" &&
			commands[2].geometry().frame == 0 && commands[2].x == 8 && commands[2].y == 67,
			"ground command mismatch");
		require(commands[3].originalOrder == 28 &&
			commands[3].geometry().resourceName == "f" + std::string(prefixes[kind]) + "1.fwl" &&
			commands[3].geometry().frame == 7 && commands[3].x == 8 && commands[3].y == 64,
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
	require(near && near->geometry().resourceName == "fcave2.fwl" && near->geometry().frame == 9,
		"nearest front wall/resource variation mismatch");
	require(middle && middle->geometry().resourceName == "fcave3.fwl" && middle->geometry().frame == 17,
		"intermediate front wall mismatch");
	require(left && left->geometry().resourceName == "scave.swl" && !left->drawOptions().horizontalFlip,
		"left side wall mismatch");
	require(right && right->geometry().resourceName == "scave.swl" && right->drawOptions().horizontalFlip,
		"right side wall/flip mismatch");
	require(cameraRight && cameraRight->geometry().frame == 1 &&
		cameraRight->drawOptions().horizontalFlip, "camera right side wall mismatch");
	require(std::is_sorted(commands.begin(), commands.end(), [](const auto &a, const auto &b) {
		return a.originalOrder < b.originalOrder;
	}), "indoor commands are not in original draw order");
	require(std::all_of(commands.begin(), commands.end(), [](const auto &command) {
		const auto options = command.drawOptions();
		return options.scaleIndex == 0 && options.sceneClipped &&
			!options.bottomClipped;
	}), "static indoor draw options mismatch");
	require(std::none_of(commands.begin(), commands.end(), [](const auto &command) {
		return command.object() ||
			command.geometry().resourceName.find("object") != std::string::npos ||
			command.geometry().resourceName.find("monster") != std::string::npos;
	}), "entities leaked into static indoor commands");

	auto farMap = emptyIndoorFixture();
	setQueryWall(farMap, camera, 27, 1);
	auto farWorld = worldWith(std::move(farMap));
	const auto farCommands = mmodern::XeenIndoorScene().build(farWorld, camera);
	const auto *far = byOrder(farCommands, 41);
	require(far && far->geometry().resourceName == "fcave4.fwl" && far->geometry().frame == 8,
		"distant front wall mismatch");

	auto differentMap = emptyIndoorFixture();
	setQueryWall(differentMap, camera, 14, 1);
	auto differentWorld = worldWith(std::move(differentMap));
	const auto different = mmodern::XeenIndoorScene().build(differentWorld, camera);
	require(byOrder(different, 89) && byOrder(different, 89)->geometry().frame == 25,
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

void testObjectProjectionAndGeometryOnlyCompatibility() {
	using namespace mmodern;
	using namespace indoor_object_test;
	const auto resolver = objectResolver();
	for (const int resource : {111, 113}) {
		for (const auto direction : {XeenDirection::North, XeenDirection::East,
				XeenDirection::South, XeenDirection::West}) {
			const XeenCamera camera{33, 8, 8, direction};
			std::vector<XeenMapEntity> records;
			const auto directionIndex = static_cast<std::size_t>(direction);
			for (const auto &placement : kPlacementSpecs) {
				const auto source = expectedSource(camera, placement);
				records.push_back({source.x, source.y, 0, 0, resource});
			}
			records.push_back({1, 1, 0, 0, resource});
			auto map = emptyIndoorFixture();
			auto world = worldWith(map, objectFile(33, records));
			const auto geometryOnly = XeenIndoorScene().build(world, camera);
			std::vector<XeenObjectVisual> diagnostics;
			const auto combined = XeenIndoorScene().build(world, camera, &resolver, &diagnostics);
			const auto objects = objectCommands(combined);
			require(objects.size() == kPlacementSpecs.size() && diagnostics.empty(),
				"all indoor object placements were not emitted");
			for (std::size_t recordIndex = 0; recordIndex < kPlacementSpecs.size(); ++recordIndex) {
				const auto &placement = kPlacementSpecs[recordIndex];
				const auto source = expectedSource(camera, placement);
				const auto *command = objectByIdentity(combined, {33, recordIndex});
				require(command && command->queryIndex == placement.query &&
					command->originalOrder == placement.order &&
					command->sourceMapId == XeenMapIdentity{33} &&
					command->sourceX == source.x && command->sourceY == source.y,
					"indoor object query/order/source mismatch");
				const auto options = command->drawOptions();
				const int expectedX = resource == 113 ? placement.resource113X : placement.normalX;
				const int expectedY = resource == 113 ? placement.resource113Y : placement.normalY;
				require(command->x == expectedX && command->y == expectedY &&
					options.scaleIndex == placement.scale && options.sceneClipped &&
					options.bottomClipped == placement.bottomClipped && !options.enlarge,
					"indoor object anchor/scale/clipping mismatch");
				const auto &visual = command->object()->visual;
				require(visual.identity == XeenObjectIdentity{33, recordIndex} &&
					visual.status == XeenObjectVisualStatus::SupportedStatic &&
					visual.frame == directionIndex &&
					visual.horizontalFlip == (directionIndex % 2 != 0),
					"indoor directional appearance mismatch");
			}
			require(std::is_sorted(combined.begin(), combined.end(),
				[](const auto &left, const auto &right) {
					return left.originalOrder < right.originalOrder;
				}), "indoor geometry/object stream is not ordered");
			std::vector<const XeenIndoorDrawCommand *> filtered;
			for (const auto &command : combined)
				if (!command.object()) filtered.push_back(&command);
			require(filtered.size() == geometryOnly.size(),
				"object-enabled build changed geometry count");
			for (std::size_t i = 0; i < geometryOnly.size(); ++i) {
				const auto &expected = geometryOnly[i];
				const auto &actual = *filtered[i];
				require(actual.originalOrder == expected.originalOrder &&
					actual.geometry().resourceName == expected.geometry().resourceName &&
					actual.geometry().frame == expected.geometry().frame &&
					actual.x == expected.x && actual.y == expected.y &&
					actual.sourceMapId == expected.sourceMapId &&
					actual.sourceX == expected.sourceX && actual.sourceY == expected.sourceY &&
					actual.sourceFace == expected.sourceFace &&
					actual.drawOptions().scaleIndex == expected.drawOptions().scaleIndex &&
					actual.drawOptions().horizontalFlip == expected.drawOptions().horizontalFlip &&
					actual.drawOptions().sceneClipped == expected.drawOptions().sceneClipped &&
					actual.drawOptions().bottomClipped == expected.drawOptions().bottomClipped,
					"object-enabled build changed a geometry command");
			}
		}
	}
}

void testResource113AnchorUsesResolvedResourceOnly() {
	using namespace mmodern;
	using namespace indoor_object_test;
	const auto resolver = objectResolver();
	const XeenCamera camera{33,8,8,XeenDirection::North};
	const auto &placement = *specForQuery(14);
	const auto source = expectedSource(camera, placement);
	auto assertNormalAnchor = [&](std::vector<XeenMapEntity> records,
			XeenObjectIdentity identity, const char *message) {
		auto world = worldWith(emptyIndoorFixture(), objectFile(33, std::move(records)));
		const auto commands = XeenIndoorScene().build(world, camera, &resolver);
		const auto *command = objectByIdentity(commands, identity);
		require(command && command->queryIndex == placement.query &&
			command->x == placement.normalX && command->y == placement.normalY,
			message);
	};
	assertNormalAnchor({{source.x,source.y,113,0,111}}, {33,0},
		"MOB table slot 113 selected the exceptional anchor");
	std::vector<XeenMapEntity> highIndex(114, {1,1,0,0,-1});
	highIndex[113] = {source.x,source.y,0,0,111};
	assertNormalAnchor(std::move(highIndex), {33,113},
		"original record index 113 selected the exceptional anchor");
}

void testObjectWallPredicatesFromRawSamples() {
	using namespace mmodern;
	using namespace mmodern::xeen_indoor_scene_tables;
	const auto resolver = objectResolver();
	const XeenCamera camera{33, 8, 8, XeenDirection::North};
	struct PredicateCase { std::size_t query; std::vector<std::vector<int>> terms; };
	const std::vector<PredicateCase> cases = {
		{2, {}}, {7, {{27}}},
		{5, {{27,25},{27,28},{23,25},{23,28}}},
		{9, {{27,26},{27,29},{24,26},{24,29}}},
		{14, {{22},{27}}},
		{12, {{27},{22,23},{22,20},{23,17},{20,17}}},
		{16, {{27},{22,24},{22,21},{24,19},{21,19}}},
		{27, {{27},{22},{15}}},
		{25, {{27},{22},{15,17},{15,12},{12,7},{17,7}}},
		{29, {{27},{15,19},{15,14},{14,9},{19,9}}},
		{23, {{27},{22,20},{22,23},{20,17},{23,17},{12},{8}}},
		{31, {{27},{22,21},{22,24},{21,19},{24,19},{14},{10}}}
	};
	auto present = [&](std::size_t query, const std::vector<int> &bits) {
		using namespace indoor_object_test;
		auto map = emptyIndoorFixture();
		for (const int bit : bits) setMazeBit(map, camera, bit);
		const auto *placement = specForQuery(static_cast<int>(query));
		require(placement != nullptr, "unknown object predicate query");
		const auto source = expectedSource(camera, *placement);
		auto world = worldWith(std::move(map),
			objectFile(33, {{source.x,source.y,0,0,111}}));
		return objectByIdentity(XeenIndoorScene().build(world, camera, &resolver), {33,0}) != nullptr;
	};
	for (const auto &test : cases) {
		require(present(test.query, {}), "open indoor object predicate rejected placement");
		for (const auto &term : test.terms) {
			require(!present(test.query, term), "indoor object blocking term was omitted");
			if (term.size() == 2) {
				require(present(test.query, {term[0]}) && present(test.query, {term[1]}),
					"single conjunct incorrectly blocked indoor object");
			}
		}
	}
	require(present(29, {22}), "query 29 acquired a standalone W22 blocker");
	for (const auto value : {std::uint8_t{0}, std::uint8_t{1}, std::uint8_t{2},
			std::uint8_t{5}, std::uint8_t{8}}) {
		auto map = emptyIndoorFixture();
		setQueryWall(map, camera, 2, value);
		const int x = camera.x + kScreenPositioningX[0][7];
		const int y = camera.y + kScreenPositioningY[0][7];
		auto world = worldWith(std::move(map), objectFile(33, {{x,y,0,0,111}}));
		const bool visible = objectByIdentity(
			XeenIndoorScene().build(world, camera, &resolver), {33,0}) != nullptr;
		require(visible == (value != 5 && value != 8),
			"front-wall nibble collapsed to a nonzero blocker");
	}
	// Query 2 is in the camera cell; its back wall never rejects the object.
	auto behind = emptyIndoorFixture();
	setQueryWall(behind, camera, 2, 8);
	auto behindWorld = worldWith(std::move(behind), objectFile(33, {{8,8,0,0,111}}));
	require(objectByIdentity(XeenIndoorScene().build(behindWorld, camera, &resolver), {33,0}),
		"wall behind same-cell object rejected query 2");
}

void testObjectIdentityPrecedenceAndBoundaries() {
	using namespace mmodern;
	const auto resolver = objectResolver();
	const XeenCamera camera{33, 8, 8, XeenDirection::North};
	auto makeWorld = [&](std::vector<XeenMapEntity> records,
			XeenMapIdentity id = XeenMapIdentity{33}) {
		auto map = emptyIndoorFixture();
		map.side = id.side;
		map.geometry.id = id.number;
		return worldWith(std::move(map), objectFile(id, std::move(records)));
	};
	for (const int firstResource : {110, 121}) {
		auto world = makeWorld({{8,8,0,0,firstResource},{8,8,0,0,111}});
		std::vector<XeenObjectVisual> diagnostics;
		const auto commands = XeenIndoorScene().build(world, camera, &resolver, &diagnostics);
		require(!objectByIdentity(commands, {33,0}) && !objectByIdentity(commands, {33,1}) &&
			diagnostics.size() == 1 && diagnostics[0].identity == XeenObjectIdentity{33,0} &&
			diagnostics[0].status == (firstResource == 110 ?
				XeenObjectVisualStatus::UnsupportedAnimation : XeenObjectVisualStatus::Invalid),
			"unsupported first indoor record promoted overlap");
		require(world.selectObject(camera) == XeenObjectIdentity{33,0},
			"rendering changed gameplay object selection");
	}
	{
		auto world = makeWorld({{8,8,0,4,111},{8,8,0,0,111}});
		std::vector<XeenObjectVisual> diagnostics;
		const auto commands = XeenIndoorScene().build(world, camera, &resolver, &diagnostics);
		require(!objectByIdentity(commands, {33,0}) && !objectByIdentity(commands, {33,1}) &&
			diagnostics.size() == 1 &&
			diagnostics[0].status == XeenObjectVisualStatus::Invalid,
			"malformed first indoor direction promoted overlap");
	}
	{
		auto world = makeWorld({{8,8,0,0,-1},{8,8,0,0,111}});
		const auto commands = XeenIndoorScene().build(world, camera, &resolver);
		require(objectByIdentity(commands, {33,1}),
			"unresolved record occupied indoor position");
	}
	{
		auto world = makeWorld({{-128,8,0,0,111},{8,8,0,0,111}});
		const auto commands = XeenIndoorScene().build(world, camera, &resolver);
		require(objectByIdentity(commands, {33,1}),
			"base-disabled record occupied indoor position");
	}
	{
		auto world = makeWorld({{8,8,0,0,111},{8,8,0,0,113},{9,9,0,0,111}});
		const auto first = XeenIndoorScene().build(world, camera, &resolver);
		require(objectByIdentity(first, {33,0}) && !objectByIdentity(first, {33,1}),
			"first eligible indoor record lost precedence");
		world.disableObject({33,0});
		const auto second = XeenIndoorScene().build(world, camera, &resolver);
		require(!objectByIdentity(second, {33,0}) && objectByIdentity(second, {33,1}) &&
			objectByIdentity(second, {33,2}),
			"session disable did not reveal next record or preserve shared resource");
		world.discardMapCache();
		const auto rebuilt = XeenIndoorScene().build(world, camera, &resolver);
		require(objectByIdentity(rebuilt, {33,1}) && objectByIdentity(rebuilt, {33,2}),
			"cache reconstruction changed indoor disabled identity");
		const auto repeated = XeenIndoorScene().build(world, camera, &resolver);
		const auto rebuiltObjects = objectCommands(rebuilt);
		const auto repeatedObjects = objectCommands(repeated);
		require(rebuiltObjects.size() == repeatedObjects.size(),
			"repeated indoor build changed object count");
		for (std::size_t i = 0; i < rebuiltObjects.size(); ++i) {
			const auto &left = *rebuiltObjects[i];
			const auto &right = *repeatedObjects[i];
			require(left.originalOrder == right.originalOrder && left.x == right.x &&
				left.y == right.y && left.queryIndex == right.queryIndex &&
				left.object()->visual.identity == right.object()->visual.identity,
				"repeated indoor build changed command identity or order");
		}
	}
	{
		XeenObjectFile absent; absent.mapId = 33;
		auto world = worldWith(emptyIndoorFixture(), absent);
		std::vector<XeenObjectVisual> diagnostics{{}};
		const auto commands = XeenIndoorScene().build(world, camera, &resolver, &diagnostics);
		require(objectCommands(commands).empty() && diagnostics.empty(),
			"missing MOB fabricated indoor objects or diagnostics");
	}
	{
		const XeenMapIdentity dark{XeenSide::Darkside, 33};
		auto world = makeWorld({{8,8,0,0,111}}, dark);
		std::vector<XeenObjectVisual> diagnostics;
		const auto commands = XeenIndoorScene().build(
			world, {dark,8,8,XeenDirection::North}, &resolver, &diagnostics);
		require(objectCommands(commands).empty() && diagnostics.size() == 1 &&
			diagnostics[0].status == XeenObjectVisualStatus::UnsupportedSide,
			"unsupported side produced indoor object");
	}
	{
		using namespace mmodern::xeen_indoor_scene_tables;
		const XeenCamera edgeCamera{33,8,15,XeenDirection::North};
		const std::size_t direction = static_cast<std::size_t>(edgeCamera.direction);
		const int outsideX = edgeCamera.x + kScreenPositioningX[direction][27];
		const int outsideY = edgeCamera.y + kScreenPositioningY[direction][27];
		auto world = makeWorld({{outsideX,outsideY,0,0,111},{15,15,0,0,111}});
		const auto commands = XeenIndoorScene().build(
			world, edgeCamera, &resolver);
		require(objectCommands(commands).empty(),
			"out-of-domain or unrelated indoor record wrapped into view");
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
		testObjectProjectionAndGeometryOnlyCompatibility();
		testResource113AnchorUsesResolvedResourceOnly();
		testObjectWallPredicatesFromRawSamples();
		testObjectIdentityPrecedenceAndBoundaries();
		std::cout << "Xeen indoor geometry and static object tests passed\n";
		return EXIT_SUCCESS;
	} catch (const std::exception &error) {
		std::cerr << "Xeen indoor scene tests failed: " << error.what() << '\n';
		return EXIT_FAILURE;
	}
}
