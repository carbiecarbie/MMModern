// M44 original-data process scenarios. Each runs a production-application
// generator, hashes the final save against tests/XeenM44BaselineDigests.h, then
// reloads that save, saves again and requires identical bytes. A scenario with
// resume checkpoints also resumes from each of them in its own resume-<label>
// directory and requires the resumed final to equal the uninterrupted one.
// Saves live in a temporary directory and are always removed: they contain
// original-data bytes.
#include "XeenChildProcessTestSupport.h"
#include "XeenM40Evidence.h"
#include "XeenM44BaselineDigests.h"
#include <iterator>
#include <string>
#include <vector>
namespace fs=std::filesystem;
namespace {
struct Scenario {
    const char *key;             // CTest argument
    const char *name;            // m44_baseline::scenarios name
    std::uint32_t seed;          // --combat-seed
    const wchar_t *environment;  // MMODERN_M44_SCENARIO for the M44 witness, or null
    const char *save;            // --save-file name
    const char *final;           // file whose bytes are hashed
    std::vector<const char *> resumes; // checkpoints resumed with MMODERN_M44_STAGE
};
const Scenario table[]{
    {"mainland","mainland-combat-myra-phirna",3626689381u,L"mainland","mainland.mmsave","mainland.mmsave-final.mmsave",{}},
    {"services","vertigo-buy-repair-training",7u,L"services","services-paid.mmsave","services-paid.mmsave-final.mmsave",{}},
    {"temple","temple-recovery",3626689381u,nullptr,"temple.mmsave","temple.mmsave",{}},
    {"roundtrip","vertigo-f3-roundtrip",3626689381u,L"roundtrip","roundtrip.mmsave","roundtrip.mmsave-final.mmsave",{"A","B"}},
};
std::vector<std::uint8_t> bytes(const fs::path &p) {
    std::ifstream in(p,std::ios::binary);child_test::require(bool(in),"Scenario save is missing");
    return {std::istreambuf_iterator<char>(in),{}};
}
fs::path wide(const char *value) {return fs::path(std::string(value));}
}
int main(int argc,char **argv) {
    fs::path dir;
    try {
        child_test::require(argc==6,"usage: mmodern_m44_scenario_tests <mainland|services|temple|roundtrip> <generator> <m44-witness> <restart> <original-installation>");
        const Scenario *scenario=nullptr;
        for(const auto &s:table)if(std::string(argv[1])==s.key)scenario=&s;
        child_test::require(scenario,"Unknown M44 scenario");
        const m44_baseline::Scenario *expected=nullptr;
        for(const auto &s:m44_baseline::scenarios)if(std::string(s.name)==scenario->name)expected=&s;
        child_test::require(!expected || expected->seed==scenario->seed,"Scenario seed differs from the recorded baseline");
        const auto generator=fs::absolute(argv[2]),witness=fs::absolute(argv[3]),restart=fs::absolute(argv[4]),game=fs::absolute(argv[5]);
        dir=child_test::freshDirectory(fs::temp_directory_path()/(std::string("mmodern-m44-scenario-")+scenario->key));
        SetEnvironmentVariableW(L"SDL_VIDEODRIVER",L"dummy");
        SetEnvironmentVariableW(L"SDL_RENDER_DRIVER",L"software");
        SetEnvironmentVariableW(L"MMODERN_M44_SCENARIO",scenario->environment);
        SetEnvironmentVariableW(L"MMODERN_M44_STAGE",nullptr);
        const auto save=dir/scenario->save;
        const auto made=child_test::launch(generator,{L"--journey-region",L"--combat-seed",std::to_wstring(scenario->seed),
            game.wstring(),L"--save-file",save.wstring()},dir/"generate.log",false,false,900000);
        child_test::require(made.exit==0,"Scenario generator failed");
        const auto final=dir/scenario->final;
        const auto digest=m40_test::sha256(bytes(final));
        std::cout<<"Scenario "<<scenario->name<<" digest "<<digest<<'\n';
        if(expected)child_test::require(digest==expected->sha256,"Scenario final save digest differs from the recorded baseline");
        else std::cout<<"Scenario "<<scenario->name<<": digest not recorded yet (awaiting the maintainer's approval)\n";
        for(const auto *label:scenario->resumes) {
            // Resume in a separate directory so checkpoint globs see only the uninterrupted run.
            const auto resumed=child_test::freshDirectory(dir/(std::string("resume-")+label));
            fs::copy_file(fs::path(save.wstring()+L"-"+wide(label).wstring()+L".mmsave"),resumed/scenario->save);
            SetEnvironmentVariableW(L"MMODERN_M44_STAGE",wide(label).c_str());
            const auto again=child_test::launch(witness,{L"--load-game",game.wstring(),(resumed/scenario->save).wstring()},
                resumed/"generate.log",false,false,900000);
            SetEnvironmentVariableW(L"MMODERN_M44_STAGE",nullptr);
            child_test::require(again.exit==0,"Scenario resume failed");
            child_test::require(bytes(resumed/scenario->final)==bytes(final),"Resumed final differs from the uninterrupted final");
            std::cout<<"Scenario "<<scenario->name<<": resumed from "<<label<<", final byte-identical\n";
        }
        const auto again=child_test::launch(restart,{witness.wstring(),game.wstring(),fs::path(scenario->key).wstring(),final.wstring()},
            dir/"restart-driver.log",false,false,900000);
        child_test::require(again.exit==0 && again.output.find("Baseline restart and final bytes exact")!=std::string::npos,
            "Scenario reload and re-save changed the save bytes");
        std::cout<<"Scenario "<<scenario->name<<": digest and reload round trip exact\n";
    }catch(const std::exception &e){
        std::cerr<<e.what()<<'\n';
        std::error_code ignored;if(!dir.empty())fs::remove_all(dir,ignored);
        return 1;
    }
    std::error_code ignored;fs::remove_all(dir,ignored);
    return 0;
}
