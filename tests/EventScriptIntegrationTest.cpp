#include "formats/xeen/XeenAssetSource.h"
#include "games/xeen/XeenEventDecoder.h"
#include "games/xeen/XeenEventLoader.h"
#include "games/xeen/XeenEventScript.h"
#include "games/xeen/XeenEventTrigger.h"
#include "games/xeen/XeenInstallationDetector.h"
#include "games/xeen/XeenMapLoader.h"

#include <array>
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

XeenEventScript loadScript(XeenAssetSource &assets, std::uint16_t mapId) {
	const XeenEventLoader loader([&assets](const std::string &resourceName)
			-> std::optional<std::vector<std::uint8_t>> {
		if (!assets.hasInitialResource(resourceName))
			return std::nullopt;
		return assets.readInitialResource(resourceName);
	});
	return XeenEventScript(loader.load(mapId));
}

void checkRecord(const XeenEventRecord *record, std::size_t offset,
		std::uint8_t opcode, const std::vector<std::uint8_t> &parameters,
		const char *message) {
	check(record && record->fileOffset == offset && record->opcode == opcode &&
		record->direction == kXeenEventDirectionAll &&
		record->parameters == parameters, message);
}

const XeenDecodedEventInstruction &decodedInstruction(
		const XeenEventDecodeResult &result, const char *message) {
	const auto *decoded = std::get_if<XeenDecodedEventInstruction>(&result);
	check(decoded != nullptr, message);
	return *decoded;
}

void checkTeleport(const XeenEventRecord *record, std::uint16_t sourceMapId,
		std::uint8_t destinationMapId, int x, int y, const char *message) {
	check(record != nullptr, message);
	const auto result = XeenEventDecoder::decode(*record,
		{sourceMapId, std::string("maze")});
	const auto &decoded = decodedInstruction(result, message);
	const auto *teleport = std::get_if<XeenEventTeleportAndExit>(&decoded.operation);
	check(teleport && teleport->mapId == destinationMapId &&
		teleport->x == x && teleport->y == y, message);
}

void checkConditional(const XeenEventRecord *record, std::uint16_t sourceMapId,
		XeenEventComparison comparison, std::uint8_t action,
		std::uint32_t value, std::uint8_t targetLine, const char *message) {
	check(record != nullptr, message);
	const auto result = XeenEventDecoder::decode(*record,
		{sourceMapId, std::string("maze")});
	const auto &decoded = decodedInstruction(result, message);
	const auto *conditional = std::get_if<XeenEventConditional>(&decoded.operation);
	check(conditional && conditional->comparison == comparison &&
		conditional->action == action && conditional->value == value &&
		conditional->targetLine == targetLine, message);
}

void checkExit(const XeenEventRecord *record, std::uint16_t sourceMapId,
		const char *message) {
	check(record != nullptr, message);
	const auto result = XeenEventDecoder::decode(*record,
		{sourceMapId, std::string("maze")});
	check(std::holds_alternative<XeenEventExit>(
		decodedInstruction(result, message).operation), message);
}

} // namespace

int main(int argc, char *argv[]) {
	try {
		if (argc != 2)
			throw std::runtime_error("usage: mmodern_event_script_smoke <game-directory>");
		const auto installation = XeenInstallationDetector().detect(argv[1]);
		check(installation && installation->hasXeen(), "Clouds installation unavailable");
		XeenAssetSource assets(*installation);
		const XeenMapLoader mapLoader;
		const std::array<XeenDirection, 4> directions{{
			XeenDirection::North, XeenDirection::East,
			XeenDirection::South, XeenDirection::West
		}};

		const XeenEventScript map31 = loadScript(assets, 31);
		const XeenMap geometry31 = mapLoader.loadGeometryMap(assets, 31);
		check(hasAutomaticTrigger(geometry31.geometry, 2, 9),
			"map 31 automatic trigger");
		for (const XeenDirection direction : directions) {
			checkRecord(map31.findInstruction(2, 9, direction, 0), 1934, 0x07,
				{0x1f, 0x04, 0x09}, "map 31 line zero lookup");
		}

		const XeenEventScript map42 = loadScript(assets, 42);
		const XeenMap geometry42 = mapLoader.loadGeometryMap(assets, 42);
		check(hasAutomaticTrigger(geometry42.geometry, 9, 12),
			"map 42 automatic trigger");
		for (const XeenDirection direction : directions) {
			checkRecord(map42.findInstruction(9, 12, direction, 0), 1190, 0x09,
				{0x14, 0x19, 0x02}, "map 42 line zero lookup");
			checkRecord(map42.findInstruction(9, 12, direction, 1), 1199, 0x08,
				{0x09, 0x00, 0x03}, "map 42 line one lookup");
			checkRecord(map42.findInstruction(9, 12, direction, 2), 1208, 0x12,
				{}, "map 42 line two lookup");
			checkRecord(map42.findInstruction(9, 12, direction, 3), 1214, 0x07,
				{0x2a, 0x04, 0x02}, "map 42 line three lookup");
		}

		checkTeleport(map31.findInstruction(2, 9, XeenDirection::North, 0),
			31, 31, 4, 9, "map 31 semantic teleport");
		checkConditional(map42.findInstruction(9, 12, XeenDirection::North, 0),
			42, XeenEventComparison::Equal, 20, 25, 2,
			"map 42 semantic equality");
		checkConditional(map42.findInstruction(9, 12, XeenDirection::North, 1),
			42, XeenEventComparison::GreaterOrEqual, 9, 0, 3,
			"map 42 semantic greater-or-equal");
		checkExit(map42.findInstruction(9, 12, XeenDirection::North, 2),
			42, "map 42 semantic exit");
		checkTeleport(map42.findInstruction(9, 12, XeenDirection::North, 3),
			42, 42, 4, 2, "map 42 semantic teleport");

		std::cout << "Real event lookup, triggers, and semantic decoding for maps 31/42 OK\n";
		return 0;
	} catch (const std::exception &error) {
		std::cerr << error.what() << '\n';
		return 1;
	}
}
