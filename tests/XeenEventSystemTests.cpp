#include "games/xeen/XeenEventSystem.h"

#include "games/xeen/XeenEventTrigger.h"
#include "games/xeen/XeenWorld.h"

#include <cstdint>
#include <iostream>
#include <map>
#include <stdexcept>
#include <string>
#include <utility>
#include <variant>
#include <vector>

using namespace mmodern;

namespace {

void check(bool value, const char *message) {
	if (!value)
		throw std::runtime_error(message);
}

XeenEventRecord record(std::uint8_t x, std::uint8_t y, std::uint8_t line,
		std::uint8_t opcode, std::vector<std::uint8_t> parameters = {},
		std::uint8_t direction = kXeenEventDirectionAll,
		std::size_t offset = 1) {
	XeenEventRecord result;
	result.fileOffset = offset;
	result.lengthField = static_cast<std::uint8_t>(5 + parameters.size());
	result.x = x;
	result.y = y;
	result.direction = direction;
	result.line = line;
	result.opcode = opcode;
	result.parameters = std::move(parameters);
	return result;
}

XeenEventScript script(std::uint16_t mapId,
		std::vector<XeenEventRecord> records = {}, bool present = true) {
	XeenEventFile file;
	file.mapId = mapId;
	file.resourceName = "maze" + std::to_string(mapId) + ".evt";
	file.resourcePresent = present;
	file.records = std::move(records);
	return XeenEventScript(std::move(file));
}

XeenMap map(std::uint16_t mapId, bool outdoors = true,
		std::vector<std::pair<int, int>> triggers = {}) {
	XeenMap result;
	result.geometry.id = mapId;
	result.geometry.flags2 = outdoors ? 0x8000 : 0;
	for (const auto &trigger : triggers) {
		const auto index = static_cast<std::size_t>(trigger.second) *
			XeenMapGeometry::kWidth + static_cast<std::size_t>(trigger.first);
		result.geometry.cells[index].rawAttributes |= kXeenAutomaticEventFlag;
	}
	return result;
}

std::vector<std::uint8_t> setFlag(std::uint8_t flag) {
	return {0, 0, 20, flag};
}

std::vector<std::uint8_t> clearFlag(std::uint8_t flag) {
	return {20, flag};
}

class Fixture {
public:
	Fixture() :
		world([this](std::uint16_t mapId) {
			++mapLoads[mapId];
			if (failingMaps.count(mapId))
				throw std::runtime_error("synthetic map load failure");
			const auto found = maps.find(mapId);
			return found == maps.end() ? map(mapId) : found->second;
		}),
		system([this](std::uint16_t mapId) {
			++scriptLoads[mapId];
			auto failure = failuresRemaining.find(mapId);
			if (failure != failuresRemaining.end() && failure->second > 0) {
				--failure->second;
				throw std::runtime_error("synthetic script load failure");
			}
			const auto found = scripts.find(mapId);
			return found == scripts.end() ? script(mapId, {}, false) : found->second;
		}) {}

