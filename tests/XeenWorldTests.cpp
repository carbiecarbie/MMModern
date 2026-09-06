#include "games/xeen/XeenWorld.h"

#include <cstdlib>
#include <iostream>
#include <map>
#include <stdexcept>

namespace {

void require(bool condition, const char *message) {
	if (!condition)
		throw std::runtime_error(message);
}

mmodern::XeenMap mapFixture(std::uint16_t id) {
	mmodern::XeenMap map;
	map.geometry.id = id;
	map.geometry.flags2 = 0x8000;
	for (auto &cell : map.geometry.cells)
		cell.geometry = mmodern::XeenOutdoorLayers{};
	return map;
}

mmodern::XeenMap indoorMapFixture(std::uint16_t id) {
	mmodern::XeenMap map;
	map.geometry.id = id;
	map.geometry.flags2 = 0;
	map.geometry.neighbors = {115, 2, 3, 4};
	for (auto &cell : map.geometry.cells)
		cell.geometry = mmodern::XeenIndoorWalls{};
	map.geometry.cells[8 * 16 + 4].rawWord = 0x1234;
	map.geometry.cells[8 * 16 + 4].geometry =
		mmodern::XeenIndoorWalls{{1, 2, 3, 4}};
	return map;
}

void requireSample(const std::optional<mmodern::XeenCellSample> &sample,
		std::uint16_t mapId, int x, int y, const char *message) {
	require(sample && sample->mapId == mapId && sample->x == x && sample->y == y &&
		sample->geometry && sample->cell, message);
}

void testCardinalAndDiagonalResolution() {
	auto map1 = mapFixture(1);
	auto map2 = mapFixture(2);
	auto map5 = mapFixture(5);
	auto map6 = mapFixture(6);
	map1.geometry.neighbors = {0, 5, 2, 0};
	map5.geometry.neighbors = {0, 0, 6, 1};
	map2.geometry.neighbors = {1, 6, 0, 0};
	map6.geometry.neighbors = {5, 0, 0, 2};
	std::map<std::uint16_t, mmodern::XeenMap> maps = {
		{1, map1}, {2, map2}, {5, map5}, {6, map6}
	};
	std::map<std::uint16_t, int> loads;
	mmodern::XeenWorld world([&](std::uint16_t id) {
		++loads[id];
		return maps.at(id);
	});

	requireSample(world.sampleCell(1, 16, 6), 5, 0, 6, "001,(16,6) nao resolveu para 005,(0,6)");
	requireSample(world.sampleCell(5, -1, 6), 1, 15, 6, "005,(-1,6) nao resolveu para 001,(15,6)");
	requireSample(world.sampleCell(1, 9, -1), 2, 9, 15, "001,(9,-1) nao resolveu para 002,(9,15)");
	requireSample(world.sampleCell(2, 9, 16), 1, 9, 0, "002,(9,16) nao resolveu para 001,(9,0)");
	requireSample(world.sampleCell(1, 16, -1), 6, 0, 15, "diagonal sudeste nao resolveu por Y antes de X");
	require(!world.sampleCell(1, -1, 6), "vizinho oeste ausente deveria retornar vazio");
	require(!world.sampleCell(1, 6, 16), "vizinho norte ausente deveria retornar vazio");

	// Repeat every access: no loader may be called a second time for a cached map.
	world.sampleCell(1, 16, 6);
	world.sampleCell(1, 9, -1);
	world.sampleCell(1, 16, -1);
	require(world.cachedMapCount() == 4, "cache deveria conter quatro mapas");
	for (const auto &entry : loads)
		require(entry.second == 1, "um mapa foi carregado mais de uma vez");
}

void testInvalidDistanceAndIdentityValidation() {
	auto map1 = mapFixture(1);
	mmodern::XeenWorld world([&](std::uint16_t) { return map1; });
	require(!world.sampleCell(1, 32, 0), "consulta alem da grade 3x3 deveria falhar");
	require(!world.sampleCell(1, 0, -17), "consulta alem da grade 3x3 deveria falhar");

	mmodern::XeenWorld wrong([](std::uint16_t) { return mapFixture(9); });
	bool rejected = false;
	try {
		wrong.map(1);
	} catch (const std::runtime_error &) {
		rejected = true;
	}
	require(rejected, "cache aceitou mapa com ID interno incorreto");
}

void testIndoorCacheAndLocalSampling() {
	std::map<std::uint16_t, int> loads;
	mmodern::XeenWorld world([&](std::uint16_t id) {
		++loads[id];
		if (id == 33)
			return indoorMapFixture(id);
		throw std::runtime_error("vizinho interior nao deveria ser carregado");
	});

	const auto local = world.sampleCell(33, 4, 8);
	requireSample(local, 33, 4, 8, "celula interior local nao foi retornada");
	require(mmodern::wallAt(*local->cell, mmodern::XeenDirection::North) == 1 &&
		mmodern::wallAt(*local->cell, mmodern::XeenDirection::East) == 2 &&
		mmodern::wallAt(*local->cell, mmodern::XeenDirection::South) == 3 &&
		mmodern::wallAt(*local->cell, mmodern::XeenDirection::West) == 4,
		"wallAt nao selecionou as quatro direcoes");
	require(!world.sampleCell(33, -1, 8) && !world.sampleCell(33, 16, 8) &&
		!world.sampleCell(33, 4, -1) && !world.sampleCell(33, 4, 16),
		"consulta interior fora de 0..15 deveria retornar vazio");
	require(loads[33] == 1 && loads.find(115) == loads.end() &&
		world.cachedMapCount() == 1,
		"cache interior recarregou o mapa ou seguiu vizinho declarado");
}

} // namespace

int main() {
	try {
		testCardinalAndDiagonalResolution();
		testInvalidDistanceAndIdentityValidation();
		testIndoorCacheAndLocalSampling();
		std::cout << "Xeen world tests passed\n";
		return EXIT_SUCCESS;
	} catch (const std::exception &error) {
		std::cerr << "Xeen world tests failed: " << error.what() << '\n';
		return EXIT_FAILURE;
	}
}
