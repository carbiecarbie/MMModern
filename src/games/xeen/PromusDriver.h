#ifndef MMODERN_PROMUS_DRIVER_H
#define MMODERN_PROMUS_DRIVER_H
#include <array>
#include <cstdint>
#include <functional>
#include <memory>
#include <vector>
namespace mmodern {
// DOS INTRO.CC promus, behavioral specification: docs/rules/audio.md.
// The interpreter's timing and register sink need no emulator or device feedback.
class PromusDriver {
public:
 using Bytes=std::shared_ptr<const std::vector<std::uint8_t>>;
 struct Write {std::uint64_t tick;unsigned reg;std::uint8_t value;bool interrupt;};
 using Sink=std::function<void(const Write &)>;
 static constexpr std::uint64_t pit=1193182,divisor=16390,rate=49716;
 explicit PromusDriver(Bytes resource,Sink sink={});
 void sink(Sink sink) {_sink=std::move(sink);}
 void load(Bytes song,bool start=true);
 void command(unsigned command,unsigned argument=0);
 void effect(unsigned id);
 void tick();
 unsigned fadeSpeed() const {return _fadeSpeed;}
 unsigned fadePhase() const {return _fadePhase;}
 std::uint8_t mixerLevel() const {return _mixerLevel;}
 bool musicPlaying() const {return _music.playing;}
 bool effectPlaying() const {return _fx.playing;}
 std::uint64_t ticks() const {return _ticks;}
 // Boundary is ceil(t * rate * divisor / pit), with no floating point drift.
 static std::uint64_t sampleBoundary(std::uint64_t tick);
 static std::uint64_t clockTicks(std::uint64_t nanoseconds);
 static std::uint64_t clockBoundary(std::uint64_t tick);
private:
 struct Stream {
  Bytes bytes;std::size_t pos=0,start=0;
  bool playing=false;unsigned countdown=0;
  std::array<std::size_t,16> patches{};
  std::vector<std::size_t> stack;
 };
 Bytes _resource;Sink _sink;Stream _music,_fx;
 std::array<std::uint8_t,18> _frequency{};
 std::array<std::array<std::uint8_t,9>,2> _carrier{},_levels{};
 struct Slide {bool enabled=false;std::uint8_t rate=0,phase=255;std::int16_t delta=0;};
 std::array<Slide,9> _slides{};
 unsigned _fadeSpeed=0,_fadePhase=0;int _fadeRemaining=0;
 std::uint8_t _mixerLevel=255;
 std::uint64_t _ticks=0;
 bool _interrupt=false;
 unsigned byte(Stream &),word(const std::vector<std::uint8_t> &,std::size_t) const;
 unsigned frequency(unsigned voice) const;
 void cache(unsigned byteIndex,unsigned frequency);
 unsigned note(unsigned value) const;
 void write(unsigned reg,unsigned value);
 void tone(unsigned voice,unsigned frequency);
 void level(unsigned bank,unsigned voice,unsigned attenuation);
 void patch(Stream &,unsigned voice,unsigned index);
 void stopMusic();void resetEffects();
 bool execute(Stream &,bool effects);
 void postprocess();
};
}
#endif
