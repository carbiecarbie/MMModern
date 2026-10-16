#include "XeenChildProcessTestSupport.h"
#include "XeenSaveTestSupport.h"
#include "XeenTestInstallation.h"
#include "formats/xeen/XeenAssetSource.h"
#include "games/xeen/XeenEventLoader.h"
#include "games/xeen/XeenEventScript.h"
#include "platform/XeenSaveFile.h"
#include "formats/xeen/XeenSaveFormat.h"
#include <array>
#include <algorithm>
#include <chrono>
#include <set>
#include <string>

namespace fs=std::filesystem;
using namespace mmodern;
namespace {
std::string bytes(const fs::path &path) {
 std::ifstream stream(path,std::ios::binary);
 child_test::require(bool(stream),"M37 checkpoint is absent");
 return {std::istreambuf_iterator<char>(stream),{}};
}
void durableCheckpoints(const fs::path &route) {
 const auto file=[&](char label){return XeenSaveFile::read(route.parent_path()/(route.stem().string()+"-"+label+".mmsave"));};
 const auto a=file('A'),b=file('B'),c=file('C'),d=file('D');
 child_test::require(a.journey && b.journey && c.journey && d.journey &&
  a.journey->schema==9 && a.journey->content==14 && (!a.find(28) || !a.find(28)->monsters) &&
  a.camera.mapId==XeenMapIdentity(23) && a.camera.x==10 && a.camera.y==12,
  "M37 A is not the admitted unvisited mainland");
 child_test::require(b.camera.mapId==XeenMapIdentity(28) && b.camera.x==16 && b.camera.y==2 &&
  b.map(28).monsters && b.map(28).monsters->slots.size()==46 &&
  b.map(28).monsters->slots.at(35).lifecycle==XeenActorLifecycle::Defeated &&
  b.map(28).monsters->slots.at(35).accounted &&
  b.map(28).monsters->slots.at(36).x==22 && b.map(28).monsters->slots.at(36).y==9,
  "M37 B did not retain original slot 35 defeat and distant slot 36");
 child_test::require(c.camera.mapId==XeenMapIdentity(23) && c.camera.x==10 && c.camera.y==12 &&
  c.map(28).monsters && c.map(28).monsters->slots.size()==46 &&
  !c.gameFlags[9] && c.map(28).monsters->slots.at(36).x==15 && c.map(28).monsters->slots.at(36).y==4,
  "M37 C did not retain the flag-9-clear original reset");
 static constexpr std::array<std::array<int,2>,41> reset{{
  {{1,11}},{{1,11}},{{2,9}},{{3,10}},{{3,11}},{{3,11}},{{3,13}},{{3,13}},
  {{3,27}},{{4,27}},{{4,26}},{{4,25}},{{4,12}},{{4,7}},{{4,7}},{{4,3}},
  {{4,3}},{{4,3}},{{5,12}},{{9,18}},{{25,14}},{{28,9}},{{30,9}},{{30,6}},
  {{29,15}},{{8,24}},{{8,24}},{{7,23}},{{7,23}},{{8,27}},{{8,27}},{{9,18}},
  {{6,2}},{{7,1}},{{6,6}},{{7,7}},{{15,4}},{{22,9}},{{21,1}},{{22,1}},
  {{30,1}}
 }};
 // Spawns of slots 50 and 51 at (7,24) and (6,27) are past the 46 MOB records:
 // DOS leaves them inert and never saves them (0x9c59, 0x266e3..0x2671c).
 for(unsigned i=0;i<46;++i) {
  const auto &actor=c.map(28).monsters->slots.at(i);
  child_test::require(actor.status==XeenActorStatus::Physical &&
   actor.spawnedType==-1,"M37 reset actor status/type changed");
  if(i<=40) {
   const auto expected=reset[i];
   child_test::require(actor.x==expected[0] && actor.y==expected[1] &&
    actor.lifecycle==XeenActorLifecycle::Present && !actor.accounted,
    "M37 original Spawn table was not fully applied");
  } else {
   const auto &prior=b.map(28).monsters->slots.at(i);
   child_test::require(actor==prior,"M37 untouched original slots 41..45 changed");
  }
 }
 child_test::require(d.camera.mapId==XeenMapIdentity(28) && d.find(28) && d.map(28).monsters &&
  d.map(28).monsters->slots.size()==46 &&
  d.map(28).monsters->slots.at(36).lifecycle==XeenActorLifecycle::Defeated &&
  d.map(28).monsters->slots.at(36).accounted &&
  d.map(28).monsters->slots.at(35).x==7 && d.map(28).monsters->slots.at(35).y==7,
  "M37 D did not preserve reset slot 36 defeat and displaced slot 35");
}
}
int main(int argc,char **argv) {
 try {
  child_test::require(argc==3,"usage: mmodern_vertigo_process_tests <CLI-witness> <original-installation>");
  const auto witness=fs::absolute(argv[1]),game=fs::absolute(argv[2]);
  const auto dir=fs::temp_directory_path()/("mmodern-m37-process-"+
   std::to_string(GetCurrentProcessId())+"-"+
   std::to_string(std::chrono::steady_clock::now().time_since_epoch().count()));
  fs::create_directories(dir);
  const auto route=dir/"route.mmsave";
  SetEnvironmentVariableW(L"SDL_VIDEODRIVER",L"dummy");
  SetEnvironmentVariableW(L"SDL_RENDER_DRIVER",L"software");
  SetEnvironmentVariableW(L"MMODERN_M37_STAGE",nullptr);
  std::set<DWORD> pids;
  const auto fresh=child_test::launch(witness,{L"--journey-region",L"--combat-seed",L"56",game.wstring(),L"--save-file",route.wstring()},dir/"fresh.log",false,false,120000);
  child_test::require(fresh.exit==0 && fresh.output.find("M37 CLI SMOKE 23,10,12 passed")!=std::string::npos,"M37 uninterrupted route failed");
  pids.insert(fresh.pid);
  durableCheckpoints(route);
  const auto originalB=XeenSaveFile::read(route.parent_path()/(route.stem().string()+"-B.mmsave"));
  const auto originalC=XeenSaveFile::read(route.parent_path()/(route.stem().string()+"-C.mmsave"));
  const auto final=bytes(route);
  for(const char stage:{'A','B','C','D'}) {
   const std::string label(1,stage);
   const auto checkpoint=dir/("route-"+label+".mmsave");
   const auto before=bytes(checkpoint);
   SetEnvironmentVariableW(L"MMODERN_M37_STAGE",fs::path(label).c_str());
   const auto result=child_test::launch(witness,{L"--load-game",game.wstring(),checkpoint.wstring()},dir/("restore-"+label+".log"),false,false,120000);
   child_test::require(result.exit==0 && pids.insert(result.pid).second,"M37 distinct resume process failed");
   child_test::require(stage!='A' || result.output.find("M37 DECISION8 A Orc 9 rebuilt asleep")!=std::string::npos,
    "M37 A restore did not show the decision 8 rebuild");
   child_test::require(result.output.find("M37 RESTORE BEFORE INPUT")!=std::string::npos &&
    result.output.find("M37 CLI SMOKE 23,10,12 passed")!=std::string::npos,"M37 restore/continuation witness missing");
   child_test::require(bytes(checkpoint)==final && before!=final,"M37 uninterrupted/resumed durable bytes differ");
   std::cout<<"M37 distinct restore "<<stage<<" PID="<<result.pid<<" final bytes="<<final.size()<<'\n';
  }
  // Artificial flag-9-true predicate control. It starts from a real B save;
  // only the separately documented city discovery flag is supplied here.
  auto flag9=originalB;flag9.gameFlags[9]=true;
  const auto flag9Path=dir/"flag9.mmsave";XeenSaveFile::write(flag9Path,flag9);
  SetEnvironmentVariableW(L"MMODERN_M37_STAGE",L"B9");
  const auto flag9Run=child_test::launch(witness,{L"--load-game",game.wstring(),flag9Path.wstring()},dir/"flag9.log",false,false,120000);
  child_test::require(flag9Run.exit==0 && pids.insert(flag9Run.pid).second &&
   flag9Run.output.find("M37 FLAG9 SKIP RESET passed")!=std::string::npos,
   "M37 artificial flag-9-true process failed");
  const auto skipped=XeenSaveFile::read(flag9Path);
  child_test::require(skipped.gameFlags[9] && skipped.gameFlags[231] &&
   skipped.journey && skipped.find(28) && skipped.map(28).monsters &&
   skipped.map(28).monsters->slots.size()==46 &&
   // CD maze0028.evt removes the exit protection and its self-disable
   // at (15,0,South,4/5), so the flag-9 skip publishes no Event overlay.
   save_test::disabledEvents(skipped).empty(),
   "M37 flag-9 true did not skip reset while retaining the CD exit prelude");
  for(unsigned i=0;i<46;++i) {
   const auto &before=flag9.map(28).monsters->slots.at(i),&after=skipped.map(28).monsters->slots.at(i);
   child_test::require(before==after,
    "M37 flag-9 true implicitly reset a city actor");
  }
  const auto rejectRestore=[&](const char *name,const XeenSaveSnapshot &snapshot) {
   const auto path=dir/(std::string("reject-")+name+".mmsave");
   try {XeenSaveFile::write(path,snapshot);}
   catch(const std::exception &) {child_test::require(!fs::exists(path),"M37 rejected wire state created a file");return;}
   const auto before=bytes(path);
   SetEnvironmentVariableW(L"MMODERN_M37_STAGE",L"B");
   const auto result=child_test::launch(witness,{L"--load-game",game.wstring(),path.wstring()},dir/(std::string("reject-")+name+".log"),false,false,120000);
   child_test::require(result.exit!=0 && result.output.find("M37 RESTORE BEFORE INPUT")==std::string::npos &&
    bytes(path)==before,"M37 malformed city save was published or changed on disk");
  };
  // Whole-map admission permits distant activation and every in-bounds cell.
  // Keep malformed-state refusal controls on structural/resource invariants.
  // v13 identities are positional; a MOB record cannot claim a spawned type.
  auto forged=originalB;forged.map(28).monsters->slots.at(36).spawnedType=0;
  rejectRestore("original-record-type",forged);
  forged=originalB;auto &slime=forged.map(28).monsters->slots.at(35);
  slime.lifecycle=XeenActorLifecycle::Present;slime.x=32;slime.y=31;slime.accounted=false;
  rejectRestore("out-of-map-actor",forged);
  forged=originalB;auto &contact=forged.map(28).monsters->slots.at(35);
  contact.lifecycle=XeenActorLifecycle::Present;contact.x=forged.camera.x;contact.y=forged.camera.y;
  contact.accounted=false;
  rejectRestore("city-contact",forged);
  // The saved original-record count binds to MOB, independently of history.
  forged=originalC;forged.map(28).monsters->originalCount=45;
  rejectRestore("original-count-resource-mismatch",forged);
  forged=originalB;forged.camera.x=32;forged.camera.y=31;
  bool refusedCamera=false;
  try {XeenSaveFile::write(dir/"reject-out-of-map-camera.mmsave",forged);}
  catch(const std::exception &) {refusedCamera=true;}
  child_test::require(refusedCamera,"M37 out-of-map city camera was admitted by the wire validator");
  // CD removed the implicit protection overlay. Bind malformed-overlay
  // controls to retained CD instructions, including an explicit reference for
  // the missing-city-actors case, preserving that same integrity check.
  const auto installation=xeenTestInstallationDetector().detect(game);
  child_test::require(bool(installation),"CD installation for overlay controls is missing");
  XeenAssetSource assets(*installation);
  const XeenEventScript city(XeenEventLoader([&](const std::string &name)->std::optional<std::vector<std::uint8_t>> {
   return assets.readInitialResource(name);
  }).load(28));
  const auto exitEntry=city.findInstructionIndex(15,0,XeenDirection::South,0);
  const auto exitLabel=city.findInstructionIndex(15,0,XeenDirection::South,1);
  child_test::require(exitEntry && exitLabel,"CD exit instructions for overlay controls are missing");
  for(unsigned mode=0;mode<4;++mode) {
   forged=originalB;
   if(mode==0){save_test::addDisabledEvent(forged,{28,*exitEntry});save_test::addDisabledEvent(forged,{28,*exitEntry});}
   if(mode==1){save_test::addDisabledEvent(forged,{28,*exitLabel});save_test::addDisabledEvent(forged,{28,*exitLabel});}
   if(mode==2)save_test::addRemovedObject(forged,{28,0});
   if(mode==3){forged=originalC;save_test::addDisabledEvent(forged,{28,*exitEntry});forged.map(28).monsters.reset();}
   // Generic Event protection may name any record, while duplicate identities
   // remain invalid. v13 is map-agnostic: a city object removal and city Event
   // overrides without a city monster table are representable; restore binds
   // them to the city's MOB/EVT and needs the camera map's table.
   bool rejected=false;try {XeenSaveFormat::encode(forged);}catch(const std::exception &){rejected=true;}
   child_test::require(rejected==(mode<2),"M37 city overlay codec admission differs");
   if(mode==3 && forged.camera.mapId==XeenMapIdentity(28))rejectRestore("city-camera-without-table",forged);
  }
  SetEnvironmentVariableW(L"MMODERN_M37_STAGE",L"fail-city-compose");
  const auto failedTransition=child_test::launch(witness,
   {L"--journey-region",L"--combat-seed",L"56",game.wstring(),L"--save-file",(dir/"failure.mmsave").wstring()},
   dir/"failure.log",false,false,120000);
  child_test::require(failedTransition.exit==0 &&
   failedTransition.output.find("M37 DESTINATION COMPOSE FAILURE atomic passed")!=std::string::npos,
   "M37 destination preparation failure published partial state");
  for(const auto stage:{L"candidate-mutation",L"candidate-aba",L"candidate-aba-container",L"candidate-aba-party",
    L"candidate-aba-camera",L"candidate-aba-flags",L"candidate-aba-overlay",L"candidate-aba-rng",
    L"candidate-aba-treasure",L"candidate-cache-rebuild"}) {
   SetEnvironmentVariableW(L"MMODERN_M37_STAGE",stage);
   const auto result=child_test::launch(witness,
    {L"--journey-region",L"--combat-seed",L"56",game.wstring(),L"--save-file",(dir/(std::wstring(stage)+L".mmsave")).wstring()},
    dir/(std::wstring(stage)+L".log"),false,false,120000);
   const bool cache=std::wstring(stage)==L"candidate-cache-rebuild";
   const auto expected=cache?"M37 CLI SMOKE":"M37 CANDIDATE INTEGRITY REJECTION passed";
   child_test::require(result.exit==0 && result.output.find(expected)!=std::string::npos,
    "M37 retained provider/candidate guard regression failed");
  }
  for(const auto stage:{L"restore-candidate-mutation",L"restore-candidate-aba"}) {
   const auto path=dir/(std::wstring(stage)+L".mmsave");
   XeenSaveFile::write(path,originalB);const auto before=bytes(path);
   SetEnvironmentVariableW(L"MMODERN_M37_STAGE",stage);
   const auto result=child_test::launch(witness,{L"--load-game",game.wstring(),path.wstring()},dir/(std::wstring(stage)+L".log"),false,false,120000);
   child_test::require(result.exit!=0 && result.output.find("M37 RESTORE BEFORE INPUT")==std::string::npos && bytes(path)==before,
    "M37 restore composer changed candidate before publication");
  }
  SetEnvironmentVariableW(L"MMODERN_M37_STAGE",nullptr);
  std::cout<<"M37 A/B/C/D original-resource, five-process route passed\n";
  child_test::removePassedRun(dir);return 0;
 }catch(const std::exception &e){std::cerr<<e.what()<<'\n';return 1;}
}
