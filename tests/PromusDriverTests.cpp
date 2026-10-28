#include "games/xeen/PromusDriver.h"
#include "compat/scummvm/XeenOpl3.h"
#include "formats/xeen/XeenAssetSource.h"
#include "games/xeen/XeenInstallationDetector.h"
#include "app/XeenAudio.h"
#include "app/XeenTitleFlow.h"
#include "app/XeenFmSource.h"
#include "XeenSaveGameplayTestSupport.h"
#include "formats/xeen/XeenMapFormat.h"
#include <cassert>
#include <iostream>
#include <fstream>
#include <map>
#include <cmath>
#include <cstdio>
#include <algorithm>
using namespace mmodern;
using Bytes=std::vector<std::uint8_t>;
using Write=PromusDriver::Write;
static PromusDriver::Bytes data(Bytes b) {return std::make_shared<const Bytes>(std::move(b));}
struct Event {std::uint64_t time;unsigned reg,value;bool interrupt=true;};
static int voice(unsigned reg) {
 const auto r=reg&255;
 if((r>=0xa0&&r<=0xa8)||(r>=0xb0&&r<=0xb8)||(r>=0xc0&&r<=0xc8))return r&15;
 for(unsigned v=0;v<9;++v)for(unsigned g:{0x20,0x40,0x60,0x80,0xe0})
  if(r==g+v/3*8+v%3 || r==g+v/3*8+v%3+3)return v;
 return -1;
}
static std::vector<Event> changes(const std::vector<Event> &input,bool fx) {
 std::array<unsigned,512> last{};last.fill(0);
 std::vector<Event> result;
 for(const auto &e:input) {
  const auto v=voice(e.reg);if(v<0 || ((v>=7)!=fx))continue;
  if(last.at(e.reg)!=e.value){last[e.reg]=e.value;result.push_back(e);}
 }
 return result;
}
struct Match {std::size_t count=0,expected=0,captured=0;double jitter=0;};
static Match compare(const std::vector<Event> &a,const std::vector<Event> &b,const char *name) {
 const auto equal=[](const Event &x,const Event &y){return x.reg==y.reg && x.value==y.value;};
 std::size_t best=0,ba=0,bb=0;
 for(std::size_t j=0;j+32<b.size();++j)for(std::size_t i=0;i+32<a.size();++i) {
  if(!equal(a[i],b[j]) || !equal(a[i+1],b[j+1]) || !equal(a[i+31],b[j+31]))continue;
  std::size_t n=0;while(i+n<a.size() && j+n<b.size() && equal(a[i+n],b[j+n]))++n;
  if(n>best){best=n;ba=i;bb=j;}
 }
 if(name)std::cout<<name<<" matched writes="<<best<<" expected@"<<ba<<" captured@"<<bb<<" of "<<b.size()<<'\n';
 double jitter=0;
 if(best>32) {
  double lo=1e99,hi=-1e99;
  for(std::size_t k=0;k<best;++k)if(a[ba+k].interrupt){const auto offset=double(b[bb+k].time)-double(a[ba+k].time)*1000*16390/1193182;lo=std::min(lo,offset);hi=std::max(hi,offset);}
  jitter=hi-lo;
  if(name) {
   std::cout<<"PIT alignment uncertainty ms="<<jitter<<'\n';
   const auto phase=(lo+hi)/2,period=1000.0*PromusDriver::divisor/PromusDriver::pit;
   for(std::size_t k=0;k<best;++k)if(a[ba+k].interrupt)
    assert(std::llround((b[bb+k].time-phase)/period)==static_cast<long long>(a[ba+k].time));
  }
 }
 if(name && best && ba+best<a.size() && bb+best<b.size()) {
  const auto &x=a[ba+best],&y=b[bb+best];std::cout<<"mismatch expected="<<x.time<<':'<<std::hex<<x.reg<<'='<<x.value<<" capture="<<std::dec<<y.time<<':'<<std::hex<<y.reg<<'='<<y.value<<std::dec<<'\n';
 }
 return {best,ba,bb,jitter};
}
static std::vector<Event> dro(const std::filesystem::path &file) {
 std::ifstream in(file,std::ios::binary);Bytes b((std::istreambuf_iterator<char>(in)),{});
 if(b.size()<26 || std::string(b.begin(),b.begin()+8)!="DBRAWOPL" || b[8]!=2 || b[22] || b[21])throw std::runtime_error("Invalid DRO v2 capture");
 const auto count=b[25];if(b.size()<26+count || (b.size()-26-count)%2)throw std::runtime_error("Truncated DRO");
 std::vector<Event> result;std::uint64_t time=0;
 for(std::size_t p=26+count;p<b.size();p+=2) {
  const auto c=b[p],v=b[p+1];
  if(c==b[23])time+=v+1;
  else if(c==b[24])time+=(v+1)*256;
  else {if((c&127)>=count)throw std::runtime_error("Invalid DRO register");result.push_back({time,unsigned(c>>7)*256+b[26+(c&127)],v});}
 }
 return result;
}
int main(int argc,char **argv) {
 std::cout<<std::unitbuf;
 try {
  assert(argc==3);
  const auto installation=XeenInstallationDetector().detect(std::filesystem::u8path(argv[1]));assert(installation);
  XeenAssetSource assets(*installation);
  const auto resource=data(assets.readArchiveResource("promus",XeenSceneArchive::IntroOnly));
  assert(XeenOpl3::systemIsNull());XeenOpl3 opl;
  PromusDriver silence(resource);silence.command(207);for(int t=0;t<79;++t)silence.tick();assert(silence.fadeSpeed()==207);
  silence.tick();assert(silence.fadeSpeed()==0);assert(silence.fadePhase()!=0);
  silence.command(207);for(int t=0;t<78;++t)silence.tick();assert(silence.fadeSpeed()==207);
  silence.tick();assert(silence.fadeSpeed()==0); // Phase survives stop and fade requests.
  PromusDriver terminal(resource);terminal.load(data({0xf0}));terminal.command(207);terminal.tick();assert(terminal.fadePhase()==0);
  for(unsigned t=0;t<79;++t)terminal.tick();assert(terminal.fadeSpeed()==207);terminal.tick();assert(!terminal.fadeSpeed());
  unsigned fxInterruptWrites=0;
  PromusDriver priority(resource,[&](const auto &w){if(w.interrupt&&voice(w.reg)>=7)++fxInterruptWrites;});
  priority.load(data({0x11,0xff}));priority.effect(20);for(unsigned t=0;t<20;++t)priority.tick();assert(fxInterruptWrites==0);
  std::vector<Write> patches;
  PromusDriver patchDriver(resource,[&](const auto &w){patches.push_back(w);});
  Bytes patchSong(27,0);patchSong[0]=0x20;patchSong[7]=0x41;patchSong[18]=0x82;
  patchSong.insert(patchSong.end(),{0xc0,0,0xa0,3,5,0x11,0xff});
  patchDriver.load(data(patchSong));patchDriver.command(207);patches.clear();patchDriver.tick();
  const auto has=[&](unsigned r,unsigned v){return std::any_of(patches.begin(),patches.end(),[&](const auto &w){return w.reg==r&&w.value==v;});};
  assert(has(0x43,0x7f)&&has(0x143,0x82)); // Only bank zero substitutes fade steps.
  patchDriver.command(0x100,75);assert(patchDriver.mixerLevel()==0xaa);
  for(unsigned id=0;id<=180;++id) {
   PromusDriver fx(resource);fx.effect(id);for(unsigned t=0;t<512;++t)fx.tick();
  }
  {
   PromusDriver admitted(resource);admitted.effect(7);admitted.effect(181);assert(admitted.effectPlaying());
   std::vector<Write> alone,repeated;
   PromusDriver first(resource,[&](const auto &w){alone.push_back(w);}),second(resource,[&](const auto &w){repeated.push_back(w);});
   first.effect(7);second.effect(7);alone.clear();repeated.clear();
   for(unsigned t=0;t<20;++t){if(t==1)second.effect(8);first.tick();second.tick();}
   assert(alone.size()==repeated.size());
   for(std::size_t i=0;i<alone.size();++i)assert(alone[i].reg==repeated[i].reg&&alone[i].value==repeated[i].value&&alone[i].tick==repeated[i].tick);
  }
  {
   // Synthetic opcode-6 effect corrupts voice 3's high cache byte. The later
   // music key-off must expose that DOS byte-index defect in both banks.
   Bytes altered=*resource;const auto target=altered.size();
   altered[0xadc]=target&255;altered[0xadd]=target>>8;
   altered.insert(altered.end(),{0x67,2,0x1f,0xf0});
   std::vector<Write> stream;
   PromusDriver defect(data(std::move(altered)),[&](const auto &w){stream.push_back(w);});
   defect.load(data({0x93,1,0,0x13,0x83,0,0x11,0xff}));defect.tick();defect.effect(0);
   for(unsigned t=0;t<3;++t)defect.tick();
   const unsigned high=resource->at(0x1b6+2*2)&31;
   for(unsigned bank=0;bank<2;++bank)
    assert(std::any_of(stream.begin(),stream.end(),[&](const auto &w){return w.tick==4&&w.reg==bank*256+0xb3&&w.value==high;}));
  }
  assert(PromusDriver::clockTicks(PromusDriver::clockBoundary(1000000))==1000000);
  for(std::uint64_t t=1;t<100000;++t)assert(PromusDriver::sampleBoundary(t)==(t*49716*16390+1193181)/1193182);
  assert(XeenOpl3::systemIsNull());
  {
   XeenOpl3 synth;
   PromusDriver pcm(resource,[&](const auto &w){synth.write(w.reg,w.value);});
   pcm.load(data(assets.readArchiveResource("tavern.m")));
   std::uint64_t left=0,right=0;unsigned differing=0;
   for(std::uint64_t sample=0;sample<49716*2;++sample) {
    while(sample>=PromusDriver::sampleBoundary(pcm.ticks()+1))pcm.tick();
    std::int16_t stereo[2]{};synth.sample(stereo);
    left+=std::abs(int(stereo[0]));right+=std::abs(int(stereo[1]));differing+=stereo[0]!=stereo[1];
   }
   assert(left>0 && right>0 && differing>0 && pcm.ticks()==145);
   assert(XeenOpl3::systemIsNull());
  }
  std::ifstream moduleInput(installation->uiModule->physicalPath,std::ios::binary);
  const Bytes module((std::istreambuf_iterator<char>(moduleInput)),{});
  unsigned loads=0;
  const auto loader=[&](const std::string &name,XeenSceneArchive archive){++loads;return assets.readArchiveResource(name,archive);};
  assert(XeenSoundOptions(XeenPartyLoader().loadInitialCloudsParty(assets).soundOptions)==XeenSoundOptions{});
  XeenAudio owner(3,resource,loader,module,0,false);assert(!owner.outputActive());
  assert(XeenOpl3::systemIsNull());
  const char *byKind[]{"town.m","cavern.m","dungeon.m","castle.m","dungeon.m","outday1.m"};
  unsigned maps=0;
  for(unsigned id=0;id<256;++id) {
   char name[20];std::snprintf(name,sizeof(name),id>=100?"mazex%03u.dat":"maze%04u.dat",id);
   if(!assets.hasInitialResource(name))continue;
   const auto b=assets.readInitialResource(name);if(b.size()!=892)continue;
   const auto map=XeenMapFormat::parseDat(b);if(map.id!=id)continue;
   ++maps;assert(owner.mapSong(map)==(map.isOutdoors()?"outdoors.m":byKind[map.wallKind]));
  }
  assert(maps==128);
  XeenMapGeometry kind5;kind5.wallKind=5;assert(owner.mapSong(kind5)=="outday1.m");
  const char *locationNames[]{"bank.m","smith.m","guild.m","tavern.m","temple.m","grounds.m","endgame.m"};
  for(unsigned action=0;action<7;++action)assert(owner.locationSong(action)==locationNames[action]);
  owner.requestSong("town.m",XeenSceneArchive::Clouds,207,0);
  owner.advance(PromusDriver::clockBoundary(79));assert(owner.pending()&&loads==0);
  owner.advance(PromusDriver::clockBoundary(80));assert(!owner.pending()&&owner.loadedName()=="town.m");
  const auto before=loads;owner.requestSong("TOWN.M",XeenSceneArchive::Clouds,207,PromusDriver::clockBoundary(81));assert(!owner.pending()&&loads==before);
  owner.requestInn(PromusDriver::clockBoundary(82));
  owner.advance(PromusDriver::clockBoundary(400));assert(owner.loadedName()=="inn.m");
  owner.requestLocation(0,PromusDriver::clockBoundary(401));
  owner.retainCdCue({19,25,3000});owner.startCd(PromusDriver::clockBoundary(401));
  owner.advance(PromusDriver::clockBoundary(600));assert(owner.pending());
  owner.stopCd(PromusDriver::clockBoundary(700));owner.advance(PromusDriver::clockBoundary(700));assert(owner.loadedName()=="bank.m");
  owner.enterTitle(PromusDriver::clockBoundary(701));assert(owner.loadedName()=="newbrigh.m");
  const auto current=owner.currentName();owner.leaveTitle(PromusDriver::clockBoundary(702));
  owner.advance(PromusDriver::clockBoundary(900));assert(owner.loadedName()=="twnnitea.m"&&owner.currentName()==current);
  {
   std::vector<PromusDriver::Write> trace;
   XeenAudio options(3,resource,loader,module,0,false,[&](const auto &w){trace.push_back(w);});
   options.enterTitle(0);const auto playing=trace.size();
   XeenSoundOptions value;value.music=false;options.observeOptions(value);
   assert(trace.size()==playing); // Loaded Music-off alone sends no stop.
   options.requestSong("town.m",XeenSceneArchive::Clouds,207,0);assert(options.pending());
   options.toggleMusic(false,0);assert(options.pending()); // Off cancels fade, keeps load.
   options.retainCdCue({19,25,100});options.startCd(0);options.advance(0);assert(options.pending()); // CD wait still holds it.
   options.toggleMusic(true,0);
   assert(!options.pending() && options.loadedName()=="newbrigh.m" && options.currentName()=="town.m");
   options.requestSong("TOWN.M",XeenSceneArchive::Clouds,207,0);assert(!options.pending());
   options.stopCd(0);options.requestInn(0);options.toggleMusic(false,0);options.advance(0);
   assert(!options.pending() && options.loadedName()=="inn.m");
   options.toggleMusic(true,0);assert(options.loadedName()=="inn.m");
   value.efx=false;options.observeOptions(value);const auto before=trace.size();
   options.effect(20,0);assert(trace.size()==before); // No new FX and no stop.
   value.efx=true;options.observeOptions(value);trace.clear();options.effect(20,0);
   std::vector<PromusDriver::Write> expected;
   PromusDriver direct(resource,[&](const auto &w){expected.push_back(w);});expected.clear();direct.effect(20);
   assert(trace.size()==expected.size() && !trace.empty());
   for(unsigned i=0;i<trace.size();++i)assert(trace[i].reg==expected[i].reg && trace[i].value==expected[i].value);
  }
  {
   XeenAudio epochs(3,resource,loader,module,1000000000000ULL,false);
   epochs.requestInn(1000000000000ULL);
   epochs.advance(1000000000000ULL+PromusDriver::clockBoundary(50));
   const auto ticks=epochs.ticks();assert(epochs.pending());
   epochs.rebaseClock(0);assert(epochs.ticks()==ticks);
   epochs.advance(PromusDriver::clockBoundary(100));assert(epochs.loadedName()=="inn.m");
   epochs.rebaseClock(9000000000000ULL);epochs.advance(9000000000000ULL+PromusDriver::clockBoundary(1));assert(epochs.ticks()==151);
  }
  XeenAudio missing(3,resource,[](const auto &,auto)->Bytes{throw std::runtime_error("missing song");},module,0,false);
  bool rejected=false;try{missing.enterTitle(0);}catch(const std::runtime_error &){rejected=true;}assert(rejected);
  for(unsigned id:{0,5,7,8,9,10,65535}) {
   unsigned writes=0;XeenAudio other(id,{},loader,module,0,false,[&](const auto &){++writes;});
   other.requestInn(0);other.advance(PromusDriver::clockBoundary(200));assert(other.loadedName()=="inn.m"&&writes==0);
  }
  std::array<std::vector<Write>,2> traces;std::array<std::vector<std::string>,2> timing;
  for(unsigned mode=0;mode<2;++mode) {
   XeenAudio route(3,resource,loader,module,0,mode!=0,[&](const auto &w){traces[mode].push_back(w);});
   if(mode)assert(route.outputActive());
   route.enterTitle(0);route.leaveTitle(PromusDriver::clockBoundary(20));
   route.requestSong("town.m",XeenSceneArchive::Clouds,207,PromusDriver::clockBoundary(21));
   route.retainCdCue({19,25,3000});route.startCd(PromusDriver::clockBoundary(21));
   for(unsigned t=22;t<900;++t) {
    const auto now=PromusDriver::clockBoundary(t);if(t==400)route.stopCd(now);route.advance(now);
    if(t==450)route.requestLocation(3,now);
    if(t==600)route.effect(20,now);
    timing[mode].push_back(route.loadedName()+":"+std::to_string(route.pending())+":"+std::to_string(route.ticks()));
   }
   assert(XeenOpl3::systemIsNull());
  }
  assert(timing[0]==timing[1]&&traces[0].size()==traces[1].size());
  for(std::size_t i=0;i<traces[0].size();++i)assert(traces[0][i].tick==traces[1][i].tick&&traces[0][i].reg==traces[1][i].reg&&traces[0][i].value==traces[1][i].value);
  std::cout<<"Clouds maps="<<maps<<"; identical device/no-device route trace writes="<<traces[0].size()<<'\n';
  {
   // Pull in lockstep to isolate callback partitioning from clock drift.
   std::array<std::vector<Write>,3> rendered;
   std::array<std::vector<std::int16_t>,3> pcm;
   for(unsigned mode=0;mode<3;++mode) {
    auto source=std::make_shared<XeenFmSource>([&](const auto &w){rendered[mode].push_back(w);});
    std::vector<Write> authoritative;
    XeenAudio route(3,resource,loader,module,0,false,[&](const auto &w){authoritative.push_back(w);},source);
    const unsigned frames=mode==0?127:mode==1?512:2048;
    unsigned fullPulls=0;
    std::vector<std::int16_t> block(frames*2);
    const auto pull=[&] {
     while(source->samples()<route.samples()) {
      const auto count=std::min<std::uint64_t>(frames,route.samples()-source->samples());
      if(count==frames)++fullPulls;
      source->pull(block.data(),count);source->check();
      pcm[mode].insert(pcm[mode].end(),block.begin(),block.begin()+count*2);
     }
     source->pull(block.data(),0);source->check(); // Tick at the endpoint, once.
    };
    route.enterTitle(0);pull();
    for(unsigned t=1;t<=600;++t) {
     const auto now=PromusDriver::clockBoundary(t);
     route.advance(now);
     // Five ticks permit full 2048-frame pulls across several ISR boundaries.
     // Still land exactly on every externally requested operation.
     if(t%5==0 || t==21)pull();
     if(t==20)route.leaveTitle(now);
     if(t==21)route.requestSong("town.m",XeenSceneArchive::Clouds,207,now);
     if(t==300)route.effect(20,now);
     if(t==400)route.requestInn(now);
     if(t==20 || t==21 || t==300 || t==400)pull();
    }
    // Finish at the same sample, including the endpoint tick.
    route.advance(PromusDriver::clockBoundary(600)+1000000);pull();
    assert(fullPulls>0);
    assert(authoritative.size()==rendered[mode].size());
    for(std::size_t i=0;i<authoritative.size();++i) {
     const auto &a=authoritative[i],&b=rendered[mode][i];
     assert(a.tick==b.tick&&a.reg==b.reg&&a.value==b.value&&a.interrupt==b.interrupt);
    }
   }
   assert(pcm[0]==pcm[1]&&pcm[0]==pcm[2]);
   for(unsigned mode=1;mode<3;++mode) {
    assert(rendered[0].size()==rendered[mode].size());
    for(std::size_t i=0;i<rendered[0].size();++i) {
     const auto &a=rendered[0][i],&b=rendered[mode][i];
     assert(a.tick==b.tick&&a.reg==b.reg&&a.value==b.value&&a.interrupt==b.interrupt);
    }
   }
   std::cout<<"Rendered callback sizes 127/512/2048: identical register writes="<<rendered[0].size()<<" and native stereo samples="<<pcm[0].size()<<'\n';
  }
  {
   gameplay_test::Fixture fixture;std::uint64_t now=0,before=0;
   fixture.programState.audio=std::make_shared<XeenAudio>(3,resource,loader,module,SdlWindow::presentationNanoseconds(),false);
   auto services=fixture.services();services.presentationClock=[&]{return now;};
   services.show=[&](const auto &,const auto &handler,const auto &,const auto &,const auto &) {
    before=fixture.programState.audio->ticks();
    // Final native input closes the session without another idle/beginCycle.
    now=PromusDriver::clockBoundary(100);handler.closed();return true;
   };
   assert(Application(services.program).playGameplay(services,{1,1,1,XeenDirection::North},{},false)==0);
   assert(fixture.programState.audio->ticks()==before+100);
  }
  {
   // Exercise the actual panel-to-party-to-audio path with a recording sink.
   gameplay_test::Fixture fixture;fixture.initial.soundOptions={false,true,false,true};
   std::uint64_t now=0;std::vector<PromusDriver::Write> trace;
   fixture.programState.audio=std::make_shared<XeenAudio>(3,resource,loader,module,SdlWindow::presentationNanoseconds(),false,
       [&](const auto &w){trace.push_back(w);});
   const XeenFontFormat font(assets.readArchiveResource("fnt"));
   auto services=fixture.services();services.presentationClock=[&]{return now;};
   const XeenPartyState *party=nullptr;
   services.observeGameplay=[&](auto &,auto &,const auto &p,const auto &,auto &){party=&p;};
   services.panel=[&](const IndexedFrame &base,bool,bool,bool,std::optional<unsigned>,const std::string &) {
    XeenTitleFlow::Services s{assets.uiText(),font,base,{}, {},
      [&](auto &frame,const char *name,unsigned index,int x,int y){assets.drawDialogSprite(frame,name,index,x,y);},[]{return std::array<XeenSaveFile::Slot,10>{};},[](unsigned){return std::filesystem::path{};}};
    s.panel=true;return std::make_unique<XeenTitleFlow>(std::move(s));
   };
   services.show=[&](auto visible,const auto &h,const auto &,const auto &,const auto &) {
    const auto present=[&]{h.framePresented(visible.presentation());h.completeInputHandoff(visible.presentation());};present();
    const auto send=[&](const PlayerAction &action) {
     const auto next=h(action);assert(next);visible=*next;present();
    };
    const auto key=[&](unsigned key){const auto context=h.inputContext(visible.presentation());assert(context.dialog);
     const auto action=context.dialog->key(key);assert(action);send(*action);};
    send(ControlPanelAction{});trace.clear();key('e');
    assert(XeenSoundOptions(party->soundOptions).efx && !trace.empty());
    std::vector<PromusDriver::Write> expected;
    PromusDriver direct(resource,[&](const auto &w){expected.push_back(w);});expected.clear();direct.effect(20);
    assert(trace.size()==expected.size());
    for(unsigned i=0;i<trace.size();++i)assert(trace[i].reg==expected[i].reg && trace[i].value==expected[i].value);
    trace.clear();key('e');assert(!XeenSoundOptions(party->soundOptions).efx && trace.empty());
    key('p');assert(!XeenSoundOptions(party->soundOptions).speech && XeenSoundOptions(party->soundOptions).text);
    key('t');assert(XeenSoundOptions(party->soundOptions).text);
    key('p');key('t');assert(!XeenSoundOptions(party->soundOptions).text && XeenSoundOptions(party->soundOptions).speech);
    key(27);h.closed();return true;
   };
   assert(Application(services.program).playGameplay(services,{1,1,1,XeenDirection::North},{},false)==0);
  }
  {
   // Nuked enters release only when a generated sample sees key-off. Effect 20
   // re-strikes its held voices 7/8 with key-off then key-on in one tick, so
   // unspaced writes kept it nearly silent. Compare two rendered seconds with
   // the DOSBox capture, one captured write per sample in any core write mode.
   constexpr std::size_t window=2*49716;
   const auto energy=[](const std::int16_t *pcm,std::size_t frames) {
    double sum=0;for(std::size_t i=0;i<frames*2;++i)sum+=double(pcm[i])*pcm[i];return sum;
   };
   const auto events=dro(std::filesystem::u8path(argv[2])/"promus-fx20-alone.dro");
   std::vector<std::int16_t> captured(window*2);
   {
    XeenOpl3 chip;std::size_t next=0;
    for(std::size_t s=0;s<window;++s) {
     if(next<events.size() && events[next].time*49716/1000<=s){chip.write(events[next].reg,events[next].value);++next;}
     chip.sample(&captured[s*2]);
    }
    assert(next==events.size());
   }
   const auto reference=energy(captured.data(),window);assert(reference>0);
   const auto comparable=[&](double value) {
    std::cout<<"effect20 energy="<<value<<" capture="<<reference<<'\n';
    return value>=reference/2 && value<=reference*2;
   };
   unsigned chunk=127,fullPulls=0;
   std::vector<std::int16_t> block(4096);
   const auto drain=[&](XeenFmSource &source,const XeenAudio &audio,std::vector<std::int16_t> &pcm) {
    while(source.samples()<audio.samples()) {
     const auto count=std::min<std::uint64_t>(chunk,audio.samples()-source.samples());
     if(count==chunk)++fullPulls;
     source.pull(block.data(),count);source.check();
     pcm.insert(pcm.end(),block.begin(),block.begin()+count*2);
    }
    source.pull(block.data(),0);source.check();
   };
   std::vector<std::int16_t> aloneReference;
   for(unsigned size:{127,512,2048}) {
    chunk=size;fullPulls=0;
    auto source=std::make_shared<XeenFmSource>();std::vector<std::int16_t> pcm;
    XeenAudio alone(3,resource,loader,module,0,false,{},source);alone.effect(20,0);
    for(unsigned t=1;t<=150;++t){alone.advance(PromusDriver::clockBoundary(t));if(t%5==0)drain(*source,alone,pcm);}
    assert(fullPulls>0);
    assert(pcm.size()>=window*2 && comparable(energy(pcm.data(),window)));
    if(aloneReference.empty())aloneReference=pcm;else assert(aloneReference==pcm);
   }
   // The actual panel-to-party-to-audio path, rendered by the chip consumer.
   const auto panel=[&](bool music) {
    fullPulls=0;
    gameplay_test::Fixture fixture;fixture.initial.soundOptions={false,music,false,true};
    std::uint64_t now=0;std::size_t start=0,toggled=0;std::vector<Write> authoritative,rendered;std::vector<std::int16_t> pcm;
    std::vector<std::pair<std::uint64_t,Write>> scheduled;
    auto source=std::make_shared<XeenFmSource>([&](const auto &w){rendered.push_back(w);});
    const XeenFontFormat font(assets.readArchiveResource("fnt"));
    auto services=fixture.services();services.presentationClock=[&]{return now;};
    services.panel=[&](const IndexedFrame &base,bool,bool,bool,std::optional<unsigned>,const std::string &) {
     XeenTitleFlow::Services s{assets.uiText(),font,base,{}, {},
       [&](auto &frame,const char *name,unsigned index,int x,int y){assets.drawDialogSprite(frame,name,index,x,y);},[]{return std::array<XeenSaveFile::Slot,10>{};},[](unsigned){return std::filesystem::path{};}};
     s.panel=true;return std::make_unique<XeenTitleFlow>(std::move(s));
    };
    services.show=[&](auto visible,const auto &h,const auto &,const auto &idle,const auto &) {
     // Attach after the real title-clock handoff; this panel witness uses
     // only the injected presentation clock, for identical PCM across runs.
     fixture.programState.audio=std::make_shared<XeenAudio>(3,resource,loader,module,now,false,
         [&](const auto &w){
          authoritative.push_back(w);
          const auto sample=w.interrupt?PromusDriver::sampleBoundary(w.tick):fixture.programState.audio?fixture.programState.audio->samples():0;
          scheduled.push_back({sample,w});
         },source);
     h.beginCycle(0);
     const auto present=[&]{h.framePresented(visible.presentation());h.completeInputHandoff(visible.presentation());};present();
     const auto send=[&](const PlayerAction &action){const auto next=h(action);assert(next);visible=*next;present();};
     const auto run=[&](unsigned from,unsigned to) {
      for(unsigned t=from;t<to;++t) {
       now=PromusDriver::clockBoundary(t);h.beginCycle(t);if(const auto next=idle()){visible=*next;present();}
       if(t%5==0)drain(*source,*fixture.programState.audio,pcm);
      }
      drain(*source,*fixture.programState.audio,pcm); // Land before the panel action.
     };
     run(1,200);send(ControlPanelAction{});run(200,220);start=pcm.size();toggled=rendered.size();
     const auto action=h.inputContext(visible.presentation()).dialog->key('e');assert(action);send(*action);
     run(220,380);h.closed();return true;
    };
    assert(Application(services.program).playGameplay(services,{1,1,1,XeenDirection::North},{},false)==0);
    assert(authoritative.size()==rendered.size() && pcm.size()>=start+window*2);
    assert(fullPulls>0);
    for(std::size_t i=0;i<rendered.size();++i)
     assert(authoritative[i].tick==rendered[i].tick && authoritative[i].reg==rendered[i].reg && authoritative[i].value==rendered[i].value);
    assert(std::any_of(rendered.begin()+toggled,rendered.end(),[](const auto &w){return voice(w.reg)>=7;}));
    XeenOpl3 referenceChip;std::vector<std::int16_t> expected(pcm.size());std::size_t next=0;
    for(std::size_t sample=0;sample<pcm.size()/2;++sample) {
     while(next<scheduled.size() && scheduled[next].first<=sample) {
      const auto &w=scheduled[next++].second;referenceChip.write(w.reg,w.value);
     }
     referenceChip.sample(expected.data()+sample*2);
    }
    assert(pcm==expected); // Same timeline PCM for every callback partition.
    pcm.erase(pcm.begin(),pcm.begin()+start);pcm.resize(window*2);return pcm;
   };
   std::array<std::vector<std::int16_t>,2> panelReference;
   for(unsigned size:{127,512,2048}) {
    chunk=size;
    for(unsigned music=0;music<2;++music) {
     const auto pcm=panel(music!=0);
     if(!music)assert(comparable(energy(pcm.data(),window)));
     if(size==127)panelReference[music]=pcm;else assert(panelReference[music]==pcm);
    }
   }
   // Music shares the chip's write queue (its writes shift behind the effect's)
   // and may set voice 7/8 levels, so PCM cannot isolate the effect there;
   // every effect write must still be fed to the chip in order.

  }
  for(const auto &name:{"promus-startup","promus-inn","promus-outdoors","promus-fx20-alone","promus-wall-bump","promus-fx20-over-music"}) {
   const auto events=dro(std::filesystem::u8path(argv[2])/(std::string(name)+".dro"));
   std::cout<<name<<" writes="<<events.size()<<" duration="<<events.back().time<<'\n';
   const bool fx=std::string(name).find("fx20")!=std::string::npos||std::string(name).find("wall-bump")!=std::string::npos;
   const auto songName=std::string(name)=="promus-startup"?"newbrigh.m":std::string(name)=="promus-outdoors"?"outdoors.m":"tavern.m";
   std::vector<Event> generated;
   const auto captured=changes(events,fx);
   const bool overlay=std::string(name)=="promus-fx20-over-music";
   if(overlay) {
    const auto town=data(assets.readArchiveResource("town.m"));
    Match best;std::vector<Event> bestEvents;
    // The capture starts during town music, without recording its ISR phase.
    // Align that phase; then require the complete remaining FX register stream.
    for(unsigned phase=0;phase<512;++phase) {
     std::vector<Event> g;PromusDriver candidate(resource,[&](const auto &w){g.push_back({w.tick,w.reg,w.value,w.interrupt});});
     candidate.load(town);for(unsigned t=0;t<phase;++t)candidate.tick();candidate.effect(20);
     for(unsigned t=0;t<300;++t)candidate.tick();
     auto changed=changes(g,true);const auto match=compare(changed,captured,nullptr);
     if(match.count>best.count || (match.count==best.count&&match.jitter<best.jitter)){best=match;bestEvents=std::move(g);}
    }
    generated=std::move(bestEvents);
   }else {
   PromusDriver d(resource,[&](const auto &w){generated.push_back({w.tick,w.reg,w.value,w.interrupt});});
   if(fx)d.effect(std::string(name)=="promus-wall-bump"?46:20);
   else d.load(data(assets.readArchiveResource(songName,std::string(name)=="promus-startup"?XeenSceneArchive::DarksideOnly:XeenSceneArchive::Clouds)));
   for(int t=0;t<(fx?300:12000);++t)d.tick();
   }
   const auto match=compare(changes(generated,fx),captured,name);
   const std::size_t minimum=std::string(name)=="promus-startup"?600:std::string(name)=="promus-inn"?4000:std::string(name)=="promus-outdoors"?2300:std::string(name)=="promus-wall-bump"?1400:std::string(name)=="promus-fx20-alone"?95:140;
   assert(match.count>=minimum);
   assert(match.captured+match.count==captured.size());
   // One fitted PIT phase maps every retained write to the same driver tick;
   // DOSBox's millisecond delays and intra-tick I/O account for this uncertainty.
   assert(match.jitter<1000.0*PromusDriver::divisor/PromusDriver::pit/2);
  }
  std::cout<<"Promus driver tests passed\n";
 }catch(const std::exception &e){std::cerr<<e.what()<<'\n';return 1;}
}
