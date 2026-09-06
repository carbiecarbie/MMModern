#include "games/xeen/XeenEventSystem.h"
#include "games/xeen/XeenEventTrigger.h"
#include "games/xeen/XeenWorld.h"

#include <cstdint>
#include <iostream>
#include <map>
#include <stdexcept>
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
		std::vector<XeenEventRecord> records = {}) {
	XeenEventFile file;
	file.mapId = mapId;
	file.resourceName = "maze" + std::to_string(mapId) + ".evt";
	file.resourcePresent = true;
	file.records = std::move(records);
	return XeenEventScript(std::move(file));
}

XeenMap map(std::uint16_t id, bool outdoors = true) {
	XeenMap result;
	result.geometry.id = id;
	result.geometry.flags2 = outdoors ? 0x8000 : 0;
	return result;
}

std::vector<std::uint8_t> setFlag(std::uint8_t flag) {
	return {0, 0, 20, flag};
}

class Fixture {
public:
	Fixture() :
		world([this](std::uint16_t id) {
			++mapLoads[id];
			return maps.at(id);
		}),
		system([this](std::uint16_t id) {
			++scriptLoads[id];
			return scripts.at(id);
		}, [this](std::uint16_t id) {
			const auto found = texts.find(id);
			return found == texts.end() ? XeenEventTextFile{id,
				XeenEventTextLoader::resourceNameForMap(id), false, {}} : found->second;
		}) {}

	std::map<std::uint16_t, XeenMap> maps;
	std::map<std::uint16_t, XeenEventScript> scripts;
	std::map<std::uint16_t, XeenEventTextFile> texts;
	std::map<std::uint16_t, int> mapLoads;
	std::map<std::uint16_t, int> scriptLoads;
	XeenWorld world;
	XeenEventSystem system;
};

XeenManualEventCompleted completed(const XeenManualEventResult &result) {
	const auto *value = std::get_if<XeenManualEventCompleted>(&result);
	check(value != nullptr, "expected completed manual event");
	return *value;
}

void testManualGateAndCurrentCell() {
	Fixture fixture;
	fixture.maps.emplace(1, map(1));
	fixture.scripts.emplace(1, script(1, {
		record(4, 5, 0, 0x0c, setFlag(7),
			static_cast<std::uint8_t>(XeenDirection::West)),
		record(4, 5, 1, 0x12),
		record(3, 5, 0, 0x0c, setFlag(8))
	}));
	XeenCamera camera{1, 4, 5, XeenDirection::West};
	XeenGameFlags flags;

	check(std::holds_alternative<XeenAutomaticEventNoTrigger>(
		fixture.system.runAutomaticEvent(fixture.world, {}, camera, flags)),
		"automatic event must remain gated by 0x10");
	const auto result = completed(fixture.system.runManualEvent(
		fixture.world, {}, camera, flags));
	check(result.instructionCount == 2 && result.flagsChanged && flags.isSet(7),
		"manual event without 0x10 executes and commits");
	check(!flags.isSet(8) && camera.x == 4 && camera.y == 5,
		"manual lookup uses current cell, not the forward cell");

	Fixture flaggedFixture;
	auto flaggedMap = map(1);
	flaggedMap.geometry.cells[5 * 16 + 4].rawAttributes |= kXeenAutomaticEventFlag;
	flaggedFixture.maps.emplace(1, flaggedMap);
	flaggedFixture.scripts.emplace(1, script(1, {
		record(4, 5, 0, 0x0c, setFlag(7),
			static_cast<std::uint8_t>(XeenDirection::West)),
		record(4, 5, 1, 0x12)
	}));
	XeenGameFlags flagged;
	completed(flaggedFixture.system.runManualEvent(
		flaggedFixture.world, {}, camera, flagged));
	check(flagged.isSet(7), "0x10 does not block manual dispatch");
}

void testDirectionOrderAndNoEvent() {
	Fixture fixture;
	fixture.maps.emplace(1, map(1));
	fixture.scripts.emplace(1, script(1, {
		record(2, 3, 0, 0x07, {2, 6, 6}, kXeenEventDirectionAll, 10),
		record(2, 3, 0, 0x07, {3, 7, 7},
			static_cast<std::uint8_t>(XeenDirection::North), 20),
		record(9, 9, 1, 0x12)
	}));
	fixture.maps.emplace(2, map(2));
	fixture.maps.emplace(3, map(3));
	XeenCamera camera{1, 2, 3, XeenDirection::North};
	XeenGameFlags flags;
	completed(fixture.system.runManualEvent(fixture.world, {}, camera, flags));
	check(camera.mapId == 2 && camera.x == 6 && camera.y == 6,
		"first matching all-directions record wins over later exact direction");

	XeenCamera wrongDirection{1, 2, 3, XeenDirection::East};
	completed(fixture.system.runManualEvent(
		fixture.world, {}, wrongDirection, flags));
	check(wrongDirection.mapId == 2, "all-directions record matches another facing");

	XeenCamera noLineZero{1, 9, 9, XeenDirection::North};
	const auto none = fixture.system.runManualEvent(
		fixture.world, {}, noLineZero, flags);
	check(std::holds_alternative<XeenManualEventNoEvent>(none) &&
		noLineZero.mapId == 1 && noLineZero.x == 9 && noLineZero.y == 9,
		"later lines without line zero return explicit no-event");
}

