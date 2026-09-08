#include "XeenChildProcessTestSupport.h"
#include "XeenSaveGameplayTestSupport.h"
#include "SyntheticXeenArchive.h"
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#include <fstream>
#include <iostream>
using namespace gameplay_test;
namespace fs=std::filesystem;
using child_test::Result;
Result launch(const fs::path &exe,const std::vector<std::wstring>&args,const fs::path &log){
 static unsigned sequence = 0;
 return child_test::launch(exe,args,fs::path(log.wstring()+L"."+std::to_wstring(GetCurrentProcessId())+L"."+std::to_wstring(++sequence)));
}
int main(int argc,char **argv){try{
 check(argc==2,"CLI test executable argument");const fs::path exe=fs::absolute(argv[1]);
 const auto dir=fs::current_path()/"save-cli-tests";const auto game=dir/"commercial";fs::create_directories(game);
 const auto log=dir/"cli.log";const auto path=dir/fs::path(L"space \u00e7 \u6e38.mmsave");fs::remove(path);
 const std::vector<std::vector<std::wstring>> bad{
 {L"--load-game"},{L"--load-game",game.wstring()},{L"--load-game",game.wstring(),path.wstring(),L"1"},
 {L"--load-game",game.wstring(),path.wstring(),L"--save-file",path.wstring()},
 {L"--render-map"},{L"--render-map",game.wstring(),L"--save-file"},
 {L"--render-map",game.wstring(),L"--save-file",path.wstring(),L"--save-file",path.wstring()},
 {L"--render-map",game.wstring(),L"1",L"0",L"0",L"bad"},
 {L"--render-map",game.wstring(),L"0",L"0",L"0",L"north"},
 {L"--render-map",game.wstring(),L"1",L"16",L"0",L"north"},
 {L"--render-map",game.wstring(),L"--save-file",L"--load-game"}};
 for(const auto &args:bad)check(launch(exe,args,log).exit==1,"invalid CLI syntax accepted");
 Fixture fixture;Bytes party(782),roster(30*354),map(892);map[768]=1;map[781]=128;
 sprite_test::archive(dir/"initial-test.cc",{{"maze.chr",roster},{"maze.pty",party},{"maze0001.dat",map}});
 std::ifstream innerFile(dir/"initial-test.cc",std::ios::binary);
 Bytes inner(std::istreambuf_iterator<char>(innerFile),{});
 for(std::size_t i=2+3*8;i<inner.size();++i)inner[i]^=0x35; // Initial archive payload is plaintext.
 sprite_test::archive(game/"xeen.cc",{{"fnt",fontBytes()},{"2a0c",inner}});
 GameInstallation installation{game,game/"xeen.cc",{},GameEdition::CloudsOfXeen};
 auto s=fixture.saved();s.resources=XeenSaveFile::fingerprint(installation);
 const auto run=[&](const char *message){const auto r=launch(exe,{L"--load-game",game.wstring(),path.wstring()},log);check(r.exit==3&&r.output.find(message)!=std::string::npos&&r.output.find("Resumed ")==std::string::npos,"CLI startup failure/fallback contract");};
 run("Inspect save file");
 for(int kind=0;kind<5;++kind){
  auto saved=s;if(kind==0)saved.resources.clouds.crc32++;if(kind==1)saved.activeRosterIds={24};
  XeenSaveFile::write(path,saved);
  if(kind==2){std::ofstream out(path,std::ios::binary|std::ios::trunc);out<<"bad";}
  if(kind==3){auto b=XeenSaveFormat::encode(s);b[8]=2;std::ofstream out(path,std::ios::binary|std::ios::trunc);out.write(reinterpret_cast<const char*>(b.data()),b.size());}
  run(kind==0?"incompatible":kind==1?"portrait":kind==2?"format":kind==3?"version":"mm4.pal");
  fs::remove(path);
 }
 XeenSaveFile::write(path,s);HANDLE lock=CreateFileW(path.c_str(),GENERIC_READ,0,nullptr,OPEN_EXISTING,FILE_ATTRIBUTE_NORMAL,nullptr);
 check(lock!=INVALID_HANDLE_VALUE,"CLI save lock");run("Open save file");CloseHandle(lock);
 // Valid old/new render syntax reaches Application rather than syntax rejection.
 for(const auto &args:std::vector<std::vector<std::wstring>>{{L"--render-map",game.wstring()},
  {L"--render-map",game.wstring(),L"1",L"0",L"0",L"north"},
  {L"--render-map",game.wstring(),L"--save-file",path.wstring()},
  {L"--render-map",game.wstring(),L"1",L"0",L"0",L"north",L"--save-file",path.wstring()}})
  check(launch(exe,args,log).exit==3,"valid render syntax rejected");
 std::cout<<"CLI syntax, Unicode paths and production startup failure matrix passed\n";return 0;
}catch(const std::exception &e){std::cerr<<e.what()<<'\n';return 1;}}
