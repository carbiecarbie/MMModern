#include "app/Application.h"

#include <cstdint>
#include <iostream>
#include <limits>
#include <string>

namespace {

bool parseMapId(const char *text, std::uint16_t &mapId) {
	try {
		std::size_t consumed = 0;
		const unsigned long value = std::stoul(text, &consumed, 10);
		if (consumed != std::string(text).size() || value == 0 || value > 9999 ||
				value > std::numeric_limits<std::uint16_t>::max())
			return false;
		mapId = static_cast<std::uint16_t>(value);
		return true;
	} catch (...) {
		return false;
	}
}

bool parseCoordinate(const char *text, int &coordinate) {
	try {
		std::size_t consumed = 0;
		const long value = std::stol(text, &consumed, 10);
		if (consumed != std::string(text).size() || value < 0 || value > 15)
			return false;
		coordinate = static_cast<int>(value);
		return true;
	} catch (...) {
		return false;
	}
}

bool parseEventCoordinate(const char *text, std::uint8_t &coordinate) {
	try {
		std::size_t consumed = 0;
		const unsigned long value = std::stoul(text, &consumed, 10);
		if (consumed != std::string(text).size() || value > 255)
			return false;
		coordinate = static_cast<std::uint8_t>(value);
		return true;
	} catch (...) {
		return false;
	}
}

bool parseDirection(const std::string &text, mmodern::XeenDirection &direction) {
	if (text == "north")
		direction = mmodern::XeenDirection::North;
	else if (text == "east")
		direction = mmodern::XeenDirection::East;
	else if (text == "south")
		direction = mmodern::XeenDirection::South;
	else if (text == "west")
		direction = mmodern::XeenDirection::West;
	else
		return false;
	return true;
}

} // namespace

int main(int argc, char *argv[]) {
	if (argc == 3 && std::string(argv[1]) == "--inspect-map")
		return mmodern::Application().inspectMap(argv[2]);
	if (argc == 4 && std::string(argv[1]) == "--inspect-map") {
		std::uint16_t mapId = 0;
		if (!parseMapId(argv[3], mapId)) {
			std::cerr << "ID de mapa invalido: " << argv[3] << '\n';
			return 1;
		}
		return mmodern::Application().inspectMap(argv[2], mapId);
	}
	if (argc == 3 && std::string(argv[1]) == "--inspect-party")
		return mmodern::Application().inspectParty(argv[2]);
	if (argc == 4 && std::string(argv[1]) == "--inspect-events") {
		std::uint16_t mapId = 0;
		if (!parseMapId(argv[3], mapId)) {
			std::cerr << "ID de mapa invalido: " << argv[3] << '\n';
			return 1;
		}
		return mmodern::Application().inspectEvents(argv[2], mapId);
	}
	if ((argc == 6 || argc == 7) &&
			std::string(argv[1]) == "--inspect-events") {
		std::uint16_t mapId = 0;
		std::uint8_t x = 0;
		std::uint8_t y = 0;
		if (!parseMapId(argv[3], mapId)) {
			std::cerr << "ID de mapa invalido: " << argv[3] << '\n';
			return 1;
		}
		if (!parseEventCoordinate(argv[4], x) ||
				!parseEventCoordinate(argv[5], y)) {
			std::cerr << "Coordenadas de evento invalidas: use valores entre 0 e 255.\n";
			return 1;
		}
		std::optional<mmodern::XeenDirection> direction;
		bool allOnly = false;
		if (argc == 7) {
			mmodern::XeenDirection parsed = mmodern::XeenDirection::North;
			if (std::string(argv[6]) == "all") {
				allOnly = true;
			} else if (parseDirection(argv[6], parsed)) {
				direction = parsed;
			} else {
				std::cerr << "Direcao de evento invalida: use north, east, south, west ou all.\n";
				return 1;
			}
		}
		return mmodern::Application().inspectEvents(argv[2], mapId, x, y,
			direction, allOnly);
	}
	if (argc == 3 && std::string(argv[1]) == "--render-map")
		return mmodern::Application().renderMap(argv[2]);
	if (argc == 7 && std::string(argv[1]) == "--render-map") {
		std::uint16_t mapId = 0;
		int x = 0;
		int y = 0;
		mmodern::XeenDirection direction = mmodern::XeenDirection::North;
		if (!parseMapId(argv[3], mapId)) {
			std::cerr << "ID de mapa invalido: " << argv[3] << '\n';
			return 1;
		}
		if (!parseCoordinate(argv[4], x) || !parseCoordinate(argv[5], y)) {
			std::cerr << "Coordenadas invalidas: X e Y devem estar entre 0 e 15.\n";
			return 1;
		}
		if (!parseDirection(argv[6], direction)) {
			std::cerr << "Direcao invalida: use north, east, south ou west.\n";
			return 1;
		}
		return mmodern::Application().renderMap(argv[2], mapId, x, y, direction);
	}
	if (argc != 2 || std::string(argv[1]).rfind("--", 0) == 0) {
		std::cerr << "Uso: " << argv[0] << " <diretorio da instalacao de Xeen>\n";
		std::cerr << "     " << argv[0] << " --inspect-map <diretorio da instalacao de Xeen>\n";
		std::cerr << "     " << argv[0] << " --inspect-map <diretorio da instalacao de Xeen> <map-id>\n";
		std::cerr << "     " << argv[0] << " --inspect-party <diretorio da instalacao de Xeen>\n";
		std::cerr << "     " << argv[0] << " --inspect-events <diretorio da instalacao de Xeen> <map-id>\n";
		std::cerr << "     " << argv[0] << " --inspect-events <diretorio> <map-id> <x> <y> [north|east|south|west|all]\n";
		std::cerr << "     " << argv[0] << " --render-map <diretorio da instalacao de Xeen>\n";
		std::cerr << "     " << argv[0] << " --render-map <diretorio> <map-id> <x> <y> <north|east|south|west>\n";
		return 1;
	}

	return mmodern::Application().run(argv[1]);
}
