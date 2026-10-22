#include "formats/xeen/XeenCdAudio.h"
#include "games/xeen/XeenCdSpeechTiming.h"
#include <vorbis/vorbisfile.h>
#include <algorithm>
#include <cstdio>
#include <stdexcept>
namespace mmodern {
namespace {
struct Vorbis {
 OggVorbis_File file{};
 explicit Vorbis(const std::filesystem::path &path) {
#ifdef _WIN32
  auto *input=_wfopen(path.c_str(),L"rb");
#else
  auto *input=std::fopen(path.c_str(),"rb");
#endif
  if(!input)throw std::runtime_error("Missing CD audio: "+path.u8string());
  if(ov_open_callbacks(input,&file,nullptr,0,OV_CALLBACKS_DEFAULT)<0) {
   std::fclose(input);throw std::runtime_error("Undecodable CD audio: "+path.u8string());
  }
  for(long link=0;link<ov_streams(&file);++link) {
   const auto *info=ov_info(&file,link);
   if(!info || info->rate!=44100 || info->channels!=2) {
    ov_clear(&file);throw std::runtime_error("CD audio must be stereo 44100 Hz: "+path.u8string());
   }
  }
 }
 ~Vorbis(){ov_clear(&file);}
};
std::uint64_t frames(unsigned value) {return ((value/10000)*60+(value%10000)/100)*75+value%100;}
}
std::vector<XeenCdExtent> inspectXeenCdTracks(const std::vector<XeenCdTrack> &tracks) {
 std::vector<XeenCdExtent> result;
 unsigned disc=0;std::optional<std::uint64_t> start;
 for(const auto &track:tracks) {
  if(track.disc!=disc){disc=track.disc;start=0;}
  XeenCdExtent extent;extent.track=track;extent.start=start;
  try {
   if(!track.error.empty())throw std::runtime_error(track.error);
   if(track.audio) {
    Vorbis input(track.file);const auto count=ov_pcm_total(&input.file,-1);
    if(count<=0)throw std::runtime_error("Empty CD audio: "+track.file.u8string());
    extent.samples=count;extent.sectors=(count+587)/588;
   } else {
    const auto size=std::filesystem::file_size(track.file);
    if(size%2352)throw std::runtime_error("Invalid CD data track extent");
    extent.sectors=size/2352;
   }
   if(track.index>=extent.sectors)throw std::runtime_error("CD index outside file");
   if(extent.start)*extent.start+=track.index;
   if(start)*start=*extent.start+extent.sectors;
  }catch(const std::exception &e){extent.error=e.what();start.reset();}
  result.push_back(std::move(extent));
 }
 return result;
}
XeenCdRequest mapXeenCdCue(const std::vector<XeenCdExtent> &tracks,unsigned disc,const XeenEventCdSpeech &cue) {
 XeenCdRequest result;result.duration=xeenCdSpeechDurationNanoseconds(cue);
 if(!result.duration)return result;
 const bool badSheet=std::any_of(tracks.begin(),tracks.end(),[&](const auto &e){return e.track.disc==disc && !e.track.error.empty();});
 // An unparsed sheet supplies no addresses, just like a disc without tracks.
 const auto requested=badSheet?tracks.end():std::find_if(tracks.begin(),tracks.end(),[&](const auto &e){return e.track.disc==disc && e.track.number==cue.track;});
 const auto begin=std::int64_t(frames(cue.start))-25;
 // A plain GAME copy still has the known refused track-2/data boundary.
 if(requested==tracks.end()) {
  if(cue.track==2 && begin<0){result.refused=true;result.duration=0;}
  else result.error="CD speech unavailable: no installed audio track";
  return result;
 }
 if(!requested->start){result.error="CD absolute address unavailable after missing track";return result;}
 const auto absolute=std::int64_t(*requested->start)+begin;
 if(absolute<0){result.refused=true;result.duration=0;return result;}
 result.absoluteStart=std::uint64_t(absolute);
 auto current=tracks.end();
 std::uint64_t lower=0;
 for(auto it=tracks.begin();it!=tracks.end();++it) {
  if(it->track.disc!=disc)continue;
  if(!it->start || !it->error.empty()) {
   result.error=it->error.empty()?"CD absolute extent unavailable":it->error;return result;
  }
  const auto upper=*it->start+it->sectors;
  if(lower<=*result.absoluteStart && *result.absoluteStart<upper){current=it;break;}
  lower=upper;
 }
 // Staging selects by absolute sector, including starts beyond the named
 // track, and refuses data/lead-out before starting the logical interval.
 if(current==tracks.end() || !current->track.audio){result.refused=true;result.duration=0;return result;}
 auto remaining=(frames(cue.finish)+25-frames(cue.start))*588;
 std::uint64_t first=0;
 if(*result.absoluteStart<*current->start) {
  const auto skipped=(*current->start-*result.absoluteStart)*588;
  if(skipped>=remaining)return result;
  remaining-=skipped; // Staging skips an explicit INDEX pregap.
 } else first=(*result.absoluteStart-*current->start)*588;
 while(remaining) {
  // A started request can reach lead-out after playing a valid prefix.
  if(current==tracks.end() || current->track.disc!=disc || !current->track.audio)break;
  if(!current->error.empty()){result.error=current->error;break;}
  const auto rounded=current->sectors*588;
  if(first>=rounded){result.error="CD request begins outside audio file";break;}
  const auto count=std::min(remaining,rounded-first);
  result.windows.push_back({current->track.file,first,count});
  remaining-=count;
  if(!remaining)break;
  // Installed Staging advances at EOF only with at least two seconds left.
  if(remaining<2*44100)break; // The already-played prefix is retained; no next-file start.
  ++current;first=0;
 }
 return result;
}
std::vector<std::int16_t> decodeXeenCdCue(const XeenCdRequest &request) {
 if(!request.error.empty())throw std::runtime_error(request.error);
 std::uint64_t total=0;for(const auto &w:request.windows)total+=w.count;
 // MMSSFF operands are 16-bit: bounded to the representable cue interval.
 if(total>7*60*44100ULL)throw std::runtime_error("CD cue exceeds operand duration bound");
 std::vector<std::int16_t> pcm(total*2,0);std::size_t offset=0;
 for(const auto &w:request.windows) {
  Vorbis input(w.file);const auto available=ov_pcm_total(&input.file,-1);
  const auto rounded=(std::uint64_t(available)+587)/588*588;
  if(w.first>=rounded || w.count>rounded-w.first)throw std::runtime_error("CD cue window exceeds rounded EOF");
  if(w.first>=std::uint64_t(available)) {
   offset+=w.count*2;continue; // The final rounded partial sector is silence.
  }
  if(ov_pcm_seek(&input.file,w.first)!=0)throw std::runtime_error("CD cue seek failed");
  const auto count=std::min(w.count,std::uint64_t(available)-w.first);
  std::size_t bytes=0;const auto size=count*4;
  while(bytes<size) {
   int link=0;const auto n=ov_read(&input.file,reinterpret_cast<char *>(pcm.data()+offset)+bytes,int(std::min<std::uint64_t>(16384,size-bytes)),0,2,1,&link);
   if(n<=0)throw std::runtime_error("Truncated or corrupt CD cue: "+w.file.u8string());
   bytes+=n;
  }
  // Separate-file lengths round upward to a sector; only that final fraction
  // is silence. EOF inside the advertised PCM count is always an error.
  offset+=w.count*2;
 }
 return pcm;
}
}
