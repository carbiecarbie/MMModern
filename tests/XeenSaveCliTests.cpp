#include "XeenChildProcessTestSupport.h"
#include "XeenSaveGameplayTestSupport.h"
#include "SyntheticXeenArchive.h"
#include "XeenRegionalTestSupport.h"
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
 const auto root=fs::current_path()/"save-cli-tests";fs::create_directories(root);
 const auto dir=child_test::freshDirectory(root/"run");
 check(child_test::freshDirectory(root/"run")!=dir,"process evidence directories are isolated");
 const auto game=dir/"GAME";fs::create_directories(game);
 const auto log=dir/"cli.log";const auto path=dir/fs::path(L"space \u00e7 \u6e38.mmsave");fs::remove(path);
 const std::vector<std::vector<std::wstring>> bad{
 {L"--encounter-26"}, {L"--encounter-26",L""}, {L"--encounter-26",L"--load-game"},
 {L"--encounter-26",game.wstring(),L"20",L"13",L"1",L"north"},
 {L"--encounter-26",game.wstring(),L"extra"},
 {L"--encounter-26",game.wstring(),L"--save-file",path.wstring()},
 {L"--encounter-26",game.wstring(),L"--load-game",path.wstring()},
 {L"--render-map",game.wstring(),L"--encounter-26"},
 {L"--manual-equipment",game.wstring(),L"--encounter-26"},
 {L"--load-game",game.wstring(),path.wstring(),L"--encounter-26"},
 {L"--load-game"},{L"--load-game",game.wstring()},{L"--load-game",game.wstring(),path.wstring(),L"1"},
 {L"--load-game",game.wstring(),path.wstring(),L"--save-file",path.wstring()},
 {L"--render-map"},{L"--render-map",game.wstring(),L"--save-file"},
 {L"--render-map",game.wstring(),L"--save-file",path.wstring(),L"--save-file",path.wstring()},
 {L"--render-map",game.wstring(),L"1",L"0",L"0",L"bad"},
 {L"--render-map",game.wstring(),L"0",L"0",L"0",L"north"},
 {L"--render-map",game.wstring(),L"1",L"16",L"0",L"north"},
 {L"--render-map",game.wstring(),L"--save-file",L"--load-game"}};
 for(const auto &args:bad)check(launch(exe,args,log).exit==1,"invalid CLI syntax accepted");
 for(bool explicitEntry:{false,true}) {
  const auto arguments=[&](std::vector<std::wstring> tail) {
   std::vector<std::wstring> args;if(explicitEntry)args.push_back(L"--new-game");
   args.push_back(game.wstring());args.insert(args.end(),tail.begin(),tail.end());return args;
  };
  for(const auto &tail:std::vector<std::vector<std::wstring>>{
   {L"--difficulty"},{L"--difficulty",L""},{L"--difficulty",L"Warrior"},{L"--difficulty",L"easy"},
   {L"--difficulty",L"warrior",L"--difficulty",L"adventurer"},
   {L"--save-file"},{L"--save-file",L""},{L"--save-file",L"--difficulty"},
   {L"--save-file",path.wstring(),L"--save-file",path.wstring()},
   {L"--combat-seed",L"1"},{L"--combat-seed",L"0"},
   {L"--difficulty",L"warrior",L"--combat-seed",L"1"},
   {L"--journey-region"},{L"--load-game",path.wstring()},{L"--new-game"},{L"extra"}}) {
   const auto result=launch(exe,arguments(tail),log);
   check(result.exit==1 && result.output.find("--new-game <game-dir> [--difficulty adventurer|warrior] [--save-file <path>]")!=std::string::npos,
    "new-game invalid/duplicate/conflicting options must print exact usage");
  }
 }
 for(const auto &args:std::vector<std::vector<std::wstring>>{
  {L"--new-game"},{L"--new-game",L""},{L""},
  {L"--journey-region",game.wstring(),L"--difficulty",L"warrior"},
  {L"--journey-region",L"--difficulty",L"adventurer",game.wstring()},
  {L"--load-game",game.wstring(),path.wstring(),L"--difficulty",L"warrior"}})
  check(launch(exe,args,log).exit==1,"new-game missing path/difficulty override on Journey or load");
 for(const auto *entry:{L"--journey-region"}) {
  for(const auto *seed:{L"0",L"-1",L"+1",L"1x",L"4294967296",L"",L" 56",L"99999999999"})
   check(launch(exe,{entry,L"--combat-seed",seed,game.wstring()},log).exit==1,"strict seed syntax");
  for(const auto &args:std::vector<std::vector<std::wstring>>{
   {entry},{entry,L""},{entry,game.wstring(),L"extra"},
   {entry,game.wstring(),L"--combat-seed",L"56"},{entry,L"--combat-seed",L"56",L"--combat-seed",L"56",game.wstring()},
   {entry,game.wstring(),L"--save-file",path.wstring(),L"--save-file",path.wstring()},
   {entry,L"--load-game",game.wstring(),path.wstring()},
   {L"--load-game",game.wstring(),path.wstring(),entry},
   {L"--load-game",game.wstring(),path.wstring(),L"--combat-seed",L"56"},
   {entry,game.wstring(),L"--encounter-26"},{entry,game.wstring(),L"--journey-skeleton"},
   {entry,game.wstring(),L"--journey-expedition"},{entry,game.wstring(),L"--encounter-27"}})
   check(launch(exe,args,log).exit==1,"Journey/diagnostic duplicate or conflict accepted");
 }
 Fixture fixture;Bytes party(782),roster(30*354),map(892);map[768]=1;map[781]=128;
 sprite_test::archive(dir/"initial-test.cc",{{"maze.chr",roster},{"maze.pty",party},{"maze0001.dat",map}});
 std::ifstream innerFile(dir/"initial-test.cc",std::ios::binary);
 Bytes inner(std::istreambuf_iterator<char>(innerFile),{});
 for(std::size_t i=2+3*8;i<inner.size();++i)inner[i]^=0x35; // Initial archive payload is plaintext.
 sprite_test::archive(game/"xeen.cc",{{"fnt",fontBytes()},{"2a0c",inner}});
 GameInstallation installation{game,game/"xeen.cc",{},GameEdition::CloudsOfXeen};
 for(bool explicitEntry:{false,true})for(const auto &tail:std::vector<std::vector<std::wstring>>{
  {},{L"--difficulty",L"adventurer"},{L"--difficulty",L"warrior"},
  {L"--save-file",path.wstring()},
  {L"--difficulty",L"warrior",L"--save-file",path.wstring()},
  {L"--save-file",path.wstring(),L"--difficulty",L"adventurer"}}) {
  std::vector<std::wstring> args;if(explicitEntry)args.push_back(L"--new-game");args.push_back(game.wstring());
  args.insert(args.end(),tail.begin(),tail.end());const auto result=launch(exe,args,log);
  check(result.exit==3 && result.output.find("complete CD source")!=std::string::npos,"valid public new-game syntax reaches production");
 }
 for(const auto &args:std::vector<std::vector<std::wstring>>{
  {L"--journey-region",game.wstring()},
  {L"--journey-region",L"--combat-seed",L"4294967295",game.wstring()},
  {L"--journey-region",L"--combat-seed",L"56",game.wstring(),L"--save-file",path.wstring()}}) {
  const auto result=launch(exe,args,log);check(result.exit==3&&result.output.find("complete CD source")!=std::string::npos,"valid regional entry parsing");
 }
 for(const auto *entry:{L"--encounter-26",L"--encounter-27",L"--journey-skeleton",L"--journey-expedition"}) {
  const auto result=launch(exe,{entry,game.wstring()},log);
  check(result.exit==1&&result.output.find("Usage:")!=std::string::npos,"Removed mode must print usage");
 }
 for(const auto &args:std::vector<std::vector<std::wstring>>{
  {L"--render-map",game.wstring(),L"--save-file",path.wstring()},
  {L"--render-map",game.wstring(),L"1",L"0",L"0",L"north",L"--save-file",path.wstring()}})
  check(launch(exe,args,log).exit==1,"Map explorer cannot accept a save target");
 sprite_test::archive(game/"dark.cc",{{"synthetic",Bytes{0}}});
 sprite_test::archive(game/"intro.cc",{{"synthetic",Bytes{0}}});
 fs::create_directories(dir/"WORLD");
 {const auto ui=dos_test::fixture();std::ofstream out(dir/"WORLD"/"XEEN.DAT",std::ios::binary);out.write(reinterpret_cast<const char *>(ui.data()),ui.size());}
 installation.darkArchive=game/"dark.cc";installation.edition=GameEdition::WorldOfXeen;
 auto s=regional_test::snapshot();s.resources=XeenSaveFile::fingerprint(installation);
 const auto run=[&](const char *message){const auto r=launch(exe,{L"--load-game",game.wstring(),path.wstring()},log);check(r.exit==3&&r.output.find(message)!=std::string::npos&&r.output.find("Resumed ")==std::string::npos,"CLI startup failure/fallback content");};
 run("Inspect save file");
 for(int kind=0;kind<6;++kind){
  auto saved=s;if(kind==0)saved.resources.clouds.crc32++;if(kind==1)saved.activeRosterIds={24};
  XeenSaveFile::write(path,saved);
  if(kind==2){std::ofstream out(path,std::ios::binary|std::ios::trunc);out<<"bad";}
  if(kind==3 || kind==5){auto b=XeenSaveFormat::encode(s);b[8]=kind==3?7:5;std::ofstream out(path,std::ios::binary|std::ios::trunc);out.write(reinterpret_cast<const char*>(b.data()),b.size());}
  run(kind==0?"archive/data-edition incompatibility":kind==1?"Journey membership":kind==2?"format":kind==3?"newer or unsupported MMModern build":kind==5?"older MMModern build":"maze0023.dat");
  fs::remove(path);
 }
 XeenSaveFile::write(path,s);HANDLE lock=CreateFileW(path.c_str(),GENERIC_READ,0,nullptr,OPEN_EXISTING,FILE_ATTRIBUTE_NORMAL,nullptr);
 check(lock!=INVALID_HANDLE_VALUE,"CLI save lock");run("Open save file");CloseHandle(lock);
 // Unsaveable map exploration still reaches initial-owner loading.
 for(const auto &args:std::vector<std::vector<std::wstring>>{{L"--render-map",game.wstring()},
  {L"--render-map",game.wstring(),L"1",L"0",L"0",L"north"}})
  check(launch(exe,args,log).exit==3,"valid render syntax rejected");
 std::cout<<"CLI syntax, Unicode paths and production startup failure matrix passed\n";return 0;
}catch(const std::exception &e){std::cerr<<e.what()<<'\n';return 1;}}
