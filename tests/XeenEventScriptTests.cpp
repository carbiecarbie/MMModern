#include "games/xeen/XeenEventDiagnostics.h"
#include "games/xeen/XeenEventScript.h"
#include "games/xeen/XeenEventTrigger.h"

#include <iostream>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

using namespace mmodern;

namespace {

void check(bool value, const char *message) {
	if (!value)
		throw std::runtime_error(message);
}

XeenEventRecord record(std::size_t offset, std::uint8_t x, std::uint8_t y,
		std::uint8_t direction, std::uint8_t line, std::uint8_t opcode = 0,
		std::vector<std::uint8_t> parameters = {}) {
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

XeenEventScript scriptWith(std::vector<XeenEventRecord> records,
		bool present = true) {
	XeenEventFile file;
	file.mapId = 31;
	file.resourceName = "maze0031.evt";
	file.resourcePresent = present;
	file.records = std::move(records);
	return XeenEventScript(std::move(file));
}

template<class Function>
bool rejects(Function function) {
	try {
		function();
	} catch (const std::invalid_argument &) {
		return true;
	}
	return false;
}

void testEmptyAndAbsentScripts() {
	const auto empty = scriptWith({});
	check(empty.file().resourcePresent && empty.records().empty(), "empty script");
	check(empty.findInstruction(1, 2, XeenDirection::North, 0) == nullptr,
		"empty lookup");
	const auto absent = scriptWith({}, false);
	check(!absent.file().resourcePresent &&
		absent.findInstruction(1, 2, XeenDirection::North, 0) == nullptr,
		"absent script");
}

void testFirstMatchWins() {
	const auto allFirst = scriptWith({
		record(10, 2, 9, kXeenEventDirectionAll, 0, 0x07),
		record(20, 2, 9, 0, 0, 0x08)
	});
	check(allFirst.findInstruction(2, 9, XeenDirection::North, 0)->fileOffset == 10,
		"All before exact must win");

	const auto exactFirst = scriptWith({
		record(30, 2, 9, 0, 0, 0x08),
		record(40, 2, 9, kXeenEventDirectionAll, 0, 0x07)
	});
	check(exactFirst.findInstruction(2, 9, XeenDirection::North, 0)->fileOffset == 30,
		"exact before All must win");
	check(exactFirst.findInstruction(2, 9, XeenDirection::East, 0)->fileOffset == 40,
		"All matches another physical direction");
	check(exactFirst.findInstruction(3, 9, XeenDirection::North, 0) == nullptr,
		"wrong X");
	check(exactFirst.findInstruction(2, 8, XeenDirection::North, 0) == nullptr,
		"wrong Y");
	check(exactFirst.findInstruction(2, 9, XeenDirection::North, 1) == nullptr,
		"wrong line");

	const auto directions = scriptWith({
		record(1, 4, 5, 1, 0),
		record(2, 4, 5, 9, 0),
		record(3, 4, 5, 2, 1)
	});
	check(directions.findInstruction(4, 5, XeenDirection::North, 0) == nullptr,
		"incompatible and unknown directions");
	check(directions.findInstruction(4, 5, XeenDirection::East, 0)->fileOffset == 1,
		"exact East direction");
	check(directions.findInstruction(4, 5, XeenDirection::South, 1)->fileOffset == 3,
		"second line and direction");
	check(rejects([&] {
		directions.findInstruction(4, 5, static_cast<XeenDirection>(4), 0);
	}), "invalid query direction");
}

void testLogicalAddressesAndLines() {
	const auto script = scriptWith({
		record(1, 75, 76, 0, 0),
		record(2, 102, 102, 1, 255),
		record(3, 255, 255, 2, 0)
	});
	check(script.findInstruction(75, 76, XeenDirection::North, 0)->fileOffset == 1,
		"logical address 75/76");
	check(script.findInstruction(102, 102, XeenDirection::East, 255)->fileOffset == 2,
		"logical address and line 255");
	check(script.findInstruction(255, 255, XeenDirection::South, 0)->fileOffset == 3,
		"logical address 255/255");
}

void testDuplicateDiagnostics() {
	const auto script = scriptWith({
		record(10, 1, 2, 0, 3),
		record(20, 1, 2, 0, 3),
		record(30, 1, 2, 0, 3),
		record(40, 1, 2, kXeenEventDirectionAll, 3)
	});
	const auto duplicates = script.duplicateKeys();
	check(duplicates.size() == 2, "three equal keys produce two duplicates");
	check(duplicates[0].firstOffset == 10 && duplicates[0].duplicateOffset == 20 &&
		duplicates[1].firstOffset == 10 && duplicates[1].duplicateOffset == 30,
		"duplicate offsets reference first record");
	check(duplicates[0].x == 1 && duplicates[0].y == 2 &&
		duplicates[0].direction == 0 && duplicates[0].line == 3,
		"duplicate key fields");
	check(script.findInstruction(1, 2, XeenDirection::North, 3)->fileOffset == 10,
		"duplicate lookup keeps first");
	const std::string output = XeenEventDiagnostics::format(script);
	check(output.find("Duplicatas exatas: 2") != std::string::npos &&
		output.find("first=@10 duplicate=@20") != std::string::npos,
		"duplicate text diagnostic");
}

void testAutomaticTrigger() {
	XeenMapGeometry indoor;
	indoor.cells[3 * XeenMapGeometry::kWidth + 7].rawAttributes = 0x10;
	indoor.cells[2 * XeenMapGeometry::kWidth + 6].rawAttributes = 0xe8;
	indoor.cells[1 * XeenMapGeometry::kWidth + 5].rawWord = 0x0010;
	indoor.cells[3 * XeenMapGeometry::kWidth + 7].seen = true;
	const auto before = indoor.cells[3 * XeenMapGeometry::kWidth + 7];
	check(hasAutomaticTrigger(indoor, 7, 3), "indoor automatic flag");
	check(!hasAutomaticTrigger(indoor, 6, 2), "other attribute bits");
	check(!hasAutomaticTrigger(indoor, 5, 1), "rawWord does not trigger");
	check(!hasAutomaticTrigger(indoor, -1, 3) &&
		!hasAutomaticTrigger(indoor, 7, -1) &&
		!hasAutomaticTrigger(indoor, 16, 3) &&
		!hasAutomaticTrigger(indoor, 7, 16) &&
		!hasAutomaticTrigger(indoor, 99, 99), "out of bounds is safe");
	const auto &after = indoor.cells[3 * XeenMapGeometry::kWidth + 7];
	check(after.rawAttributes == before.rawAttributes && after.seen == before.seen &&
		after.stepped == before.stepped, "trigger query is non-mutating");

	XeenMapGeometry outdoor;
	outdoor.flags2 = 0x8000;
	outdoor.cells[4 * XeenMapGeometry::kWidth + 9].rawAttributes = 0x17;
	check(outdoor.isOutdoors() && hasAutomaticTrigger(outdoor, 9, 4),
		"outdoor automatic flag and orientation");

	const auto noInstruction = scriptWith({});
	check(hasAutomaticTrigger(indoor, 7, 3) &&
		noInstruction.findInstruction(7, 3, XeenDirection::North, 0) == nullptr,
		"trigger without instruction");
	const auto instruction = scriptWith({record(1, 1, 1, 0, 0)});
	check(!hasAutomaticTrigger(indoor, 1, 1) &&
		instruction.findInstruction(1, 1, XeenDirection::North, 0) != nullptr,
		"instruction without trigger");
}

void testFilteredDiagnostics() {
	auto script = scriptWith({
		record(10, 2, 9, 0, 0, 0x08, {0xaa}),
		record(20, 2, 9, kXeenEventDirectionAll, 1, 0x09, {0xbb}),
		record(30, 2, 9, 1, 2, 0x12),
		record(40, 3, 9, kXeenEventDirectionAll, 0, 0x07)
	});
	const auto originalParameters = script.records()[0].parameters;

	XeenEventDiagnosticFilter any;
	any.x = 2;
	any.y = 9;
	any.automatic = true;
	const std::string anyText = XeenEventDiagnostics::format(script, any);
	check(anyText.find("Registros exibidos: 3") != std::string::npos &&
		anyText.find("@10 ") != std::string::npos &&
		anyText.find("@20 ") != std::string::npos &&
		anyText.find("@30 ") != std::string::npos &&
		anyText.find("@40 ") == std::string::npos,
		"unqualified direction filter preserves position order");
	check(anyText.find("Automatic: yes") != std::string::npos &&
		anyText.find("Selected line 0: n/a") != std::string::npos,
		"automatic and no direction selection");

	XeenEventDiagnosticFilter north = any;
	north.directionFilter = XeenEventDirectionFilter::Physical;
	north.direction = XeenDirection::North;
	const std::string northText = XeenEventDiagnostics::format(script, north);
	check(northText.find("Registros exibidos: 2") != std::string::npos &&
		northText.find("@10 ") != std::string::npos &&
		northText.find("@20 ") != std::string::npos &&
		northText.find("@30 ") == std::string::npos,
		"physical filter includes exact and All");
	check(northText.find("Selected line 0:\n@10") != std::string::npos,
		"physical filter selects line zero");

	XeenEventDiagnosticFilter all = any;
	all.directionFilter = XeenEventDirectionFilter::AllOnly;
	const std::string allText = XeenEventDiagnostics::format(script, all);
	check(allText.find("Registros exibidos: 1") != std::string::npos &&
		allText.find("@20 ") != std::string::npos &&
		allText.find("@10 ") == std::string::npos,
		"All filter means records with byte four only");
	check(allText.find("Selected line 0: n/a") != std::string::npos,
		"All is not a physical query direction");

	XeenEventDiagnosticFilter logical;
	logical.x = 75;
	logical.y = 76;
	const std::string logicalText = XeenEventDiagnostics::format(script, logical);
	check(logicalText.find("Automatic: n/a") != std::string::npos &&
		logicalText.find("Logical address: yes") != std::string::npos,
		"logical address diagnostic");
	check(script.records()[0].parameters == originalParameters,
		"filter diagnostics are non-mutating");
}

} // namespace

int main() {
	try {
		testEmptyAndAbsentScripts();
		testFirstMatchWins();
		testLogicalAddressesAndLines();
		testDuplicateDiagnostics();
		testAutomaticTrigger();
		testFilteredDiagnostics();
		std::cout << "EVT script lookup, triggers, duplicates, and filters OK\n";
		return 0;
	} catch (const std::exception &error) {
		std::cerr << error.what() << '\n';
		return 1;
	}
}
