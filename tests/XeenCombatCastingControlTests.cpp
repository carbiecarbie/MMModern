#include "XeenChildProcessTestSupport.h"
#include "platform/XeenSaveFile.h"
#include <cstdlib>
namespace fs=std::filesystem;
int main(int argc,char **argv){try{
    child_test::require(argc==3,"usage: cast-control-process <witness> <installation>");
    const auto dir=child_test::freshDirectory(fs::temp_directory_path()/"mmodern-m39-controls");
    if(std::getenv("MMODERN_M39_ONLY_INPUT")) {
        SetEnvironmentVariableW(L"SDL_VIDEODRIVER",L"dummy");SetEnvironmentVariableW(L"SDL_RENDER_DRIVER",L"software");
        SetEnvironmentVariableW(L"MMODERN_M39_BRANCH",L"input");
        for(const std::string control:{"attack","block","run","cast","target"}) {
            SetEnvironmentVariableW(L"MMODERN_M39_CONTROL",fs::path(control).c_str());
            const auto result=child_test::launch(fs::absolute(argv[1]),{L"--journey-region",L"--combat-seed",L"1",fs::absolute(argv[2]).wstring(),L"--save-file",(dir/(control+".mmsave")).wstring()},dir/(control+".log"));
            if(result.exit!=0)std::cerr<<result.output;
            child_test::require(result.exit==0 && result.output.find("M39 PLAYERREADY INPUT PASSED "+control)!=std::string::npos,"M39 native PlayerReady input failed");
        }
        std::cout<<"M39 PlayerReady input evidence "<<dir<<'\n';return 0;
    }
    // Obtain the retained city actor facts from a genuine original A branch.
    SetEnvironmentVariableW(L"SDL_VIDEODRIVER",L"dummy");SetEnvironmentVariableW(L"SDL_RENDER_DRIVER",L"software");
    SetEnvironmentVariableW(L"MMODERN_M39_BRANCH",L"A");SetEnvironmentVariableW(L"MMODERN_M39_STAGE",L"fresh");SetEnvironmentVariableW(L"MMODERN_M39_CONTROL",nullptr);
    const auto source=child_test::launch(fs::absolute(argv[1]),{L"--journey-region",L"--combat-seed",L"1",fs::absolute(argv[2]).wstring(),L"--save-file",(dir/"city-source.mmsave").wstring()},dir/"city-source.log");
    child_test::require(source.exit==0,"M39 city fixture original source failed");
    SetEnvironmentVariableW(L"MMODERN_M39_BRANCH",L"fault");SetEnvironmentVariableW(L"MMODERN_M39_STAGE",L"fresh");
    const bool authorityOnly=std::getenv("MMODERN_M39_ONLY_AUTHORITY")!=nullptr;
    for(const std::string control:{"aba-hp","aba-inactive","aba-sp","aba-book","aba-presence","aba-class","aba-item","aba-supplement",
        "aba-membership","aba-purse","aba-context","aba-flags","aba-rng","aba-mainland","aba-city","aba-statistics","aba-map","aba-mob","aba-event",
        "names","reservation-retry","before-debit","target-fail","refund-fail","arrow-fail-1","arrow-fail-2","arrow-fail-3","arrow-yield","arrow-xp-overflow","arrow-drop-overflow","arrow-successor-fail","partial",
        "recursive","target-retry","result-retry","projectile-retry",
        "observation-payer","observation-selection","observation-target","observation-hp","observation-refund",
        "observation-recovery","observation-awaken","observation-yield","observation-successor","observation-guard"}) {
        if(authorityOnly && control.rfind("observation-",0)!=0)continue;
        SetEnvironmentVariableW(L"MMODERN_M39_CONTROL",fs::path(control).c_str());
        const auto save=dir/(control+".mmsave");
        std::vector<std::wstring> args{L"--journey-region",L"--combat-seed",L"1",fs::absolute(argv[2]).wstring(),L"--save-file",save.wstring()};
        if(control=="arrow-xp-overflow" || control=="arrow-drop-overflow" || control=="aba-city") {
            // Disclosed detached synthetic save fixture; no live owner edit.
            auto fixture=mmodern::XeenSaveFile::read(dir/"aba-hp.mmsave");
            if(control=="arrow-xp-overflow")fixture.journey->supplements[0].inputs.experience=0xffffffffu;
            else if(control=="arrow-drop-overflow")fixture.journey->treasure->gold=0xffffffffu;
            else fixture.journey->vertigoActors=mmodern::XeenSaveFile::read(dir/"city-source-city.mmsave").journey->vertigoActors;
            mmodern::XeenSaveFile::write(save,fixture);args={L"--load-game",fs::absolute(argv[2]).wstring(),save.wstring()};
        }
        const auto result=child_test::launch(fs::absolute(argv[1]),args,dir/(control+".log"));
        if(result.exit!=0)std::cerr<<result.output;
        child_test::require(result.exit==0 && result.output.find("M39 ARTIFICIAL CONTROL PASSED "+control)!=std::string::npos,"M39 authority/publication control failed");
    }
    SetEnvironmentVariableW(L"SDL_VIDEODRIVER",L"dummy");SetEnvironmentVariableW(L"SDL_RENDER_DRIVER",L"software");
    SetEnvironmentVariableW(L"MMODERN_M39_BRANCH",L"native-fault");
    for(const std::string control:{"upload-target","copy-target","upload-result","copy-result","upload-projectile","copy-projectile"}) {
        if(authorityOnly)continue;
        SetEnvironmentVariableW(L"MMODERN_M39_CONTROL",fs::path(control).c_str());
        const auto result=child_test::launch(fs::absolute(argv[1]),{L"--journey-region",L"--combat-seed",L"1",fs::absolute(argv[2]).wstring(),L"--save-file",(dir/(control+".mmsave")).wstring()},dir/(control+".log"));
        if(result.exit!=0)std::cerr<<result.output;
        child_test::require(result.exit==0 && result.output.find("M39 NATIVE FAILURE PREFIX PASSED "+control)!=std::string::npos,"M39 native upload/copy prefix control failed");
    }
    std::cout<<"M39 authority/failure evidence "<<dir<<'\n';return 0;
}catch(const std::exception &e){std::cerr<<e.what()<<'\n';return 1;}}
