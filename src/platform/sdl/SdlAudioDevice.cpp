#include "platform/sdl/SdlAudioDevice.h"
#define SDL_MAIN_HANDLED
#include <SDL.h>
#include <algorithm>
#include <array>
#include <atomic>
#include <exception>
#include <mutex>
#include <vector>
namespace mmodern {
struct SdlAudioDevice::Impl {
 Source source;Prepare prepare;std::function<void()> onRemoved;
 SDL_AudioDeviceID device=0;int rate=0;
 bool initialized=false,disabled=false;
 std::atomic<bool> callbackFailed{false};
 std::array<char,512> error{};std::string notice,defaultName;std::mutex errors;
 Uint64 nextProbe=0,disconnectedAt=0;
 // Non-owning routing only. Registration, events, probes and shutdown all run
 // on the main thread; the callback never visits this list.
 static std::vector<Impl *> &devices(){static std::vector<Impl *> list;return list;}
 Impl(Source s,Prepare p,std::function<void()> r):source(std::move(s)),prepare(std::move(p)),onRemoved(std::move(r)) {devices().push_back(this);}
 static void callback(void *context,Uint8 *output,int bytes) noexcept {
  auto &s=*static_cast<Impl *>(context);SDL_memset(output,0,bytes);
  if(s.callbackFailed.load(std::memory_order_acquire))return;
  try {s.source(reinterpret_cast<std::int16_t *>(output),bytes/4,s.rate);}
  catch(...) {
   SDL_memset(output,0,bytes);
   std::lock_guard<std::mutex> lock(s.errors);
   try {throw;}catch(const std::exception &e){SDL_strlcpy(s.error.data(),e.what(),s.error.size());}
   catch(...){SDL_strlcpy(s.error.data(),"unknown callback error",s.error.size());}
   s.callbackFailed.store(true,std::memory_order_release);
  }
 }
 void close(){if(device)SDL_CloseAudioDevice(device);device=0;rate=0;}
 void fail(const std::string &reason) {
  close();disabled=true;
  if(notice.empty())notice="Audio output unavailable: "+reason+". Continuing without sound.";
 }
 static std::pair<std::string,int> defaults() {
  SDL_AudioSpec native{};char *name=nullptr;
  const auto rate=SDL_GetDefaultAudioInfo(&name,&native,0)==0?SdlAudioDevice::preferredRate(native.freq):48000;
  std::string value=name?name:"";SDL_free(name);return {std::move(value),rate};
 }
 bool open() {
  const auto current=defaults();defaultName=current.first;
  SDL_AudioSpec desired{},obtained{};
  desired.freq=current.second;desired.format=AUDIO_S16SYS;desired.channels=2;desired.samples=512;
  desired.callback=callback;desired.userdata=this;
  device=SDL_OpenAudioDevice(nullptr,0,&desired,&obtained,0);
  if(!device)return false;
  rate=obtained.freq;
  try {
   if(prepare && !prepare(rate)){close();return false;}
  }catch(...){close();return false;}
  SDL_PauseAudioDevice(device,0);disconnectedAt=0;nextProbe=SDL_GetTicks64()+250;return true;
 }
 void reopen(bool removal) {
  if(disabled || !device)return;
  auto old=device;
  close(); // Join before touching resamplers or PCM references.
  // SDL can reuse device IDs immediately. Discard already queued notifications
  // for the joined device before a new device can acquire its ID.
  SDL_FilterEvents([](void *context,SDL_Event *event)->int {
   return !(event->type==SDL_AUDIODEVICEREMOVED && !event->adevice.iscapture &&
            event->adevice.which==*static_cast<const SDL_AudioDeviceID *>(context));
  },&old);
  if(removal && onRemoved)onRemoved();
  if(!open())fail("failed to reopen the default output");
 }
 ~Impl(){close();auto &list=devices();list.erase(std::remove(list.begin(),list.end(),this),list.end());if(initialized)SDL_QuitSubSystem(SDL_INIT_AUDIO);}
};
SdlAudioDevice::SdlAudioDevice(Source source,bool open,Prepare prepare,std::function<void()> removed):_impl(new Impl(std::move(source),std::move(prepare),std::move(removed))) {
 if(!open)return;
 SDL_SetMainReady();if(SDL_InitSubSystem(SDL_INIT_AUDIO)!=0)return;
 _impl->initialized=true;
 if(!_impl->open())_impl->close();
}
SdlAudioDevice::~SdlAudioDevice()=default;
bool SdlAudioDevice::active() const {return _impl->device!=0 && !_impl->callbackFailed.load(std::memory_order_acquire);}
int SdlAudioDevice::rate() const {return active()?_impl->rate:0;}
int SdlAudioDevice::preferredRate(int reported) {return reported>0?reported:48000;}
std::string SdlAudioDevice::check() {
 auto &s=*_impl;
 if(!s.disabled && s.callbackFailed.load(std::memory_order_acquire)) {
  std::string error;{std::lock_guard<std::mutex> lock(s.errors);error=s.error.data();}
  s.fail("callback/conversion failure ("+error+")");
 }
 if(s.device && !s.disabled) {
  // WASAPI can switch defaults without a removal event. Probe SDL's current
  // default and reopen at its native rate, avoiding hidden internal conversion.
  // This wall clock never enters gameplay.
  const auto now=SDL_GetTicks64();
  if(now>=s.nextProbe) {
   s.nextProbe=now+250;const auto current=Impl::defaults();
   if(current.first!=s.defaultName || current.second!=s.rate)s.reopen(false);
  }
  if(s.device && SDL_GetAudioDeviceStatus(s.device)==SDL_AUDIO_STOPPED) {
   // SDL marks STOPPED before posting removal. Give the main event loop time
   // to deliver that event and recover before classifying a bare disconnect.
   if(!s.disconnectedAt)s.disconnectedAt=now+250;
   else if(now>=s.disconnectedAt && !SDL_HasEvent(SDL_AUDIODEVICEREMOVED))
    s.fail("backend disconnected without a removal event");
  }else s.disconnectedAt=0;
 }
 auto notice=std::move(s.notice);s.notice.clear();return notice;
}
void SdlAudioDevice::removed(std::uint32_t id) {
 for(auto *s:Impl::devices())if(s->device && s->device==id) {
  if(s->callbackFailed.load(std::memory_order_acquire))continue;
  s->reopen(true);break;
 }
}
}
