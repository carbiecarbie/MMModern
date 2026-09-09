#include "app/XeenNavigationFlow.h"

#include "games/xeen/XeenEventScript.h"
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

XeenEventScript script(mmodern::XeenMapIdentity mapId,
		std::vector<XeenEventRecord> records = {}) {
	XeenEventFile file;
	file.mapId = mapId;
	file.resourceName = "maze" + std::to_string(mapId.number) + ".evt";
	file.resourcePresent = true;
	file.records = std::move(records);
	return XeenEventScript(std::move(file));
}

XeenMap freeMap(mmodern::XeenMapIdentity id) {
	XeenMap result;
	result.geometry.id = id.number;
	result.side = id.side;
	result.geometry.flags2 = 0x8000;
	result.geometry.surfaceTypes[6] = 6;
	for (auto &cell : result.geometry.cells) {
		XeenOutdoorLayers layers;
		layers.surface = 6;
		cell.geometry = layers;
	}
	return result;
}

void addTrigger(XeenMap &map, int x, int y) {
	map.geometry.cells[static_cast<std::size_t>(y) * 16 +
		static_cast<std::size_t>(x)].rawAttributes |= kXeenAutomaticEventFlag;
}

XeenOutdoorLayers &layersAt(XeenMap &map, int x, int y) {
	return std::get<XeenOutdoorLayers>(
		map.geometry.cells[static_cast<std::size_t>(y) * 16 +
			static_cast<std::size_t>(x)].geometry);
}

std::vector<std::uint8_t> setFlag(std::uint8_t flag) {
	return {0, 0, 20, flag};
}

class Fixture {
public:
	XeenPartyState party;
	Fixture() :
		world([this](mmodern::XeenMapIdentity id) {
			const auto found = maps.find(id);
			if (found == maps.end())
				throw std::runtime_error("synthetic map missing");
			return found->second;
		}),
		events([this](mmodern::XeenMapIdentity id) {
			++scriptLoads[id];
			const auto found = scripts.find(id);
			return found == scripts.end() ? script(id) : found->second;
		}),
		flow(events) {}

	std::map<mmodern::XeenMapIdentity, XeenMap> maps;
	std::map<mmodern::XeenMapIdentity, XeenEventScript> scripts;
	std::map<mmodern::XeenMapIdentity, int> scriptLoads;
	XeenWorld world;
	XeenEventSystem events;
	XeenNavigationFlow flow;
};

XeenAutomaticEventCompleted completed(const XeenAutomaticEventResult &result) {
	const auto *value = std::get_if<XeenAutomaticEventCompleted>(&result);
	check(value != nullptr, "expected successful automatic event");
	return *value;
}

XeenEventExecutionError failure(const XeenAutomaticEventResult &result,
		XeenEventExecutionErrorKind kind) {
	const auto *value = std::get_if<XeenEventExecutionError>(&result);
	check(value && value->kind == kind, "unexpected event error");
	return *value;
}

void checkCamera(const XeenCamera &camera, mmodern::XeenMapIdentity mapId, int x, int y,
		XeenDirection direction, const char *message) {
	check(camera.mapId == mapId && camera.x == x && camera.y == y &&
		camera.direction == direction, message);
}

void testInitialAndBasicActions() {
	Fixture initial;
	auto map1 = freeMap(1);
	addTrigger(map1, 1, 1);
	initial.maps.emplace(1, map1);
	initial.maps.emplace(2, freeMap(2));
	initial.scripts.emplace(1, script(1,
		{record(1, 1, 0, 0x07, {2, 3, 4})}));
	XeenCamera camera{1, 1, 1, XeenDirection::North};
	XeenGameFlags flags;
	const auto initialResult = completed(initial.flow.processInitialEvent(
		initial.world, initial.party, camera, flags));
	check(initialResult.instructionCount == 1,
		"initial event executes before state is consumed");
	checkCamera(camera, 2, 3, 4, XeenDirection::North,
		"initial state is event destination");

	Fixture forward;
	auto forwardMap = freeMap(1);
	addTrigger(forwardMap, 1, 2);
	forward.maps.emplace(1, forwardMap);
	forward.maps.emplace(2, freeMap(2));
	forward.scripts.emplace(1, script(1,
		{record(1, 2, 0, 0x07, {2, 2, 2})}));
	XeenCamera forwardCamera{1, 1, 1, XeenDirection::North};
	const auto forwardResult = forward.flow.processNavigationAction(forward.world, forward.party, forwardCamera, flags, NavigationAction::MoveForward);
	check(forwardResult.movementResult == XeenMovementResult::Moved,
		"forward movement processed");
	completed(forwardResult.automaticEvent);
	checkCamera(forwardResult.cameraAfterMovement, 1, 1, 2, XeenDirection::North,
		"movement observation occurred after automatic teleport");
	checkCamera(forwardCamera, 2, 2, 2, XeenDirection::North,
		"forward destination event executes before output state");

	Fixture backward;
	auto backwardMap = freeMap(1);
	addTrigger(backwardMap, 1, 1);
	backward.maps.emplace(1, backwardMap);
	backward.scripts.emplace(1, script(1,
		{record(1, 1, 0, 0x0c, setFlag(7))}));
	XeenCamera backwardCamera{1, 1, 2, XeenDirection::North};
	XeenGameFlags backwardFlags;
	const auto backwardResult = backward.flow.processNavigationAction(backward.world, backward.party, backwardCamera, backwardFlags, NavigationAction::MoveBackward);
	check(backwardResult.movementResult == XeenMovementResult::Moved &&
		backwardFlags.isSet(7), "backward checks resulting cell");

	Fixture none;
	none.maps.emplace(1, freeMap(1));
	XeenCamera noneCamera{1, 1, 1, XeenDirection::North};
	XeenGameFlags noneFlags;
	const auto noneResult = none.flow.processNavigationAction(none.world, none.party,
		noneCamera, noneFlags, NavigationAction::TurnLeft);
	check(noneResult.movementResult == XeenMovementResult::Turned &&
		std::holds_alternative<XeenAutomaticEventNoTrigger>(noneResult.automaticEvent) &&
		none.scriptLoads.empty(), "cell without trigger has no event effect");
}

