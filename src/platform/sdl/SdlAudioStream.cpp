#include "platform/sdl/SdlAudioStream.h"
#define SDL_MAIN_HANDLED
#include <SDL.h>
#include <algorithm>
#include <array>
#include <stdexcept>
#include <string>
namespace mmodern {
struct SdlAudioStream::Impl {
 int sourceRate,outputRate;float gain;
 std::array<std::int16_t,1024> refill{};
 unsigned resetRefills=0;
 SDL_AudioStream *stream;
 static bool drain(SDL_AudioStream *stream,std::int16_t *buffer) {
  int available;
  while((available=SDL_AudioStreamAvailable(stream))>0)
   if(SDL_AudioStreamGet(stream,buffer,std::min(available,2048))<=0)return false;
  return available==0;
 }
 static SDL_AudioStream *create(int source,int output,unsigned &refills) {
  auto *stream=SDL_NewAudioStream(AUDIO_S16SYS,2,source,AUDIO_S16SYS,2,output);
  if(!stream)return nullptr;
  // Allocate SDL's conversion workspace and queue packets before callbacks.
  // Drain instead of Clear: Clear trims SDL's packet pool to 8 KiB, too small
  // for a 512-frame refill at high output rates. Flush resets conversion start.
  std::array<std::int16_t,1024> warm{};
  refills=0;
  while(SDL_AudioStreamAvailable(stream)<16384) {
   if(SDL_AudioStreamPut(stream,warm.data(),sizeof(warm))<0) {SDL_FreeAudioStream(stream);return nullptr;}
   ++refills;
  }
  if(SDL_AudioStreamFlush(stream)<0 || !drain(stream,warm.data())) {SDL_FreeAudioStream(stream);return nullptr;}
  return stream;
 }
 Impl(int source,int output,float scale):sourceRate(source),outputRate(output),gain(scale),stream(create(source,output,resetRefills)){}
 ~Impl(){if(stream)SDL_FreeAudioStream(stream);}
};
SdlAudioStream::SdlAudioStream(int source,int output,float gain):_impl(new Impl(source,output,gain)){}
SdlAudioStream::~SdlAudioStream()=default;
bool SdlAudioStream::valid() const {return _impl->stream!=nullptr;}
bool SdlAudioStream::reconfigure(int rate) {
 auto &s=*_impl;if(rate==s.outputRate)return valid();
 unsigned refills=0;auto *next=Impl::create(s.sourceRate,rate,refills);if(!next)return false;
 SDL_FreeAudioStream(s.stream);s.stream=next;s.outputRate=rate;s.resetRefills=refills;return true;
}
void SdlAudioStream::clear() {
 if(!valid())return;
 auto &s=*_impl;
 if(!Impl::drain(s.stream,s.refill.data()))throw std::runtime_error(SDL_GetError());
 // Discard the old filter tail, replace its state with silence and retain SDL's
 // warmed storage. No source is pulled and no playback history is replayed.
 for(unsigned n=0;n<s.resetRefills;++n) {
  s.refill.fill(0);
  if(SDL_AudioStreamPut(s.stream,s.refill.data(),sizeof(s.refill))<0 || !Impl::drain(s.stream,s.refill.data()))
   throw std::runtime_error(SDL_GetError());
 }
 if(SDL_AudioStreamFlush(s.stream)<0 || !Impl::drain(s.stream,s.refill.data()))throw std::runtime_error(SDL_GetError());
}
void SdlAudioStream::pull(const Source &source,std::int16_t *stereo,std::size_t frames) {
 if(!valid())throw std::logic_error("Audio conversion stream was not prepared");
 auto &s=*_impl;
 // Bound queued native sound to M82's 512-frame refill. Recreating on rate
 // change accepts one small discontinuity without adding steady-state latency.
 for(std::size_t offset=0;offset<frames;) {
  const auto count=std::min<std::size_t>(512,frames-offset);
  const auto bytes=int(count*4);int available;
  while((available=SDL_AudioStreamAvailable(s.stream))<bytes) {
   if(available<0)throw std::runtime_error(SDL_GetError());
   source(s.refill.data(),512);
   for(auto &sample:s.refill)sample=int(sample*s.gain);
   if(SDL_AudioStreamPut(s.stream,s.refill.data(),sizeof(s.refill))<0)throw std::runtime_error(SDL_GetError());
  }
  const auto got=SDL_AudioStreamGet(s.stream,stereo+offset*2,bytes);
  if(got<0)throw std::runtime_error(std::string("Audio conversion failed: ")+SDL_GetError());
  if(got!=bytes)throw std::runtime_error("Incomplete audio conversion");
  offset+=count;
 }
}
}
