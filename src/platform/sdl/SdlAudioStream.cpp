#include "platform/sdl/SdlAudioStream.h"
#define SDL_MAIN_HANDLED
#include <SDL.h>
#include <array>
#include <stdexcept>
namespace mmodern {
struct SdlAudioStream::Impl {
 SDL_AudioStream *stream;
 Impl(int source,int output):stream(SDL_NewAudioStream(AUDIO_S16SYS,2,source,AUDIO_S16SYS,2,output)){}
 ~Impl(){if(stream)SDL_FreeAudioStream(stream);}
};
SdlAudioStream::SdlAudioStream(int source,int output):_impl(new Impl(source,output)){}
SdlAudioStream::~SdlAudioStream()=default;
bool SdlAudioStream::valid() const {return _impl->stream!=nullptr;}
void SdlAudioStream::pull(const Source &source,std::int16_t *stereo,std::size_t frames) {
 if(!valid())throw std::logic_error("Audio conversion stream was not prepared");
 std::array<std::int16_t,1024> native{};
 const auto bytes=int(frames*2*sizeof(std::int16_t));
 int available;
 while((available=SDL_AudioStreamAvailable(_impl->stream))<bytes) {
  if(available<0)throw std::runtime_error(SDL_GetError());
  source(native.data(),native.size()/2);
  if(SDL_AudioStreamPut(_impl->stream,native.data(),sizeof(native))<0)throw std::runtime_error(SDL_GetError());
 }
 if(SDL_AudioStreamGet(_impl->stream,stereo,bytes)!=bytes)throw std::runtime_error("Incomplete audio conversion");
}
}
