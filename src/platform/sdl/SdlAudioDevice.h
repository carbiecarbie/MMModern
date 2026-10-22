#ifndef MMODERN_SDL_AUDIO_DEVICE_H
#define MMODERN_SDL_AUDIO_DEVICE_H
#include <cstdint>
#include <functional>
#include <memory>
#include <string>
namespace mmodern {
class SdlAudioDevice {
public:
 // Source supplies stereo S16 already mixed at the opened device rate.
 using Source=std::function<void(std::int16_t *,std::size_t,int)>;
 // Runs while the opened device is paused. False closes it and keeps silence.
 using Prepare=std::function<bool(int)>;
 explicit SdlAudioDevice(Source source,bool open=true,Prepare prepare={},std::function<void()> removed={});
 ~SdlAudioDevice();
 bool active() const;
 // The opened device's mix rate (0 when inactive).
 int rate() const;
 // The device's reported mix rate, or 48000 when it reports none.
 static int preferredRate(int reported);
 // Main thread only. Returns a failure notice once; callback errors never escape.
 std::string check();
 // Called by the native event loop on the main thread, for output events only.
 static void removed(std::uint32_t device);
private:
 struct Impl;std::unique_ptr<Impl> _impl;
};
}
#endif
