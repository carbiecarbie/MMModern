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
		XeenWorld &world, const XeenCamera &camera,
		const XeenObjectVisualResolver *resolver, std::vector<XeenObjectVisual> *diagnostics) const {
	if (diagnostics) diagnostics->clear();
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
		command.terrain().resourceName = resource;
		command.terrain().frame = frame;
		command.x = x;
		command.y = y;
		command.terrain().options.sceneClipped = true;
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
		command.terrain().resourceName = resource;
		command.terrain().frame = static_cast<std::size_t>(kGroundFrames[i]);
		command.x = placement.x;
		command.y = placement.y;
		if (sampled) {
			command.sourceMapId = sampled->mapId;
			command.sourceX = sampled->x;
			command.sourceY = sampled->y;
		}
		command.terrain().options = optionsFor(placement);
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
		command.terrain().resourceName = std::string(baseName) + ".wal";
		command.terrain().frame = static_cast<std::size_t>(terrain.placement.frame);
		command.x = terrain.placement.x;
		command.y = terrain.placement.y;
		command.sourceMapId = sampled->mapId;
		command.sourceX = sampled->x;
		command.sourceY = sampled->y;
		command.terrain().options = optionsFor(terrain.placement);
		commands.push_back(std::move(command));
	}

	if (resolver) {
		// Terrain may cross map edges. Objects match raw coordinates in this MOB
		// only; never feed object coordinates through world.sampleCell().
		const auto &file = world.objectFile(camera.mapId);
		if (file.resourcePresent) for (const auto &placement : kObjectPlacements) {
			const auto offset = sampleOffset(camera.direction, placement.sample);
			const int rawX = camera.x + offset.first, rawY = camera.y + offset.second;
			for (std::size_t i = 0; i < file.entities.objects.size(); ++i) {
				const auto &object = file.entities.objects[i];
				if (object.x != rawX || object.y != rawY || !object.isActive() ||
						object.resourceId >= 255 || world.isObjectDisabled({camera.mapId, i})) continue;
				auto visual = resolver->resolve(file, i, camera.direction);
				if (visual.status == XeenObjectVisualStatus::SupportedStatic) {
					const unsigned row = object.resourceId == 113 ? 1 : 0;
					XeenOutdoorDrawCommand command;
					command.originalOrder = placement.order;
					command.x = placement.x[row]; command.y = placement.y[row];
					command.sourceMapId = camera.mapId;
					command.sourceX = rawX; command.sourceY = rawY;
					command.sampleIndex = placement.sample;
					command.content = XeenOutdoorObjectDraw{std::move(visual), placement.scale, placement.sample == 2};
					commands.push_back(std::move(command));
				} else if (diagnostics) diagnostics->push_back(std::move(visual));
				// The first applicable record owns this position even if unsupported.
				break;
			}
		}
	}

	std::stable_sort(commands.begin(), commands.end(),
		[](const auto &left, const auto &right) { return left.originalOrder < right.originalOrder; });
	return commands;
}

} // namespace mmodern
