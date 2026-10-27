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
// DOSBox Staging 0.83.0 reference mixers. SB2 installs no mixer ports.
struct XeenCardMixer {
 enum class Model { None, Sb2, SbPro, Sb16 };
 explicit XeenCardMixer(Model selected=Model::Sb16,std::uint16_t port=0x220):model(selected),base(port) {}
 Model model;std::uint16_t base;std::uint8_t index=0,line=0xff;
 // ctmixer_reset: 31 per side for master, FM, DAC and CD.
 std::uint8_t master=0xff,fm=0xff,dac=0xff,cd=0xff;
 void reset() {index=0;line=master=fm=dac=cd=0xff;}
 void write(std::uint16_t port,std::uint8_t value) {
  if(model==Model::None || model==Model::Sb2)return;
  if(port==std::uint16_t(base+4)){index=value;return;}
  if(port!=std::uint16_t(base+5))return;
  switch(index) {
   case 0x00:reset();break;
   case 0x22:master=value;break;
   case 0x26:fm=value;break;
   case 0x04:dac=value;break;
   case 0x28:cd=value;break;
   case 0x2e:line=value;break;
   default:break;
  }
 }
 // The driver's flag gates its writes, which still go to its actual port.
 void initialize(bool flag,std::uint16_t driverPort) {
  if(!flag)return;
  for(const auto &item:std::array<std::array<std::uint8_t,2>,4>{{{{0x2e,0xff}},{{0x22,0xff}},{{0x26,0xff}},{{0x04,0x99}}}}) {
   write(std::uint16_t(driverPort+4),item[0]);write(std::uint16_t(driverPort+5),item[1]);
  }
 }
 void initialize(bool flag) {initialize(flag,base);}
 // SB16 stores each nibble as (n << 1) | 1; 2 dB per step below 31, one
 // dB less beyond twenty steps. High nibble is left, low nibble right.
 static float level(std::uint8_t reg,unsigned side) {
  const unsigned nibble=side?reg&15:reg>>4,count=31-((nibble<<1)|1);
  return std::pow(10.0f,-0.05f*float(count*2-(count>20?1:0)));
 }
 static float proLevel(std::uint8_t reg,unsigned side) {
  const unsigned nibble=side?reg&15:reg>>4,c=31-((nibble<<1)|3);
  if(c==0)return 1.0f;
  if(c>27)return 0.0f;
  const unsigned db=(c<16?c-1:c==16?c:c+1)+(c==24?2:0);
  return std::pow(10.0f,-0.05f*float(db));
 }
 // FM, VOC and CD gains per side, relative to the source base above.
 std::array<float,6> gains() const {
  std::array<float,6> g{};
  for(unsigned side=0;side<2;++side) {
   const auto volume=[&](std::uint8_t value) {
    return model==Model::None || model==Model::Sb2?1.0f:model==Model::SbPro?proLevel(value,side):level(value,side);
   };
   const auto m=volume(master);
   g[side]=m*volume(fm);g[2+side]=m*volume(dac)*kXeenSourceGain;g[4+side]=m*volume(cd);
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
