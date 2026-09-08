#include "app/Application.h"
#include "app/XeenGameplayServices.h"
#include "platform/XeenSaveFile.h"
#include <chrono>
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
	case XeenDirection::North: return "Norte";
	case XeenDirection::East: return "Leste";
	case XeenDirection::South: return "Sul";
	case XeenDirection::West: return "Oeste";
	}
	return "?";
}

const char *blockedReason(XeenMovementResult result) {
	switch (result) {
	case XeenMovementResult::BlockedByMapBoundary: return "limite do mapa";
	case XeenMovementResult::BlockedByWall: return "parede intransponivel";
	case XeenMovementResult::BlockedByTerrain: return "terreno intransponivel";
	case XeenMovementResult::BlockedBySurface: return "superficie intransponivel";
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
		throw std::runtime_error("evento automatico: " + formatEventError(*error));
}

void printManualEventResult(const XeenManualEventResult &result) {
	if (std::holds_alternative<XeenManualEventNoEvent>(result)) {
		std::cout << "Interacao: nenhum evento nesta posicao e direcao.\n";
	} else if (const auto *completed =
			std::get_if<XeenManualEventCompleted>(&result)) {
		std::cout << "Interacao: evento concluido ("
			<< completed->instructionCount << " instrucoes).\n";
	} else if (const auto *special =
			std::get_if<XeenManualSpecialInteractionUnsupported>(&result)) {
		std::cout << "Interacao especial ainda nao suportada (parede "
			<< static_cast<unsigned>(special->wallValue) << ").\n";
	} else if (const auto *pending =
			std::get_if<XeenEventExecutionSuspended>(&result)) {
		std::cout << "Interacao: apresentacao semantica pendente (texto ";
		if (pending->request.textIndex)
			std::cout << static_cast<unsigned>(*pending->request.textIndex);
		else
			std::cout << "sem indice";
		std::cout << ").\n";
	} else if (const auto *error = std::get_if<XeenEventExecutionError>(&result)) {
		std::cout << "Interacao: " << formatEventError(*error) << '\n';
	}
}

void printPartyDiagnostics(const XeenPartyState &state) {
	for (const std::string &diagnostic : state.diagnostics)
		std::cerr << "Aviso: " << diagnostic << '\n';
}

} // namespace

int Application::run(const std::filesystem::path &gameDirectory) const {
	const XeenInstallationDetector detector;
	const std::optional<GameInstallation> installation = detector.detect(gameDirectory);
	if (!installation) {
		std::cerr << "Nenhuma instalacao de Xeen encontrada em: "
			<< gameDirectory.string() << '\n';
		return 2;
	}

	std::cout << "Instalacao detectada: " << editionName(installation->edition) << '\n';
	if (!installation->hasXeen()) {
		std::cerr << "A composicao inicial do MMModern requer xeen.cc.\n";
		return 3;
	}

	try {
		XeenAssetSource assets(*installation, CloudsUiComposer::kWidth,
			CloudsUiComposer::kHeight);
		const XeenPartyState partyState = XeenPartyLoader().loadInitialCloudsParty(assets);
		printPartyDiagnostics(partyState);
		const XeenCharacterRulesContext rulesContext{kCloudsInitialYear};
		const CloudsUiComposer composer;
		const IndexedFrame frame = composer.compose(assets, partyState, rulesContext);

		std::cout << "Interface estatica de Clouds of Xeen composta com sucesso.\n";
		SdlWindow window;
		return window.show(frame, "MMModern - Clouds of Xeen") ? 0 : 4;
	} catch (const std::exception &error) {
		std::cerr << "Falha ao iniciar MMModern: " << error.what() << '\n';
		return 3;
	}
}

