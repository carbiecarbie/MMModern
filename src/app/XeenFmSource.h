#ifndef MMODERN_XEEN_FM_SOURCE_H
#define MMODERN_XEEN_FM_SOURCE_H
#include "games/xeen/PromusDriver.h"
#include <filesystem>
namespace mmodern {
// The consumer owns its driver and chip; the producer only queues operations.
class XeenFmSource {
public:
 explicit XeenFmSource(PromusDriver::Sink renderedTrace={},std::filesystem::path romDirectory={},PromusDriver::MidiSink midiTrace={});
 ~XeenFmSource();
 // Called once, before the device opens. A null resource selects silence.
 void initialize(PromusDriver::Bytes resource,unsigned profile=3);
 void load(PromusDriver::Bytes song,bool play);
 void command(unsigned command,unsigned argument=0);
 void effect(unsigned id);
 void restartProgram();
 void fadeBarrier();
 void pull(std::int16_t *stereo,std::size_t frames);
 void check() const;
 const std::string &notice() const;
 std::uint64_t samples() const;
 std::uint64_t midiDeferrals() const;
 std::size_t pendingOperations() const;
private:
 struct Impl;std::unique_ptr<Impl> _impl;
};
}
#endif
