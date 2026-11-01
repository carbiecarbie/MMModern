#include "formats/xeen/XeenEventTextFormat.h"
#include "games/xeen/XeenEventTextLoader.h"

#include <cstdint>
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

void testFormatPreservesRawStrings() {
	const auto strings = XeenEventTextFormat::parse({
		'A', 0, 0, 'B', '\n', 0x03, 'c', 0, 'C'
	});
	check(strings.size() == 4, "string count including empty and unterminated");
	check(strings[0] == "A", "first string");
	check(strings[1].empty(), "empty entry is preserved");
	check(strings[2] == std::string("B\n\x03" "c", 4), "control bytes preserved");
	check(strings[3] == "C", "unterminated final string");
	check(XeenEventTextFormat::parse({}).empty(), "empty resource has no entries");
	check(XeenEventTextFormat::parse({0}).size() == 1, "single empty entry");
}

void testNaming() {
	check(XeenEventTextLoader::resourceNameForMap(1) == "aaze0001.txt", "map 1 name");
	check(XeenEventTextLoader::resourceNameForMap(31) == "aaze0031.txt", "map 31 name");
	check(XeenEventTextLoader::resourceNameForMap(99) == "aaze0099.txt", "map 99 name");
	check(XeenEventTextLoader::resourceNameForMap(100) == "aazex100.txt", "map 100 name");
	check(XeenEventTextLoader::resourceNameForMap(999) == "aazex999.txt", "map 999 name");
}

void testLoadingAndLookup() {
	std::vector<std::string> requested;
	const XeenEventTextLoader loader([&requested](const std::string &name) {
		requested.push_back(name);
		return std::optional<Bytes>{Bytes{'o', 'n', 'e', 0, 0, 't', 'w', 'o', 0}};
	});
	const XeenEventTextFile text = loader.load(31);
	// The map's text, then DisplayLarge's aaze2 lines from the same archive.
	check(requested == std::vector<std::string>{"aaze0031.txt", "aaze2031.txt"}, "requested correct resources");
	check(text.mapId == 31 && text.resourcePresent &&
		text.resourceName == requested[0], "identity and presence");
	check(text.largeResourcePresent && text.largeResourceName == requested[1] &&
		text.largeStrings == text.strings && text.largeStringAt(3) == nullptr, "large text lines");
	check(XeenEventTextLoader::largeResourceNameForMap(121) == "aaze2121.txt", "map 121 large text name");
	check(text.strings.size() == 3 && *text.stringAt(0) == "one" &&
		text.stringAt(1)->empty() && *text.stringAt(2) == "two", "zero based lookup");
	check(text.stringAt(3) == nullptr, "invalid index distinct from empty string");

	const XeenEventTextFile missing = XeenEventTextLoader([](const std::string &)
			-> std::optional<Bytes> { return std::nullopt; }).load(42);
	check(!missing.resourcePresent && missing.strings.empty() && missing.stringAt(0) == nullptr &&
		!missing.largeResourcePresent && missing.largeStringAt(0) == nullptr, "missing resource");
	const XeenEventTextFile empty = XeenEventTextLoader([](const std::string &) {
		return std::optional<Bytes>{Bytes{}};
	}).load(42);
	check(empty.resourcePresent && empty.strings.empty() && empty.stringAt(0) == nullptr,
		"empty resource distinct from missing");
}

void testReaderErrorsAndValidation() {
	const std::string message = rejectionMessage([] {
		XeenEventTextLoader loader(XeenEventTextLoader::ResourceReader{});
		(void)loader;
	});
	check(message.find("absent") != std::string::npos, "missing reader rejected");

	const std::string readFailure = rejectionMessage([] {
		XeenEventTextLoader([](const std::string &) -> std::optional<Bytes> {
			throw std::runtime_error("synthetic read failure");
		}).load(1);
	});
	check(readFailure == "synthetic read failure", "reader error propagation");
}

} // namespace

int main() {
	try {
		testFormatPreservesRawStrings();
		testNaming();
		testLoadingAndLookup();
		testReaderErrorsAndValidation();
		std::cout << "Xeen event text format and loader OK\n";
		return 0;
	} catch (const std::exception &error) {
		std::cerr << error.what() << '\n';
		return 1;
	}
}
