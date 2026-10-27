#ifndef MMODERN_FORMATS_XEEN_VOC_H
#define MMODERN_FORMATS_XEEN_VOC_H

#include <cstdint>
#include <stdexcept>
#include <vector>

namespace mmodern {
// INTRO blastsnd/prosnd, not a general VOC parser. Both use a fixed PCM
// address, a low length word and literal time constant 188. The adopted
// unsplit count-plus-one model omits allocation-dependent DMA splits.
enum class XeenDmaDigital { Blast, Pro };
inline std::size_t xeenVocSampleCount(const std::vector<std::uint8_t> &bytes,XeenDmaDigital driver=XeenDmaDigital::Pro) {
    if(bytes.size()<0x20)throw std::invalid_argument(driver==XeenDmaDigital::Pro?
        "VOC too short for prosnd fixed layout (PCM at +0x20)":"VOC too short for blastsnd fixed layout (PCM at +0x20)");
    const auto word=unsigned(bytes[0x1b])|(unsigned(bytes[0x1c])<<8);
    const auto count=std::size_t(std::uint16_t(word-(driver==XeenDmaDigital::Blast?14:10)))+1;
    if(count>bytes.size()-0x20)throw std::invalid_argument(driver==XeenDmaDigital::Pro?
        "VOC too short for prosnd transfer count plus one":"VOC too short for blastsnd transfer count plus one");
    return count;
}
inline std::uint64_t xeenVocDurationNanoseconds(const std::vector<std::uint8_t> &bytes,XeenDmaDigital driver=XeenDmaDigital::Pro) {
    return xeenVocSampleCount(bytes,driver)*68000ULL;
}
struct XeenVocSample {
    // Owned unsigned 8-bit mono PCM, exactly 68 microseconds per sample.
    std::vector<std::uint8_t> pcm;
    std::uint64_t durationNanoseconds() const {return pcm.size()*68000ULL;}
};
inline XeenVocSample decodeXeenVoc(const std::vector<std::uint8_t> &bytes,XeenDmaDigital driver=XeenDmaDigital::Pro) {
    const auto count=xeenVocSampleCount(bytes,driver);
    return {{bytes.begin()+0x20,bytes.begin()+0x20+count}};
}
}
#endif
