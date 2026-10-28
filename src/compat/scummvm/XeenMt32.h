#ifndef MMODERN_XEEN_MT32_H
#define MMODERN_XEEN_MT32_H
#include "games/xeen/PromusDriver.h"
#include <filesystem>
#include <memory>
#include <string>
#include <vector>
namespace mmodern {
// Direct mt32emu core; ROM discovery and all synth allocation happen before
// the audio callback. An empty override uses the player's Local AppData path.
class XeenMt32 {
public:
 explicit XeenMt32(const std::filesystem::path &romDirectory={});
 ~XeenMt32();
 bool available() const;
 const std::string &notice() const;
 // Reserve room for an indivisible driver operation before emitting it.
 bool hasHeadroom(std::size_t messages,std::size_t sysexBytes) const;
 void send(const PromusDriver::MidiWrite &);
 void render(std::int16_t *stereo,std::size_t frames);
private:
 friend struct XeenMt32TestAccess;
 explicit XeenMt32(const std::vector<std::filesystem::path> &files);
 struct Impl;std::unique_ptr<Impl> _impl;
};
}
#endif
