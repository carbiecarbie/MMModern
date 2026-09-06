#include "formats/xeen/XeenAssetSource.h"
#include "formats/xeen/XeenGameFlagsFormat.h"
#include "games/xeen/XeenGameFlagsLoader.h"
#include "games/xeen/XeenInstallationDetector.h"

#include <cstddef>
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

} // namespace

int main(int argc, char *argv[]) {
	if (argc != 2) {
		std::cerr << "Usage: mmodern_game_flags_smoke <game directory>\n";
		return 1;
	}

	try {
		const auto installation = XeenInstallationDetector().detect(argv[1]);
		check(installation && installation->hasXeen(),
			"Clouds installation unavailable");
		XeenAssetSource assets(*installation);
		check(assets.hasInitialResource("maze.pty"), "maze.pty unavailable");
		const auto bytes = assets.readInitialResource("maze.pty");
		check(bytes.size() >= XeenGameFlagsFormat::kCloudsRequiredPartySize,
			"real maze.pty is too small for Clouds flags");

		const XeenGameFlags flags = XeenGameFlagsLoader().loadInitialCloudsFlags(assets);
		std::vector<int> active;
		// The initial installation happens to contain an all-zero block, which by
		// itself cannot expose a one-byte offset error. Synthetic tests use literal
		// positions and nonzero sentinels; this integration comparison deliberately
		// uses the independently specified offset rather than the parser constant.
		constexpr std::size_t kExpectedCloudsFlagsOffset = 659;
		for (int index = 0; index < static_cast<int>(XeenGameFlags::kCount); ++index) {
			const bool expected = ((bytes[kExpectedCloudsFlagsOffset +
				static_cast<std::size_t>(index) / 8] >> (index % 8)) & 1U) != 0;
			check(flags.isSet(index) == expected,
				"loaded flag differs from original maze.pty bit");
			if (flags.isSet(index))
				active.push_back(index);
		}

		check(flags.values().size() == 256, "unexpected Clouds flag count");
		static_cast<void>(flags.isSet(0));
		static_cast<void>(flags.isSet(25));
		static_cast<void>(flags.isSet(255));

		XeenGameFlags mutated = flags;
		if (mutated.isSet(25))
			mutated.clear(25);
		else
			mutated.set(25);
		check(mutated.isSet(25) != flags.isSet(25),
			"copy mutation did not remain independent");

		std::cout << "Resource: maze.pty (" << bytes.size() << " bytes)\n";
		std::cout << "Active Clouds flags: " << active.size() << "\n";
		std::cout << "Active indices:";
		for (const int index : active)
			std::cout << ' ' << index;
		std::cout << "\nFlag 25: " << (flags.isSet(25) ? "set" : "clear") << '\n';
		return 0;
	} catch (const std::exception &error) {
		std::cerr << error.what() << '\n';
		return 1;
	}
}
