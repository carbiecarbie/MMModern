#include "app/XeenAudio.h"
#include "app/XeenFmSource.h"
#include "formats/xeen/XeenAssetSource.h"
#include "XeenTestInstallation.h"
#include <algorithm>
#include <atomic>
#include <chrono>
#include <fstream>
#include <iostream>
#include <thread>
using namespace mmodern;
using Bytes=std::vector<std::uint8_t>;
namespace {
void check(bool value,const char *message) {if(!value)throw std::runtime_error(message);}
Bytes voc(unsigned count,std::uint8_t value) {
 Bytes bytes(32+count,value);
 const auto word=std::uint16_t(count+9);bytes[27]=word;bytes[28]=word>>8;
 return bytes;
}
void pull(XeenAudio &audio,std::int16_t *pcm,std::size_t frames,int rate) {
 check(audio.prepareRender(rate),"Could not prepare device-free mixer");audio.render(pcm,frames,rate);
}
void render(XeenAudio &audio,std::vector<std::int16_t> &pcm,unsigned block,int rate) {
 for(std::size_t f=0;f<pcm.size()/2;f+=block)pull(audio,pcm.data()+2*f,std::min<std::size_t>(block,pcm.size()/2-f),rate);
}
void constant(const std::vector<std::int16_t> &pcm,int value) {
 check(std::all_of(pcm.begin(),pcm.end(),[&](auto v){return v==value;}),"Unexpected digital PCM");
}
}
int main(int argc,char **argv) {try {
 check(argc==2,"usage: mmodern_digital_audio_tests <game data>");
 const auto installation=xeenTestInstallationDetector().detect(std::filesystem::u8path(argv[1]));
 check(bool(installation),"No installation");XeenAssetSource assets(*installation);
 const auto driver=std::make_shared<const Bytes>(assets.readArchiveResource("promus",XeenSceneArchive::IntroOnly));
 std::ifstream input(installation->uiModule->physicalPath,std::ios::binary);
 const Bytes module((std::istreambuf_iterator<char>(input)),{});
 unsigned reads=0;
 const auto loader=[&](const std::string &name,XeenSceneArchive archive) {
  if(name=="a.voc"){++reads;return voc(512,160);}
  if(name=="b.voc"){++reads;return voc(32,96);}
  if(name=="long.voc"){++reads;return voc(65536,255);}
  return assets.readArchiveResource(name,archive);
 };
 const auto archive=XeenSceneArchive::Clouds;
 // Busy is a presentation-clock query, including boundaries, replacement,
 // Efx masking of every wrapper command, and an unrelated clock epoch.
 {
  XeenAudio audio(0,{},loader,module,100,false);
  check(!audio.digitalBusy(100),"Fresh process is digitally busy");
  audio.startDigital("a.voc",archive,100);
  check(audio.digitalBusy(100+512*68000ULL-1) && !audio.digitalBusy(100+512*68000ULL),"Driver-count busy boundary differs");
  audio.startDigital("b.voc",archive,100+1000000);
  check(!audio.digitalBusy(100+1000000+32*68000ULL),"Replacement retained old deadline");
  audio.startDigital("a.voc",archive,100+4000000);
  auto options=XeenSoundOptions{};options.efx=false;audio.observeOptions(options);
  const auto before=reads;
  check(!audio.digitalBusy(100+5000000),"Efx off did not mask query");
  audio.startDigital("absent.voc",archive,100+5000000);audio.stopDigital(100+5000000);
  check(reads==before,"Masked start loaded a sample");
  options.efx=true;audio.observeOptions(options);
  check(audio.digitalBusy(100+5000000),"Efx off stopped or replaced the active voice");
  audio.advance(100+5000000);audio.rebaseClock(900000000000ULL);
  check(audio.digitalBusy(900000000000ULL+512*68000ULL-1000000-1),"Rebase lost remaining busy interval");
  check(!audio.digitalBusy(900000000000ULL+512*68000ULL-1000000),"Rebase extended busy interval");
  audio.stopDigital(900000000000ULL);check(!audio.digitalBusy(900000000000ULL),"Stop retained busy");
 }
 {
  constexpr std::uint64_t origin=1000000,action=origin+2000000,end=action+512*68000ULL;
  XeenAudio audio(0,{},loader,module,origin,false);
  audio.advance(origin+40000000);const auto samples=audio.samples();
  audio.startDigital("a.voc",archive,action); // Action clock lags latest presentation.
  check(audio.samples()==samples,"Lagging start rewound the audio clock");
  check(audio.digitalBusy(end-1) && !audio.digitalBusy(end),"Lagging start/busy clamped the action deadline");
  check(!audio.digitalBusy(origin+40000000),"Lagging start extended busy past the exact action interval");
  audio.stopDigital(origin+3000000);
  check(!audio.digitalBusy(end-1) && audio.samples()==samples,"Lagging stop failed or rewound the clock");
  audio.advance(origin+40000000); // A normal advance still sees the latest epoch.
  bool backwards=false;
  try{audio.advance(origin+39000000);}catch(const std::logic_error &){backwards=true;}
  check(backwards,"Ordinary presentation advance accepted a backwards clock");
  XeenAudio fresh(0,{},loader,module,origin,false);
  for(unsigned command=0;command<3;++command) {
   bool checked=false;
   try {
    if(command==0)fresh.startDigital("a.voc",archive,origin-1);
    if(command==1)fresh.stopDigital(origin-1);
    if(command==2)(void)fresh.digitalBusy(origin-1);
   }catch(const std::underflow_error &){checked=true;}
   check(checked,"Digital action before the elapsed clock origin was not checked");
  }
 }
 // Exact fixed rational VOC rate, FM at 49716, sum once at the device rate.
 // Partitioning must change neither source cursor nor PCM. Unity gains here;
 // listening balance belongs to Part C.
 for(int rate:{22050,44100,48000})for(unsigned block:{1,73,512,2049}) {
  auto fm=std::make_shared<XeenFmSource>(),both=std::make_shared<XeenFmSource>();
  XeenAudio music(3,driver,loader,module,0,false,{},fm),mix(3,driver,loader,module,0,false,{},both);
  music.enterTitle(0);mix.enterTitle(0);mix.startDigital("a.voc",archive,0);
  std::vector<std::int16_t> baseline(8192),actual(8192);
  render(music,baseline,257,rate);render(mix,actual,block,rate);
  const auto end=(512ULL*68*rate+999999)/1000000;
  for(std::size_t f=0;f<actual.size()/2;++f)for(unsigned c=0;c<2;++c)
   check(actual[2*f+c]==std::clamp(int(baseline[2*f+c])+(f<end?8192:0),-32768,32767),"FM/VOC sum, resampled length or callback partition differs");
 }
 for(bool disabled:{false,true}) {
  auto fm=std::make_shared<XeenFmSource>();
  XeenAudio audio(disabled?3:0,disabled?driver:PromusDriver::Bytes{},loader,module,0,false,{},fm);
  if(disabled){audio.enterTitle(0);audio.toggleMusic(false,0);}
  // The loader returns temporary bytes; requests must keep their own PCM.
  audio.startDigital("a.voc",archive,0);
  std::vector<std::int16_t> pcm(34);pull(audio,pcm.data(),17,48000);constant(pcm,8192);
  audio.startDigital("b.voc",archive,0);pull(audio,pcm.data(),17,48000);constant(pcm,-8192);
  audio.stopDigital(0);pull(audio,pcm.data(),17,48000);constant(pcm,0);
  audio.startDigital("a.voc",archive,0);
  auto options=XeenSoundOptions{};options.efx=false;audio.observeOptions(options);audio.stopDigital(0);
  pull(audio,pcm.data(),17,48000);constant(pcm,8192);
  options.efx=true;audio.observeOptions(options);
  audio.startDigital("long.voc",archive,0);audio.startDigital("b.voc",archive,0);
  pull(audio,pcm.data(),17,48000);constant(pcm,-8192); // No queued old start.
  audio.startDigital("a.voc",archive,0);audio.stopDigital(0);
  pull(audio,pcm.data(),17,48000);constant(pcm,0);
 }
 {
  XeenAudio audio(0,{},loader,module,0,false);
  audio.requestSong("not-read.m",archive,128,0);
  audio.startDigital("a.voc",archive,0);audio.stopDigital(0);
  check(audio.pending(),"Digital command serviced an unrelated pending FM load");
 }
 // Stop/replacement while a large callback is in progress. The render trace
 // injects a request during a later native FM block; conversion must not keep
 // an old VOC tail. No other producer writes concurrently in this test.
 for(bool stop:{false,true}) {
  XeenAudio *owner=nullptr;bool changed=false;std::uint64_t trigger=5;
  auto fm=std::make_shared<XeenFmSource>([&](const auto &w) {
   if(owner && w.tick>=trigger && !changed) {
    changed=true;
    if(stop)owner->stopDigital(0);else owner->startDigital("b.voc",archive,0);
   }
  });
  XeenAudio audio(3,driver,loader,module,0,false,{},fm);audio.enterTitle(0);owner=&audio;
  // Compare against an identical FM-only render below.
  audio.startDigital("long.voc",archive,0);
  std::vector<std::int16_t> actual(20000),base(20000);
  auto cleanFm=std::make_shared<XeenFmSource>();XeenAudio clean(3,driver,loader,module,0,false,{},cleanFm);clean.enterTitle(0);
  render(clean,base,10000,48000);pull(audio,actual.data(),10000,48000);
  check(changed,"Mid-callback command never published");
  check(actual[0]==std::clamp(int(base[0])+32512,-32768,32767),"Voice missing before mid-callback command");
  check(actual.back()==base.back(),"Old digital tail survived mid-callback command");
  if(!stop) {
   unsigned replacementFrames=0;
   for(std::size_t f=0;f<actual.size()/2;++f)
    replacementFrames+=actual[2*f]==std::clamp(int(base[2*f])-8192,-32768,32767);
   check(replacementFrames==(32*68*48000+999999)/1000000,"Mid-callback replacement length differs");
  }
 }
 // Timeline fade completed ahead of rendering: its queued FM barrier remains
 // pending, but VOC start and stop must reach the mixer on the next block.
 {
  auto fm=std::make_shared<XeenFmSource>();XeenAudio audio(3,driver,loader,module,0,false,{},fm);
  audio.enterTitle(0);audio.requestInn(0);audio.advance(PromusDriver::clockBoundary(200));
  check(audio.loadedName()=="inn.m","Timeline fade did not complete");
  audio.startDigital("long.voc",archive,PromusDriver::clockBoundary(200));
  auto baselineFm=std::make_shared<XeenFmSource>();XeenAudio baseline(3,driver,loader,module,0,false,{},baselineFm);
  baseline.enterTitle(0);baseline.requestInn(0);baseline.advance(PromusDriver::clockBoundary(200));
  std::vector<std::int16_t> pcm(2),base(4);pull(baseline,base.data(),2,48000);pull(audio,pcm.data(),1,48000);
  check(pcm[0]==std::clamp(int(base[0])+32512,-32768,32767),"FM fade barrier delayed digital start");
  audio.stopDigital(PromusDriver::clockBoundary(200));pull(audio,pcm.data(),1,48000);
  check(pcm[0]==base[2],"FM fade barrier delayed digital stop");
 }
 // Identical route and digital busy boundaries with and without an SDL device.
 std::vector<PromusDriver::Write> traces[2];std::vector<bool> busy[2];
 for(unsigned device=0;device<2;++device) {
  auto source=std::make_shared<XeenFmSource>();
  XeenAudio audio(3,driver,loader,module,0,device!=0,[&](const auto &w){traces[device].push_back(w);},source);
  check(audio.outputActive()==(device!=0),"Requested dummy device did not open");
  audio.enterTitle(0);audio.startDigital("a.voc",archive,0);
  for(std::uint64_t t:{1000000,10000000,40000000,2000000000}) {
   audio.advance(t);busy[device].push_back(audio.digitalBusy(t));
   audio.effect(20,t);
  }
  audio.startDigital("long.voc",archive,2000000000);
  if(device) {
   for(unsigned n=0;n<200 && source->samples()==0;++n)std::this_thread::sleep_for(std::chrono::milliseconds(10));
   check(source->samples()>0,"Device never rendered before playing-owner shutdown");
  }
  // Destruction while playing must close/join before PCM, queues and chip die.
 }
 check(traces[0].size()==traces[1].size() && std::equal(traces[0].begin(),traces[0].end(),traces[1].begin(),[](const auto &a,const auto &b){
  return a.tick==b.tick && a.reg==b.reg && a.value==b.value && a.interrupt==b.interrupt;
 }) && busy[0]==busy[1],"Device changed logical route trace");
 std::cout<<"Digital commands, mixing, timing and shutdown passed\n";return 0;
}catch(const std::exception &e){std::cerr<<e.what()<<'\n';return 1;}}
