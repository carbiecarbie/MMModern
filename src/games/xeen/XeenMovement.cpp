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

	const auto *outdoor = std::get_if<XeenOutdoorLayers>(&target->cell->geometry);
	if (!outdoor)
		throw std::runtime_error("celula interior encontrada em mapa exterior");

	if (blocksWithoutMountaineer(outdoor->middle))
		return XeenMovementResult::BlockedByTerrain;
	if (checksSurface(outdoor->middle)) {
		const std::uint8_t surface = target->geometry->surfaceTypes[outdoor->surface];
		if (blockedSurface(surface))
			return XeenMovementResult::BlockedBySurface;
	}

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
	if (!std::holds_alternative<XeenIndoorWalls>(target->cell->geometry))
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

XeenMovementResult XeenMovement::apply(XeenWorld &world, XeenCamera &camera,
		NavigationAction action) const {
	const XeenMap &currentMap = world.map(camera.mapId);
	if (camera.mapId != currentMap.geometry.id)
		throw std::runtime_error("camera e mapa possuem IDs diferentes");
	if (camera.x < 0 || camera.x >= 16 || camera.y < 0 || camera.y >= 16)
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
		action == NavigationAction::MoveBackward ? opposite(camera.direction) : camera.direction;
	if (currentMap.geometry.isOutdoors())
		return applyOutdoor(world, camera, effectiveDirection);
	return applyIndoor(world, camera, currentMap.geometry, effectiveDirection);
}

} // namespace mmodern
