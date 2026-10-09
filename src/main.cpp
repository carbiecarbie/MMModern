#include "app/Application.h"

#include <cstdint>
#include <iostream>
#include <limits>
#include <string>
#include <vector>
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#include <shellapi.h>

namespace {

void newGameUsage() {
	std::cerr << "Usage: mmodern --new-game <game-dir> [--difficulty adventurer|warrior] [--save-file <path>] [--ui-data <XEEN.DAT>]\n"
		"       mmodern <game-dir> [--ui-data <XEEN.DAT>]\n"
		"Plain launch opens the original title menu.\n"
		"Developer --new-game defaults to Adventurer. F9 requires an explicit save target.\n"
		"--combat-seed is accepted only by --journey-region.\n";
}

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
	// The CRT narrow argv loses non-ASCII Windows paths. Decode the native
	// command line once, and use UTF-8 explicitly at filesystem boundaries.
	int wideCount = 0;
	wchar_t **wide = CommandLineToArgvW(GetCommandLineW(), &wideCount);
	if (!wide) return 1;
	std::vector<std::string> arguments;
	for (int i = 0; i < wideCount; ++i) {
		const int size = WideCharToMultiByte(CP_UTF8, 0, wide[i], -1, nullptr, 0, nullptr, nullptr);
		std::string text(size, '\0');
		WideCharToMultiByte(CP_UTF8, 0, wide[i], -1, text.data(), size, nullptr, nullptr);
		text.pop_back(); arguments.push_back(std::move(text));
	}
	LocalFree(wide);
	std::vector<char *> pointers;
	for (auto &argument : arguments) pointers.push_back(argument.data());
	argc = wideCount; argv = pointers.data();
 // UI data is a global source option, removed before entry-mode parsing.
 std::optional<std::filesystem::path> uiData;
 for(std::size_t i=1;i<arguments.size();) {
  if(arguments[i]!="--ui-data") {++i;continue;}
  if(uiData || i+1>=arguments.size() || arguments[i+1].empty() || arguments[i+1].rfind("--",0)==0) {
   std::cerr<<"--ui-data requires one nonempty installed uncompressed English XEEN.DAT path\n";return 1;
  }
  uiData=std::filesystem::u8path(arguments[i+1]);arguments.erase(arguments.begin()+i,arguments.begin()+i+2);
 }
 pointers.clear();for(auto &argument:arguments)pointers.push_back(argument.data());
 argc=static_cast<int>(pointers.size());argv=pointers.data();
 const mmodern::Application application(uiData.value_or(std::filesystem::path{}));
	// Plain launch enters the original title. Overrides require developer entry.
	if (argc >= 2 && (std::string(argv[1]) == "--new-game" ||
			std::string(argv[1]).rfind("--", 0) != 0)) {
		const int path = std::string(argv[1]) == "--new-game" ? 2 : 1;
		if(path==1) {
			if(argc!=2 || std::string(argv[1]).empty()){newGameUsage();return 1;}
			return application.run(std::filesystem::u8path(argv[1]));
		}
		bool valid = path < argc && std::string(argv[path]).size() &&
			std::string(argv[path]).rfind("--", 0) != 0;
		auto difficulty = mmodern::XeenDifficulty::Adventurer;
		bool difficultySeen = false;
		std::optional<std::filesystem::path> save;
		for (int i = path + 1; valid && i < argc; i += 2) {
			const std::string option = argv[i];
			if (i + 1 >= argc) { valid = false; break; }
			const std::string value = argv[i + 1];
			if (option == "--difficulty" && !difficultySeen) {
				difficultySeen = true;
				if (value == "warrior") difficulty = mmodern::XeenDifficulty::Warrior;
				else if (value != "adventurer") valid = false;
			} else if (option == "--save-file" && !save && !value.empty() && value.rfind("--", 0) != 0) {
				save = std::filesystem::u8path(value);
			} else valid = false;
		}
		if (!valid) { newGameUsage(); return 1; }
		return application.newGame(std::filesystem::u8path(argv[path]), difficulty, save);
	}
	for (int i=1;i<argc;++i) if (std::string(argv[i]) == "--combat-seed" || std::string(argv[i]) == "--journey-region") {
		const bool regional = argc >= 2 && std::string(argv[1]) == "--journey-region";
		std::optional<std::uint32_t> seed;
		std::optional<std::filesystem::path> save;
		int positional = argc;
		if (argc >= 5 && std::string(argv[argc-2]) == "--save-file") {
			const std::string target = argv[argc-1];
			if (target.empty() || target.rfind("--",0) == 0) {
				std::cerr << "Usage: --journey-region requires a nonempty save path\n"; return 1;
			}
			save = std::filesystem::u8path(target); positional -= 2;
		}
		bool valid = argc >= 3 && regional;
		int path = 2;
		if (valid && positional == 5 && std::string(argv[2]) == "--combat-seed") {
			const std::string text = argv[3];
			std::uint64_t value = 0;
			valid = !text.empty() && text.size() <= 10;
			for (char ch:text) {
				if (ch<'0' || ch>'9') { valid=false; break; }
				value=value*10+static_cast<unsigned>(ch-'0');
			}
			valid = valid && value && value <= std::numeric_limits<std::uint32_t>::max();
			seed=static_cast<std::uint32_t>(value); path=4;
		} else valid = valid && positional == 3;
		valid = valid && path<argc && std::string(argv[path]).size() && std::string(argv[path]).rfind("--",0)!=0;
		if (!valid) { std::cerr << "Usage: " << "--journey-region" << " [--combat-seed <nonzero-u32>] <game-directory> [--save-file <path>] [--ui-data <XEEN.DAT>]\n"; return 1; }
		return application.journeyRegion(std::filesystem::u8path(argv[path]),seed,save);
	}

