#include "formats/xeen/XeenAssetSource.h"
#include "app/XeenAudio.h"
#include "app/XeenFmSource.h"
#include "compat/scummvm/XeenMt32.h"
#include "games/xeen/PromusDriver.h"
#include "games/xeen/XeenInstallationDetector.h"
#include "audio/softsynth/mt32/File.h"
#include "audio/softsynth/mt32/ROMInfo.h"
#include <algorithm>
#include <cassert>
#include <chrono>
#include <cmath>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <map>
#include <sstream>
#include <stdexcept>
#include <vector>
using namespace mmodern;
using Bytes=std::vector<std::uint8_t>;
static PromusDriver::Bytes data(Bytes value){return std::make_shared<const Bytes>(std::move(value));}
struct Captured {std::uint64_t ms;Bytes bytes;};
struct CapturedEffect {std::uint64_t ms;unsigned id;};
static constexpr std::array<CapturedEffect,22> townEffects{{{73988,8},{74217,7},{74505,26},{74630,26},{75274,100},{75604,65},{75713,4},
 {83604,8},{83933,7},{104693,8},{105021,47},{107332,8},{107661,7},{107875,8},{109487,39},
 {111070,7},{111399,8},{111841,7},{112055,8},{112497,7},{112712,8},{112940,7}}};
