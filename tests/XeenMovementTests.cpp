#include "games/xeen/XeenMovement.h"
#include "games/xeen/XeenWorld.h"

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

bool sameCamera(const mmodern::XeenCamera &a, const mmodern::XeenCamera &b) {
	return a.mapId == b.mapId && a.x == b.x && a.y == b.y && a.direction == b.direction;
}

mmodern::XeenMap freeMap(mmodern::XeenMapIdentity id = 1) {
	mmodern::XeenMap map;
	map.geometry.id = id.number;
	map.side = id.side;
	map.geometry.flags2 = 0x8000;
	map.geometry.surfaceTypes[6] = 6;
	for (auto &cell : map.geometry.cells) {
		mmodern::XeenOutdoorLayers layers;
		layers.surface = 6;
		cell.geometry = layers;
	}
	return map;
}

mmodern::XeenWorld worldWith(std::map<mmodern::XeenMapIdentity, mmodern::XeenMap> maps) {
	return mmodern::XeenWorld([maps = std::move(maps)](mmodern::XeenMapIdentity id) {
		const auto found = maps.find(id);
		if (found == maps.end())
			throw std::runtime_error("fixture de mapa ausente");
		return found->second;
	});
}

mmodern::XeenOutdoorLayers &layersAt(mmodern::XeenMap &map, int x, int y) {
	return std::get<mmodern::XeenOutdoorLayers>(
		map.geometry.cells[static_cast<std::size_t>(y) * 16 + x].geometry);
}

mmodern::XeenMap freeIndoorMap(int wallNoPass = 7) {
	mmodern::XeenMap map;
	map.geometry.id = 33;
	map.geometry.neighbors = {115, 116, 117, 118};
	map.geometry.difficulties[0] = wallNoPass;
	for (auto &cell : map.geometry.cells)
		cell.geometry = mmodern::XeenIndoorWalls{};
	return map;
}

mmodern::XeenMapCell &indoorCellAt(mmodern::XeenMap &map, int x, int y) {
	return map.geometry.cells[static_cast<std::size_t>(y) * 16 + x];
}

void setIndoorWall(mmodern::XeenMap &map, int x, int y,
		mmodern::XeenDirection direction, std::uint8_t value) {
	std::get<mmodern::XeenIndoorWalls>(indoorCellAt(map, x, y).geometry)
		.walls[static_cast<std::size_t>(direction)] = value;
}

void testRotations() {
	auto world = worldWith({{1, freeMap()}});
	const mmodern::XeenMovement movement;
	mmodern::XeenCamera camera{1, 9, 6, mmodern::XeenDirection::North};
	const std::array<mmodern::XeenDirection, 4> right = {
		mmodern::XeenDirection::East, mmodern::XeenDirection::South,
		mmodern::XeenDirection::West, mmodern::XeenDirection::North
	};
	for (const auto expected : right) {
		require(movement.apply(world, camera, mmodern::NavigationAction::TurnRight) ==
			mmodern::XeenMovementResult::Turned, "rotacao direita nao aplicada");
		require(camera.direction == expected && camera.x == 9 && camera.y == 6 &&
			camera.mapId == 1, "rotacao direita alterou estado indevido");
	}
	const std::array<mmodern::XeenDirection, 4> left = {
		mmodern::XeenDirection::West, mmodern::XeenDirection::South,
		mmodern::XeenDirection::East, mmodern::XeenDirection::North
	};
	for (const auto expected : left) {
		require(movement.apply(world, camera, mmodern::NavigationAction::TurnLeft) ==
			mmodern::XeenMovementResult::Turned, "rotacao esquerda nao aplicada");
		require(camera.direction == expected, "sequencia de rotacao esquerda incorreta");
	}
}

