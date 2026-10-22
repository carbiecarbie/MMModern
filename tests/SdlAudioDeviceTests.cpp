#include "platform/sdl/SdlAudioDevice.h"
#include "platform/sdl/SdlAudioStream.h"
#include "platform/sdl/SdlWindow.h"
#define SDL_MAIN_HANDLED
#include <SDL.h>
#include <algorithm>
#include <array>
#include <atomic>
#include <chrono>
#include <cstddef>
#include <iostream>
#include <stdexcept>
#include <string>
#include <thread>
#include <cmath>
using namespace mmodern;
namespace {
void check(bool value,const std::string &message) {if(!value)throw std::runtime_error(message);}
SDL_AudioDeviceID opened=0;
bool refuseOpen=false,disconnected=false;
int defaultRate=0;
const char *defaultName="injected default";
Uint64 probeTime=0;
std::atomic<bool> failConversion{false};
std::atomic<bool> injectedFailure{false},silentAfterFailure{true};
SDL_AudioCallback originalCallback=nullptr;void *originalContext=nullptr;
SDL_malloc_func originalMalloc=nullptr;SDL_calloc_func originalCalloc=nullptr;
SDL_realloc_func originalRealloc=nullptr;SDL_free_func originalFree=nullptr;
thread_local bool watchAllocations=false;
unsigned converterAllocations=0;
void *trackedMalloc(size_t n){if(watchAllocations)++converterAllocations;return originalMalloc(n);}
void *trackedCalloc(size_t n,size_t size){if(watchAllocations)++converterAllocations;return originalCalloc(n,size);}
void *trackedRealloc(void *p,size_t n){if(watchAllocations)++converterAllocations;return originalRealloc(p,n);}
void trackedFree(void *p){originalFree(p);}
template<class F> void until(F ready) {
 const auto end=std::chrono::steady_clock::now()+std::chrono::seconds(5);
 while(!ready() && std::chrono::steady_clock::now()<end)std::this_thread::sleep_for(std::chrono::milliseconds(1));
 check(ready(),"Callback did not reach expected state");
}
}
extern "C" SDL_AudioDeviceID __real_SDL_OpenAudioDevice(const char *,int,const SDL_AudioSpec *,SDL_AudioSpec *,int);
extern "C" SDL_AudioDeviceID __wrap_SDL_OpenAudioDevice(const char *name,int capture,const SDL_AudioSpec *want,SDL_AudioSpec *have,int changes) {
 if(refuseOpen){SDL_SetError("injected reopen failure");return 0;}
 auto monitored=*want;originalCallback=want->callback;originalContext=want->userdata;
 monitored.callback=[](void *,Uint8 *pcm,int bytes) {
  originalCallback(originalContext,pcm,bytes);
  if(injectedFailure && std::any_of(pcm,pcm+bytes,[](auto b){return b!=0;}))silentAfterFailure=false;
 };
 opened=__real_SDL_OpenAudioDevice(name,capture,&monitored,have,changes);return opened;
}
extern "C" int __real_SDL_GetDefaultAudioInfo(char **,SDL_AudioSpec *,int);
extern "C" int __wrap_SDL_GetDefaultAudioInfo(char **name,SDL_AudioSpec *spec,int capture) {
 if(!defaultRate)return __real_SDL_GetDefaultAudioInfo(name,spec,capture);
 *name=SDL_strdup(defaultName);SDL_zero(*spec);spec->freq=defaultRate;return 0;
}
extern "C" SDL_AudioStatus __real_SDL_GetAudioDeviceStatus(SDL_AudioDeviceID);
extern "C" SDL_AudioStatus __wrap_SDL_GetAudioDeviceStatus(SDL_AudioDeviceID id) {
 return disconnected?SDL_AUDIO_STOPPED:__real_SDL_GetAudioDeviceStatus(id);
}
extern "C" Uint64 __real_SDL_GetTicks64();
extern "C" Uint64 __wrap_SDL_GetTicks64(){return probeTime?probeTime:__real_SDL_GetTicks64();}
extern "C" int __real_SDL_AudioStreamGet(SDL_AudioStream *,void *,int);
extern "C" int __wrap_SDL_AudioStreamGet(SDL_AudioStream *stream,void *out,int bytes) {
 if(failConversion.exchange(false)){injectedFailure=true;SDL_SetError("injected conversion failure");return -1;}
 return __real_SDL_AudioStreamGet(stream,out,bytes);
}
// The device opens at its own mix rate, never at the chip's 49716 Hz: letting
// SDL accept 49716 made it resample internally and drop blocks on WASAPI.
int main() {try {
 SDL_GetMemoryFunctions(&originalMalloc,&originalCalloc,&originalRealloc,&originalFree);
 check(SDL_SetMemoryFunctions(trackedMalloc,trackedCalloc,trackedRealloc,trackedFree)==0,"Could not instrument converter allocations");
 check(SdlAudioDevice::preferredRate(44100)==44100,"Reported rate not kept");
 check(SdlAudioDevice::preferredRate(0)==48000,"Missing rate not 48000");
 SDL_SetMainReady();check(SDL_InitSubSystem(SDL_INIT_AUDIO)==0,"SDL audio unavailable");
 SDL_AudioSpec native{};char *name=nullptr;
 const int expected=SDL_GetDefaultAudioInfo(&name,&native,0)==0?SdlAudioDevice::preferredRate(native.freq):48000;SDL_free(name);
 std::atomic<std::size_t> frames{0};
 {
  SdlAudioDevice device([&](std::int16_t *stereo,std::size_t count,int rate){check(rate==expected,"Callback rate differs");std::fill_n(stereo,count*2,std::int16_t(0));frames+=count;});
  check(device.active(),"Device did not open");
  check(device.rate()==expected,"Device opened at "+std::to_string(device.rate())+", not "+std::to_string(expected));
  check(device.rate()!=49716,"Device opened at the chip rate");
  for(int i=0;i<200 && !frames;++i)std::this_thread::sleep_for(std::chrono::milliseconds(10));
  check(frames>0,"Device never pulled the source");
  device.check();
 }
 // Callback and conversion errors must erase even a partially written buffer,
 // close on check(), issue one notice and never escape to gameplay.
 for(bool conversion:{false,true}) {
  injectedFailure=false;silentAfterFailure=true;
  std::atomic<unsigned> calls{0};std::atomic<bool> inject{false};
  SdlAudioStream stream(49716,48000);
  SdlAudioDevice failed([&](auto *pcm,auto frames,auto) {
   std::fill_n(pcm,frames*2,1234);++calls;
   if(conversion) {
    stream.pull([](auto *p,auto n){std::fill_n(p,n*2,1234);},pcm,frames);
   }else if(inject.load()){injectedFailure=true;throw std::runtime_error("injected callback failure");}
  });
  until([&]{return calls.load()>0;});
  if(conversion)failConversion=true;else inject=true;
  until([&]{return !failed.active();});
  check(!failed.check().empty() && !failed.active(),"Failure did not close and notify");
  const auto stopped=calls.load();
  for(unsigned i=0;i<5;++i)check(failed.check().empty(),"Failure notice repeated");
  std::this_thread::sleep_for(std::chrono::milliseconds(20));
  check(calls==stopped,"Callback survived permanent failure");
  check(silentAfterFailure,"Partial callback output escaped after failure");
 }
 injectedFailure=false;
 // A removal hook can fail while clearing a real SDL converter. It must
 // silence/notify once without reopening or escaping a direct removed() call.
 for(bool unknown:{false,true}) {
  SdlAudioStream stream(49716,48000);std::array<std::int16_t,1024> pcm{};
  stream.pull([](auto *p,auto n){std::fill_n(p,n*2,1234);},pcm.data(),512);
  unsigned prepares=0;std::atomic<unsigned> calls{0};
  SdlAudioDevice device([&](auto *,auto,auto){++calls;},true,[&](int){++prepares;return true;},[&]{
   if(unknown)throw 7;
   failConversion=true;stream.clear();
  });
  until([&]{return calls.load()>0;});
  SdlAudioDevice::removed(opened);
  const auto stopped=calls.load();
  check(!device.active() && prepares==1,"Failed removal cleanup reopened output");
  check(device.check().find("removal cleanup failure")!=std::string::npos,"Missing removal failure notice");
  check(device.check().empty(),"Removal failure notice repeated");
  SdlAudioDevice::removed(opened);
  std::this_thread::sleep_for(std::chrono::milliseconds(20));
  check(calls==stopped,"Failed removal cleanup kept callback running");
 }
 injectedFailure=false;
 // Native event loop filters capture/unrelated/stale events and recovers the
 // opened output on the main thread after its callback has joined.
 {
  SDL_setenv("SDL_VIDEODRIVER","dummy",1);SDL_setenv("SDL_RENDER_DRIVER","software",1);
  check(SDL_InitSubSystem(SDL_INIT_VIDEO|SDL_INIT_EVENTS)==0,"Dummy video init");
  const auto mainThread=std::this_thread::get_id();unsigned prepares=0,removals=0;probeTime=1000;
  SdlAudioDevice inactive([](auto *,auto,auto){},false);
  SdlAudioDevice::removed(0);check(!inactive.active(),"Removal matched an unopened device");
  std::atomic<unsigned> calls{0};
  SdlAudioDevice device([&](auto *,auto,auto){++calls;},true,[&](int) {
   check(std::this_thread::get_id()==mainThread,"Prepare ran off main thread");++prepares;return true;
  },[&]{check(std::this_thread::get_id()==mainThread,"Removal ran off main thread");++removals;disconnected=false;});
  const auto old=opened;until([&]{return calls.load()>0;});
  disconnected=true;check(device.check().empty() && device.active(),"STOPPED bypassed removal grace");
  probeTime+=100;check(device.check().empty(),"STOPPED failed before 250 ms");
  SDL_Event event{};event.type=SDL_AUDIODEVICEREMOVED;event.adevice.which=old+100;
  SDL_PushEvent(&event);event.adevice.which=old;event.adevice.iscapture=1;SDL_PushEvent(&event);
  event.adevice.iscapture=0;SDL_PushEvent(&event);SDL_PushEvent(&event);event.type=SDL_QUIT;SDL_PushEvent(&event);
  check(SdlWindow().show(IndexedFrame(320,200,std::vector<std::uint8_t>(64000)),"Audio removal test"),"Native event loop failed");
  check(prepares==2 && removals==1 && device.active(),"Removal routing did not reopen exactly once");
  check(device.check().empty(),"STOPPED then REMOVED produced a failure notice");probeTime=0;
  refuseOpen=true;SdlAudioDevice::removed(opened);refuseOpen=false;
  check(!device.active() && !device.check().empty() && device.check().empty(),"Failed reopen contract");
  SDL_QuitSubSystem(SDL_INIT_VIDEO|SDL_INIT_EVENTS);
 }
 {
  unsigned prepares=0;probeTime=1000;defaultRate=48000;
  SdlAudioDevice device([](auto *,auto,auto){},true,[&](int){++prepares;return true;});
  defaultRate=44100;probeTime+=251;check(device.check().empty(),"Rate change produced failure");
  check(prepares==2 && device.rate()==44100,"Default rate change did not reopen at its rate");
  defaultName="different default, same rate";probeTime+=251;
  check(device.check().empty() && prepares==3 && device.rate()==44100,"Name-only default change was ignored");
  disconnected=true;device.check();probeTime+=251;
  check(!device.check().empty() && !device.active() && device.check().empty(),"Bare backend disconnect contract");
  disconnected=false;defaultRate=0;probeTime=0;defaultName="injected default";
 }
 // A live rate change discards one small converter tail. Thereafter output is
 // exactly a fresh conversion at the existing source cursor, with small refills.
 for(unsigned block:{1,73,512,2049}) {
  SdlAudioStream stream(49716,48000);std::uint64_t generated=0;
  auto source=[&](auto *out,auto n) {
   check(n<=512,"Converter requested a large native refill");
   for(std::size_t i=0;i<n;++i,++generated)out[i*2]=out[i*2+1]=std::lround(12000*std::sin(generated*100.0*6.283185307179586/49716));
  };
  for(int rate:{48000,44100,96000,32000,48000}) {
   const auto before=generated;check(stream.reconfigure(rate),"Rate reconfiguration failed");
   check(generated==before,"Preparing conversion advanced the source");
   SdlAudioStream reference(49716,rate);auto cursor=before;
   auto fresh=[&](auto *out,auto n) {for(std::size_t i=0;i<n;++i,++cursor)out[i*2]=out[i*2+1]=std::lround(12000*std::sin(cursor*100.0*6.283185307179586/49716));};
   for(unsigned f=0;f<5000;) {
    const auto count=std::min(block,5000-f);std::vector<std::int16_t> pcm(count*2);stream.pull(source,pcm.data(),count);
    std::vector<std::int16_t> expected(count*2);reference.pull(fresh,expected.data(),count);
    check(pcm==expected,"Rate change restarted the source or left continuous converter gaps");
    f+=count;
   }
  }
 }
 for(int sourceRate:{44100,49716})for(int rate:{11025,22050,44100,48000,96000,192000}) {
  SdlAudioStream stream(sourceRate,rate);std::array<std::int16_t,1024> pcm{};
  SdlAudioStream::Source source=[](auto *out,auto n){std::fill_n(out,n*2,1234);};
  watchAllocations=true;
  for(unsigned n=0;n<100;++n){if(n==50)stream.clear();stream.pull(source,pcm.data(),512);}
  watchAllocations=false;
 }
 check(converterAllocations==0,"Converter allocated callback memory despite preparation");
 {
  // A full-scale square wave has inter-sample peaks above S16. Attenuation
  // before conversion must retain those peaks instead of attenuating a clip.
  SdlAudioStream gained(44100,48000,0.34375f),unity(44100,48000);
  unsigned gainedCursor=0,rawCursor=0;
  auto source=[](unsigned &cursor,auto *out,auto n) {
   for(std::size_t i=0;i<n;++i,++cursor)out[i*2]=out[i*2+1]=(cursor/4)%2?32767:-32768;
  };
  std::array<std::int16_t,4096> scaled{},raw{};
  gained.pull([&](auto *p,auto n){source(gainedCursor,p,n);},scaled.data(),2048);
  unity.pull([&](auto *p,auto n){source(rawCursor,p,n);},raw.data(),2048);
  int peak=0;bool preserved=false;
  for(unsigned i=128;i<scaled.size();++i) {
   peak=std::max(peak,std::abs(int(scaled[i])));
   if(std::abs(int(scaled[i]))>std::abs(int(raw[i]*0.34375f))+100)preserved=true;
  }
  check(preserved && peak>11400 && peak<15000,"Source gain happened after inter-sample clipping");
 }
 const auto closedFrames=frames.load();
 std::this_thread::sleep_for(std::chrono::milliseconds(20));
 check(frames==closedFrames,"Callback continued after device shutdown");
 {
  // Exercise a real SDL stream-creation failure before the device unpauses.
  // It must keep the old silent-open behavior, with no callback failure.
  bool prepared=false;std::atomic<unsigned> calls{0};
  SdlAudioDevice failed([&](auto *,auto,auto){++calls;},true,[&](int rate) {
   prepared=true;SdlAudioStream invalid(0,rate);return invalid.valid();
  });
  check(prepared && !failed.active() && failed.rate()==0,"Conversion failure did not leave a silent device");
  failed.check();check(calls==0,"Callback started before conversion was prepared");
 }
 SDL_QuitSubSystem(SDL_INIT_AUDIO);
 std::cout<<"SDL audio device opens at "<<expected<<" Hz\n";
 return 0;
} catch(const std::exception &e){std::cerr<<e.what()<<'\n';return 1;}}
