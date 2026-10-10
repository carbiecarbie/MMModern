#include "app/Application.h"
#include "app/XeenTitleFlow.h"
#include "formats/xeen/XeenCharacterFormat.h"
#include "app/XeenGameplayServices.h"
#include "platform/XeenSaveFile.h"
#include <chrono>
#include <random>
#include "formats/xeen/XeenGameplayContextFormat.h"
#include "formats/xeen/XeenMonsterFormat.h"
#include "app/XeenEventFlow.h"

#include "formats/xeen/XeenAssetSource.h"
#include "formats/xeen/XeenFontFormat.h"
#include "games/xeen/CloudsUiComposer.h"
#include "games/xeen/CloudsMapComposer.h"
#include "games/xeen/XeenEventPresenter.h"
#include "games/xeen/XeenInstallationDetector.h"
#include "games/xeen/XeenEventDiagnostics.h"
#include "games/xeen/XeenEventLoader.h"
#include "games/xeen/XeenEventScript.h"
#include "games/xeen/XeenEventSystem.h"
#include "games/xeen/XeenEventTextLoader.h"
#include "formats/xeen/XeenQuestFlagFormat.h"
#include "games/xeen/XeenEventTrigger.h"
#include "games/xeen/XeenGameFlagsLoader.h"
#include "games/xeen/XeenMapLoader.h"
#include "games/xeen/XeenMovement.h"
#include "games/xeen/XeenCharacterRules.h"
#include "games/xeen/XeenPartyLoader.h"
#include "games/xeen/XeenWorld.h"
#include "platform/sdl/SdlWindow.h"

#include <exception>
#include <functional>
#include <iomanip>
#include <iostream>
#include <sstream>
#include <stdexcept>
#include <variant>
#include <vector>

