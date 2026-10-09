#include "XeenTestInstallation.h"
#include "compat/scummvm/ScummVmXeenBridge.h"
#include "games/xeen/XeenInstallationDetector.h"
#include "SyntheticXeenArchive.h"
#include "platform/XeenSaveFile.h"
#include "formats/xeen/XeenSaveFormat.h"
#include <zlib.h>
#include <chrono>
#include <fstream>
#include <iostream>
#include <stdexcept>
using namespace mmodern;
using namespace sprite_test;
namespace {
void check(bool value,const char *message){if(!value)throw std::runtime_error(message);}
template<typename F>void refuses(F work,const char *message){try{work();}catch(const std::exception &){return;}throw std::runtime_error(message);}
Bytes bytes(const std::filesystem::path &file){std::ifstream stream(file,std::ios::binary);return Bytes(std::istreambuf_iterator<char>(stream),{});}
void write(const std::filesystem::path &file,const Bytes &bytes){std::ofstream stream(file,std::ios::binary);stream.write(reinterpret_cast<const char *>(bytes.data()),bytes.size());}
}
int main(int argc,char **argv){try{
 check(argc>=2,"provide synthetic evidence directory");const auto root=std::filesystem::u8path(argv[1])/std::to_string(std::chrono::steady_clock::now().time_since_epoch().count());std::filesystem::create_directories(root);
 GameInstallation installation;installation.root=root;installation.xeenArchive=root/"XEEN.CC";installation.darkArchive=root/"DARK.CC";
 const auto image=[](unsigned color){return sprite(cell(0,1,0,1,{3,0,0,std::uint8_t(color)}));};
 archive(installation.xeenArchive,{{"shared.srf",image(3)},{"binary",Bytes{1,2,3}},{"no-terminator",{1}}});
 archive(installation.darkArchive,{{"shared.srf",image(4)},{"spells.xen",Bytes(77*64,0)}});
 archive(root/"INTRO.CC",{{"fallback.srf",image(5)}});installation.introData=ReadOnlyDataFile::plain(root/"INTRO.CC");
 {
  ScummVmXeenBridge bridge(installation,320,200);check(bridge.readArchiveResource("binary")==Bytes({1,2,3}),"outer XOR decoded once");
  check(!bridge.hasArchiveResource("absent"),"missing member distinction");refuses([&]{bridge.readArchiveResource("absent");},"missing read must refuse");
  bridge.drawSceneSprite("shared.srf",0,10,10,{});check(bridge.snapshot().pixels[3210]==3,"current archive selection");
  XeenSpriteDrawOptions options;options.archive=XeenSceneArchive::Darkside;bridge.drawSceneSprite("shared.srf",0,11,10,options);check(bridge.snapshot().pixels[3211]==4,"selected DARK isolation");
  bridge.drawSceneSprite("fallback.srf",0,12,10,options);check(bridge.snapshot().pixels[3212]==5,"INTRO fallback");
  check(bridge.readLearnedSpellNamesFromDarkArchive()->size()==77*64,"spell format ceiling replaces exact floppy size");
  bridge.discardSpriteCache();bridge.drawSceneSprite("SHARED.SRF",0,10,10,{});check(bridge.cachedSpriteCount()==1,"pinned resource ID key");
  auto changed=bytes(installation.xeenArchive);changed.back()^=1;write(installation.xeenArchive,changed);
  bridge.discardSpriteCache();refuses([&]{bridge.drawSceneSprite("shared.srf",0,10,10,{});},"cold admitted bytes must refuse changes");
 }
 // Index short reads/reserved fields/extents are caught before upstream fatal paths.
 archive(installation.xeenArchive,{{"binary",Bytes{1,2,3}}});const auto valid=bytes(installation.xeenArchive);
 for(unsigned mode=0;mode<4;++mode){auto malformed=valid;
  if(mode==0)malformed.resize(1);if(mode==1)malformed.resize(5);if(mode==2)malformed[9]^=1;
  if(mode==3){malformed[4]=0;malformed[5]=0;malformed[6]=0;}
  write(installation.xeenArchive,malformed);refuses([&]{ScummVmXeenBridge bridge(installation);bridge.readArchiveResource("binary");},"malformed CC index/member must refuse");}
 write(installation.xeenArchive,valid);
 {ScummVmXeenBridge bridge(installation);auto changed=valid;changed[2]^=1;write(installation.xeenArchive,changed);refuses([&]{bridge.readArchiveResource("binary");},"retained index mismatch must refuse");}
 write(installation.xeenArchive,valid);
 {ScummVmXeenBridge bridge(installation);auto changed=valid;changed.pop_back();write(installation.xeenArchive,changed);refuses([&]{bridge.readArchiveResource("binary");},"reopened source truncation must refuse");}
 if(argc>=3) {
  const auto original=std::filesystem::u8path(argv[2]);const auto detected=xeenTestInstallationDetector().detect(original);check(bool(detected),"original source detection");
  std::vector<GameInstallation> sources{*detected};
  for(const auto &cue:xeenTestCues(original)) {
   const auto roles=readCueCdArchives(cue);auto selected=*detected;
   selected.cloudsData=roles[0];selected.darksideData=roles[1];selected.introData=roles[2];sources.push_back(std::move(selected));
  }
  for(const auto &selected:sources) {
   ScummVmXeenBridge bridge(selected,320,200);
   check(bridge.readInitialResource("maze0023.dat").size()==892,"original map archive stream");check(bridge.readInitialResource("maze.chr").size()==30*354,"initial chunks/inner plaintext");
   check(bridge.readLearnedSpellNamesFromDarkArchive()->size()==937,"original checked metadata stream");check(bridge.spriteFrameCount("esc.icn")==2,"original sprite structure");bridge.discardSpriteCache();check(bridge.spriteFrameCount("esc.icn")==2,"original cold image reopen");
   const auto signature=XeenSaveFile::fingerprint(selected);
   check(signature.clouds.size==13438429 && signature.clouds.crc32==0x2d073b21 && signature.darkside && signature.darkside->size==11288403 && signature.darkside->crc32==0x6421f2f8,"original logical encrypted CC fingerprint oracle");
  }
 }
 std::cout<<"Checked CC source/archive checks passed\n";return 0;
}catch(const std::exception &error){std::cerr<<error.what()<<'\n';return 1;}}
