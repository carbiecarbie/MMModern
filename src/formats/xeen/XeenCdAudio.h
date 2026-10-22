#ifndef MMODERN_XEEN_CD_AUDIO_H
#define MMODERN_XEEN_CD_AUDIO_H
#include "core/GameInstallation.h"
#include "games/xeen/XeenEventDecoder.h"
#include <cstdint>
#include <vector>
#include <string>
namespace mmodern {
// Metadata is independent of a playback device. Missing files remain entries so
// runtime can retain timing and text; validation treats every error as a failure.
struct XeenCdExtent {
 XeenCdTrack track;
 std::optional<std::uint64_t> start;
 std::uint64_t sectors=0, samples=0;
 std::string error;
};
struct XeenCdWindow {std::filesystem::path file;std::uint64_t first=0,count=0;};
struct XeenCdRequest {
 bool refused=false;
 std::uint64_t duration=0;
 std::optional<std::uint64_t> absoluteStart;
 std::vector<XeenCdWindow> windows;
 std::string error;
};
std::vector<XeenCdExtent> inspectXeenCdTracks(const std::vector<XeenCdTrack> &);
XeenCdRequest mapXeenCdCue(const std::vector<XeenCdExtent> &,unsigned disc,const XeenEventCdSpeech &);
// Decode just the bounded cue window, outside the callback. Stereo 44100 Hz.
std::vector<std::int16_t> decodeXeenCdCue(const XeenCdRequest &);
}
#endif
