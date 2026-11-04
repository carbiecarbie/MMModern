#include "XeenChildProcessTestSupport.h"
#include "XeenSaveGameplayTestSupport.h"
#include "SyntheticXeenArchive.h"
#include "XeenRegionalTestSupport.h"
#ifndef NOMINMAX
#define NOMINMAX
#endif
#ifdef _WIN32
#include <windows.h>
#endif
#include <fstream>
#include <iostream>
using namespace gameplay_test;
namespace fs=std::filesystem;
using child_test::Result;
Result launch(const fs::path &exe,const std::vector<std::string>&args,const fs::path &log){
 static unsigned sequence = 0;
 // Preserve explicit CFG arguments in the parsing matrix. Once its synthetic
 // nullmus CFG exists, select it even if the build overrides the UI module.
 auto isolated=args;const auto cfg=log.parent_path()/"WORLD"/"XEEN.CFG";
 if(fs::is_regular_file(cfg) && std::find(args.begin(),args.end(),"--xeen-cfg")==args.end()) {
  isolated.push_back("--xeen-cfg");isolated.push_back(cfg.u8string());
 }
 return child_test::launch(exe,isolated,fs::u8path(log.u8string()+"."+std::to_string(child_test::processId())+"."+std::to_string(++sequence)),false,false,300000,false);
}
int main(int argc,char **argv){try{
 check(argc==2,"CLI test executable argument");const fs::path exe=fs::absolute(argv[1]);
 const auto root=fs::current_path()/"save-cli-tests";fs::create_directories(root);
 const auto dir=child_test::freshDirectory(root/"run");
 check(child_test::freshDirectory(root/"run")!=dir,"process evidence directories are isolated");
 const auto game=dir/"GAME";fs::create_directories(game);
 const auto log=dir/"cli.log";const auto path=dir/fs::u8path(u8"space \u00e7 \u6e38.mmsave");fs::remove(path);
 for(const auto &args:std::vector<std::vector<std::string>>{
  {"--inspect-party",game.u8string(),"--xeen-cfg"},
  {"--inspect-party",game.u8string(),"--xeen-cfg",""},
  {"--inspect-party",game.u8string(),"--xeen-cfg","--wall-clock"},
  {"--inspect-party",game.u8string(),"--xeen-cfg","a","--xeen-cfg","b"}}) {
  const auto r=launch(exe,args,log);
  check(r.exit==1 && r.output.find("--xeen-cfg")!=std::string::npos,"invalid CFG override accepted or missing diagnostic");
 }
 for(const auto &args:std::vector<std::vector<std::string>>{
  {game.u8string(),"--wall-clock","1"},
  {"--inspect-party",game.u8string(),"--wall-clock","4294967296"},
  {"--inspect-party",game.u8string(),"--wall-clock","-1"},
  {"--inspect-party",game.u8string(),"--wall-clock","0","--wall-clock","1"},
  {"--inspect-party",game.u8string(),"--wall-clock"}}) {
  const auto r=launch(exe,args,log);
  check(r.exit==1 && r.output.find("--wall-clock")!=std::string::npos,"invalid wall-clock override accepted or missing clear diagnostic");
 }
 const std::vector<std::vector<std::string>> bad{
 {"--encounter-26"}, {"--encounter-26",""}, {"--encounter-26","--load-game"},
 {"--encounter-26",game.u8string(),"20","13","1","north"},
 {"--encounter-26",game.u8string(),"extra"},
 {"--encounter-26",game.u8string(),"--save-file",path.u8string()},
 {"--encounter-26",game.u8string(),"--load-game",path.u8string()},
 {"--render-map",game.u8string(),"--encounter-26"},
 {"--manual-equipment",game.u8string(),"--encounter-26"},
 {"--load-game",game.u8string(),path.u8string(),"--encounter-26"},
 {"--load-game"},{"--load-game",game.u8string()},{"--load-game",game.u8string(),path.u8string(),"1"},
 {"--load-game",game.u8string(),path.u8string(),"--save-file",path.u8string()},
 {"--render-map"},{"--render-map",game.u8string(),"--save-file"},
 {"--render-map",game.u8string(),"--save-file",path.u8string(),"--save-file",path.u8string()},
 {"--render-map",game.u8string(),"1","0","0","bad"},
 {"--render-map",game.u8string(),"0","0","0","north"},
 {"--render-map",game.u8string(),"1","16","0","north"},
 {"--render-map",game.u8string(),"--save-file","--load-game"}};
 for(const auto &args:bad)check(launch(exe,args,log).exit==1,"invalid CLI syntax accepted");
 for(const auto &args:std::vector<std::vector<std::string>>{
  {game.u8string(),"--difficulty","warrior"},{game.u8string(),"--save-file",path.u8string()}}) {
  const auto result=launch(exe,args,log);
  check(result.exit==1 && result.output.find("Usage:")!=std::string::npos && !fs::exists(path),
   "bare launch developer override must print usage without writing");
 }
 for(bool explicitEntry:{false,true}) {
  const auto arguments=[&](std::vector<std::string> tail) {
   std::vector<std::string> args;if(explicitEntry)args.push_back("--new-game");
   args.push_back(game.u8string());args.insert(args.end(),tail.begin(),tail.end());return args;
  };
  for(const auto &tail:std::vector<std::vector<std::string>>{
   {"--difficulty"},{"--difficulty",""},{"--difficulty","Warrior"},{"--difficulty","easy"},
   {"--difficulty","warrior","--difficulty","adventurer"},
   {"--save-file"},{"--save-file",""},{"--save-file","--difficulty"},
   {"--save-file",path.u8string(),"--save-file",path.u8string()},
   {"--combat-seed","1"},{"--combat-seed","0"},
   {"--difficulty","warrior","--combat-seed","1"},
   {"--journey-region"},{"--load-game",path.u8string()},{"--new-game"},{"extra"}}) {
   const auto result=launch(exe,arguments(tail),log);
   check(result.exit==1 && result.output.find("--new-game <game-dir> [--difficulty adventurer|warrior] [--save-file <path>]")!=std::string::npos,
    "new-game invalid/duplicate/conflicting options must print exact usage");
  }
 }
 for(const auto &args:std::vector<std::vector<std::string>>{
  {"--new-game"},{"--new-game",""},{""},
  {"--journey-region",game.u8string(),"--difficulty","warrior"},
  {"--journey-region","--difficulty","adventurer",game.u8string()},
  {"--load-game",game.u8string(),path.u8string(),"--difficulty","warrior"}})
  check(launch(exe,args,log).exit==1,"new-game missing path/difficulty override on Journey or load");
 for(const auto *entry:{"--journey-region"}) {
  for(const auto *seed:{"0","-1","+1","1x","4294967296",""," 56","99999999999"})
   check(launch(exe,{entry,"--combat-seed",seed,game.u8string()},log).exit==1,"strict seed syntax");
  for(const auto &args:std::vector<std::vector<std::string>>{
   {entry},{entry,""},{entry,game.u8string(),"extra"},
   {entry,game.u8string(),"--combat-seed","56"},{entry,"--combat-seed","56","--combat-seed","56",game.u8string()},
   {entry,game.u8string(),"--save-file",path.u8string(),"--save-file",path.u8string()},
   {entry,"--load-game",game.u8string(),path.u8string()},
   {"--load-game",game.u8string(),path.u8string(),entry},
   {"--load-game",game.u8string(),path.u8string(),"--combat-seed","56"},
   {entry,game.u8string(),"--encounter-26"},{entry,game.u8string(),"--journey-skeleton"},
   {entry,game.u8string(),"--journey-expedition"},{entry,game.u8string(),"--encounter-27"}})
   check(launch(exe,args,log).exit==1,"Journey/diagnostic duplicate or conflict accepted");
 }
 Fixture fixture;Bytes party(782),roster(30*354),map(892);map[768]=1;map[781]=128;
 sprite_test::archive(dir/"initial-test.cc",{{"maze.chr",roster},{"maze.pty",party},{"maze0001.dat",map}});
 std::ifstream innerFile(dir/"initial-test.cc",std::ios::binary);
 Bytes inner(std::istreambuf_iterator<char>(innerFile),{});
 for(std::size_t i=2+3*8;i<inner.size();++i)inner[i]^=0x35; // Initial archive payload is plaintext.
 sprite_test::archive(game/"xeen.cc",{{"fnt",fontBytes()},{"2a0c",inner}});
 GameInstallation installation{game,game/"xeen.cc",{},GameEdition::CloudsOfXeen};
 for(bool explicitEntry:{true})for(const auto &tail:std::vector<std::vector<std::string>>{
  {},{"--difficulty","adventurer"},{"--difficulty","warrior"},
  {"--save-file",path.u8string()},
  {"--difficulty","warrior","--save-file",path.u8string()},
  {"--save-file",path.u8string(),"--difficulty","adventurer"}}) {
  std::vector<std::string> args;if(explicitEntry)args.push_back("--new-game");args.push_back(game.u8string());
  args.insert(args.end(),tail.begin(),tail.end());const auto result=launch(exe,args,log);
  check(result.exit==3 && result.output.find("complete CD source")!=std::string::npos,"valid public new-game syntax reaches production");
 }
 for(const auto &args:std::vector<std::vector<std::string>>{
  {"--journey-region",game.u8string()},
  {"--journey-region","--combat-seed","4294967295",game.u8string()},
  {"--journey-region","--combat-seed","56",game.u8string(),"--save-file",path.u8string()}}) {
  const auto result=launch(exe,args,log);check(result.exit==3&&result.output.find("complete CD source")!=std::string::npos,"valid regional entry parsing");
 }
 for(const auto *entry:{"--encounter-26","--encounter-27","--journey-skeleton","--journey-expedition"}) {
  const auto result=launch(exe,{entry,game.u8string()},log);
  check(result.exit==1&&result.output.find("Usage:")!=std::string::npos,"Removed mode must print usage");
 }
 for(const auto &args:std::vector<std::vector<std::string>>{
  {"--render-map",game.u8string(),"--save-file",path.u8string()},
  {"--render-map",game.u8string(),"1","0","0","north","--save-file",path.u8string()}})
  check(launch(exe,args,log).exit==1,"Map explorer cannot accept a save target");
 // Inspect now reads the full DARK New Game tail for two-sided party state.
 sprite_test::archive(dir/"initial-dark-test.cc",{{"maze.pty",Bytes(1530)}});
 std::ifstream darkFile(dir/"initial-dark-test.cc",std::ios::binary);
 Bytes darkInner(std::istreambuf_iterator<char>(darkFile),{});
 for(std::size_t i=2+8;i<darkInner.size();++i)darkInner[i]^=0x35;
 sprite_test::archive(game/"dark.cc",{{"2a0c",darkInner}});
 // The synthetic CFG selects the two null drivers, which must exist in INTRO.
 sprite_test::archive(game/"intro.cc",{{"synthetic",Bytes{0}},{"nullmus",Bytes{0}},{"nullsnd",Bytes{0}}});
 fs::create_directories(dir/"WORLD");
 {
  auto ui=dos_test::fixture();
  // Program audio reads this table even with nullmus. Neutral sample names
  // keep the synthetic module complete; this startup-failure matrix never
  // plays them. Append outside the fixture's established UI fields.
  for(unsigned i=0;i<6;++i) {
   const auto name="event"+std::to_string(i)+".voc";
   dos_test::u16(ui,0x508cc+i*4,unsigned(ui.size()-0x4d890));
   dos_test::u16(ui,0x508ce + i*4,0x481d);
   ui.insert(ui.end(),name.begin(),name.end());ui.push_back(0);
  }
  dos_test::u16(ui,2,ui.size()%512);dos_test::u16(ui,4,(ui.size()+511)/512);
  std::ofstream out(dir/"WORLD"/"XEEN.DAT",std::ios::binary);
  out.write(reinterpret_cast<const char *>(ui.data()),ui.size());
 }
 {std::ofstream out(dir/"WORLD"/"XEEN.CFG",std::ios::binary);const char cfg[6]{};out.write(cfg,sizeof(cfg));} // Synthetic nullmus configuration.
 const auto alternateCfg=dir/fs::u8path(u8"alternate \u00e7.cfg");
 {std::ofstream out(alternateCfg,std::ios::binary);const char cfg[29]{};out.write(cfg,sizeof(cfg));}
 {
  const auto good=launch(exe,{"--inspect-party",game.u8string(),"--xeen-cfg",alternateCfg.u8string()},log);
  check(good.exit==0 && good.output.find("Initial Clouds party:")!=std::string::npos,"Valid Unicode CFG override did not reach the entry mode");
  const auto missing=launch(exe,{"--inspect-party",game.u8string(),"--xeen-cfg",(dir/"missing.cfg").u8string()},log);
  check(missing.exit==3 && missing.output.find("Selected XEEN.CFG is missing")!=std::string::npos,"Missing override silently used the installed CFG");
  check(fs::file_size(alternateCfg)==29 && fs::file_size(dir/"WORLD"/"XEEN.CFG")==6,"CFG file was modified");
 }
 for(const auto *seconds:{"0","1","4294967295"}) {
  const auto r=launch(exe,{"--inspect-party",game.u8string(),"--wall-clock",seconds},log);
  const auto value=std::filesystem::path(seconds).string();
  check(r.exit==0 && r.output.find("Wall clock: fixed "+value)!=std::string::npos &&
   r.output.find("Initial Clouds party:")!=std::string::npos,"valid developer wall-clock override must use the fixed clock and run normally");
 }
 installation.darkArchive=game/"dark.cc";installation.edition=GameEdition::WorldOfXeen;
 auto s=regional_test::snapshot();s.resources=XeenSaveFile::fingerprint(installation);
 const auto run=[&](const char *message){const auto r=launch(exe,{"--load-game",game.u8string(),path.u8string()},log);check(r.exit==3&&r.output.find(message)!=std::string::npos&&r.output.find("Resumed ")==std::string::npos,"CLI startup failure/fallback content");};
#ifdef _WIN32
 run("Inspect save file");
#else
 run("Open save file");
#endif
 for(int kind=0;kind<6;++kind){
  auto saved=s;if(kind==0)saved.resources.clouds.crc32++;
  XeenSaveFile::write(path,saved);
  if(kind==1){auto b=XeenSaveFormat::encode(saved);b[53]=24;save_test::fixIndependentEnvelope(b);std::ofstream out(path,std::ios::binary|std::ios::trunc);out.write(reinterpret_cast<const char*>(b.data()),b.size());}
  if(kind==2){std::ofstream out(path,std::ios::binary|std::ios::trunc);out<<"bad";}
  if(kind==3 || kind==5){auto b=XeenSaveFormat::encode(s);const auto version=kind==3?XeenSaveFormat::kJourneyVersion+1:XeenSaveFormat::kJourneyVersion-1;b[8]=version;b[9]=version>>8;std::ofstream out(path,std::ios::binary|std::ios::trunc);out.write(reinterpret_cast<const char*>(b.data()),b.size());}
  run(kind==0?"archive/data-edition incompatibility":kind==1?"active roster identity":kind==2?"format":kind==3?"newer or unsupported MMModern build":kind==5?"older MMModern build":"maze0023.dat");
  fs::remove(path);
 }
#ifdef _WIN32
 XeenSaveFile::write(path,s);HANDLE lock=CreateFileW(path.c_str(),GENERIC_READ,0,nullptr,OPEN_EXISTING,FILE_ATTRIBUTE_NORMAL,nullptr);
 check(lock!=INVALID_HANDLE_VALUE,"CLI save lock");run("Open save file");CloseHandle(lock);
#else
 if(::geteuid()==0)std::cout<<"SKIP: unreadable-save case; root bypasses file read permissions (geteuid()==0)\n";
 else {
 XeenSaveFile::write(path,s);const auto oldMode=fs::status(path).permissions();
 fs::permissions(path,fs::perms::none);run("Open save file");fs::permissions(path,oldMode);
 }
#endif
 // Unsaveable map exploration still reaches initial-owner loading.
 for(const auto &args:std::vector<std::vector<std::string>>{{"--render-map",game.u8string()},
  {"--render-map",game.u8string(),"1","0","0","north"}})
  check(launch(exe,args,log).exit==3,"valid render syntax rejected");
 std::cout<<"CLI syntax, Unicode paths and production startup failure matrix passed\n";return 0;
}catch(const std::exception &e){std::cerr<<e.what()<<'\n';return 1;}}