void testBlockedAndRotatedActions() {
	Fixture blocked;
	auto blockedMap = freeMap(1);
	addTrigger(blockedMap, 1, 1);
	layersAt(blockedMap, 1, 2).middle = 1;
	blocked.maps.emplace(1, blockedMap);
	blocked.maps.emplace(2, freeMap(2));
	blocked.scripts.emplace(1, script(1, {
		record(1, 1, 0, 0x09, {20, 25, 3}),
		record(1, 1, 1, 0x0c, setFlag(25)),
		record(1, 1, 2, 0x12),
		record(1, 1, 3, 0x07, {2, 2, 2})
	}));
	XeenCamera blockedCamera{1, 1, 1, XeenDirection::North};
	XeenGameFlags blockedFlags;
	const auto blockedResult = blocked.flow.processNavigationAction(blocked.world, blocked.party,
		blockedCamera, blockedFlags, NavigationAction::MoveForward);
	check(blockedResult.movementResult == XeenMovementResult::BlockedByTerrain &&
		blockedFlags.isSet(25), "blocked movement still executes current trigger");
	checkCamera(blockedCamera, 1, 1, 1, XeenDirection::North,
		"automatic event executes exactly once after blocked movement");

	for (const auto action : {NavigationAction::TurnLeft,
			NavigationAction::TurnRight}) {
		Fixture rotated;
		auto rotatedMap = freeMap(1);
		addTrigger(rotatedMap, 1, 1);
		rotated.maps.emplace(1, rotatedMap);
		const XeenDirection expected = action == NavigationAction::TurnLeft ?
			XeenDirection::West : XeenDirection::East;
		rotated.scripts.emplace(1, script(1, {
			record(1, 1, 0, 0x0c, setFlag(9),
				static_cast<std::uint8_t>(expected))
		}));
		XeenCamera rotatedCamera{1, 1, 1, XeenDirection::North};
		XeenGameFlags rotatedFlags;
		const auto result = rotated.flow.processNavigationAction(rotated.world, rotated.party,
			rotatedCamera, rotatedFlags, action);
		check(result.movementResult == XeenMovementResult::Turned &&
			rotatedCamera.direction == expected && rotatedFlags.isSet(9),
			"rotation event uses resulting direction");
	}
}

void testMapTransitionAndErrorBoundary() {
	Fixture transition;
	auto map1 = freeMap(1);
	auto map2 = freeMap(2);
	map1.geometry.neighbors[0] = 2;
	map2.geometry.neighbors[2] = 1;
	addTrigger(map2, 4, 0);
	transition.maps.emplace(1, map1);
	transition.maps.emplace(2, map2);
	transition.maps.emplace(3, freeMap(3));
	transition.scripts.emplace(2, script(2,
		{record(4, 0, 0, 0x07, {3, 5, 5})}));
	XeenCamera camera{1, 4, 15, XeenDirection::North};
	XeenGameFlags flags;
	const auto result = transition.flow.processNavigationAction(transition.world, transition.party,
		camera, flags, NavigationAction::MoveForward);
	check(result.movementResult == XeenMovementResult::Moved,
		"neighbor transition movement succeeds");
	checkCamera(result.cameraAfterMovement, 2, 4, 0, XeenDirection::North,
		"crossing observation lost normalized movement camera");
	checkCamera(camera, 3, 5, 5, XeenDirection::North,
		"event is checked at normalized neighbor destination");

	Fixture error;
	auto errorMap = freeMap(1);
	addTrigger(errorMap, 1, 2);
	error.maps.emplace(1, errorMap);
	error.scripts.emplace(1, script(1,
		{record(1, 2, 0, 0x06, {}, kXeenEventDirectionAll, 987)}));
	XeenCamera errorCamera{1, 1, 1, XeenDirection::North};
	XeenGameFlags errorFlags;
	const auto errorResult = error.flow.processNavigationAction(error.world, error.party,
		errorCamera, errorFlags, NavigationAction::MoveForward);
	const auto diagnostic = failure(errorResult.automaticEvent,
		XeenEventExecutionErrorKind::UnsupportedOpcode);
	checkCamera(errorCamera, 1, 1, 2, XeenDirection::North,
		"failed event does not undo completed movement");
	check(diagnostic.source && diagnostic.source->fileOffset == 987,
		"flow preserves structured event diagnostic");
}

