#include "games/xeen/XeenEventSystem.h"

#include "games/xeen/XeenEventTrigger.h"
#include "games/xeen/XeenWorld.h"

#include <stdexcept>
#include <string>
#include <type_traits>
#include <utility>
#include <optional>

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

XeenEventSystem::XeenEventSystem(ScriptProvider scriptProvider,
		TextProvider textProvider) :
	_scriptProvider(std::move(scriptProvider)),
	_textProvider(std::move(textProvider)) {
	if (!_scriptProvider)
		throw std::invalid_argument("XeenEventSystem requires a script provider");
}

XeenEventTextFile XeenEventSystem::textForMap(XeenMapIdentity mapId) {
	const auto cached = _texts.find(mapId);
	if (cached != _texts.end())
		return cached->second;
	XeenEventTextFile loaded;
	if (_textProvider) {
		loaded = _textProvider(mapId);
	} else {
		loaded.mapId = mapId;
		// An absent provider is not a Clouds resource adapter.
	}
	if (loaded.mapId != mapId)
		return loaded; // Let the interpreter report TextMapMismatch; never cache it.
	return _texts.emplace(mapId, std::move(loaded)).first->second;
}

XeenEventScript XeenEventSystem::scriptForMap(XeenMapIdentity mapId) {
	const auto cached = _scripts.find(mapId);
	if (cached != _scripts.end())
		return cached->second;

	XeenEventScript loaded = _scriptProvider(mapId);
	if (loaded.file().mapId != mapId)
		throw std::runtime_error("event script identity differs from requested map");
	return _scripts.emplace(mapId, std::move(loaded)).first->second;
}

XeenAutomaticEventResult XeenEventSystem::runAutomaticEvent(
		XeenWorld &world, XeenPartyState &partyState, XeenCamera &camera,
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
	const auto provider = [this](XeenMapIdentity mapId) {
		return scriptForMap(mapId);
	};
	const auto textProvider = [this](XeenMapIdentity mapId) {
		return textForMap(mapId);
	};
	XeenEventExecutionStepResult execution = _interpreter.begin(camera,
		partyState, gameFlags, world, provider, textProvider);
	if (const auto *executionError =
			std::get_if<XeenEventExecutionError>(&execution))
		return *executionError;
	if (auto *suspended =
			std::get_if<XeenEventExecutionSuspended>(&execution))
		return std::move(*suspended);

	const auto &completed = std::get<XeenEventExecutionCompleted>(execution);
	XeenAutomaticEventCompleted result;
	result.instructionCount = completed.instructionCount;
	result.cameraChanged = !sameCamera(beforeCamera, completed.finalCamera);
	result.flagsChanged = beforeFlags.values() != completed.finalGameFlags.values();
	camera = completed.finalCamera;
	gameFlags = completed.finalGameFlags;
	return result;
}

