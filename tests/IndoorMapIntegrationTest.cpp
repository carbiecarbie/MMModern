#include "formats/xeen/XeenAssetSource.h"
#include "games/xeen/XeenInstallationDetector.h"
#include "games/xeen/XeenIndoorScene.h"
#include "games/xeen/XeenMapLoader.h"
#include "games/xeen/XeenMovement.h"
#include "games/xeen/XeenWorld.h"

#include <array>
#include <cstdlib>
#include <iostream>
#include <stdexcept>
#include <string>

namespace {

void require(bool condition, const char *message) {
	if (!condition)
		throw std::runtime_error(message);
}

struct ExpectedCommand {
	int order;
	const char *resource;
	std::size_t frame;
	int x;
	int y;
	bool flipped;
	int sourceX;
	int sourceY;
	mmodern::XeenDirection sourceFace;
};

} // namespace

int main(int argc, char *argv[]) {
	if (argc != 2) {
		std::cerr << "Usage: mmodern_indoor_map_smoke <game directory>\n";
		return EXIT_FAILURE;
	}
	try {
		const auto installation = mmodern::XeenInstallationDetector().detect(argv[1]);
		require(installation && installation->hasXeen(), "Clouds installation unavailable");
		mmodern::XeenAssetSource assets(*installation);
		const auto bytes = assets.readInitialResource("maze0033.dat");
		require(bytes.size() == 892, "unexpected real maze0033.dat size");

		const mmodern::XeenMapLoader loader;
		const auto geometryMap = loader.loadGeometryMap(assets, 33);
		const auto &geometry = geometryMap.geometry;
		require(geometry.id == 33 && !geometry.isOutdoors(), "map 33 identity/type mismatch");
		require(geometry.flags2 == 0x4000 && (geometry.flags2 & 0x4000),
			"map 33 dark flags mismatch");
		require(geometry.wallKind == 1 && geometry.floorType == 0 &&
			geometry.difficulties[0] == 7, "map 33 wall/floor metadata mismatch");
		require(geometry.neighbors == std::array<std::uint16_t, 4>{115, 0, 0, 0},
			"map 33 neighbors mismatch");

		bool outdoorRejected = false;
		try {
			(void)loader.loadOutdoorMap(assets, 33);
		} catch (const std::runtime_error &) {
			outdoorRejected = true;
		}
		require(outdoorRejected, "loadOutdoorMap accepted real interior map 33");

		mmodern::XeenWorld world([&](std::uint16_t id) {
			return loader.loadGeometryMap(assets, id);
		});
		require(world.sampleCell(33, 4, 8).has_value(), "real interior local sample failed");
		require(!world.sampleCell(33, 4, 16), "real interior followed north neighbor");
		require(world.cachedMapCount() == 1, "real interior neighbor entered cache");

		const auto samples = mmodern::XeenIndoorScene().sampleWalls(world,
			{33, 4, 8, mmodern::XeenDirection::North});
		const std::array<std::uint8_t, 44> expected = {{
			0, 0, 0, 8, 8, 0, 0, 0, 0, 0,
			8, 8, 0, 0, 8, 0, 8, 0, 0,
			8, 8, 8, 8, 8, 8, 0, 8, 8, 8, 8, 8, 8,
			0, 0, 0, 8, 8, 8, 8, 8, 8, 8, 8, 8
		}};
		for (std::size_t i = 0; i < samples.size(); ++i)
			require(samples[i].wallValue && *samples[i].wallValue == expected[i],
				"real map 33 wall query vector mismatch");

		const auto commands = mmodern::XeenIndoorScene().build(world,
			{33, 4, 8, mmodern::XeenDirection::North});
		const std::array<ExpectedCommand, 9> expectedCommands = {{
			{0, "cave.sky", 0, 8, 8, false, 4, 8, mmodern::XeenDirection::North},
			{1, "cave.sky", 1, 8, 25, false, 4, 8, mmodern::XeenDirection::North},
			{2, "cave.gnd", 0, 8, 67, false, 4, 8, mmodern::XeenDirection::North},
			{28, "fcave1.fwl", 7, 8, 64, false, 4, 8, mmodern::XeenDirection::North},
			{72, "scave.swl", 12, 8, 55, false, 2, 11, mmodern::XeenDirection::West},
			{73, "scave.swl", 10, 32, 52, false, 3, 11, mmodern::XeenDirection::West},
			{89, "fcave3.fwl", 17, 88, 52, false, 4, 10, mmodern::XeenDirection::North},
			{90, "fcave3.fwl", 17, 144, 52, false, 5, 10, mmodern::XeenDirection::North},
			{146, "scave.swl", 1, 200, 12, true, 4, 8, mmodern::XeenDirection::East}
		}};
		if (commands.size() != expectedCommands.size()) {
			for (const auto &command : commands)
				std::cerr << command.originalOrder << ' ' << command.resourceName << ' '
					<< command.frame << " (" << command.x << ',' << command.y << ") source ("
					<< command.sourceX << ',' << command.sourceY << ")\n";
			throw std::runtime_error("real map 33 visible indoor command count mismatch");
		}
		for (std::size_t i = 0; i < commands.size(); ++i) {
			const auto &actual = commands[i];
			const auto &wanted = expectedCommands[i];
			require(actual.originalOrder == wanted.order &&
				actual.resourceName == wanted.resource && actual.frame == wanted.frame &&
				actual.x == wanted.x && actual.y == wanted.y &&
				actual.options.horizontalFlip == wanted.flipped &&
				actual.sourceMapId == 33 && actual.sourceX == wanted.sourceX &&
				actual.sourceY == wanted.sourceY && actual.sourceFace == wanted.sourceFace &&
				actual.options.scaleIndex == 0 && actual.options.sceneClipped &&
				!actual.options.bottomClipped,
				"real map 33 visible indoor command mismatch");
		}

		const mmodern::XeenMovement movement;
		const mmodern::XeenCamera start{33, 4, 8, mmodern::XeenDirection::North};
		auto forward = start;
		require(movement.apply(world, forward, mmodern::NavigationAction::MoveForward) ==
			mmodern::XeenMovementResult::Moved && forward.mapId == 33 &&
			forward.x == 4 && forward.y == 9 &&
			forward.direction == mmodern::XeenDirection::North,
			"real map 33 initial forward movement mismatch");
		auto backward = start;
		require(movement.apply(world, backward, mmodern::NavigationAction::MoveBackward) ==
			mmodern::XeenMovementResult::Moved && backward.mapId == 33 &&
			backward.x == 4 && backward.y == 7 &&
			backward.direction == mmodern::XeenDirection::North,
			"real map 33 initial backward movement mismatch");
		require(movement.apply(world, forward, mmodern::NavigationAction::MoveForward) ==
			mmodern::XeenMovementResult::Moved && forward.y == 10,
			"real map 33 approach to north wall mismatch");
		const auto beforeWall = forward;
		require(movement.apply(world, forward, mmodern::NavigationAction::MoveForward) ==
			mmodern::XeenMovementResult::BlockedByWall &&
			forward.mapId == beforeWall.mapId && forward.x == beforeWall.x &&
			forward.y == beforeWall.y && forward.direction == beforeWall.direction,
			"real map 33 north wall did not block atomically");
		std::cout << "Real Clouds map 33 geometry, scene commands and indoor movement OK\n";
		return EXIT_SUCCESS;
	} catch (const std::exception &error) {
		std::cerr << error.what() << '\n';
		return EXIT_FAILURE;
	}
}
