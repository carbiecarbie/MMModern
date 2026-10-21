#ifndef MMODERN_XEEN_FM_SOURCE_H
#define MMODERN_XEEN_FM_SOURCE_H
#include "games/xeen/PromusDriver.h"
namespace mmodern {
// A single producer publishes completed driver time. The audio consumer may
// pull ahead, but silence then leaves the native timeline and chip untouched.
class XeenFmSource {
public:
 struct Write {std::uint64_t sample;PromusDriver::Write reg;};
 explicit XeenFmSource(PromusDriver::Sink renderedTrace={});
 ~XeenFmSource();
 void publish(std::uint64_t through,std::vector<Write>);
 void pull(std::int16_t *stereo,std::size_t frames);
 std::uint64_t samples() const;
private:
 struct Impl;std::unique_ptr<Impl> _impl;
};
}
#endif
