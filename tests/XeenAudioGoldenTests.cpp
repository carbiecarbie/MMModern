#include "app/XeenAudio.h"
#include "app/XeenFmSource.h"
#include "formats/xeen/XeenAssetSource.h"
#include "XeenTestInstallation.h"
#include "formats/xeen/XeenVoc.h"
#define SDL_MAIN_HANDLED
#include <SDL2/SDL.h>
#include <map>
#include <algorithm>
#include <chrono>
#include <cstring>
#include <fstream>
#include <iostream>
#include <thread>

namespace {
enum class Source { All, Opl, Digital, Cd };
Source selectedSource=Source::All;
std::map<SDL_AudioStream *,int> sourceRates;
}
extern "C" SDL_AudioStream *__real_SDL_NewAudioStream(SDL_AudioFormat,Uint8,int,SDL_AudioFormat,Uint8,int);
extern "C" SDL_AudioStream *__wrap_SDL_NewAudioStream(SDL_AudioFormat sourceFormat,Uint8 sourceChannels,int sourceRate,
 SDL_AudioFormat targetFormat,Uint8 targetChannels,int targetRate) {
 auto *stream=__real_SDL_NewAudioStream(sourceFormat,sourceChannels,sourceRate,targetFormat,targetChannels,targetRate);
 if(stream)sourceRates[stream]=sourceRate;
 return stream;
}
extern "C" int __real_SDL_AudioStreamGet(SDL_AudioStream *,void *,int);
extern "C" int __wrap_SDL_AudioStreamGet(SDL_AudioStream *stream,void *out,int bytes) {
 const int count=__real_SDL_AudioStreamGet(stream,out,bytes);
 // Mute at the converter output so every source still consumes the original
 // requests and advances through exactly the same Installed 3/10 scenario.
 if(count>0 && selectedSource!=Source::All) {
  const int rate=sourceRates.at(stream);
  if((rate==49716 && selectedSource!=Source::Opl) || (rate==44100 && selectedSource!=Source::Cd))
   std::memset(out,0,count);
 }
 return count;
}
using namespace mmodern;
namespace mmodern {
struct XeenCdAudioTestAccess {
 static bool pending(const XeenAudio &audio) {return audio.cdDecodePending();}
};
}
namespace {
void check(bool value,const char *message) {if(!value)throw std::runtime_error(message);}
struct Hash {
 std::uint64_t value=14695981039346656037ULL;
 void word(std::uint64_t word,unsigned bytes) {
  for(unsigned n=0;n<bytes;++n){value^=(word>>(n*8))&255;value*=1099511628211ULL;}
 }
};
void decoded(XeenAudio &audio) {
 const auto end=std::chrono::steady_clock::now()+std::chrono::seconds(5);
 while(XeenCdAudioTestAccess::pending(audio) && std::chrono::steady_clock::now()<end)std::this_thread::yield();
 check(!XeenCdAudioTestAccess::pending(audio),"Golden CD decode did not finish");
 check(audio.takeCdNotice().empty(),"Golden CD decode failed");
}
}