	if (argc == 3 && std::string(argv[1]) == "--inspect-map")
		return application.inspectMap(argv[2]);
	if (argc == 4 && std::string(argv[1]) == "--inspect-map") {
		std::uint16_t mapId = 0;
		if (!parseMapId(argv[3], mapId)) {
			std::cerr << "Invalid map ID: " << argv[3] << '\n';
			return 1;
		}
		return application.inspectMap(argv[2], mapId);
	}
	if (argc == 3 && std::string(argv[1]) == "--inspect-party")
		return application.inspectParty(argv[2]);
	if (argc == 4 && std::string(argv[1]) == "--inspect-events") {
		std::uint16_t mapId = 0;
		if (!parseMapId(argv[3], mapId)) {
			std::cerr << "Invalid map ID: " << argv[3] << '\n';
			return 1;
		}
		return application.inspectEvents(argv[2], mapId);
	}
	if ((argc == 6 || argc == 7) &&
			std::string(argv[1]) == "--inspect-events") {
		std::uint16_t mapId = 0;
		std::uint8_t x = 0;
		std::uint8_t y = 0;
		if (!parseMapId(argv[3], mapId)) {
			std::cerr << "Invalid map ID: " << argv[3] << '\n';
			return 1;
		}
		if (!parseEventCoordinate(argv[4], x) ||
				!parseEventCoordinate(argv[5], y)) {
			std::cerr << "Invalid event coordinates: use values from 0 through 255.\n";
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
				std::cerr << "Invalid event direction: use north, east, south, west or all.\n";
				return 1;
			}
		}
		return application.inspectEvents(argv[2], mapId, x, y,
			direction, allOnly);
	}
    if (argc >= 2 && std::string(argv[1]) == "--load-game") {
        if (argc != 4 || std::string(argv[2]).empty() || std::string(argv[3]).empty() || std::string(argv[2]).rfind("--", 0) == 0 || std::string(argv[3]).rfind("--", 0) == 0) {
            std::cerr << "Usage: --load-game <game-directory> <save-path>\n"; return 1;
        }
        return application.loadGame(std::filesystem::u8path(argv[2]), std::filesystem::u8path(argv[3]));
    }
    if (argc >= 2 && std::string(argv[1]) == "--render-map") {
        std::uint16_t mapId = 1; int x = 9, y = 6;
        auto direction = mmodern::XeenDirection::South;
        if ((argc != 3 && argc != 7) || std::string(argv[2]).rfind("--", 0) == 0 ||
            (argc == 7 && (!parseMapId(argv[3], mapId) || !parseCoordinate(argv[4], x) ||
                !parseCoordinate(argv[5], y) || !parseDirection(argv[6], direction)))) {
            std::cerr << "Usage: --render-map <game-directory> [<map> <x> <y> <north|east|south|west>]\n";
            return 1;
        }
        return application.renderMap(std::filesystem::u8path(argv[2]), mapId, x, y, direction);
    }
	if (argc != 2 || std::string(argv[1]).rfind("--", 0) == 0) {
		newGameUsage();
		std::cerr << "     " << argv[0] << " --inspect-map <game-directory>\n";
		std::cerr << "     " << argv[0] << " --inspect-map <game-directory> <map-id>\n";
		std::cerr << "     " << argv[0] << " --inspect-party <game-directory>\n";
		std::cerr << "     " << argv[0] << " --inspect-events <game-directory> <map-id>\n";
		std::cerr << "     " << argv[0] << " --inspect-events <game-directory> <map-id> <x> <y> [north|east|south|west|all]\n";
		std::cerr << "     " << argv[0] << " --render-map <game-directory>\n";
		std::cerr << "     " << argv[0] << " --render-map <game-directory> [<map-id> <x> <y> <north|east|south|west>]\n";
		std::cerr << "     " << argv[0] << " --journey-region [--combat-seed <nonzero-u32>] <game-directory> [--save-file <path>] [--ui-data <XEEN.DAT>]\n";
		std::cerr << "     " << argv[0] << " --load-game <game-directory> <save-path>\n";
		return 1;
	}

	return 1;
}
