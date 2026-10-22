#include "platform/sdl/SdlAudioDevice.h"
#define SDL_MAIN_HANDLED
#include <SDL.h>
#include <exception>
#include <mutex>
#include <stdexcept>
namespace mmodern {
struct SdlAudioDevice::Impl {
 Source source;SDL_AudioDeviceID device=0;int rate=0;
 bool initialized=false;std::exception_ptr failure;std::mutex errors;
 explicit Impl(Source s):source(std::move(s)){}
 static void callback(void *context,Uint8 *output,int bytes) noexcept {
  auto &self=*static_cast<Impl *>(context);SDL_memset(output,0,bytes);
  try {
   self.source(reinterpret_cast<std::int16_t *>(output),bytes/(2*sizeof(std::int16_t)),self.rate);
  }catch(...){std::lock_guard<std::mutex> lock(self.errors);if(!self.failure)self.failure=std::current_exception();}
 }
 // SDL_CloseAudioDevice waits for an in-flight callback before source dies.
 ~Impl(){if(device)SDL_CloseAudioDevice(device);if(initialized)SDL_QuitSubSystem(SDL_INIT_AUDIO);}
};
SdlAudioDevice::SdlAudioDevice(Source source,bool open,Prepare prepare):_impl(new Impl(std::move(source))) {
 if(!open)return;
 SDL_SetMainReady();if(SDL_InitSubSystem(SDL_INIT_AUDIO)!=0)return;
 _impl->initialized=true;
 // Open at the device's own mix rate and convert from the chip's 49716 Hz in
 // the owner's mixer. Letting SDL accept 49716 made WASAPI resample internally,
 // which dropped ~9 ms blocks about once a second (heard as crackle).
 SDL_AudioSpec desired{},obtained{},native{};char *name=nullptr;
 desired.freq=SDL_GetDefaultAudioInfo(&name,&native,0)==0?preferredRate(native.freq):preferredRate(0);SDL_free(name);
 desired.format=AUDIO_S16SYS;desired.channels=2;desired.samples=512;
 desired.callback=Impl::callback;desired.userdata=_impl.get();
 _impl->device=SDL_OpenAudioDevice(nullptr,0,&desired,&obtained,0);
 if(!_impl->device)return;
 _impl->rate=obtained.freq;
 if(prepare && !prepare(_impl->rate)) {SDL_CloseAudioDevice(_impl->device);_impl->device=0;return;}
 SDL_PauseAudioDevice(_impl->device,0);
}
SdlAudioDevice::~SdlAudioDevice()=default;
bool SdlAudioDevice::active() const {return _impl->device!=0;}
int SdlAudioDevice::rate() const {return _impl->device?_impl->rate:0;}
int SdlAudioDevice::preferredRate(int reported) {return reported>0?reported:48000;}
void SdlAudioDevice::check(){std::lock_guard<std::mutex> lock(_impl->errors);if(_impl->failure)std::rethrow_exception(_impl->failure);}
}
