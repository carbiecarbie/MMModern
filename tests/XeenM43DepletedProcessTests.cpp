#include "XeenChildProcessTestSupport.h"
#include "XeenM42Evidence.h"
#include "platform/XeenSaveFile.h"
#include "formats/xeen/XeenSaveFormat.h"
using namespace mmodern;
namespace fs=std::filesystem;
int main(int argc,char **argv) {
    try {
        child_test::require(argc==4,"usage: mmodern_m43_depleted_process_tests <m42-witness> <m43-witness> <original-installation>");
        const auto m42=fs::absolute(argv[1]),m43=fs::absolute(argv[2]),game=fs::absolute(argv[3]);
        const auto dir=child_test::freshDirectory(fs::temp_directory_path()/"mmodern-m43-depleted");
        SetEnvironmentVariableW(L"SDL_VIDEODRIVER",L"dummy");
        SetEnvironmentVariableW(L"SDL_RENDER_DRIVER",L"software");
        SetEnvironmentVariableW(L"MMODERN_M42_CONTENT14",L"1");
        SetEnvironmentVariableW(L"MMODERN_M42_STAGE",L"depleted");
        const auto prefix=dir/"bought.mmsave";
        const auto buy=child_test::launch(m42,
            {L"--journey-region",L"--combat-seed",L"7",game.wstring(),L"--save-file",prefix.wstring()},
            dir/"buy.log",false,false,180000);
        child_test::require(buy.exit==0 &&
            buy.output.find("M43 DEPLETED BUY PREFIX PASSED")!=std::string::npos &&
            buy.output.find("depletion7->6 stable")!=std::string::npos,
            "fresh content-14 production Buy prefix failed");
        const auto b=dir/"bought-B.mmsave";
        const auto original=XeenSaveFile::read(b);
        child_test::require(original.journey && original.journey->schema==9 &&
            original.journey->contract==14 && original.journey->context->day==9 &&
            original.journey->treasure->gold==670 && original.journey->random->state==799325555u &&
            original.journey->random->count==1101,
            "real bought checkpoint is not the exact content-14 depleted preimage");
        m42_test::sameCategory(original.journey->serviceEconomy->wares[0][0][1],m42_test::armorAfter());
        SetEnvironmentVariableW(L"MMODERN_M42_CONTENT14",nullptr);
        SetEnvironmentVariableW(L"MMODERN_M42_STAGE",nullptr);
        for(const auto &branch:{L"depleted",L"depleted-unpaid",L"depleted-training"}) {
            SetEnvironmentVariableW(L"MMODERN_M43_STAGE",branch);
            const auto target=dir/(std::wstring(branch)+L".mmsave");
            fs::copy_file(b,target);
            const auto result=child_test::launch(m43,
                {L"--load-game",game.wstring(),target.wstring()},dir/(std::wstring(branch)+L".log"),
                false,false,120000);
            child_test::require(result.exit==0 &&
                result.output.find("M43 RESTORE EXACT BEFORE INPUT")!=std::string::npos &&
                result.output.find("M43 DEPLETED ORIGINAL BUY PREIMAGE")!=std::string::npos &&
                result.output.find(branch==std::wstring(L"depleted-training") ?
                    "M43 DEPLETED TRAINING DAYS9/10/11/12 GOLD490 ALL1152 ONE GENERATION" :
                    branch==std::wstring(L"depleted") ?
                    "M43 DEPLETED PAID REPLACEMENT ALL1152 DAY11" :
                    "M43 DEPLETED UNPAID RETAINED DAY10")!=std::string::npos &&
                std::make_pair(buy.pid,buy.created)!=std::make_pair(result.pid,result.created),
                "fresh-process depleted Temple/Training continuation failed");
            if(branch==std::wstring(L"depleted-training")) {
                const auto trained=XeenSaveFile::read(fs::path(target.wstring()+L"-T.mmsave"));
                m42_test::StockOracle oracle{original.journey->random->state,original.journey->random->count,{}};
                const auto generated=oracle.generate();
                child_test::require(trained.journey && trained.journey->context->day==12 &&
                    trained.journey->treasure->gold==490 && trained.characters[18].permanentLevel==4 &&
                    trained.characters[1].permanentLevel==4 &&
                    m40_test::stockBytes(*trained.journey->serviceEconomy)==m40_test::stockBytes(generated) &&
                    trained.journey->random->state==oracle.state && trained.journey->random->count==oracle.count &&
                    trained.journey->serviceEconomy->bank==original.journey->serviceEconomy->bank,
                    "M43 depleted Training saved owners or one-generation successor differ");
            }
            child_test::require(XeenSaveFormat::encode(XeenSaveFile::read(b))==XeenSaveFormat::encode(original),
                "source Buy checkpoint was changed by a continuation process");
        }
        SetEnvironmentVariableW(L"MMODERN_M43_STAGE",nullptr);
        std::cout<<"M43 production Buy depletion, fresh-process Temple and Training continuations passed\n";
        return 0;
    }catch(const std::exception &error){std::cerr<<error.what()<<'\n';return 1;}
}
