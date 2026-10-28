#include "games/xeen/PromusDriver.h"
#include "compat/scummvm/XeenOpl3.h"
#include "app/XeenFmSource.h"
#include "formats/xeen/XeenAssetSource.h"
#include "games/xeen/XeenInstallationDetector.h"
#include <algorithm>
#include <array>
#include <cassert>
#include <cmath>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <stdexcept>
#include <vector>
using namespace mmodern;
using Bytes=std::vector<std::uint8_t>;
struct Event {std::uint64_t time;unsigned reg,value;};
static PromusDriver::Bytes data(Bytes b){return std::make_shared<const Bytes>(std::move(b));}
static int voice(unsigned r) {
 if((r>=0xa0&&r<=0xa8)||(r>=0xb0&&r<=0xb8)||(r>=0xc0&&r<=0xc8))return r&15;
 for(unsigned v=0;v<9;++v)for(unsigned base:{0x20,0x40,0x60,0x80,0xe0})
  if(r==base+v/3*8+v%3 || r==base+v/3*8+v%3+3)return v;
 return -1;
}
static std::vector<Event> changes(const std::vector<Event> &input) {
 std::array<unsigned,256> cache{};std::vector<Event> result;
 for(const auto &e:input)if(e.reg<256 && voice(e.reg)>=0 && voice(e.reg)<7 && cache[e.reg]!=e.value) {
  cache[e.reg]=e.value;result.push_back(e);
 }
 return result;
}
static std::vector<Event> dro(const std::filesystem::path &path) {
 std::ifstream in(path,std::ios::binary);Bytes b((std::istreambuf_iterator<char>(in)),{});
 if(b.size()<26 || std::string(b.begin(),b.begin()+8)!="DBRAWOPL" || b[8]!=2 || b[21] || b[22])throw std::runtime_error("Invalid M94 DRO");
 const auto n=b[25];if(b.size()<26+n || (b.size()-26-n)%2)throw std::runtime_error("Truncated M94 DRO");
 std::vector<Event> out;std::uint64_t t=0;
 for(std::size_t p=26+n;p<b.size();p+=2){const auto c=b[p],v=b[p+1];
  if(c==b[23])t+=v+1;else if(c==b[24])t+=(v+1)*256;
  else {if((c&127)>=n)throw std::runtime_error("Bad DRO map");out.push_back({t,unsigned(c>>7)*256+b[26+(c&127)],v});}}
 return changes(out);
}
static void window(const std::vector<Event> &generated,const std::vector<Event> &captured,
 unsigned from,unsigned to,std::uint64_t earliest,std::uint64_t latest,const char *label) {
 const auto filtered=changes(generated);
 std::vector<Event> selected;for(const auto &e:filtered)if(e.time>=from && e.time<=to)selected.push_back(e);
 if(selected.size()<10)throw std::runtime_error(std::string(label)+": insufficient events");
 // Compare the chip state just before the first selected write. DRO retains
 // a cache snapshot followed by changed writes, so replaying it from the
 // beginning preserves the state established outside this window.
 std::array<unsigned,256> expected{},actual{};std::array<bool,256> known{};
 for(const auto &e:generated)if(e.time<selected.front().time && e.reg<256 && voice(e.reg)>=0 && voice(e.reg)<7) {
  expected[e.reg]=e.value;known[e.reg]=true;
 }
 bool stateMismatch=false;
 for(std::size_t j=0;j+selected.size()<=captured.size();++j) {
  if(j)actual[captured[j-1].reg]=captured[j-1].value;
  if(captured[j].time<earliest || captured[j].time>latest)continue;
  double low=1e30,high=-1e30;std::size_t i=0;
  for(;i<selected.size();++i){const auto &a=selected[i],&b=captured[j+i];
   if(a.reg!=b.reg || a.value!=b.value)break;
   const double d=double(b.time)-double(a.time)*1000.0*16390/1193182;
   low=std::min(low,d);high=std::max(high,d);
  }
  if(i==selected.size() && high-low<=2.0){
   bool same=true;for(unsigned reg=0;reg<256;++reg)if(known[reg] && expected[reg]!=actual[reg]){same=false;break;}
   if(!same){stateMismatch=true;continue;}
   std::cout<<label<<": "<<i<<" writes and entering register state, DRO "<<captured[j].time<<".."<<captured[j+i-1].time<<" ms, alignment span "<<high-low<<" ms\n";return;
  }
 }
 throw std::runtime_error(std::string(label)+(stateMismatch?": entering register state did not match":": ordered register/tick window did not match"));
}
static void stopOrder(const std::vector<Event> &captured,std::uint64_t before,const char *label) {
 std::array<unsigned,256> cache{};std::size_t i=0;
 while(i<captured.size()) {
  const auto &e=captured[i];
  if(e.time>=before-100 && e.time<=before-40 && e.value==0 &&
     ((e.reg>=0xa0 && e.reg<=0xa6)||(e.reg>=0xb0 && e.reg<=0xb6)))break;
  cache[e.reg]=e.value;++i;
 }
 std::vector<unsigned> expected;
 for(int v=6;v>=0;--v)for(unsigned base:{0xa0,0xb0})if(cache[base+v])expected.push_back(base+v);
 if(expected.size()<8 || i+expected.size()>captured.size())throw std::runtime_error(std::string(label)+": missing fade key-off");
 for(auto reg:expected){if(captured[i].reg!=reg || captured[i].value!=0)throw std::runtime_error(std::string(label)+": fade key-off order differs");++i;}
 std::cout<<label<<": "<<expected.size()<<" projected key-off writes in order\n";
}
static std::vector<std::int16_t> render(unsigned waveform,bool retrigger) {
 XeenOpl3 chip;
 for(const auto &w:std::vector<std::pair<unsigned,unsigned>>{{1,0x20},{0x20,1},{0x23,1},{0x40,63},{0x43,0},
  {0x60,0xf0},{0x63,0xf0},{0x80,7},{0x83,7},{0xe0,waveform},{0xe3,waveform},{0xc0,1},{0xa0,0x98},{0xb0,0x31}})chip.write(w.first,w.second);
 std::array<std::int16_t,2> s{};for(unsigned i=0;i<3000;++i)chip.sample(s.data());
 if(retrigger){chip.write(0xb0,0x11);chip.write(0xb0,0x31);}else chip.write(0xb0,0x31);
 std::vector<std::int16_t> pcm(2000);for(unsigned i=0;i<1000;++i)chip.sample(pcm.data()+2*i);
 return pcm;
}
int main(int argc,char **argv){try {
 if(argc!=3)throw std::runtime_error("Expected game and M94 capture directories");
 auto install=XeenInstallationDetector().detect(std::filesystem::u8path(argv[1]));if(!install)throw std::runtime_error("Game not found");
 XeenAssetSource assets(*install);
 const auto sine=render(0,false),wave=render(1,false);
 assert(sine!=wave && wave==render(5,false));
 assert(render(0,true)!=sine);
 auto title=data(assets.readArchiveResource("newbrigh.m",XeenSceneArchive::DarksideOnly));
 auto outdoor=data(assets.readArchiveResource("outdoors.m",XeenSceneArchive::Clouds));
 auto town=data(assets.readArchiveResource("town.m",XeenSceneArchive::Clouds));
 auto blast=assets.readArchiveResource("blastmus",XeenSceneArchive::IntroOnly);
 if(blast!=assets.readArchiveResource("specmus",XeenSceneArchive::IntroOnly))throw std::runtime_error("INTRO specmus is not the authenticated blastmus alias");
 for(const auto profile:{1u,2u,6u}) {
  const char *name=profile==1?"admus":profile==2?"blastmus":"specmus";
  auto resource=data(assets.readArchiveResource(name,XeenSceneArchive::IntroOnly));
  auto captured=dro(std::filesystem::u8path(argv[2])/name/"rawopl0001.dro");
  for(unsigned id=0;id<=180;++id){PromusDriver d(resource,{},profile);d.effect(id);for(unsigned t=0;t<512;++t)d.tick();}
  {
   std::vector<PromusDriver::Write> logical,rendered;
   PromusDriver d(resource,[&](const auto &w){logical.push_back(w);},profile);
   XeenFmSource source([&](const auto &w){rendered.push_back(w);});source.initialize(resource,profile);
   d.load(title);source.load(title,true);for(unsigned t=0;t<100;++t)d.tick();
   const auto end=PromusDriver::sampleBoundary(100)+1;std::vector<std::int16_t> pcm(1024*2);
   while(source.samples()<end)source.pull(pcm.data(),std::min<std::uint64_t>(1024,end-source.samples()));
   source.check();assert(logical.size()==rendered.size());
   for(std::size_t i=0;i<logical.size();++i)assert(logical[i].tick==rendered[i].tick && logical[i].reg==rendered[i].reg && logical[i].value==rendered[i].value && logical[i].interrupt==rendered[i].interrupt);
   d.restartProgram();source.restartProgram();assert(d.ticks()==100 && !d.musicPlaying());
   d.load(title);source.load(title,true);for(unsigned t=0;t<20;++t)d.tick();
   const auto next=PromusDriver::sampleBoundary(120)+1;
   while(source.samples()<next)source.pull(pcm.data(),std::min<std::uint64_t>(1024,next-source.samples()));
   source.check();assert(logical.size()==rendered.size());
   for(std::size_t i=0;i<logical.size();++i)assert(logical[i].tick==rendered[i].tick && logical[i].reg==rendered[i].reg && logical[i].value==rendered[i].value && logical[i].interrupt==rendered[i].interrupt);
  }
  {
   std::vector<PromusDriver::Write> writes;
   PromusDriver d(resource,[&](const auto &w){writes.push_back(w);},profile);
   writes.clear();d.command(255);assert(d.fadeSpeed()==0);
   d.command(0);assert(writes.size()==14);
   for(unsigned i=0;i<7;++i){assert(!writes[2*i].interrupt && writes[2*i].reg==0xa6-i && writes[2*i].value==0);
    assert(!writes[2*i+1].interrupt && writes[2*i+1].reg==0xb6-i && writes[2*i+1].value==0);}
   d.load(data({0x1f,0xff}));d.command(255);
   for(unsigned t=0;t<64;++t)d.tick();assert(d.fadeSpeed()==255);
   d.tick();assert(d.fadeSpeed()==0 && !d.musicPlaying());
   assert(std::count_if(writes.begin(),writes.end(),[](const auto &w){return w.reg>=0xb0 && w.reg<=0xb6 && w.value==0;})>=7);
   writes.clear();d.effect(7);assert(d.effectPlaying());const auto first=writes.size();
   d.effect(8);d.effect(181);assert(writes.size()==first);
   d.effect(180);assert(writes.size()>first && d.effectPlaying());
   const auto gain=d.mixerLevel();d.command(0x100,0);assert(d.mixerLevel()==gain);
   PromusDriver terminal(resource,{},profile);terminal.load(data({0xf0}));terminal.command(207);
   const auto phase=terminal.fadePhase();terminal.tick();assert(terminal.fadePhase()==phase && terminal.fadeSpeed()==207);
  }
  {
   Bytes song{0x22};for(unsigned i=0;i<26;++i)song.push_back(std::uint8_t(0x40+i));
   song.insert(song.end(),{0x10,255,0x10,155,0xc0,2,0x1f,0xff});
   std::vector<PromusDriver::Write> writes;
   PromusDriver d(resource,[&](const auto &w){writes.push_back(w);},profile);d.load(data(song));
   for(unsigned t=0;t<411;++t)d.tick();
   const auto it=std::find_if(writes.begin(),writes.end(),[](const auto &w){return w.tick==411 && w.reg==0x20;});
   assert(it!=writes.end() && it->value==(profile==1?song[0]:song[1]));
   d.restartProgram();song.resize(27);song.insert(song.end(),{0x10,100,0xc0,2,0x1f,0xff});d.load(data(song));
   for(unsigned t=0;t<101;++t)d.tick();
   const auto fresh=std::find_if(writes.begin(),writes.end(),[](const auto &w){return w.tick==512 && w.reg==0x20;});
   assert(fresh!=writes.end() && fresh->value==song[1]);
  }
  {
   struct Case {unsigned volume,carrier,expected;};
   for(const auto test:{Case{0,10,63},Case{127,10,10},Case{127,0xc1,63}}) {
    Bytes song{0x20};for(unsigned i=0;i<26;++i)song.push_back(i==6?test.carrier:0);
    song.insert(song.end(),{0xc0,0,0x1f,0xff});
    std::vector<PromusDriver::Write> writes;
    PromusDriver d(resource,[&](const auto &w){writes.push_back(w);},profile);d.command(0x100,test.volume);d.load(data(song));d.tick();
    const auto it=std::find_if(writes.begin(),writes.end(),[](const auto &w){return w.tick==1 && w.reg==0x43;});
    assert(it!=writes.end() && it->value==test.expected);
   }
  }
  {
   auto image=*resource;const auto start=image.size();const auto table=profile==1?0x975u:0x94fu;
   image[table]=start;image[table+1]=start>>8;
   image.insert(image.end(),{0xe8,255,0,1,0x10,255,0x10,255,0xf0});
   std::vector<PromusDriver::Write> writes;
   PromusDriver d(data(std::move(image)),[&](const auto &w){writes.push_back(w);},profile);d.effect(0);
   for(unsigned t=0;t<500;++t)d.tick();
   const auto count=[&](unsigned lo,unsigned hi){return std::count_if(writes.begin(),writes.end(),[&](const auto &w){return w.reg==0xa8 && w.tick>=lo && w.tick<=hi;});};
   assert(count(2,400)>0);assert((count(401,500)==0)==(profile==1));
  }
  {
   auto image=*resource;image[0x1ca+3*2]=image[0x1ca+8*2];image[0x1cb+3*2]=image[0x1cb+8*2];
   std::vector<PromusDriver::Write> writes;
   PromusDriver d(data(std::move(image)),[&](const auto &w){writes.push_back(w);},profile);
   d.load(data({0x30,0,0x1f,0xff}));writes.clear();d.tick();
   assert(writes.size()==2 && writes[0].reg==0xa0 && writes[1].reg==0xb0);
  }
  {
   auto image=*resource;const auto start=image.size();const auto table=profile==1?0x975u:0x94fu;
   image[table]=start;image[table+1]=start>>8;image.push_back(0x20);
   for(unsigned i=0;i<11;++i)image.push_back(std::uint8_t(i+1));
   image.insert(image.end(),{0xc8,0,0x1f,0xf0});
   std::vector<PromusDriver::Write> writes;
   PromusDriver d(data(std::move(image)),[&](const auto &w){writes.push_back(w);},profile);d.effect(0);writes.clear();d.tick();
   assert(writes.size()==11 && std::all_of(writes.begin(),writes.end(),[](const auto &w){return w.reg<256;}));
  }
  const auto make=[&](PromusDriver::Bytes song,unsigned ticks=1800) {
   std::vector<Event> raw;PromusDriver d(resource,[&](const auto &w){raw.push_back({w.tick,w.reg,w.value});},profile);
   d.load(song);for(unsigned t=0;t<ticks;++t)d.tick();return raw;
  };
  auto titleWrites=make(title);const auto start=profile==2?24700:24740;
  window(titleWrites,captured,2,100,start,start+2000,(std::string(name)+" title start").c_str());
  window(titleWrites,captured,300,900,start,start+15000,(std::string(name)+" stable title").c_str());
  auto outdoorWrites=make(outdoor);const auto out=profile==1?115000:profile==2?174700:81100;
  window(outdoorWrites,captured,2,250,out,out+5000,(std::string(name)+" outdoor").c_str());
  stopOrder(captured,profile==1?115068:profile==2?174803:81181,(std::string(name)+" fade stop").c_str());
  if(profile==1)window(make(town,2200),captured,2080,2120,83000,84000,"admus town pointer overlap");
  {
   // The same music record has a distinct attenuation for each selector.
   // Only the selected driver's command may change the carrier level.
   std::vector<PromusDriver::Write> writes;
   PromusDriver d(resource,[&](const auto &w){writes.push_back(w);},profile);
   d.load(data({0xa0,2,11,0xa0,5,22,0x1f,0xff}));writes.clear();d.tick();
   std::vector<unsigned> levels;for(const auto &w:writes)if(w.reg==0x43)levels.push_back(w.value&63);
   assert(levels==std::vector<unsigned>{profile==1?11u:22u});
  }
  for(unsigned compatibility:{0u,1u,2u}) {
   const auto effectWrites=[&](unsigned operation) {
    auto image=*resource;image.at(15)=compatibility;image.at(16)=0;
    const auto table=profile==1?0x975u:0x94fu;const auto start=image.size();
    image.at(table)=start;image.at(table+1)=start>>8;
    if(operation==1) {
     image.push_back(0x20);for(unsigned i=0;i<11;++i)image.push_back(i+1);
     image.insert(image.end(),{0xc7,0,0x1f,0xf0});
    }else if(operation==0)image.insert(image.end(),{0x37,12,0x1f,0xf0});
    else if(operation==2)image.insert(image.end(),{0x97,0x20,0x1f,0xf0});
    else if(operation==3)image.insert(image.end(),{0xe7,255,0,1,0x1f,0xf0});
    else image.insert(image.end(),{0x67,0x20,0x1f,0xf0});
    std::vector<PromusDriver::Write> writes;
    PromusDriver d(data(std::move(image)),[&](const auto &w){writes.push_back(w);},profile);
    d.effect(0);writes.clear();d.tick();
    const unsigned reg=operation<2?0x54:operation==2?0xb7:0xa7;
    return std::any_of(writes.begin(),writes.end(),[reg](const auto &w){return w.reg==reg;});
   };
   const bool guarded=profile==1?compatibility==0:compatibility!=2;
   assert(effectWrites(0)==guarded && effectWrites(2)==guarded && effectWrites(3)==guarded);
   assert(effectWrites(1)==(profile==1?compatibility==0:true));
   assert(effectWrites(4)==(compatibility==0));
  }
 }
 std::cout<<"M94 mono OPL windows passed\n";
}catch(const std::exception &e){std::cerr<<e.what()<<'\n';return 1;}}
