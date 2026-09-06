#include "games/xeen/XeenEventSystem.h"

#include "games/xeen/XeenEventTrigger.h"
#include "games/xeen/XeenWorld.h"

#include <stdexcept>
#include <string>
#include <type_traits>
#include <utility>

namespace mmodern {
namespace {

bool validDirection(XeenDirection direction) {
	switch (direction) {
	case XeenDirection::North:
	case XeenDirection::East:
	case XeenDirection::South:
	case XeenDirection::West:
		return true;
	}
	return false;
}

bool sameCamera(const XeenCamera &left, const XeenCamera &right) {
	return left.mapId == right.mapId && left.x == right.x && left.y == right.y &&
		left.direction == right.direction;
}

XeenEventExecutionError systemError(XeenEventExecutionErrorKind kind,
		std::string message, const XeenCamera &camera) {
	return {kind, std::move(message), 0,
		{camera.mapId, camera.x, camera.y, 0}, std::nullopt, std::nullopt};
}

static_assert(std::is_nothrow_copy_assignable<XeenCamera>::value,
	"XeenEventSystem requires a non-throwing camera commit");
static_assert(std::is_nothrow_copy_assignable<XeenGameFlags>::value,
	"XeenEventSystem requires a non-throwing game-flags commit");

} // namespace

XeenEventSystem::XeenEventSystem(ScriptProvider scriptProvider) :
	_scriptProvider(std::move(scriptProvider)) {
	if (!_scriptProvider)
		throw std::invalid_argument("XeenEventSystem requires a script provider");
}

XeenEventScript XeenEventSystem::scriptForMap(std::uint16_t mapId) {
	const auto cached = _scripts.find(mapId);
	if (cached != _scripts.end())
		return cached->second;

	XeenEventScript loaded = _scriptProvider(mapId);
	return _scripts.emplace(mapId, std::move(loaded)).first->second;
}

XeenAutomaticEventResult XeenEventSystem::runAutomaticEvent(
		XeenWorld &world, const XeenPartyState &partyState, XeenCamera &camera,
		XeenGameFlags &gameFlags) {
	if (!camera.mapId || camera.x < 0 || camera.x > 15 || camera.y < 0 ||
			camera.y > 15 || !validDirection(camera.direction)) {
		return systemError(XeenEventExecutionErrorKind::InvalidInitialCamera,
			"automatic event camera is outside the supported Xeen map domain",
			camera);
	}

	const XeenMap *map = nullptr;
	try {
		map = &world.map(camera.mapId);
	} catch (const std::exception &exception) {
		return systemError(XeenEventExecutionErrorKind::MapLoadFailed,
			std::string("failed to load automatic-event map: ") + exception.what(),
			camera);
	}
	if (!hasAutomaticTrigger(map->geometry, camera.x, camera.y))
		return XeenAutomaticEventNoTrigger{};

	const XeenCamera beforeCamera = camera;
	const XeenGameFlags beforeFlags = gameFlags;
	const auto provider = [this](std::uint16_t mapId) {
		return scriptForMap(mapId);
	};
	const XeenEventExecutionResult execution = _interpreter.execute(camera,
		partyState, gameFlags, world, provider);
	if (const auto *executionError =
			std::get_if<XeenEventExecutionError>(&execution))
		return *executionError;

	const auto &completed = std::get<XeenEventExecutionCompleted>(execution);
	XeenAutomaticEventCompleted result;
	result.instructionCount = completed.instructionCount;
	result.cameraChanged = !sameCamera(beforeCamera, completed.finalCamera);
	result.flagsChanged = beforeFlags.values() != completed.finalGameFlags.values();
	camera = completed.finalCamera;
	gameFlags = completed.finalGameFlags;
	return result;
}

} // namespace mmodern