void testForwardAndBackward() {
	auto world = worldWith({{1, freeMap()}});
	const mmodern::XeenMovement movement;
	struct Expected { mmodern::XeenDirection direction; int dx; int dy; };
	const Expected expected[] = {
		{mmodern::XeenDirection::North, 0, 1},
		{mmodern::XeenDirection::East, 1, 0},
		{mmodern::XeenDirection::South, 0, -1},
		{mmodern::XeenDirection::West, -1, 0}
	};
	for (const auto &item : expected) {
		mmodern::XeenCamera forward{1, 8, 8, item.direction};
		require(movement.apply(world, forward, mmodern::NavigationAction::MoveForward) ==
			mmodern::XeenMovementResult::Moved, "movimento para frente bloqueado");
		require(forward.x == 8 + item.dx && forward.y == 8 + item.dy &&
			forward.direction == item.direction, "delta para frente incorreto");

		mmodern::XeenCamera backward{1, 8, 8, item.direction};
		require(movement.apply(world, backward, mmodern::NavigationAction::MoveBackward) ==
			mmodern::XeenMovementResult::Moved, "movimento para tras bloqueado");
		require(backward.x == 8 - item.dx && backward.y == 8 - item.dy &&
			backward.direction == item.direction, "delta para tras incorreto");
	}
}

void testNeighborTransitions() {
	auto map1 = freeMap(1);
	auto map2 = freeMap(2);
	auto map5 = freeMap(5);
	map1.geometry.neighbors = {0, 5, 2, 0};
	map5.geometry.neighbors[3] = 1;
	map2.geometry.neighbors[0] = 1;
	auto world = worldWith({{1, map1}, {2, map2}, {5, map5}});
	const mmodern::XeenMovement movement;

	mmodern::XeenCamera east{1, 15, 6, mmodern::XeenDirection::East};
	require(movement.apply(world, east, mmodern::NavigationAction::MoveForward) ==
		mmodern::XeenMovementResult::Moved && east.mapId == 5 && east.x == 0 && east.y == 6 &&
		east.direction == mmodern::XeenDirection::East, "transicao 001 -> 005 incorreta");
	east.direction = mmodern::XeenDirection::West;
	require(movement.apply(world, east, mmodern::NavigationAction::MoveForward) ==
		mmodern::XeenMovementResult::Moved && east.mapId == 1 && east.x == 15 && east.y == 6 &&
		east.direction == mmodern::XeenDirection::West,
		"transicao 005 -> 001 incorreta");

	mmodern::XeenCamera south{1, 9, 0, mmodern::XeenDirection::South};
	require(movement.apply(world, south, mmodern::NavigationAction::MoveForward) ==
		mmodern::XeenMovementResult::Moved && south.mapId == 2 && south.x == 9 && south.y == 15 &&
		south.direction == mmodern::XeenDirection::South, "transicao 001 -> 002 incorreta");
	south.direction = mmodern::XeenDirection::North;
	require(movement.apply(world, south, mmodern::NavigationAction::MoveForward) ==
		mmodern::XeenMovementResult::Moved && south.mapId == 1 && south.x == 9 && south.y == 0 &&
		south.direction == mmodern::XeenDirection::North,
		"transicao 002 -> 001 incorreta");
}

void testBlockedDestinationIsAtomic() {
	auto map1 = freeMap(1);
	auto map5 = freeMap(5);
	map1.geometry.neighbors[1] = 5;
	map5.geometry.neighbors[3] = 1;
	layersAt(map5, 0, 6).middle = 1;
	auto world = worldWith({{1, map1}, {5, map5}});
	const mmodern::XeenMovement movement;
	mmodern::XeenCamera camera{1, 15, 6, mmodern::XeenDirection::East};
	const auto before = camera;
	require(movement.apply(world, camera, mmodern::NavigationAction::MoveForward) ==
		mmodern::XeenMovementResult::BlockedByTerrain, "terreno do mapa destino nao bloqueou");
	require(sameCamera(camera, before), "bloqueio no destino alterou parcialmente a camera");

	auto water1 = freeMap(1);
	auto water5 = freeMap(5);
	water1.geometry.neighbors[1] = 5;
	water5.geometry.neighbors[3] = 1;
	water5.geometry.surfaceTypes[5] = 0;
	layersAt(water5, 0, 6).surface = 5;
	auto waterWorld = worldWith({{1, water1}, {5, water5}});
	camera = before;
	require(movement.apply(waterWorld, camera, mmodern::NavigationAction::MoveForward) ==
		mmodern::XeenMovementResult::BlockedBySurface, "superficie do mapa destino nao bloqueou");
	require(sameCamera(camera, before), "bloqueio de superficie alterou parcialmente a camera");
}

