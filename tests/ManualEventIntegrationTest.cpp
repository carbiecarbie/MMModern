#include "formats/xeen/XeenAssetSource.h"
#include "games/xeen/XeenEventLoader.h"
#include "games/xeen/XeenEventSystem.h"
#include "games/xeen/XeenEventTextLoader.h"
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
		const XeenEventTextLoader textLoader([&](const std::string &resourceName)
				-> std::optional<std::vector<std::uint8_t>> {
			if (!assets.hasArchiveResource(resourceName))
				return std::nullopt;
			return assets.readArchiveResource(resourceName);
		});
		XeenEventSystem events([&](std::uint16_t mapId) {
			return XeenEventScript(eventLoader.load(mapId));
		}, [&](std::uint16_t mapId) {
			return textLoader.load(mapId);
		});

		XeenCamera camera{1, 8, 8, XeenDirection::West};
		check(!hasAutomaticTrigger(world.map(1).geometry, camera.x, camera.y),
			"Castle Basenji interaction unexpectedly has automatic gate");
		const auto result = events.runManualEvent(world, party, camera, flags);
		const auto *display = std::get_if<XeenEventExecutionSuspended>(&result);
		check(display && display->request.kind == XeenPresentationKind::CenteredMessage &&
			display->request.response == XeenPresentationResponseRequirement::Presented,
			"expected Castle Basenji centered display request");
		check(display->request.source.fileOffset == 461 &&
			display->request.source.opcode == 0x01 && display->request.mapId == 1 &&
			display->request.textIndex == 19 && !display->request.text.empty(),
			"unexpected Castle Basenji text request");
		check(camera.mapId == 1 && camera.x == 8 && camera.y == 8 &&
			camera.direction == XeenDirection::West && flags.values() == beforeFlags,
			"pending display changed committed state");

		const auto confirmationResult = events.resumeManualEvent(display->state,
			XeenPresentationResponse::Presented, world, party, camera, flags);
		const auto *confirmation =
			std::get_if<XeenEventExecutionSuspended>(&confirmationResult);
		check(confirmation && confirmation->request.kind ==
			XeenPresentationKind::Confirmation && confirmation->request.response ==
			XeenPresentationResponseRequirement::YesNo,
			"Castle Basenji Action 44 did not request Yes/No");
		const auto noResult = events.resumeManualEvent(confirmation->state,
			XeenPresentationResponse::No, world, party, camera, flags);
		check(std::holds_alternative<XeenManualEventCompleted>(noResult) &&
			camera.mapId == 1 && camera.x == 8 && camera.y == 8 &&
			flags.values() == beforeFlags,
			"Castle Basenji No response should complete without teleport");

		XeenCamera yesCamera{1, 8, 8, XeenDirection::West};
		XeenGameFlags yesFlags = XeenGameFlagsLoader().loadInitialCloudsFlags(assets);
		const auto yesDisplayResult = events.runManualEvent(
			world, party, yesCamera, yesFlags);
		const auto *yesDisplay =
			std::get_if<XeenEventExecutionSuspended>(&yesDisplayResult);
		check(yesDisplay != nullptr, "Castle Basenji Yes path display missing");
		const auto yesConfirmationResult = events.resumeManualEvent(yesDisplay->state,
			XeenPresentationResponse::Presented, world, party, yesCamera, yesFlags);
		const auto *yesConfirmation =
			std::get_if<XeenEventExecutionSuspended>(&yesConfirmationResult);
		check(yesConfirmation != nullptr, "Castle Basenji Yes path confirmation missing");
		const auto yesResult = events.resumeManualEvent(yesConfirmation->state,
			XeenPresentationResponse::Yes, world, party, yesCamera, yesFlags);
		check(std::holds_alternative<XeenManualEventCompleted>(yesResult) &&
			(yesCamera.mapId != 1 || yesCamera.x != 8 || yesCamera.y != 8),
			"Castle Basenji Yes response did not follow the teleport path");

		std::cout << "Castle Basenji text 19, Action 44 No, and Yes teleport semantics OK\n";
		return 0;
	} catch (const std::exception &error) {
		std::cerr << error.what() << '\n';
		return 1;
	}
}
