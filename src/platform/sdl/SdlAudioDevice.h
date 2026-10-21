#ifndef MMODERN_SDL_AUDIO_DEVICE_H
#define MMODERN_SDL_AUDIO_DEVICE_H
#include <cstdint>
#include <functional>
#include <memory>
namespace mmodern {
class SdlAudioDevice {
public:
 using Source=std::function<void(std::int16_t *,std::size_t)>;
 explicit SdlAudioDevice(Source source,bool open=true);
 ~SdlAudioDevice();
 bool active() const;
 void check();
private:
 struct Impl;std::unique_ptr<Impl> _impl;
};
}
#endif
