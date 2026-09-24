// Artificial resource mutation/reversion controls; no commercial bytes.
#include "SyntheticXeenArchive.h"
#include "formats/xeen/XeenAssetSource.h"
#include <chrono>
#include <iostream>
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
 archive(installation.xeenArchive,original);
 XeenAssetSource effects(installation,320,200);
 constexpr unsigned offsets[]{0x41,0x20,0x40,0x21,0x48,0x46,0x43,0x40};
 constexpr unsigned masks[]{15,7,7,15,7,7,7,7};
 for(auto kind:{XeenMonsterSpriteKind::Normal,XeenMonsterSpriteKind::Attack}) for(int phase=-1;phase<8;++phase) {
  XeenSpriteDrawOptions options;options.sceneClipped=true;options.bottomClipped=true;options.slimePalettePhase=phase;
  effects.drawMonster(0,{kind,0},100,100,options);
  check(effects.snapshot().pixels[100*320+100]==(phase<0?0x9d:(0x9d&masks[phase])+offsets[phase]),
   "Explicit bounded effect-1 palette sequence differs from pinned drawer");
 }
 for(int phase:{-2,8}) {XeenSpriteDrawOptions options;options.sceneClipped=true;options.slimePalettePhase=phase;
  rejects([&]{effects.drawMonster(0,{0},100,100,options);});}
 XeenSpriteDrawOptions wrongImage;wrongImage.sceneClipped=true;wrongImage.slimePalettePhase=0;
 rejects([&]{effects.drawMonster(8,{0},100,100,wrongImage);});
 std::cout<<"Synthetic MON/ATT/POW integrity and bounded effect palette controls passed\n";return 0;
}catch(const std::exception &e){std::cerr<<e.what()<<'\n';return 1;}}
