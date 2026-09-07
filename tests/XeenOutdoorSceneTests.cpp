#include "games/xeen/XeenOutdoorScene.h"
#include "games/xeen/XeenWorld.h"

#include <algorithm>
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

const mmodern::XeenOutdoorDrawCommand &byOrder(
		const std::vector<mmodern::XeenOutdoorDrawCommand> &commands, int order) {
	const auto found = std::find_if(commands.begin(), commands.end(),
		[order](const auto &command) { return command.originalOrder == order; });
	if (found == commands.end())
		throw std::runtime_error("comando esperado ausente");
	return *found;
}

mmodern::XeenMap makeAreaA1ViewFixture() {
	mmodern::XeenMap map;
	map.geometry.id = 1;
	map.geometry.flags2 = 0x8000;
	map.geometry.surfaceTypes[6] = 6; // Local 6 -> global desert surface.
	map.geometry.wallTypes[1] = 1;    // Local 1 -> global mountain terrain.
	for (auto &cell : map.geometry.cells) {
		mmodern::XeenOutdoorLayers layers;
		layers.surface = 6;
		cell.geometry = layers;
	}
	for (const auto &point : {std::pair<int, int>{8, 3}, {8, 4}, {8, 5}, {9, 5}}) {
		auto &cell = map.geometry.cells[static_cast<std::size_t>(point.second) * 16 + point.first];
		auto layers = std::get<mmodern::XeenOutdoorLayers>(cell.geometry);
		layers.middle = 1;
		cell.geometry = layers;
	}
	return map;
}

mmodern::XeenMap makeFlatFixture(mmodern::XeenMapIdentity id = 1) {
	auto map = makeAreaA1ViewFixture();
	map.geometry.id = id.number;
	map.side = id.side;
	for (auto &cell : map.geometry.cells) {
		auto outdoor = std::get<mmodern::XeenOutdoorLayers>(cell.geometry);
		outdoor.middle = 0;
		cell.geometry = outdoor;
	}
	return map;
}

mmodern::XeenWorld worldWith(std::map<mmodern::XeenMapIdentity, mmodern::XeenMap> maps) {
	return mmodern::XeenWorld([maps = std::move(maps)](mmodern::XeenMapIdentity id) {
		return maps.at(id);
	});
}

void testAreaA1CameraCommands() {
	auto world = worldWith({{1, makeAreaA1ViewFixture()}});
	const auto commands = mmodern::XeenOutdoorScene().build(world);
	require(commands.size() == 32, "a cena deve produzir 32 comandos");
	require(std::is_sorted(commands.begin(), commands.end(), [](const auto &a, const auto &b) {
		return a.originalOrder < b.originalOrder;
	}), "ordem original de desenho nao preservada");
	require(std::count_if(commands.begin(), commands.end(), [](const auto &command) {
		return command.resourceName == "desert.srf";
	}) == 25, "a camera deve produzir 25 partes de deserto");
	require(std::count_if(commands.begin(), commands.end(), [](const auto &command) {
		return command.resourceName == "mount.wal";
	}) == 4, "a camera deve produzir quatro montanhas");
	require(std::all_of(commands.begin(), commands.end(), [](const auto &command) {
		return command.options.sceneClipped;
	}), "todo comando da cena deve usar o recorte original");

	const auto &farMountain = byOrder(commands, 64);
	require(farMountain.resourceName == "mount.wal" && farMountain.frame == 1 &&
		farMountain.x == 86 && farMountain.y == 54 && farMountain.sourceX == 8 &&
		farMountain.sourceY == 3 && farMountain.options.scaleIndex == 11 &&
		farMountain.options.horizontalFlip, "montanha distante divergente");
	const auto &middleMountain = byOrder(commands, 85);
	require(middleMountain.frame == 2 && middleMountain.x == 146 &&
		middleMountain.y == 40 && middleMountain.sourceX == 8 &&
		middleMountain.sourceY == 4 && middleMountain.options.horizontalFlip,
		"montanha intermediaria divergente");
	const auto &nearRight = byOrder(commands, 104);
	require(nearRight.frame == 0 && nearRight.x == 169 && nearRight.y == 24 &&
		nearRight.sourceX == 8 && nearRight.sourceY == 5 &&
		nearRight.options.horizontalFlip, "montanha proxima direita divergente");
	const auto &nearCenter = byOrder(commands, 105);
	require(nearCenter.frame == 1 && nearCenter.x == 32 && nearCenter.y == 24 &&
		nearCenter.sourceX == 9 && nearCenter.sourceY == 5 &&
		!nearCenter.options.horizontalFlip, "montanha proxima central divergente");
}