void testTeleportBoundariesAndPersistentFlags() {
	Fixture exit;
	auto map1 = freeMap(1);
	auto map2 = freeMap(2);
	addTrigger(map1, 1, 2);
	addTrigger(map2, 2, 2);
	exit.maps.emplace(1, map1);
	exit.maps.emplace(2, map2);
	exit.maps.emplace(3, freeMap(3));
	exit.scripts.emplace(1, script(1,
		{record(1, 2, 0, 0x07, {2, 2, 2})}));
	exit.scripts.emplace(2, script(2,
		{record(2, 2, 0, 0x07, {3, 3, 3})}));
	XeenCamera camera{1, 1, 1, XeenDirection::North};
	XeenGameFlags flags;
	exit.flow.processNavigationAction(exit.world, exit.party, camera, flags,
		NavigationAction::MoveForward);
	checkCamera(camera, 2, 2, 2, XeenDirection::North,
		"TeleportAndExit destination is next render state");
	check(exit.scriptLoads[2] == 0,
		"destination trigger is not chained in same action");

	Fixture continuation;
	auto continuationMap = freeMap(1);
	addTrigger(continuationMap, 1, 2);
	continuation.maps.emplace(1, continuationMap);
	continuation.maps.emplace(2, freeMap(2));
	continuation.maps.emplace(3, freeMap(3));
	continuation.scripts.emplace(1, script(1,
		{record(1, 2, 0, 0x1f, {2, 3, 4})}));
	continuation.scripts.emplace(2, script(2,
		{record(3, 4, 0, 0x07, {3, 5, 6})}));
	XeenCamera continued{1, 1, 1, XeenDirection::North};
	continuation.flow.processNavigationAction(continuation.world, continuation.party, continued,
		flags, NavigationAction::MoveForward);
	checkCamera(continued, 3, 5, 6, XeenDirection::North,
		"TeleportAndContinue completes before next render state");

	Fixture persistent;
	auto persistentMap = freeMap(1);
	addTrigger(persistentMap, 1, 1);
	addTrigger(persistentMap, 2, 1);
	persistent.maps.emplace(1, persistentMap);
	persistent.maps.emplace(2, freeMap(2));
	persistent.scripts.emplace(1, script(1, {
		record(1, 1, 0, 0x0c, setFlag(25)),
		record(2, 1, 0, 0x09, {20, 25, 2}),
		record(2, 1, 1, 0x12),
		record(2, 1, 2, 0x07, {2, 4, 4})
	}));
	XeenCamera persistentCamera{1, 0, 1, XeenDirection::East};
	XeenGameFlags persistentFlags;
	persistent.flow.processNavigationAction(persistent.world, persistent.party, persistentCamera,
		persistentFlags, NavigationAction::MoveForward);
	check(persistentFlags.isSet(25), "first action commits persistent flag");
	persistent.flow.processNavigationAction(persistent.world, persistent.party, persistentCamera,
		persistentFlags, NavigationAction::MoveForward);
	checkCamera(persistentCamera, 2, 4, 4, XeenDirection::East,
		"next action observes previously committed flag");
	check(persistent.scriptLoads[1] == 1,
		"same EventSystem cache persists across actions");
}

void testManualInteractionDoesNotRunAutomaticDispatch() {
	Fixture fixture;
	auto map1 = freeMap(1);
	addTrigger(map1, 4, 5);
	fixture.maps.emplace(1, map1);
	fixture.scripts.emplace(1, script(1, {
		record(4, 5, 0, 0x0c, setFlag(26)),
		record(4, 5, 1, 0x12)
	}));
	XeenCamera camera{1, 4, 5, XeenDirection::South};
	XeenGameFlags flags;
	const auto result = fixture.flow.processInteraction(
		fixture.world, fixture.party, camera, flags);
	const auto *value = std::get_if<XeenManualEventCompleted>(&result);
	check(value && value->instructionCount == 2 && flags.isSet(26),
		"interaction routes through manual dispatch");
	check(fixture.scriptLoads[1] == 1,
		"interaction executes once even when the cell has 0x10");
	checkCamera(camera, 1, 4, 5, XeenDirection::South,
		"interaction does not move or rotate the camera");
}

} // namespace

int main() {
	try {
		testInitialAndBasicActions();
		testBlockedAndRotatedActions();
		testMapTransitionAndErrorBoundary();
		testTeleportBoundariesAndPersistentFlags();
		testManualInteractionDoesNotRunAutomaticDispatch();
		std::cout << "Navigation action, automatic event and final state flow OK\n";
		return 0;
	} catch (const std::exception &error) {
		std::cerr << error.what() << '\n';
		return 1;
	}
}