namespace mmodern {
namespace {

const char *editionName(GameEdition edition) {
	switch (edition) {
	case GameEdition::CloudsOfXeen:
		return "Might & Magic IV: Clouds of Xeen";
	case GameEdition::DarksideOfXeen:
		return "Might & Magic V: Darkside of Xeen";
	case GameEdition::WorldOfXeen:
		return "World of Xeen";
	}

	return "Xeen";
}

const char *directionName(XeenDirection direction) {
	switch (direction) {
	case XeenDirection::North: return "North";
	case XeenDirection::East: return "East";
	case XeenDirection::South: return "South";
	case XeenDirection::West: return "West";
	}
	return "?";
}

const char *blockedReason(XeenMovementResult result) {
	switch (result) {
	case XeenMovementResult::BlockedByMapBoundary: return "map boundary";
	case XeenMovementResult::BlockedByWall: return "impassable wall";
	case XeenMovementResult::BlockedByTerrain: return "impassable terrain";
	case XeenMovementResult::BlockedBySurface: return "impassable surface";
	default: return nullptr;
	}
}

const char *eventErrorName(XeenEventExecutionErrorKind kind) {
	switch (kind) {
	case XeenEventExecutionErrorKind::InvalidInitialCamera: return "InvalidInitialCamera";
	case XeenEventExecutionErrorKind::ScriptLoadFailed: return "ScriptLoadFailed";
	case XeenEventExecutionErrorKind::ScriptMapMismatch: return "ScriptMapMismatch";
	case XeenEventExecutionErrorKind::MalformedInstruction: return "MalformedInstruction";
	case XeenEventExecutionErrorKind::UnsupportedOpcode: return "UnsupportedOpcode";
	case XeenEventExecutionErrorKind::UnsupportedOperand: return "UnsupportedOperand";
	case XeenEventExecutionErrorKind::LineOverflow: return "LineOverflow";
	case XeenEventExecutionErrorKind::UnsupportedConditionAction: return "UnsupportedConditionAction";
	case XeenEventExecutionErrorKind::UnsupportedOperationMode: return "UnsupportedOperationMode";
	case XeenEventExecutionErrorKind::QuestItemOverflow: return "QuestItemOverflow";
	case XeenEventExecutionErrorKind::QuestItemUnderflow: return "QuestItemUnderflow";
	case XeenEventExecutionErrorKind::EmptyParty: return "EmptyParty";
	case XeenEventExecutionErrorKind::InvalidFlagIndex: return "InvalidFlagIndex";
	case XeenEventExecutionErrorKind::InvalidJumpTarget: return "InvalidJumpTarget";
	case XeenEventExecutionErrorKind::InvalidCallTarget: return "InvalidCallTarget";
	case XeenEventExecutionErrorKind::InvalidReturn: return "InvalidReturn";
	case XeenEventExecutionErrorKind::CallStackOverflow: return "CallStackOverflow";
	case XeenEventExecutionErrorKind::UnsupportedTeleportDestination: return "UnsupportedTeleportDestination";
	case XeenEventExecutionErrorKind::ObjectLoadFailed: return "ObjectLoadFailed";
	case XeenEventExecutionErrorKind::InvalidRemoveContext: return "InvalidRemoveContext";
	case XeenEventExecutionErrorKind::MapLoadFailed: return "MapLoadFailed";
	case XeenEventExecutionErrorKind::UnsupportedExecutionContext: return "UnsupportedExecutionContext";
	case XeenEventExecutionErrorKind::InstructionLimitExceeded: return "InstructionLimitExceeded";
	case XeenEventExecutionErrorKind::TextMapMismatch: return "TextMapMismatch";
	case XeenEventExecutionErrorKind::MissingTextResource: return "MissingTextResource";
	case XeenEventExecutionErrorKind::InvalidTextIndex: return "InvalidTextIndex";
	case XeenEventExecutionErrorKind::InvalidPresentationResponse: return "InvalidPresentationResponse";
	case XeenEventExecutionErrorKind::PresentationRequired: return "PresentationRequired";
	case XeenEventExecutionErrorKind::PresentationFailed: return "PresentationFailed";
	}
	return "UnknownEventError";
}

std::string formatEventError(const XeenEventExecutionError &error) {
	std::ostringstream output;
	output << eventErrorName(error.kind) << ": " << error.message
		<< " [logical map=" << error.logicalAddress.mapId
		<< " x=" << error.logicalAddress.x << " y=" << error.logicalAddress.y
		<< " line=" << error.logicalAddress.line << ']';
	if (error.rewards.discarded || error.rewards.count || error.rewards.overflow || error.rewards.invalid)
		output << " [rewards discarded=" << error.rewards.discarded
			<< " reason=" << static_cast<unsigned>(error.rewards.discardReason)
			<< " delivered=" << error.rewards.delivered << " lost=" << error.rewards.lost
			<< " overflow=" << error.rewards.overflow << " invalid=" << error.rewards.invalid << ']';
	if (error.source) {
		output << " [source";
		if (error.source->resourceName)
			output << " resource=" << *error.source->resourceName;
		output << " offset=" << error.source->fileOffset
			<< " opcode=0x" << std::uppercase << std::hex
			<< static_cast<unsigned>(error.source->opcode) << std::dec
			<< ' ' << XeenEventDiagnostics::opcodeName(error.source->opcode) << ']';
	}
	if (error.requestedTarget) {
		output << " [target map=" << error.requestedTarget->mapId
			<< " x=" << error.requestedTarget->x
			<< " y=" << error.requestedTarget->y
			<< " line=" << error.requestedTarget->line << ']';
	}
	return output.str();
}

void requireAutomaticEventSuccess(const XeenAutomaticEventResult &result) {
	if (const auto *error = std::get_if<XeenEventExecutionError>(&result))
		throw std::runtime_error("Automatic event: " + formatEventError(*error));
}

void printManualEventResult(const XeenManualEventResult &result) {
	if (std::holds_alternative<XeenManualEventNoEvent>(result)) {
		std::cout << "Interaction: no event at this position and direction.\n";
	} else if (const auto *completed =
			std::get_if<XeenManualEventCompleted>(&result)) {
		std::cout << "Interaction: event completed ("
			<< completed->instructionCount << " instructions).\n";
	} else if (const auto *special =
			std::get_if<XeenManualSpecialInteractionUnsupported>(&result)) {
		std::cout << "Special interaction not supported yet (wall "
			<< static_cast<unsigned>(special->wallValue) << ").\n";
	} else if (const auto *pending =
			std::get_if<XeenEventExecutionSuspended>(&result)) {
		std::cout << "Interaction: semantic presentation pending (text ";
		if (pending->request.textIndex)
			std::cout << static_cast<unsigned>(*pending->request.textIndex);
		else
			std::cout << "no index";
		std::cout << ").\n";
	} else if (const auto *error = std::get_if<XeenEventExecutionError>(&result)) {
		std::cout << "Interaction: " << formatEventError(*error) << '\n';
	}
}

void printPartyDiagnostics(const XeenPartyState &state) {
	for (const std::string &diagnostic : state.diagnostics)
		std::cerr << "Warning: " << diagnostic << '\n';
}

} // namespace

int Application::newGame(const std::filesystem::path &gameDirectory,
        XeenDifficulty difficulty, std::optional<std::filesystem::path> savePath) const {
    return gameplay(gameDirectory, {}, savePath, false, XeenEncounterEntry::Journey, {}, difficulty);
}

int Application::inspectParty(const std::filesystem::path &gameDirectory) const {
	try {
		const auto installation = XeenInstallationDetector(_uiData).detect(gameDirectory);
		if (!installation) {
			std::cerr << "No Xeen installation found: " << gameDirectory.string() << '\n';
			return 2;
		}
		if (!installation->hasXeen()) {
			std::cerr << "Initial party inspection requires XEEN.CC (Clouds).\n";
			return 3;
		}

		// Resource parsing only: this path creates no framebuffer or SDL window.
		std::cout << "Data source: " << installation->sourceOrigin << '\n';
        XeenAssetSource assets(*installation);
		const XeenPartyState state = XeenPartyLoader().loadInitialCloudsParty(assets);
		printPartyDiagnostics(state);
		const XeenCharacterRulesContext rulesContext{kCloudsInitialYear};
		std::cout << "Initial Clouds party: " << state.party.size() << " members\n";
		for (std::size_t i = 0; i < state.party.size(); ++i) {
			const XeenCharacter &character = state.party.member(state.roster, i);
			const auto portrait = character.portraitResourceName();
			const int maxHp = XeenCharacterRules::maxHp(character, rulesContext);
			const int maxSp = XeenCharacterRules::maxSp(character, rulesContext);
			std::cout << "\n" << (i + 1) << ". " << character.name << '\n'
				<< "   Roster: " << static_cast<unsigned>(character.rosterId) << '\n'
				<< "   Portrait: " << (portrait ? *portrait : "no supported portrait") << '\n'
				<< "   Class: " << xeenClassName(character.characterClass) << '\n'
				<< "   Level: " << character.currentLevel() << '\n'
				<< "   HP: " << character.currentHp << " / " << maxHp << '\n'
				<< "   SP: " << character.currentSp << " / " << maxSp << '\n'
				<< "   Condition: " << xeenConditionName(character.worstCondition()) << '\n';
		}
		return 0;
	} catch (const std::exception &error) {
		std::cerr << "Party inspection failed: " << error.what() << '\n';
		return 3;
	}
}

int Application::inspectMap(const std::filesystem::path &gameDirectory,
		std::uint16_t mapId) const {
	try {
		const auto installation = XeenInstallationDetector(_uiData).detect(gameDirectory);
		if (!installation) {
			std::cerr << "No Xeen installation found: " << gameDirectory.string() << '\n';
			return 2;
		}
		if (!installation->hasXeen()) {
			std::cerr << "Map inspection requires XEEN.CC (Clouds).\n";
			return 3;
		}
		// No framebuffer, composer, SDL initialization, or window in this path.
		std::cout << "Data source: " << installation->sourceOrigin << '\n';
        XeenAssetSource assets(*installation);
		const XeenMap map = XeenMapLoader().loadGeometryMap(assets, mapId);
		const auto &geometry = map.geometry;
		std::cout << "Source: xeen.cc, initial Clouds archive\n"
			<< "Loaded map: " << std::setfill('0') << std::setw(3) << geometry.id
			<< std::setfill(' ') << '\n'
			<< "Dimensions: " << geometry.kWidth << 'x' << geometry.kHeight << '\n'
			<< "Type: " << (geometry.isOutdoors() ? "outdoor" : "indoor") << '\n'
			<< "Cells: " << geometry.cells.size() << '\n'
			<< "Flags: 0x" << std::hex << std::setw(4) << std::setfill('0') << geometry.flags << '\n'
			<< "Flags2: 0x" << std::setw(4) << geometry.flags2 << std::dec << std::setfill(' ') << '\n'
			<< "Dark: " << ((geometry.flags2 & 0x4000) ? "yes" : "no") << '\n'
			<< "Wall kind: " << static_cast<unsigned>(geometry.wallKind) << '\n'
			<< "Floor type: " << static_cast<unsigned>(geometry.floorType) << '\n'
			<< "Wall no-pass: " << static_cast<unsigned>(geometry.difficulties[0]) << '\n';
		const char *const directions[] = {"north", "east", "south", "west"};
		for (std::size_t i = 0; i < geometry.neighbors.size(); ++i) {
			std::cout << "Neighbor " << directions[i] << ": ";
			if (geometry.neighbors[i])
				std::cout << std::setfill('0') << std::setw(3) << geometry.neighbors[i] << std::setfill(' ');
			else
				std::cout << "none";
			std::cout << '\n';
		}
		std::cout << "Wall types (indices):\n";
		for (std::size_t i = 0; i < geometry.wallTypes.size(); ++i)
			std::cout << std::hex << i << std::dec << "->" << static_cast<unsigned>(geometry.wallTypes[i]) << ' ';
		std::cout << "\nSurface types (indices):\n";
		for (std::size_t i = 0; i < geometry.surfaceTypes.size(); ++i)
			std::cout << std::hex << i << std::dec << "->" << static_cast<unsigned>(geometry.surfaceTypes[i]) << ' ';
		std::cout << '\n';

		std::cout << (geometry.isOutdoors() ? "\nSurfaces" : "\nWalls N/E/S/W")
			<< ": hexadecimal; north upward\n    x:";
		for (std::size_t x = 0; x < geometry.kWidth; ++x)
			std::cout << ' ' << std::hex << x;
		std::cout << std::dec << '\n';
		for (int y = static_cast<int>(geometry.kHeight) - 1; y >= 0; --y) {
			std::cout << "y=" << std::setw(2) << y << " :";
			for (std::size_t x = 0; x < geometry.kWidth; ++x) {
				const auto &cell = geometry.cells[static_cast<std::size_t>(y) * geometry.kWidth + x];
				if (geometry.isOutdoors()) {
					std::cout << ' ' << std::hex << static_cast<unsigned>(cell.surfaceIndex);
				} else {
					std::cout << ' ' << std::hex
						<< static_cast<unsigned>(wallAt(cell, XeenDirection::North))
						<< '/' << static_cast<unsigned>(wallAt(cell, XeenDirection::East))
						<< '/' << static_cast<unsigned>(wallAt(cell, XeenDirection::South))
						<< '/' << static_cast<unsigned>(wallAt(cell, XeenDirection::West));
				}
			}
			std::cout << std::dec << '\n';
		}
		return 0;
	} catch (const std::exception &error) {
		std::cerr << "Map inspection failed: " << error.what() << '\n';
		return 3;
	}
}

int Application::inspectEvents(const std::filesystem::path &gameDirectory,
		std::uint16_t mapId, std::optional<std::uint8_t> x,
		std::optional<std::uint8_t> y,
		std::optional<XeenDirection> direction, bool allOnly) const {
	try {
		const auto installation = XeenInstallationDetector(_uiData).detect(gameDirectory);
		if (!installation) {
			std::cerr << "No Xeen installation found: "
				<< gameDirectory.string() << '\n';
			return 2;
		}
		if (!installation->hasXeen()) {
			std::cerr << "Event inspection requires XEEN.CC (Clouds).\n";
			return 3;
		}

		// Resource diagnostics only: no map geometry, framebuffer, or SDL window.
		std::cout << "Data source: " << installation->sourceOrigin << '\n';
        XeenAssetSource assets(*installation);
		const XeenEventLoader loader([&assets](const std::string &resourceName)
				-> std::optional<std::vector<std::uint8_t>> {
			if (!assets.hasInitialResource(resourceName))
				return std::nullopt;
			return assets.readInitialResource(resourceName);
		});
		XeenEventScript script(loader.load(mapId));
		if (x.has_value() != y.has_value() ||
				(!x && (direction || allOnly)) || (direction && allOnly))
			throw std::invalid_argument("inconsistent Event filter");
		if (!x && !y) {
			std::cout << XeenEventDiagnostics::format(script);
			return 0;
		}
		XeenEventDiagnosticFilter filter;
		filter.x = *x;
		filter.y = *y;
		if (allOnly) {
			filter.directionFilter = XeenEventDirectionFilter::AllOnly;
		} else if (direction) {
			filter.directionFilter = XeenEventDirectionFilter::Physical;
			filter.direction = *direction;
		}
		if (*x < XeenMapGeometry::kWidth && *y < XeenMapGeometry::kHeight) {
			const XeenMap map = XeenMapLoader().loadGeometryMap(assets, mapId);
			filter.automatic = hasAutomaticTrigger(map.geometry, *x, *y);
		}
		std::cout << XeenEventDiagnostics::format(script, filter);
		return 0;
	} catch (const std::exception &error) {
		std::cerr << "Event inspection failed: " << error.what() << '\n';
		return 3;
	}
}

int Application::renderMap(const std::filesystem::path &gameDirectory,
        std::uint16_t mapId, int x, int y, XeenDirection direction) const {
    return gameplay(gameDirectory, {mapId, x, y, direction}, {}, false);
}
int Application::loadGame(const std::filesystem::path &gameDirectory,
        const std::filesystem::path &savePath) const {
    return gameplay(gameDirectory, {}, savePath, true);
}

int Application::gameplay(const std::filesystem::path &gameDirectory, XeenCamera camera,
        const std::optional<std::filesystem::path> &savePath, bool resume, XeenEncounterEntry entry,
        std::optional<std::uint32_t> seed, std::optional<XeenDifficulty> originalStart,
        const XeenSessionEntry *managed, XeenAssetSource *sharedAssets,InitialPublication publication,XeenSessionOutcome *outcome) const {
    try {
        if (originalStart && (resume || entry != XeenEncounterEntry::Journey || seed ||
                static_cast<unsigned>(*originalStart) > 1))
            throw std::invalid_argument("Invalid new-game configuration");
        if (entry != XeenEncounterEntry::Ordinary && resume)
            throw std::invalid_argument("Encounter entry cannot load or configure a save");
        const auto installation = XeenInstallationDetector(_uiData).detect(gameDirectory);
        if (!installation) {
            std::cerr << "No Xeen installation found: " << gameDirectory.u8string() << '\n';
            return 2;
        }
        if (!installation->hasXeen())
            throw std::runtime_error("Clouds gameplay requires an installation containing xeen.cc");
        if (entry != XeenEncounterEntry::Ordinary && !installation->hasDarkside())
            throw std::runtime_error("Encounter entry requires World of Xeen");
        std::optional<std::filesystem::path> target;
        if (savePath) target = XeenSaveFile::resolve(*savePath, *installation);
        XeenSaveResourceSignature signature;
        {
            const auto begin = std::chrono::steady_clock::now();
            signature = XeenSaveFile::fingerprint(*installation);
            std::cout << "Archive fingerprints: "
                << std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now() - begin).count()
                << " ms\n";
        }
        std::cout << "Data source: " << installation->sourceOrigin << '\n';
        std::unique_ptr<XeenAssetSource> ownedAssets;
        if(!sharedAssets)ownedAssets=std::make_unique<XeenAssetSource>(*installation,CloudsUiComposer::kWidth,CloudsUiComposer::kHeight);
        auto &assets=sharedAssets?*sharedAssets:*ownedAssets;
        const XeenMapLoader maps;
        const XeenEventLoader events([&](const std::string &name) -> std::optional<std::vector<std::uint8_t>> {
            if (!assets.hasInitialResource(name)) return std::nullopt;
            return assets.readInitialResource(name);
        });
        const XeenEventTextLoader texts([&](const std::string &name) -> std::optional<std::vector<std::uint8_t>> {
            if (!assets.hasArchiveResource(name)) return std::nullopt;
            return assets.readArchiveResource(name);
        });
        if (!assets.hasArchiveResource("fnt")) throw std::runtime_error("Missing Xeen font resource 'fnt'");
        const XeenFontFormat font(assets.readArchiveResource("fnt"));
        const CloudsMapComposer composer;
        bool journeyControls = entry == XeenEncounterEntry::Journey;
        const auto catalog = loadXeenItemCatalog(assets);
        if (!catalog.diagnostic.empty()) std::cerr << "Item catalog: " << catalog.diagnostic << '\n';
        XeenGameplayServices services{
            {signature, [&] { return XeenPartyLoader().loadInitialCloudsParty(assets); },
                [&](XeenMapIdentity id) { return events.load(id); }},
            [&] { return XeenGameFlagsLoader().loadInitialCloudsFlags(assets); },
            [&](XeenMapIdentity id) { return maps.loadGeometryMap(assets, id); },
            [&](XeenMapIdentity id) { return maps.loadObjects(assets, id); },
            [&](XeenMapIdentity id) { return texts.load(id); }, font,
            [&](XeenWorld &world, const XeenPartyState &party, const XeenCamera &position, std::uint64_t phase) {
                XeenEventFlow::Composition result;
                result.frame = composer.compose(assets, world, party, position, {kCloudsInitialYear},
                    nullptr, phase, &result.containsOrdinaryAnimation);
                return result;
            },
            [&](IndexedFrame &frame, std::uint8_t portrait, std::size_t index) { assets.drawNpc(frame, portrait, index); },
            [&](XeenEventFlow &flow, const XeenCamera &position) {
                journeyControls = journeyControls || flow.journey();
                flow.rebuildEncounterPresentation = [&] { assets.discardSpriteCache(); };
                flow.dialogText = &assets.uiText();
				flow.drawSmithArt = [&](IndexedFrame &frame) { assets.drawSmith(frame); };
				flow.drawTrainingArt = [&](IndexedFrame &frame) { assets.drawTraining(frame); };
				flow.drawTempleArt = [&](IndexedFrame &frame) { assets.drawTemple(frame); };
                flow.drawCombatButtons = [&](IndexedFrame &frame) { CloudsUiComposer().drawCombatButtons(assets,frame); };
                flow.drawDialogSprite = [&](IndexedFrame &frame,const char *name,unsigned index,int x,int y) { assets.drawDialogSprite(frame,name,index,x,y); };
                flow.loadRestDream = [&] { return assets.restDreamImage(); };
                flow.reportManual = printManualEventResult;
                flow.reportAutomatic = requireAutomaticEventSuccess;
                flow.reportText = [](const std::string &message) { std::cerr << "Text warning: " << message << '\n'; };
                flow.reportMovement = [&position](XeenMovementResult result) {
                    if (const char *reason = blockedReason(result))
                        std::cout << "Movement blocked: " << reason << ". Camera: map " << position.mapId
                            << " X=" << position.x << " Y=" << position.y << ' ' << directionName(position.direction) << '\n';
                };
            },
            [&](const IndexedFrame &first, const auto &handler, const auto &escape, const auto &idle, const auto &status) {
                if (journeyControls) {} // The shared startup prints the bounded Journey controls.
                else std::cout << "Controls: Up/Down move, Left/Right turn, Space interacts, Enter acknowledges, "
                    "Y/N answers, F1-F6 selects, I opens inventory, 1-9 selects a slot, T transfers, "
                    "Escape closes/cancels or exits. Map exploration cannot save.\n";
                auto nativeHandler=handler;
                nativeHandler.cursorImage=[&,current=handler.frameCurrent,palette=first.palette] {
                    if(current && !current())return IndexedFrame{};
                    auto cursor=assets.cursorImage();
                    cursor.palette=palette; // Title retry UI may have used the shared asset surface.
                    return cursor;
                };
                return SdlWindow().showInteractive(first, status(), nativeHandler, escape, idle, status);
            }
        };
        services.catalog = &catalog.catalog;
        services.originalStart = originalStart;
        if(!managed)services.developerSavePath=target;
        std::optional<std::filesystem::path> slotDirectory;
        const std::filesystem::path repository=MMODERN_REPOSITORY_ROOT;
        const auto directory=[&]() -> const std::filesystem::path & {
            if(!slotDirectory)slotDirectory=XeenSaveFile::createSlotDirectory(*installation,repository);
            return *slotDirectory;
        };
        const auto slotPath=[&](unsigned slot) {
            return XeenSaveFile::resolve(XeenSaveFile::resolve(XeenSaveFile::slotPath(directory(),slot),*installation),repository);
        };
        services.panel=[&](const IndexedFrame &base,bool combat,bool restricted,bool saveable,
                std::optional<unsigned> current,const std::string &name) {
            const auto &dos=assets.uiText();
            const auto resource=std::string(dos.scalar("PANEL_SPRITES"));
            if(assets.spriteFrameCount(resource)!=2)throw std::runtime_error("Unexpected DOS control-panel sprite frames");
            for(unsigned i=0;i<2;++i){auto scratch=base;assets.drawDialogSprite(scratch,resource.c_str(),i,0,0);}
            XeenTitleFlow::Services presentation{dos,font,base,{}, {},
                [&](IndexedFrame &frame,const char *resource,unsigned index,int x,int y){assets.drawDialogSprite(frame,resource,index,x,y);},
                [&] {
                    std::array<XeenSaveFile::Slot,10> slots;
                    for(unsigned i=0;i<10;++i)slots[i]=XeenSaveFile::inspectSlot(slotPath(i),signature);
                    return slots;
                },slotPath};
            presentation.panel=true;presentation.combat=combat;presentation.saveRestricted=restricted;
            presentation.saveable=saveable;presentation.currentSlot=current;presentation.currentName=name;
            return std::make_unique<XeenTitleFlow>(std::move(presentation));
        };
        services.writeManaged=[&](unsigned slot,const auto &snapshot,const auto &check) {
            check();XeenSaveFile::writeSlot(directory(),slot,snapshot,*installation,repository,{},check);check();
        };
        if(managed) {
            services.restoreSnapshot=managed->snapshot;
            services.saveName=managed->name;
            services.initialSlot=managed->slot;
            services.publishInitial=std::move(publication);
        }
        services.loadInitialCamera = [&] { return XeenCharacterFormat::parsePartyLocation(assets.readInitialResource("maze.pty")); };
        services.resources.loadInitialCharacters = [&] { return assets.readInitialResource("maze.chr"); };
        services.resources.regionalManifest = [&](const XeenMap &map,const XeenObjectFile &mob,const XeenEventFile &evt,const std::vector<XeenMonsterRecord> &mon) {
            xeenValidateRegionalManifest(map,mob,evt,mon,assets.readInitialResource("maze0023.dat"),
                assets.readInitialResource("maze0023.mob"),assets.readInitialResource("maze0023.evt"));
        };
        services.resources.loadInitialPurse = [&] { return XeenCharacterFormat::parseMonsterPurse(assets.readInitialResource("maze.pty")); };
        services.resources.loadInitialBankBalances = [&] { return XeenCharacterFormat::parseBankBalances(assets.readInitialResource("maze.pty")); };
        services.resources.loadInitialRegionalRecovery = [&] { return XeenQuestFlagFormat::parseRegionalRecovery(assets.readInitialResource("maze.pty")); };
        services.resources.loadInitialContext = [&] { return XeenGameplayContextFormat::parse(assets.readInitialResource("maze.pty")); };
        services.resources.loadMonsterStatistics = [&] {
            const auto bytes = assets.readCloudsMonsterStatisticsFromDarkArchive();
            if (!bytes) throw std::runtime_error("Missing DARK.CC/xeen.mon");
            return XeenMonsterFormat::parse(*bytes);
        };
		services.resources.loadLearnedSpellNames = [&] {
			const auto bytes=assets.readLearnedSpellNamesFromDarkArchive();
			if (!bytes) throw std::runtime_error("Missing DARK.CC/spells.xen");
			return XeenLearnedSpellNames::parse(*bytes);
		};

        services.validateEncounterSprite = [&](std::uint8_t image) { assets.validateNormalMonster(image); };
        services.validateCombatSprite = [&](std::uint8_t image) { assets.validateAttackMonster(image); };
        services.composeEncounter = [&](XeenWorld &w, const XeenPartyState &p, const XeenCamera &c,
                std::uint64_t ordinary, XeenMonsterAppearance actor) {
            XeenEventFlow::Composition result;
            result.frame = composer.compose(assets, w, p, c, {p.encounterContext ? std::uint32_t(p.encounterContext->year) : kCloudsInitialYear}, nullptr, ordinary,
                &result.containsOrdinaryAnimation, actor);
            return result;
        };
        // Explicit developer entries also restart from the retained validated
        // value, after playGameplay has destroyed the old Flow and owners.
        for(;;) {
            XeenSessionOutcome result;services.outcome=&result;
            const int status=playGameplay(services,camera,target,resume,entry,entry==XeenEncounterEntry::Journey?seed:std::nullopt);
            if(status || result.kind!=XeenSessionOutcome::Kind::Load)return status;
            if(outcome){*outcome=std::move(result);return 0;}
            services.restoreSnapshot=result.entry.snapshot;services.initialSlot=result.entry.slot;
            services.publishInitial={};services.originalStart.reset();seed.reset();
            target=result.entry.path;resume=true;entry=XeenEncounterEntry::Ordinary;
        }
    } catch (const std::exception &error) {
        std::cerr << "Gameplay startup failed";
        if (savePath) std::cerr << " [" << std::filesystem::absolute(*savePath).u8string() << ']';
        std::cerr << ": " << error.what() << '\n';
        return 3;
    }
}

} // namespace mmodern
