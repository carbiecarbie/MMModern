#ifndef MMODERN_SDL_AUDIO_STREAM_H
#define MMODERN_SDL_AUDIO_STREAM_H
#include <cstddef>
#include <cstdint>
#include <functional>
#include <memory>
namespace mmodern {
// Stereo S16 conversion without exposing SDL to the audio owner. Construct
// before starting the device callback; a failed stream leaves output silent.
class SdlAudioStream {
public:
 using Source=std::function<void(std::int16_t *,std::size_t)>;
 // Fixed source gain is applied before SDL's S16 resampler output clamp.
 SdlAudioStream(int sourceRate,int outputRate,float gain=1.0f);
 ~SdlAudioStream();
 bool valid() const;
 // Callback must be stopped. Recreate conversion; discard its small queued tail.
 bool reconfigure(int outputRate);
 void clear();
 void pull(const Source &,std::int16_t *stereo,std::size_t frames);
private:
 struct Impl;std::unique_ptr<Impl> _impl;
};
}
#endif
