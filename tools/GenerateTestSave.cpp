#include "tools/XeenTestSave.h"
#include "platform/XeenSaveFile.h"
#include "games/xeen/XeenInstallationDetector.h"
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#include <shellapi.h>
#include <iostream>
#include <stdexcept>
#include <vector>

int main() {
    int count=0;
    wchar_t **arguments=CommandLineToArgvW(GetCommandLineW(),&count);
    if (!arguments) return 1;
    // Native paths retain Unicode, as in mmodern's command-line entry point.
    std::vector<std::wstring> args(arguments,arguments+count);
    LocalFree(arguments);
    try {
        if (args.size()!=4 && (args.size()!=6 || args[4]!=L"--ui-data" || args[5].empty())) throw std::invalid_argument(
            "Usage: mmodern_test_save <broken-armor|train-ready|injured-dead|poisoned|monster-control|monster-damage|party-healing|m76-spells|combat-items|m78-playtest|m78-one-holder|m79-playtest|m80-playtest|m80-arena|m80-arena-maxed|m81-drops|m81-fullpacks|m81-event|m84-playtest-trap|m84-playtest-label|m84-playtest-save|m85-playtest-label|m85-playtest-run|m85-playtest-trap|m86-playtest-fall|m86-playtest-moon-ray|m86-playtest-border|m87-inn|m87-three|m87-shangrila|m88-roster|m92-lord-xeen|m92-sixth-mirror|m89-lord-xeen|m89-clan-king|m89-draco|m89-dragons|m89-trolls|m90-newcastle|m90-golem|m90-illusion|m90-high-magic|m90-darzog|m90-clouds|m90-castle|m90-winterkill|m90-emerson-short|m90-emerson-walls|m90-taxman-walls|m90-emerson-keep|m90-taxman-keep|m90-artemus|m90-emerson-dungeon|m90-taxman-dungeon|m90-laboratory|m90-sword|m90-mirabeth|m90-mirabeth-elixir|m90-elixir|m90-credit-yak|m90-golem-terrors|m90-credit-terrors|m90-golem-stone|m90-credit-golem|m90-darzog-key|m90-crodo|m90-slick|m90-slick-trade|m90-generator|m90-winterkill-gong|m90-winterkill-wave2|m90-winterkill-wave3|m90-winterkill-final|m95-witch-skull|m96-chest|m97-open-chest|m98-load|m98-inn> <game-directory> <output.mmsave> [--ui-data <path>]");
        const std::string preset(args[1].begin(),args[1].end());
        const auto installation=mmodern::XeenInstallationDetector(args.size()==6?std::filesystem::path(args[5]):std::filesystem::path{}).detect(std::filesystem::path(args[2]));
        if (!installation) throw std::runtime_error("Game installation not found");
        mmodern::developer::generateTestSave(*installation,preset,std::filesystem::path(args[3]));
        if(preset.rfind("m98-",0)==0 || preset=="m97-open-chest" || preset=="m96-chest" || preset=="m95-witch-skull" || preset.rfind("m90-",0)==0 || preset.rfind("m92-",0)==0 || preset=="m86-playtest-fall" || preset=="m86-playtest-border") {
            const auto saved=mmodern::XeenSaveFile::read(std::filesystem::path(args[3]));
            const char *directions[]{"North","East","South","West"};
            std::cout<<"Generated "<<preset<<" at Clouds map "<<saved.camera.mapId.number<<" ("<<saved.camera.x<<","<<saved.camera.y
                <<"), facing "<<directions[unsigned(saved.camera.direction)]<<".\n";
            return 0;
        }
        std::cout<<"Generated "<<preset<<(preset=="m89-lord-xeen"?" before Lord Xeen, map 78 (5,8), facing North; Slayer Sword equipped.\n":preset=="m89-clan-king"?" before the Clan King, map 41 (30,25), facing North.\n":preset=="m89-draco"?" before Count Draco's coffin, map 29 (1,13), facing North at 21:00.\n":preset=="m89-dragons"?" on the lake, map 15 (0,12), facing North; Walk on Water active.\n":preset=="m89-trolls"?" at the Ice Troll lair, map 8 (2,7), facing North.\n":preset=="m88-roster"?" outside Vertigo tavern (24,4), facing North; 22 lodged characters, slots 12 and 21 free.\n":preset=="m87-shangrila"?" outside Shangri-La tavern (14,12), facing North.\n":preset=="m87-inn" || preset=="m87-three"?" outside Vertigo tavern (24,4), facing North.\n":preset=="m85-playtest-trap"?" at Tower of High Magic map 55 (7,8), facing East, with original monster 8.\n":preset=="m85-playtest-label" || preset=="m85-playtest-run"?" at Vertigo map 28 (13,4), facing East, with original monster 35.\n":preset=="m84-playtest-trap"?" at the damage trap, Tower of High Magic map 55 (7,8), facing North.\n":preset=="m84-playtest-label"?" at the door label, Vertigo map 28 (13,4), facing West.\n":preset=="m84-playtest-save"?" at the flagged save cell, Clouds map 25 (27,3), facing North.\n":preset=="m81-drops" || preset=="m81-fullpacks"?" beside the Barbarians, Clouds map 3 (5,12), facing West.\n":preset=="m81-event"?" before the Root Event, Clouds map 23 (8,2), facing North.\n":preset=="m80-arena" || preset=="m80-arena-maxed"?" at the Warzone entrance, Clouds map 6 (13,4), facing West.\n":preset=="m80-playtest"?" at Reaper entrance, Clouds map 24 (10,9), facing North.\n":preset=="m79-playtest"?" at Rivercity (12,12), facing North.\n":(preset=="monster-control" || preset=="monster-damage" || preset=="party-healing")?" at Vertigo (18,4), facing West.\n":" at Clouds map 23 (9,11), facing West.\n");
        return 0;
    } catch (const std::exception &e) {std::cerr<<e.what()<<'\n';return 1;}
}
