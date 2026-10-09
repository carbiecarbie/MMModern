#include "XeenTestInstallation.h"
#include "games/xeen/XeenInstallationDetector.h"
#include <array>
#include <algorithm>
#include <chrono>
#include <cstring>
#include <fstream>
#include <iostream>
#include <stdexcept>
#include <vector>
using namespace mmodern;
namespace {
void check(bool value,const char *message){if(!value)throw std::runtime_error(message);}
template<typename F>void refuses(F work,const char *message){try{work();}catch(const std::exception &){return;}throw std::runtime_error(message);}
using Bytes=std::vector<std::uint8_t>;
void write(const std::filesystem::path &path,const Bytes &bytes){std::ofstream output(path,std::ios::binary);output.write(reinterpret_cast<const char *>(bytes.data()),bytes.size());check(bool(output),"fixture write");}
void text(const std::filesystem::path &path,const std::string &bytes){write(path,Bytes(bytes.begin(),bytes.end()));}
void word(Bytes &bytes,std::size_t offset,unsigned value){bytes[offset]=value;bytes[offset+1]=value>>8;bytes[offset+2]=value>>8;bytes[offset+3]=value;}
void dword(Bytes &bytes,std::size_t offset,unsigned value){for(unsigned i=0;i<4;++i){bytes[offset+i]=value>>(i*8);bytes[offset+4+i]=value>>((3-i)*8);}}
std::size_t record(Bytes &bytes,std::size_t offset,const std::string &name,unsigned block,unsigned size,bool directory){
 const auto length=33+name.size()+(name.size()%2==0?1:0);bytes[offset]=length;dword(bytes,offset+2,block);dword(bytes,offset+10,size);
 bytes[offset+25]=directory?2:0;word(bytes,offset+28,1);bytes[offset+32]=name.size();std::copy(name.begin(),name.end(),bytes.begin()+offset+33);return length;
}
struct Fixture {
 Bytes logical=Bytes(40*2048), raw;std::array<Bytes,3> archives;
 explicit Fixture(unsigned base=3){
  for(unsigned i=0;i<3;++i){archives[i].resize(4097+i*7);for(std::size_t j=0;j<archives[i].size();++j)archives[i][j]=std::uint8_t(j*13+i);}
  const auto pvd=16*2048u;logical[pvd]=1;std::memcpy(logical.data()+pvd+1,"CD001",5);logical[pvd+6]=1;
  dword(logical,pvd+80,40);word(logical,pvd+120,1);word(logical,pvd+124,1);word(logical,pvd+128,2048);logical[pvd+881]=1;
  record(logical,pvd+156,std::string(1,'\0'),20,2048,true);
  logical[17*2048]=255;std::memcpy(logical.data()+17*2048+1,"CD001",5);logical[17*2048+6]=1;
  auto offset=20*2048u;offset+=record(logical,offset,std::string(1,'\0'),20,2048,true);offset+=record(logical,offset,std::string(1,'\1'),20,2048,true);record(logical,offset,"GAME",21,2048,true);
  offset=21*2048u;offset+=record(logical,offset,std::string(1,'\0'),21,2048,true);offset+=record(logical,offset,std::string(1,'\1'),20,2048,true);
  const char *names[]{"XEEN.CC;1","DARK.CC;1","INTRO.CC;1"};for(unsigned i=0;i<3;++i){offset+=record(logical,offset,names[i],24+i*4,archives[i].size(),false);std::copy(archives[i].begin(),archives[i].end(),logical.begin()+(24+i*4)*2048);}
  rebuild(base);
 }
 void rebuild(unsigned base=3){raw.assign((40+base)*2352,0);for(unsigned block=0;block<40;++block){const auto offset=(base+block)*2352;std::fill(raw.begin()+offset+1,raw.begin()+offset+11,255);raw[offset+15]=1;std::copy_n(logical.begin()+block*2048,2048,raw.begin()+offset+16);}}
};
Bytes bytes(const ReadOnlyDataFile &file){auto stream=file.open();Bytes result(stream->size());check(stream->read(result.data(),result.size())==result.size(),"logical source read");return result;}
}
int main(int argc,char **argv){try{
 check(argc>=2,"provide synthetic output directory");const auto root=std::filesystem::u8path(argv[1])/std::to_string(std::chrono::high_resolution_clock::now().time_since_epoch().count());std::filesystem::create_directories(root);
 Fixture fixture;write(root/"track.raw",fixture.raw);text(root/"disc.ins","FILE \"track.raw\" BINARY\nTRACK 01 MODE1/2352\nINDEX 01 00:00:03\n");
 const auto source=readCueCdArchives(root/"disc.ins");const char *names[]{"XEEN.CC","DARK.CC","INTRO.CC"};
 for(unsigned role=0;role<3;++role){check(bytes(source[role])==fixture.archives[role],"cross-sector complete payload");write(root/names[role],fixture.archives[role]);const auto plain=ReadOnlyDataFile::plain(root/names[role]);check(bytes(plain)==bytes(source[role]),"plain/raw payload equality");
  auto stream=source[role].open();check(stream->seek(2047),"boundary seek");std::array<std::uint8_t,3> cross{};check(stream->read(cross.data(),cross.size())==3,"boundary read");check(cross[0]==fixture.archives[role][2047]&&cross[2]==fixture.archives[role][2049],"boundary values");check(!stream->seek(stream->size()+1),"bounded seek");check(bytes(source[role])==fixture.archives[role],"repeated cold open");}
 text(root/"ui.dat","MZsynthetic detector module");const auto installation=XeenInstallationDetector(root/"ui.dat").detect(root);check(installation&&installation->cloudsData->transport==ReadOnlyDataFile::Transport::Mode1Raw,"cue selected ahead of root files");
 refuses([&]{XeenInstallationDetector{}.detect(root);},"missing DAT must refuse");
 // Malformed raw sync and truncation are read errors, never treated as absence.
 auto corrupt=fixture.raw;corrupt[(3+24)*2352+15]=2;write(root/"track.raw",corrupt);refuses([&]{bytes(source[0]);},"bad MODE1 must refuse");write(root/"track.raw",fixture.raw);
 corrupt.resize((3+24)*2352+18);write(root/"track.raw",corrupt);refuses([&]{bytes(source[0]);},"short raw stream must refuse");write(root/"track.raw",fixture.raw);
 auto invalid=fixture;invalid.logical[16*2048+84]^=1;invalid.rebuild();write(root/"track.raw",invalid.raw);refuses([&]{readCueCdArchives(root/"disc.ins");},"both-endian volume mismatch must refuse");
 invalid=fixture;invalid.logical[21*2048+68+25]=0x80;invalid.rebuild();write(root/"track.raw",invalid.raw);refuses([&]{readCueCdArchives(root/"disc.ins");},"multi-extent must refuse");
 invalid=fixture;dword(invalid.logical,21*2048+68+2,0xfffffff0);invalid.rebuild();write(root/"track.raw",invalid.raw);refuses([&]{readCueCdArchives(root/"disc.ins");},"out of volume extent must refuse");
 invalid=fixture;record(invalid.logical,21*2048+196,"xeen.cc;1",24,fixture.archives[0].size(),false);invalid.rebuild();write(root/"track.raw",invalid.raw);refuses([&]{readCueCdArchives(root/"disc.ins");},"duplicate case-insensitive ISO names must refuse");
 invalid=fixture;invalid.logical[21*2048+2047]=1;invalid.rebuild();write(root/"track.raw",invalid.raw);refuses([&]{readCueCdArchives(root/"disc.ins");},"nonzero directory padding must refuse");write(root/"track.raw",fixture.raw);
 text(root/"bad.ins","FILE \"track.raw\" BINARY\nTRACK 01 MODE2/2352\nINDEX 01 00:00:03\n");refuses([&]{readCueCdArchives(root/"bad.ins");},"unsupported track must refuse");
 text(root/"bad.ins","FILE \"track.raw\" BINARY\nTRACK 01 MODE1/2352\nINDEX 01 00:60:03\n");refuses([&]{readCueCdArchives(root/"bad.ins");},"invalid time must refuse");
 // Same-source candidates are compared, conflict is never silently selected.
 text(root/"bad.ins","FILE \"track.raw\" BINARY\nTRACK 01 MODE1/2352\nINDEX 01 00:00:03\n");check(XeenInstallationDetector(root/"ui.dat").detect(root).has_value(),"identical discs selectable");
 auto other=fixture;other.logical[24*2048+7]^=1;other.rebuild();write(root/"other.raw",other.raw);text(root/"bad.ins","FILE \"other.raw\" BINARY\nTRACK 01 MODE1/2352\nINDEX 01 00:00:03\n");refuses([&]{XeenInstallationDetector(root/"ui.dat").detect(root);},"conflicting discs must refuse");
 const auto plainRoot=root/"plain";std::filesystem::create_directories(plainRoot/"GAME");for(unsigned i=0;i<3;++i)write(plainRoot/"GAME"/names[i],fixture.archives[i]);
 const auto plainInstallation=XeenInstallationDetector(root/"ui.dat").detect(plainRoot);check(plainInstallation&&plainInstallation->cloudsData->transport==ReadOnlyDataFile::Transport::Plain,"plain GAME detection");
 check(XeenInstallationDetector(root/"ui.dat").detect(plainRoot/"GAME").has_value(),"direct GAME detection");
 const auto loose=root/"loose";std::filesystem::create_directories(loose);
 for(unsigned i=0;i<3;++i)write(loose/names[i],fixture.archives[i]);
 refuses([&]{XeenInstallationDetector(root/"ui.dat").detect(loose);},"loose root archives cannot become floppy fallback");
 text(root/"bad.ins","FILE \"track.raw\" BINARY\nTRACK 01 MODE1/2352\nFILE \"track.raw\" BINARY\nTRACK 02 MODE1/2352\nINDEX 01 00:00:03\n");
 refuses([&]{readCueCdArchives(root/"bad.ins");},"earlier data track missing INDEX must refuse");
 const auto retained=ReadOnlyDataFile::plain(plainRoot/"GAME"/names[0]);auto appended=fixture.archives[0];appended.push_back(0);write(plainRoot/"GAME"/names[0],appended);refuses([&]{bytes(retained);},"plain source size change must refuse");
 const auto missingRoot=root/"missing";std::filesystem::create_directories(missingRoot);write(missingRoot/"XEEN.CC",fixture.archives[0]);refuses([&]{XeenInstallationDetector(root/"ui.dat").detect(missingRoot);},"incomplete plain source must refuse");
 if(argc>=3) {
  const auto original=std::filesystem::u8path(argv[2]);const auto detected=xeenTestInstallationDetector().detect(original);
  check(detected&&detected->uiModule&&detected->introData,"original complete source and UI");
  const std::array<ReadOnlyDataFile,3> selected{*detected->cloudsData,*detected->darksideData,*detected->introData};
  for(const auto &file:selected)check(!bytes(file).empty(),"configured logical archive is empty");
  for(const auto &cue:xeenTestCues(original)) {
   const auto disc=readCueCdArchives(cue);
   for(unsigned role=0;role<3;++role)check(bytes(selected[role])==bytes(disc[role]),"available disc differs from selected logical archive");
  }
  std::cout<<detected->sourceOrigin<<'\n';
 }
 std::cout<<"Read-only CD source checks passed\n";return 0;
}catch(const std::exception &error){std::cerr<<error.what()<<'\n';return 1;}}
