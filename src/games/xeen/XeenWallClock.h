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
 explicit XeenWallClock(Source source):_source(std::move(source)) {
  if(!_source)throw std::invalid_argument("Missing Xeen wall clock");
 }
 static XeenWallClock fixed(std::uint32_t seconds) {return XeenWallClock([seconds]{return seconds;});}
 std::uint32_t now() const {return _source();}
private:
 Source _source;
};
}
#endif
