#ifndef MMODERN_XEEN_CD_SPEECH_TIMING_H
#define MMODERN_XEEN_CD_SPEECH_TIMING_H
#include "games/xeen/XeenEventDecoder.h"
#include <stdexcept>
namespace mmodern {
// DAT 0x3daec..0x3dbbf: final digits are frames, with 25-sector preroll.
inline std::uint64_t xeenCdSpeechDurationNanoseconds(const XeenEventCdSpeech &cue) {
    // DAT 0x3da98..0x3daa7: the all-zero request does not start or stop audio.
    if(!cue.track && !cue.start && !cue.finish)return 0;
    const auto frames=[](unsigned value) {return ((value/10000)*60+(value%10000)/100)*75+value%100;};
    const auto start=frames(cue.start),finish=frames(cue.finish);
    if(cue.track>=100 || finish+25<start)throw std::invalid_argument("Unsupported CD speech span");
    return (std::uint64_t(finish+25-start)*1000000000+74)/75;
}
}
#endif
