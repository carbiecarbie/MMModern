#ifndef MMODERN_XEEN_AUDIO_MIX_H
#define MMODERN_XEEN_AUDIO_MIX_H
#include <cstdint>
namespace mmodern {
// DOSBox Staging 0.83.0 referenced balance; see docs/rules/audio.md.
// Fixed FM/VOC/CD gains 0.5, 0.01875, 0.34375. Their sum is 138/160,
// leaving 1.28 dB for coherent full-scale inputs before interpolation overshoot.
constexpr float kXeenFmGain=0.5f,kXeenCdGain=0.34375f;
// FM/CD arrive already gained before conversion; VOC has no S16 converter.
constexpr std::int16_t mixXeenAudio(std::int16_t fm,std::int16_t voc,std::int16_t cd) {
 const int sum=int(fm)+int(voc)*3/160+int(cd);
 return sum<-32768?-32768:sum>32767?32767:sum;
}
}
#endif
