#include "games/xeen/XeenOutdoorScene.h"

#include "games/xeen/XeenOutdoorSceneTables.h"
#include "games/xeen/XeenWorld.h"

#include <algorithm>
#include <stdexcept>
#include <utility>

namespace mmodern {
namespace {

using namespace xeen_scene_tables;

std::pair<int, int> sampleOffset(XeenDirection direction, int sampleIndex) {
	const int southX = kSouthX[static_cast<std::size_t>(sampleIndex)];
	const int southY = kSouthY[static_cast<std::size_t>(sampleIndex)];
	switch (direction) {
	case XeenDirection::North: return {-southX, -southY};
	case XeenDirection::East:  return {-southY, southX};
	case XeenDirection::South: return {southX, southY};
	case XeenDirection::West:  return {southY, -southX};
	}
	throw std::runtime_error("direcao de camera invalida");
}

std::optional<XeenCellSample> sample(XeenWorld &world,
		const XeenCamera &camera, int sampleIndex) {
	if (sampleIndex < 0 || sampleIndex >= static_cast<int>(kSouthX.size()))
		throw std::runtime_error("indice de consulta exterior invalido");
	const auto offset = sampleOffset(camera.direction, sampleIndex);
	const int x = camera.x + offset.first;
	const int y = camera.y + offset.second;
	return world.sampleCell(camera.mapId, x, y);
}

const XeenOutdoorLayers &layers(const XeenMapCell &cell) {
	const auto *result = std::get_if<XeenOutdoorLayers>(&cell.geometry);
	if (!result)
		throw std::runtime_error("celula interior encontrada em mapa exterior");
	return *result;
}

XeenSpriteDrawOptions optionsFor(const Placement &placement) {
	XeenSpriteDrawOptions options;
	options.scaleIndex = placement.scale;
	options.horizontalFlip = placement.flipped;
	options.sceneClipped = true;
	options.bottomClipped = placement.bottomClipped;
	options.enlarge = placement.enlarge;
	return options;
}

} // namespace

std::vector<XeenOutdoorDrawCommand> XeenOutdoorScene::build(
		XeenWorld &world, const XeenCamera &camera) const {
	const XeenMap &map = world.map(camera.mapId);
	if (!map.geometry.isOutdoors())
		throw std::runtime_error("XeenOutdoorScene requer um mapa exterior");
	if (camera.mapId != map.identity())
		throw std::runtime_error("camera e mapa possuem IDs diferentes");
	if (camera.x < 0 || camera.y < 0 || camera.x >= 16 || camera.y >= 16)
		throw std::runtime_error("camera fora dos limites do mapa");

	std::vector<XeenOutdoorDrawCommand> commands;
	commands.reserve(32);
	auto addFixed = [&](int order, const char *resource, std::size_t frame, int x, int y) {
		XeenOutdoorDrawCommand command;
		command.originalOrder = order;
		command.resourceName = resource;
		command.frame = frame;
		command.x = x;
		command.y = y;
		command.options.sceneClipped = true;
		commands.push_back(std::move(command));
	};
	addFixed(0, "sky.sky", 0, 8, 8);
	addFixed(1, "sky.sky", 1, 8, 25);
	addFixed(2, "water.out", 0, 8, 67);

	for (std::size_t i = 0; i < kGroundPlacements.size(); ++i) {
		const int sampleIndex = i == 24 ? 2 : kDrawNumbers[i];
		const auto sampled = sample(world, camera, sampleIndex);
		// Map::getCell represents the exterior beyond a missing neighbor as SPACE.
		const std::uint8_t surfaceType = sampled ?
			sampled->geometry->surfaceTypes[layers(*sampled->cell).surface] : 15;
		const char *resource = kSurfaceNames[surfaceType];
		if (!resource || !*resource)
			continue;
		const Placement &placement = kGroundPlacements[i];
		XeenOutdoorDrawCommand command;
		command.originalOrder = placement.order;
		command.resourceName = resource;
		command.frame = static_cast<std::size_t>(kGroundFrames[i]);
		command.x = placement.x;
		command.y = placement.y;
		if (sampled) {
			command.sourceMapId = sampled->mapId;
			command.sourceX = sampled->x;
			command.sourceY = sampled->y;
		}
		command.options = optionsFor(placement);
		commands.push_back(std::move(command));
	}

	for (const TerrainPlacement &terrain : kTerrainPlacements) {
		const auto sampled = sample(world, camera, terrain.sample);
		if (!sampled)
			continue;
		const auto &outdoor = layers(*sampled->cell);
		const std::uint8_t terrainType = sampled->geometry->wallTypes[outdoor.middle];
		const char *baseName = kOutdoorWallNames[terrainType];
		if (!baseName || !*baseName)
			continue;
		XeenOutdoorDrawCommand command;
		command.originalOrder = terrain.placement.order;
		command.resourceName = std::string(baseName) + ".wal";
		command.frame = static_cast<std::size_t>(terrain.placement.frame);
		command.x = terrain.placement.x;
		command.y = terrain.placement.y;
		command.sourceMapId = sampled->mapId;
		command.sourceX = sampled->x;
		command.sourceY = sampled->y;
		command.options = optionsFor(terrain.placement);
		commands.push_back(std::move(command));
	}

	std::stable_sort(commands.begin(), commands.end(),
		[](const auto &left, const auto &right) { return left.originalOrder < right.originalOrder; });
	return commands;
}

} // namespace mmodern