void testTerrainAndSurfaceCollision() {
	const mmodern::XeenMovement movement;
	auto mountainMap = freeMap();
	layersAt(mountainMap, 9, 5).middle = 1;
	auto world = worldWith({{1, mountainMap}});
	mmodern::XeenCamera camera{1, 9, 6, mmodern::XeenDirection::South};
	const auto before = camera;
	require(movement.apply(world, camera, mmodern::NavigationAction::MoveForward) ==
		mmodern::XeenMovementResult::BlockedByTerrain, "montanha deveria bloquear");
	require(sameCamera(camera, before), "montanha alterou a camera bloqueada");

	for (const std::uint8_t surface : {std::uint8_t{0}, std::uint8_t{8}, std::uint8_t{15}}) {
		auto surfaceMap = freeMap();
		surfaceMap.geometry.surfaceTypes[5] = surface;
		layersAt(surfaceMap, 9, 5).surface = 5;
		auto surfaceWorld = worldWith({{1, surfaceMap}});
		camera = before;
		require(movement.apply(surfaceWorld, camera, mmodern::NavigationAction::MoveForward) ==
			mmodern::XeenMovementResult::BlockedBySurface, "superficie deveria bloquear");
		require(sameCamera(camera, before), "superficie alterou a camera bloqueada");
	}

	camera = before;
	require(movement.apply(world, camera, mmodern::NavigationAction::MoveBackward) ==
		mmodern::XeenMovementResult::Moved && camera.x == 9 && camera.y == 7,
		"passo inicial para tras deveria chegar a (9,7)");

	const std::array<std::uint8_t, 5> blockedTerrain = {1, 7, 9, 10, 12};
	for (const auto terrain : blockedTerrain) {
		auto map = freeMap();
		layersAt(map, 9, 5).middle = terrain;
		auto terrainWorld = worldWith({{1, map}});
		camera = before;
		require(movement.apply(terrainWorld, camera, mmodern::NavigationAction::MoveForward) ==
			mmodern::XeenMovementResult::BlockedByTerrain, "terreno intransponivel nao bloqueou");
	}

	const std::array<std::uint8_t, 8> surfaceCheckedTerrain = {0, 2, 4, 5, 8, 11, 13, 14};
	for (const auto terrain : surfaceCheckedTerrain) {
		auto map = freeMap();
		map.geometry.surfaceTypes[5] = 0;
		auto &layers = layersAt(map, 9, 5);
		layers.middle = terrain;
		layers.surface = 5;
		auto terrainWorld = worldWith({{1, map}});
		camera = before;
		require(movement.apply(terrainWorld, camera, mmodern::NavigationAction::MoveForward) ==
			mmodern::XeenMovementResult::BlockedBySurface,
			"terreno deveria consultar a superficie de agua");
	}

	const std::array<std::uint8_t, 3> passableTerrain = {3, 6, 15};
	for (const auto terrain : passableTerrain) {
		auto map = freeMap();
		map.geometry.surfaceTypes[5] = 0;
		auto &layers = layersAt(map, 9, 5);
		layers.middle = terrain;
		layers.surface = 5;
		auto terrainWorld = worldWith({{1, map}});
		camera = before;
		require(movement.apply(terrainWorld, camera, mmodern::NavigationAction::MoveForward) ==
			mmodern::XeenMovementResult::Moved,
			"terreno transitavel consultou a superficie indevidamente");
	}
}

