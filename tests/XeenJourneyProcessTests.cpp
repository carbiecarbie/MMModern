#include "XeenChildProcessTestSupport.h"
#include "XeenSaveTestSupport.h"
#include "platform/XeenSaveFile.h"
#include <iostream>
#include <set>
namespace fs=std::filesystem;
int main(int argc,char **argv) {try {
 child_test::require(argc==3,"usage: journey-process <CLI-witness> <original-installation>");
 const auto exe=fs::absolute(argv[1]),game=fs::absolute(argv[2]);
 const auto dir=fs::current_path()/("journey-process-"+std::to_string(GetCurrentProcessId()));fs::create_directories(dir);
 const auto connected=dir/fs::path(L"connected \u00e7 \u6e38.mmsave"),moved=dir/"moved.mmsave",fresh=dir/"fresh.mmsave";
 SetEnvironmentVariableW(L"SDL_VIDEODRIVER",L"dummy");SetEnvironmentVariableW(L"SDL_RENDER_DRIVER",L"software");
 std::ofstream report(dir/"processes.log");std::set<DWORD> pids;
 for(const std::string mode:{"producer","consumer","final","moved","moved-load","fresh"}) {
  SetEnvironmentVariableW(L"MMODERN_JOURNEY_WITNESS",fs::path(mode).c_str());
  const bool resume=mode=="consumer"||mode=="final"||mode=="moved-load";
  const auto path=mode=="fresh"?fresh:mode=="moved"||mode=="moved-load"?moved:connected;
  const std::vector<std::wstring> args=resume?std::vector<std::wstring>{L"--load-game",game.wstring(),path.wstring()}:
   std::vector<std::wstring>{L"--journey-skeleton",L"--combat-seed",L"56",game.wstring(),L"--save-file",path.wstring()};
  const auto result=child_test::launch(exe,args,dir/(mode+".log"));
  report<<mode<<" PID="<<result.pid<<" exit="<<result.exit<<'\n'<<std::flush;
  if(result.exit)std::cerr<<result.output;
  child_test::require(result.exit==0&&pids.insert(result.pid).second,"independent CLI/SDL process witness failed");
  child_test::require(result.output.find("CLI SDL witness "+mode+" passed")!=std::string::npos,"missing exact-state witness");
  if(mode=="producer"||mode=="consumer")fs::copy_file(connected,dir/(mode+".mmsave"));
 }
 SetEnvironmentVariableW(L"MMODERN_JOURNEY_WITNESS",nullptr);
 // Uninstrumented product smoke: real startup, inventory keys, and window close.
 // Detailed mutation/oracle driving remains in the separately linked witness.
 const auto cli=exe.parent_path()/"mmodern.exe";
 const auto disk=[](const fs::path &path){std::ifstream in(path,std::ios::binary);return std::string(std::istreambuf_iterator<char>(in),{});};
 for(bool resume:{false,true}) {
  const auto path=resume?connected:fresh;const auto before=disk(path);
  const auto args=resume?std::vector<std::wstring>{L"--load-game",game.wstring(),path.wstring()}:
   std::vector<std::wstring>{L"--journey-skeleton",L"--combat-seed",L"56",game.wstring(),L"--save-file",path.wstring()};
  const auto result=child_test::launch(cli,args,dir/(resume?"product-load.log":"product-fresh.log"),true,true);
  report<<"product "<<(resume?"load":"fresh")<<" PID="<<result.pid<<" exit="<<result.exit<<'\n';
  child_test::require(result.exit==0&&result.output.find(resume?"Camera 20 (Clouds) 13 2 3":"Camera 20 (Clouds) 13 1 0")!=std::string::npos,"uninstrumented product startup state");
  child_test::require(disk(path)==before,"product startup/inspection/exit unexpectedly rewrote save");
 }
 const auto incompatible=dir/"incompatible.mmsave";
 auto saved=mmodern::XeenSaveFile::read(connected);++saved.resources.clouds.crc32;mmodern::XeenSaveFile::write(incompatible,saved);
 const auto rejected=child_test::launch(cli,{L"--load-game",game.wstring(),incompatible.wstring()},dir/"product-incompatible.log");
 child_test::require(rejected.exit==3&&rejected.output.find("Resumed ")==std::string::npos&&rejected.output.find("incompatible")!=std::string::npos,"incompatible v4 must fail without fallback");
 std::cout<<"Six distinct CLI/SDL processes passed, including Unicode target, moved anchor, fresh control and repeated load. Evidence: "<<dir.u8string()<<'\n';return 0;
 }catch(const std::exception &e){std::cerr<<e.what()<<'\n';return 1;}}
