#include "tools/XeenTestSave.h"
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
            "Usage: mmodern_test_save <broken-armor|train-ready|injured-dead|poisoned> <game-directory> <output.mmsave> [--ui-data <path>]");
        const std::string preset(args[1].begin(),args[1].end());
        const auto installation=mmodern::XeenInstallationDetector(args.size()==6?std::filesystem::path(args[5]):std::filesystem::path{}).detect(std::filesystem::path(args[2]));
        if (!installation) throw std::runtime_error("Game installation not found");
        mmodern::developer::generateTestSave(*installation,preset,std::filesystem::path(args[3]));
        std::cout<<"Generated "<<preset<<" at Clouds map 23 (9,11), facing West.\n";
        return 0;
    } catch (const std::exception &e) {std::cerr<<e.what()<<'\n';return 1;}
}
