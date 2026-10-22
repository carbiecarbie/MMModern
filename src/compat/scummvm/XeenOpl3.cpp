#define FORBIDDEN_SYMBOL_ALLOW_ALL
#include "compat/scummvm/XeenOpl3.h"
#include "audio/softsynth/opl/nuked.h"
#include "common/system.h"
namespace mmodern {
struct XeenOpl3::Impl {OPL::NUKED::opl3_chip chip{};};
XeenOpl3::XeenOpl3():_impl(new Impl) {OPL::NUKED::OPL3_Reset(&_impl->chip,rate);}
XeenOpl3::~XeenOpl3()=default;
// DOS delay loops space every write; Nuked sees a key-off only in a generated
// sample, so the pin's buffered spacing keeps a tick's re-strikes audible.
void XeenOpl3::write(unsigned reg,std::uint8_t value) {OPL::NUKED::OPL3_WriteRegBuffered(&_impl->chip,reg,value);}
void XeenOpl3::sample(std::int16_t *stereo) {OPL::NUKED::OPL3_Generate(&_impl->chip,stereo);}
bool XeenOpl3::systemIsNull() {return g_system==nullptr;}
}
