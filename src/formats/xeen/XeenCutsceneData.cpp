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
 char name[32];const auto size=std::snprintf(name,sizeof(name),pattern.c_str(),n);
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
 return d;
}
std::string XeenCutsceneData::firstCaption(const std::vector<std::uint8_t> &b) {
 auto text=token(b,0,std::min<std::size_t>(800,b.size()));
 if(text.empty())throw std::runtime_error("Empty Clouds intro caption record");XeenDosText::validateControls(text);return text;
}
std::string XeenCutsceneData::hand(unsigned n) const {if(n>15)bad();return numbered(handPattern,n);}
std::string XeenCutsceneData::marble(unsigned n) const {if(n<1 || n>4)bad();return numbered(marblePattern,n);}
}
