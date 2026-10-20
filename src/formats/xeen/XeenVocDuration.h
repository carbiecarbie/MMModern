#ifndef MMODERN_FORMATS_XEEN_VOC_DURATION_H
#define MMODERN_FORMATS_XEEN_VOC_DURATION_H

#include "core/PresentationClock.h"
#include <algorithm>
#include <cstdint>
#include <string>
#include <vector>

namespace mmodern {
// Timing metadata only; no decoder, mixer or audio scheduler. The original CD
// samples use unsigned mono PCM, time constant 188 (68 microseconds/sample).
inline std::uint64_t xeenVocDurationNanoseconds(const std::vector<std::uint8_t> &bytes) {
    const auto invalid=[] {throw std::invalid_argument("Invalid or unsupported VOC timing data");};
    // DOS plays structurally valid samples regardless of descriptor text.
    // Clouds sphinx13.voc has a damaged descriptor; see original-quirks.md.
    if(bytes.size()<26)invalid();
    const auto little=[&](std::size_t at,unsigned count) {
        if(at>bytes.size() || count>bytes.size()-at)invalid();
        std::uint64_t value=0;
        for(unsigned i=0;i<count;++i)value|=std::uint64_t(bytes[at+i])<<(8*i);
        return value;
    };
    const auto multiply=[&](std::uint64_t a,std::uint64_t b) {
        if(b && a>std::numeric_limits<std::uint64_t>::max()/b)invalid();
        return a*b;
    };
    auto position=std::size_t(little(20,2));
    if(position<26 || position>=bytes.size())invalid();
    std::uint64_t total=0,rate=0;
    unsigned channels=0,bits=0;
    while(position<bytes.size()) {
        const auto type=bytes[position++];
        if(!type)return total;
        const auto length=std::size_t(little(position,3));position+=3;
        if(length>bytes.size()-position)invalid();
        const auto end=position+length;
        std::uint64_t duration=0;
        if(type==1) {
            if(length<2 || bytes[position+1]!=0)invalid();
            const auto period=std::uint64_t(256-bytes[position])*1000;
            duration=multiply(length-2,period);
            // Retain the exact integer period for continuation PCM blocks.
            rate=period;channels=1;bits=8;
        } else if(type==2) {
            if(!rate || !channels || !bits)invalid();
            if(channels!=1 || bits!=8)invalid();
            duration=multiply(length,rate);
        } else if(type==3) {
            if(length!=3)invalid();
            duration=multiply(little(position,2)+1,std::uint64_t(256-bytes[position+2])*1000);
        } else if(type!=4 && type!=5) {
            // Do not guess compressed, repeated or extended-rate timing.
            invalid();
        }
        total=PresentationClock::checkedAdd(total,duration);
        position=end;
    }
    invalid();
    return 0;
}
}
#endif
