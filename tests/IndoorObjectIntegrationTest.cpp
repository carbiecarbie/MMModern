#include "app/XeenEventFlow.h"
#include "formats/xeen/XeenAssetSource.h"
#include "formats/xeen/XeenFontFormat.h"
#include "formats/xeen/XeenSaveFormat.h"
#include "games/xeen/CloudsMapComposer.h"
#include "games/xeen/CloudsUiComposer.h"
#include "games/xeen/XeenCharacterRules.h"
#include "games/xeen/XeenEventLoader.h"
#include "games/xeen/XeenEventSystem.h"
#include "games/xeen/XeenEventTextLoader.h"
#include "games/xeen/XeenGameFlagsLoader.h"
#include "games/xeen/XeenInstallationDetector.h"
#include "games/xeen/XeenMapLoader.h"
#include "games/xeen/XeenPartyLoader.h"
#include "games/xeen/XeenSaveState.h"
#include "games/xeen/XeenWorld.h"
#include "platform/sdl/SdlWindow.h"

#define SDL_MAIN_HANDLED
#include <SDL.h>

#include <algorithm>
#include <array>
#include <iostream>
#include <optional>
#include <stdexcept>
#include <string>
#include <variant>
#include <vector>

using namespace mmodern;

namespace {

void check(bool condition, const char *message) {
	if (!condition) throw std::runtime_error(message);
}

const XeenIndoorDrawCommand *findTarget(
		const std::vector<XeenIndoorDrawCommand> &commands,
		XeenObjectIdentity target) {
	const auto found = std::find_if(commands.begin(), commands.end(), [&](const auto &command) {
		return command.object() && command.object()->visual.identity == target;
	});
	return found == commands.end() ? nullptr : &*found;
}

IndexedFrame replay(XeenAssetSource &assets, const CloudsMapComposer &composer,
		const XeenPartyState &party, const XeenCharacterRulesContext &context,
		const std::vector<XeenIndoorDrawCommand> &commands,
		std::optional<XeenObjectIdentity> omitted = std::nullopt,
		const XeenIndoorDrawCommand *targetLast = nullptr) {
	CloudsUiComposer().loadBackground(assets);
	for (const auto &command : commands) {
		if (omitted && command.object() && command.object()->visual.identity == *omitted)
			continue;
		composer.drawIndoorCommands(assets, {command});
	}
	if (targetLast) composer.drawIndoorCommands(assets, {*targetLast});
	composer.drawInterfaceLayers(assets, party, context);
	return assets.snapshot();
}

XeenEventExecutionSuspended suspended(const XeenManualEventResult &result,
		const char *message) {
	const auto *value = std::get_if<XeenEventExecutionSuspended>(&result);
	check(value != nullptr, message);
	return *value;
}

XeenManualEventCompleted completed(const XeenManualEventResult &result,
		const char *message) {
	const auto *value = std::get_if<XeenManualEventCompleted>(&result);
	check(value != nullptr, message);
	return *value;
}

} // namespace

