#ifndef MMODERN_FORMATS_XEEN_VOC_H
#define MMODERN_FORMATS_XEEN_VOC_H

#include <cstdint>
#include <stdexcept>
#include <vector>

namespace mmodern {
// INTRO prosnd, not a general VOC parser: fixed PCM address, low length word
// and literal time constant 188. The adopted unsplit count-plus-one model
// ignores DOS allocation-dependent DMA splits; see dev-prosnd-dma-timing.
inline std::size_t xeenVocSampleCount(const std::vector<std::uint8_t> &bytes) {
    if(bytes.size()<0x20)throw std::invalid_argument("VOC too short for prosnd fixed layout (PCM at +0x20)");
    const auto word=unsigned(bytes[0x1b])|(unsigned(bytes[0x1c])<<8);
    const auto count=std::size_t(std::uint16_t(word-10))+1;
    if(count>bytes.size()-0x20)throw std::invalid_argument("VOC too short for prosnd transfer count plus one");
    return count;
}
inline std::uint64_t xeenVocDurationNanoseconds(const std::vector<std::uint8_t> &bytes) {
    return xeenVocSampleCount(bytes)*68000ULL;
}
struct XeenVocSample {
    // Owned unsigned 8-bit mono PCM, exactly 68 microseconds per sample.
    std::vector<std::uint8_t> pcm;
    std::uint64_t durationNanoseconds() const {return pcm.size()*68000ULL;}
};
inline XeenVocSample decodeXeenVoc(const std::vector<std::uint8_t> &bytes) {
    const auto count=xeenVocSampleCount(bytes);
    return {{bytes.begin()+0x20,bytes.begin()+0x20+count}};
}
}
#endif
