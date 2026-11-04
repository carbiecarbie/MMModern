#include "XeenPortableTestSupport.h"
#include "XeenChildProcessTestSupport.h"
namespace fs=std::filesystem;
int main(int argc,char **argv){
    try{
        child_test::require(argc==3,"usage: mmodern_m43_process_tests <witness> <original-installation>");
        const auto exe=fs::absolute(argv[1]),game=fs::absolute(argv[2]);
        const auto dir=child_test::freshDirectory(fs::temp_directory_path()/"mmodern-m43-process");
        portable_test::setEnvironment("SDL_VIDEODRIVER","dummy");
        portable_test::setEnvironment("SDL_RENDER_DRIVER","software");
        const auto fresh=dir/"fresh.mmsave";
        const auto first=child_test::launch(exe,
            {"--journey-region","--combat-seed","3626689381",game.u8string(),"--save-file",fresh.u8string()},
            dir/"fresh.log",false,false,180000);
        child_test::require(first.exit==0 && first.output.find("M43 CHECKPOINT A")!=std::string::npos,
            "M43 original unpaid quote-cancellation fixture failed");
        const auto a=fs::path(fresh.u8string()+"-A.mmsave");
        const auto refused=dir/"refused.mmsave";fs::copy_file(a,refused);
        portable_test::setEnvironment("MMODERN_M43_STAGE","unpaid");
        const auto refusal=child_test::launch(exe,{"--load-game",game.u8string(),refused.u8string()},
            dir/"refused.log",false,false,120000);
        portable_test::setEnvironment("MMODERN_M43_STAGE",nullptr);
        child_test::require(refusal.exit==0 &&
            refusal.output.find("M43 RESTORE EXACT BEFORE INPUT")!=std::string::npos &&
            refusal.output.find("M43 ORIGINAL A REFUSAL DAY9 STOCK/RNG RETAINED")!=std::string::npos &&
            std::make_pair(first.pid,first.created)!=std::make_pair(refusal.pid,refusal.created),
            "M43 original A refusal process branch failed");
        std::cout<<"M43 original-resource unpaid quote cancellation and one-day departure passed\n";
        child_test::removePassedRun(dir);return 0;
    }catch(const std::exception &error){std::cerr<<error.what()<<'\n';return 1;}
}
