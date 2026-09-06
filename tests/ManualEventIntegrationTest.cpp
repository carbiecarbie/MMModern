#include "formats/xeen/XeenAssetSource.h"
#include "games/xeen/XeenEventLoader.h"
#include "games/xeen/XeenEventSystem.h"
#include "games/xeen/XeenEventTrigger.h"
#include "games/xeen/XeenGameFlagsLoader.h"
#include "games/xeen/XeenInstallationDetector.h"
#include "games/xeen/XeenMapLoader.h"
#include "games/xeen/XeenPartyLoader.h"
#include "games/xeen/XeenWorld.h"

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

} // namespace

int main(int argc, char *argv[]) {
	if (argc != 2) {
		std::cerr << "Usage: mmodern_manual_event_smoke <game directory>\n";
		return 1;
	}
	try {
		const auto installation = XeenInstallationDetector().detect(argv[1]);
		check(installation && installation->hasXeen(),
			"Clouds installation unavailable");
		XeenAssetSource assets(*installation);
		const XeenPartyState party = XeenPartyLoader().loadInitialCloudsParty(assets);
		XeenGameFlags flags = XeenGameFlagsLoader().loadInitialCloudsFlags(assets);
		const auto beforeFlags = flags.values();
		const XeenMapLoader mapLoader;
		XeenWorld world([&](std::uint16_t mapId) {
			return mapLoader.loadGeometryMap(assets, mapId);
		});
		const XeenEventLoader eventLoader([&](const std::string &resourceName)
				-> std::optional<std::vector<std::uint8_t>> {
			if (!assets.hasInitialResource(resourceName))
				return std::nullopt;
			return assets.readInitialResource(resourceName);
		});
		XeenEventSystem events([&](std::uint16_t mapId) {
			return XeenEventScript(eventLoader.load(mapId));
		});

		XeenCamera camera{1, 8, 8, XeenDirection::West};
		check(!hasAutomaticTrigger(world.map(1).geometry, camera.x, camera.y),
			"Castle Basenji interaction unexpectedly has automatic gate");
		const auto result = events.runManualEvent(world, party, camera, flags);
		const auto *error = std::get_if<XeenEventExecutionError>(&result);
		check(error && error->kind == XeenEventExecutionErrorKind::UnsupportedOpcode,
			"expected 14B to reach the unsupported Display0x01 opcode");
		check(error->source && error->source->fileOffset == 461 &&
			error->source->opcode == 0x01,
			"unexpected Castle Basenji source instruction");
		check(camera.mapId == 1 && camera.x == 8 && camera.y == 8 &&
			camera.direction == XeenDirection::West && flags.values() == beforeFlags,
			"unsupported manual event changed state");
		std::cout << "Real map 1 (8,8) West manual dispatch reaches Display0x01 safely\n";
		return 0;
	} catch (const std::exception &error) {
		std::cerr << error.what() << '\n';
		return 1;
	}
}
