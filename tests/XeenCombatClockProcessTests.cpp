#include "XeenChildProcessTestSupport.h"
#include "XeenM40Evidence.h"
#include "platform/XeenSaveFile.h"
#include "formats/xeen/XeenSaveFormat.h"
#include <set>
using namespace mmodern;
namespace fs=std::filesystem;
int main(int argc,char **argv) {
    try {
        child_test::require(argc==3,"usage: mmodern_combat_clock_process_tests <M40-witness> <original-installation>");
        const auto exe=fs::absolute(argv[1]),game=fs::absolute(argv[2]);
        const auto dir=child_test::freshDirectory(fs::temp_directory_path()/"mmodern-m40-combat-clock");
        std::cout<<"M40 combat-clock process evidence: "<<dir<<'\n';
        SetEnvironmentVariableW(L"SDL_VIDEODRIVER",L"dummy");SetEnvironmentVariableW(L"SDL_RENDER_DRIVER",L"software");
        SetEnvironmentVariableW(L"MMODERN_M40_CONTROL",nullptr);
        SetEnvironmentVariableW(L"MMODERN_M40_BRANCH",L"production");
        SetEnvironmentVariableW(L"MMODERN_M40_STAGE",L"fresh");
        std::set<std::pair<DWORD,std::uint64_t>> incarnations;
        fs::path source,expected;
        if(const auto reuse=std::getenv("MMODERN_M40_CLOCK_SOURCE")) {
            source=fs::path(reuse);
            const auto final=std::getenv("MMODERN_M40_CLOCK_EXPECTED");
            child_test::require(final,"Focused clock run needs certified post-service and final snapshots");expected=fs::path(final);
        } else {
            const auto save=dir/"production.mmsave";
            const auto fresh=child_test::launch(exe,{L"--journey-region",L"--combat-seed",L"3626689381",game.wstring(),L"--save-file",save.wstring()},dir/"production.log");
            child_test::require(fresh.exit==0 && fresh.output.find("M40 PRODUCTION WITNESS PASSED")!=std::string::npos,"Genuine service route failed before clock regression");
            incarnations.insert({fresh.pid,fresh.created});source=dir/"production-D.mmsave";expected=save;
        }
        const auto before=XeenSaveFormat::encode(XeenSaveFile::read(source));
        const auto diskBefore=m40_test::diskBytes(source);
        const auto d=XeenSaveFile::read(source);
        child_test::require(d.journey && d.journey->schema==9 && d.journey->contract==11 && d.journey->context->day==12 &&
            d.journey->treasure->gold==807 && d.characters[6].armor[0].state==0 && d.characters[6].armor[1].state==0,
            "Clock regression source is not genuine post-service D");
        const auto run=[&](const std::string &control,bool healthy=false) {
            const auto save=dir/(control+".mmsave");fs::copy_file(source,save);
            SetEnvironmentVariableW(L"MMODERN_M40_STAGE",L"D");SetEnvironmentVariableW(L"MMODERN_M40_BRANCH",L"clock");
            SetEnvironmentVariableW(L"MMODERN_M40_CONTROL",fs::path(control).c_str());
            const auto result=child_test::launch(exe,{L"--load-game",game.wstring(),save.wstring()},dir/(control+".log"));
            child_test::require(incarnations.insert({result.pid,result.created}).second &&
                result.output.find("M40 RESTORE EXACT BEFORE INPUT")!=std::string::npos &&
                result.output.find("M40 POST-SERVICE RESTORED PLAYERREADY CLOCK ARMED")!=std::string::npos,
                "Clock regression lacks separate process/exact pre-input restore/real PlayerReady evidence");
            if(healthy) {
                const auto actual=XeenSaveFile::read(save),final=XeenSaveFile::read(expected);m40_test::equalFields(actual,final);
                child_test::require(result.exit==0 && result.output.find("M40 COMBAT CLOCK HEALTHY CONTINUATION PASSED")!=std::string::npos &&
                    XeenSaveFormat::encode(actual)==XeenSaveFormat::encode(final),"Healthy clock/redraw/recovery changed continuation");
            } else child_test::require(result.exit==4 &&
                result.output.find("M40 COMBAT CLOCK ABA MONOTONIC / NO QUIET / F9 DISK PRESERVED / RETRY REJECTED")!=std::string::npos &&
                XeenSaveFormat::encode(XeenSaveFile::read(save))==before && m40_test::diskBytes(save)==diskBefore,
                "Clock/presentation ABA continued or changed previous save");
        };
        for(const std::string path:{"scheduling","cosmetic"})
            for(const std::string field:{"bank-gold","bank-gems","stock","presence"})run("clock-"+path+"-"+field);
        run("clock-scheduling-bank-gold-throw");run("clock-cosmetic-presence-throw");
        for(const std::string field:{"bank-gold","bank-gems","stock","presence"})run("presentation-"+field);
        run("clock-scheduling-healthy",true);run("clock-cosmetic-recovery",true);
        std::cout<<"M40 BOTH CLOCK PATHS / FOUR ECONOMY ABAS / THROW / PRESENTATION / MONOTONIC FAILURE / POST-SERVICE FULL-EXIT RESTORE PASSED\n";
        return 0;
    }catch(const std::exception &error){std::cerr<<error.what()<<'\n';return 1;}
}