int Application::inspectParty(const std::filesystem::path &gameDirectory) const {
	try {
		const auto installation = XeenInstallationDetector().detect(gameDirectory);
		if (!installation) {
			std::cerr << "Nenhuma instalacao de Xeen encontrada em: " << gameDirectory.string() << '\n';
			return 2;
		}
		if (!installation->hasXeen()) {
			std::cerr << "A inspecao da Party inicial requer xeen.cc (Clouds).\n";
			return 3;
		}

		// Resource parsing only: this path creates no framebuffer or SDL window.
		XeenAssetSource assets(*installation);
		const XeenPartyState state = XeenPartyLoader().loadInitialCloudsParty(assets);
		printPartyDiagnostics(state);
		const XeenCharacterRulesContext rulesContext{kCloudsInitialYear};
		std::cout << "Party inicial de Clouds: " << state.party.size() << " membros\n";
		for (std::size_t i = 0; i < state.party.size(); ++i) {
			const XeenCharacter &character = state.party.member(state.roster, i);
			const auto portrait = character.portraitResourceName();
			const int maxHp = XeenCharacterRules::maxHp(character, rulesContext);
			const int maxSp = XeenCharacterRules::maxSp(character, rulesContext);
			std::cout << "\n" << (i + 1) << ". " << character.name << '\n'
				<< "   Roster: " << static_cast<unsigned>(character.rosterId) << '\n'
				<< "   Retrato: " << (portrait ? *portrait : "sem retrato suportado") << '\n'
				<< "   Classe: " << xeenClassName(character.characterClass) << '\n'
				<< "   Nivel: " << character.currentLevel() << '\n'
				<< "   HP: " << character.currentHp << " / " << maxHp << '\n'
				<< "   SP: " << character.currentSp << " / " << maxSp << '\n'
				<< "   Condicao: " << xeenConditionName(character.worstCondition()) << '\n';
		}
		return 0;
	} catch (const std::exception &error) {
		std::cerr << "Falha ao inspecionar Party: " << error.what() << '\n';
		return 3;
	}
}

int Application::inspectMap(const std::filesystem::path &gameDirectory,
		std::uint16_t mapId) const {
	try {
		const auto installation = XeenInstallationDetector().detect(gameDirectory);
		if (!installation) {
			std::cerr << "Nenhuma instalacao de Xeen encontrada em: " << gameDirectory.string() << '\n';
			return 2;
		}
		if (!installation->hasXeen()) {
			std::cerr << "A inspecao de mapas requer xeen.cc (Clouds).\n";
			return 3;
		}
		// No framebuffer, composer, SDL initialization, or window in this path.
		XeenAssetSource assets(*installation);
		const XeenMap map = XeenMapLoader().loadGeometryMap(assets, mapId);
		const auto &geometry = map.geometry;
		std::cout << "Origem: xeen.cc, conteiner inicial de Clouds\n"
			<< "Mapa carregado: " << std::setfill('0') << std::setw(3) << geometry.id
			<< std::setfill(' ') << '\n'
			<< "Dimensoes: " << geometry.kWidth << 'x' << geometry.kHeight << '\n'
			<< "Tipo: " << (geometry.isOutdoors() ? "exterior" : "interior") << '\n'
			<< "Celulas: " << geometry.cells.size() << '\n'
			<< "Flags: 0x" << std::hex << std::setw(4) << std::setfill('0') << geometry.flags << '\n'
			<< "Flags2: 0x" << std::setw(4) << geometry.flags2 << std::dec << std::setfill(' ') << '\n'
			<< "Escuro: " << ((geometry.flags2 & 0x4000) ? "sim" : "nao") << '\n'
			<< "Wall kind: " << static_cast<unsigned>(geometry.wallKind) << '\n'
			<< "Floor type: " << static_cast<unsigned>(geometry.floorType) << '\n'
			<< "Wall no-pass: " << static_cast<unsigned>(geometry.difficulties[0]) << '\n';
		const char *const directions[] = {"norte", "leste", "sul", "oeste"};
		for (std::size_t i = 0; i < geometry.neighbors.size(); ++i) {
			std::cout << "Vizinho " << directions[i] << ": ";
			if (geometry.neighbors[i])
				std::cout << std::setfill('0') << std::setw(3) << geometry.neighbors[i] << std::setfill(' ');
			else
				std::cout << "nenhum";
			std::cout << '\n';
		}
		std::cout << "Wall types (indices):\n";
		for (std::size_t i = 0; i < geometry.wallTypes.size(); ++i)
			std::cout << std::hex << i << std::dec << "->" << static_cast<unsigned>(geometry.wallTypes[i]) << ' ';
		std::cout << "\nSurface types (indices):\n";
		for (std::size_t i = 0; i < geometry.surfaceTypes.size(); ++i)
			std::cout << std::hex << i << std::dec << "->" << static_cast<unsigned>(geometry.surfaceTypes[i]) << ' ';
		std::cout << '\n';

		std::cout << (geometry.isOutdoors() ? "\nSuperficies" : "\nParedes N/E/S/W")
			<< ": hexadecimal; norte para cima\n    x:";
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
		std::cerr << "Falha ao inspecionar mapa: " << error.what() << '\n';
		return 3;
	}
}

