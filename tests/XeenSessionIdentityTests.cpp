#include "games/xeen/XeenWorld.h"
#include "games/xeen/XeenMovement.h"
#include "games/xeen/XeenEventSystem.h"
#include "games/xeen/XeenEventLoader.h"

#include <iostream>
#include <map>
#include <sstream>
#include <stdexcept>

using namespace mmodern;

namespace {
void check(bool value, const char *message) {
	if (!value) throw std::runtime_error(message);
}

XeenMap map(XeenMapIdentity id) {
	XeenMap result;
	result.side = id.side;
	result.geometry.id = id.number;
	result.geometry.flags2 = 0x8000;
	result.geometry.surfaceTypes.fill(1);
	for (auto &cell : result.geometry.cells) cell.geometry = XeenOutdoorLayers{};
	return result;
}

XeenEventRecord record(int x, int y, int line, int opcode,
		std::vector<std::uint8_t> parameters = {}) {
	return {10, static_cast<std::uint8_t>(5 + parameters.size()),
		static_cast<std::uint8_t>(x), static_cast<std::uint8_t>(y),
		kXeenEventDirectionAll, static_cast<std::uint8_t>(line),
		static_cast<std::uint8_t>(opcode), std::move(parameters)};
}

XeenEventScript script(XeenMapIdentity id, std::vector<XeenEventRecord> records) {
	return XeenEventScript({id, "synthetic.evt", true, std::move(records)});
}

void worldLifetimeAndNavigation() {
	const XeenMapIdentity clouds{XeenSide::Clouds, 1}, dark{XeenSide::Darkside, 1};
	std::map<XeenMapIdentity, int> loads;
	XeenWorld world([&](XeenMapIdentity id) {
		++loads[id];
		auto result = map(id);
		result.geometry.neighbors = {2, 3, 4, 5};
		return result;
	});
	const auto *session = &world.sessionState();
	check(world.map(clouds).identity() == clouds && world.map(dark).identity() == dark,
		"equal numbers must have separate side identities");
	check(world.cachedMapCount() == 2 && loads[clouds] == 1 && loads[dark] == 1,
		"world side caches collided");
	XeenCamera camera{dark, 4, 4, XeenDirection::North};
	check(XeenMovement().apply(world, camera, NavigationAction::MoveForward) ==
		XeenMovementResult::Moved && camera.mapId == dark && camera.y == 5,
		"ordinary movement lost side");
	camera.y = 15;
	XeenMovement().apply(world, camera, NavigationAction::MoveForward);
	check(camera.mapId == XeenMapIdentity{XeenSide::Darkside, 2} && camera.y == 0,
		"neighbor navigation lost side");
	const auto diagonal = world.sampleCell(dark, 16, -1);
	check(diagonal && diagonal->mapId == XeenMapIdentity{XeenSide::Darkside, 3} &&
		diagonal->x == 0 && diagonal->y == 15 && loads[{XeenSide::Darkside, 4}] == 1,
		"diagonal must resolve Y then X on the same side");
	world.discardMapCache();
	check(world.cachedMapCount() == 0 && session == &world.sessionState(),
		"discard replaced the session state");
	world.map(dark);
	check(loads[dark] == 2, "map cache did not reload");
	XeenWorld fresh([](XeenMapIdentity id) { return map(id); });
	check(&fresh.sessionState() != session, "new world reused session owner");
	int indoorLoads = 0;
	XeenWorld indoor([&](XeenMapIdentity id) {
		++indoorLoads; auto result = map(id); result.geometry.flags2 = 0;
		result.geometry.neighbors = {2,3,4,5};
		for (auto &cell : result.geometry.cells) cell.geometry = XeenIndoorWalls{};
		return result;
	});
	check(indoor.sampleCell(dark, 4, 4)->mapId == dark && !indoor.sampleCell(dark, -1, 4) &&
		indoorLoads == 1, "indoor sampling lost side or followed neighbors");
	XeenWorld wrong([](XeenMapIdentity id) { return map(id.number); });
	bool rejected = false;
	try { wrong.map(dark); } catch (const std::runtime_error &) { rejected = true; }
	check(rejected && wrong.cachedMapCount() == 0, "wrong-side map accepted");
}

void eventCachesCallsAndResume() {
	XeenPartyState party;
	std::map<XeenMapIdentity, int> scripts, texts, maps;
	XeenWorld world([&](XeenMapIdentity id) { ++maps[id]; return map(id); });
	XeenEventSystem events([&](XeenMapIdentity id) {
		++scripts[id];
		if (id.number == 2) return script(id, {record(2, 2, 0, 0x01, {0}),
			record(2, 2, 1, 0x12)});
		return script(id, {record(1, 1, 0, 0x19, {7, 8, 0}),
			record(1, 1, 1, 0x1f, {2, 2, 2}),
			record(7, 8, 0, 0x01, {0}), record(7, 8, 1, 0x1a)});
	}, [&](XeenMapIdentity id) {
		++texts[id];
		return XeenEventTextFile{id, "synthetic.txt", true,
			{id.side == XeenSide::Clouds ? "Clouds" : "Darkside"}};
	});
	for (auto side : {XeenSide::Clouds, XeenSide::Darkside}) {
		const XeenMapIdentity id{side, 1};
		XeenCamera camera{id, 1, 1, XeenDirection::North};
		XeenGameFlags flags;
		auto start = events.runManualEvent(world, party, camera, flags);
		auto pending = std::get<XeenEventExecutionSuspended>(start);
		check(pending.state.logicalAddress.mapId == id && pending.state.logicalAddress.x == 7 &&
			pending.state.workingCamera.x == 1 && pending.state.callStack.size() == 1 &&
			pending.state.callStack[0].returnAddress.mapId == id && pending.request.source.mapId == id,
			"call/suspension identities or physical/logical distinction lost");
		check(pending.request.text == (side == XeenSide::Clouds ? "Clouds" : "Darkside"),
			"text cache crossed sides");
		// Remove base caches while a call is suspended; its owned script stays valid.
		world.discardMapCache();
		events.discardScriptCache();
		events.discardTextCache();
		auto resumed = events.resumeManualEvent(pending.state, XeenPresentationResponse::Presented,
			world, party, camera, flags);
		pending = std::get<XeenEventExecutionSuspended>(resumed);
		check(pending.state.logicalAddress.mapId == XeenMapIdentity{side, 2} &&
			pending.state.workingCamera.mapId == XeenMapIdentity{side, 2} &&
			pending.state.callStack.empty() && camera.mapId == id,
			"return/teleport or deferred commit lost side");
		auto done = events.resumeManualEvent(pending.state, XeenPresentationResponse::Presented,
			world, party, camera, flags);
		check(std::holds_alternative<XeenManualEventCompleted>(done) &&
			camera.mapId == XeenMapIdentity{side, 2}, "teleport commit failed");
	}
	// Populate both side keys concurrently, then prove reuse and actual reload.
	events.discardScriptCache(); events.discardTextCache();
	for (int pass = 0; pass < 2; ++pass) {
		for (auto side : {XeenSide::Clouds, XeenSide::Darkside}) {
			XeenCamera camera{{side, 2}, 2, 2, XeenDirection::North};
			XeenGameFlags flags;
			check(std::holds_alternative<XeenEventExecutionSuspended>(
				events.runManualEvent(world, party, camera, flags)), "cache interaction failed");
			check(scripts[{side, 2}] == 2 && texts[{side, 2}] == 2,
				"script/text cache failed to distinguish or reuse side keys");
		}
	}
	check(events.cachedScriptCount() == 2 && events.cachedTextCount() == 2,
		"side entries missing");
	events.discardScriptCache(); events.discardTextCache();
	XeenCamera camera{{XeenSide::Darkside, 2}, 2, 2, XeenDirection::North};
	XeenGameFlags flags;
	events.runManualEvent(world, party, camera, flags);
	check(scripts[camera.mapId] == 3 && texts[camera.mapId] == 3,
		"script/text discard failed to reload");
}

void rejectionAndRollback() {
	XeenPartyState party;
	const XeenMapIdentity dark{XeenSide::Darkside, 1};
	XeenWorld world([](XeenMapIdentity id) { return map(id); });
	XeenCamera camera{dark, 1, 1, XeenDirection::North};
	XeenGameFlags flags;
	XeenEventSystem wrong([](XeenMapIdentity id) {
		return script(id.number, {record(1, 1, 0, 0x12)});
	});
	check(std::holds_alternative<XeenEventExecutionError>(
		wrong.runManualEvent(world, party, camera, flags)) && wrong.cachedScriptCount() == 0,
		"wrong-side script accepted/cached");
	XeenEventSystem rollback([](XeenMapIdentity id) {
		if (id.number == 1) return script(id, {record(1, 1, 0, 0x0c, {0,0,20,5}),
			record(1, 1, 1, 0x1f, {2,2,2})});
		return script(id, {record(2, 2, 0, 0x01, {0}), record(2, 2, 1, 0xff)});
	}, [](XeenMapIdentity id) { return XeenEventTextFile{id, "test", true, {"pending"}}; });
	auto pending = std::get<XeenEventExecutionSuspended>(rollback.runManualEvent(world, party, camera, flags));
	check(!flags.isSet(5) && camera.mapId == dark && pending.state.workingGameFlags.isSet(5),
		"pending transaction leaked");
	rollback.discardScriptCache(); rollback.discardTextCache(); world.discardMapCache();
	auto result = rollback.resumeManualEvent(pending.state, XeenPresentationResponse::Presented,
		world, party, camera, flags);
	const auto &error = std::get<XeenEventExecutionError>(result);
	check(camera.mapId == dark && !flags.isSet(5) &&
		error.logicalAddress.mapId == XeenMapIdentity{XeenSide::Darkside, 2} &&
		error.source->mapId == error.logicalAddress.mapId, "rollback/error side lost");
	XeenEventSystem wrongText([](XeenMapIdentity id) {
		return script(id, {record(1, 1, 0, 0x01, {0})});
	}, [](XeenMapIdentity id) { return XeenEventTextFile{id.number, "wrong", true, {"wrong side"}}; });
	check(std::holds_alternative<XeenEventExecutionError>(
		wrongText.runManualEvent(world, party, camera, flags)) && wrongText.cachedTextCount() == 0,
		"wrong-side text accepted/cached");
	XeenEventInterpreter interpreter;
	const auto direct = interpreter.begin(camera, party, flags, world,
		[](XeenMapIdentity id) { return script(id.number, {}); }, {});
	check(std::get<XeenEventExecutionError>(direct).kind == XeenEventExecutionErrorKind::ScriptMapMismatch,
		"direct interpreter accepted wrong-side script");
	const auto badText = interpreter.begin(camera, party, flags, world,
		[](XeenMapIdentity id) { return script(id, {record(1, 1, 0, 0x01, {0})}); },
		[](XeenMapIdentity id) { return XeenEventTextFile{id.number, "wrong", true, {"wrong"}}; });
	check(std::get<XeenEventExecutionError>(badText).kind == XeenEventExecutionErrorKind::TextMapMismatch,
		"direct interpreter accepted wrong-side text");
	const auto badTarget = interpreter.execute(camera, party, flags, world,
		[](XeenMapIdentity id) { return script(id, {record(1, 1, 0, 0x07, {2, 255, 1})}); });
	check(std::get<XeenEventExecutionError>(badTarget).requestedTarget->mapId ==
		XeenMapIdentity{XeenSide::Darkside, 2}, "requested target lost side");
	XeenEventSystem commit([](XeenMapIdentity id) {
		return script(id, {record(1, 1, 0, 0x0c, {0,0,20,5}), record(1, 1, 1, 0x07, {2,2,2})});
	});
	check(std::holds_alternative<XeenManualEventCompleted>(commit.runManualEvent(world, party, camera, flags)) &&
		camera.mapId == XeenMapIdentity{XeenSide::Darkside, 2} && flags.isSet(5),
		"TeleportAndExit/flag commit lost side");
	int reads = 0;
	auto reader = [&](const std::string &) -> std::optional<std::vector<std::uint8_t>> {
		++reads; return std::nullopt;
	};
	bool eventRejected = false, textRejected = false;
	try { XeenEventLoader(reader).load(dark); } catch (const std::invalid_argument &) { eventRejected = true; }
	try { XeenEventTextLoader(reader).load(dark); } catch (const std::invalid_argument &) { textRejected = true; }
	check(eventRejected && textRejected && reads == 0, "Clouds adapters read Darkside resources");
	std::ostringstream diagnostic; diagnostic << dark;
	check(diagnostic.str().find("Darkside") != std::string::npos, "diagnostic lost side");
}
} // namespace

int main() {
	try {
		worldLifetimeAndNavigation(); eventCachesCallsAndResume(); rejectionAndRollback();
		std::cout << "Session identity, cache lifetime, navigation and event regressions OK\n";
		return 0;
	} catch (const std::exception &error) { std::cerr << error.what() << '\n'; return 1; }
}
