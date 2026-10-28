#include "app/XeenAudio.h"
#include "app/XeenFmSource.h"
#include "app/XeenAudioMix.h"
#include "formats/xeen/XeenAssetSource.h"
#include "XeenTestInstallation.h"
#include "platform/sdl/SdlAudioDevice.h"
#define SDL_MAIN_HANDLED
#include <SDL.h>
#include <algorithm>
#include <atomic>
#include <chrono>
#include <fstream>
#include <iostream>
#include <sstream>
#include <thread>
#include <cstdlib>
#include <new>
namespace render_allocation_test {thread_local bool active=false;thread_local unsigned count=0;}
void *operator new(std::size_t size) {
 if(render_allocation_test::active)++render_allocation_test::count;
 if(auto *p=std::malloc(size?size:1))return p;throw std::bad_alloc();
}
void operator delete(void *p) noexcept{std::free(p);}
void operator delete(void *p,std::size_t) noexcept{std::free(p);}
void *operator new[](std::size_t size){return ::operator new(size);}
void operator delete[](void *p) noexcept{::operator delete(p);}
void operator delete[](void *p,std::size_t) noexcept{::operator delete(p);}
using namespace mmodern;
using Bytes=std::vector<std::uint8_t>;
namespace mmodern {
struct XeenAudioTestAccess {static void failDsp(XeenAudio &audio){audio.latchDigitalFailureForTest();}};
}
namespace {
SDL_AudioDeviceID openedDevice=0;bool failOpen=false,disconnected=false;
std::atomic<bool> conversionError{false};
SDL_AudioCallback deviceCallback=nullptr;void *deviceContext=nullptr;
std::atomic<unsigned> callbacks{0};std::atomic<int> lastPeak{0};
std::atomic<bool> expectSilence{false},unexpectedSound{false};
}
extern "C" SDL_AudioDeviceID __real_SDL_OpenAudioDevice(const char *,int,const SDL_AudioSpec *,SDL_AudioSpec *,int);
extern "C" SDL_AudioDeviceID __wrap_SDL_OpenAudioDevice(const char *name,int capture,const SDL_AudioSpec *want,SDL_AudioSpec *have,int changes) {
 if(failOpen){SDL_SetError("injected reopen failure");return 0;}
 auto monitored=*want;deviceCallback=want->callback;deviceContext=want->userdata;
 monitored.callback=[](void *,Uint8 *output,int bytes) {
  deviceCallback(deviceContext,output,bytes);const auto *pcm=reinterpret_cast<std::int16_t *>(output);int peak=0;
  for(int i=0;i<bytes/2;++i)peak=std::max(peak,std::abs(int(pcm[i])));
  if(expectSilence && peak)unexpectedSound=true;
  lastPeak=peak;++callbacks;
 };
 openedDevice=__real_SDL_OpenAudioDevice(name,capture,&monitored,have,changes);return openedDevice;
}
extern "C" int __real_SDL_AudioStreamGet(SDL_AudioStream *,void *,int);
extern "C" int __wrap_SDL_AudioStreamGet(SDL_AudioStream *stream,void *out,int bytes) {
 if(conversionError.exchange(false)){SDL_SetError("injected conversion failure");return -1;}
 return __real_SDL_AudioStreamGet(stream,out,bytes);
}
extern "C" SDL_AudioStatus __real_SDL_GetAudioDeviceStatus(SDL_AudioDeviceID);
extern "C" SDL_AudioStatus __wrap_SDL_GetAudioDeviceStatus(SDL_AudioDeviceID id) {
 return disconnected?SDL_AUDIO_STOPPED:__real_SDL_GetAudioDeviceStatus(id);
}
namespace {
void check(bool value,const char *message) {if(!value)throw std::runtime_error(message);}
struct CaptureErrors {
 std::ostringstream text;std::streambuf *previous=std::cerr.rdbuf(text.rdbuf());
 ~CaptureErrors(){std::cerr.rdbuf(previous);}
};
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
template<class F> void until(F ready) {
 const auto end=std::chrono::steady_clock::now()+std::chrono::seconds(5);
 while(!ready() && std::chrono::steady_clock::now()<end)std::this_thread::yield();
 check(ready(),"Dummy callback did not reach expected playback state");
}
}
int main(int argc,char **argv) {try {
 check(argc==2,"usage: mmodern_digital_audio_tests <game data>");
 const auto installation=xeenTestInstallationDetector().detect(std::filesystem::u8path(argv[1]));
 check(bool(installation),"No installation");XeenAssetSource assets(*installation);
 const auto driver=std::make_shared<const Bytes>(assets.readArchiveResource("promus",XeenSceneArchive::IntroOnly));
 const auto musicDriver=[&](unsigned id)->PromusDriver::Bytes {
  if(id==3)return driver;
  if(id==1 || id==2 || id==6)return std::make_shared<const Bytes>(assets.readArchiveResource(id==1?"admus":id==2?"blastmus":"specmus",XeenSceneArchive::IntroOnly));
  return {};
 };
 std::ifstream input(installation->uiModule->physicalPath,std::ios::binary);
 const Bytes module((std::istreambuf_iterator<char>(input)),{});
 unsigned reads=0;
 const auto loader=[&](const std::string &name,XeenSceneArchive archive) {
  if(name=="a.voc"){++reads;return voc(512,160);}
  if(name=="b.voc"){++reads;return voc(32,96);}
  if(name=="long.voc"){++reads;return voc(65536,255);}
  if(name=="ramp.voc") {
   auto sample=voc(10000,128);for(unsigned i=0;i<10000;++i)sample[32+i]=128+(i%16)*7;return sample;
  }
  return assets.readArchiveResource(name,archive);
 };
 const auto archive=XeenSceneArchive::Clouds;
 // Raw IDs select the transport independently of captions and EXE admission.
 for(unsigned id:{0u,1u,2u,3u,4u,5u,6u,7u,8u,9u,10u,11u,65535u}) {
  std::string notice;
  {
   CaptureErrors captured;XeenAudioConfig cfg;cfg.music=0;cfg.digital=std::uint16_t(id);cfg.dma=3;
   XeenAudio audio(cfg,{},loader,module,0,false);
   check(audio.digitalDeviceId()==id && audio.entranceCaptions(true)==(id<2) && audio.entranceCaptions(false),"Digital id or caption gate");
   const auto before=reads;
   audio.startDigital("a.voc",archive,100);
   if(cfg.dmaDigital()) {
    const auto count=id==6?508ULL:512ULL,deadline=100+count*68000;
    check(audio.digitalDma()==(id==6?1:3),"Digital DMA channel");
    check(audio.digitalDuration("a.voc",archive)==count*68000 && audio.digitalDeadline(100)==deadline,"Selected digital count or deadline");
    check(audio.digitalBusy(deadline-1) && !audio.digitalBusy(deadline),"Digital busy endpoint");
    audio.startDigital("b.voc",archive,200);
    const auto replacement=200+(id==6?28ULL:32ULL)*68000;
    check(audio.digitalDeadline(200)==replacement,"Digital replacement deadline");
    audio.stopDigital(201);check(!audio.digitalBusy(201),"Digital stop did not clear busy");
   }else {
    check(!audio.digitalBusy(100) && !audio.digitalDeadline(100) && audio.digitalDuration("a.voc",archive)==0 && reads==before,
          "Null digital path loaded data or stayed busy");
   }
   check(audio.startProgram(201,XeenAudioProgram::Dat),"DAT digital selection refused");
   const bool exe=audio.startProgram(201,XeenAudioProgram::Exe);
   check(exe==!(id==3 || id==4 || id==9),"EXE digital exception");
   check(audio.startProgram(201,XeenAudioProgram::Exe)==exe,"EXE exception changed on retry");
   notice=captured.text.str();
  }
  const bool unsupported=id==1 || id==2 || id==3 || id==4 || id==5 || id==8 || id==9;
  check(notice==(unsupported?"Digital device "+std::to_string(id)+": not supported yet\n":""),"Digital notice missing or repeated");
 }
 for(unsigned id:{0u,1u,2u,3u,4u,5u,6u,7u,8u,9u,10u}) {
  std::string notice;
  {
   CaptureErrors captured;XeenAudioConfig cfg;cfg.music=std::uint16_t(id);cfg.digital=0;
   XeenAudio audio(cfg,musicDriver(id),loader,module,0,false);
   audio.startProgram(0);audio.startProgram(0);notice=captured.text.str();
  }
  const bool unsupported=id==4 || id==5 || id==7 || id==8 || id==9;
  check(notice==(unsupported?"Music device "+std::to_string(id)+": not supported yet\n":""),"Music notice missing or repeated");
 }
 for(unsigned music:{0u,1u,2u,3u,4u,5u,6u,7u,8u,9u,10u})for(unsigned digital:{0u,6u,7u,10u}) {
  CaptureErrors captured;XeenAudioConfig cfg;cfg.music=music;cfg.digital=digital;cfg.musicPort=0x220;cfg.digitalPort=0x240;
  XeenAudio audio(cfg,musicDriver(music),loader,module,0,false);
  const bool writesDigitalPort=music==1 || music==2 || music==4 || music==6;
  const float attenuation=music==3?std::pow(10.0f,-24.0f/20):
   writesDigitalPort && digital==7?std::pow(10.0f,-11.0f/20):
   writesDigitalPort && digital==10?std::pow(10.0f,-24.0f/20):1.0f;
  const auto before=audio.mixerGains();
  check(std::abs(before[2]-attenuation*kXeenSourceGain)<1e-6f,"Music/digital mixer matrix or port gating");
  audio.mixer(75,0);const auto after=audio.mixerGains();
  check(music==3?after[0]<before[0]:after==before,"Only promus may change card FM level");
  audio.processMixer(false);check(audio.mixerGains()[2]==kXeenSourceGain,"Mixer flag did not preserve reset DAC");
 }
 for(unsigned music:{0u,5u,7u,8u,9u,65535u})for(unsigned digital:{7u,10u}) {
  CaptureErrors captured;XeenAudioConfig cfg;cfg.music=music;cfg.digital=digital;cfg.musicPort=cfg.digitalPort=0x240;
  XeenAudio audio(cfg,{},loader,module,0,false);
  const float attenuation=std::pow(10.0f,(digital==7?-11.0f:-24.0f)/20);
  check(std::abs(audio.mixerGains()[2]-attenuation*kXeenSourceGain)<1e-6f,"Null music init did not use its music-port mixer transaction");
  audio.requestSong("inn.m",archive,207,0);
  check(audio.deadline()==0,"Silent music introduced a fade/load wait");audio.advance(0);
  check(!audio.pending() && audio.loadedName()=="inn.m","Silent music did not complete without a driver wait");
 }
 {
  // A latched blastsnd DSP failure clears busy on its next admitted query;
  // disabling Efx masks the query and must not consume that latch.
  XeenAudioConfig cfg;cfg.digital=6;XeenAudio audio(cfg,{},loader,module,0,false);
  audio.startDigital("a.voc",archive,0);XeenAudioTestAccess::failDsp(audio);
  audio.observeOptions({false,true,false,true});check(!audio.digitalBusy(0),"Efx-off query was not masked");
  audio.observeOptions({});check(!audio.digitalBusy(0),"blastsnd failure did not clear busy");
  audio.startDigital("a.voc",archive,0);check(audio.digitalBusy(0),"blastsnd failure latch did not clear");
 }
 for(unsigned id:{0u,6u,7u,10u}) {
  std::vector<std::uint64_t> outcomes[2];
  for(unsigned device=0;device<2;++device) {
   XeenAudioConfig cfg;cfg.digital=std::uint16_t(id);XeenAudio audio(cfg,{},loader,module,0,device!=0);
   check(audio.outputActive()==(device!=0),"Timing test dummy output state");
   audio.startDigital("a.voc",archive,0);
   for(std::uint64_t now:{0ULL,1000000ULL,20000000ULL,40000000ULL}) {
    audio.advance(now);outcomes[device].push_back(audio.digitalBusy(now));
    outcomes[device].push_back(audio.digitalDeadline(now).value_or(0));outcomes[device].push_back(audio.ticks());
   }
  }
  check(outcomes[0]==outcomes[1],"Output changed selected digital timing");
 }
 // Retain/decode without starting; chained starts do no resource I/O. The
 // published owner survives release of the caller's handle and exact EOF is
 // still determined by the presentation clock, including after a rebase.
 {
  XeenAudio audio(0,{},loader,module,0,false);const auto before=reads;
  auto first=audio.retainDigital("a.voc",archive),next=audio.retainDigital("b.voc",archive);
  check(reads==before+2 && !audio.digitalBusy(0),"retain started playback or failed to decode once");
  audio.startDigital(first,0);first.sample.reset();
  check(reads==before+2 && audio.digitalBusy(512*68000ULL-1) && !audio.digitalBusy(512*68000ULL),"retained voice ownership/EOF");
  audio.startDigital(next,512*68000ULL);
  check(reads==before+2 && audio.digitalBusy(544*68000ULL-1) && !audio.digitalBusy(544*68000ULL),"retained chain reloaded or used stale EOF");
  audio.rebaseClock(0);check(audio.digitalBusy(32*68000ULL-1) && !audio.digitalBusy(32*68000ULL),"retained sample changed at clock handoff");
  audio.observeOptions({false,true,false,true});audio.startDigital(next,544*68000ULL);
  check(!audio.digitalBusy(544*68000ULL),"retained start bypassed party gate outside EXE handoff");
 }
 {
  unsigned loads=0;auto counted=[&](const auto &name,auto selected){++loads;return loader(name,selected);};
  XeenAudio audio(3,driver,counted,module,0,false);audio.enterTitle(0);const auto loaded=audio.loadedName();const auto before=loads;
  audio.requestSong("mm4theme.m",archive,207,0);check(audio.pending(),"song replacement not queued");
  audio.fadeSong(50,0);check(!audio.pending(),"fade-only retained a replacement");audio.advance(3000000000ULL);
  check(audio.loadedName()==loaded && loads==before,"fade-only loaded or replaced the song");
  audio.fadeSong(0,3000000000ULL);check(audio.loadedName()==loaded && loads==before,"stop command reloaded song");
  audio.requestSong("mm4theme.m",archive,0,3000000000ULL);audio.advance(3000000000ULL);
  check(audio.loadedName()=="mm4theme.m" && loads==before+1,"stopped song request identity suppressed a later intro");
  bool refused=false;try{audio.fadeSong(256,3000000000ULL);}catch(const std::invalid_argument &){refused=true;}
  check(refused,"fade-only admitted mixer/pointer command");
 }
 // Measure request-to-audible FM latency through the real 48 kHz converter.
 // Sweep 512-frame callback
 // phases so a request just after a refill cannot hide behind a favorable phase.
 unsigned worstLatencyFrames=0;
 for(unsigned warm=512;warm<=16384;warm+=512) {
  auto fm=std::make_shared<XeenFmSource>(),quietFm=std::make_shared<XeenFmSource>();
  XeenAudio audio(3,driver,loader,module,0,false,{},fm),quiet(3,driver,loader,module,0,false,{},quietFm);
  std::vector<std::int16_t> pcm(warm*2),base(warm*2);
  render(audio,pcm,512,48000);render(quiet,base,512,48000);audio.effect(20,0);
  bool audible=false;
  for(unsigned elapsed=0;elapsed<6000 && !audible;elapsed+=128) {
   pull(audio,pcm.data(),128,48000);pull(quiet,base.data(),128,48000);
   for(unsigned f=0;f<128;++f)if(std::abs(int(pcm[f*2])-base[f*2])>8 || std::abs(int(pcm[f*2+1])-base[f*2+1])>8) {
    worstLatencyFrames=std::max(worstLatencyFrames,elapsed+f);audible=true;break;
   }
  }
  check(audible,"Requested FM effect never became audible through converter");
 }
 std::cout<<"Worst 48 kHz FM effect latency: "<<worstLatencyFrames<<" frames, "<<worstLatencyFrames/48.0<<" ms\n";
 // Approximately 30 ms, allowing PIT phase and the effect's initial attack.
 check(worstLatencyFrames<=1584,"FM effect exceeded M82's approximately 30 ms latency bound (33 ms guard)");
 {
  // Card mixer as DOSBox Staging's SB16 applies the promus registers.
  const auto db=[](float g){return 20*std::log10(g);};
  const auto near=[&](std::uint8_t reg,float expected){return std::abs(db(XeenCardMixer::level(reg,0))-expected)<0.001f && XeenCardMixer::level(reg,0)==XeenCardMixer::level(reg,1);};
  check(near(0xff,0) && near(0x99,-24) && near(0xaa,-20) && near(0xbb,-16) && near(0x55,-40) && near(0x11,-55),"SB16 mixer level mapping differs");
  check(std::abs(db(XeenCardMixer::level(0x9f,1))-0)<0.001f && std::abs(db(XeenCardMixer::level(0x9f,0))+24)<0.001f,"Mixer nibbles are not left/right");
  PromusDriver promus(driver);
  for(auto [argument,reg]:std::vector<std::pair<unsigned,unsigned>>{{48,0x99},{75,0xaa},{95,0xbb},{255,0x55},{0x90,0xff}}) {
   promus.command(0x100,argument);check(promus.mixerLevel()==reg,"Promus command 0x100 register byte differs");
  }
  XeenCardMixer reset,on,off;on.initialize(true);off.initialize(false);
  check(reset.dac==0xff && on.dac==0x99 && on.fm==0xff && on.master==0xff && off.dac==0xff,"Driver-init mixer writes differ");
  off.fm=0xbb;off.initialize(false);on.fm=0xbb;on.initialize(true);
  check(off.fm==0xbb && on.fm==0xff,"Init without its flag rewrote the mixer, or with it kept the old FM level");
  const auto g=on.gains();
  // M83 DOSBox source ratios: VOC/FM 0.0407, CD/FM about 0.69, tolerance 1.5 dB.
  check(g[0]==1 && std::abs(db(g[2]/kXeenFmGain/0.0407f))<1.5f && std::abs(db(kXeenCdGain/kXeenFmGain/0.69f))<1.5f,"Init balance left the M83 tolerance");
  check(kXeenFmGain==0.5f && mixXeenAudio(0,16000,0,g,0)==int(std::lrint(16000*g[2])) && mixXeenAudio(-20000,0,0,g,1)==-20000,"Fixed gain per source differs");
  const auto coherent=mixXeenAudio(int(32767*kXeenFmGain),32767,int(32767*kXeenCdGain),g,0);
  check(coherent>27900 && coherent<28100 && mixXeenAudio(-32768,-32768,-32768,XeenCardMixer{}.gains(),0)==-32768,"All-source nominal headroom changed");
 }
 {
  auto fm=std::make_shared<XeenFmSource>();XeenAudio audio(3,driver,loader,module,0,false,{},fm);
  audio.enterTitle(0);audio.requestInn(0);audio.advance(PromusDriver::clockBoundary(200));
  audio.startDigital("long.voc",archive,PromusDriver::clockBoundary(200));audio.prepareRender(48000);
  std::array<std::int16_t,1024> pcm{};
  for(unsigned n=0;n<200;++n) {
   if(n%20==0)audio.effect(20,PromusDriver::clockBoundary(200)); // Producer allocation is permitted.
   render_allocation_test::active=true;audio.render(pcm.data(),512,48000);render_allocation_test::active=false;
  }
  check(render_allocation_test::count==0,"Mixer/FM request consumer allocated callback memory");
 }
 {
  auto fm=std::make_shared<XeenFmSource>();XeenAudio audio(3,driver,loader,module,0,false,{},fm);audio.enterTitle(0);
  XeenFmSource raw;raw.initialize(driver);raw.load(std::make_shared<const Bytes>(loader("newbrigh.m",XeenSceneArchive::DarksideOnly)),true);
  std::vector<std::int16_t> actual(8192),expected(8192);raw.pull(expected.data(),4096);render(audio,actual,73,49716);
  for(unsigned i=0;i<actual.size();++i)check(actual[i]==expected[i]/2,"FM gain was omitted or applied twice");
  // Command 0x100 scales FM from the next mix block; a program start restores the init levels.
  audio.mixer(95,0);const auto lowered=audio.mixerGains();check(std::abs(lowered[0]-std::pow(10.0f,-0.8f))<1e-6f && lowered[2]==audio.mixerGains()[3],"Mixer 95 is not FM -16 dB");
  raw.pull(expected.data(),4096);render(audio,actual,73,49716);
  for(unsigned i=0;i<actual.size();++i)check(actual[i]==std::lrint(float(expected[i]/2)*lowered[0]),"FM mixer gain not applied at the mix");
  audio.mixer(75,0);check(std::abs(audio.mixerGains()[1]-0.1f)<1e-6f,"Mixer 75 is not FM -20 dB");
  audio.startProgram(0);check(audio.mixerGains()[0]==1 && std::abs(audio.mixerGains()[2]-std::pow(10.0f,-1.2f)*kXeenSourceGain)<1e-7f,"Program start did not rewrite init levels");
  audio.processMixer(false);check(audio.mixerGains()[0]==1 && audio.mixerGains()[2]==kXeenSourceGain,"CFG flag 0 must keep the SB16 reset levels");
  audio.mixer(95,0);audio.startProgram(0);check(audio.mixerGains()[0]<0.2f,"Init without its flag rewrote FM");
  XeenAudio silent(0,{},loader,module,0,false);const auto before=silent.mixerGains();silent.mixer(75,0);
  check(silent.mixerGains()==before && before[2]<0.03f,"Only promus implements command 0x100; every driver init writes the mixer");
 }
 // VOC's rational phase survives every live output rate and callback size.
 for(unsigned block:{1,73,512,2049}) {
  XeenAudio audio(0,{},loader,module,0,false);audio.startDigital("ramp.voc",archive,0);long double seconds=0;
  for(int rate:{48000,44100,96000,32000,48000}) {
   check(audio.prepareRender(rate),"Live VOC rate change failed");
   for(unsigned f=0;f<1987;) {
    const auto n=std::min(block,1987-f);std::vector<std::int16_t> pcm(n*2);audio.render(pcm.data(),n,rate);
    for(unsigned i=0;i<n;++i) {
     const auto index=std::uint64_t(seconds*1000000/68+1e-8L);
     const auto expected=std::lrint(int((index%16)*7*256)*audio.mixerGains()[2]);
     check(pcm[i*2]==expected && pcm[i*2+1]==expected,"VOC cursor jumped on rate change");seconds+=1.0L/rate;
    }
    f+=n;
   }
  }
 }
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
 // Partitioning must change neither source cursor nor PCM. Fixed source gains;
 // measured balance is documented in audio.md.
 for(int rate:{22050,44100,48000})for(unsigned block:{1,73,512,2049}) {
  auto fm=std::make_shared<XeenFmSource>(),both=std::make_shared<XeenFmSource>();
  XeenAudio music(3,driver,loader,module,0,false,{},fm),mix(3,driver,loader,module,0,false,{},both);
  music.enterTitle(0);mix.enterTitle(0);mix.startDigital("a.voc",archive,0);
  std::vector<std::int16_t> baseline(8192),actual(8192);
  render(music,baseline,257,rate);render(mix,actual,block,rate);
  const auto end=(512ULL*68*rate+999999)/1000000;
  for(std::size_t f=0;f<actual.size()/2;++f)for(unsigned c=0;c<2;++c)
   check(actual[2*f+c]==std::clamp(int(baseline[2*f+c])+(f<end?int(std::lrint(8192*mix.mixerGains()[2])):0),-32768,32767),"FM/VOC sum, resampled length or callback partition differs");
 }
 for(bool disabled:{false,true}) {
  auto fm=std::make_shared<XeenFmSource>();
  XeenAudio audio(disabled?3:0,disabled?driver:PromusDriver::Bytes{},loader,module,0,false,{},fm);
  if(disabled){audio.enterTitle(0);audio.toggleMusic(false,0);}
  // The loader returns temporary bytes; requests must keep their own PCM.
  const int voc8192=int(std::lrint(8192*audio.mixerGains()[2]));
  audio.startDigital("a.voc",archive,0);
  std::vector<std::int16_t> pcm(34);pull(audio,pcm.data(),17,48000);constant(pcm,voc8192);
  audio.startDigital("b.voc",archive,0);pull(audio,pcm.data(),17,48000);constant(pcm,-voc8192);
  audio.stopDigital(0);pull(audio,pcm.data(),17,48000);constant(pcm,0);
  audio.startDigital("a.voc",archive,0);
  auto options=XeenSoundOptions{};options.efx=false;audio.observeOptions(options);audio.stopDigital(0);
  pull(audio,pcm.data(),17,48000);constant(pcm,voc8192);
  options.efx=true;audio.observeOptions(options);
  audio.startDigital("long.voc",archive,0);audio.startDigital("b.voc",archive,0);
  pull(audio,pcm.data(),17,48000);constant(pcm,-voc8192); // No queued old start.
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
  check(actual[0]==std::clamp(int(base[0])+int(std::lrint(32512*audio.mixerGains()[2])),-32768,32767),"Voice missing before mid-callback command");
  check(actual.back()==base.back(),"Old digital tail survived mid-callback command");
  if(!stop) {
   unsigned replacementFrames=0;
   for(std::size_t f=0;f<actual.size()/2;++f)
    replacementFrames+=actual[2*f]==std::clamp(int(base[2*f])-int(std::lrint(8192*audio.mixerGains()[2])),-32768,32767);
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
  check(pcm[0]==std::clamp(int(base[0])+int(std::lrint(32512*audio.mixerGains()[2])),-32768,32767),"FM fade barrier delayed digital start");
  audio.stopDigital(PromusDriver::clockBoundary(200));pull(audio,pcm.data(),1,48000);
  check(pcm[0]==base[2],"FM fade barrier delayed digital stop");
 }
 // A removed device must discard the current VOC, including its published
 // request. Logical busy stays intact; only a new start makes sound again.
 {
  XeenAudio audio(0,{},loader,module,0,true);audio.startDigital("long.voc",archive,0);
  until([&]{return lastPeak.load()>0;});SdlAudioDevice::removed(openedDevice);
  check(audio.outputActive() && audio.digitalBusy(0),"Removal changed logical VOC state");
  unexpectedSound=false;expectSilence=true;const auto before=callbacks.load();
  until([&]{return callbacks.load()>=before+4;});expectSilence=false;
  check(!unexpectedSound && lastPeak==0,"VOC resumed after removal without a new start");
  audio.startDigital("long.voc",archive,0);until([&]{return lastPeak.load()>0;});
 }
 // Identical route and digital busy boundaries with and without an SDL device.
 std::vector<PromusDriver::Write> traces[7];std::vector<bool> busy[7];
 for(unsigned device=0;device<7;++device) {
  std::atomic<bool> callbackError{false};
  auto source=std::make_shared<XeenFmSource>([&](const auto &){if(callbackError.exchange(false))throw std::runtime_error("injected FM callback failure");});
  XeenAudio audio(3,driver,loader,module,0,device!=0,[&](const auto &w){traces[device].push_back(w);},source);
  check(audio.outputActive()==(device!=0),"Requested dummy device did not open");
  audio.enterTitle(0);audio.startDigital("a.voc",archive,0);
  if(device>=2) {
   const auto end=std::chrono::steady_clock::now()+std::chrono::seconds(5);
   while(source->samples()==0 && std::chrono::steady_clock::now()<end)std::this_thread::yield();
   check(source->samples()>0,"Device did not start before failure injection");
   if(device==2)callbackError=true;
   if(device==3)conversionError=true;
   if(device==4 || device==5) {
    const auto before=source->samples();failOpen=device==5;SdlAudioDevice::removed(openedDevice);failOpen=false;
    check(source->samples()>=before && audio.loadedName()=="newbrigh.m","Reopen restarted the song or its native clock");
   }
   if(device==6){disconnected=true;audio.advance(0);SDL_Delay(260);audio.advance(0);disconnected=false;}
   if(device!=4) {
    while(audio.outputActive() && std::chrono::steady_clock::now()<end)std::this_thread::yield();
    check(!audio.outputActive(),"Failure did not silence output");
   }else check(audio.outputActive(),"Removal did not reopen output");
  }
  for(std::uint64_t t:{1000000,10000000,40000000,2000000000}) {
   audio.advance(t);busy[device].push_back(audio.digitalBusy(t));
   audio.effect(20,t);
  }
  audio.startDigital("long.voc",archive,2000000000);
  if(device>=2 && device!=4)check(!audio.takeCdNotice().empty() && audio.takeCdNotice().empty(),"Output failure notice was missing or repeated");
  if(device) {
   for(unsigned n=0;n<200 && source->samples()==0;++n)std::this_thread::sleep_for(std::chrono::milliseconds(10));
   check(source->samples()>0,"Device never rendered before playing-owner shutdown");
  }
  // Destruction while playing must close/join before PCM, queues and chip die.
 }
 for(unsigned device=1;device<7;++device)check(traces[0].size()==traces[device].size() && std::equal(traces[0].begin(),traces[0].end(),traces[device].begin(),[](const auto &a,const auto &b){
  return a.tick==b.tick && a.reg==b.reg && a.value==b.value && a.interrupt==b.interrupt;
 }) && busy[0]==busy[device],"Device failure/recovery changed logical route trace");
 std::cout<<"Digital commands, mixing, timing and shutdown passed\n";return 0;
}catch(const std::exception &e){std::cerr<<e.what()<<'\n';return 1;}}
