#ifndef MMODERN_XEEN_WALL_CLOCK_H
#define MMODERN_XEEN_WALL_CLOCK_H
#include <cstdint>
#include <functional>
#include <stdexcept>
#include <string>
#include <utility>
namespace mmodern {
struct XeenCivilTime { int year,month,day,hour,minute,second; };
// Borland's no-TZ DOS runtime; deliberately independent of the host TZ database.
bool xeenDosDaylightTime(XeenCivilTime);
std::uint32_t xeenDosTime(XeenCivilTime);
XeenCivilTime xeenDosLocalTime(std::uint32_t);
std::string xeenPanelCivilTime(std::uint32_t);
std::string xeenPanelPlayTime(std::uint32_t);
class XeenWallClock {
public:
 using Source=std::function<std::uint32_t()>;
 struct Sample {
  std::uint32_t seconds;unsigned hundredth;XeenCivilTime civil;
  // Keep DOS seconds for play time and the original civil fields for INT 21h.
  static Sample fromCivil(XeenCivilTime civil,unsigned hundredth) {
   return {xeenDosTime(civil),hundredth,civil};
  }
 };
 using SampleSource=std::function<Sample()>;
 explicit XeenWallClock(Source source) {
  if(!source)throw std::invalid_argument("Missing Xeen wall clock");
  // Seconds-only injected/dev clocks have a deterministic civil interpretation.
  _source=[source=std::move(source)]{const auto s=source();return Sample{s,0,xeenDosLocalTime(s)};};
 }
 static XeenWallClock fixed(std::uint32_t seconds) {return XeenWallClock([seconds]{return seconds;});}
 static XeenWallClock withHundredths(SampleSource source) {
  if(!source)throw std::invalid_argument("Missing Xeen wall clock");
  auto clock=fixed(0);clock._source=std::move(source);return clock;
 }
 Sample sample() const {auto s=_source();if(s.hundredth>=100)throw std::invalid_argument("Invalid wall-clock hundredth");return s;}
 std::uint32_t now() const {return sample().seconds;}
private:
 SampleSource _source;
};
}
#endif