void testMissingNeighborAndInvariant() {
	auto world = worldWith({{1, freeMap()}});
	const mmodern::XeenMovement movement;
	const mmodern::XeenCamera boundaryCases[] = {
		{1,0,8,mmodern::XeenDirection::West}, {1,15,8,mmodern::XeenDirection::East},
		{1,8,0,mmodern::XeenDirection::South}, {1,8,15,mmodern::XeenDirection::North}
	};
	for (const auto &initial : boundaryCases) {
		auto camera = initial;
		require(movement.apply(world, camera, mmodern::NavigationAction::MoveForward) ==
			mmodern::XeenMovementResult::BlockedByMapBoundary, "lado sem vizinho nao bloqueado");
		require(sameCamera(camera, initial), "lado sem vizinho alterou a camera");
	}

	mmodern::XeenCamera camera{1, 0, 0, mmodern::XeenDirection::South};
	for (int i = 0; i < 64; ++i) {
		movement.apply(world, camera, mmodern::NavigationAction::MoveForward);
		movement.apply(world, camera, mmodern::NavigationAction::TurnLeft);
		require(camera.mapId == 1 && camera.x >= 0 && camera.x <= 15 &&
			camera.y >= 0 && camera.y <= 15, "invariante da camera violada");
	}
}

void testIndoorForwardBackwardAndRotations() {
	auto world = worldWith({{33, freeIndoorMap()}});
	const mmodern::XeenMovement movement;
	struct Expected { mmodern::XeenDirection direction; int dx; int dy; };
	const Expected expected[] = {
		{mmodern::XeenDirection::North, 0, 1},
		{mmodern::XeenDirection::East, 1, 0},
		{mmodern::XeenDirection::South, 0, -1},
		{mmodern::XeenDirection::West, -1, 0}
	};
	for (const auto &item : expected) {
		mmodern::XeenCamera forward{33, 8, 8, item.direction};
		require(movement.apply(world, forward, mmodern::NavigationAction::MoveForward) ==
			mmodern::XeenMovementResult::Moved && forward.mapId == 33 &&
			forward.x == 8 + item.dx && forward.y == 8 + item.dy &&
			forward.direction == item.direction, "movimento interior para frente incorreto");

		mmodern::XeenCamera backward{33, 8, 8, item.direction};
		require(movement.apply(world, backward, mmodern::NavigationAction::MoveBackward) ==
			mmodern::XeenMovementResult::Moved && backward.mapId == 33 &&
			backward.x == 8 - item.dx && backward.y == 8 - item.dy &&
			backward.direction == item.direction, "recuo interior incorreto");
	}

	mmodern::XeenCamera camera{33, 8, 8, mmodern::XeenDirection::North};
	require(movement.apply(world, camera, mmodern::NavigationAction::TurnLeft) ==
		mmodern::XeenMovementResult::Turned &&
		camera.direction == mmodern::XeenDirection::West && camera.x == 8 && camera.y == 8,
		"rotacao interior esquerda incorreta");
	require(movement.apply(world, camera, mmodern::NavigationAction::TurnRight) ==
		mmodern::XeenMovementResult::Turned &&
		camera.direction == mmodern::XeenDirection::North,
		"rotacao interior direita incorreta");
}

