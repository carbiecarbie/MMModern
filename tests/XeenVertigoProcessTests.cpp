#include "XeenChildProcessTestSupport.h"
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
  a.journey->schema==8 && a.journey->contract==8 && !a.journey->vertigoActors &&
  a.camera.mapId==XeenMapIdentity(23) && a.camera.x==10 && a.camera.y==12,
  "M37 A is not the admitted unvisited mainland");
 child_test::require(b.camera.mapId==XeenMapIdentity(28) && b.camera.x==16 && b.camera.y==2 &&
  b.journey->vertigoActors && b.journey->vertigoActors->size()==46 &&
  b.journey->vertigoActors->at(35).lifecycle==XeenActorLifecycle::Defeated &&
  b.journey->vertigoActors->at(35).accounted &&
  b.journey->vertigoActors->at(36).x==22 && b.journey->vertigoActors->at(36).y==9,
  "M37 B did not retain original slot 35 defeat and distant slot 36");
 child_test::require(c.camera.mapId==XeenMapIdentity(23) && c.camera.x==10 && c.camera.y==12 &&
  c.journey->vertigoActors && c.journey->vertigoActors->size()==52 &&
  !c.gameFlags[9] && c.journey->vertigoActors->at(36).x==15 && c.journey->vertigoActors->at(36).y==4,
  "M37 C did not retain the flag-9-clear original reset");
 static constexpr std::array<std::array<int,2>,43> reset{{
  {{1,11}},{{1,11}},{{2,9}},{{3,10}},{{3,11}},{{3,11}},{{3,13}},{{3,13}},
  {{3,27}},{{4,27}},{{4,26}},{{4,25}},{{4,12}},{{4,7}},{{4,7}},{{4,3}},
  {{4,3}},{{4,3}},{{5,12}},{{9,18}},{{25,14}},{{28,9}},{{30,9}},{{30,6}},
  {{29,15}},{{8,24}},{{8,24}},{{7,23}},{{7,23}},{{8,27}},{{8,27}},{{9,18}},
  {{6,2}},{{7,1}},{{6,6}},{{7,7}},{{15,4}},{{22,9}},{{21,1}},{{22,1}},
  {{30,1}},{{7,24}},{{6,27}}
 }};
 for(unsigned i=0;i<52;++i) {
  const auto &actor=c.journey->vertigoActors->at(i);
  child_test::require(actor.id==XeenMonsterIdentity{28,i} && actor.status==XeenActorStatus::Physical,
   "M37 reset actor identity/status changed");
  if(i>=46 && i<=49) {
   child_test::require(actor.x==0 && actor.y==0 && actor.hp==0 && !actor.activated &&
    actor.lifecycle==XeenActorLifecycle::Unresolved && !actor.accounted,"M37 script gap is materialized");
  } else if(i<=40 || i>=50) {
   const auto expected=reset[i>=50?i-9:i];
   child_test::require(actor.x==expected[0] && actor.y==expected[1] && actor.hp>0 &&
    !actor.activated && actor.lifecycle==XeenActorLifecycle::Present && !actor.accounted,
    "M37 original Spawn table was not fully applied");
  } else {
   const auto &prior=b.journey->vertigoActors->at(i);
   child_test::require(actor.x==prior.x && actor.y==prior.y && actor.hp==prior.hp &&
    actor.activated==prior.activated && actor.lifecycle==prior.lifecycle &&
    actor.accounted==prior.accounted,"M37 untouched original slots 41..45 changed");
  }
 }
 child_test::require(d.camera.mapId==XeenMapIdentity(28) && d.journey->vertigoActors &&
  d.journey->vertigoActors->size()==52 &&
  d.journey->vertigoActors->at(36).lifecycle==XeenActorLifecycle::Defeated &&
  d.journey->vertigoActors->at(36).accounted &&
  d.journey->vertigoActors->at(35).x==7 && d.journey->vertigoActors->at(35).y==7,
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
  const auto fresh=child_test::launch(witness,{L"--journey-region",L"--combat-seed",L"56",game.wstring(),L"--save-file",route.wstring()},dir/"fresh.log");
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
   const auto result=child_test::launch(witness,{L"--load-game",game.wstring(),checkpoint.wstring()},dir/("restore-"+label+".log"));
   child_test::require(result.exit==0 && pids.insert(result.pid).second,"M37 distinct resume process failed");
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
  const auto flag9Run=child_test::launch(witness,{L"--load-game",game.wstring(),flag9Path.wstring()},dir/"flag9.log");
  child_test::require(flag9Run.exit==0 && pids.insert(flag9Run.pid).second &&
   flag9Run.output.find("M37 FLAG9 SKIP RESET passed")!=std::string::npos,
   "M37 artificial flag-9-true process failed");
  const auto skipped=XeenSaveFile::read(flag9Path);
  child_test::require(skipped.gameFlags[9] && skipped.gameFlags[231] &&
   skipped.journey && skipped.journey->vertigoActors &&
   skipped.journey->vertigoActors->size()==46 &&
   std::find(skipped.disabledEvents.begin(),skipped.disabledEvents.end(),XeenEventIdentity{28,764})!=skipped.disabledEvents.end(),
   "M37 flag-9 true did not skip reset while retaining prelude/protection");
  for(unsigned i=0;i<46;++i) {
   const auto &before=flag9.journey->vertigoActors->at(i),&after=skipped.journey->vertigoActors->at(i);
   child_test::require(before.id==after.id && before.x==after.x && before.y==after.y &&
    before.hp==after.hp && before.activated==after.activated &&
    before.lifecycle==after.lifecycle && before.accounted==after.accounted,
    "M37 flag-9 true implicitly reset a city actor");
  }
  const auto rejectRestore=[&](const char *name,const XeenSaveSnapshot &snapshot) {
   const auto path=dir/(std::string("reject-")+name+".mmsave");
   XeenSaveFile::write(path,snapshot);const auto before=bytes(path);
   SetEnvironmentVariableW(L"MMODERN_M37_STAGE",L"B");
   const auto result=child_test::launch(witness,{L"--load-game",game.wstring(),path.wstring()},dir/(std::string("reject-")+name+".log"));
   child_test::require(result.exit!=0 && result.output.find("M37 RESTORE BEFORE INPUT")==std::string::npos &&
    bytes(path)==before,"M37 malformed city save was published or changed on disk");
  };
  auto forged=originalB;forged.journey->vertigoActors->at(36).activated=true;
  rejectRestore("distant-active",forged);
  forged=originalB;auto &slime=forged.journey->vertigoActors->at(35);
  slime.lifecycle=XeenActorLifecycle::Present;slime.x=31;slime.y=31;slime.hp=2;slime.activated=true;slime.accounted=false;
  rejectRestore("outside-closure",forged);
  forged=originalB;auto &contact=forged.journey->vertigoActors->at(35);
  contact.lifecycle=XeenActorLifecycle::Present;contact.x=forged.camera.x;contact.y=forged.camera.y;
  contact.hp=2;contact.activated=true;contact.accounted=false;
  rejectRestore("city-contact",forged);
  forged=originalC;forged.journey->vertigoActors->at(46).hp=1;
  rejectRestore("materialized-gap",forged);
  forged=originalC;forged.disabledEvents.erase(std::remove(forged.disabledEvents.begin(),forged.disabledEvents.end(),XeenEventIdentity{28,764}),forged.disabledEvents.end());
  rejectRestore("missing-protection",forged);
  forged=originalB;forged.camera.x=31;forged.camera.y=31;
  bool refusedCamera=false;
  try {XeenSaveFile::write(dir/"reject-off-route-camera.mmsave",forged);}
  catch(const std::exception &) {refusedCamera=true;}
  child_test::require(refusedCamera,"M37 off-route city camera was admitted by the wire validator");
  for(unsigned mode=0;mode<4;++mode) {
   forged=originalB;
   if(mode==0)forged.disabledEvents.push_back({28,539});
   if(mode==1)forged.disabledEvents.push_back({28,761});
   if(mode==2)forged.disabledObjects.push_back({28,0});
   if(mode==3){forged=originalC;forged.journey->vertigoActors.reset();}
   bool rejected=false;try {XeenSaveFormat::encode(forged);}catch(const std::exception &){rejected=true;}
   child_test::require(rejected,"M37 forged city overlay admitted by codec");
  }
  SetEnvironmentVariableW(L"MMODERN_M37_STAGE",L"fail-city-compose");
  const auto failedTransition=child_test::launch(witness,
   {L"--journey-region",L"--combat-seed",L"56",game.wstring(),L"--save-file",(dir/"failure.mmsave").wstring()},
   dir/"failure.log");
  child_test::require(failedTransition.exit==0 &&
   failedTransition.output.find("M37 DESTINATION COMPOSE FAILURE atomic passed")!=std::string::npos,
   "M37 destination preparation failure published partial state");
  for(const auto stage:{L"candidate-mutation",L"candidate-aba",L"candidate-aba-container",L"candidate-aba-party",
    L"candidate-aba-camera",L"candidate-aba-flags",L"candidate-aba-overlay",L"candidate-aba-rng",
    L"candidate-aba-treasure",L"candidate-cache-rebuild",L"manifest-mismatch"}) {
   SetEnvironmentVariableW(L"MMODERN_M37_STAGE",stage);
   const auto result=child_test::launch(witness,
    {L"--journey-region",L"--combat-seed",L"56",game.wstring(),L"--save-file",(dir/(std::wstring(stage)+L".mmsave")).wstring()},
    dir/(std::wstring(stage)+L".log"));
   const bool cache=std::wstring(stage)==L"candidate-cache-rebuild";
   const auto expected=cache?"M37 CLI SMOKE":std::wstring(stage)==L"manifest-mismatch"?
    "M37 IMMUTABLE INTEGRITY REJECTION passed":"M37 CANDIDATE INTEGRITY REJECTION passed";
   child_test::require(result.exit==0 && result.output.find(expected)!=std::string::npos,
    "M37 retained provider/candidate guard regression failed");
  }
  for(const auto stage:{L"restore-candidate-mutation",L"restore-candidate-aba"}) {
   const auto path=dir/(std::wstring(stage)+L".mmsave");
   XeenSaveFile::write(path,originalB);const auto before=bytes(path);
   SetEnvironmentVariableW(L"MMODERN_M37_STAGE",stage);
   const auto result=child_test::launch(witness,{L"--load-game",game.wstring(),path.wstring()},dir/(std::wstring(stage)+L".log"));
   child_test::require(result.exit!=0 && result.output.find("M37 RESTORE BEFORE INPUT")==std::string::npos && bytes(path)==before,
    "M37 restore composer changed candidate before publication");
  }
  SetEnvironmentVariableW(L"MMODERN_M37_STAGE",nullptr);
  std::cout<<"M37 A/B/C/D original-resource, five-process route passed; logs "<<dir.u8string()<<'\n';
  return 0;
 }catch(const std::exception &e){std::cerr<<e.what()<<'\n';return 1;}
}
