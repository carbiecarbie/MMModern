#include "formats/xeen/XeenGameFlagsFormat.h"
#include "games/xeen/XeenGameFlags.h"

#include <algorithm>
#include <cstdint>
#include <iostream>
#include <stdexcept>
#include <string>
#include <vector>

using namespace mmodern;

namespace {

void check(bool value, const char *message) {
	if (!value)
		throw std::runtime_error(message);
}

template<class Function>
bool throwsOutOfRange(Function function) {
	try {
		function();
	} catch (const std::out_of_range &) {
		return true;
	}
	return false;
}

template<class Function>
std::string runtimeErrorMessage(Function function) {
	try {
		function();
	} catch (const std::runtime_error &error) {
		return error.what();
	}
	return {};
}

std::vector<std::uint8_t> emptyPartyBytes(std::size_t size =
		XeenGameFlagsFormat::kCloudsRequiredPartySize) {
	return std::vector<std::uint8_t>(size, 0);
}

void checkOnlyFlag(const std::vector<std::uint8_t> &bytes,
		std::size_t expectedIndex, const char *message) {
	const auto flags = XeenGameFlagsFormat::parseClouds(bytes);
	for (std::size_t index = 0; index < flags.size(); ++index)
		check(flags[index] == (index == expectedIndex), message);
}

void testLiteralPositionsDetectOffByOne() {
	std::vector<std::uint8_t> bytes(692, 0);
	bytes[659] = 0x01;
	checkOnlyFlag(bytes, 0, "literal byte 659 bit 0 must be flag 0 only");

	std::fill(bytes.begin(), bytes.end(), 0);
	bytes[659] = 0x80;
	checkOnlyFlag(bytes, 7, "literal byte 659 bit 7 must be flag 7 only");

	std::fill(bytes.begin(), bytes.end(), 0);
	bytes[660] = 0x01;
	checkOnlyFlag(bytes, 8, "literal byte 660 bit 0 must be flag 8 only");

	std::fill(bytes.begin(), bytes.end(), 0);
	bytes[690] = 0x80;
	checkOnlyFlag(bytes, 255, "literal byte 690 bit 7 must be flag 255 only");

	std::fill(bytes.begin(), bytes.end(), 0);
	bytes[658] = 0xff;
	bytes[691] = 0xff;
	const auto sentinels = XeenGameFlagsFormat::parseClouds(bytes);
	check(std::none_of(sentinels.begin(), sentinels.end(),
		[](bool value) { return value; }),
		"literal sentinel bytes 658 and 691 must be ignored");
}

void testBitOrderAndBoundaries() {
	auto bytes = emptyPartyBytes();
	bytes[659] = 0x81;
	bytes[660] = 0x01;
	bytes[690] = 0x80;
	const auto flags = XeenGameFlagsFormat::parseClouds(bytes);
	check(flags[0] && flags[7] && flags[8] && flags[255],
		"flags 0, 7, 8 and 255");
	check(!flags[1] && !flags[6] && !flags[9] && !flags[254],
		"least-significant-bit-first order");

	std::fill(bytes.begin() + XeenGameFlagsFormat::kCloudsFlagsOffset,
		bytes.begin() + XeenGameFlagsFormat::kCloudsRequiredPartySize, 0xff);
	const auto all = XeenGameFlagsFormat::parseClouds(bytes);
	check(std::all_of(all.begin(), all.end(), [](bool value) { return value; }),
		"all flags set");

	std::fill(bytes.begin(), bytes.end(), 0);
	const auto none = XeenGameFlagsFormat::parseClouds(bytes);
	check(std::none_of(none.begin(), none.end(), [](bool value) { return value; }),
		"all flags clear");
}

void testOnlyTheCloudsBlockIsRead() {
	std::vector<std::uint8_t> bytes(692, 0);
	bytes[658] = 0xff;
	bytes[691] = 0xff;
	const auto flags = XeenGameFlagsFormat::parseClouds(bytes);
	check(std::none_of(flags.begin(), flags.end(), [](bool value) { return value; }),
		"bytes adjacent to the Clouds block are ignored");

	bytes[662] = 0x52;
	const auto pattern = XeenGameFlagsFormat::parseClouds(bytes);
	check(pattern[25] && !pattern[24] && !pattern[26] && !pattern[27] &&
		pattern[28] && !pattern[29] && pattern[30] && !pattern[31],
		"mixed bit pattern");
}

void testSizesInputPurityAndLifetime() {
	for (const std::size_t size : {std::size_t{0}, std::size_t{659}, std::size_t{690}}) {
		const std::string message = runtimeErrorMessage([size] {
			XeenGameFlagsFormat::parseClouds(emptyPartyBytes(size));
		});
		check(message.find("maze.pty") != std::string::npos &&
			message.find(std::to_string(size)) != std::string::npos &&
			message.find("691") != std::string::npos,
			"truncation reports resource, actual size, and minimum 691");
	}

	auto exact = emptyPartyBytes(691);
	exact[659] = 1;
	const auto original = exact;
	const auto exactFlags = XeenGameFlagsFormat::parseClouds(exact);
	check(exactFlags[0] && exact == original, "exact size and input purity");

	auto oneExtra = emptyPartyBytes(692);
	oneExtra[690] = 0x80;
	check(XeenGameFlagsFormat::parseClouds(oneExtra)[255],
		"692-byte resource accepted");

	auto larger = emptyPartyBytes(812);
	larger[690] = 0x80;
	const XeenGameFlags owned(XeenGameFlagsFormat::parseClouds(larger));
	larger.clear();
	check(owned.isSet(255), "result owns flags after source destruction");
}

void testDomainOperations() {
	XeenGameFlags flags;
	check(!flags.isSet(0) && !flags.isSet(255), "default flags are clear");
	flags.set(0);
	flags.set(255);
	flags.set(0);
	check(flags.isSet(0) && flags.isSet(255) && !flags.isSet(1),
		"set is isolated and idempotent");
	flags.clear(0);
	flags.clear(0);
	check(!flags.isSet(0) && flags.isSet(255), "clear is isolated and idempotent");

	XeenGameFlags copy = flags;
	copy.clear(255);
	copy.set(1);
	check(flags.isSet(255) && !flags.isSet(1) &&
		!copy.isSet(255) && copy.isSet(1), "copies are independent");
}

void testInvalidIndicesPreserveState() {
	XeenGameFlags flags;
	flags.set(25);
	const auto original = flags.values();
	for (const int index : {-1, 256}) {
		check(throwsOutOfRange([&] { static_cast<void>(flags.isSet(index)); }),
			"invalid query index");
		check(flags.values() == original, "query error preserves state");
		check(throwsOutOfRange([&] { flags.set(index); }), "invalid set index");
		check(flags.values() == original, "set error preserves state");
		check(throwsOutOfRange([&] { flags.clear(index); }), "invalid clear index");
		check(flags.values() == original, "clear error preserves state");
	}
}

} // namespace

int main() {
	try {
		testLiteralPositionsDetectOffByOne();
		testBitOrderAndBoundaries();
		testOnlyTheCloudsBlockIsRead();
		testSizesInputPurityAndLifetime();
		testDomainOperations();
		testInvalidIndicesPreserveState();
		std::cout << "Clouds game flags format and state operations OK\n";
		return 0;
	} catch (const std::exception &error) {
		std::cerr << error.what() << '\n';
		return 1;
	}
}
