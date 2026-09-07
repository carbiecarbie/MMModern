#include "formats/xeen/XeenAssetSource.h"
#include "games/xeen/XeenEventInterpreter.h"
#include "games/xeen/XeenEventLoader.h"
#include "games/xeen/XeenGameFlagsLoader.h"
#include "games/xeen/XeenInstallationDetector.h"
#include "games/xeen/XeenMapLoader.h"
#include "games/xeen/XeenPartyLoader.h"
#include "games/xeen/XeenWorld.h"

#include <array>
#include <cstdint>
#include <iostream>
#include <optional>
#include <stdexcept>
#include <string>
#include <variant>
#include <vector>

using namespace mmodern;

namespace {

void check(bool value, const char *message) {
	if (!value)
		throw std::runtime_error(message);
}

XeenEventExecutionCompleted completed(const XeenEventExecutionResult &result,
		const char *message) {
	const auto *value = std::get_if<XeenEventExecutionCompleted>(&result);
	check(value != nullptr, message);
	return *value;
}

void checkCamera(const XeenCamera &camera, mmodern::XeenMapIdentity mapId, int x, int y,
		XeenDirection direction, const char *message) {
	check(camera.mapId == mapId && camera.x == x && camera.y == y &&
		camera.direction == direction, message);
}

} // namespace

int main(int argc, char *argv[]) {
	if (argc != 2) {
		std::cerr << "Usage: mmodern_event_interpreter_smoke <game directory>\n";
		return 1;
	}

	try {
		const auto installation = XeenInstallationDetector().detect(argv[1]);
		check(installation && installation->hasXeen(),
			"Clouds installation unavailable");
		XeenAssetSource assets(*installation);
		const XeenPartyState party = XeenPartyLoader().loadInitialCloudsParty(assets);
		const XeenGameFlags initialFlags =
			XeenGameFlagsLoader().loadInitialCloudsFlags(assets);
		check(party.party.size() && party.party.member(party.roster, 0).currentSp == 2,
			"unexpected initial first-member SP");
		check(!initialFlags.isSet(25), "expected initial Clouds flag 25 to be clear");
		const auto originalFlags = initialFlags.values();

		const XeenMapLoader mapLoader;
		XeenWorld world([&](mmodern::XeenMapIdentity mapId) {
			return mapLoader.loadGeometryMap(assets, mapId);
		}, [&](XeenMapIdentity id) { return mapLoader.loadObjects(assets, id); });
		const XeenEventLoader eventLoader([&](const std::string &resourceName)
				-> std::optional<std::vector<std::uint8_t>> {
			if (!assets.hasInitialResource(resourceName))
				return std::nullopt;
			return assets.readInitialResource(resourceName);
		});
		const XeenEventInterpreter::ScriptProvider scripts =
			[&](mmodern::XeenMapIdentity mapId) {
				return XeenEventScript(eventLoader.load(mapId));
			};
		const XeenEventInterpreter interpreter;
		const std::array<XeenDirection, 4> directions{{
			XeenDirection::North, XeenDirection::East,
			XeenDirection::South, XeenDirection::West
		}};

		for (const XeenDirection direction : directions) {
			const XeenCamera input31{31, 2, 9, direction};
			const auto result31 = completed(interpreter.execute(input31, party,
				initialFlags, world, scripts), "map 31 interpreter failed");
			check(result31.instructionCount == 1, "map 31 instruction count");
			check(result31.finalGameFlags.values() == initialFlags.values(),
				"map 31 unexpectedly changed flags");
			checkCamera(result31.finalCamera, 31, 4, 9, direction,
				"map 31 final camera");
			checkCamera(input31, 31, 2, 9, direction, "map 31 input camera changed");

			const XeenCamera input42{42, 9, 12, direction};
			const auto clearResult = completed(interpreter.execute(input42, party,
				initialFlags, world, scripts), "map 42 clear-flag branch failed");
			check(clearResult.instructionCount == 3,
				"map 42 clear-flag instruction count");
			check(clearResult.finalGameFlags.values() == initialFlags.values(),
				"map 42 clear branch unexpectedly changed flags");
			checkCamera(clearResult.finalCamera, 42, 4, 2, direction,
				"map 42 clear-flag final camera");

			XeenGameFlags setFlags = initialFlags;
			setFlags.set(25);
			const auto setResult = completed(interpreter.execute(input42, party,
				setFlags, world, scripts), "map 42 set-flag branch failed");
			check(setResult.instructionCount == 2,
				"map 42 set-flag instruction count");
			check(setResult.finalGameFlags.values() == setFlags.values(),
				"map 42 set branch unexpectedly changed flags");
			checkCamera(setResult.finalCamera, 42, 9, 12, direction,
				"map 42 set-flag final camera");
			check(setFlags.isSet(25), "interpreter changed test flag copy");
			checkCamera(input42, 42, 9, 12, direction, "map 42 input camera changed");
		}

		check(initialFlags.values() == originalFlags && !initialFlags.isSet(25),
			"interpreter changed initial real flags");
		check(party.party.member(party.roster, 0).currentSp == 2,
			"interpreter changed initial real Party");
		std::cout << "Real headless event execution for maps 31/42 and both flag branches OK\n";
		return 0;
	} catch (const std::exception &error) {
		std::cerr << error.what() << '\n';
		return 1;
	}
}
