#include "XeenTestInstallation.h"
#include "formats/xeen/XeenAssetSource.h"
#include "formats/xeen/XeenGameFlagsFormat.h"
#include "games/xeen/XeenGameFlagsLoader.h"
#include "games/xeen/XeenInstallationDetector.h"

#include <cstddef>
#include <cstdint>
#include <cstdio>
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
		const auto installation = xeenTestInstallationDetector().detect(argv[1]);
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
			const bool expected = (bytes[kExpectedCloudsFlagsOffset +
				static_cast<std::size_t>(index) / 8] & (0x80U >> (index % 8))) != 0;
			check(flags.isSet(index) == expected,
				"loaded flag differs from original maze.pty bit");
			if (flags.isSet(index))
				active.push_back(index);
		}

		check(flags.values().size() == 256, "unexpected Clouds flag count");
		// DOS packs these banks MSB first and MMModern used to read them LSB
		// first. Both orders agree only while every initial bit is zero, which
		// these checks pin: game, world and quest flags, and the DAT bitmaps.
		check(bytes.size() >= 747, "real maze.pty lacks the quest-flag bank");
		for (std::size_t offset = 659; offset < 747; ++offset)
			check(bytes[offset] == 0, "initial maze.pty flag banks are not all zero");
		for (unsigned number = 1; number < 100; ++number) {
			char name[16];
			std::snprintf(name, sizeof(name), "maze%04u.dat", number);
			if (!assets.hasInitialResource(name))
				continue;
			const auto dat = assets.readInitialResource(name);
			check(dat.size() >= 892, "initial DAT is too small for its bitmaps");
			for (std::size_t offset = 828; offset < 892; ++offset)
				check(dat[offset] == 0, "initial DAT seen/stepped bitmaps are not all zero");
		}
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
