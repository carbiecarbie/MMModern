#include "XeenChildProcessTestSupport.h"
#include <cstdlib>
#include "platform/XeenSaveFile.h"
#include "formats/xeen/XeenSaveFormat.h"
#include "XeenM42Evidence.h"
#include <set>
#include <sstream>
using namespace mmodern;
namespace fs=std::filesystem;
namespace {
std::string bytes(const fs::path &p) {
 std::ifstream in(p,std::ios::binary);
 child_test::require(bool(in),"M38 evidence file absent");
 return {std::istreambuf_iterator<char>(in),{}};
}
// Native input batches and fault/retry frames have different speaking draw
// opportunities. Replay every original Smith request from the source save,
// then retain a complete encoded-state comparison for the finished visit(s).
void equalSmithContinuation(const fs::path &actual,const fs::path &canonical,
 const fs::path &source,const std::string &log) {
 auto expected=XeenSaveFile::read(canonical);const auto before=XeenSaveFile::read(source);
 m42_test::StockOracle oracle{before.journey->random->state,before.journey->random->count,{}};
 std::istringstream lines(log);std::string line,observed;
 while(std::getline(lines,line)) {
  if(!line.empty() && line.back()=='\r')line.pop_back();
  if(line.rfind("DRAW ",0)==0)observed+=line+'\n';
 }
 std::size_t offset=0;
 while(observed.compare(offset,9,"DRAW 5:7:")==0) {
  const auto previous=oracle.trace.size();oracle.draw(5,7);
  const auto predicted=oracle.trace.substr(previous);
  child_test::require(observed.compare(offset,predicted.size(),predicted)==0,"M38 process Smith request/value/cursor differs");
  offset+=predicted.size();
 }
 child_test::require(offset==observed.size(),"M38 process unexpected Smith random request");
 auto random=*before.journey->random;random.state=oracle.state;random.count=oracle.count;
 expected.journey->random=random;
 child_test::require(XeenSaveFormat::encode(expected)==XeenSaveFormat::encode(XeenSaveFile::read(actual)),
  "M38 Smith continuation changed unrelated durable state or predicted RNG");
}
}
int main(int argc,char **argv) {
 try {
  child_test::require(argc==3,"usage: mmodern_smith_process_tests <CLI-witness> <original-installation>");
  const auto exe=fs::absolute(argv[1]),game=fs::absolute(argv[2]);
  const auto dir=child_test::freshDirectory(fs::temp_directory_path()/"mmodern-m38-process");
  std::cout<<"M38 evidence: "<<dir<<'\n';
  SetEnvironmentVariableW(L"SDL_VIDEODRIVER",L"dummy");
  SetEnvironmentVariableW(L"SDL_RENDER_DRIVER",L"software");
  SetEnvironmentVariableW(L"MMODERN_M38_INSTALLATION",game.c_str());
  SetEnvironmentVariableW(L"MMODERN_M38_STAGE",nullptr);
  SetEnvironmentVariableW(L"MMODERN_M38_CONTROL",nullptr);
  const auto route=dir/"production.mmsave";
  const auto fresh=child_test::launch(exe,{L"--journey-region",L"--combat-seed",L"1114",game.wstring(),L"--save-file",route.wstring()},dir/"production.log",false,false,120000);
  child_test::require(fresh.exit==0 && fresh.output.find("M38 PRODUCTION WITNESS PASSED")!=std::string::npos,"M38 production witness failed");
  const auto a=XeenSaveFile::read(dir/"production-A.mmsave");
  child_test::require(a.journey && a.journey->schema==9 && a.journey->content==14 &&
   a.journey->context->day==8 && a.journey->context->minutes==805 &&
   a.journey->treasure->gold==840 && a.characters[6].armor[0].state==128 &&
   a.characters[6].armor[1].state==128,"M38 production-input checkpoint differs from content");
  // Windows may recycle a terminated process PID; creation time identifies its incarnation.
  std::set<std::pair<DWORD,std::uint64_t>> processes{{fresh.pid,fresh.created}};
  const auto run=[&](const std::string &name,const std::string &stage,const fs::path &source,const std::string &control="") {
   const auto path=dir/(name+".mmsave");fs::copy_file(source,path);
   const auto before=bytes(path);
   SetEnvironmentVariableW(L"MMODERN_M38_STAGE",fs::path(stage).c_str());
   SetEnvironmentVariableW(L"MMODERN_M38_CONTROL",control.empty()?nullptr:fs::path(control).c_str());
   const auto result=child_test::launch(exe,{L"--load-game",game.wstring(),path.wstring()},dir/(name+".log"),false,false,120000);
   child_test::require(processes.insert({result.pid,result.created}).second,"M38 process incarnation was reused");
   child_test::require(result.output.find("M38 RESTORE EXACT BEFORE INPUT")!=std::string::npos,"M38 pre-input restore evidence absent");
   if(control.rfind("upload-",0)==0 || control.rfind("copy-",0)==0) {
    child_test::require(result.exit==4 && result.output.find("M38 NATIVE FAILURE PRESERVATION PASSED")!=std::string::npos && before==bytes(path),"M38 native failure crossed publication/save boundary");
   } else if(control.rfind("aba-",0)==0 || control=="immutable-art") {
    child_test::require(result.exit==4 && result.output.find("M38 ABA UNSAVEABLE PRESERVATION PASSED")!=std::string::npos &&
     before==bytes(path),"M38 ABA changed disk, published across boundary or did not fail monotonically");
   } else {
    child_test::require(result.exit==0 && result.output.find("M38 PRODUCTION WITNESS PASSED")!=std::string::npos,"M38 process continuation failed");
   }
   return path;
  };
  for(const auto *stage:{"A","B","C","native"}) {
   const auto source=dir/(std::string("production-")+(std::string(stage)=="native"?"A":stage)+".mmsave");
   const auto name=std::string("restore-")+stage;const auto restored=run(name,stage,source);
   if(std::string(stage)=="native")equalSmithContinuation(restored,route,source,bytes(dir/(name+".log")));
   else child_test::require(bytes(restored)==bytes(route),"M38 uninterrupted/resumed bytes differ");
  }
  for(const auto gold:{0u,1u,0xffffffffu}) for(bool hole:{false,true}) {
   auto artificial=a;artificial.journey->treasure->gold=gold;
   for(unsigned slot=2;slot<9;++slot)artificial.characters[6].armor[slot]={38,1,0xc5,0};
   if(hole)artificial.characters[6].armor[2]={255,0,255,0};
   artificial.characters[29].armor[8]=artificial.characters[6].armor[8];
   bool source=false;
   for(const auto &actor:artificial.journey->actors)if(actor.accounted && actor.id.recordIndex<12) {
    artificial.journey->treasure->armor[0].source=actor.id.recordIndex;
    artificial.journey->treasure->armor[0].item={0,1,0,0};source=true;break;
   }
   child_test::require(source,"M38 synthetic dormant fixture needs a legitimate defeated source");
   const auto name=std::string("synthetic-")+std::to_string(gold)+(hole?"-hole":"-full");
   const auto input=dir/(name+"-input.mmsave");XeenSaveFile::write(input,artificial);
   run(name,gold==0 && hole?"synthetic-native":"synthetic",input);
  }
  run("multi","multi",dir/"production-A.mmsave");
  const auto empty=run("empty","empty",dir/"production-A.mmsave");
  for(const auto *label:{"E","F"})
   child_test::require(bytes(run(std::string("empty-restore-")+label,std::string("empty-")+label,
    dir/(std::string("empty-")+label+".mmsave")))==bytes(empty),"M38 transaction-free branch restore diverged");
  const auto detour=run("detour","detour-A",dir/"production-A.mmsave");
  child_test::require(bytes(run("views-original","views",dir/"production-A.mmsave"))==bytes(dir/"production-A.mmsave"),"M38 original views changed save");
  child_test::require(bytes(run("views-reset","views",dir/"detour-F.mmsave"))==bytes(dir/"detour-F.mmsave"),"M38 reset views changed save");
  for(const auto *label:{"E","F","G"})
   child_test::require(bytes(run(std::string("detour-restore-")+label,std::string("detour-")+label,
    dir/(std::string("detour-")+label+".mmsave")))==bytes(detour),"M38 exit/reset/revisit restore diverged");
  for(const auto *control:{"fail-before-admission","fail-after-admission","fail-quote","fail-before-repair","fail-after-repair",
   "fail-before-departure","fail-after-departure","fail-return","render-admission","render-repair","render-return","recursive"}) {
   const auto source=dir/"production-A.mmsave";const auto restored=run(control,"A",source,control);
   equalSmithContinuation(restored,route,source,bytes(dir/(std::string(control)+".log")));
  }
  for(const auto *control:{"immutable-art","aba-art-provider","aba-quote","aba-departure","aba-inactive","aba-mainland","aba-cache","aba-object-cache","aba-membership","aba-purse","aba-context","aba-city","aba-item","aba-after-repair","upload-admission","upload-repair","upload-departure","copy-admission","copy-repair","copy-departure"})
   run(control,"A",dir/"production-A.mmsave",control);
  std::cout<<"M38 original-resource/native-input/distinct-process acceptance passed\n";
  return 0;
 } catch(const std::exception &e) {std::cerr<<e.what()<<'\n';return 1;}
}
