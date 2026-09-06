#include "games/xeen/XeenEventDiagnostics.h"
#include "games/xeen/XeenEventLoader.h"

#include <array>
#include <iostream>
#include <optional>
#include <stdexcept>
#include <string>
#include <vector>

using namespace mmodern;
using Bytes = std::vector<std::uint8_t>;

namespace {

void check(bool value, const char *message) {
	if (!value)
		throw std::runtime_error(message);
}

template<class Function>
std::string rejectionMessage(Function function) {
	try {
		function();
	} catch (const std::exception &error) {
		return error.what();
	}
	throw std::runtime_error("expected rejection did not occur");
}

void testResourceNames() {
	check(XeenEventLoader::resourceNameForMap(1) == "maze0001.evt", "map 1 name");
	check(XeenEventLoader::resourceNameForMap(31) == "maze0031.evt", "map 31 name");
	check(XeenEventLoader::resourceNameForMap(42) == "maze0042.evt", "map 42 name");
}

void testPresenceAndParsing() {
	std::string requestedName;
	const XeenEventLoader present([&requestedName](const std::string &name) {
		requestedName = name;
		return std::optional<Bytes>{Bytes{
			7, 75, 76, 9, 0xff, 0xff, 0x0a, 0x00,
			5, 1, 2, 4, 0, 0x07
		}};
	});
	const XeenEventFile file = present.load(31);
	check(requestedName == "maze0031.evt", "reader resource name");
	check(file.mapId == 31 && file.resourceName == requestedName && file.resourcePresent,
		"event file identity");
	check(file.records.size() == 2, "record count");
	check(file.records[0].fileOffset == 0 && file.records[1].fileOffset == 8,
		"physical order and offsets");
	check(file.records[0].x == 75 && file.records[0].y == 76 &&
		file.records[0].direction == 9 && file.records[0].opcode == 0xff,
		"opaque coordinates, direction, and opcode");
	check(file.records[0].parameters == Bytes{0x0a, 0x00}, "raw parameters");
	check(file.records[1].parameters.empty(), "record without parameters");

	const XeenEventFile absent = XeenEventLoader([](const std::string &)
			-> std::optional<Bytes> { return std::nullopt; }).load(42);
	check(absent.mapId == 42 && absent.resourceName == "maze0042.evt" &&
		!absent.resourcePresent && absent.records.empty(), "absent resource");

	const XeenEventFile empty = XeenEventLoader([](const std::string &) {
		return std::optional<Bytes>{Bytes{}};
	}).load(1);
	check(empty.resourcePresent && empty.records.empty(), "present empty resource");
}

void testErrorsRemainDistinct() {
	const std::string malformed = rejectionMessage([] {
		XeenEventLoader([](const std::string &) {
			return std::optional<Bytes>{Bytes{4, 0, 0, 0, 0}};
		}).load(31);
	});
	check(malformed.find("maze0031.evt") != std::string::npos &&
		malformed.find("offset 0") != std::string::npos, "malformed context");

	const std::string readFailure = rejectionMessage([] {
		XeenEventLoader([](const std::string &) -> std::optional<Bytes> {
			throw std::runtime_error("synthetic read failure");
		}).load(31);
	});
	check(readFailure == "synthetic read failure", "read failure propagation");
}

void testOpcodeCatalog() {
	static constexpr std::array<const char *, 0x3d> expected{{
		"None", "Display0x01", "DoorTextSml", "DoorTextLrg", "SignText",
		"NPC", "PlayFX", "TeleportAndExit", "If1", "If2", "If3", "MoveObj",
		"TakeOrGive", "NoAction", "Remove", "SetChar", "Spawn", "DoTownEvent",
		"Exit", "AfterMap", "GiveMulti", "ConfirmWord", "Damage", "JumpRnd",
		"AfterEvent", "CallEvent", "Return", "SetVar", "TakeOrGive_2",
		"TakeOrGive_3", "CutsceneEndClouds", "TeleportAndContinue", "WhoWill",
		"RndDamage", "MoveWallObj", "AlterCellFlag", "AlterHed", "DisplayStat",
		"TakeOrGive_4", "SeatTextSml", "PlayEventVoc", "DisplayBottom", "IfMapFlag",
		"SelectRandomChar", "GiveEnchanted", "ItemType", "MakeNothingHere",
		"NoAction_2", "ChooseNumeric", "DisplayBottomTwoLines", "DisplayLarge",
		"ExchObj", "FallToMap", "DisplayMain", "Goto", "ConfirmWord_2",
		"GotoRandom", "CutsceneEndDarkside", "CutsceneEdWorld", "FlipWorld", "PlayCD"
	}};
	for (std::size_t i = 0; i < expected.size(); ++i)
		check(XeenEventDiagnostics::opcodeName(static_cast<std::uint8_t>(i)) == expected[i],
			"known opcode name");
	check(XeenEventDiagnostics::opcodeName(0x00) == "None", "opcode 00");
	check(XeenEventDiagnostics::opcodeName(0x07) == "TeleportAndExit", "opcode 07");
	check(XeenEventDiagnostics::opcodeName(0x3c) == "PlayCD", "opcode 3c");
	check(XeenEventDiagnostics::opcodeName(0x3d) == "Unknown(0x3D)", "opcode 3d");
	check(XeenEventDiagnostics::opcodeName(0xff) == "Unknown(0xFF)", "opcode ff");
}

void testDirectionsAndDiagnostics() {
	const char *const expected[] = {"North", "East", "South", "West", "All"};
	for (std::uint8_t direction = 0; direction < 5; ++direction)
		check(XeenEventDiagnostics::directionName(direction) == expected[direction],
			"known direction name");
	check(XeenEventDiagnostics::directionName(9) == "Unknown(9)",
		"unknown direction name");

	XeenEventFile file;
	file.mapId = 31;
	file.resourceName = "maze0031.evt";
	file.resourcePresent = true;
	file.records = XeenEventFormat::parse({
		7, 75, 76, 9, 0xff, 0xff, 0x0a, 0x00,
		5, 2, 9, 4, 0, 0x07
	});
	const std::size_t originalCount = file.records.size();
	const Bytes originalParameters = file.records[0].parameters;
	const std::string output = XeenEventDiagnostics::format(file);
	check(output.find("Mapa: 31") != std::string::npos &&
		output.find("Recurso: maze0031.evt") != std::string::npos &&
		output.find("Presente: sim") != std::string::npos &&
		output.find("Registros: 2") != std::string::npos, "diagnostic header");
	check(output.find("@0 length=7 X=75 Y=76 dir=Unknown(9) line=255") !=
		std::string::npos, "opaque diagnostic fields");
	check(output.find("opcode=0xFF Unknown(0xFF)") != std::string::npos &&
		output.find("params=0A 00") != std::string::npos, "hex diagnostics");
	check(output.find("@8 length=5 X=2 Y=9 dir=All(4) line=0") !=
		std::string::npos && output.find("params=[]") != std::string::npos,
		"known direction and empty parameters");
	check(file.records.size() == originalCount &&
		file.records[0].parameters == originalParameters, "diagnostic is non-mutating");
}

} // namespace

int main() {
	try {
		testResourceNames();
		testPresenceAndParsing();
		testErrorsRemainDistinct();
		testOpcodeCatalog();
		testDirectionsAndDiagnostics();
		std::cout << "EVT loader, catalog, and diagnostics OK\n";
		return 0;
	} catch (const std::exception &error) {
		std::cerr << error.what() << '\n';
		return 1;
	}
}