void testIndoorWallThresholdAndCurrentCell() {
	const mmodern::XeenMovement movement;
	for (const std::uint8_t wall : {std::uint8_t{1}, std::uint8_t{4}, std::uint8_t{6},
			std::uint8_t{7}, std::uint8_t{9}, std::uint8_t{13}}) {
		auto map = freeIndoorMap();
		setIndoorWall(map, 8, 8, mmodern::XeenDirection::North, wall);
		auto world = worldWith({{33, map}});
		mmodern::XeenCamera camera{33, 8, 8, mmodern::XeenDirection::North};
		const auto before = camera;
		const auto result = movement.apply(world, camera, mmodern::NavigationAction::MoveForward);
		if (wall < 7) {
			require(result == mmodern::XeenMovementResult::Moved && camera.y == 9,
				"parede interior abaixo do limite bloqueou");
		} else {
			require(result == mmodern::XeenMovementResult::BlockedByWall &&
				sameCamera(camera, before), "parede interior no/acima do limite nao bloqueou atomicamente");
		}
	}

	auto custom = freeIndoorMap(4);
	setIndoorWall(custom, 8, 8, mmodern::XeenDirection::North, 4);
	auto customWorld = worldWith({{33, custom}});
	mmodern::XeenCamera customCamera{33, 8, 8, mmodern::XeenDirection::North};
	require(movement.apply(customWorld, customCamera, mmodern::NavigationAction::MoveForward) ==
		mmodern::XeenMovementResult::BlockedByWall,
		"wallNoPass customizado nao foi respeitado");

	for (const auto direction : {mmodern::XeenDirection::North,
			mmodern::XeenDirection::East, mmodern::XeenDirection::South,
			mmodern::XeenDirection::West}) {
		auto map = freeIndoorMap();
		setIndoorWall(map, 8, 8, direction, 7);
		auto world = worldWith({{33, map}});
		mmodern::XeenCamera camera{33, 8, 8, direction};
		const auto before = camera;
		require(movement.apply(world, camera, mmodern::NavigationAction::MoveForward) ==
			mmodern::XeenMovementResult::BlockedByWall && sameCamera(camera, before),
			"parede frontal nao bloqueou uma das quatro direcoes");

		auto reverseMap = freeIndoorMap();
		const auto reverseDirection = static_cast<mmodern::XeenDirection>(
			static_cast<unsigned>(direction) ^ 2U);
		setIndoorWall(reverseMap, 8, 8, reverseDirection, 7);
		auto reverseWorld = worldWith({{33, reverseMap}});
		camera = {33, 8, 8, direction};
		const auto reverseBefore = camera;
		require(movement.apply(reverseWorld, camera, mmodern::NavigationAction::MoveBackward) ==
			mmodern::XeenMovementResult::BlockedByWall && sameCamera(camera, reverseBefore),
			"parede traseira nao bloqueou recuo em uma das quatro direcoes");
	}

	auto backwardMap = freeIndoorMap();
	setIndoorWall(backwardMap, 8, 8, mmodern::XeenDirection::South, 7);
	auto backwardWorld = worldWith({{33, backwardMap}});
	mmodern::XeenCamera backward{33, 8, 8, mmodern::XeenDirection::North};
	const auto backwardBefore = backward;
	require(movement.apply(backwardWorld, backward, mmodern::NavigationAction::MoveBackward) ==
		mmodern::XeenMovementResult::BlockedByWall && sameCamera(backward, backwardBefore),
		"recuo nao consultou a parede traseira da celula atual");

	auto targetWallMap = freeIndoorMap();
	setIndoorWall(targetWallMap, 8, 9, mmodern::XeenDirection::South, 13);
	auto targetWallWorld = worldWith({{33, targetWallMap}});
	mmodern::XeenCamera targetWallCamera{33, 8, 8, mmodern::XeenDirection::North};
	require(movement.apply(targetWallWorld, targetWallCamera,
		mmodern::NavigationAction::MoveForward) == mmodern::XeenMovementResult::Moved &&
		targetWallCamera.y == 9,
		"face oposta divergente da celula destino afetou o movimento");
}