int main(int argc, char *argv[]) {
	if (argc != 2 && !(argc == 3 && std::string(argv[2]) == "sdl")) {
		std::cerr << "Usage: mmodern_indoor_object_smoke <game directory> [sdl]\n";
		return 1;
	}
	try {
		const auto installation = XeenInstallationDetector().detect(argv[1]);
		check(installation && installation->hasXeen(), "Clouds installation unavailable");
		XeenAssetSource assets(*installation, 320, 200);
		const XeenMapLoader mapLoader;
		auto mapProvider = [&](XeenMapIdentity id) {
			return mapLoader.loadGeometryMap(assets, id);
		};
		auto objectProvider = [&](XeenMapIdentity id) {
			return mapLoader.loadObjects(assets, id);
		};
		const XeenEventLoader eventLoader([&](const std::string &name)
				-> std::optional<std::vector<std::uint8_t>> {
			if (!assets.hasInitialResource(name)) return std::nullopt;
			return assets.readInitialResource(name);
		});
		const XeenEventTextLoader textLoader([&](const std::string &name)
				-> std::optional<std::vector<std::uint8_t>> {
			if (!assets.hasArchiveResource(name)) return std::nullopt;
			return assets.readArchiveResource(name);
		});
		auto scriptProvider = [&](XeenMapIdentity id) {
			return XeenEventScript(eventLoader.load(id));
		};
		auto textProvider = [&](XeenMapIdentity id) {
			return textLoader.load(id);
		};
		XeenWorld world(mapProvider, objectProvider);
		const XeenMapIdentity mapId{XeenSide::Clouds, 29};
		const XeenObjectIdentity target{mapId, 14};
		const auto &objects = world.objectFile(mapId);
		check(objects.resourcePresent && objects.resourceName == "maze0029.mob" &&
			objects.entities.objects.size() > target.recordIndex,
			"Nightshadow original MOB identity mismatch");
		const auto &record = objects.entities.objects[target.recordIndex];
		check(record.x == 4 && record.y == 6 && record.direction == 3 &&
			record.tableIndex == 3 && record.resourceId == 14,
			"Nightshadow gravestone original record mismatch");
		check(assets.readArchiveResource("014.obj").size() == 4546,
			"Nightshadow gravestone sprite identity/bytes mismatch");
		const auto resolver = XeenObjectVisualResolver::load(assets);
		const auto party = XeenPartyLoader().loadInitialCloudsParty(assets);
		const XeenCharacterRulesContext context{kCloudsInitialYear};
		const CloudsMapComposer composer;
		struct View {
			XeenCamera camera;
			int query;
			int order;
			int x;
			int y;
			int scale;
			bool partial;
			bool blocked;
		};
		const View views[] = {
			{{mapId,4,6,XeenDirection::West},  2,149, -5, 2, 0,false,false},
			{{mapId,5,6,XeenDirection::West},  7,125, -7,25, 7,false,false},
			{{mapId,6,6,XeenDirection::West}, 14, 97, -8,50,12,false,false},
			{{mapId,7,6,XeenDirection::West}, 27, 55, -9,58,14,false,false},
			{{mapId,6,7,XeenDirection::West}, 12, 98,-65,50,12,true, false},
			{{mapId,4,8,XeenDirection::South},14, 97, -8,50,12,false,true }
		};
		for (const auto &view : views) {
			std::vector<XeenObjectVisual> diagnostics;
			const auto commands = XeenIndoorScene().build(
				world, view.camera, &resolver, &diagnostics);
			const auto *command = findTarget(commands, target);
			if (view.blocked) {
				check(!command, "W27-blocked Nightshadow view emitted target command");
				const auto full = composer.compose(assets, world, party, view.camera, context,
					&diagnostics);
				check(full.pixels == replay(assets, composer, party, context, commands).pixels,
					"blocked Nightshadow production composition disagreed with command replay");
				continue;
			}
			check(command && command->queryIndex == view.query &&
				command->originalOrder == view.order && command->x == view.x &&
				command->y == view.y && command->sourceMapId == mapId &&
				command->sourceX == 4 && command->sourceY == 6,
				"Nightshadow target command placement mismatch");
			const auto options = command->drawOptions();
			check(options.scaleIndex == view.scale && options.sceneClipped &&
				options.bottomClipped == (view.query == 2) && !options.enlarge &&
				command->object()->visual.identity == target &&
				command->object()->visual.spriteName == "014.obj" &&
				command->object()->visual.frame == 0 &&
				!command->object()->visual.horizontalFlip &&
				command->object()->visual.status == XeenObjectVisualStatus::SupportedStatic,
				"Nightshadow target visual/options mismatch");
			const auto full = composer.compose(assets, world, party, view.camera, context,
				&diagnostics);
			const auto actualReplay = replay(assets, composer, party, context, commands);
			check(full.pixels == actualReplay.pixels,
				"Nightshadow production composition disagreed with ordered replay");
			const auto without = replay(assets, composer, party, context, commands, target);
			std::size_t surviving = 0;
			for (std::size_t pixel = 0; pixel < full.pixels.size(); ++pixel)
				if (full.pixels[pixel] != without.pixels[pixel]) {
					++surviving;
					const auto x = pixel % 320, y = pixel / 320;
					check(x >= 8 && x < 223 && y >= 8 && y < 141,
						"Nightshadow target contribution escaped scene bounds");
				}
			check(surviving > 0, "Nightshadow selected identity has no surviving pixels");
			std::size_t covered = 0;
			if (view.partial) {
				const auto targetLast = replay(
					assets, composer, party, context, commands, target, command);
				for (std::size_t pixel = 0; pixel < full.pixels.size(); ++pixel)
					if (targetLast.pixels[pixel] != full.pixels[pixel] &&
							targetLast.pixels[pixel] != without.pixels[pixel] &&
							full.pixels[pixel] == without.pixels[pixel])
						++covered;
				check(covered > 0,
					"Nightshadow partial view did not expose later-wall target coverage");
			}
			std::cout << "Nightshadow view " << view.camera.x << ',' << view.camera.y
				<< " direction=" << static_cast<unsigned>(view.camera.direction)
				<< " query=" << view.query << " surviving=" << surviving
				<< " covered=" << covered << '\n';
		}

		const auto eventFile = eventLoader.load(mapId);
		check(eventFile.resourcePresent && eventFile.resourceName == "maze0029.evt" &&
			eventFile.records.size() == 524, "Nightshadow event file identity mismatch");
		const auto &display = eventFile.records.at(65);
		const auto &acknowledgment = eventFile.records.at(66);
		check(display.fileOffset == 583 && display.x == 4 && display.y == 6 &&
			display.direction == kXeenEventDirectionAll && display.line == 0 &&
			display.opcode == 0x29 && display.parameters == std::vector<std::uint8_t>{6},
			"Nightshadow DisplayBottom record bytes mismatch");
		check(acknowledgment.fileOffset == 590 && acknowledgment.x == 4 &&
			acknowledgment.y == 6 && acknowledgment.direction == kXeenEventDirectionAll &&
			acknowledgment.line == 1 && acknowledgment.opcode == 0x09 &&
			acknowledgment.parameters == std::vector<std::uint8_t>({44,1,2}),
			"Nightshadow acknowledgment record bytes mismatch");
		const auto textFile = textLoader.load(mapId);
		check(textFile.resourcePresent && textFile.resourceName == "aaze0029.txt" &&
			textFile.strings.size() > 7 && textFile.strings[6] ==
				"\nOnly at night can you put up a fight." &&
			textFile.strings[7] != textFile.strings[6],
			"Nightshadow original clue identity/bytes mismatch");

		for (const auto direction : {XeenDirection::North, XeenDirection::East,
				XeenDirection::South, XeenDirection::West}) {
			XeenWorld eventWorld(mapProvider, objectProvider);
			XeenEventSystem events(scriptProvider, textProvider);
			auto eventParty = party;
			XeenGameFlags flags = XeenGameFlagsLoader().loadInitialCloudsFlags(assets);
			XeenCamera camera{mapId,4,6,direction};
			auto first = suspended(events.runManualEvent(
				eventWorld, eventParty, camera, flags),
				"Nightshadow all-facing DisplayBottom did not suspend");
			check(first.state.selectedObject == target && first.state.instructionCount == 1 &&
				first.request.kind == XeenPresentationKind::BottomWindowMessage &&
				first.request.response == XeenPresentationResponseRequirement::Presented &&
				first.request.textIndex == 6 && first.request.text == textFile.strings[6],
				"Nightshadow DisplayBottom presentation mismatch");
			auto second = suspended(events.resumeManualEvent(std::move(first.state),
				XeenPresentationResponse::Presented, eventWorld, eventParty, camera, flags),
				"Nightshadow Action-44 acknowledgment did not suspend");
			check(second.state.selectedObject == target && second.state.instructionCount == 2 &&
				second.request.kind == XeenPresentationKind::Confirmation &&
				second.request.response == XeenPresentationResponseRequirement::Acknowledgment,
				"Nightshadow acknowledgment presentation mismatch");
			const auto done = completed(events.resumeManualEvent(std::move(second.state),
				XeenPresentationResponse::Acknowledged, eventWorld, eventParty, camera, flags),
				"Nightshadow adjacent absent line did not complete naturally");
			check(done.instructionCount == 2 && !done.cameraChanged && !done.flagsChanged &&
				camera.mapId == mapId && camera.x == 4 && camera.y == 6 &&
				camera.direction == direction && !eventWorld.isObjectDisabled(target),
				"Nightshadow original interaction mutated state");
		}
		{
			XeenWorld negativeWorld(mapProvider, objectProvider);
			XeenEventSystem events(scriptProvider, textProvider);
			auto negativeParty = party;
			XeenGameFlags flags = XeenGameFlagsLoader().loadInitialCloudsFlags(assets);
			for (XeenCamera camera : {XeenCamera{mapId,5,6,XeenDirection::West},
					XeenCamera{mapId,4,8,XeenDirection::South}})
				check(std::holds_alternative<XeenManualEventNoEvent>(events.runManualEvent(
					negativeWorld, negativeParty, camera, flags)),
					"adjacent Nightshadow cell dispatched target event");
			XeenCamera sibling{mapId,4,10,XeenDirection::West};
			const auto other = suspended(events.runManualEvent(
				negativeWorld, negativeParty, sibling, flags),
				"sibling Nightshadow gravestone clue missing");
			check(other.request.textIndex == 7 && other.request.text == textFile.strings[7] &&
				other.request.text != textFile.strings[6],
				"sibling Nightshadow clue aliased target text");
		}

		XeenWorld flowWorld(mapProvider, objectProvider);
		XeenEventSystem flowEvents(scriptProvider, textProvider);
		auto flowParty = party;
		XeenGameFlags flowFlags = XeenGameFlagsLoader().loadInitialCloudsFlags(assets);
		XeenCamera flowCamera{mapId,4,6,XeenDirection::West};
		const XeenFontFormat font(assets.readArchiveResource("fnt"));
		std::uint64_t now = 0;
		std::vector<std::uint64_t> phases;
		std::vector<XeenManualEventResult> reports;
		XeenEventFlow flow(flowWorld, flowEvents, flowParty, flowCamera, flowFlags, font,
			[&](std::uint64_t phase) {
				phases.push_back(phase);
				XeenEventFlow::Composition result;
				result.frame = composer.compose(assets, flowWorld, flowParty, flowCamera,
					context, nullptr, phase, &result.containsOrdinaryAnimation);
				return result;
			}, {}, [&] { return now; });
		flow.reportManual = [&](const auto &result) { reports.push_back(result); };
		const auto base = flow.frame();
		const auto beforeSnapshot = XeenSaveFormat::encode(XeenSaveState::capture(
			{}, flowParty, flowCamera, flowFlags, flowWorld));
		const auto pending = flow.handle(InteractionAction{});
		check(flow.blocksGameplay() && flow.presentationGeneration() && reports.size() == 2 &&
			std::holds_alternative<XeenEventExecutionSuspended>(reports[0]) &&
			std::holds_alternative<XeenEventExecutionSuspended>(reports[1]),
			"Nightshadow Flow did not preserve the two presentation continuations");
		const auto generation = flow.presentationGeneration();
		const auto page = flow.presenter().pageIndex();
		const auto pages = flow.presenter().pageCount();
		std::size_t targetVisibleUnderMessage = 0;
		XeenWorld omittedWorld(mapProvider, objectProvider);
		omittedWorld.disableObject(target);
		const auto omittedBase = composer.compose(
			assets, omittedWorld, flowParty, flowCamera, context);
		for (std::size_t pixel = 0; pixel < base.pixels.size(); ++pixel)
			if (base.pixels[pixel] != omittedBase.pixels[pixel] &&
				pending.pixels[pixel] == base.pixels[pixel])
				++targetVisibleUnderMessage;
		check(targetVisibleUnderMessage > 0,
			"Nightshadow target was not visible beneath pending bottom presentation");
		const auto blockedCamera = flowCamera;
		flow.handle(NavigationAction::MoveForward);
		check(flowCamera.mapId == blockedCamera.mapId && flowCamera.x == blockedCamera.x &&
			flowCamera.y == blockedCamera.y && flowCamera.direction == blockedCamera.direction &&
			flow.presentationGeneration() == generation,
			"pending Nightshadow acknowledgment accepted gameplay movement");
		flowWorld.discardMapCache();
		flowEvents.discardScriptCache();
		flowEvents.discardTextCache();
		assets.discardSpriteCache();
		check(flow.refresh(true).pixels == pending.pixels &&
			flow.presentationGeneration() == generation && flow.presenter().pageIndex() == page &&
			flow.presenter().pageCount() == pages && flow.blocksGameplay(),
			"Nightshadow pending presentation changed during reconstruction");
		now = 1000;
		const auto compositionsBeforeIdle = phases.size();
		check(!flow.updatePresentation() && phases.size() == compositionsBeforeIdle,
			"indoor idle activated ordinary animation composition");
		flow.handle(AcknowledgeAction{});
		check(!flow.blocksGameplay() && !flow.presentationGeneration() && reports.size() == 3 &&
			completed(reports.back(), "Nightshadow Flow did not report completion").instructionCount == 2 &&
			flow.frame().pixels != base.pixels,
			"Nightshadow acknowledgment replayed or prematurely cleared retained text");
		const auto afterSnapshot = XeenSaveFormat::encode(XeenSaveState::capture(
			{}, flowParty, flowCamera, flowFlags, flowWorld));
		check(beforeSnapshot == afterSnapshot,
			"Nightshadow original interaction changed encoded durable state");
		flow.handle(NavigationAction::TurnRight);
		check(!flow.blocksGameplay() && flowCamera.direction == XeenDirection::North &&
			flow.frame().pixels == composer.compose(
				assets, flowWorld, flowParty, flowCamera, context).pixels,
			"next gameplay action did not clear retained Nightshadow text");
		flow.handle(NavigationAction::TurnLeft);
		flow.handle(InteractionAction{});
		check(flow.blocksGameplay(), "repeat Nightshadow interaction did not suspend");
		flow.handle(AcknowledgeAction{});
		check(!flow.blocksGameplay() && reports.size() == 6 &&
			completed(reports.back(), "repeat Nightshadow interaction did not complete")
				.instructionCount == 2,
			"repeat Nightshadow interaction changed instruction path");
		check(std::all_of(phases.begin(), phases.end(),
			[](std::uint64_t phase) { return phase == 0; }),
			"indoor Flow forwarded an advancing outdoor ordinary phase");

		XeenWorld disabledWorld(mapProvider, objectProvider);
		disabledWorld.disableObject(target);
		check(!findTarget(XeenIndoorScene().build(
			disabledWorld, {mapId,4,6,XeenDirection::West}, &resolver), target),
			"object-only disable retained Nightshadow target command");
		XeenEventSystem disabledEvents(scriptProvider, textProvider);
		auto disabledParty = party;
		XeenGameFlags disabledFlags = XeenGameFlagsLoader().loadInitialCloudsFlags(assets);
		XeenCamera disabledCamera{mapId,4,6,XeenDirection::West};
		const auto objectOnly = suspended(disabledEvents.runManualEvent(
			disabledWorld, disabledParty, disabledCamera, disabledFlags),
			"object-only disable incorrectly suppressed independent cell event");
		check(!objectOnly.state.selectedObject && objectOnly.request.textIndex == 6,
			"object-only disabled event selected a hidden object or wrong clue");

		const auto disabledSnapshot = XeenSaveFormat::decode(XeenSaveFormat::encode(
			XeenSaveState::capture({}, disabledParty, disabledCamera, disabledFlags, disabledWorld)));
		XeenWorld restoredWorld(mapProvider, objectProvider);
		XeenPartyState restoredParty;
		XeenCamera restoredCamera;
		XeenGameFlags restoredFlags;
		XeenSaveState::Resources resources{
			{}, [&] { return XeenPartyLoader().loadInitialCloudsParty(assets); },
			[&](XeenMapIdentity id) { return eventLoader.load(id); }};
		XeenSaveState::restoreBeforeGameplay(disabledSnapshot, resources,
			restoredParty, restoredCamera, restoredFlags, restoredWorld,
			[&](XeenWorld &candidateWorld, const XeenPartyState &candidateParty,
					const XeenCamera &candidateCamera, const XeenGameFlags &) {
				check(composer.compose(assets, candidateWorld, candidateParty,
					candidateCamera, context).isValid(),
					"restored indoor disabled identity failed preflight");
			});
		check(restoredWorld.isObjectDisabled(target) &&
			!findTarget(XeenIndoorScene().build(restoredWorld, restoredCamera, &resolver), target),
			"saved indoor disabled identity was resurrected after owner reconstruction");

		XeenWorld removedWorld(mapProvider, objectProvider);
		removedWorld.applyRemove({mapId,4,6,XeenDirection::West}, target, eventFile);
		XeenEventSystem removedEvents(scriptProvider, textProvider);
		auto removedParty = party;
		XeenGameFlags removedFlags = XeenGameFlagsLoader().loadInitialCloudsFlags(assets);
		XeenCamera removedCamera{mapId,4,6,XeenDirection::West};
		const auto removedResult = removedEvents.runManualEvent(
			removedWorld, removedParty, removedCamera, removedFlags);
		check(!std::holds_alternative<XeenEventExecutionSuspended>(removedResult) &&
			removedWorld.isEventDisabled({mapId,65}) &&
			removedWorld.isEventDisabled({mapId,66}) &&
			!findTarget(XeenIndoorScene().build(removedWorld, removedCamera, &resolver), target),
			"applyRemove did not independently suppress object and physical-cell events");

		if (argc == 3) {
			XeenWorld sdlWorld(mapProvider, objectProvider);
			XeenEventSystem sdlEvents(scriptProvider, textProvider);
			auto sdlParty = party;
			XeenGameFlags sdlFlags = XeenGameFlagsLoader().loadInitialCloudsFlags(assets);
			XeenCamera sdlCamera{mapId,4,6,XeenDirection::West};
			std::optional<XeenManualEventResult> terminal;
			XeenEventFlow sdlFlow(sdlWorld, sdlEvents, sdlParty, sdlCamera, sdlFlags, font,
				[&](std::uint64_t phase) {
					XeenEventFlow::Composition result;
					result.frame = composer.compose(assets, sdlWorld, sdlParty, sdlCamera,
						context, nullptr, phase, &result.containsOrdinaryAnimation);
					return result;
				});
			sdlFlow.reportManual = [&](const auto &result) {
				if (!std::holds_alternative<XeenEventExecutionSuspended>(result)) terminal = result;
			};
			const std::array<SDL_Keycode,4> keys = {
				SDLK_SPACE, SDLK_UP, SDLK_RETURN, SDLK_RIGHT};
			std::size_t queued = 0, handled = 0;
			auto pushKey = [](SDL_Keycode key) {
				SDL_Event event{};
				event.type = SDL_KEYDOWN;
				event.key.state = SDL_PRESSED;
				event.key.keysym.sym = key;
				check(SDL_PushEvent(&event) == 1, "failed to queue SDL checkpoint input");
			};
			const bool shown = SdlWindow().showInteractive(sdlFlow.frame(),
				"M23 Nightshadow automated SDL", [&](const PlayerAction &action)
						-> std::optional<IndexedFrame> {
					check(handled < keys.size(), "unexpected extra SDL checkpoint action");
					if (handled == 0)
						check(std::holds_alternative<InteractionAction>(action),
							"SDL Space did not map to interaction");
					if (handled == 1) {
						const auto *navigation = std::get_if<NavigationAction>(&action);
						check(navigation && *navigation == NavigationAction::MoveForward,
							"SDL Up did not map to forward movement");
					}
					if (handled == 2)
						check(std::holds_alternative<AcknowledgeAction>(action),
							"SDL Enter did not map to acknowledgment");
					if (handled == 3) {
						const auto *navigation = std::get_if<NavigationAction>(&action);
						check(navigation && *navigation == NavigationAction::TurnRight,
							"SDL Right did not map to turn");
					}
					const auto frame = sdlFlow.handle(action);
					if (handled < 2)
						check(sdlFlow.blocksGameplay() && sdlCamera.x == 4 &&
							sdlCamera.y == 6 && sdlCamera.direction == XeenDirection::West,
							"SDL pending interaction failed to block gameplay");
					if (handled == 2)
						check(!sdlFlow.blocksGameplay(),
							"SDL acknowledgment did not complete interaction");
					++handled;
					if (handled == keys.size()) {
						SDL_Event quit{};
						quit.type = SDL_QUIT;
						check(SDL_PushEvent(&quit) == 1,
							"failed to queue SDL checkpoint quit");
					}
					return frame;
				}, [&] { return sdlFlow.handlesEscape(); },
				[&]() -> std::optional<IndexedFrame> {
					if (queued == handled && queued < keys.size()) pushKey(keys[queued++]);
					return sdlFlow.updatePresentation();
				});
			check(shown && handled == keys.size() && terminal &&
				completed(*terminal, "SDL Nightshadow interaction did not complete")
					.instructionCount == 2 &&
				sdlCamera.direction == XeenDirection::North && !sdlFlow.blocksGameplay(),
				"Nightshadow production SDL checkpoint failed");
			std::cout << "Nightshadow automated SDL interaction passed\n";
		}

		std::cout << "Nightshadow indoor object, original clue, Flow and persistence passed\n";
		return 0;
	} catch (const std::exception &error) {
		std::cerr << error.what() << '\n';
		return 1;
	}
}
