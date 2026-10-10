#ifndef MMODERN_DOS_TEXT_TEST_SUPPORT_H
#define MMODERN_DOS_TEXT_TEST_SUPPORT_H
#include "formats/xeen/XeenDosText.h"
#include <algorithm>
#include <set>
#include <stdexcept>
namespace dos_test {
using mmodern::XeenDosText;
inline void check(bool v,const char *message) {if(!v)throw std::runtime_error(message);}
// Synthetic neutral text and format arguments. No DOS strings are fixtures.
inline void u16(std::vector<std::uint8_t> &b,std::size_t at,unsigned v) {b.at(at)=v;b.at(at+1)=v>>8;}
inline std::vector<std::uint8_t> fixture() {
 std::vector<std::uint8_t> b(0x564e7);
 const auto fill=[&](std::size_t begin,std::size_t end,const char *signature) {
  std::string token;
  for(const char *p=signature;*p;++p) {if(p==signature || p[-1]==',')token+='%';if(*p!=',')token+=*p;else token+=' ';}
  check(token.size()<end-begin,"synthetic field capacity");token.resize(end-begin-1,'\x05');
  std::copy(token.begin(),token.end(),b.begin()+begin);b[end-1]=0;
 };
 for(const auto &f:XeenDosText::layout())if(!f.pointers) {
  std::size_t begin=f.begin;
  for(unsigned i=0;i<f.count;++i) {
   const auto end=f.begin+(f.end-f.begin)*(i+1)/f.count;
   fill(begin,end,f.arguments);begin=end;
  }
 }
 std::set<std::size_t> patches;
 for(unsigned i=0;i<XeenDosText::kOtherButtonKeyOffsets.size();++i) {
  const auto at=XeenDosText::kOtherButtonKeyOffsets[i];
  b[at-3]=0xc6;b[at-2]=0x46;b[at-1]=0xf0+i;b[at]='j'+i;
 }
 for(const auto &layout:XeenDosText::buttonLayouts()) {
  for(unsigned i=0;i<layout.count;++i) {
   u16(b,layout.x+i*2,10);b[layout.y+i]=10;b[layout.width+i]=10;b[layout.height+i]=10;
   b[layout.key+i]='a'+i;b[layout.painted+i]=1;
  }
  u16(b,layout.x+layout.count*2,65535);
 }
 for(const auto &f:XeenDosText::layout())if(f.pointers) {
  unsigned i=0;
  for(const auto bounds:XeenDosText::targets(f.name)) {
   fill(bounds.first,bounds.second,f.arguments);
   u16(b,f.begin+i*4,unsigned(bounds.first-0x4d890));u16(b,f.begin+i*4+2,0x481d);
   patches.insert(f.begin+i*4+2);++i;
  }
 }
 u16(b,0,0x5a4d);u16(b,2,b.size()%512);u16(b,4,(b.size()+511)/512);
 u16(b,6,patches.size());u16(b,8,0x56c);u16(b,24,0x20);u16(b,26,386);
 unsigned i=0;for(auto target:patches) {
  const auto relative=target-0x56c0;u16(b,0x20+i*4,relative%16);u16(b,0x22+i*4,relative/16);++i;
 }
 return b;
}
inline const XeenDosText &text() {static const XeenDosText value(fixture());return value;}
}
#endif
