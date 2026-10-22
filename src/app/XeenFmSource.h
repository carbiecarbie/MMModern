#ifndef MMODERN_XEEN_FM_SOURCE_H
#define MMODERN_XEEN_FM_SOURCE_H
#include "games/xeen/PromusDriver.h"
namespace mmodern {
// The consumer owns its driver and chip; the producer only queues operations.
class XeenFmSource {
public:
 explicit XeenFmSource(PromusDriver::Sink renderedTrace={});
 ~XeenFmSource();
 // Called once, before the device opens. A null resource selects silence.
 void initialize(PromusDriver::Bytes resource);
 void load(PromusDriver::Bytes song,bool play);
 void command(unsigned command);
 void effect(unsigned id);
 void fadeBarrier();
 void pull(std::int16_t *stereo,std::size_t frames);
 void check() const;
 std::uint64_t samples() const;
private:
 struct Impl;std::unique_ptr<Impl> _impl;
};
}
#endif
