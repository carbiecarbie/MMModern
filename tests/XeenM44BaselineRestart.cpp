#include "XeenPortableTestSupport.h"
#include "XeenChildProcessTestSupport.h"
#include <iterator>
namespace fs=std::filesystem;
std::vector<char> bytes(const fs::path &p) {
    std::ifstream in(p,std::ios::binary);child_test::require(bool(in),"Missing baseline save");
    return {std::istreambuf_iterator<char>(in),{}};
}
int main(int argc,char **argv) {
    try {
        child_test::require(argc==5,"usage: baseline-restart witness game scenario save");
        const auto source=fs::absolute(argv[4]);
        const auto dir=child_test::freshDirectory(source.parent_path()/"m44-restart");
        const auto target=dir/"restart.mmsave";
        fs::copy_file(source,target);
        portable_test::setEnvironment("SDL_VIDEODRIVER","dummy");
        portable_test::setEnvironment("SDL_RENDER_DRIVER","software");
        portable_test::setEnvironment("MMODERN_M44_SCENARIO",fs::path(argv[3]).c_str());
        const auto r=child_test::launch(fs::absolute(argv[1]),{"--wall-clock","0","--load-game",fs::absolute(argv[2]).u8string(),target.u8string()},dir/"restart.log");
        child_test::require(r.exit==0 && r.output.find("M44 RESTORE EXACT BEFORE INPUT")!=std::string::npos,"Baseline restart failed");
        child_test::require(bytes(source)==bytes(target),"Baseline saved state changed after restart and F9");
        std::cout<<"Baseline restart and final bytes exact: "<<argv[3]<<'\n';
    }catch(const std::exception &e){std::cerr<<e.what()<<'\n';return 1;}
}

