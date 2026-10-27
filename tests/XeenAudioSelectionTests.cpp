#include "app/XeenAudio.h"
#include "formats/xeen/XeenAssetSource.h"
#include "formats/xeen/XeenCutsceneData.h"
#include "games/xeen/XeenInstallationDetector.h"
#include "XeenTestInstallation.h"
#include <chrono>
#include <fstream>
#include <iostream>
#include <stdexcept>

using namespace mmodern;
namespace mmodern {
struct XeenAudioTestAccess {
 static std::vector<std::uint8_t> resource(const std::filesystem::path &folder,const std::string &name,
  XeenSceneArchive archive,const XeenAudio::Loader &fallback) {
  return XeenAudio::readProgramResource(folder,name,archive,fallback);
 }
};
}
namespace {
using Bytes=std::vector<std::uint8_t>;
void check(bool value,const char *message) {if(!value)throw std::runtime_error(message);}
void write(const std::filesystem::path &path,const Bytes &bytes) {
 std::ofstream out(path,std::ios::binary|std::ios::trunc);
 out.write(reinterpret_cast<const char *>(bytes.data()),bytes.size());
 check(bool(out),"Fixture write failed");
}
struct Cleanup {std::filesystem::path path;~Cleanup(){std::error_code error;std::filesystem::remove_all(path,error);}};
}
int main(int argc,char **argv) {try {
 check(argc==3,"usage: mmodern_audio_selection_tests <game data> <test directory>");
 const auto root=std::filesystem::u8path(argv[1]);
 const auto original=xeenTestInstallationDetector().detect(root);check(original && original->uiModule,"Original installation missing");
 const auto folder=std::filesystem::u8path(argv[2])/std::to_string(std::chrono::steady_clock::now().time_since_epoch().count());
 std::filesystem::create_directories(folder);Cleanup cleanup{folder};
 const auto cfgPath=folder/"alternate.cfg";
 const auto verify=[&](unsigned music,unsigned digital) {
  Bytes bytes(29);const auto put=[&](unsigned offset,unsigned value){bytes[offset]=value;bytes[offset+1]=value>>8;};
  put(0,digital);put(2,0x220);put(4,music);put(6,0x220);put(8,7);put(0x14,1);put(0x18,3);write(cfgPath,bytes);
  const auto selected=XeenInstallationDetector(original->uiModule->physicalPath,cfgPath).detect(root);
  check(selected && selected->configuration && selected->configuration->physicalPath==cfgPath,"Selected CFG was not retained by detector");
  const auto cutscene=XeenCutsceneData::installed(*selected);
  check(cutscene.digitalDevice==digital && cutscene.musicDevice==music,"Cutscene read a different CFG");
  XeenAssetSource assets(*selected);auto audio=XeenAudio::installed(*selected,assets,0,false);
  check(audio->deviceId()==music && audio->digitalDeviceId()==digital,"Audio read a different CFG");
  check(audio->hasDmaDigital()==(digital==6 || digital==7 || digital==10),"Original-data digital selection");
  check(audio->startProgram(0,XeenAudioProgram::Dat),"Original-data DAT selection refused");
  check(audio->startProgram(0,XeenAudioProgram::Exe)==!(digital==3 || digital==4 || digital==9),"Original-data EXE exception");
  if(digital==6 || digital==7 || digital==10) {
   const auto expected=digital==6?3981740000ULL:3982012000ULL;
   check(audio->digitalDuration("crodo1.voc",XeenSceneArchive::CloudsProgram)==expected,"Original Crodo selected duration");
   audio->startDigital("crodo1.voc",XeenSceneArchive::CloudsProgram,0);
   check(audio->digitalBusy(expected-1) && !audio->digitalBusy(expected),"Original Crodo selected busy endpoint");
  }
  auto read=selected->configuration->open();Bytes after(read->size());check(read->read(after.data(),after.size())==after.size() && after==bytes,"Selected CFG was modified");
 };
 for(unsigned music=0;music<=10;++music)verify(music,10);
 for(unsigned digital=0;digital<=11;++digital)verify(0,digital);
 verify(65535,65535);

 unsigned calls=0;
 const XeenAudio::Loader archive=[&](const std::string &,XeenSceneArchive from) {
  ++calls;if(from!=XeenSceneArchive::IntroOnly)throw std::runtime_error("Unexpected cross-archive lookup");
  return Bytes{'a','r','c','h','i','v','e'};
 };
 write(folder/"PrOmUs",Bytes{'l','o','o','s','e'});
 check(XeenAudioTestAccess::resource(folder,"promus",XeenSceneArchive::IntroOnly,archive)==Bytes({'l','o','o','s','e'}) && calls==0,"Loose program driver did not override INTRO");
 std::filesystem::remove(folder/"PrOmUs");
 check(XeenAudioTestAccess::resource(folder,"promus",XeenSceneArchive::IntroOnly,archive)==Bytes({'a','r','c','h','i','v','e'}) && calls==1,"INTRO lookup was not selected");
 unsigned attempted=0;
 const XeenAudio::Loader absent=[&](const std::string &,XeenSceneArchive from)->Bytes {
  ++attempted;if(from==XeenSceneArchive::IntroOnly)throw std::runtime_error("Absent INTRO driver");
  return Bytes{'w','r','o','n','g'};
 };
 bool refused=false;try{(void)XeenAudioTestAccess::resource(folder,"promus",XeenSceneArchive::IntroOnly,absent);}catch(const std::runtime_error &){refused=true;}
 check(refused && attempted==1,"Missing INTRO driver fell back to another archive");
 std::cout<<"Original driver selection, CFG readers and loose override passed\n";return 0;
}catch(const std::exception &e){std::cerr<<e.what()<<'\n';return 1;}}
