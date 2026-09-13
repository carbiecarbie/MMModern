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
		const XeenObjectVisualResolver *resolver, std::vector<XeenObjectVisual> *diagnostics,
		std::optional<std::uint64_t> ordinaryPhase, std::optional<XeenMonsterAppearance> actorFrame) const {
	// Retain values before terrain/resource providers can publish or discard caches.
	const auto actors = actorFrame ? actorCommands(world.sessionState().actors(), camera, *actorFrame) :
		std::vector<XeenOutdoorDrawCommand>{};
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
				auto visual = resolver->resolve(file, i, camera.direction, ordinaryPhase);
				if (visual.status == XeenObjectVisualStatus::SupportedStatic ||
						visual.status == XeenObjectVisualStatus::SupportedAnimated) {
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

	commands.insert(commands.end(), actors.begin(), actors.end());
	std::stable_sort(commands.begin(), commands.end(),
		[](const auto &left, const auto &right) { return left.originalOrder < right.originalOrder; });
	return commands;
}

std::vector<XeenOutdoorDrawCommand> XeenOutdoorScene::actorCommands(
		const std::vector<XeenActor> &actors, const XeenCamera &camera, XeenMonsterAppearance appearance) {
	if (!appearance.valid()) throw std::invalid_argument("Unsupported actor appearance");
	const auto view = XeenActorApproach::classify(actors, camera);
	std::vector<XeenOutdoorDrawCommand> commands;
	// Selected-slot projection from pinned ScummVM; M30 MON/ATT table.
	struct Group { int query, count; int slots[3], orders[3], xs[3]; int y, scale; int pair[2]; };
	static constexpr Group groups[]{
		{2,3,{0,1,2},{118,112,115},{-5,-67,58},2,0,{31,-36}},
		{7,3,{3,4,5},{94,92,93},{-7,-38,25},34,8,{8,-23}},
		{5,1,{12},{90},{-112},34,8,{-112}}, {9,1,{13},{91},{98},34,8,{98}},
		{14,3,{6,7,8},{75,73,74},{-8,-24,9},53,12,{0,-16}},
		{12,2,{14,20},{69,70},{-65,-85},53,12,{-65,-85}},
		{16,2,{15,21},{71,72},{49,65},53,12,{49,65}},
		{27,3,{9,10,11},{52,50,51},{-9,-17,-1},59,14,{-5,-13}},
		{25,3,{16,22,24},{44,42,43},{-34,-41,-26},59,14,{-27,-37}},
		{23,1,{18},{48},{-58},59,14,{-58}},
		{29,3,{17,23,25},{47,45,46},{16,-16,23},59,14,{20,-12}},
		{31,1,{19},{49},{40},59,14,{40}}
	};
	for (const auto &g : groups) {
		int count=0; for (int i=0;i<g.count;++i) if (view.slots[g.slots[i]]) ++count;
		for (int i=0;i<g.count;++i) {
			const auto id=view.slots[g.slots[i]]; if (!id) continue;
			const auto found=std::find_if(actors.begin(),actors.end(),[&](const auto &actor){return actor.id==*id;});
			if (found==actors.end() || !found->statistics || !found->statistics->supportsRendering() ||
				found->lifecycle!=XeenActorLifecycle::Present || found->status!=XeenActorStatus::Physical)
				throw std::runtime_error("Unsupported selected encounter actor");
			const auto &actor=*found;
			const bool special=appearance.kind==XeenMonsterSpriteKind::Attack &&
				(appearance.identity ? *appearance.identity==actor.id : actor.id==XeenMonsterIdentity{20,5});
			if (special && g.query!=2) throw std::invalid_argument("Attack appearance requires same-cell placement");
			XeenOutdoorDrawCommand c;
			c.sampleIndex=g.query; c.originalOrder=special ? 121 : g.orders[i];
			c.x=count==2 && i<2 ? g.pair[i] : g.xs[i]; c.y=g.y;
			c.sourceMapId=actor.id.mapId; c.sourceX=actor.x; c.sourceY=actor.y;
			XeenOutdoorActorDraw draw{actor.id,actor.statistics->image(),
				static_cast<std::uint8_t>(special || appearance.kind==XeenMonsterSpriteKind::Normal ? appearance.frame : 0),
				special ? XeenMonsterSpriteKind::Attack : XeenMonsterSpriteKind::Normal};
			draw.selectedSlot=g.slots[i]; draw.scaleIndex=g.scale; draw.bottomClipped=g.query==2;
			c.content=draw; commands.push_back(std::move(c));
		}
	}
	return commands;
}

} // namespace mmodern