void testFourDirections() {
	struct Expected {
		mmodern::XeenDirection direction;
		int x;
		int y;
	};
	const Expected expected[] = {
		{mmodern::XeenDirection::North, 9, 7},
		{mmodern::XeenDirection::East, 10, 6},
		{mmodern::XeenDirection::South, 9, 5},
		{mmodern::XeenDirection::West, 8, 6}
	};
	for (const auto &item : expected) {
		auto map = makeFlatFixture();
		auto &cell = map.geometry.cells[static_cast<std::size_t>(item.y) * 16 + item.x];
		auto outdoor = std::get<mmodern::XeenOutdoorLayers>(cell.geometry);
		outdoor.middle = 1;
		cell.geometry = outdoor;
		const mmodern::XeenCamera camera{1, 9, 6, item.direction};
		auto world = worldWith({{1, map}});
		const auto commands = mmodern::XeenOutdoorScene().build(world, camera);
		const auto &front = byOrder(commands, 105); // Terrain sample index 7.
		require(front.sourceX == item.x && front.sourceY == item.y,
			"consulta frontal incorreta apos rotacao");
	}
}

void testVisibleEntitiesAreIgnored() {
	auto map = makeFlatFixture();
	map.entities.objects.push_back({9, 5, 0, 0, 1});
	map.entities.monsters.push_back({9, 5, 0, 0, 1});
	auto world = worldWith({{1, map}});
	const auto commands = mmodern::XeenOutdoorScene().build(world);
	require(commands.size() == 28, "entidades nao devem alterar a lista de terreno");
}

void testViewAtMapEdge() {
	auto world = worldWith({{1, makeFlatFixture()}});
	const mmodern::XeenCamera camera{1, 0, 0, mmodern::XeenDirection::South};
	const auto commands = mmodern::XeenOutdoorScene().build(world, camera);
	require(!commands.empty(), "fallback da borda perdeu ceu e fundo");
	require(std::all_of(commands.begin(), commands.end(), [](const auto &command) {
		return (command.sourceMapId == 0 && command.sourceX == -1 && command.sourceY == -1) ||
			(command.sourceX >= 0 && command.sourceX < 16 &&
			 command.sourceY >= 0 && command.sourceY < 16);
	}), "cena da borda gerou coordenada de origem invalida");
	require(std::any_of(commands.begin(), commands.end(), [](const auto &command) {
		return command.resourceName == "space.srf" && command.sourceMapId == 0;
	}), "lado sem vizinho nao foi representado como SPACE");
}

void testNeighborAndDiagonalSources() {
	auto map1 = makeFlatFixture(1);
	auto map2 = makeFlatFixture(2);
	auto map5 = makeFlatFixture(5);
	auto map6 = makeFlatFixture(6);
	map1.geometry.neighbors = {0, 5, 2, 0};
	map5.geometry.neighbors = {0, 0, 6, 1};
	map2.geometry.neighbors = {1, 6, 0, 0};
	map6.geometry.neighbors = {5, 0, 0, 2};
	auto world = worldWith({{1, map1}, {2, map2}, {5, map5}, {6, map6}});

	const auto east = mmodern::XeenOutdoorScene().build(world,
		{1, 15, 6, mmodern::XeenDirection::East});
	require(std::any_of(east.begin(), east.end(), [](const auto &command) {
		return command.sourceMapId == 5;
	}), "cena da borda leste nao consultou o mapa 005");

	const auto south = mmodern::XeenOutdoorScene().build(world,
		{1, 9, 0, mmodern::XeenDirection::South});
	require(std::any_of(south.begin(), south.end(), [](const auto &command) {
		return command.sourceMapId == 2;
	}), "cena da borda sul nao consultou o mapa 002");

	const auto corner = mmodern::XeenOutdoorScene().build(world,
		{1, 15, 0, mmodern::XeenDirection::South});
	require(std::any_of(corner.begin(), corner.end(), [](const auto &command) {
		return command.sourceMapId == 6;
	}), "cena do canto sudeste nao consultou o mapa diagonal 006");
}

} // namespace

int main() {
	try {
		testAreaA1CameraCommands();
		testFourDirections();
		testVisibleEntitiesAreIgnored();
		testViewAtMapEdge();
		testNeighborAndDiagonalSources();
		std::cout << "Xeen outdoor scene tests passed\n";
		return EXIT_SUCCESS;
	} catch (const std::exception &error) {
		std::cerr << "Xeen outdoor scene tests failed: " << error.what() << '\n';
		return EXIT_FAILURE;
	}
}
