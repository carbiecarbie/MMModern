#include "formats/xeen/XeenEventFormat.h"

#include <iostream>
#include <stdexcept>
#include <string>

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
	} catch (const std::runtime_error &error) {
		return error.what();
	}
	throw std::runtime_error("malformed EVT input was accepted");
}

void testEmptyAndMinimumRecord() {
	check(XeenEventFormat::parse({}).empty(), "empty EVT");

	const auto records = XeenEventFormat::parse({5, 1, 2, 4, 0, 0x12});
	check(records.size() == 1, "minimum record count");
	const auto &record = records[0];
	check(record.fileOffset == 0 && record.lengthField == 5 &&
		record.totalSerializedSize() == 6, "minimum record sizes");
	check(record.x == 1 && record.y == 2 && record.direction == 4 &&
		record.line == 0 && record.opcode == 0x12, "minimum record fields");
	check(record.parameters.empty(), "minimum record parameters");
}

void testVariableRecordsAndOffsets() {
	const Bytes bytes = {
		7, 1, 2, 4, 0, 0xfa, 0x35, 0x80,
		5, 75, 76, 9, 0xff, 0xfb,
		6, 102, 102, 3, 1, 0x07, 0xcc
	};
	const auto records = XeenEventFormat::parse(bytes);
	check(records.size() == 3, "variable record count");
	check(records[0].fileOffset == 0 && records[0].lengthField == 7 &&
		records[0].parameters == Bytes{0x35, 0x80}, "first variable record");
	check(records[1].fileOffset == 8 && records[1].lengthField == 5 &&
		records[1].x == 75 && records[1].y == 76 &&
		records[1].direction == 9 && records[1].line == 0xff &&
		records[1].opcode == 0xfb && records[1].parameters.empty(),
		"logical coordinates and unknown values");
	check(records[2].fileOffset == 14 && records[2].lengthField == 6 &&
		records[2].x == 102 && records[2].y == 102 &&
		records[2].parameters == Bytes{0xcc}, "third record offset and parameters");
}

void testDirectionBytesRemainOpaque() {
	for (std::uint8_t direction = 0; direction <= 4; ++direction) {
		const auto records = XeenEventFormat::parse(
			{5, 0, 0, direction, 0, 0});
		check(records.size() == 1 && records[0].direction == direction,
			"direction byte 0..4");
	}

	const auto unknown = XeenEventFormat::parse({5, 0, 0, 0xa5, 0, 0});
	check(unknown[0].direction == 0xa5, "unknown direction byte");
}

void testTruncation() {
	const std::string shortLength = rejectionMessage([] {
		XeenEventFormat::parse({4, 0, 0, 0, 0});
	});
	check(shortLength.find("offset 0") != std::string::npos,
		"short length error offset");

	const std::string missingFields = rejectionMessage([] {
		XeenEventFormat::parse({5, 0, 0});
	});
	check(missingFields.find("offset 0") != std::string::npos,
		"mandatory fields truncation offset");

	const std::string missingParameters = rejectionMessage([] {
		XeenEventFormat::parse({7, 0, 0, 0, 0, 0, 1});
	});
	check(missingParameters.find("offset 0") != std::string::npos,
		"parameter truncation offset");

	const std::string secondRecord = rejectionMessage([] {
		XeenEventFormat::parse({5, 0, 0, 0, 0, 0, 4, 0, 0, 0, 0});
	});
	check(secondRecord.find("offset 6") != std::string::npos,
		"second record error offset");
}

} // namespace

int main() {
	try {
		testEmptyAndMinimumRecord();
		testVariableRecordsAndOffsets();
		testDirectionBytesRemainOpaque();
		testTruncation();
		std::cout << "EVT envelope, offsets, opaque values and bounds OK\n";
		return 0;
	} catch (const std::exception &error) {
		std::cerr << error.what() << '\n';
		return 1;
	}
}
