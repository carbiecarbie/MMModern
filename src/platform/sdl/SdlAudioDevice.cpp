#include "platform/sdl/SdlAudioDevice.h"
#define SDL_MAIN_HANDLED
#include <SDL.h>
#include <array>
#include <algorithm>
#include <exception>
#include <mutex>
#include <stdexcept>
namespace mmodern {
struct SdlAudioDevice::Impl {
 Source source;SDL_AudioDeviceID device=0;SDL_AudioStream *fm=nullptr;
 bool initialized=false;std::exception_ptr failure;std::mutex errors;
 explicit Impl(Source s):source(std::move(s)){}
 static void callback(void *context,Uint8 *output,int bytes) noexcept {
  auto &self=*static_cast<Impl *>(context);SDL_memset(output,0,bytes);
  try {
   std::array<std::int16_t,1024> native{},mixed{};
   for(;;) {
    const auto available=SDL_AudioStreamAvailable(self.fm);
    if(available<0)throw std::runtime_error(SDL_GetError());
    if(available>=bytes)break;
    self.source(native.data(),native.size()/2);
    if(SDL_AudioStreamPut(self.fm,native.data(),sizeof(native))<0)throw std::runtime_error(SDL_GetError());
   }
   int offset=0;
   while(offset<bytes) {
    const auto count=std::min<int>(sizeof(mixed),bytes-offset);
    const auto got=SDL_AudioStreamGet(self.fm,mixed.data(),count);
    if(got<0)throw std::runtime_error(SDL_GetError());
    if(!got)break;
    // M83 adds independent resamplers here; each source is mixed once at the device rate.
    SDL_MixAudioFormat(output+offset,reinterpret_cast<const Uint8 *>(mixed.data()),AUDIO_S16SYS,got,SDL_MIX_MAXVOLUME);
    offset+=got;
   }
  }catch(...){std::lock_guard<std::mutex> lock(self.errors);if(!self.failure)self.failure=std::current_exception();}
 }
 ~Impl(){if(device)SDL_CloseAudioDevice(device);if(fm)SDL_FreeAudioStream(fm);if(initialized)SDL_QuitSubSystem(SDL_INIT_AUDIO);}
};
SdlAudioDevice::SdlAudioDevice(Source source,bool open):_impl(new Impl(std::move(source))) {
 if(!open)return;
 SDL_SetMainReady();if(SDL_InitSubSystem(SDL_INIT_AUDIO)!=0)return;
 _impl->initialized=true;
 SDL_AudioSpec desired{},obtained{};
 desired.freq=49716;desired.format=AUDIO_S16SYS;desired.channels=2;desired.samples=512;
 desired.callback=Impl::callback;desired.userdata=_impl.get();
 _impl->device=SDL_OpenAudioDevice(nullptr,0,&desired,&obtained,SDL_AUDIO_ALLOW_FREQUENCY_CHANGE);
 if(!_impl->device)return;
 _impl->fm=SDL_NewAudioStream(AUDIO_S16SYS,2,49716,obtained.format,obtained.channels,obtained.freq);
 if(!_impl->fm){SDL_CloseAudioDevice(_impl->device);_impl->device=0;return;}
 SDL_PauseAudioDevice(_impl->device,0);
}
SdlAudioDevice::~SdlAudioDevice()=default;
bool SdlAudioDevice::active() const {return _impl->device!=0;}
void SdlAudioDevice::check(){std::lock_guard<std::mutex> lock(_impl->errors);if(_impl->failure)std::rethrow_exception(_impl->failure);}
}
