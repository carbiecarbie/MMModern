#include "platform/sdl/SdlAudioDevice.h"
#define SDL_MAIN_HANDLED
#include <SDL.h>
#include <algorithm>
#include <atomic>
#include <chrono>
#include <cstddef>
#include <iostream>
#include <stdexcept>
#include <string>
#include <thread>
using namespace mmodern;
namespace {
void check(bool value,const std::string &message) {if(!value)throw std::runtime_error(message);}
}
// The device opens at its own mix rate, never at the chip's 49716 Hz: letting
// SDL accept 49716 made it resample internally and drop blocks on WASAPI.
int main() {try {
 check(SdlAudioDevice::preferredRate(44100)==44100,"Reported rate not kept");
 check(SdlAudioDevice::preferredRate(0)==48000,"Missing rate not 48000");
 SDL_SetMainReady();check(SDL_InitSubSystem(SDL_INIT_AUDIO)==0,"SDL audio unavailable");
 SDL_AudioSpec native{};char *name=nullptr;
 const int expected=SDL_GetDefaultAudioInfo(&name,&native,0)==0?SdlAudioDevice::preferredRate(native.freq):48000;SDL_free(name);
 std::atomic<std::size_t> frames{0};
 {
  SdlAudioDevice device([&](std::int16_t *stereo,std::size_t count){std::fill_n(stereo,count*2,std::int16_t(0));frames+=count;});
  check(device.active(),"Device did not open");
  check(device.rate()==expected,"Device opened at "+std::to_string(device.rate())+", not "+std::to_string(expected));
  check(device.rate()!=49716,"Device opened at the chip rate");
  for(int i=0;i<200 && !frames;++i)std::this_thread::sleep_for(std::chrono::milliseconds(10));
  check(frames>0,"Device never pulled the source");
  device.check();
 }
 SDL_QuitSubSystem(SDL_INIT_AUDIO);
 std::cout<<"SDL audio device opens at "<<expected<<" Hz\n";
 return 0;
} catch(const std::exception &e){std::cerr<<e.what()<<'\n';return 1;}}
