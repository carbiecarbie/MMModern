#include "formats/xeen/XeenAssetSource.h"
#include "formats/xeen/XeenFontFormat.h"
#include "games/xeen/XeenInstallationDetector.h"
#include <algorithm>
#include <iostream>
#include <stdexcept>
using namespace mmodern;
int main(int argc,char **argv) {try {
 if(argc!=2)throw std::runtime_error("Title resource test requires the CD installation");
 const auto source=XeenInstallationDetector().detect(argv[1]);
 if(!source)throw std::runtime_error("Missing CD installation");
 XeenAssetSource assets(*source,320,200);
 constexpr auto dark=XeenSceneArchive::DarksideOnly;
 assets.loadPalette("dark.pal",dark);
 for(const auto *name:{"world.raw","marb.raw"}) {
  assets.loadRawFramebuffer(name,dark);
  if(!assets.snapshot().isValid())throw std::runtime_error("Invalid title/credits background");
  std::cout<<name<<" DARK.CC 320x200\n";
 }
 for(const auto *name:{"world0.int","world1.int","world2.int","start.icn","special.icn","choice.icn","scroll.icn","confirm.icn"}) {
  const auto count=assets.spriteFrameCount(name,dark);
  if(!count)throw std::runtime_error("Empty title sprite");
  for(unsigned frame=0;frame<count;++frame) {
   XeenSpriteDrawOptions options;options.archive=dark;
   assets.drawSceneSprite(name,frame,0,0,options);
  }
  std::cout<<name<<" DARK.CC frames "<<count<<'\n';
  if(std::string(name)=="scroll.icn") {
   const auto data=assets.readArchiveResource(name,dark);
   const auto word=[&](unsigned at){return unsigned(data.at(at))|unsigned(data.at(at+1))<<8;};
   for(unsigned i=0;i<count;++i){const auto at=word(2+4*i);std::cout<<"Cell "<<i<<" x "<<word(at)<<" width "<<word(at+2)<<" y "<<word(at+4)<<" height "<<word(at+6)<<'\n';}
  }
 }
 auto credits=assets.readArchiveResource(std::string(assets.uiText().scalar("CREDITS_RESOURCE")),dark);
 // The existing CC reader already performs the outer XOR 0x35 exactly once.
 std::cout<<"Credits size "<<credits.size()<<" NUL "<<std::count(credits.begin(),credits.end(),0)<<'\n';
 for(unsigned i=0;i<credits.size();++i)if(credits[i]==0)std::cout<<"Page terminator "<<i<<'\n';
 std::size_t begin=0;
 for(unsigned i=0;i<credits.size();++i)if(!credits[i]) {
  XeenDosText::validateControls(std::string_view(reinterpret_cast<const char *>(credits.data()+begin),i-begin));begin=i+1;
 }
 if(begin!=credits.size())throw std::runtime_error("Unterminated credits");
 const auto titleFont=assets.readArchiveResource("fnt",dark);
 std::cout<<"DARK title font equals Clouds font: "<<(titleFont==assets.readArchiveResource("fnt"))<<'\n';
 const auto cursor=assets.readArchiveResource("mouse.icn",dark);
 std::cout<<"DARK title cursor equals Clouds cursor: "<<(cursor==assets.readArchiveResource("mouse.icn"))<<'\n';
 const XeenFontFormat font(titleFont);
 for(unsigned c=0x20;c<=0x7e;++c) {
  const auto glyph=font.glyph(c,XeenFontSize::Normal);
  if(!glyph.advance || (c!=0x20 && std::none_of(glyph.pixels.begin(),glyph.pixels.end(),[](auto p){return p!=0;})))
   throw std::runtime_error("Original font cannot display printable code "+std::to_string(c));
 }
 return 0;
}catch(const std::exception &e){std::cerr<<e.what()<<'\n';return 1;}}