XeenManualEventResult XeenEventSystem::runManualEvent(
		XeenWorld &world, XeenPartyState &partyState, XeenCamera &camera,
		XeenGameFlags &gameFlags) {
	if (!camera.mapId || camera.x < 0 || camera.x > 15 || camera.y < 0 ||
			camera.y > 15 || !validDirection(camera.direction)) {
		return systemError(XeenEventExecutionErrorKind::InvalidInitialCamera,
			"manual event camera is outside the supported Xeen map domain", camera);
	}

	const XeenMap *map = nullptr;
	try {
		map = &world.map(camera.mapId);
	} catch (const std::exception &exception) {
		return systemError(XeenEventExecutionErrorKind::MapLoadFailed,
			std::string("failed to load manual-event map: ") + exception.what(), camera);
	}
	if (const auto wall = unsupportedManualSpecialInteraction(map->geometry,
			camera.x, camera.y, camera.direction))
		return XeenManualSpecialInteractionUnsupported{*wall};

	std::optional<XeenEventScript> script;
	try {
		script.emplace(scriptForMap(camera.mapId));
	} catch (const std::exception &exception) {
		return systemError(XeenEventExecutionErrorKind::ScriptLoadFailed,
			std::string("failed to load manual event script: ") + exception.what(), camera);
	}
	if (!script->findInstruction(static_cast<std::uint8_t>(camera.x),
			static_cast<std::uint8_t>(camera.y), camera.direction, 0))
		return XeenManualEventNoEvent{};

	const XeenCamera beforeCamera = camera;
	const XeenGameFlags beforeFlags = gameFlags;
	const auto provider = [this](XeenMapIdentity mapId) {
		return scriptForMap(mapId);
	};
	const auto textProvider = [this](XeenMapIdentity mapId) {
		return textForMap(mapId);
	};
	XeenEventExecutionStepResult execution = _interpreter.begin(camera,
		partyState, gameFlags, world, provider, textProvider);
	if (const auto *executionError =
			std::get_if<XeenEventExecutionError>(&execution))
		return *executionError;
	if (auto *suspended =
			std::get_if<XeenEventExecutionSuspended>(&execution))
		return std::move(*suspended);

	const auto &completed = std::get<XeenEventExecutionCompleted>(execution);
	XeenManualEventCompleted result;
	result.instructionCount = completed.instructionCount;
	result.cameraChanged = !sameCamera(beforeCamera, completed.finalCamera);
	result.flagsChanged = beforeFlags.values() != completed.finalGameFlags.values();
	camera = completed.finalCamera;
	gameFlags = completed.finalGameFlags;
	return result;
}

XeenAutomaticEventResult XeenEventSystem::resumeAutomaticEvent(
		XeenEventExecutionState state, XeenPresentationResponse response,
		XeenWorld &world, XeenPartyState &partyState, XeenCamera &camera,
		XeenGameFlags &gameFlags) {
	const XeenCamera beforeCamera = camera;
	const XeenGameFlags beforeFlags = gameFlags;
	const auto provider = [this](XeenMapIdentity mapId) { return scriptForMap(mapId); };
	const auto textProvider = [this](XeenMapIdentity mapId) { return textForMap(mapId); };
	XeenEventExecutionStepResult execution = _interpreter.resume(
		std::move(state), response, partyState, world, provider, textProvider);
	if (const auto *value = std::get_if<XeenEventExecutionError>(&execution))
		return *value;
	if (auto *value = std::get_if<XeenEventExecutionSuspended>(&execution))
		return std::move(*value);
	const auto &value = std::get<XeenEventExecutionCompleted>(execution);
	XeenAutomaticEventCompleted result{value.instructionCount,
		!sameCamera(beforeCamera, value.finalCamera),
		beforeFlags.values() != value.finalGameFlags.values()};
	camera = value.finalCamera;
	gameFlags = value.finalGameFlags;
	return result;
}

XeenManualEventResult XeenEventSystem::resumeManualEvent(
		XeenEventExecutionState state, XeenPresentationResponse response,
		XeenWorld &world, XeenPartyState &partyState, XeenCamera &camera,
		XeenGameFlags &gameFlags) {
	const XeenCamera beforeCamera = camera;
	const XeenGameFlags beforeFlags = gameFlags;
	const auto provider = [this](XeenMapIdentity mapId) { return scriptForMap(mapId); };
	const auto textProvider = [this](XeenMapIdentity mapId) { return textForMap(mapId); };
	XeenEventExecutionStepResult execution = _interpreter.resume(
		std::move(state), response, partyState, world, provider, textProvider);
	if (const auto *value = std::get_if<XeenEventExecutionError>(&execution))
		return *value;
	if (auto *value = std::get_if<XeenEventExecutionSuspended>(&execution))
		return std::move(*value);
	const auto &value = std::get<XeenEventExecutionCompleted>(execution);
	XeenManualEventCompleted result{value.instructionCount,
		!sameCamera(beforeCamera, value.finalCamera),
		beforeFlags.values() != value.finalGameFlags.values()};
	camera = value.finalCamera;
	gameFlags = value.finalGameFlags;
	return result;
}

} // namespace mmodern
