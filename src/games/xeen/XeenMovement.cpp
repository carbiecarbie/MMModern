#include "games/xeen/XeenMovement.h"

#include "games/xeen/XeenWorld.h"

#include <stdexcept>
#include <utility>

namespace mmodern {
namespace {

XeenDirection turnLeft(XeenDirection direction) {
	return static_cast<XeenDirection>((static_cast<unsigned>(direction) + 3) & 3);
}

XeenDirection turnRight(XeenDirection direction) {
	return static_cast<XeenDirection>((static_cast<unsigned>(direction) + 1) & 3);
}

bool blocksWithoutMountaineer(std::uint8_t middle) {
	switch (middle) {
	case 1:
	case 7:
	case 9:
	case 10:
	case 12:
		return true;
	default:
		return false;
	}
}

bool checksSurface(std::uint8_t middle) {
	// Clouds semantics from Interface::checkMoveDirection(). Middle value 5 is
	// in this group on the Clouds side; Darkside's exception is out of scope.
	switch (middle) {
	case 0:
	case 2:
	case 4:
	case 5:
	case 8:
	case 11:
	case 13:
	case 14:
		return true;
	default:
		return false;
	}
}

bool blockedSurface(std::uint8_t surface) {
	// No Swimming, Walk on Water, or Party state exists in this milestone.
	return surface == 0 || surface == 8 || surface == 15;
}

std::pair<int, int> directionDelta(XeenDirection direction) {
	switch (direction) {
	case XeenDirection::North: return {0, 1};
	case XeenDirection::East:  return {1, 0};
	case XeenDirection::South: return {0, -1};
	case XeenDirection::West:  return {-1, 0};
	}
	throw std::runtime_error("direcao de camera invalida");
}

XeenDirection opposite(XeenDirection direction) {
	return static_cast<XeenDirection>(static_cast<unsigned>(direction) ^ 2U);
}

XeenMovementResult applyOutdoor(XeenWorld &world, XeenCamera &camera,
		XeenDirection effectiveDirection) {
	const auto delta = directionDelta(effectiveDirection);
	const auto target = world.sampleCell(camera.mapId,
		camera.x + delta.first, camera.y + delta.second);
	if (!target)
		return XeenMovementResult::BlockedByMapBoundary;

	const auto admission = XeenMovement::outdoorDestination(*target->geometry, *target->cell, {});
	if (admission != XeenMovementResult::Moved) return admission;

	XeenCamera destination = camera;
	destination.mapId = target->mapId;
	destination.x = target->x;
	destination.y = target->y;
	camera = destination;
	return XeenMovementResult::Moved;
}

XeenMovementResult applyIndoor(XeenWorld &world, XeenCamera &camera,
		const XeenMapGeometry &geometry, XeenDirection effectiveDirection) {
	const auto delta = directionDelta(effectiveDirection);
	const int targetX = camera.x + delta.first;
	const int targetY = camera.y + delta.second;
	const bool vertigo = camera.mapId == XeenMapIdentity(28) && world.regionalContract8();
	if (vertigo) {
		if (!xeenJourneyContent(world.sessionState().journeyContract()).vertigoCell(targetX,targetY))
			return XeenMovementResult::BlockedByMapBoundary;
		const auto source = world.sampleCell(camera.mapId,camera.x,camera.y);
		const auto target = world.sampleCell(camera.mapId,targetX,targetY);
		if (!source || !target) return XeenMovementResult::BlockedByMapBoundary;
		if (wallAt(*source->cell,effectiveDirection) >= source->geometry->difficulties[0])
			return XeenMovementResult::BlockedByWall;
		if (target->cell->surfaceIndex == 4) return XeenMovementResult::BlockedBySurface;
		camera.x=targetX;camera.y=targetY;
		return XeenMovementResult::Moved;
	}
	// Interior exits are event-driven. They do not use the neighbor plane here.
	if (targetX < 0 || targetX >= 16 || targetY < 0 || targetY >= 16)
		return XeenMovementResult::BlockedByMapBoundary;

	const std::size_t currentIndex = static_cast<std::size_t>(camera.y) *
		XeenMapGeometry::kWidth + static_cast<std::size_t>(camera.x);
	const std::uint8_t wall = wallAt(geometry.cells[currentIndex], effectiveDirection);
	if (wall >= geometry.difficulties[0])
		return XeenMovementResult::BlockedByWall;

	const auto target = world.sampleCell(camera.mapId, targetX, targetY);
	if (!target)
		return XeenMovementResult::BlockedByMapBoundary;
	if (!xeenHolds<XeenIndoorWalls>(target->cell->geometry))
		throw std::runtime_error("celula exterior encontrada em mapa interior");
	if (target->cell->surfaceIndex == 4)
		return XeenMovementResult::BlockedBySurface;

	XeenCamera destination = camera;
	destination.x = targetX;
	destination.y = targetY;
	camera = destination;
	return XeenMovementResult::Moved;
}

} // namespace

XeenMovementResult XeenMovement::outdoorDestination(const XeenMapGeometry &geometry,
		const XeenMapCell &cell, Capabilities capabilities) {
	if (capabilities.swimming || capabilities.walkOnWater || capabilities.mountaineer)
		throw std::invalid_argument("Unsupported outdoor traversal capability");
	const auto *layers = xeenGetIf<XeenOutdoorLayers>(&cell.geometry);
	if (!geometry.isOutdoors() || !layers || layers->surface >= 16 || layers->middle >= 16)
		throw std::invalid_argument("Invalid outdoor destination geometry");
	if (blocksWithoutMountaineer(layers->middle)) return XeenMovementResult::BlockedByTerrain;
	if (checksSurface(layers->middle) && blockedSurface(geometry.surfaceTypes[layers->surface]))
		return XeenMovementResult::BlockedBySurface;
	return XeenMovementResult::Moved;
}

XeenMovementResult XeenMovement::localOutdoor(const XeenMap &map, int fromX, int fromY,
		int toX, int toY, Capabilities capabilities) {
	if (map.side != XeenSide::Clouds || !map.geometry.isOutdoors())
		throw std::invalid_argument("Local outdoor traversal requires Clouds geometry");
	if (fromX < 0 || fromX >= 16 || fromY < 0 || fromY >= 16)
		throw std::invalid_argument("Invalid local traversal source");
	if (toX < 0 || toX >= 16 || toY < 0 || toY >= 16) return XeenMovementResult::BlockedByMapBoundary;
	if (std::abs(toX-fromX) + std::abs(toY-fromY) != 1)
		throw std::invalid_argument("Local traversal requires adjacent cells");
	return outdoorDestination(map.geometry, map.geometry.cells[toY*16+toX], capabilities);
}

std::bitset<256> XeenMovement::component(const XeenMap &map, int anchorX, int anchorY,
		Capabilities capabilities) {
	if (map.side != XeenSide::Clouds || anchorX < 0 || anchorX >= 16 || anchorY < 0 || anchorY >= 16 ||
		outdoorDestination(map.geometry, map.geometry.cells[anchorY*16+anchorX], capabilities) != XeenMovementResult::Moved)
		throw std::invalid_argument("Invalid or impassable outdoor component anchor");
	std::bitset<256> cells;
	std::array<int,256> queue{};
	unsigned begin = 0, end = 0;
	queue[end++] = anchorY*16+anchorX; cells.set(queue[0]);
	while (begin != end) {
		const auto index = queue[begin++];
		const int x = index%16, y = index/16;
		for (const auto d : {std::pair<int,int>{1,0}, {-1,0}, {0,1}, {0,-1}}) {
			const int nx=x+d.first, ny=y+d.second;
			if (localOutdoor(map,x,y,nx,ny,capabilities) != XeenMovementResult::Moved) continue;
			if (!cells[ny*16+nx]) { cells.set(ny*16+nx); queue[end++]=ny*16+nx; }
		}
	}
	return cells;
}

XeenMovementResult XeenMovement::apply(XeenWorld &world, XeenCamera &camera,
		NavigationAction action) const {
	const XeenMap &currentMap = world.map(camera.mapId);
	if (camera.mapId != currentMap.identity())
		throw std::runtime_error("camera e mapa possuem IDs diferentes");
	const bool vertigo = camera.mapId == XeenMapIdentity(28) && world.regionalContract8();
	if (camera.x < 0 || camera.x >= (vertigo ? 32 : 16) || camera.y < 0 || camera.y >= (vertigo ? 32 : 16))
		throw std::runtime_error("camera invalida antes do movimento");

	if (action == NavigationAction::TurnLeft) {
		camera.direction = turnLeft(camera.direction);
		return XeenMovementResult::Turned;
	}
	if (action == NavigationAction::TurnRight) {
		camera.direction = turnRight(camera.direction);
		return XeenMovementResult::Turned;
	}

	const XeenDirection effectiveDirection =
		action == NavigationAction::MoveBackward ? opposite(camera.direction) : XeenDirection(camera.direction);
	if (currentMap.geometry.isOutdoors())
		return applyOutdoor(world, camera, effectiveDirection);
	return applyIndoor(world, camera, currentMap.geometry, effectiveDirection);
}

} // namespace mmodern