// Captured before M94 A1 production changes, dfd05c1 (b4448b8 audio behavior).
// This fixed script exercises the installed 3/10 path without opening a device.
// Keep the expected values unchanged throughout M94; they are regression guards,
// not assertions of physical-card waveform equivalence.
void scenario(const std::filesystem::path &data,Source source) {
 selectedSource=source;sourceRates.clear();
 const auto installation=xeenTestInstallationDetector().detect(data);
 check(bool(installation),"No installation for installed audio golden");
 XeenAssetSource assets(*installation);
 auto cfgFile=installation->configuration->open();
 std::vector<std::uint8_t> cfg(cfgFile->size());cfgFile->read(cfg.data(),cfg.size());
 check(cfg.size()>=0x16 && cfg[0]==10 && cfg[1]==0 && cfg[4]==3 && cfg[5]==0,
       "Installed audio golden requires the calibrated 3/10 configuration");
 const bool mixerFlag=cfg[0x14] || cfg[0x15];
 std::ifstream moduleFile(installation->uiModule->physicalPath,std::ios::binary);
 const std::vector<std::uint8_t> module((std::istreambuf_iterator<char>(moduleFile)),{});
 const auto loader=[&](const std::string &name,XeenSceneArchive archive){return assets.readArchiveResource(name,archive);};
 const auto resource=std::make_shared<const std::vector<std::uint8_t>>(loader("promus",XeenSceneArchive::IntroOnly));
 Hash registers,pcm,gains,timing;
 auto output=std::make_shared<XeenFmSource>();
 XeenAudio audio(3,resource,loader,module,0,false,[&](const PromusDriver::Write &write) {
  registers.word(write.tick,8);registers.word(write.reg,2);registers.word(write.value,1);registers.word(write.interrupt,1);
 },output);
 check(!audio.outputActive(),"Golden opened an audio device");
 audio.processMixer(mixerFlag);
 audio.cdTracks(inspectXeenCdTracks(installation->cdTracks));
 audio.enterTitle(0);
 check(audio.prepareRender(48000),"Golden render preparation failed");
 std::vector<std::int16_t> block(2048);
 std::uint64_t rendered=0,fadeCompleted=0,sampleCount=0;bool audible=false;
 const auto startDigital=[&](const char *name,std::uint64_t now) {
  if(source==Source::All || source==Source::Digital) {
   audio.startDigital(name,XeenSceneArchive::CloudsProgram,now);return;
  }
  auto voice=audio.retainDigital(name,XeenSceneArchive::CloudsProgram);
  auto silent=std::make_shared<XeenVocSample>(*voice.sample);
  std::fill(silent->pcm.begin(),silent->pcm.end(),128);voice.sample=std::move(silent);
  audio.startDigital(voice,now);
 };
 for(unsigned tick=0;tick<=400;++tick) {
  const auto now=PromusDriver::clockBoundary(tick);
  audio.advance(now);
  if(tick==10)audio.effect(20,now);
  if(tick==20) {
   startDigital("crodo1.voc",now);
   audio.retainCdCue({19,25,100});check(audio.startCd(now),"Golden CD cue refused");decoded(audio);
  }
  if(tick==25)audio.mixer(75,now);
  if(tick==35)audio.requestInn(now);
  if(tick>35 && !fadeCompleted && audio.loadedName()=="inn.m")fadeCompleted=tick;
  if(tick==160)audio.mixer(95,now);
  if(tick==175)audio.startProgram(now);
  if(tick==200)audio.stopDigital(now);
  if(tick==210)startDigital("crodo2.voc",now);
  if(tick==235) {
   audio.retainCdCue({19,25,100});check(audio.startCd(now),"Golden replacement CD cue refused");decoded(audio);
  }
  if(tick==270)audio.stopCd(now);
  timing.word(now,8);timing.word(audio.ticks(),8);timing.word(audio.samples(),8);
  timing.word(audio.pending(),1);timing.word(audio.deadline().value_or(0),8);
  timing.word(audio.digitalDeadline(now).value_or(0),8);timing.word(audio.cdDeadline(now).value_or(0),8);
  for(float gain:audio.mixerGains()) {
   std::uint32_t bits=0;static_assert(sizeof(bits)==sizeof(gain));std::memcpy(&bits,&gain,sizeof(bits));gains.word(bits,4);
  }
  const auto target=now*48000/1000000000;
  while(rendered<target) {
   const auto count=std::min<std::uint64_t>(block.size()/2,target-rendered);
   audio.render(block.data(),count,48000);rendered+=count;
   for(std::size_t n=0;n<count*2;++n){pcm.word(std::uint16_t(block[n]),2);audible=audible || block[n]!=0;}
   sampleCount+=count*2;
  }
 }
 if(source==Source::All)std::cout<<"Installed 3/10 golden: registers="<<std::hex<<registers.value<<" PCM="<<pcm.value
          <<" gains="<<gains.value<<" timing="<<timing.value<<std::dec<<" fade tick="<<fadeCompleted<<'\n';
 check(registers.value==0xb17c018ac773a847ULL,"Installed 3/10 register golden changed");
 if(source==Source::All) {
#ifdef _WIN32
  check(pcm.value==0x77a3d33cb53fbbddULL,"Installed 3/10 mixed PCM golden changed");
#else
  // M113 comparison: system libvorbis builds differ by at most 1 LSB in a
  // few samples. Accepted as a decoder-library difference, 2026-11-04.
  check(sampleCount==527476,"Installed 3/10 mixed PCM sample count changed");
#endif
 } else if(source==Source::Opl) {
  check(pcm.value==0xd6f9b63ffefc3d7eULL,"Installed 3/10 OPL-only PCM golden changed");
 } else if(source==Source::Digital) {
  check(pcm.value==0x9b217f3b7d89e435ULL,"Installed 3/10 digital-only PCM golden changed");
 } else {
  check(sampleCount==527476,"Installed 3/10 CD-only PCM sample count changed");
  check(audible,"Installed 3/10 CD source is silent");
 }
 check(gains.value==0x7179aff652cb30d1ULL,"Installed 3/10 mixer golden changed");
 check(timing.value==0xd9bd3e9b003290ecULL && fadeCompleted==163,"Installed 3/10 deadline golden changed");
}
int main(int argc,char **argv) {try {
 check(argc==2,"usage: mmodern_audio_golden_tests <game data>");
 const auto data=std::filesystem::u8path(argv[1]);
 scenario(data,Source::All);scenario(data,Source::Opl);scenario(data,Source::Digital);
#ifndef _WIN32
 scenario(data,Source::Cd);
#endif
 return 0;
}catch(const std::exception &error){std::cerr<<error.what()<<'\n';return 1;}}
