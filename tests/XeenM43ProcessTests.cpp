#include "XeenChildProcessTestSupport.h"
#include "XeenM40Evidence.h"
#include "platform/XeenSaveFile.h"
#include "formats/xeen/XeenSaveFormat.h"
#include <fstream>
#include <iterator>
#include <vector>
using namespace mmodern;
namespace fs=std::filesystem;
namespace {
void equal(const fs::path &a,const fs::path &b){
    std::ifstream leftFile(a,std::ios::binary),rightFile(b,std::ios::binary);
    child_test::require(bool(leftFile) && bool(rightFile),"M43 checkpoint byte comparison cannot open saves");
    const std::vector<char> leftBytes{std::istreambuf_iterator<char>(leftFile),{}},
        rightBytes{std::istreambuf_iterator<char>(rightFile),{}};
    child_test::require(leftBytes==rightBytes,"M43 uninterrupted and restored save file bytes differ");
    const auto left=XeenSaveFile::read(a),right=XeenSaveFile::read(b);
    child_test::require(XeenSaveFormat::encode(left)==XeenSaveFormat::encode(right),
        "M43 uninterrupted and fresh-process restore bytes differ");
    m40_test::equalFields(left,right);
}
}
int main(int argc,char **argv){
    try{
        child_test::require(argc==3,"usage: mmodern_m43_process_tests <witness> <original-installation>");
        const auto exe=fs::absolute(argv[1]),game=fs::absolute(argv[2]);
        const auto dir=child_test::freshDirectory(fs::temp_directory_path()/"mmodern-m43-process");
        SetEnvironmentVariableW(L"SDL_VIDEODRIVER",L"dummy");
        SetEnvironmentVariableW(L"SDL_RENDER_DRIVER",L"software");
        const auto fresh=dir/"fresh.mmsave";
        const auto first=child_test::launch(exe,
            {L"--journey-region",L"--combat-seed",L"3626689381",game.wstring(),L"--save-file",fresh.wstring()},
            dir/"fresh.log",false,false,180000);
        child_test::require(first.exit==0 && first.output.find("M43 EARNED DEATH")!=std::string::npos &&
            first.output.find("M43 PAID DEPARTURE")!=std::string::npos &&
            first.output.find("M43 CHECKPOINT C")!=std::string::npos &&
            first.output.find("M43 RESET SLIME SEYMOUR MAGIC ARROW")!=std::string::npos &&
            first.output.find("M43 CHECKPOINT E")!=std::string::npos,
            "M43 earned fresh production witness failed");
        const auto a=fs::path(fresh.wstring()+L"-A.mmsave"),b=fs::path(fresh.wstring()+L"-B.mmsave"),
            c=fs::path(fresh.wstring()+L"-C.mmsave"),e=fs::path(fresh.wstring()+L"-E.mmsave");
        const auto before=XeenSaveFile::read(a),paid=XeenSaveFile::read(b);
        child_test::require(before.journey && paid.journey && before.journey->schema==9 && before.journey->contract==14 &&
            before.journey->context->day==8 && before.journey->context->minutes==604 &&
            before.journey->treasure->gold==810 && before.characters[6].currentHp==-15 &&
            before.characters[1].currentHp==0 && paid.journey->context->day==10 &&
            paid.journey->context->minutes==604 && paid.journey->treasure->gold==340 &&
            paid.characters[6].currentHp==15 && paid.characters[6].currentSp==27 &&
            paid.characters[1].currentHp==21 && paid.characters[1].currentSp==21 &&
            paid.journey->random->state==2583579601u && paid.journey->random->count==2144,
            "M43 original-resource A/B owner or successor values differ");
        const auto refused=dir/"refused.mmsave";fs::copy_file(a,refused);
        SetEnvironmentVariableW(L"MMODERN_M43_STAGE",L"unpaid");
        const auto refusal=child_test::launch(exe,{L"--load-game",game.wstring(),refused.wstring()},
            dir/"refused.log",false,false,120000);
        SetEnvironmentVariableW(L"MMODERN_M43_STAGE",nullptr);
        child_test::require(refusal.exit==0 &&
            refusal.output.find("M43 RESTORE EXACT BEFORE INPUT")!=std::string::npos &&
            refusal.output.find("M43 ORIGINAL A REFUSAL DAY9 STOCK/RNG RETAINED")!=std::string::npos &&
            std::make_pair(first.pid,first.created)!=std::make_pair(refusal.pid,refusal.created),
            "M43 original A refusal process branch failed");
        const auto restored=dir/"restored.mmsave";fs::copy_file(b,restored);
        const auto second=child_test::launch(exe,{L"--load-game",game.wstring(),restored.wstring()},
            dir/"restore.log",false,false,180000);
        child_test::require(second.exit==0 && second.output.find("M43 RESTORE EXACT BEFORE INPUT")!=std::string::npos &&
            second.output.find("M43 CHECKPOINT C")!=std::string::npos &&
            second.output.find("M43 RESET SLIME SEYMOUR MAGIC ARROW")!=std::string::npos &&
            second.output.find("M43 CHECKPOINT E")!=std::string::npos &&
            std::make_pair(first.pid,first.created)!=std::make_pair(second.pid,second.created),
            "M43 fresh process restore or pre-input check failed");
        equal(c,fs::path(restored.wstring()+L"-C.mmsave"));
        equal(e,fs::path(restored.wstring()+L"-E.mmsave"));
        const auto returned=dir/"returned.mmsave";fs::copy_file(e,returned);
        SetEnvironmentVariableW(L"MMODERN_M43_STAGE",L"E");
        const auto third=child_test::launch(exe,{L"--load-game",game.wstring(),returned.wstring()},
            dir/"return-restore.log",false,false,120000);
        SetEnvironmentVariableW(L"MMODERN_M43_STAGE",nullptr);
        child_test::require(third.exit==0 &&
            third.output.find("M43 RESTORE EXACT BEFORE INPUT")!=std::string::npos &&
            third.output.find("M43 CHECKPOINT F")!=std::string::npos &&
            std::make_pair(first.pid,first.created)!=std::make_pair(third.pid,third.created) &&
            std::make_pair(second.pid,second.created)!=std::make_pair(third.pid,third.created),
            "M43 checkpoint E fresh-process restore or continuation failed");
        equal(e,fs::path(returned.wstring()+L"-E-restored.mmsave"));
        equal(fs::path(fresh.wstring()+L"-F.mmsave"),fs::path(restored.wstring()+L"-F.mmsave"));
        equal(fs::path(fresh.wstring()+L"-F.mmsave"),fs::path(returned.wstring()+L"-F.mmsave"));
        std::cout<<"M43 original-resource earned death, recovery, reset combat, return Heal and exact fresh-process continuation passed\n";
        return 0;
    }catch(const std::exception &error){std::cerr<<error.what()<<'\n';return 1;}
}
