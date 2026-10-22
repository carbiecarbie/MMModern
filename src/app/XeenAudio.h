#ifndef MMODERN_XEEN_AUDIO_H
#define MMODERN_XEEN_AUDIO_H
#include "games/xeen/PromusDriver.h"
#include "games/xeen/XeenSoundOptions.h"
#include "core/GameInstallation.h"
#include "formats/xeen/XeenSpriteDrawOptions.h"
#include <string>
#include <optional>
namespace mmodern {
class XeenAssetSource;class XeenFmSource;struct XeenMapGeometry;
// Program state, independent of every session and save snapshot.
class XeenAudio {
public:
 using Loader=std::function<std::vector<std::uint8_t>(const std::string &,XeenSceneArchive)>;
 XeenAudio(unsigned deviceId,PromusDriver::Bytes driver,Loader,std::vector<std::uint8_t> module,
  std::uint64_t start,bool device=true,PromusDriver::Sink trace={},std::shared_ptr<XeenFmSource> output={});
 ~XeenAudio();
 static std::shared_ptr<XeenAudio> installed(const GameInstallation &,XeenAssetSource &,std::uint64_t now,bool device=true);
 unsigned deviceId() const;
 bool outputActive() const;
 std::string mapSong(const XeenMapGeometry &) const;
 std::string locationSong(unsigned action) const;
 void requestSong(std::string,XeenSceneArchive,unsigned fade,std::uint64_t now);
 void requestMap(const XeenMapGeometry &,std::uint64_t now);
 void requestLocation(unsigned action,std::uint64_t now);
 void requestInn(std::uint64_t now);
 void enterTitle(std::uint64_t now);
 void leaveTitle(std::uint64_t now);
 bool titleTeardownReady(std::uint64_t now);
 void effect(unsigned,std::uint64_t now);
 // Observe admitted requests after the Efx gate; never drives gameplay.
 using EffectSink=std::function<void(unsigned)>;
 void effectSink(EffectSink);
 void weaponEffect(unsigned weapon,std::uint64_t now);
 void missEffect(std::uint64_t now);
 void eventVoice(unsigned index,std::uint64_t now);
 // prosnd wrapper: replacement, no queue. Efx off masks all three commands
 // without stopping a voice already playing. PCM is decoded before publication.
 void startDigital(const std::string &,XeenSceneArchive,std::uint64_t now);
 void stopDigital(std::uint64_t now);
 bool digitalBusy(std::uint64_t now) const;
 std::optional<std::uint64_t> digitalDeadline(std::uint64_t now) const;
 using DigitalSink=std::function<void(const std::string &,XeenSceneArchive)>;
 void digitalSink(DigitalSink);
 // Called before output starts, automatically for a device. False keeps
 // silence. The rate stays fixed for this output lifetime (Part C handles changes).
 bool prepareRender(int deviceRate);
 // Single consumer: device callback, or a prepared device-free renderer. Never
 // call concurrently with the device. Logical busy never consults this path.
 void render(std::int16_t *stereo,std::size_t frames,int deviceRate);
 void observeOptions(XeenSoundOptions);
 void toggleMusic(bool on,std::uint64_t now);
 void advance(std::uint64_t now,std::uint64_t cdSpeechEnd=0);
 // At a session/title clock-source handoff, retain elapsed driver time and
 // fractional phase while accepting the next source's independent epoch.
 void rebaseClock(std::uint64_t sourceNow) noexcept;
 std::optional<std::uint64_t> deadline() const;
 const std::string &currentName() const;
 const std::string &loadedName() const;
 std::uint64_t ticks() const;
 // Presentation-clock position, never taken from the device.
 std::uint64_t samples() const;
 bool pending() const;
private:
 struct Impl;std::unique_ptr<Impl> _impl;
};
}
#endif
