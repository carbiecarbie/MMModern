// Artificial resource mutation/reversion controls; no commercial bytes.
#include "SyntheticXeenArchive.h"
#include "formats/xeen/XeenAssetSource.h"
#include <chrono>
#include <iostream>
#include <algorithm>
#include <random>
using namespace mmodern;
using namespace sprite_test;
namespace {
void check(bool v,const char *m){if(!v)throw std::runtime_error(m);}
template<class F> void rejects(F f){bool failed=false;try{f();}catch(const std::exception &){failed=true;}check(failed,"Changed resource did not fail");}
Bytes frames(unsigned count,unsigned color){return multiFrameSprite(std::vector<std::pair<Bytes,Bytes>>(count,{cell(0,1,0,1,{3,0,0,std::uint8_t(color)}),{}}));}
}
int main(){try{
 const auto root=std::filesystem::temp_directory_path()/("mmodern-consequence-sprites-"+std::to_string(std::chrono::steady_clock::now().time_since_epoch().count()));
 std::filesystem::create_directories(root);struct Cleanup{std::filesystem::path p;~Cleanup(){std::error_code e;std::filesystem::remove_all(p,e);}} cleanup{root};
 std::map<std::string,Bytes> original;for(unsigned id:{3u,6u,8u,9u,13u}){original[XeenAssetSource::normalMonsterResource(id)]=frames(8,7);original[XeenAssetSource::attackMonsterResource(id)]=frames(4,7);}
 original["pow11.icn"]=frames(3,7);original["pow12.icn"]=frames(3,7);
 GameInstallation installation;installation.xeenArchive=root/"xeen.cc";
 for(const auto &[name,data]:original){archive(installation.xeenArchive,original);XeenAssetSource assets(installation);
  const auto validate=[&]{if(name=="pow11.icn" || name=="pow12.icn")assets.validateProjectile(name=="pow12.icn");else if(name.substr(4)=="mon")assets.validateNormalMonster(std::stoi(name));else assets.validateAttackMonster(std::stoi(name));};
  validate();assets.discardSpriteCache();validate(); // Compatible reconstruction.
  assets.discardSpriteCache();auto changed=original;changed[name]=frames(data[0],8);archive(installation.xeenArchive,changed);
  rejects(validate);archive(installation.xeenArchive,original);assets.discardSpriteCache();rejects(validate);
 }
 // Explicit effect requests only: independent pinned DRAWER1 offset/mask
 // expectations, retaining bounded support without assigning an effect by species.
 original["000.mon"]=frames(8,0x9d);original["000.att"]=frames(4,0x9d);
 original["effect.pal"]=Bytes(768,1);original["effect.raw"]=Bytes(64000,0x48);
 // Two overlapping components, including opaque palette zero/255. The
 // engine-free adapter must preserve component order and transparency.
 original["overlap.icn"]=sprite(cell(0,1,0,1,{3,0,0,0}),cell(0,1,0,1,{3,0,0,255}));
 archive(installation.xeenArchive,original);
 XeenAssetSource effects(installation,320,200);
 constexpr unsigned offsets[]{0x41,0x20,0x40,0x21,0x48,0x46,0x43,0x40};
 constexpr unsigned masks[]{15,7,7,15,7,7,7,7};
 for(auto kind:{XeenMonsterSpriteKind::Normal,XeenMonsterSpriteKind::Attack}) for(int phase=-1;phase<8;++phase) {
  XeenSpriteDrawOptions options;options.sceneClipped=true;options.bottomClipped=true;options.monsterEffectFlags=phase<0?0:0x104+phase;
  effects.drawMonster(0,{kind,0},100,100,options);
  check(effects.snapshot().pixels[100*320+100]==(phase<0?0x9d:(0x9d&masks[phase])+offsets[phase]),
   "Explicit bounded effect-1 palette sequence differs from pinned drawer");
 }
 for(unsigned flags:{0x1000u,0xffffu,0x118u,0x204u,0x304u,0x404u,0x504u,0x610u,0x700u}) {XeenSpriteDrawOptions options;options.sceneClipped=true;options.monsterEffectFlags=flags;
  rejects([&]{effects.drawMonster(0,{0},100,100,options);});}
 XeenSpriteDrawOptions otherImage;otherImage.sceneClipped=true;otherImage.monsterEffectFlags=0x104;
 effects.drawMonster(8,{0},100,100,otherImage);
 check(effects.snapshot().pixels[100*320+100]==0x48,"Data-derived palette effect rejected on another image");
 effects.loadPalette("effect.pal");
 // Expected per-pixel operations from the reference helpers, exercising
 // deterministic cosmetic seeds, jitter, fuzz, and all ghost intensity levels.
 for(unsigned index=0;index<4;++index) for(unsigned seed=0;seed<16;++seed) {
  std::mt19937 random(seed);unsigned r1=std::uniform_int_distribution<unsigned>(0,65535)(random),r2=std::uniform_int_distribution<unsigned>(0,65535)(random);
  const bool carry=r1&0x8000;r1=((r1<<1)-r2-(carry?1:0))&65535;
  const bool next=r2&1;r2=(r2>>1)|(carry?0x8000:0);r2=((r2>>1)|(next?0x8000:0))^r1;
  constexpr unsigned thresholds[]{0x3333,0x6666,0x999a,0xcccd};
  XeenSpriteDrawOptions option;option.sceneClipped=true;option.bottomClipped=true;option.monsterEffectSeed=seed;
  option.monsterEffectFlags=0x500+index;effects.loadRawFramebuffer("effect.raw");effects.drawMonster(0,{0},100,100,option);
  check(effects.snapshot().pixels[32100]==(r2>thresholds[index]?0x9d:0x48),"Fuzz pixel differs from reference threshold");
  option.monsterEffectFlags=0x200+index;effects.loadRawFramebuffer("effect.raw");effects.drawMonster(0,{0},100,100,option);
  const int dx=(index%2)?-3:((r2&2)?3:-3),dy=(index%2)?-3:((r2&0x200)?3:-3);
  const auto jitter=effects.snapshot();check(jitter.pixels[(100+dy)*320+100+dx]==0x9d,"Jitter pixel differs from reference offsets");
  check(std::count(jitter.pixels.begin(),jitter.pixels.end(),0x9d)==1,"Jitter introduced extra pixels");
  constexpr unsigned mask[]{1,3,7,15},offset[]{1,2,4,8};
  const int level=(0x9d&mask[index])-offset[index]+8;
  option.monsterEffectFlags=0x300+index;effects.loadRawFramebuffer("effect.raw");effects.drawMonster(0,{0},100,100,option);
  check(effects.snapshot().pixels[32100]==0x40+std::clamp(level,0,15),"Ghost shade differs from reference");
 }
 effects.loadRawFramebuffer("effect.raw");XeenSpriteDrawOptions overlap;overlap.monsterEffectFlags=0x300;
 effects.drawSprite("overlap.icn",0,100,100,overlap);
 check(effects.snapshot().pixels[32100]==0x47,"Effect lost overlapping component or opaque zero/255");
 overlap.monsterEffectFlags=0x201;overlap.sceneClipped=true;overlap.bottomClipped=true;
 effects.loadRawFramebuffer("effect.raw");effects.drawMonster(0,{0},8,139,overlap);
 check(effects.snapshot().pixels==Bytes(64000,0x48),"Jitter escaped original scene/bottom clipping");
 std::cout<<"Synthetic MON/ATT/POW integrity and bounded effect palette controls passed\n";return 0;
}catch(const std::exception &e){std::cerr<<e.what()<<'\n';return 1;}}
