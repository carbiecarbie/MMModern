#ifndef MMODERN_XEEN_OPL3_H
#define MMODERN_XEEN_OPL3_H
#include <cstdint>
#include <memory>
namespace mmodern {
// Plain Nuked state/functions only: no ScummVM audio class or system instance.
class XeenOpl3 {
public:
 static constexpr unsigned rate=49716;
 XeenOpl3();
 ~XeenOpl3();
 void write(unsigned reg,std::uint8_t value);
 void sample(std::int16_t *stereo);
 static bool systemIsNull();
private:
 struct Impl;
 std::unique_ptr<Impl> _impl;
};
}
#endif
