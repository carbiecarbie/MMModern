#ifndef MMODERN_XEEN_PRESENTATION_RANDOM_H
#define MMODERN_XEEN_PRESENTATION_RANDOM_H
#include "games/xeen/XeenWallClock.h"
#include <array>
namespace mmodern {
// XEEN.EXE's private source; never borrows a Journey generator or saved state.
class XeenPresentationRandom {
public:
 explicit XeenPresentationRandom(XeenWallClock clock,std::array<std::uint16_t,3> words):_clock(std::move(clock)),_words(words) {}
 unsigned range(unsigned lo,unsigned hi);
 static unsigned reduce(std::uint16_t value,unsigned lo,unsigned hi);
 // Time registers and explicit AL for arithmetic vectors. The presentation
 // binding uses zero: its BIOS timer reads consume the midnight indication.
 struct Time {unsigned hour,minute,second,hundredth;unsigned midnight=0;};
 std::uint16_t mix(Time);
private:
 XeenWallClock _clock;
 std::array<std::uint16_t,3> _words;
};
}
#endif
