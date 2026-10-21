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
 void effect(unsigned,std::uint64_t now);
 // Observe admitted requests after the Efx gate; never drives gameplay.
 using EffectSink=std::function<void(unsigned)>;
 void effectSink(EffectSink);
 void weaponEffect(unsigned weapon,std::uint64_t now);
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
 bool pending() const;
private:
 struct Impl;std::unique_ptr<Impl> _impl;
};
}
#endif
