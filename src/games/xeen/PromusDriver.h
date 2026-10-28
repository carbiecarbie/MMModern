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
 // Complete MIDI messages, timestamped on the renderer-only MPU timeline.
 // Storage is bounded so serializing a song SysEx never allocates in a callback.
 struct MidiWrite {
  std::uint64_t tick,timeNanoseconds;std::array<std::uint8_t,4096> bytes{};
  std::size_t size=0;bool interrupt=false;
 };
 using MidiSink=std::function<void(const MidiWrite &)>;
 using MpuCommandSink=std::function<void(unsigned)>;
 static constexpr std::uint64_t pit=1193182,divisor=16390,rate=49716;
 explicit PromusDriver(Bytes resource,Sink sink={},unsigned profile=3,MidiSink midi={},MpuCommandSink mpu={});
 void sink(Sink sink) {_sink=std::move(sink);}
 void load(Bytes song,bool start=true);
 void command(unsigned command,unsigned argument=0);
 void effect(unsigned id);
 void restartProgram();
 void midiTime(std::uint64_t now) noexcept {_midiNow=now;}
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
 friend struct RolmusOracleTestAccess;
 struct Stream {
  Bytes bytes;std::size_t pos=0,start=0;
  bool playing=false;unsigned countdown=0;
  std::array<std::size_t,16> patches{};
  std::vector<std::size_t> stack;
 };
 Bytes _resource;Sink _sink;MidiSink _midiSink;MpuCommandSink _mpuSink;Stream _music,_fx;
 unsigned _profile=3;
 std::vector<std::uint8_t> _image;
 std::array<std::uint8_t,18> _frequency{};
 std::array<std::array<std::uint8_t,9>,2> _carrier{},_levels{};
 struct Slide {bool enabled=false;std::uint8_t rate=0,phase=255;std::int16_t delta=0;};
 std::array<Slide,9> _slides{};
 unsigned _fadeSpeed=0,_fadePhase=0;int _fadeRemaining=0;
 std::uint8_t _mixerLevel=255;
 std::uint64_t _ticks=0;
 std::uint64_t _midiNow=0,_midiCursor=0;
 // Closed healthy DOSBox intelligent-MPU state, on the render timeline only.
 std::array<std::uint8_t,32> _mpuReplies{};
 unsigned _mpuHead=0,_mpuCount=0;int _mpuPending=-1;
 std::uint64_t _mpuResetDeadline=0;bool _mpuReset=false;
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
 void monoInit();void monoCommand(unsigned,unsigned);void monoEffect(unsigned);
 bool monoExecute(Stream &,bool);void monoPostprocess();
 void monoTone(unsigned,unsigned);void monoStop();void monoResetEffects();
 void monoPatch(Stream &,unsigned,unsigned,bool);
 unsigned monoFreq(unsigned) const;void monoCache(unsigned,unsigned);
 unsigned monoNote(unsigned) const;void monoLevel(unsigned,unsigned);
 unsigned monoBase() const {return _profile==1?0:2;}
 void rolInit();void rolCommand(unsigned,unsigned);void rolEffect(unsigned);
 bool rolExecute(Stream &,bool);void rolPostprocess();void rolStop();
 void rolShort(unsigned,unsigned,unsigned,unsigned=3);
 void rolSysEx(const std::vector<std::uint8_t> &,std::size_t &,bool);
 void rolMpuUpdate();void rolMpuCommand(unsigned);void rolMpuData();
 void rolMpuReply(std::uint8_t);void rolMpuPoll(unsigned);
 unsigned rolNote(unsigned) const;void rolNoteOff(unsigned,unsigned);
 void rolPitch(unsigned,unsigned);void rolWord(std::size_t,unsigned);
};
}
#endif
