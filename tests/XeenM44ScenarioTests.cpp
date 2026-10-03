// M44 original-data process scenarios. Each runs a production-application
// generator, hashes the final save against tests/XeenM44BaselineDigests.h, then
// reloads that save, saves again and requires identical bytes. Saves live in a
// temporary directory and are always removed: they contain original-data bytes.
#include "XeenChildProcessTestSupport.h"
#include "XeenM40Evidence.h"
#include "XeenM44BaselineDigests.h"
#include <iterator>
#include <string>
namespace fs=std::filesystem;
namespace {
struct Scenario {
    const char *key;             // CTest argument
    std::size_t digest;          // index into m44_baseline::scenarios
    const wchar_t *environment;  // MMODERN_M44_SCENARIO for the M44 witness, or null
    const char *save;            // --save-file name
    const char *final;           // file whose bytes are hashed
};
constexpr Scenario table[]{
    {"mainland",0,L"mainland","mainland.mmsave","mainland.mmsave-final.mmsave"},
    {"services",1,L"services","services-paid.mmsave","services-paid.mmsave-final.mmsave"},
    {"temple",2,nullptr,"temple.mmsave","temple.mmsave"},
};
std::vector<std::uint8_t> bytes(const fs::path &p) {
    std::ifstream in(p,std::ios::binary);child_test::require(bool(in),"Scenario save is missing");
    return {std::istreambuf_iterator<char>(in),{}};
}
}
int main(int argc,char **argv) {
    fs::path dir;
    try {
        child_test::require(argc==6,"usage: mmodern_m44_scenario_tests <mainland|services|temple> <generator> <m44-witness> <restart> <original-installation>");
        const Scenario *scenario=nullptr;
        for(const auto &s:table)if(std::string(argv[1])==s.key)scenario=&s;
        child_test::require(scenario,"Unknown M44 scenario");
        const auto &expected=m44_baseline::scenarios[scenario->digest];
        const auto generator=fs::absolute(argv[2]),witness=fs::absolute(argv[3]),restart=fs::absolute(argv[4]),game=fs::absolute(argv[5]);
        dir=child_test::freshDirectory(fs::temp_directory_path()/(std::string("mmodern-m44-scenario-")+scenario->key));
        SetEnvironmentVariableW(L"SDL_VIDEODRIVER",L"dummy");
        SetEnvironmentVariableW(L"SDL_RENDER_DRIVER",L"software");
        SetEnvironmentVariableW(L"MMODERN_M44_SCENARIO",scenario->environment);
        const auto save=dir/scenario->save;
        const auto made=child_test::launch(generator,{L"--journey-region",L"--combat-seed",std::to_wstring(expected.seed),
            game.wstring(),L"--save-file",save.wstring()},dir/"generate.log",false,false,900000);
        child_test::require(made.exit==0,"Scenario generator failed");
        const auto final=dir/scenario->final;
        const auto digest=m40_test::sha256(bytes(final));
        std::cout<<"Scenario "<<expected.name<<" digest "<<digest<<'\n';
        child_test::require(digest==expected.sha256,"Scenario final save digest differs from the Step-0 baseline");
        const auto again=child_test::launch(restart,{witness.wstring(),game.wstring(),fs::path(scenario->key).wstring(),final.wstring()},
            dir/"restart-driver.log",false,false,900000);
        child_test::require(again.exit==0 && again.output.find("Baseline restart and final bytes exact")!=std::string::npos,
            "Scenario reload and re-save changed the save bytes");
        std::cout<<"Scenario "<<expected.name<<": digest and reload round trip exact\n";
    }catch(const std::exception &e){
        std::cerr<<e.what()<<'\n';
        std::error_code ignored;if(!dir.empty())fs::remove_all(dir,ignored);
        return 1;
    }
    std::error_code ignored;fs::remove_all(dir,ignored);
    return 0;
}
