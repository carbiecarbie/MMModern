#include "app/XeenAudio.h"
#include "app/XeenFmSource.h"
#include "formats/xeen/XeenAssetSource.h"
#include "games/xeen/XeenInstallationDetector.h"
#include "core/PresentationClock.h"
#include <cassert>
#include <fstream>
#include <iostream>
#include <stdexcept>
#include <algorithm>
using namespace mmodern;
using Bytes=std::vector<std::uint8_t>;
namespace {
void check(bool value,const std::string &message) {if(!value)throw std::runtime_error(message);}
int voice(unsigned reg) {
 const auto r=reg&255;
 if((r>=0xa0&&r<=0xa8)||(r>=0xb0&&r<=0xb8)||(r>=0xc0&&r<=0xc8))return r&15;
 for(unsigned v=0;v<9;++v)for(unsigned g:{0x20,0x40,0x60,0x80,0xe0})
  if(r==g+v/3*8+v%3 || r==g+v/3*8+v%3+3)return v;
 return -1;
}
}
// The SDL device pulls fixed blocks at its own pace while the game loop
// advances in uneven steps. Output must never wait for the game: no inserted
// silence, and a request is heard within a bound that does not grow.
int main(int argc,char **argv) {try {
 std::cout<<std::unitbuf;
 check(argc==2,"usage: mmodern_fm_output_tests <game data>");
 const auto installation=XeenInstallationDetector().detect(std::filesystem::u8path(argv[1]));check(bool(installation),"No installation");
 XeenAssetSource assets(*installation);
 const auto resource=std::make_shared<const Bytes>(assets.readArchiveResource("promus",XeenSceneArchive::IntroOnly));
 std::ifstream moduleInput(installation->uiModule->physicalPath,std::ios::binary);
 const Bytes module((std::istreambuf_iterator<char>(moduleInput)),{});
 const auto loader=[&](const std::string &name,XeenSceneArchive archive){return assets.readArchiveResource(name,archive);};

 constexpr std::uint64_t rate=49716,block=512,lead=2,second=1000000000;
 const auto blockStart=[&](std::uint64_t k){return k*block*second/rate;};
 std::uint64_t pulled=0,heard=0; // Device block being rendered; its audible time.
 std::vector<std::uint64_t> fxHeard,musicHeard;
 auto source=std::make_shared<XeenFmSource>([&](const PromusDriver::Write &w) {
  const auto v=voice(w.reg);if(v<0)return;
  (v>=7?fxHeard:musicHeard).push_back(heard);
 });
 XeenAudio audio(3,resource,loader,module,0,false,{},source);
 audio.enterTitle(0);
 auto steadySource=std::make_shared<XeenFmSource>();
 XeenAudio steady(3,resource,loader,module,0,false,{},steadySource);steady.enterTitle(0);

 // Main loop: idle waits of one BIOS tick, a 300 ms stall every 4 s (map
 // load or a long redraw), an effect every 5 s, Music off at 58 s.
 const auto tick=PresentationClock::after(0,1);
 std::vector<std::uint64_t> effects;std::uint64_t musicOff=0;
 std::uint64_t game=0,nextEffect=5*second,nextStall=4*second,inserted=0,insertedLate=0;
 std::vector<std::int16_t> pcm(block*2),steadyPcm(block*2);
 while(game<62*second) {
  if(blockStart(pulled)<=game) {
   heard=blockStart(pulled+lead);
   const auto before=source->samples();source->pull(pcm.data(),block);
   source->check();
   if(effects.empty()) {
    steady.advance(blockStart(pulled));steadySource->pull(steadyPcm.data(),block);steadySource->check();
    check(pcm==steadyPcm,"A game-loop stall changed uninterrupted music PCM");
   }
   const auto silent=block-(source->samples()-before);
   inserted+=silent;if(blockStart(pulled)>=second)insertedLate+=silent;
   ++pulled;continue;
  }
  audio.advance(game);
  if(game>=nextEffect){audio.effect(20,game);effects.push_back(game);nextEffect+=5*second;}
  if(!musicOff && game>=58*second){audio.toggleMusic(false,game);musicOff=game;}
  game+=tick;
  if(game>=nextStall){game+=300000000;nextStall+=4*second;}
 }
 // Each effect is a burst of voice 7/8 writes; bursts are 5 s apart.
 // Song setup may touch voices 7/8 before the first effect request.
 std::vector<std::uint64_t> bursts;
 for(std::size_t i=0;i<fxHeard.size();++i)if(fxHeard[i]>=effects.at(0) && (bursts.empty() || fxHeard[i]-fxHeard[i-1]>second))bursts.push_back(fxHeard[i]);
 std::uint64_t worst=0;
 for(std::size_t i=0;i<std::min(bursts.size(),effects.size());++i) {
  const auto latency=bursts[i]>effects[i]?bursts[i]-effects[i]:0;worst=std::max(worst,latency);
  std::cout<<"effect "<<i<<" requested "<<effects[i]/1000000<<" ms heard "<<bursts[i]/1000000<<" ms latency "<<latency/1000000<<" ms\n";
 }
 const auto musicLatency=!musicHeard.empty() && musicHeard.back()>musicOff?musicHeard.back()-musicOff:0;
 std::cout<<"inserted silence "<<inserted*1000/rate<<" ms (after 1 s: "<<insertedLate*1000/rate<<" ms); effects "<<effects.size()
  <<" bursts "<<bursts.size()<<" worst latency "<<worst/1000000<<" ms; Music off heard after "<<musicLatency/1000000<<" ms\n";
 // Bound: device lead, one block until the request is taken, one driver tick.
 const auto bound=blockStart(lead+1)+PromusDriver::clockBoundary(1)+1000000;
 check(inserted==0,"The device output waited for the game loop (inserted silence)");
 check(bursts.size()==effects.size(),"Every effect request must be heard once");
 check(worst<=bound,"Effect latency exceeds the bound");
 check(musicLatency<=bound,"Music off latency exceeds the bound");
 for(const auto time:fxHeard)if(time>=effects.front())
  check(std::any_of(effects.begin(),effects.end(),[&](auto request){return time>=request && time<=request+2*second;}),
        "Unexpected effect-voice writes between effects");
 check(!musicHeard.empty() && musicHeard.back()>=musicOff,"Music off was not rendered");

 // A distinct voice identifies each synthetic song without relying on PCM
 // thresholds. Exercise the owner's pending completion, including title exit.
 for(bool ahead:{false,true})for(bool titleExit:{false,true}) {
  std::vector<PromusDriver::Write> rendered;
  auto output=std::make_shared<XeenFmSource>([&](const auto &w){rendered.push_back(w);});
  const auto songs=[](const std::string &name,XeenSceneArchive) {
   return name=="newbrigh.m"?Bytes{0x90,1,0,0x10,200,0xff}:Bytes{0x91,2,0,0x10,200,0xff};
  };
  XeenAudio route(3,resource,songs,module,0,false,{},output);route.enterTitle(0);
  const auto to=[&](std::uint64_t target) {
   while(output->samples()<target)output->pull(pcm.data(),std::min<std::uint64_t>(block,target-output->samples()));
   output->pull(pcm.data(),0);output->check();
  };
  to(PromusDriver::sampleBoundary(10));
  if(titleExit){route.leaveTitle(PromusDriver::clockBoundary(10));route.requestInn(PromusDriver::clockBoundary(10));}
  else route.requestSong("town.m",XeenSceneArchive::Clouds,207,PromusDriver::clockBoundary(10));
  to(output->samples()); // Apply fade at tick 10.
  if(ahead)to(PromusDriver::sampleBoundary(200));
  route.advance(PromusDriver::clockBoundary(200));
  check(route.loadedName()==(titleExit?"twnnitea.m":"town.m"),"Timeline completion changed");
  const auto start=rendered.size();to(output->samples());
  if(!ahead)check(rendered.size()==start,"Pending completion bypassed the render fade");
  else if(titleExit)check(rendered.size()==start+28,"Completed render fade held title-exit stop");
  to(PromusDriver::sampleBoundary(ahead?201:91));
  std::size_t keyoffs=0;std::uint64_t fadeEnd=0,newNote=0,stop=0;
  for(const auto &w:rendered) {
   if(w.interrupt && (w.reg&255)>=0xb0 && (w.reg&255)<=0xb6 && w.value==0){++keyoffs;fadeEnd=w.tick;}
   if(!w.interrupt && w.reg==0xb0 && w.value==0)stop=w.tick;
   if(w.interrupt && w.reg==0xb1 && (w.value&32))newNote=w.tick;
  }
  check(keyoffs==14 && fadeEnd==90,"Fade must key off every music voice in both banks");
  if(titleExit)check(stop==(ahead?200:90) && newNote==0,"Title preload/stop ordering changed");
  else check(newNote==(ahead?201:91),"New song must start after fade key-offs, with no hold when ahead");
  if(titleExit) {
   // The after-title Inn fade must start only after stop/preload, then its
   // pending load has its own barrier if the timeline gets ahead again.
   // The retained phase (80*207 mod 256) makes this Inn fade 127 ticks.
   route.advance(PromusDriver::clockBoundary(400));
   to(PromusDriver::sampleBoundary(ahead?328:218));
   check(std::any_of(rendered.begin(),rendered.end(),[&](const auto &w){return w.reg==0xb1 && (w.value&32) && w.tick==(ahead?328:218);}),
         "After-title fade command escaped its completion barrier");
  }
 }
 // Rendering ahead of, or behind, the same presentation script must leave
 // every selected transport's logical clock, busy and song boundary intact.
 for(const auto pair:std::vector<std::pair<unsigned,unsigned>>{{3,6},{0,0},{0,6},{1,7},{4,10},{5,0}}) {
  std::vector<std::uint64_t> snapshots[2];std::string loaded[2];
  for(unsigned ahead=0;ahead<2;++ahead) {
   auto source=std::make_shared<XeenFmSource>();XeenAudioConfig config;config.music=pair.first;config.digital=pair.second;
   const auto mixedLoader=[&](const std::string &name,XeenSceneArchive archive) {
    if(name=="boundary.voc") {Bytes b(32+512,160);b[27]=std::uint8_t(521);b[28]=521>>8;return b;}
    return loader(name,archive);
   };
   XeenAudio route(config,pair.first==3?resource:PromusDriver::Bytes{},mixedLoader,module,0,false,{},source);
   route.enterTitle(0);route.startDigital("boundary.voc",XeenSceneArchive::Clouds,0);
   route.requestSong("town.m",XeenSceneArchive::Clouds,207,0);
   check(route.prepareRender(48000),"Generalized boundary renderer preparation");
   const auto renderSecond=[&] {for(unsigned n=0;n<48000;n+=block)route.render(pcm.data(),std::min<unsigned>(block,48000-n),48000);};
   if(ahead)renderSecond();
   for(std::uint64_t now:{0ULL,10000000ULL,200000000ULL,3000000000ULL}) {
    route.advance(now);snapshots[ahead].push_back(route.ticks());snapshots[ahead].push_back(route.samples());
    snapshots[ahead].push_back(route.digitalBusy(now));snapshots[ahead].push_back(route.digitalDeadline(now).value_or(0));
    snapshots[ahead].push_back(route.pending());snapshots[ahead].push_back(route.deadline().value_or(0));
   }
   if(!ahead)renderSecond();loaded[ahead]=route.loadedName();
  }
  check(snapshots[0]==snapshots[1] && loaded[0]==loaded[1],"Render lead changed generalized logical boundary");
 }
 {
  // Only pending completion is gated: explicit Off cancels a live fade;
  // title entry and Music On restart immediately, even during that fade.
  for(unsigned action=0;action<3;++action) {
   std::vector<PromusDriver::Write> writes;
   auto output=std::make_shared<XeenFmSource>([&](const auto &w){writes.push_back(w);});
   XeenAudio route(3,resource,loader,module,0,false,{},output);route.enterTitle(0);
   output->pull(pcm.data(),block);route.requestInn(0);output->pull(pcm.data(),0);
   if(action==0)route.toggleMusic(false,0);
   else if(action==1)route.toggleMusic(true,0);
   else route.enterTitle(0);
   const auto before=writes.size();output->pull(pcm.data(),0);output->check();
   if(action==0)check(writes.size()==before+28,"Music off waited for a live fade");
   output->pull(pcm.data(),block);output->check();
   if(action)check(std::any_of(writes.begin()+before,writes.end(),[](const auto &w){return w.interrupt && (w.reg&255)>=0xb0 && (w.reg&255)<=0xb6 && (w.value&32);}),"Restart waited for a live fade");
  }
 }
 {
  // A pathological fade cannot block later operations forever.
  std::vector<PromusDriver::Write> writes;
  XeenFmSource observed([&](const auto &w){writes.push_back(w);});observed.initialize(resource);
  observed.command(2);observed.fadeBarrier();observed.effect(20);
  while(observed.samples()<PromusDriver::sampleBoundary(256)+1)
   observed.pull(pcm.data(),std::min<std::uint64_t>(block,PromusDriver::sampleBoundary(256)+1-observed.samples()));
  observed.check();
  check(std::any_of(writes.begin(),writes.end(),[](const auto &w){return !w.interrupt && voice(w.reg)>=7 && w.tick==256;}),"Fade barrier exceeded its defensive limit");
 }
 {
  XeenFmSource broken;broken.initialize(resource);broken.load(std::make_shared<const Bytes>(Bytes{0x90}),true);
  broken.pull(pcm.data(),block);broken.pull(pcm.data(),block);
  check(std::all_of(pcm.begin(),pcm.end(),[](auto v){return v==0;}),"Failed callback was not silenced");
  bool failed=false;try{broken.check();}catch(const std::runtime_error &){failed=true;}
  check(failed,"Render exception did not reach check()");
  std::fill(pcm.begin(),pcm.end(),1);broken.pull(pcm.data(),block);
  check(std::all_of(pcm.begin(),pcm.end(),[](auto v){return v==0;}),"Failed source resumed rendering");
 }
 for(unsigned id:{0,1,10}) {
  unsigned writes=0;auto output=std::make_shared<XeenFmSource>([&](const auto &){++writes;});
  XeenAudio silent(id,{},loader,module,0,false,{},output);silent.enterTitle(0);silent.effect(20,0);output->pull(pcm.data(),block);
  check(writes==0 && std::all_of(pcm.begin(),pcm.end(),[](auto v){return v==0;}),"Silent device created a render driver");
 }
 // A missing song stays pending: an effect's advance may swallow the error,
 // but the next ordinary advance must still report it (DOS exits).
 {
  const auto missing=[&](const std::string &name,XeenSceneArchive archive){return name=="absent.m"?Bytes{}:assets.readArchiveResource(name,archive);};
  XeenAudio broken(3,resource,missing,module,0,false);
  broken.requestSong("absent.m",XeenSceneArchive::Clouds,207,0);
  try {broken.effect(20,10*second);} catch(...) {}
  bool reported=false;
  try {broken.advance(11*second);} catch(const std::runtime_error &e) {reported=std::string(e.what()).find("absent.m")!=std::string::npos;}
  check(reported && broken.pending(),"A missing song was lost instead of reported");
 }
 std::cout<<"FM output pacing passed\n";return 0;
}catch(const std::exception &e){std::cerr<<e.what()<<'\n';return 1;}}