void testErrorsRollbackAndDiagnostics() {
	Fixture fixture;
	fixture.maps.emplace(1, map(1));
	fixture.texts.emplace(1, XeenEventTextFile{1, "aaze0001.txt", true,
		{"zero", "one", "two", "shown"}});
	fixture.scripts.emplace(1, script(1, {
		record(4, 5, 0, 0x0c, setFlag(9), kXeenEventDirectionAll, 40),
		record(4, 5, 1, 0x04, {3}, kXeenEventDirectionAll, 55),
		record(4, 5, 2, 0x06, {}, kXeenEventDirectionAll, 65)
	}));
	XeenCamera camera{1, 4, 5, XeenDirection::South};
	XeenGameFlags flags;
	const auto result = fixture.system.runManualEvent(
		fixture.world, {}, camera, flags);
	const auto *pending = std::get_if<XeenEventExecutionSuspended>(&result);
	check(pending && pending->request.text == "shown" && !flags.isSet(9),
		"manual text request suspends without committing flags");
	const auto resumed = fixture.system.resumeManualEvent(pending->state,
		XeenPresentationResponse::Presented, fixture.world, {}, camera, flags);
	const auto *error = std::get_if<XeenEventExecutionError>(&resumed);
	check(error && error->kind == XeenEventExecutionErrorKind::UnsupportedOpcode,
		"unsupported opcode after presentation remains an execution error");
	check(error->source && error->source->fileOffset == 65 &&
		error->source->opcode == 0x06,
		"manual error preserves source diagnostics");
	check(!flags.isSet(9) && camera.mapId == 1 && camera.x == 4 && camera.y == 5,
		"manual execution failure rolls back camera and flags");
}

void testUnsupportedSpecialInteractionPrecedence() {
	for (const std::uint8_t wall : {std::uint8_t{1}, std::uint8_t{6},
			std::uint8_t{9}}) {
		XeenMap specialMap = map(1, false);
		auto &specialCell = specialMap.geometry.cells[5 * 16 + 4];
		std::get<XeenIndoorWalls>(specialCell.geometry).walls[
			static_cast<std::size_t>(XeenDirection::West)] = wall;
		const auto detected = unsupportedManualSpecialInteraction(
			specialMap.geometry, 4, 5, XeenDirection::West);
		check(detected && *detected == wall,
			"Clouds grate wall value should be recognized");
	}
	XeenMap ordinaryIndoor = map(1, false);
	std::get<XeenIndoorWalls>(ordinaryIndoor.geometry.cells[5 * 16 + 4].geometry)
		.walls[static_cast<std::size_t>(XeenDirection::West)] = 2;
	check(!unsupportedManualSpecialInteraction(ordinaryIndoor.geometry, 4, 5,
		XeenDirection::West), "ordinary indoor wall is not a special interaction");
	XeenMap outdoor = map(1, true);
	check(!unsupportedManualSpecialInteraction(outdoor.geometry, 4, 5,
		XeenDirection::West), "outdoor terrain is not classified as a grate");

	Fixture fixture;
	fixture.maps.emplace(1, map(1, false));
	fixture.scripts.emplace(1, script(1, {
		record(4, 5, 0, 0x0c, setFlag(10)), record(4, 5, 1, 0x12)
	}));
	auto &cell = fixture.maps.at(1).geometry.cells[5 * 16 + 4];
	std::get<XeenIndoorWalls>(cell.geometry).walls[
		static_cast<std::size_t>(XeenDirection::West)] = 1;
	XeenCamera camera{1, 4, 5, XeenDirection::West};
	XeenGameFlags flags;
	const auto special = fixture.system.runManualEvent(
		fixture.world, {}, camera, flags);
	const auto *unsupported =
		std::get_if<XeenManualSpecialInteractionUnsupported>(&special);
	check(unsupported && unsupported->wallValue == 1,
		"recognized grate interaction is explicit");
	check(fixture.scriptLoads.empty() && !flags.isSet(10),
		"special interaction takes precedence over ordinary script dispatch");

	Fixture lockedFixture;
	lockedFixture.maps.emplace(1, map(1, false));
	lockedFixture.scripts.emplace(1, script(1, {
		record(4, 5, 0, 0x0c, setFlag(10)), record(4, 5, 1, 0x12)
	}));
	auto &lockedCell = lockedFixture.maps.at(1).geometry.cells[5 * 16 + 4];
	std::get<XeenIndoorWalls>(lockedCell.geometry).walls[
		static_cast<std::size_t>(XeenDirection::West)] = 13;
	XeenGameFlags lockedFlags;
	const auto locked = completed(lockedFixture.system.runManualEvent(
		lockedFixture.world, {}, camera, lockedFlags));
	check(locked.instructionCount == 2 && lockedFlags.isSet(10),
		"locked wall 13 falls through to ordinary event lookup");

	Fixture unlockedFixture;
	unlockedFixture.maps.emplace(1, map(1, false));
	unlockedFixture.scripts.emplace(1, script(1, {}));
	auto &unlockedCell = unlockedFixture.maps.at(1).geometry.cells[5 * 16 + 4];
	std::get<XeenIndoorWalls>(unlockedCell.geometry).walls[
		static_cast<std::size_t>(XeenDirection::West)] = 13;
	unlockedCell.rawAttributes |= kXeenGrateUnlockedFlag;
	check(std::holds_alternative<XeenManualSpecialInteractionUnsupported>(
		unlockedFixture.system.runManualEvent(unlockedFixture.world, {}, camera,
			lockedFlags)),
		"unlocked wall 13 is a recognized special interaction");
}

} // namespace

int main() {
	try {
		testManualGateAndCurrentCell();
		testDirectionOrderAndNoEvent();
		testErrorsRollbackAndDiagnostics();
		testUnsupportedSpecialInteractionPrecedence();
		std::cout << "Manual event dispatch and special-interaction boundaries OK\n";
		return 0;
	} catch (const std::exception &error) {
		std::cerr << error.what() << '\n';
		return 1;
	}
}
