#include "formats/xeen/XeenCutsceneData.h"
#include "formats/xeen/XeenDosText.h"
#include <algorithm>
#include <cstdio>
#include <stdexcept>
#include <zlib.h>
namespace mmodern {
namespace {
[[noreturn]] void bad() {throw std::runtime_error("Clouds intro unavailable: missing or different WORLD/XEEN.EXE (calibrated English CD build required)");}
unsigned word(const std::vector<std::uint8_t> &b,std::size_t p) {if(p+2>b.size())bad();return b[p]|unsigned(b[p+1])<<8;}
std::vector<std::uint8_t> read(const ReadOnlyDataFile &file) {
 auto in=file.open();if(in->size()>1024*1024)bad();std::vector<std::uint8_t> b(in->size());
 if(in->read(b.data(),b.size())!=b.size())bad();return b;
}
std::string token(const std::vector<std::uint8_t> &b,std::size_t p,std::size_t limit) {
 if(p>=limit || limit>b.size())bad();auto end=std::find(b.begin()+p,b.begin()+limit,0);
 if(end==b.begin()+limit)bad();return {b.begin()+p,end};
}
std::string numbered(const std::string &pattern,unsigned n) {
 char name[32];const bool signedArgument=pattern.find("%d")!=std::string::npos || pattern.find("%03d")!=std::string::npos;
 const auto size=signedArgument?std::snprintf(name,sizeof(name),pattern.c_str(),int(n)):std::snprintf(name,sizeof(name),pattern.c_str(),n);
 if(size<=0 || size>=int(sizeof(name)))bad();return name;
}
}
XeenCutsceneData XeenCutsceneData::installed(const GameInstallation &installation) {
 if(!installation.uiModule || !installation.configuration)bad();
 std::filesystem::path exe;
 const auto folder=installation.uiModule->physicalPath.parent_path();if(!std::filesystem::is_directory(folder))bad();
 for(const auto &entry:std::filesystem::directory_iterator(folder)) {
  auto name=entry.path().filename().u8string();for(auto &c:name)if(c>='A'&&c<='Z')c+=32;
  if(name=="xeen.exe") {if(!exe.empty() || !entry.is_regular_file())bad();exe=entry.path();}
 }
 if(exe.empty())bad();return parse(read(ReadOnlyDataFile::plain(exe)),read(*installation.configuration));
}
XeenCutsceneData XeenCutsceneData::parse(const std::vector<std::uint8_t> &b,const std::vector<std::uint8_t> &cfg) {
 // Part 0 identity plus MZ, pointer and bounded table checks. CRC refuses any
 // differing image, including changes outside the consumed tables.
 if(b.size()!=137737 || word(b,0)!=0x5a4d || word(b,8)*16!=0x2e30 ||
    crc32(0,b.data(),static_cast<uInt>(b.size()))!=0xaf60a284UL)bad();
 if(cfg.size()!=29 && cfg.size()!=30)throw std::runtime_error("Clouds intro unavailable: malformed WORLD/XEEN.CFG");
 XeenCutsceneData d;d.digitalDevice=word(cfg,0);
 constexpr std::size_t offsets[]{0x1f645,0x1f656,0x1f677,0x1f682,0x1f68e,0x1f697,
  0x1f6a1,0x1f6aa,0x1f6b7,0x1f6bf,0x1f6c7,0x1f6d1,0x1f6dc,0x1f6e5,0x1f6ee,
  0x1f6fb,0x1f708,0x1f712,0x1f71c,0x1f726,0x1f730,0x1f73a,0x1f745,0x1f74f,0x1f7b3,0x1f7bf};
 for(unsigned i=0;i<ResourceCount;++i)d.resources[i]=token(b,offsets[i],offsets[i]+32);
 for(unsigned i=0;i<14;++i) {
  if(word(b,0x1f542+4*i)!=0x1aa5)bad();
  const auto target=0x1d880+word(b,0x1f540+4*i);
  if(target<0x1f5bd || target>=0x1f645)bad();d.voices[i]=token(b,target,0x1f645);
  d.rows[i]=b.at(0x1f578+i);if(d.rows[i]>=8)bad();
 }
 for(unsigned i=0;i<248;++i) {
  d.mouths[i]=b.at(0x1f448+i);d.waits[i]=word(b,0x1f258+2*i);
  if(d.mouths[i]>4)bad();
 }
 for(unsigned i=0;i<16;++i) {d.towerLeft[i]=b.at(0x1f586+i);d.towerRight[i]=b.at(0x1f596+i);d.towerY[i]=b.at(0x1f5a6+i);}
 d.labels=token(b,0x1f759,0x1f7b3);XeenDosText::validateControls(d.labels);
 // Caption helper 0x11971 passes DS:36e0 to the original formatter.
 d.captionFormat=token(b,0x20f60,0x20fa0);XeenDosText::validateControls(d.captionFormat);
 if(d.captionFormat.find("%s")==std::string::npos || d.captionFormat.find('%',d.captionFormat.find('%')+1)!=std::string::npos)bad();
 d.handPattern=token(b,0x1f003,0x1f010);d.marblePattern=token(b,0x1f010,0x1f020);
 // Only the calibrated integer substitution is admitted to snprintf.
 if(d.handPattern.find("%02u")!=4 || d.handPattern.find('%',5)!=std::string::npos ||
    d.marblePattern.find("%02u")!=4 || d.marblePattern.find('%',5)!=std::string::npos)bad();
 auto &e=d.ending;
 constexpr std::size_t endingOffsets[]{0x1f8d1,0x1f8e9,0x1f8f3,0x1f93b,0x1f92e,0x1f924,0x1f943,0x1f94d,0x1f957,
  0x1f964,0x1f96e,0x1f97a,0x1f984,0x1f990,0x1f999,0x1f9a2,0x1f9ad,0x1f9b8,0x1f9c3,0x1f9cd,0x1f9da,0x1f9e7,
  0x1f9f1,0x1f9fd,0x1fa09,0x1fa11,0x1fa1c};
 static_assert(std::size(endingOffsets)==Ending::Count);
 for(unsigned i=0;i<Ending::Count;++i)e.resources[i]=token(b,endingOffsets[i],endingOffsets[i]+24);
 const auto pattern=[&](std::size_t p,const char *conversion) {
  auto value=token(b,p,p+24);const auto at=value.find('%');
  if(at==std::string::npos || value.compare(at,std::char_traits<char>::length(conversion),conversion)!=0 || value.find('%',at+1)!=std::string::npos)bad();
  return value;
 };
 const auto prec=pattern(0x1f8da,"%02u"),vort=pattern(0x1f8fc,"%02u"),cast=pattern(0x1f909,"%02u");
 const auto lord=pattern(0x1f916,"%d"),power=pattern(0x1fa51,"%d");
 const auto idle=pattern(0x1fa38,"%03d"),attack=pattern(0x1fa41,"%03d");
 const auto voc=pattern(0x1fa4a,"%s");
 for(unsigned i=0;i<41;++i)e.prec[i]=numbered(prec,i+1);
 for(unsigned i=0;i<20;++i)e.vortex[i]=numbered(vort,i+1);
 for(unsigned i=0;i<6;++i) {
  e.castleTop[i]=numbered(cast,i+1);e.castleBottom[i]=e.castleTop[i];
  if(e.castleBottom[i].size()<4)bad();e.castleBottom[i][3]='b'; // EXE 0x8b9a.
  e.castleFrames[i]=b.at(0x1f812+i);if(!e.castleFrames[i] || e.castleFrames[i]>32)bad();
  e.powers[i]=numbered(power,i);
 }
 for(unsigned i=0;i<3;++i) {
  e.darkLord[i]=numbered(lord,i+1);
  if(word(b,0x1f88c+4*i)!=0x1aa5)bad();
  const auto target=0x1d880+word(b,0x1f88a+4*i);
  if(target<0x1f896 || target>=0x1f8b4)bad();e.kingVoices[i]=token(b,target,0x1f8b4);
  e.randomWords[i]=word(b,0x13e32+2*i); // CS:16f2, private EXE source.
 }
 for(unsigned i=0;i<13;++i) {
  e.kingLeft[i]=std::int16_t(word(b,0x1f818+2*i));e.kingRight[i]=std::int16_t(word(b,0x1f832+2*i));e.kingY[i]=std::int16_t(word(b,0x1f84c+2*i));
 }
 for(unsigned i=0;i<9;++i){e.mirrorX[i]=std::int16_t(word(b,0x1f866+2*i));e.mirrorY[i]=std::int16_t(word(b,0x1f878+2*i));}
 const auto cue=[&](std::size_t table,unsigned index,std::size_t none) {
  const auto target=0x8970+word(b,table+2*index);
  if(target==none)return std::uint8_t(0);
  if(target<0x8afd || target>0x9025 || b.at(target)!=0x6a || (b.at(target+1)!=33 && b.at(target+1)!=34))bad();
  return b.at(target+1);
 };
 for(unsigned i=0;i<5;++i){const auto frame=word(b,0xa8ba+2*i);if(frame>=e.stormFm.size())bad();e.stormFm[frame]=cue(0xa8c4,i,0);}
 constexpr std::size_t cueTables[]{0xa89a,0xa87c,0xa85e,0xa840},noCue[]{0x8d2b,0x8e22,0x8f22,0x9025};
 for(unsigned t=0;t<4;++t)for(unsigned i=0;i<(t?15u:16u);++i)e.vortexFm[t][i]=cue(cueTables[t],i,noCue[t]);
 for(unsigned i=0;i<e.parade.size();++i) {
  auto &actor=e.parade[i];actor.selector=b.at(0x1f7c8+i);if(actor.selector>=90)bad();
  const auto p=0x1d94e + 40*actor.selector;std::copy_n(b.begin()+p,40,actor.metadata.begin());
  if(actor.metadata[1]>=90)bad();
  actor.idle=numbered(idle,actor.metadata[1]);actor.attack=numbered(attack,actor.metadata[1]);
  const auto target=0x2e30+16*word(b,p+6)+word(b,p+4);
  if(target<0x1d880 || target>=0x1f7c8)bad();
  const auto stem=token(b,target,std::min<std::size_t>(b.size(),target+32));
  actor.voice=voc;actor.voice.replace(actor.voice.find("%s"),2,stem);
 }
 return d;
}
XeenCutsceneData::Captions XeenCutsceneData::captions(const std::vector<std::uint8_t> &b) {
 if(b.empty() || b.size()>65535 || b.back()!=0)throw std::runtime_error("Malformed cutscene captions");
 Captions result;std::size_t start=0;
 for(std::size_t i=0;i<b.size();++i)if(b[i]==0) {
  result.offsets.push_back(std::uint16_t(start));result.records.emplace_back(b.begin()+start,b.begin()+i);
  XeenDosText::validateControls(result.records.back());start=i+1;
 }
 return result;
}
const std::string &XeenCutsceneData::Captions::record(unsigned index,unsigned bound) const {
 if(index>=records.size() || records[index].empty() || records[index].size()+1>bound)throw std::runtime_error("Invalid cutscene caption record");
 return records[index];
}
std::string XeenCutsceneData::firstCaption(const std::vector<std::uint8_t> &b) {
 auto text=token(b,0,std::min<std::size_t>(800,b.size()));
 if(text.empty())throw std::runtime_error("Empty Clouds intro caption record");XeenDosText::validateControls(text);return text;
}
std::string XeenCutsceneData::hand(unsigned n) const {if(n>15)bad();return numbered(handPattern,n);}
std::string XeenCutsceneData::marble(unsigned n) const {if(n<1 || n>4)bad();return numbered(marblePattern,n);}
}