int Application::inspectEvents(const std::filesystem::path &gameDirectory,
		std::uint16_t mapId, std::optional<std::uint8_t> x,
		std::optional<std::uint8_t> y,
		std::optional<XeenDirection> direction, bool allOnly) const {
	try {
		const auto installation = XeenInstallationDetector().detect(gameDirectory);
		if (!installation) {
			std::cerr << "Nenhuma instalacao de Xeen encontrada em: "
				<< gameDirectory.string() << '\n';
			return 2;
		}
		if (!installation->hasXeen()) {
			std::cerr << "A inspecao de eventos requer xeen.cc (Clouds).\n";
			return 3;
		}

		// Resource diagnostics only: no map geometry, framebuffer, or SDL window.
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
			throw std::invalid_argument("filtro de eventos inconsistente");
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
		std::cerr << "Falha ao inspecionar eventos: " << error.what() << '\n';
		return 3;
	}
}

int Application::renderMap(const std::filesystem::path &gameDirectory,
        std::uint16_t mapId, int x, int y, XeenDirection direction,
        std::optional<std::filesystem::path> savePath) const {
    return gameplay(gameDirectory, {mapId, x, y, direction}, savePath, false);
}
int Application::loadGame(const std::filesystem::path &gameDirectory,
        const std::filesystem::path &savePath) const {
    return gameplay(gameDirectory, {}, savePath, true);
}
int Application::gameplay(const std::filesystem::path &gameDirectory, XeenCamera camera,
        const std::optional<std::filesystem::path> &savePath, bool resume) const {
    try {
        const auto installation = XeenInstallationDetector().detect(gameDirectory);
        if (!installation) {
            std::cerr << "No Xeen installation found: " << gameDirectory.u8string() << '\n';
            return 2;
        }
        if (!installation->hasXeen())
            throw std::runtime_error("Clouds gameplay requires an installation containing xeen.cc");
        std::optional<std::filesystem::path> target;
        if (savePath) target = XeenSaveFile::resolve(*savePath, installation->root);
        XeenSaveResourceSignature signature;
        if (target) {
            const auto begin = std::chrono::steady_clock::now();
            signature = XeenSaveFile::fingerprint(*installation);
            std::cout << "Archive fingerprints: "
                << std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now() - begin).count()
                << " ms\n";
        }
        XeenAssetSource assets(*installation, CloudsUiComposer::kWidth, CloudsUiComposer::kHeight);
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
        XeenGameplayServices services{
            {signature, [&] { return XeenPartyLoader().loadInitialCloudsParty(assets); },
                [&](XeenMapIdentity id) { return events.load(id); }},
            [&] { return XeenGameFlagsLoader().loadInitialCloudsFlags(assets); },
            [&](XeenMapIdentity id) { return maps.loadGeometryMap(assets, id); },
            [&](XeenMapIdentity id) { return maps.loadObjects(assets, id); },
            [&](XeenMapIdentity id) { return texts.load(id); }, font,
            [&](XeenWorld &world, const XeenPartyState &party, const XeenCamera &position) {
                return composer.compose(assets, world, party, position, {kCloudsInitialYear});
            },
            [&](IndexedFrame &frame, std::uint8_t portrait, std::size_t index) { assets.drawNpc(frame, portrait, index); },
            [&](XeenEventFlow &flow, const XeenCamera &position) {
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
                std::cout << "Controls: W/S move, A/D turn, Space interacts, Enter acknowledges, "
                    "Y/N answers, F1-F6 selects, F9 saves, Escape acknowledges NPC/cancels WhoWill/exits.\n";
                return SdlWindow().showInteractive(first, status(), handler, escape, idle, status);
            }
        };
        return playGameplay(services, camera, target, resume);
    } catch (const std::exception &error) {
        std::cerr << "Gameplay startup failed";
        if (savePath) std::cerr << " [" << std::filesystem::absolute(*savePath).u8string() << ']';
        std::cerr << ": " << error.what() << '\n';
        return 3;
    }
}

} // namespace mmodern
