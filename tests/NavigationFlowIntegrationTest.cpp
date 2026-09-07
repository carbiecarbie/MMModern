#include "app/XeenNavigationFlow.h"

#include "formats/xeen/XeenAssetSource.h"
#include "games/xeen/XeenEventLoader.h"
#include "games/xeen/XeenEventScript.h"
#include "games/xeen/XeenEventSystem.h"
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

XeenAutomaticEventCompleted completed(const XeenAutomaticEventResult &result,
		const char *message) {
	const auto *value = std::get_if<XeenAutomaticEventCompleted>(&result);
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
		std::cerr << "Usage: mmodern_navigation_flow_smoke <game directory>\n";
		return 1;
	}

	try {
		const auto installation = XeenInstallationDetector().detect(argv[1]);
		check(installation && installation->hasXeen(),
			"Clouds installation unavailable");
		XeenAssetSource assets(*installation);
		const XeenPartyState party =
			XeenPartyLoader().loadInitialCloudsParty(assets);
		const XeenGameFlags initialFlags =
			XeenGameFlagsLoader().loadInitialCloudsFlags(assets);
		check(!initialFlags.isSet(25),
			"expected initial Clouds flag 25 to be clear");

		const XeenMapLoader mapLoader;
		XeenWorld world([&](mmodern::XeenMapIdentity mapId) {
			return mapLoader.loadGeometryMap(assets, mapId);
		});
		const XeenEventLoader eventLoader([&](const std::string &resourceName)
				-> std::optional<std::vector<std::uint8_t>> {
			if (!assets.hasInitialResource(resourceName))
				return std::nullopt;
			return assets.readInitialResource(resourceName);
		});
		XeenEventSystem events([&](mmodern::XeenMapIdentity mapId) {
			return XeenEventScript(eventLoader.load(mapId));
		});
		XeenNavigationFlow flow(events);
		const std::array<XeenDirection, 4> directions{{
			XeenDirection::North, XeenDirection::East,
			XeenDirection::South, XeenDirection::West
		}};

		for (const XeenDirection direction : directions) {
			XeenCamera camera31{31, 2, 9, direction};
			XeenGameFlags flags31 = initialFlags;
			const auto result31 = completed(flow.processInitialEvent(
				world, party, camera31, flags31), "map 31 flow failed");
			check(result31.instructionCount == 1 && result31.cameraChanged &&
				!result31.flagsChanged, "map 31 flow metadata");
			checkCamera(camera31, 31, 4, 9, direction,
				"map 31 flow committed camera");
			check(flags31.values() == initialFlags.values(),
				"map 31 flow changed flags");

			XeenCamera camera42{42, 9, 12, direction};
			XeenGameFlags clearFlags = initialFlags;
			const auto clearResult = completed(flow.processInitialEvent(
				world, party, camera42, clearFlags),
				"map 42 clear flow failed");
			check(clearResult.instructionCount == 3 && clearResult.cameraChanged &&
				!clearResult.flagsChanged, "map 42 clear flow metadata");
			checkCamera(camera42, 42, 4, 2, direction,
				"map 42 clear flow committed camera");
			check(clearFlags.values() == initialFlags.values(),
				"map 42 clear flow changed flags");

			XeenCamera setCamera42{42, 9, 12, direction};
			XeenGameFlags setFlags = initialFlags;
			setFlags.set(25);
			const auto beforeSet = setFlags.values();
			const auto setResult = completed(flow.processInitialEvent(
				world, party, setCamera42, setFlags),
				"map 42 set flow failed");
			check(setResult.instructionCount == 2 && !setResult.cameraChanged &&
				!setResult.flagsChanged, "map 42 set flow metadata");
			checkCamera(setCamera42, 42, 9, 12, direction,
				"map 42 set flow camera");
			check(setFlags.values() == beforeSet && setFlags.isSet(25),
				"map 42 set flow changed flags");
		}

		check(events.cachedScriptCount() == 2,
			"navigation flow should reuse the persistent event cache");
		std::cout << "Real initial navigation flow for maps 31/42 OK\n";
		return 0;
	} catch (const std::exception &error) {
		std::cerr << error.what() << '\n';
		return 1;
	}
}