void testIndoorSurfaceBoundariesAndAtomicity() {
	const mmodern::XeenMovement movement;
	auto surfaceMap = freeIndoorMap();
	indoorCellAt(surfaceMap, 8, 9).surfaceIndex = 4;
	auto surfaceWorld = worldWith({{33, surfaceMap}});
	mmodern::XeenCamera camera{33, 8, 8, mmodern::XeenDirection::North};
	const auto before = camera;
	require(movement.apply(surfaceWorld, camera, mmodern::NavigationAction::MoveForward) ==
		mmodern::XeenMovementResult::BlockedBySurface && sameCamera(camera, before),
		"superficie interior local 4 nao bloqueou atomicamente");

	auto passableMap = freeIndoorMap();
	indoorCellAt(passableMap, 8, 9).surfaceIndex = 3;
	auto passableWorld = worldWith({{33, passableMap}});
	camera = before;
	require(movement.apply(passableWorld, camera, mmodern::NavigationAction::MoveForward) ==
		mmodern::XeenMovementResult::Moved, "superficie interior diferente de 4 bloqueou");

	struct Boundary { mmodern::XeenCamera camera; mmodern::NavigationAction action; };
	const Boundary boundaries[] = {
		{{33,8,15,mmodern::XeenDirection::North}, mmodern::NavigationAction::MoveForward},
		{{33,15,8,mmodern::XeenDirection::East}, mmodern::NavigationAction::MoveForward},
		{{33,8,0,mmodern::XeenDirection::South}, mmodern::NavigationAction::MoveForward},
		{{33,0,8,mmodern::XeenDirection::West}, mmodern::NavigationAction::MoveForward},
		{{33,8,0,mmodern::XeenDirection::North}, mmodern::NavigationAction::MoveBackward},
		{{33,0,8,mmodern::XeenDirection::East}, mmodern::NavigationAction::MoveBackward},
		{{33,8,15,mmodern::XeenDirection::South}, mmodern::NavigationAction::MoveBackward},
		{{33,15,8,mmodern::XeenDirection::West}, mmodern::NavigationAction::MoveBackward}
	};
	for (const auto &boundary : boundaries) {
		int loads = 0;
		mmodern::XeenWorld world([&](mmodern::XeenMapIdentity id) {
			++loads;
			if (id != 33)
				throw std::runtime_error("movimento interior tentou carregar vizinho");
			return freeIndoorMap();
		});
		camera = boundary.camera;
		const auto initial = camera;
		require(movement.apply(world, camera, boundary.action) ==
			mmodern::XeenMovementResult::BlockedByMapBoundary && sameCamera(camera, initial),
			"borda interior nao bloqueou atomicamente");
		require(loads == 1 && world.cachedMapCount() == 1,
			"borda interior seguiu vizinho declarado");
	}

	// Boundary has priority over a blocking wall on the current cell.
	auto priorityMap = freeIndoorMap();
	setIndoorWall(priorityMap, 8, 15, mmodern::XeenDirection::North, 13);
	auto priorityWorld = worldWith({{33, priorityMap}});
	camera = {33, 8, 15, mmodern::XeenDirection::North};
	require(movement.apply(priorityWorld, camera, mmodern::NavigationAction::MoveForward) ==
		mmodern::XeenMovementResult::BlockedByMapBoundary,
		"prioridade borda antes de parede nao foi preservada");

	// Wall has priority over a blocking destination surface.
	auto wallPriorityMap = freeIndoorMap();
	setIndoorWall(wallPriorityMap, 8, 8, mmodern::XeenDirection::North, 7);
	indoorCellAt(wallPriorityMap, 8, 9).surfaceIndex = 4;
	auto wallPriorityWorld = worldWith({{33, wallPriorityMap}});
	camera = before;
	require(movement.apply(wallPriorityWorld, camera, mmodern::NavigationAction::MoveForward) ==
		mmodern::XeenMovementResult::BlockedByWall,
		"prioridade parede antes de superficie nao foi preservada");
}

} // namespace

int main() {
	try {
		testRotations();
		testForwardAndBackward();
		testNeighborTransitions();
		testBlockedDestinationIsAtomic();
		testTerrainAndSurfaceCollision();
		testMissingNeighborAndInvariant();
		testIndoorForwardBackwardAndRotations();
		testIndoorWallThresholdAndCurrentCell();
		testIndoorSurfaceBoundariesAndAtomicity();
		std::cout << "Xeen movement tests passed\n";
		return EXIT_SUCCESS;
	} catch (const std::exception &error) {
		std::cerr << "Xeen movement tests failed: " << error.what() << '\n';
		return EXIT_FAILURE;
	}
}