	std::map<std::uint16_t, XeenMap> maps;
	std::map<std::uint16_t, XeenEventScript> scripts;
	std::map<std::uint16_t, int> mapLoads;
	std::map<std::uint16_t, int> scriptLoads;
	std::map<std::uint16_t, int> failuresRemaining;
	std::map<std::uint16_t, bool> failingMaps;
	XeenWorld world;
	XeenEventSystem system;
};

XeenAutomaticEventCompleted completed(const XeenAutomaticEventResult &result) {
	const auto *value = std::get_if<XeenAutomaticEventCompleted>(&result);
	check(value != nullptr, "expected completed automatic event");
	return *value;
}

XeenEventExecutionError failure(const XeenAutomaticEventResult &result,
		XeenEventExecutionErrorKind kind) {
	const auto *value = std::get_if<XeenEventExecutionError>(&result);
	check(value && value->kind == kind, "unexpected automatic-event error");
	return *value;
}

void checkCamera(const XeenCamera &camera, std::uint16_t mapId, int x, int y,
		XeenDirection direction, const char *message) {
	check(camera.mapId == mapId && camera.x == x && camera.y == y &&
		camera.direction == direction, message);
}

void testValidationAndNoTrigger() {
	Fixture invalid;
	XeenCamera bad{0, -1, 16, static_cast<XeenDirection>(9)};
	XeenGameFlags flags;
	failure(invalid.system.runAutomaticEvent(invalid.world, {}, bad, flags),
		XeenEventExecutionErrorKind::InvalidInitialCamera);
	check(invalid.mapLoads.empty() && invalid.scriptLoads.empty(),
		"invalid camera performs no loads");

	Fixture mapFailure;
	mapFailure.failingMaps[1] = true;
	XeenCamera camera{1, 1, 1, XeenDirection::North};
	failure(mapFailure.system.runAutomaticEvent(mapFailure.world, {}, camera, flags),
		XeenEventExecutionErrorKind::MapLoadFailed);
	check(mapFailure.scriptLoads.empty(), "map failure does not load script");

	Fixture noTrigger;
	noTrigger.maps.emplace(1, map(1));
	const auto result = noTrigger.system.runAutomaticEvent(
		noTrigger.world, {}, camera, flags);
	check(std::holds_alternative<XeenAutomaticEventNoTrigger>(result),
		"cell without flag returns NoTrigger");
	check(noTrigger.scriptLoads.empty() && noTrigger.system.cachedScriptCount() == 0,
		"NoTrigger does not load or cache scripts");
}

void testTriggerKindsDirectionsAndEmptyScripts() {
	for (const bool outdoors : {false, true}) {
		Fixture fixture;
		fixture.maps.emplace(1, map(1, outdoors, {{1, 1}}));
		fixture.scripts.emplace(1, script(1, {record(1, 1, 0, 0x12)}));
		XeenCamera camera{1, 1, 1, XeenDirection::North};
		XeenGameFlags flags;
		const auto result = completed(fixture.system.runAutomaticEvent(
			fixture.world, {}, camera, flags));
		check(result.instructionCount == 1, "interior/outdoor trigger executes");
	}

	const std::vector<XeenDirection> directions{
		XeenDirection::North, XeenDirection::East,
		XeenDirection::South, XeenDirection::West
	};
	for (std::size_t index = 0; index < directions.size(); ++index) {
		Fixture fixture;
		fixture.maps.emplace(1, map(1, true, {{1, 1}}));
		fixture.scripts.emplace(1, script(1, {
			record(1, 1, 0, 0x07,
				{static_cast<std::uint8_t>(index + 2), 2, 2},
				static_cast<std::uint8_t>(index))
		}));
		XeenCamera camera{1, 1, 1, directions[index]};
		XeenGameFlags flags;
		const auto result = completed(fixture.system.runAutomaticEvent(
			fixture.world, {}, camera, flags));
		check(result.instructionCount == 1 && result.cameraChanged,
			"direction-specific trigger executes");
		checkCamera(camera, static_cast<std::uint16_t>(index + 2), 2, 2,
			directions[index], "direction preserved by trigger");
	}

	for (const int kind : {0, 1, 2}) {
		Fixture fixture;
		fixture.maps.emplace(1, map(1, true, {{1, 1}}));
		if (kind == 1)
			fixture.scripts.emplace(1, script(1, {}, true));
		else if (kind == 2)
			fixture.scripts.emplace(1, script(1,
				{record(1, 1, 0, 0x12, {},
					static_cast<std::uint8_t>(XeenDirection::North))}));
		XeenCamera camera{1, 1, 1, XeenDirection::West};
		XeenGameFlags flags;
		const auto result = completed(fixture.system.runAutomaticEvent(
			fixture.world, {}, camera, flags));
		check(result.instructionCount == 0 && !result.cameraChanged &&
			!result.flagsChanged, "trigger without matching instruction is success");
	}
}

void testCommitAndRollback() {
	Fixture commit;
	commit.maps.emplace(1, map(1, true, {{1, 1}}));
	commit.scripts.emplace(1, script(1, {
		record(1, 1, 0, 0x0c, setFlag(25)),
		record(1, 1, 1, 0x07, {2, 3, 4})
	}));
	XeenCamera camera{1, 1, 1, XeenDirection::East};
	XeenGameFlags flags;
	const auto committed = completed(commit.system.runAutomaticEvent(
		commit.world, {}, camera, flags));
	check(committed.instructionCount == 2 && committed.cameraChanged &&
		committed.flagsChanged && flags.isSet(25),
		"camera and SET commit together");
	checkCamera(camera, 2, 3, 4, XeenDirection::East, "teleport committed");

	Fixture clear;
	clear.maps.emplace(1, map(1, true, {{1, 1}}));
	clear.scripts.emplace(1, script(1,
		{record(1, 1, 0, 0x0c, clearFlag(25))}));
	XeenCamera clearCamera{1, 1, 1, XeenDirection::North};
	XeenGameFlags clearFlags;
	clearFlags.set(25);
	const auto cleared = completed(clear.system.runAutomaticEvent(
		clear.world, {}, clearCamera, clearFlags));
	check(cleared.flagsChanged && !clearFlags.isSet(25) &&
		!cleared.cameraChanged, "CLEAR committed");

	Fixture netZero;
	netZero.maps.emplace(1, map(1, true, {{1, 1}}));
	netZero.scripts.emplace(1, script(1, {
		record(1, 1, 0, 0x0c, setFlag(25)),
		record(1, 1, 1, 0x0c, clearFlag(25))
	}));
	XeenCamera netCamera{1, 1, 1, XeenDirection::North};
	XeenGameFlags netFlags;
	const auto net = completed(netZero.system.runAutomaticEvent(
		netZero.world, {}, netCamera, netFlags));
	check(!net.flagsChanged && !netFlags.isSet(25),
		"Changed reports final difference, not intermediate mutation");

	Fixture rollback;
	rollback.maps.emplace(1, map(1, true, {{1, 1}}));
	rollback.scripts.emplace(1, script(1, {
		record(1, 1, 0, 0x0c, setFlag(25)),
		record(1, 1, 1, 0x06, {}, kXeenEventDirectionAll, 987)
	}));
	XeenCamera rollbackCamera{1, 1, 1, XeenDirection::South};
	XeenGameFlags rollbackFlags;
	const auto rollbackError = failure(rollback.system.runAutomaticEvent(
		rollback.world, {}, rollbackCamera, rollbackFlags),
		XeenEventExecutionErrorKind::UnsupportedOpcode);
	check(rollbackError.source && rollbackError.source->fileOffset == 987 &&
		!rollbackFlags.isSet(25), "rollback preserves diagnostic and flags");
	checkCamera(rollbackCamera, 1, 1, 1, XeenDirection::South,
		"rollback preserves pre-event camera");

	Fixture history;
	history.maps.emplace(1, map(1, true, {{1, 1}, {2, 2}}));
	history.scripts.emplace(1, script(1, {
		record(1, 1, 0, 0x0c, setFlag(7)),
		record(2, 2, 0, 0x0c, setFlag(8)),
		record(2, 2, 1, 0x06)
	}));
	XeenCamera historyCamera{1, 1, 1, XeenDirection::North};
	XeenGameFlags historyFlags;
	completed(history.system.runAutomaticEvent(
		history.world, {}, historyCamera, historyFlags));
	check(historyFlags.isSet(7), "first event committed before later failure");
	// This position represents movement completed before the second event call.
	historyCamera.x = 2;
	historyCamera.y = 2;
	failure(history.system.runAutomaticEvent(
		history.world, {}, historyCamera, historyFlags),
		XeenEventExecutionErrorKind::UnsupportedOpcode);
	checkCamera(historyCamera, 1, 2, 2, XeenDirection::North,
		"event rollback does not undo earlier movement");
	check(historyFlags.isSet(7) && !historyFlags.isSet(8),
		"later failure preserves previously committed flags only");

	Fixture workingTeleport;
	workingTeleport.maps.emplace(1, map(1, true, {{1, 1}}));
	workingTeleport.scripts.emplace(1, script(1, {
		record(1, 1, 0, 0x1f, {2, 3, 4})
	}));
	workingTeleport.scripts.emplace(2, script(2, {
		record(3, 4, 0, 0x06)
	}));
	XeenCamera teleportCamera{1, 1, 1, XeenDirection::West};
	XeenGameFlags teleportFlags;
	failure(workingTeleport.system.runAutomaticEvent(workingTeleport.world, {},
		teleportCamera, teleportFlags), XeenEventExecutionErrorKind::UnsupportedOpcode);
	checkCamera(teleportCamera, 1, 1, 1, XeenDirection::West,
		"working teleport is not committed after error");
}

void testCacheAndRetry() {
	Fixture fixture;
	fixture.maps.emplace(1, map(1, true, {{1, 1}}));
	fixture.maps.emplace(2, map(2, true, {{1, 1}}));
	fixture.scripts.emplace(1, script(1, {record(1, 1, 0, 0x12)}));
	fixture.scripts.emplace(2, script(2, {}, true));
	XeenGameFlags flags;
	for (int iteration = 0; iteration < 2; ++iteration) {
		XeenCamera one{1, 1, 1, XeenDirection::North};
		completed(fixture.system.runAutomaticEvent(fixture.world, {}, one, flags));
		XeenCamera two{2, 1, 1, XeenDirection::North};
		completed(fixture.system.runAutomaticEvent(fixture.world, {}, two, flags));
	}
	check(fixture.scriptLoads[1] == 1 && fixture.scriptLoads[2] == 1 &&
		fixture.system.cachedScriptCount() == 2,
		"present and empty scripts are cached across A-B-A");

	Fixture absent;
	absent.maps.emplace(1, map(1, true, {{1, 1}}));
	for (int iteration = 0; iteration < 2; ++iteration) {
		XeenCamera camera{1, 1, 1, XeenDirection::North};
		completed(absent.system.runAutomaticEvent(absent.world, {}, camera, flags));
	}
	check(absent.scriptLoads[1] == 1 && absent.system.cachedScriptCount() == 1,
		"absent event resource is cached");

	Fixture retry;
	retry.maps.emplace(1, map(1, true, {{1, 1}}));
	retry.scripts.emplace(1, script(1, {record(1, 1, 0, 0x12)}));
	retry.failuresRemaining[1] = 1;
	XeenCamera camera{1, 1, 1, XeenDirection::North};
	failure(retry.system.runAutomaticEvent(retry.world, {}, camera, flags),
		XeenEventExecutionErrorKind::ScriptLoadFailed);
	completed(retry.system.runAutomaticEvent(retry.world, {}, camera, flags));
	check(retry.scriptLoads[1] == 2 && retry.system.cachedScriptCount() == 1,
		"failed load is not cached and can be retried");
}

void testTeleportTriggerBoundary() {
	Fixture exit;
	exit.maps.emplace(1, map(1, true, {{1, 1}}));
	exit.maps.emplace(2, map(2, true, {{2, 2}}));
	exit.scripts.emplace(1, script(1, {record(1, 1, 0, 0x07, {2, 2, 2})}));
	exit.scripts.emplace(2, script(2, {record(2, 2, 0, 0x07, {3, 3, 3})}));
	XeenCamera camera{1, 1, 1, XeenDirection::North};
	XeenGameFlags flags;
	completed(exit.system.runAutomaticEvent(exit.world, {}, camera, flags));
	checkCamera(camera, 2, 2, 2, XeenDirection::North,
		"TeleportAndExit stops at first destination");
	check(exit.scriptLoads[1] == 1 && exit.scriptLoads[2] == 0,
		"TeleportAndExit does not load destination trigger script");
	completed(exit.system.runAutomaticEvent(exit.world, {}, camera, flags));
	checkCamera(camera, 3, 3, 3, XeenDirection::North,
		"separate call executes destination trigger");

	Fixture continuation;
	continuation.maps.emplace(1, map(1, true, {{1, 1}}));
	continuation.maps.emplace(2, map(2, true));
	continuation.scripts.emplace(1, script(1, {
		record(1, 1, 0, 0x1f, {2, 3, 4})
	}));
	continuation.scripts.emplace(2, script(2, {
		record(3, 4, 0, 0x12)
	}));
	XeenCamera continued{1, 1, 1, XeenDirection::East};
	completed(continuation.system.runAutomaticEvent(
		continuation.world, {}, continued, flags));
	checkCamera(continued, 2, 3, 4, XeenDirection::East,
		"TeleportAndContinue executes destination without physical trigger");
	check(continuation.scriptLoads[1] == 1 && continuation.scriptLoads[2] == 1 &&
		continuation.system.cachedScriptCount() == 2,
		"TeleportAndContinue uses shared script cache");
}

} // namespace

int main() {
	try {
		testValidationAndNoTrigger();
		testTriggerKindsDirectionsAndEmptyScripts();
		testCommitAndRollback();
		testCacheAndRetry();
		testTeleportTriggerBoundary();
		std::cout << "Automatic Xeen event orchestration, cache and commit OK\n";
		return 0;
	} catch (const std::exception &error) {
		std::cerr << error.what() << '\n';
		return 1;
	}
}
