#ifndef MMODERN_XEEN_AUDIO_MIX_H
#define MMODERN_XEEN_AUDIO_MIX_H
#include <array>
#include <cmath>
#include <cstdint>
namespace mmodern {
// DOSBox Staging 0.83.0 referenced balance; see docs/rules/audio.md.
// One common base g = 1/3 for every source: OPL keeps DOSBox's 1.5 scalar,
// VOC and CD are unity. The card mixer then scales each source.
constexpr float kXeenSourceGain=1.0f/3,kXeenFmGain=1.5f*kXeenSourceGain,kXeenCdGain=kXeenSourceGain;
// Sound Blaster Pro mixer registers as the installed DOSBox Staging SB16
// applies them (write_sb_pro_volume, calc_vol). XEEN.CFG word 10 selects the
// SB path; the music drivers write ports base+4/+5 of the CFG music port.
struct XeenCardMixer {
 // ctmixer_reset: 31 per side for master, FM, DAC and CD.
 std::uint8_t master=0xff,fm=0xff,dac=0xff,cd=0xff;
 // Every music driver's init writes 2e/22/26 = 255 and 04 = 153 when its
 // flag (XEEN.CFG word +0x14) is nonzero, and nothing otherwise.
 void initialize(bool flag) {if(flag){master=fm=0xff;dac=0x99;}}
 // SB16 stores each nibble as (n << 1) | 1; 2 dB per step below 31, one
 // dB less beyond twenty steps. High nibble is left, low nibble right.
 static float level(std::uint8_t reg,unsigned side) {
  const unsigned nibble=side?reg&15:reg>>4,count=31-((nibble<<1)|1);
  return std::pow(10.0f,-0.05f*float(count*2-(count>20?1:0)));
 }
 // FM, VOC and CD gains per side, relative to the source base above.
 std::array<float,6> gains() const {
  std::array<float,6> g{};
  for(unsigned side=0;side<2;++side) {
   const auto m=level(master,side);
   g[side]=m*level(fm,side);g[2+side]=m*level(dac,side)*kXeenSourceGain;g[4+side]=m*level(cd,side);
  }
  return g;
 }
};
// FM/CD arrive with their base gain applied before conversion; VOC has no
// S16 converter. The card-mixer gains apply here, at the final sum.
inline std::int16_t mixXeenAudio(std::int16_t fm,std::int16_t voc,std::int16_t cd,const std::array<float,6> &gains,unsigned side) {
 const float sum=float(fm)*gains[side]+float(voc)*gains[2+side]+float(cd)*gains[4+side];
 return sum<=-32768.0f?-32768:sum>=32767.0f?32767:std::int16_t(std::lrint(sum));
}
}
#endif
