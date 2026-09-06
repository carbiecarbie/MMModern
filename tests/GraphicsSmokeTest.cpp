#include "app/Application.h"
#include "formats/xeen/XeenAssetSource.h"
#include "games/xeen/CloudsMapComposer.h"
#include "games/xeen/CloudsUiComposer.h"
#include "games/xeen/XeenCharacterRules.h"
#include "games/xeen/XeenInstallationDetector.h"
#include "games/xeen/XeenIndoorScene.h"
#include "games/xeen/XeenMapLoader.h"
#include "games/xeen/XeenMovement.h"
#include "games/xeen/XeenOutdoorScene.h"
#include "games/xeen/XeenPartyLoader.h"
#include "games/xeen/XeenWorld.h"

#define SDL_MAIN_HANDLED
#include <SDL.h>

#include <atomic>
#include <algorithm>
#include <array>
#include <chrono>
#include <iostream>
#include <string>
#include <thread>
#include <vector>

// Exercises the existing Application::run path with real assets. Run with
// SDL_VIDEODRIVER=dummy and SDL_RENDER_DRIVER=software for a headless smoke test.
// SDL_PushEvent is thread-safe; all initialization/rendering stays on main.
int main(int argc, char *argv[]) {
	const bool oldForm = argc == 3;
	const std::string mode = oldForm ? "ui" : (argc == 4 ? argv[2] : "");
	const std::string closeMode = oldForm ? (argc == 3 ? argv[2] : "") :
		(argc == 4 ? argv[3] : "");
	if ((argc != 3 && argc != 4) ||
			(mode != "ui" && mode != "map" && mode != "indoor" &&
				mode != "event" && mode != "manual") ||
			(closeMode != "escape" && closeMode != "quit")) {
		std::cerr << "Usage: mmodern_graphics_smoke <game directory> "
			"[ui|map|indoor|event|manual] <escape|quit>\n";
		return 1;
	}
	std::atomic<bool> finished{false};
	std::atomic<int> sent{0};
	const bool escape = closeMode == "escape";
	if (mode == "map") {
		const auto installation = mmodern::XeenInstallationDetector().detect(argv[1]);
		if (!installation || !installation->hasXeen()) {
			std::cerr << "Area A1 integration fixture unavailable\n";
			return 1;
		}
		mmodern::XeenAssetSource assets(*installation);
		const mmodern::XeenMapLoader mapLoader;
		mmodern::XeenWorld world([&](std::uint16_t mapId) {
			return mapLoader.loadOutdoorMap(assets, mapId);
		});
		const auto &map = world.map(1);
		const auto commands = mmodern::XeenOutdoorScene().build(world);
		const auto mountains = std::count_if(commands.begin(), commands.end(), [](const auto &command) {
			return command.resourceName == "mount.wal";
		});
		if (commands.size() != 32 || mountains != 4) {
			std::cerr << "Unexpected real Area A1 draw list: " << commands.size()
				<< " commands, " << mountains << " mountains\n";
			return 1;
		}
		const mmodern::XeenMovement movement;
		mmodern::XeenCamera camera = mmodern::XeenOutdoorScene::kAreaA1Camera;
		if (movement.apply(world, camera, mmodern::NavigationAction::MoveForward) !=
				mmodern::XeenMovementResult::BlockedByTerrain ||
				camera.mapId != 1 || camera.x != 9 || camera.y != 6 ||
				camera.direction != mmodern::XeenDirection::South) {
			std::cerr << "Real Area A1 cell (9,5) did not preserve the blocked camera\n";
			return 1;
		}
		if (movement.apply(world, camera, mmodern::NavigationAction::MoveBackward) !=
				mmodern::XeenMovementResult::Moved || camera.mapId != 1 ||
				camera.x != 9 || camera.y != 7 ||
				camera.direction != mmodern::XeenDirection::South) {
			std::cerr << "Real Area A1 initial backward step did not reach (9,7)\n";
			return 1;
		}

		const auto &map5 = world.map(5);
		const auto &map2 = world.map(2);
		if (map.geometry.neighbors != std::array<std::uint16_t, 4>{0, 5, 2, 0} ||
				map5.geometry.neighbors[3] != 1 || map2.geometry.neighbors[0] != 1 ||
				map5.geometry.cells[6 * 16].rawWord != 0x0006 ||
				map2.geometry.cells[15 * 16 + 9].rawWord != 0x0006) {
			std::cerr << "Unexpected real neighbor metadata for maps 001/005/002\n";
			return 1;
		}
		mmodern::XeenCamera east{1, 15, 6, mmodern::XeenDirection::East};
		if (movement.apply(world, east, mmodern::NavigationAction::MoveForward) !=
				mmodern::XeenMovementResult::Moved || east.mapId != 5 || east.x != 0 || east.y != 6 ||
				mmodern::XeenOutdoorScene().build(world, east).empty()) {
			std::cerr << "Real transition 001 -> 005 failed\n";
			return 1;
		}
		mmodern::XeenCamera south{1, 9, 0, mmodern::XeenDirection::South};
		if (movement.apply(world, south, mmodern::NavigationAction::MoveForward) !=
				mmodern::XeenMovementResult::Moved || south.mapId != 2 || south.x != 9 || south.y != 15 ||
				mmodern::XeenOutdoorScene().build(world, south).empty()) {
			std::cerr << "Real transition 001 -> 002 failed\n";
			return 1;
		}
		const auto diagonal = world.sampleCell(1, 16, -1);
		if (!diagonal || diagonal->mapId != 6 || diagonal->x != 0 || diagonal->y != 15) {
			std::cerr << "Real southeast diagonal resolution failed\n";
			return 1;
		}
		mmodern::XeenAssetSource graphics(*installation,
			mmodern::CloudsUiComposer::kWidth, mmodern::CloudsUiComposer::kHeight);
		const auto partyState = mmodern::XeenPartyLoader().loadInitialCloudsParty(graphics);
		const mmodern::XeenCharacterRulesContext rulesContext{
			mmodern::kCloudsInitialYear};
		const auto hpPlacements = mmodern::CloudsUiComposer::buildHpPlacements(
			partyState, rulesContext);
		const std::array<int, 6> expectedHpX = {13, 50, 86, 122, 158, 194};
		if (hpPlacements.size() != expectedHpX.size()) {
			std::cerr << "Unexpected real initial HP indicator count\n";
			return 1;
		}
		for (std::size_t i = 0; i < hpPlacements.size(); ++i) {
			if (hpPlacements[i].partySlot != i || hpPlacements[i].frame != 0 ||
					hpPlacements[i].x != expectedHpX[i] || hpPlacements[i].y != 182) {
				std::cerr << "Unexpected real initial HP indicator placement\n";
				return 1;
			}
		}
		const mmodern::CloudsMapComposer composer;
		const auto eastFrame = composer.compose(graphics, world, partyState, east,
			rulesContext);
		const auto southFrame = composer.compose(graphics, world, partyState, south,
			rulesContext);
		const auto spaceFrame = composer.compose(graphics, world, partyState,
			{1, 9, 15, mmodern::XeenDirection::North}, rulesContext);
		if (!eastFrame.isValid() || !southFrame.isValid() || !spaceFrame.isValid()) {
			std::cerr << "Real neighbor transition frame composition failed\n";
			return 1;
		}
	} else if (mode == "indoor") {
		const auto installation = mmodern::XeenInstallationDetector().detect(argv[1]);
		if (!installation || !installation->hasXeen()) {
			std::cerr << "Dwarf Mine integration fixture unavailable\n";
			return 1;
		}
		mmodern::XeenAssetSource assets(*installation,
			mmodern::CloudsUiComposer::kWidth, mmodern::CloudsUiComposer::kHeight);
		const mmodern::XeenMapLoader mapLoader;
		mmodern::XeenWorld world([&](std::uint16_t mapId) {
			return mapLoader.loadGeometryMap(assets, mapId);
		});
		const auto partyState = mmodern::XeenPartyLoader().loadInitialCloudsParty(assets);
		const mmodern::XeenCharacterRulesContext rulesContext{
			mmodern::kCloudsInitialYear};
		const mmodern::CloudsMapComposer composer;
		const mmodern::XeenMovement movement;
		const mmodern::XeenCamera start{33, 4, 8, mmodern::XeenDirection::North};
		const auto commands = mmodern::XeenIndoorScene().build(world, start);
		if (commands.size() != 9) {
			std::cerr << "Unexpected real Dwarf Mine draw list\n";
			return 1;
		}

		auto camera = start;
		std::vector<mmodern::IndexedFrame> frames;
		frames.push_back(composer.compose(assets, world, partyState, camera, rulesContext));
		const std::array<mmodern::NavigationAction, 6> actions = {{
			mmodern::NavigationAction::TurnLeft,
			mmodern::NavigationAction::TurnRight,
			mmodern::NavigationAction::MoveForward,
			mmodern::NavigationAction::MoveBackward,
			mmodern::NavigationAction::MoveForward,
			mmodern::NavigationAction::MoveForward
		}};
		for (const auto action : actions) {
			const auto result = movement.apply(world, camera, action);
			if (result != mmodern::XeenMovementResult::Moved &&
					result != mmodern::XeenMovementResult::Turned) {
				std::cerr << "Unexpected block during indoor framebuffer sequence\n";
				return 1;
			}
			frames.push_back(composer.compose(assets, world, partyState, camera, rulesContext));
		}
		const auto beforeWall = camera;
		if (movement.apply(world, camera, mmodern::NavigationAction::MoveForward) !=
				mmodern::XeenMovementResult::BlockedByWall ||
				camera.mapId != beforeWall.mapId || camera.x != beforeWall.x ||
				camera.y != beforeWall.y || camera.direction != beforeWall.direction ||
				!std::all_of(frames.begin(), frames.end(), [](const auto &frame) {
					return frame.isValid();
				})) {
			std::cerr << "Indoor wall block or framebuffer recomposition failed\n";
			return 1;
		}
	}
	std::thread closer([&] {
		if (escape && mode == "manual") {
			struct KeyEvent {
				std::uint32_t type;
				SDL_Keycode key;
				std::uint8_t repeat;
			};
			const std::array<KeyEvent, 6> events{{
				{SDL_KEYDOWN, SDLK_SPACE, 0},
				{SDL_KEYDOWN, SDLK_SPACE, 1},
				{SDL_KEYUP, SDLK_SPACE, 0},
				{SDL_KEYDOWN, SDLK_SPACE, 0},
				{SDL_KEYDOWN, SDLK_w, 0},
				{SDL_KEYDOWN, SDLK_ESCAPE, 0}
			}};
			for (const auto &key : events) {
				for (int attempt = 0; attempt < 10 && !finished; ++attempt) {
					std::this_thread::sleep_for(std::chrono::milliseconds(200));
					SDL_Event event{};
					event.type = key.type;
					event.key.state = key.type == SDL_KEYUP ? SDL_RELEASED : SDL_PRESSED;
					event.key.repeat = key.repeat;
					event.key.keysym.sym = key.key;
					if (SDL_PushEvent(&event) == 1) {
						++sent;
						break;
					}
				}
			}
			return;
		}
		const std::vector<SDL_Keycode> keys = mode == "map" ?
			std::vector<SDL_Keycode>{SDLK_w, SDLK_s, SDLK_LEFT, SDLK_RIGHT, SDLK_ESCAPE} :
			mode == "indoor" ?
			std::vector<SDL_Keycode>{SDLK_LEFT, SDLK_RIGHT, SDLK_w, SDLK_s,
				SDLK_w, SDLK_w, SDLK_w, SDLK_ESCAPE} :
			std::vector<SDL_Keycode>{SDLK_ESCAPE};
		if (!escape) {
			std::this_thread::sleep_for(std::chrono::milliseconds(500));
			SDL_Event event{};
			event.type = SDL_QUIT;
			if (SDL_PushEvent(&event) == 1)
				++sent;
			return;
		}
		for (const SDL_Keycode key : keys) {
			for (int attempt = 0; attempt < 10 && !finished; ++attempt) {
				std::this_thread::sleep_for(std::chrono::milliseconds(200));
				SDL_Event event{};
				event.type = SDL_KEYDOWN;
				event.key.state = SDL_PRESSED;
				event.key.repeat = 0;
				event.key.keysym.sym = key;
				if (SDL_PushEvent(&event) == 1) {
					++sent;
					break;
				}
			}
		}
	});
	const int result = mode == "map" ? mmodern::Application().renderMap(argv[1]) :
		mode == "indoor" ? mmodern::Application().renderMap(argv[1], 33, 4, 8,
			mmodern::XeenDirection::North) :
		mode == "manual" ? mmodern::Application().renderMap(argv[1], 1, 8, 8,
			mmodern::XeenDirection::West) :
		mode == "event" ? mmodern::Application().renderMap(argv[1], 31, 2, 9,
			mmodern::XeenDirection::North) : mmodern::Application().run(argv[1]);
	finished = true;
	closer.join();
	const int expectedEvents = escape && mode == "map" ? 5 :
		escape && mode == "indoor" ? 8 : escape && mode == "manual" ? 6 : 1;
	if (result != 0 || sent != expectedEvents) {
		std::cerr << "Graphics smoke test failed\n";
		return 1;
	}
	std::cout << "SDL " << mode << " composition/presentation and " << closeMode
		<< " shutdown OK\n";
	return 0;
}