static std::uint32_t be32(const Bytes &b,std::size_t at) {
 if(at>b.size() || b.size()-at<4)throw std::runtime_error("Truncated MIDI header");
 return unsigned(b[at])<<24|unsigned(b[at+1])<<16|unsigned(b[at+2])<<8|b[at+3];
}
static unsigned vlq(const Bytes &b,std::size_t &at) {
 unsigned result=0;
 for(unsigned i=0;i<4;++i){if(at>=b.size())throw std::runtime_error("Truncated MIDI quantity");const auto c=b[at++];result=(result<<7)|(c&127);if(!(c&128))return result;}
 throw std::runtime_error("Invalid MIDI quantity");
}
static std::vector<Captured> midi(const std::filesystem::path &path) {
 std::ifstream in(path,std::ios::binary);Bytes b((std::istreambuf_iterator<char>(in)),{});
 if(b.size()<22 || std::string(b.begin(),b.begin()+4)!="MThd" || be32(b,4)!=6 || b[8] || b[9] || b[10] || b[11]!=1 || b[12]!=1 || b[13]!=0xf4 ||
    std::string(b.begin()+14,b.begin()+18)!="MTrk" || be32(b,18)!=b.size()-22)throw std::runtime_error("Invalid M94 MIDI capture");
 std::size_t at=22;std::uint64_t ms=0;unsigned running=0;std::vector<Captured> out;
 while(at<b.size()) {
  ms+=vlq(b,at);if(at>=b.size())throw std::runtime_error("Truncated MIDI event");
  unsigned status=b[at];if(status&128){++at;if(status<0xf0)running=status;}else status=running;
  if(status==0xff){if(at>=b.size())throw std::runtime_error("Truncated MIDI meta event");++at;const auto n=vlq(b,at);if(n>b.size()-at)throw std::runtime_error("Truncated MIDI meta data");at+=n;continue;}
  if(status==0xf0 || status==0xf7){const auto n=vlq(b,at);if(n>b.size()-at)throw std::runtime_error("Truncated MIDI SysEx");Bytes e{std::uint8_t(status)};e.insert(e.end(),b.begin()+at,b.begin()+at+n);at+=n;out.push_back({ms,std::move(e)});continue;}
  if(status<0x80 || status>=0xf0)throw std::runtime_error("Unsupported MIDI status");
  const unsigned n=(status&0xe0)==0xc0?1:2;if(n>b.size()-at)throw std::runtime_error("Truncated MIDI channel event");
  Bytes e{std::uint8_t(status)};e.insert(e.end(),b.begin()+at,b.begin()+at+n);at+=n;out.push_back({ms,std::move(e)});
 }
 return out;
}
static Bytes message(const PromusDriver::MidiWrite &m){return Bytes(m.bytes.begin(),m.bytes.begin()+m.size);}
#ifdef MMODERN_ROLMUS_ORACLE
namespace mmodern {
extern unsigned rolmusOracleBX;
struct RolmusOracleTestAccess {
 static const std::vector<std::uint8_t> &image(const PromusDriver &d){return d._image;}
 static int remaining(const PromusDriver &d){return d._fadeRemaining;}
 static bool mpuIdle(const PromusDriver &d){return !d._mpuReset && d._mpuPending<0 && d._mpuCount==0;}
 static void unexpectedReply(PromusDriver &d){d.rolMpuReply(0x12);}
 static void stuckPending(PromusDriver &d){d._mpuPending=0xd0;d._mpuReset=false;}
 static bool streamDue(const PromusDriver &d) {
  const auto due=[](const auto &s){return s.playing && s.countdown<=1;};return due(d._music) || due(d._fx);
 }
 static void rebindFx(PromusDriver &d){if(d._fx.bytes)d._fx.bytes=PromusDriver::Bytes(d._resource,&d._image);}
};
}
struct OracleEvent {std::uint64_t tick;bool interrupt;Bytes bytes;};
static std::vector<Captured> capturedBetween(const std::vector<Captured> &all,std::uint64_t first,std::uint64_t end) {
 std::vector<Captured> result;for(const auto &e:all)if(e.ms>=first && e.ms<end)result.push_back(e);return result;
}
static void compareMessages(const std::vector<OracleEvent> &actual,const std::vector<Captured> &expected,const char *name,bool checkTicks=false,std::uint64_t tickBase=0,bool exe=false) {
 const double period=16390.0/1193182*1000;
 const auto n=std::min(actual.size(),expected.size());
 for(std::size_t i=0;i<n;++i) {
  const auto tick=exe?std::uint64_t(std::llround(expected[i].ms/period)):
   expected[i].ms>=25314?std::uint64_t(std::llround((expected[i].ms-25314)/period))+1+tickBase:0;
  if(actual[i].bytes!=expected[i].bytes || (checkTicks && actual[i].interrupt && actual[i].tick!=tick)) {
   std::ostringstream o;o<<name<<" mismatch at "<<i<<" capture "<<expected[i].ms<<" ms / tick "<<tick<<" actual tick "<<actual[i].tick<<" capture:";
   for(auto b:expected[i].bytes)o<<' '<<unsigned(b);o<<" actual:";for(auto b:actual[i].bytes)o<<' '<<unsigned(b);
   throw std::runtime_error(o.str());
  }
 }
 if(actual.size()!=expected.size())throw std::runtime_error(std::string(name)+" event count differs: "+std::to_string(actual.size())+" versus "+std::to_string(expected.size()));
}
static std::vector<Bytes> fadePreview(const PromusDriver &driver,unsigned index) {
 const auto &image=RolmusOracleTestAccess::image(driver);std::vector<Bytes> out;
 if(!driver.fadeSpeed() || driver.fadePhase()+driver.fadeSpeed()<256)return out;
 const auto stop=[&](){for(int v=8;v>=0;--v)out.push_back({std::uint8_t(0xb1+v),123,0});};
 if(RolmusOracleTestAccess::remaining(driver)-1<0){stop();return out;}
 const auto active=[](unsigned value){return value>=40 && value<128;};
 const unsigned original=image.at(0x14d+index);bool quiet=true;
 if(active(original)){out.push_back({0xb9,11,std::uint8_t(original-1)});quiet=false;}
 for(int v=6;v>=0;--v) {
  unsigned value=image.at(0x14d+v);if(index==unsigned(v) && active(original))--value;
  if(active(value)){out.push_back({std::uint8_t(0xb1+v),11,std::uint8_t(value-1)});quiet=false;}
 }
 if(quiet)stop();return out;
}
static void fadeTick(PromusDriver &driver,std::vector<OracleEvent> &events,const std::vector<Captured> &expected,bool mutation,std::uint64_t tickBase) {
 if(RolmusOracleTestAccess::streamDue(driver)) {
  if(mutation)throw std::runtime_error("Inferred mutation was not on an idle fade carry");
  events.clear();driver.tick();compareMessages(events,expected,"fade stream tick",true,tickBase);return;
 }
 std::vector<Bytes> wanted;for(const auto &e:expected)wanted.push_back(e.bytes);
 const auto &image=RolmusOracleTestAccess::image(driver);
 std::vector<unsigned> candidates;if(mutation)candidates.push_back(0x146c);
 else {
  for(unsigned i=0;i<=8;++i)candidates.push_back(i);
  // This code is used only during driver initialization, before the compared
  // segment. Other foreground BX values in this fade are not identifiable.
  for(unsigned at=0x1c0;at<0x335;++at)candidates.push_back(at-0x14d);
  // Init-upload tables are likewise not read again before driver reload.
  for(unsigned at=0xa86;at<0x10eb;++at)candidates.push_back(at-0x14d);
 }
 const auto found=std::find_if(candidates.begin(),candidates.end(),[&](unsigned i){return fadePreview(driver,i)==wanted;});
 if(found==candidates.end())throw std::runtime_error("No single foreground-BX fade access explains capture tick "+std::to_string(driver.ticks()+1));
 const auto before=mutation?image.at(0x15b9):0;
 if(mutation && (before<40 || before>=128 || (before&15)!=7))
  throw std::runtime_error("FX 26 opcode cannot undergo the inferred decrement");
 rolmusOracleBX=*found;events.clear();driver.tick();compareMessages(events,expected,"fade",true,tickBase);
 if(mutation && image.at(0x15b9)!=before-1)throw std::runtime_error("The inferred carry did not decrement the FX 26 opcode");
}
static void captureOracle(XeenAssetSource &assets,PromusDriver::Bytes resource,const std::vector<Captured> &captured) {
 std::vector<OracleEvent> events;
 PromusDriver d(resource,{},4,[&](const auto &e){events.push_back({e.tick,e.interrupt,message(e)});});
 if(!RolmusOracleTestAccess::mpuIdle(d))throw std::runtime_error("MPU reset left an unconsumed response or pending command");
 compareMessages(events,std::vector<Captured>(captured.begin(),captured.begin()+22),"EXE initialization");
 struct ExeEffect {unsigned beforeTick,id;};
 const std::array<ExeEffect,9> intro{{{38,1},{502,40},{566,0},{694,3},{718,3},{742,2},{1494,60},{1662,60},{1838,0}}};
 std::size_t introNext=0;
 while(d.ticks()<1839) {
  d.tick();
  if(d.ticks()==846)d.command(0x100,95);
  if(d.ticks()==865)d.command(0);
  if(introNext<intro.size() && d.ticks()==intro[introNext].beforeTick)d.effect(intro[introNext++].id);
 }
 if(introNext!=intro.size())throw std::runtime_error("Not all captured EXE effects were replayed");
 d.command(0);d.restartProgram();
 if(events.size()<22)throw std::runtime_error("Missing DAT initialization");
 const auto dat=events.end()-22;
 compareMessages(std::vector<OracleEvent>(events.begin(),dat),capturedBetween(captured,0,25301),"EXE from first MIDI message",true,0,true);
 compareMessages(std::vector<OracleEvent>(dat,events.end()),capturedBetween(captured,25301,25305),"DAT initialization");
 for(const auto span:std::array<std::pair<unsigned,unsigned>,5>{{{25305,25306},{25307,25314},{69380,69470},{70481,70662},{116031,116088}}})
  if(!capturedBetween(captured,span.first,span.second).empty())throw std::runtime_error("Unexpected MIDI in a capture gap");
 if(std::any_of(captured.begin(),captured.end(),[](const auto &e){return e.ms<69353 && e.bytes==Bytes{0xc8,5};}))
  throw std::runtime_error("Capture contains an earlier FX 26 program message");
 const auto base=d.ticks();
 events.clear();d.load(data(assets.readArchiveResource("newbrigh.m",XeenSceneArchive::DarksideOnly)));
 compareMessages(events,capturedBetween(captured,25306,25307),"title load");
 events.clear();for(unsigned i=0;i<3205;++i)d.tick();
 compareMessages(events,capturedBetween(captured,25314,69353),"title from first note",true,base);
 const double period=16390.0/1193182*1000;
 const auto tickOf=[&](std::uint64_t ms){return std::uint64_t(std::llround((ms-25314)/period))+1;};
 const auto group=[&](std::uint64_t t,std::uint64_t lo,std::uint64_t hi) {
  std::vector<Captured> result;for(const auto &e:captured)if(e.ms>=lo && e.ms<hi && tickOf(e.ms)==t)result.push_back(e);return result;
 };
 d.command(207);
 std::size_t compared=0;
 for(std::uint64_t t=3206;t<=3208;++t){const auto expected=group(t,69340,69379);compared+=expected.size();fadeTick(d,events,expected,false,base);}
 if(compared!=capturedBetween(captured,69340,69379).size())throw std::runtime_error("Incomplete first-fade capture projection");
 events.clear();d.command(0);compareMessages(events,capturedBetween(captured,69379,69380),"interrupted title fade");
 while(d.ticks()<base+3215)d.tick();d.command(207);
 compared=0;
 for(std::uint64_t t=3216;t<=3289;++t){const auto expected=group(t,69470,70481);compared+=expected.size();fadeTick(d,events,expected,t==3288,base);}
 if(compared!=capturedBetween(captured,69470,70481).size())throw std::runtime_error("Incomplete follow-on fade capture projection");
 if(d.fadeSpeed())throw std::runtime_error("Captured fade did not finish on its recorded tick");
 while(d.ticks()<base+3302)d.tick();events.clear();
 d.load(data(assets.readArchiveResource("town.m",XeenSceneArchive::Clouds)));
 std::size_t next=0;
 while(d.ticks()<base+6463) {
  d.tick();
  while(next<townEffects.size() && std::uint64_t((townEffects[next].ms-25314)/period)+1+base==d.ticks())d.effect(townEffects[next++].id);
 }
 if(next!=townEffects.size())throw std::runtime_error("Not all captured town effects were replayed");
 compareMessages(events,capturedBetween(captured,70662,114093),"town and FX after inferred decrement",true,base);
 d.command(207);
 if(std::none_of(captured.begin(),captured.end(),[](const auto &e){return e.ms==114093 && e.bytes==Bytes{0xb7,11,84};}))
  throw std::runtime_error("Capture lacks the channel-7 first fade value");
 // The production BX=8 rule is compared separately. Channel 9 and each music
 // channel become excluded only once captured BX has changed that cache.
 auto fixed=d;RolmusOracleTestAccess::rebindFx(fixed);rolmusOracleBX=8;
 // Channel 9 and channels whose expression caches were altered by captured
 // foreground BX are excluded; independent channels, notes and stop remain.
 const auto independent=[&](const Bytes &b,std::uint64_t tick) {
  return b.size()>=2 && ((b[0]>=0x80 && b[0]<=0x9f) ||
   (b.size()==3 && b[1]==11 && (b[0]==0xb2 || b[0]==0xb4 || b[0]==0xb6 ||
     (b[0]==0xb3 && tick<tickOf(114299)) || (b[0]==0xb5 && tick<tickOf(114642)))) ||
   (b.size()==3 && b[1]==123 && b[0]>=0xb1 && b[0]<=0xb9));
 };
 for(std::uint64_t t=6464;t<=6605;++t) {
  events.clear();fixed.tick();const auto all=group(t,114093,116031);
  std::vector<OracleEvent> actual;std::vector<Captured> expected;
  for(const auto &e:events)if(independent(e.bytes,t))actual.push_back(e);
  for(const auto &e:all)if(independent(e.bytes,t))expected.push_back(e);
  compareMessages(actual,expected,"fixed-BX independent fade",true,base);
 }
 if(fixed.fadeSpeed())throw std::runtime_error("Fixed-BX fade did not finish on captured tick 6605");
 while(fixed.ticks()<base+6609)fixed.tick();events.clear();
 fixed.load(data(assets.readArchiveResource("outdoors.m",XeenSceneArchive::Clouds)));fixed.tick();
 compareMessages(events,capturedBetween(captured,116088,116100),"fixed-BX outdoor load and first note",true,base);
 events.clear();while(fixed.ticks()<base+8524)fixed.tick();
 compareMessages(events,capturedBetween(captured,116100,142391),"fixed-BX outdoor continuation",true,base);
 compared=0;
 for(std::uint64_t t=6464;t<=6605;++t){
  const auto expected=group(t,114093,116031);compared+=expected.size();fadeTick(d,events,expected,false,base);
  if(!expected.empty() && ((expected.front().ms==114299 && rolmusOracleBX!=2) || (expected.front().ms==114642 && rolmusOracleBX!=4)))
   throw std::runtime_error("A captured foreground-BX fade selection was not reproduced");
 }
 if(compared!=capturedBetween(captured,114093,116031).size())throw std::runtime_error("Incomplete town-fade capture projection");
 if(d.fadeSpeed())throw std::runtime_error("Town fade did not finish on captured tick 6605");
 // The capture shows four foreground ticks between fade stop and load traffic;
 // this is the replayed caller schedule, not an additional driver wait.
 while(d.ticks()<base+6609)d.tick();events.clear();
 d.load(data(assets.readArchiveResource("outdoors.m",XeenSceneArchive::Clouds)));d.tick();
 compareMessages(events,capturedBetween(captured,116088,116100),"outdoor load and first note",true,base);
 events.clear();while(d.ticks()<base+8524)d.tick();
 compareMessages(events,capturedBetween(captured,116100,142391),"outdoor continuation to capture end",true,base);
 rolmusOracleBX=8;
 {
  PromusDriver unexpected(resource,{},4);RolmusOracleTestAccess::unexpectedReply(unexpected);
  bool stopped=false;try{unexpected.command(0);}catch(const std::runtime_error &e){stopped=std::string(e.what()).find("Unexpected MPU response")!=std::string::npos;}
  if(!stopped)throw std::runtime_error("Unexpected MPU response was accepted");
  PromusDriver timeout(resource,{},4);RolmusOracleTestAccess::stuckPending(timeout);
  const auto tick=timeout.ticks();stopped=false;
  try{timeout.command(0);}catch(const std::runtime_error &e){stopped=std::string(e.what()).find("polling counter exhausted")!=std::string::npos;}
  if(!stopped || timeout.ticks()!=tick)throw std::runtime_error("MPU polling was unbounded or advanced the game clock");
 }
}
#endif
static std::map<std::string,std::filesystem::path> romFiles(const std::filesystem::path &directory) {
 std::map<std::string,std::filesystem::path> result;
 for(const auto &entry:std::filesystem::directory_iterator(directory)) {
  if(!entry.is_regular_file() || entry.file_size()>1048576)continue;
  std::ifstream in(entry.path(),std::ios::binary);Bytes bytes((std::istreambuf_iterator<char>(in)),{});
  MT32Emu::ArrayFile file(bytes.data(),bytes.size());
  if(const auto *info=MT32Emu::ROMInfo::getROMInfo(&file))result[info->shortName]=entry.path();
 }
 return result;
}
namespace mmodern {struct XeenMt32TestAccess {
 static std::unique_ptr<XeenMt32> open(std::vector<std::filesystem::path> paths) {return std::unique_ptr<XeenMt32>(new XeenMt32(paths));}
};}
static void romTests(const std::filesystem::path &directory,PromusDriver::Bytes resource) {
 const auto files=romFiles(directory);
 assert(files.count("ctrl_mt32_1_07") && files.count("pcm_mt32"));
 const auto open=[](std::vector<std::filesystem::path> paths){return XeenMt32TestAccess::open(std::move(paths));};
 {auto missing=open({});assert(!missing->available() && missing->notice().find("not supported yet")!=std::string::npos);}
 {auto incomplete=open({files.at("ctrl_mt32_1_07")});assert(!incomplete->available() && !incomplete->notice().empty());}
 {auto correct=open({files.at("ctrl_mt32_1_07"),files.at("pcm_mt32")});assert(correct->available() && correct->notice().empty());}
 {XeenMt32 scanned(directory);assert(scanned.available() && scanned.notice().empty());}
#ifndef MMODERN_ROLMUS_ORACLE
 // Hard links exercise arbitrary file names without copying ROM contents.
 // A restricted sandbox or a different-volume TEMP may forbid hard links;
 // the required unsandboxed suite exercises this on the maintainer's setup.
 {
  struct Links {
   std::filesystem::path dir,a,b;bool owned=false;
   ~Links(){if(!owned)return;std::error_code ec;std::filesystem::remove(a,ec);std::filesystem::remove(b,ec);std::filesystem::remove(dir,ec);}
  } links;
  links.dir=std::filesystem::temp_directory_path()/("mmodern-mt32-names-"+std::to_string(std::chrono::steady_clock::now().time_since_epoch().count()));
  std::error_code ec;
  if(std::filesystem::create_directory(links.dir,ec)) {
   links.owned=true;
   links.a=links.dir/"first.arbitrary";links.b=links.dir/"second.arbitrary";
   std::filesystem::create_hard_link(files.at("ctrl_mt32_1_07"),links.a,ec);
   if(!ec)std::filesystem::create_hard_link(files.at("pcm_mt32"),links.b,ec);
   if(!ec){XeenMt32 renamed(links.dir);assert(renamed.available());std::cout<<"Arbitrary ROM file names accepted\n";}
  }
 }
#endif
 for(const auto &wrong:{"ctrl_mt32_1_06","ctrl_cm32l_1_02"})if(files.count(wrong)) {
  auto refused=open({files.at(wrong),files.at("pcm_mt32")});
  assert(!refused->available() && refused->notice().find("not supported yet")!=std::string::npos);
 }
 if(files.count("ctrl_mt32_1_07_a") && files.count("ctrl_mt32_1_07_b") && files.count("pcm_mt32_l") && files.count("pcm_mt32_h")) {
  auto split=open({files.at("ctrl_mt32_1_07_a"),files.at("ctrl_mt32_1_07_b"),files.at("pcm_mt32_l"),files.at("pcm_mt32_h")});
  assert(split->available());
 }
 // Expression is synth state that init does not write. A quiet expression
 // followed by a handoff must still affect a subsequent note; a new synth
 // would sound like the control, which has the default expression.
 std::vector<PromusDriver::MidiWrite> traffic;
 XeenFmSource source({},directory,[&](const auto &event){traffic.push_back(event);});
 XeenFmSource control({},directory);
 source.initialize(resource,4);assert(source.notice().empty() && traffic.size()==22);
 control.initialize(resource,4);assert(control.notice().empty());
 source.load(data({0xa0,0,40,0x10,255,0xff}),true);
 std::vector<std::int16_t> pcm(4800*2),controlPcm(4800*2);
 source.pull(pcm.data(),4800);control.pull(controlPcm.data(),4800);source.check();control.check();
 const auto before=traffic.size();source.restartProgram();control.restartProgram();
 const auto note=data({0x90,0x20,127,0x10,255,0xff});source.load(note,true);control.load(note,true);
 source.pull(pcm.data(),4800);control.pull(controlPcm.data(),4800);source.check();control.check();
 assert(traffic.size()>=before+32);
 for(std::size_t i=0;i<22;++i)assert(message(traffic[before+10+i])==message(traffic[i]));
 std::uint64_t energy=0,controlEnergy=0;
 for(auto sample:pcm)energy+=std::abs(int(sample));
 for(auto sample:controlPcm)controlEnergy+=std::abs(int(sample));
 assert(energy && energy<controlEnergy);
 if(source.midiDeferrals() || control.midiDeferrals())throw std::runtime_error("Ordinary MT-32 handoff deferred an operation");
 XeenFmSource silent({},directory/"mmodern-nonexistent-rom-fixture");silent.initialize(resource,4);
 assert(!silent.notice().empty());silent.load(data({0x90,0x20,127,0x10,255,0xff}),true);
 silent.pull(pcm.data(),4800);silent.check();
 assert(std::all_of(pcm.begin(),pcm.end(),[](auto s){return s==0;}));
}
#ifndef MMODERN_ROLMUS_ORACLE
static void fixedCaptureOracle(XeenAssetSource &assets,PromusDriver::Bytes resource,const std::vector<Captured> &captured) {
 std::vector<Bytes> writes;bool collect=false;
 PromusDriver d(resource,{},4,[&](const auto &e){if(collect)writes.push_back(message(e));});
 d.load(data(assets.readArchiveResource("newbrigh.m",XeenSceneArchive::DarksideOnly)));
 while(d.ticks()<3205)d.tick();
 const double period=16390.0/1193182*1000;
 const auto tickOf=[&](std::uint64_t ms){return std::uint64_t(std::llround((ms-25314)/period))+1;};
 // The title capture first selects BX=3, then BX=0, 4 and 6 in the
 // follow-on fade. Exclude each affected expression channel only from its
 // first such carry; channel 9 directly reflects the foreground selection.
 const auto titleIndependent=[&](const Bytes &b,std::uint64_t tick) {
  if(b.size()!=3 || b[1]!=11 || (b[0]&0xf0)!=0xb0)return true;
  return b[0]!=0xb9 && b[0]!=0xb4 &&
   !(b[0]==0xb1 && tick>=tickOf(69477)) &&
   !(b[0]==0xb5 && tick>=tickOf(69518)) &&
   !(b[0]==0xb7 && tick>=tickOf(69532));
 };
 const auto titleTick=[&](std::uint64_t tick,std::uint64_t lo,std::uint64_t hi) {
  writes.clear();d.tick();std::vector<Bytes> actual,expected;
  for(const auto &b:writes)if(titleIndependent(b,tick))actual.push_back(b);
  for(const auto &e:captured)if(e.ms>=lo && e.ms<hi && tickOf(e.ms)==tick && titleIndependent(e.bytes,tick))expected.push_back(e.bytes);
  if(actual!=expected)throw std::runtime_error("Production fixed-BX title fade differs at tick "+std::to_string(tick));
 };
 collect=true;d.command(207);
 for(std::uint64_t tick=3206;tick<=3208;++tick)titleTick(tick,69340,69379);
 writes.clear();d.command(0);std::vector<Bytes> interrupted;
 for(const auto &e:captured)if(e.ms>=69379 && e.ms<69380)interrupted.push_back(e.bytes);
 if(writes!=interrupted)throw std::runtime_error("Production fixed-BX title interruption differs from capture");
 for(std::uint64_t tick=3209;tick<=3215;++tick)titleTick(tick,69380,69470);
 d.command(207);
 for(std::uint64_t tick=3216;tick<=3289;++tick)titleTick(tick,69470,70481);
 if(d.fadeSpeed())throw std::runtime_error("Production fixed-BX title fade did not finish on captured tick 3289");
 collect=false;while(d.ticks()<3302)d.tick();
 d.load(data(assets.readArchiveResource("town.m",XeenSceneArchive::Clouds)));
 std::size_t next=0;
 while(d.ticks()<6463) {
  d.tick();while(next<townEffects.size() && std::uint64_t((townEffects[next].ms-25314)/period)+1==d.ticks())d.effect(townEffects[next++].id);
 }
 if(next!=townEffects.size())throw std::runtime_error("Fixed-BX town effect schedule incomplete");
 const auto firstBx2=std::uint64_t(std::llround((114299-25314)/period))+1;
 const auto firstBx4=std::uint64_t(std::llround((114642-25314)/period))+1;
 const auto independent=[&](const Bytes &b,std::uint64_t tick) {
  return b.size()>=2 && ((b[0]>=0x80 && b[0]<=0x9f) ||
   (b.size()==3 && b[1]==11 && (b[0]==0xb2 || b[0]==0xb4 || b[0]==0xb6 ||
     (b[0]==0xb3 && tick<firstBx2) || (b[0]==0xb5 && tick<firstBx4))) ||
   (b.size()==3 && b[1]==123 && b[0]>=0xb1 && b[0]<=0xb9));
 };
 d.command(207);std::uint64_t stop=0;collect=true;
 for(std::uint64_t tick=6464;tick<=6605;++tick) {
  writes.clear();d.tick();if(!d.fadeSpeed() && !stop)stop=tick;
  std::vector<Bytes> actual,expected;
  for(const auto &b:writes)if(independent(b,tick))actual.push_back(b);
  for(const auto &e:captured)if(e.ms>=114093 && e.ms<116031 &&
    std::uint64_t(std::llround((e.ms-25314)/period))+1==tick && independent(e.bytes,tick))expected.push_back(e.bytes);
  if(actual!=expected)throw std::runtime_error("Production fixed-BX independent fade differs at tick "+std::to_string(tick));
 }
 if(stop!=6605)throw std::runtime_error("Production fixed-BX fade did not finish on captured tick 6605");
 while(d.ticks()<6609)d.tick();d.load(data(assets.readArchiveResource("outdoors.m",XeenSceneArchive::Clouds)));
 writes.clear();d.tick();
 const auto note=[](const Bytes &b){return b.size()==3 && (b[0]&0xf0)==0x90 && b[2]!=0;};
 const auto actual=std::find_if(writes.begin(),writes.end(),note);
 const auto expected=std::find_if(captured.begin(),captured.end(),[&](const auto &e){return e.ms==116098 && note(e.bytes);});
 if(d.ticks()!=6610 || actual==writes.end() || expected==captured.end() || *actual!=expected->bytes)
  throw std::runtime_error("Production fixed-BX first outdoor note differs from capture");
}
static void outputTests(XeenAssetSource &assets,const GameInstallation &install,const std::filesystem::path &directory,PromusDriver::Bytes resource) {
 std::ifstream input(install.uiModule->physicalPath,std::ios::binary);Bytes module((std::istreambuf_iterator<char>(input)),{});
 const auto loader=[&](const std::string &name,XeenSceneArchive archive){return assets.readArchiveResource(name,archive);};
 XeenAudioConfig config;config.music=4;config.digital=0;
 std::vector<std::uint64_t> snapshot[3];
 const auto run=[&](unsigned fixture,const std::filesystem::path &romDirectory) {
  auto output=std::make_shared<XeenFmSource>(PromusDriver::Sink{},romDirectory);
  XeenAudio audio(config,resource,loader,module,0,false,{},output);audio.enterTitle(0);
  audio.requestSong("town.m",XeenSceneArchive::Clouds,207,0);
  for(std::uint64_t now:{0ULL,10000000ULL,200000000ULL,3000000000ULL}) {
   audio.advance(now);snapshot[fixture].push_back(audio.ticks());snapshot[fixture].push_back(audio.pending());
   snapshot[fixture].push_back(audio.deadline().value_or(0));
  }
  if(fixture) {
   std::array<std::int16_t,1024> pcm;pcm.fill(1);output->pull(pcm.data(),512);output->check();
   if(std::any_of(pcm.begin(),pcm.end(),[](auto sample){return sample!=0;}))
    throw std::runtime_error("Unsupported ROM set did not render silence");
  }
 };
 run(0,directory);
 const auto unavailable=[&](unsigned fixture,const std::filesystem::path &romDirectory) {
  std::ostringstream notices;
  struct Capture {std::streambuf *old;explicit Capture(std::streambuf *next):old(std::cerr.rdbuf(next)){}~Capture(){std::cerr.rdbuf(old);}} capture(notices.rdbuf());
  run(fixture,romDirectory);
  const auto text=notices.str();const auto where=text.find("not supported yet");
  if(where==std::string::npos || text.find("not supported yet",where+1)!=std::string::npos)
   throw std::runtime_error("Unsupported ROM set must produce one not-supported notice");
  if(snapshot[0]!=snapshot[fixture])throw std::runtime_error("Unsupported ROM set changed logical timing");
 };
 unavailable(1,directory/"mmodern-nonexistent-rom-fixture");
 // A wrong-set directory uses hard links only; no ROM content is copied.
 // As above, some sandboxes or cross-volume TEMP directories disallow links.
 {
  struct Links {
   std::filesystem::path dir,a,b;bool owned=false;
   ~Links(){if(!owned)return;std::error_code ec;std::filesystem::remove(a,ec);std::filesystem::remove(b,ec);std::filesystem::remove(dir,ec);}
  } links;
  const auto files=romFiles(directory);const auto wrong=files.find("ctrl_cm32l_1_02"),pcm=files.find("pcm_mt32");
  if(wrong!=files.end() && pcm!=files.end()) {
   links.dir=std::filesystem::temp_directory_path()/("mmodern-mt32-wrong-"+std::to_string(std::chrono::steady_clock::now().time_since_epoch().count()));
   std::error_code ec;
   if(std::filesystem::create_directory(links.dir,ec)) {
    links.owned=true;links.a=links.dir/"wrong.rom";links.b=links.dir/"pcm.rom";
    std::filesystem::create_hard_link(wrong->second,links.a,ec);
    if(!ec)std::filesystem::create_hard_link(pcm->second,links.b,ec);
    if(!ec){unavailable(2,links.dir);std::cout<<"Wrong ROM set: one notice, silence and unchanged logical timing\n";}
   }
  }
 }
 {
  XeenFmSource silentBacklog({},directory/"mmodern-nonexistent-rom-fixture");silentBacklog.initialize(resource,4);
  for(unsigned i=0;i<5001;++i)silentBacklog.command(0);
  std::array<std::int16_t,2> sample{};silentBacklog.pull(sample.data(),0);silentBacklog.check();silentBacklog.command(0);
 }
 // Exercise the existing 33-ms request-to-audible guard through the real SDL
 // converter and the player's synth, over all 512-frame refill phases.
 unsigned worst=0;
 for(unsigned warm=512;warm<=16384;warm+=512) {
  auto source=std::make_shared<XeenFmSource>(PromusDriver::Sink{},directory);
  auto quietSource=std::make_shared<XeenFmSource>(PromusDriver::Sink{},directory);
  XeenAudio audio(config,resource,loader,module,0,false,{},source),quiet(config,resource,loader,module,0,false,{},quietSource);
  assert(audio.prepareRender(48000) && quiet.prepareRender(48000));
  std::array<std::int16_t,1024> pcm{},base{};
  for(unsigned n=0;n<warm;n+=512){audio.render(pcm.data(),512,48000);quiet.render(base.data(),512,48000);}
  audio.effect(1,0);bool audible=false;
  for(unsigned elapsed=0;elapsed<6000 && !audible;elapsed+=128) {
   audio.render(pcm.data(),128,48000);quiet.render(base.data(),128,48000);
   for(unsigned f=0;f<128;++f)if(std::abs(int(pcm[f*2])-base[f*2])>8 || std::abs(int(pcm[f*2+1])-base[f*2+1])>8) {
    worst=std::max(worst,elapsed+f);audible=true;break;
   }
  }
  if(!audible)throw std::runtime_error("MT-32 effect was not audible through the converter");
  if(source->midiDeferrals() || quietSource->midiDeferrals())throw std::runtime_error("Ordinary MT-32 latency witness deferred an operation");
 }
 std::cout<<"MT-32 worst request-to-audible latency "<<worst/48.0<<" ms\n";
 if(worst>1584)throw std::runtime_error("MT-32 effect exceeded the 33 ms latency guard");
}
#endif
int main(int argc,char **argv){try {
 if(argc!=4 || !*argv[3])throw std::runtime_error("Expected game, M94 capture and MT-32 ROM directories");
 const auto install=XeenInstallationDetector().detect(std::filesystem::u8path(argv[1]));
 if(!install)throw std::runtime_error("Game installation missing");
 XeenAssetSource assets(*install);auto resource=data(assets.readArchiveResource("rolmus",XeenSceneArchive::IntroOnly));
 auto captured=midi(std::filesystem::u8path(argv[2])/"rolmus"/"midi0001.mid");
 std::vector<PromusDriver::MidiWrite> events;
 PromusDriver driver(resource,{},4,[&](const auto &event){events.push_back(event);});
 assert(events.size()==22 && captured.size()>=events.size());
 const std::array<unsigned,13> lengths{33,267,18,256,18,256,18,256,18,256,18,256,18};
 double low=-1e30,high=1e30;
 for(std::size_t i=0;i<events.size();++i){
  assert(message(events[i])==captured[i].bytes);
  if(i<9)assert(events[i].size==3 && events[i].bytes[1]==7);
  else {assert(events[i].size==lengths[i-9]);unsigned sum=0;for(std::size_t p=5;p+1<events[i].size;++p)sum+=events[i].bytes[p];assert(!(sum&127));}
  const double delta=double(captured[i].ms)-double(events[i].timeNanoseconds-events[0].timeNanoseconds)/1000000.0;
  low=std::max(low,delta-1.0);high=std::min(high,delta+1.0);
 }
 assert(low<=high);
 assert(events.back().timeNanoseconds-events.front().timeNanoseconds==2987050);
 const auto original=events;events.clear();driver.restartProgram();
 assert(driver.ticks()==0 && events.size()==original.size()+10);
 for(std::size_t i=0;i<original.size();++i)assert(message(events[i+10])==message(original[i]));
 std::vector<Captured> secondInit;for(const auto &e:captured)if(e.ms>=25301 && e.ms<25305)secondInit.push_back(e);
 assert(secondInit.size()==22);low=-1e30;high=1e30;
 for(std::size_t i=0;i<secondInit.size();++i) {
  assert(message(events[i+10])==secondInit[i].bytes);
  const auto delta=double(secondInit[i].ms)-double(events[i+10].timeNanoseconds-events[10].timeNanoseconds)/1000000.0;
  low=std::max(low,delta-1.0);high=std::min(high,delta+1.0);
 }
 assert(low<=high && events.back().timeNanoseconds-events[10].timeNanoseconds==2987050);
 // Music records use byte 24 for the program; expression selector zero sends
 // CC11, while another selector consumes its operand without sending it.
 Bytes song{0x20};for(unsigned i=0;i<26;++i)song.push_back(std::uint8_t(i));
 song.insert(song.end(),{0xc0,0,0xa0,1,80,0xa0,0,40,0xa8,0,40,0x12,0xff});
 events.clear();driver.load(data(std::move(song)));events.clear();driver.tick();
 assert(std::any_of(events.begin(),events.end(),[](const auto &e){return e.size==2 && e.bytes[0]==0xc1 && e.bytes[1]==24;}));
 assert(std::none_of(events.begin(),events.end(),[](const auto &e){return e.size==3 && e.bytes[1]==11 && e.bytes[2]==80;}));
 driver.command(255);events.clear();for(unsigned i=0;i<8 && driver.fadeSpeed();++i)driver.tick();
 assert(!driver.fadeSpeed());
 assert(std::any_of(events.begin(),events.end(),[](const auto &e){return e.size==3 && e.bytes[0]==0xb9 && e.bytes[1]==11 && e.bytes[2]==39;}));
 assert(std::any_of(events.begin(),events.end(),[](const auto &e){return e.size==3 && e.bytes[0]==0xb1 && e.bytes[1]==11 && e.bytes[2]==39;}));
 for(unsigned id=0;id<=180;++id){PromusDriver each(resource,{},4);each.effect(id);for(unsigned tick=0;tick<512;++tick)each.tick();}
 for(unsigned id:{5u,9u,41u,42u,43u,44u,48u,59u,66u,67u,68u,69u,70u}) {
  std::vector<PromusDriver::MidiWrite> fx;PromusDriver timed(resource,{},4,[&](const auto &e){fx.push_back(e);});
  timed.effect(id);fx.clear();for(unsigned tick=0;tick<10;++tick)timed.tick();
  assert(timed.effectPlaying() && std::none_of(fx.begin(),fx.end(),[](const auto &e){return e.size && (e.bytes[0]&0xf0)==0x90;}));
  timed.tick();assert(!timed.effectPlaying());
 }
 for(unsigned id=0;id<=180;++id) {
  PromusDriver each(resource,{},4);each.effect(id);each.command(207);
  for(unsigned tick=0;tick<1024 && each.fadeSpeed();++tick)each.tick();
  assert(!each.fadeSpeed()); // Witnesses these original effect streams, not arbitrary external input.
 }
 PromusDriver protection(resource,{},4);protection.effect(7);assert(protection.effectPlaying());protection.effect(8);assert(protection.effectPlaying());
 {
  std::vector<PromusDriver::MidiWrite> fx;PromusDriver noteOff(resource,{},4,[&](const auto &e){fx.push_back(e);});
  noteOff.effect(1);noteOff.tick();noteOff.effect(0);fx.clear();noteOff.tick();
  assert(std::any_of(fx.begin(),fx.end(),[](const auto &e){return message(e)==Bytes{0x88,0,127};}));
  noteOff.effect(2);noteOff.tick();noteOff.effect(0);fx.clear();noteOff.tick();
  assert(std::any_of(fx.begin(),fx.end(),[](const auto &e){return message(e)==Bytes{0x89,69,127};}));
 }
#ifdef MMODERN_ROLMUS_ORACLE
 captureOracle(assets,resource,captured);
#endif
 romTests(std::filesystem::u8path(argv[3]),resource);
#ifndef MMODERN_ROLMUS_ORACLE
 fixedCaptureOracle(assets,resource,captured);
 outputTests(assets,*install,std::filesystem::u8path(argv[3]),resource);
#endif
 std::cout<<"Rolmus: exact initial MIDI bytes and upload timing; handoff, selector, fade and 181 effects checked\n";
 return 0;
}catch(const std::exception &e){std::cerr<<e.what()<<'\n';return 1;}}
