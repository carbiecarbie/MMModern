#ifndef MMODERN_FORMATS_XEEN_AUDIO_CONFIG_H
#define MMODERN_FORMATS_XEEN_AUDIO_CONFIG_H
#include <cstdint>
#include <stdexcept>
#include <string_view>
#include <vector>

namespace mmodern {
enum class XeenAudioProgram { Dat, Exe };

// The two programs use the same CFG words but have different digital tables.
// The raw ids are retained: presentation caption gates test the id, not the
// selected driver's capabilities.
struct XeenAudioConfig {
 std::uint16_t digital=10,digitalPort=0x220,music=0,musicPort=0x220,irq=0,mixer=1,dma=1;

 static XeenAudioConfig parse(const std::vector<std::uint8_t> &bytes) {
  if(bytes.size()<6)throw std::runtime_error("Truncated WORLD/XEEN.CFG");
  const auto word=[&](std::size_t offset) -> std::uint16_t {
   return offset+1<bytes.size()?std::uint16_t(bytes[offset]|unsigned(bytes[offset+1])<<8):0;
  };
  return {word(0),word(2),word(4),word(6),word(8),word(0x14),word(0x18)};
 }
 std::string_view musicDriver() const {
  constexpr std::string_view names[]{"nullmus","admus","blastmus","promus","rolmus","canmus","specmus","ibmmus","wavemus","tapmus"};
  return music<10?names[music]:names[0];
 }
 std::string_view digitalDriver(XeenAudioProgram program) const {
  constexpr std::string_view dat[]{"nullsnd","ibmsnd","adsnd","nullsnd","nullsnd","source","blastsnd","prosnd","specsnd","covxsnd","prosnd"};
  constexpr std::string_view exe[]{"nullsnd","ibmsnd","adsnd","rolsnd","cansnd","source","blastsnd","prosnd","specsnd","covsnd","prosnd"};
  return digital<11?(program==XeenAudioProgram::Dat?dat[digital]:exe[digital]):dat[0];
 }
 bool unavailableExeDigital() const {return digital==3 || digital==4 || digital==9;}
 bool deferredMusic() const {return music==5 || music==7 || music==8 || music==9;}
 bool deferredDigital() const {return digital==1 || digital==2 || digital==5 || digital==8 || digital==9;}
 bool dmaDigital() const {return digital==6 || digital==7 || digital==10;}
 unsigned digitalDma() const {return digital==6?1:dma;}
 bool captions(bool efx) const {return !efx || digital<2;}
};
}
#endif
