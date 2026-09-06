#include "formats/xeen/XeenAssetSource.h"
#include "games/xeen/XeenEventTextLoader.h"
#include "games/xeen/XeenInstallationDetector.h"

#include <cstdint>
#include <iostream>
#include <optional>
#include <stdexcept>
#include <string>
#include <vector>

using namespace mmodern;

namespace {

void check(bool value, const char *message) {
	if (!value)
		throw std::runtime_error(message);
}

} // namespace

int main(int argc, char *argv[]) {
	if (argc != 2) {
		std::cerr << "Usage: mmodern_event_text_smoke <game directory>\n";
		return 1;
	}

	try {
		const auto installation = XeenInstallationDetector().detect(argv[1]);
		check(installation && installation->hasXeen(), "Clouds installation unavailable");
		XeenAssetSource assets(*installation);
		const XeenEventTextLoader loader([&assets](const std::string &resourceName)
				-> std::optional<std::vector<std::uint8_t>> {
			if (!assets.hasArchiveResource(resourceName))
				return std::nullopt;
			return assets.readArchiveResource(resourceName);
		});

		const XeenEventTextFile map1 = loader.load(1);
		check(map1.resourcePresent, "aaze0001.txt unavailable from xeen.cc");
		check(map1.strings.size() == 23, "unexpected aaze0001.txt string count");
		check(map1.stringAt(5) != nullptr && !map1.stringAt(5)->empty(),
			"expected map 1 event text index 5");
		check(map1.stringAt(map1.strings.size()) == nullptr,
			"out-of-range text index should be invalid");

		std::cout << "Resource: " << map1.resourceName << "\n";
		std::cout << "Bytes: " << assets.readArchiveResource(map1.resourceName).size() << "\n";
		std::cout << "Strings: " << map1.strings.size() << "\n";
		return 0;
	} catch (const std::exception &error) {
		std::cerr << error.what() << '\n';
		return 1;
	}
}
