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
 std::cout<<"Twelve synthetic MON/ATT/POW compatible-reload and permanent mismatch/reversion controls passed\n";return 0;
}catch(const std::exception &e){std::cerr<<e.what()<<'\n';return 1;}}
