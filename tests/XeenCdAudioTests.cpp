#include "formats/xeen/XeenCdAudio.h"
#include "games/xeen/XeenInstallationDetector.h"
#include "app/XeenAudio.h"
#include "app/XeenFmSource.h"
#include "app/XeenEventFlow.h"
#include "games/xeen/XeenPartyLoader.h"
#include "games/xeen/XeenEventLoader.h"
#include "games/xeen/XeenEventTextLoader.h"
#include "XeenRemoveTestSupport.h"
#include "formats/xeen/XeenAssetSource.h"
#include "XeenTestInstallation.h"
#include "platform/sdl/SdlAudioDevice.h"
#define SDL_MAIN_HANDLED
#include <SDL.h>
#include <algorithm>
#include <chrono>
#include <fstream>
#include <iostream>
#include <thread>
#include <future>
#include <cmath>
#include <atomic>
using namespace mmodern;
namespace mmodern {
struct XeenCdAudioTestAccess {static bool pending(const XeenAudio &audio){return audio.cdDecodePending();}};
}
namespace {SDL_AudioDeviceID openedDevice=0;bool failOpen=false;std::atomic<bool> conversionError{false};}
extern "C" SDL_AudioDeviceID __real_SDL_OpenAudioDevice(const char *,int,const SDL_AudioSpec *,SDL_AudioSpec *,int);
extern "C" SDL_AudioDeviceID __wrap_SDL_OpenAudioDevice(const char *name,int capture,const SDL_AudioSpec *want,SDL_AudioSpec *have,int changes) {
 if(failOpen){SDL_SetError("injected reopen failure");return 0;}
 openedDevice=__real_SDL_OpenAudioDevice(name,capture,want,have,changes);return openedDevice;
}
extern "C" int __real_SDL_AudioStreamGet(SDL_AudioStream *,void *,int);
extern "C" int __wrap_SDL_AudioStreamGet(SDL_AudioStream *stream,void *out,int bytes) {
 if(conversionError.exchange(false)){SDL_SetError("injected conversion failure");return -1;}
 return __real_SDL_AudioStreamGet(stream,out,bytes);
}
namespace {
void check(bool value,const char *message) {if(!value)throw std::runtime_error(message);}
void decoded(XeenAudio &audio) {
 const auto limit=std::chrono::steady_clock::now()+std::chrono::seconds(5);
 while(XeenCdAudioTestAccess::pending(audio) && std::chrono::steady_clock::now()<limit)std::this_thread::yield();
 check(!XeenCdAudioTestAccess::pending(audio),"CD worker did not finish");
}
template<class F> void rejects(F f) {bool failed=false;try{f();}catch(const std::exception &){failed=true;}check(failed,"Invalid input was accepted");}
void failurePolling(const GameInstallation &installation,XeenAssetSource &assets,const std::vector<XeenCdExtent> &extents,
                    const std::vector<std::uint8_t> &module) {
 const XeenFontFormat font(assets.readArchiveResource("fnt"));
 const auto loader=[&](const auto &name,auto archive){return assets.readArchiveResource(name,archive);};
 const auto driver=std::make_shared<const std::vector<std::uint8_t>>(loader("promus",XeenSceneArchive::IntroOnly));
 std::vector<std::uint64_t> expected;std::vector<PromusDriver::Write> expectedWrites;
 for(unsigned failure=0;failure<6;++failure) {
  std::atomic<bool> callbackError{false};std::vector<PromusDriver::Write> writes;
  auto fm=std::make_shared<XeenFmSource>([&](const auto &){if(callbackError.exchange(false))throw std::runtime_error("injected callback failure");});
  XeenProgramState program{mmodern::XeenWallClock::fixed(0)};
  program.audio=std::make_shared<XeenAudio>(3,driver,loader,module,0,failure!=5,[&](const auto &w){writes.push_back(w);},fm);
  program.audio->cdTracks(extents);program.audio->enterTitle(0);
  auto party=XeenPartyLoader().loadInitialCloudsParty(assets);auto options=XeenSoundOptions{};options.text=false;party.soundOptions=options;
  XeenCamera camera{1,1,1,XeenDirection::North};XeenGameFlags flags;
  XeenWorld world([](auto id){return remove_test::map(id);});
  const auto script=remove_test::script(1,{remove_test::record(1,1,0,60,{19,114,1,113,8}),
    remove_test::record(1,1,1,5,{0,1,9,1,99}),remove_test::record(1,1,2,0x12)});
  XeenEventSystem events([&](auto){return script;},[](auto){return XeenEventTextFile{1,"test.txt",true,{"Title","Body"}};});
  IndexedFrame base(320,200,std::vector<std::uint8_t>(64000,90));std::uint64_t now=0;unsigned random=0;
  XeenEventFlow flow(program,world,events,party,camera,flags,font,
    [&](auto){return XeenEventFlow::Composition{base,false};},[](auto &,auto,auto){},{},[&]{++random;return 2;},
    nullptr,{},nullptr,{},&assets.uiText(),[&]{return now;});
  flow.drawDialogSprite=[](auto &,const char *,unsigned,int,int){};flow.handle(InteractionAction{});
  decoded(*program.audio);flow.framePresented(flow.frame().presentation());
  const auto end=std::chrono::steady_clock::now()+std::chrono::seconds(5);
  if(failure>0 && failure<5) {
   while(!fm->samples() && std::chrono::steady_clock::now()<end)std::this_thread::yield();
   check(fm->samples()>0,"Failure witness callback never started");
   if(failure==1)callbackError=true;
   if(failure==2)conversionError=true;
   if(failure>=3){failOpen=failure==4;SdlAudioDevice::removed(openedDevice);failOpen=false;}
   if(failure!=3) {
    while(program.audio->outputActive() && std::chrono::steady_clock::now()<end)std::this_thread::yield();
    check(!program.audio->outputActive(),"Injected failure was not observed");
   }
  }
  std::vector<std::uint64_t> trace;
  for(unsigned tick:{1,2,3,5,10,20}) {
   now=PresentationClock::after(0,tick);program.audio->advance(now);
   // A notice may publish a cosmetic fallback frame. Servicing the same
   // timestamp again must retain the very same NPC animation/RNG schedule.
   for(unsigned cycle=0;cycle<2;++cycle)if(flow.updatePresentation())flow.framePresented(flow.frame().presentation());
   trace.insert(trace.end(),{random,program.audio->cdDeadline(now).value_or(0),program.audio->samples(),program.audio->ticks(),flow.presenter().npcTiming().deadline});
  }
  if(failure>0 && failure<5)check(flow.presenter().window11Enabled(),"Output loss did not restore Text-off NPC body");
  flow.respond(*flow.presentationGeneration(),XeenPresentationResponse::Acknowledged);
  trace.push_back(program.audio->cdBusy(now));trace.push_back(random);
  if(!failure){expected=trace;expectedWrites=writes;}
  else {
   check(trace==expected,"Output failure changed gameplay timing or RNG trace");
   check(writes.size()==expectedWrites.size() && std::equal(writes.begin(),writes.end(),expectedWrites.begin(),[](const auto &a,const auto &b){return a.tick==b.tick && a.reg==b.reg && a.value==b.value && a.interrupt==b.interrupt;}),"Output failure changed logical FM trace");
  }
 }
}
void npcPolling(const GameInstallation &installation,XeenAssetSource &assets,
                const std::vector<XeenCdExtent> &extents) {
 const XeenFontFormat font(assets.readArchiveResource("fnt"));
 for(bool device:{false,true})for(bool text:{false,true})for(unsigned mode:{1,2})for(bool missing:{false,true}) {
  XeenProgramState program{mmodern::XeenWallClock::fixed(0)};
  program.audio=XeenAudio::installed(installation,assets,0,device);
  program.audio->cdTracks(missing?std::vector<XeenCdExtent>{}:extents);
  check(program.audio->outputActive()==device,"Requested dummy device did not open");
  auto party=XeenPartyLoader().loadInitialCloudsParty(assets);
  auto options=XeenSoundOptions(party.soundOptions);options.text=text;options.speech=true;party.soundOptions=options;
  XeenCamera camera{1,1,1,XeenDirection::North};XeenGameFlags flags;
  XeenWorld world([](auto id){return remove_test::map(id);});
  auto npc=remove_test::record(1,1,1,5,{0,1,9,std::uint8_t(mode)});
  if(mode==1)npc.parameters.push_back(99);
  const auto script=remove_test::script(1,{remove_test::record(1,1,0,60,{19,114,1,113,8}),npc,
      remove_test::record(1,1,2,0x20,{1,0}),remove_test::record(1,1,3,0x12)});
  XeenEventSystem events([&](auto){return script;},[](auto){return XeenEventTextFile{1,"test.txt",true,{"Title","Body"}};});
  IndexedFrame base;base.width=320;base.height=200;base.pixels.assign(64000,90);
  std::uint64_t now=0;unsigned random=0;
  XeenEventFlow flow(program,world,events,party,camera,flags,font,
      [&](auto){return XeenEventFlow::Composition{base,false};},[](auto &,auto,auto){},
      {},[&]{++random;return 2;},nullptr,{},nullptr,{},&assets.uiText(),[&]{return now;});
  flow.drawDialogSprite=[](auto &,const char *,unsigned,int,int){};
  flow.handle(InteractionAction{});
  check(flow.blocksGameplay(),"NPC polling witness did not suspend");
  const bool ordinaryHidden=!text && mode==1;
  check(random==(ordinaryHidden?1:0),"Text-off ordinary NPC must poll busy before its first hold; mode2 must not");
  flow.framePresented(flow.frame().presentation());
  now=PresentationClock::after(0,1);
  if(flow.updatePresentation())flow.framePresented(flow.frame().presentation());
  check(random==(ordinaryHidden?2:0),"One-tick polling leaked into Text-on or passive helper");
  if(mode==2) {
   now=PresentationClock::after(0,3);
   if(flow.updatePresentation())flow.framePresented(flow.frame().presentation());
   check(random==1,"Passive NPC helper must retain three-tick polling");
   flow.respond(*flow.presentationGeneration(),CharacterSelectionCancelled{});
   check(!program.audio->cdBusy(now),"WhoWill exit must stop process CD playback");
  }
 }
}

void fallbackAndReaders(const GameInstallation &installation,XeenAssetSource &assets,
                        const std::vector<XeenCdExtent> &extents) {
 const XeenFontFormat font(assets.readArchiveResource("fnt"));
 for(bool text:{false,true})for(bool speech:{false,true})for(unsigned source:{0,1,2,3,4,5}) {
  XeenProgramState program{mmodern::XeenWallClock::fixed(0)};program.audio=XeenAudio::installed(installation,assets,0,false);
  auto media=extents;
  if(source==1)media.clear(); // Synthetic plain GAME source has no track table.
  if(source==2)media[18].error="Missing CD audio: synthetic missing file";
  if(source==3)media[18].error="Undecodable CD audio: synthetic invalid file";
  if(source==5)media[18].track.file="missing-after-inspection.ogg";
  program.audio->cdTracks(media);
  auto party=XeenPartyLoader().loadInitialCloudsParty(assets);
  auto options=XeenSoundOptions(party.soundOptions);options.text=text;options.speech=speech;party.soundOptions=options;
  XeenCamera camera{1,1,1,XeenDirection::North};XeenGameFlags flags;
  // Part 0: isolate manual audio presentations; flagged passes are covered by A1.
  XeenWorld world([](auto id){auto m=remove_test::map(id);m.geometry.cells[17].rawAttributes=0;return m;});
  const auto cue=source==4?std::vector<std::uint8_t>{2,24,0,160,1}:std::vector<std::uint8_t>{19,114,1,113,8};
  auto script=remove_test::script(1,{remove_test::record(1,1,0,60,cue),
      remove_test::record(1,1,1,5,{0,1,9,1,99}),remove_test::record(1,1,2,0x12)});
  XeenEventSystem events([&](auto){return script;},[](auto){return XeenEventTextFile{1,"test.txt",true,{"Title","Body remains readable"}};});
  IndexedFrame base;base.width=320;base.height=200;base.pixels.assign(64000,90);
  std::uint64_t now=0;
  XeenEventFlow flow(program,world,events,party,camera,flags,font,
      [&](auto){return XeenEventFlow::Composition{base,false};},[](auto &,auto,auto){},
      {},[]{return 2;},nullptr,{},nullptr,{},&assets.uiText(),[&]{return now;});
  flow.drawDialogSprite=[](auto &,const char *,unsigned,int,int){};
  flow.handle(InteractionAction{});
  if(source==5 && speech) {decoded(*program.audio);flow.framePresented(flow.frame().presentation());flow.updatePresentation();}
  check(flow.blocksGameplay() && flow.presenter().window11Enabled()==(text || !speech || source!=0),
        "NPC Text-off body/fallback reader differs");
  check(program.audio->cdBusy(now)==(speech && source!=4),"Unavailable cue lost its timing or refusal created a wait");
  flow.framePresented(flow.frame().presentation());
  flow.respond(*flow.presentationGeneration(),XeenPresentationResponse::Acknowledged);
  check(!program.audio->cdBusy(now) && program.audio->retainedCdCue().has_value(),"NPC dismissal must stop without clearing operands");
  // Event messages retain their text independently of the NPC Text-off byte.
  for(unsigned retry=0;retry<4 && flow.blocksGameplay() && !flow.presentationGeneration();++retry){flow.framePresented(flow.frame().presentation());flow.updatePresentation();} // Retire the previous input pass.
  script=remove_test::script(1,{remove_test::record(1,1,0,0x35,{1}),remove_test::record(1,1,1,0x12)});
   events.discardScriptCache();flow.handle(InteractionAction{false}); // Part 0: click preserves passive text; Space continues as Wait.
  check(flow.frame().pixels!=base.pixels,"Text off hid an ordinary Event message");
 }
}
void npcGeometry(XeenAssetSource &assets) {
 const XeenFontFormat font(assets.readArchiveResource("fnt"));
 IndexedFrame base;base.width=320;base.height=200;base.pixels.assign(64000,90);
 XeenEventPresenter presenter(font,[](auto &,auto,auto){});
 struct Draw {std::string name;unsigned frame;int x,y;};std::vector<Draw> draws;
 presenter.originalAssets(&assets.uiText(),[&](auto &image,const char *name,unsigned frame,int x,int y){
  draws.push_back({name,frame,x,y});
  if(std::string(name)=="global.icn")for(int row=74;row<138;++row)
   std::fill_n(image.pixels.begin()+row*320+232,80,77);
 });
 XeenPresentationRequest request;request.kind=XeenPresentationKind::NpcAcknowledgment;
 XeenEventTextLoader textLoader([&](const std::string &name)->std::optional<std::vector<std::uint8_t>> {
  if(!assets.hasArchiveResource(name))return {};return assets.readArchiveResource(name);
 });
 const auto titles=textLoader.load(28).strings;
 const auto title=std::find_if(titles.begin(),titles.end(),[](const auto &s){return s.find("Gunther")==0;});
 check(title!=titles.end(),"Original Gunther title missing");
 request.title=*title;request.text="Hidden body";request.npc=XeenEventNpc{0,1,9,1,99};
 request.npcBodyHidden=true;request.response=XeenPresentationResponseRequirement::Acknowledgment;
 const auto shown=presenter.present(base,request).frame;
 // DOS window 40 table/margin: 0x3dc4e/0x3dcae/0x3dd0e/0x3dd6e/0x3de2e.
 XeenTextRenderOptions o;o.originalControls=true;o.drawWindow=true;
 o.windowBounds={226,0,319,100};o.bounds={234,8,311,92};o.x=234;o.y=8;
 XeenTextRenderer renderer(font);
 // Explicit title positions, independent of the presenter's template/wrapping:
 // 0x53b20 starts at y=64; resource newline adds ten pixels.
 const auto line=[&](const std::string &word,int relativeY) {
  const auto x=(546-renderer.textWidth(word,XeenFontSize::Normal))/2;
  const auto digits=[](int n){auto t=std::to_string(n);return std::string(3-t.size(),'0')+t;};
  return std::string("\v")+digits(relativeY)+"\t"+digits(x-234)+word;
 };
 const auto expected=renderer.render(base,"\r"+line("Gunther",56)+line("the Mayor",66),o).pages.front();
 check(shown.pixels==expected.pixels,"DOS Text-off window rectangle or centered two-line title differs");
 for(int y=0;y<200;++y)for(int x=0;x<320;++x)if(x<226 || x>=319 || y>=100)
  check(shown.pixels[y*320+x]==90,"Text-off NPC overwrote scene, party bar or remaining buttons");
 check(draws.size()==2 && draws[0].name=="frame.fac" && draws[0].x==248 && draws[0].y==12 &&
  draws[1].name=="face09.fac" && draws[1].x==255 && draws[1].y==18,"DOS Text-off portrait coordinates differ");
 auto input=presenter.input();
 check(input.key(InputKey::Escape).has_value() && input.key(InputKey::Enter).has_value() &&
  input.click(270,65).has_value() && input.click(20,170).has_value() && input.click(0,0).has_value(),
  "Compact acknowledgment lost DOS any-key/any-mouse dismissal");
 presenter.clear();draws.clear();request.npc->confirmationMode=0;request.response=XeenPresentationResponseRequirement::YesNo;
 presenter.present(base,request);input=presenter.input();
 for(int y=0;y<100;++y)for(int x=226;x<319;++x)
  check(presenter.frame().pixels[y*320+x]==shown.pixels[y*320+x],"YesNo backing overwrote compact NPC title/portrait");
 check(input.hits.size()==2 && input.hits[0].left==235 && input.hits[0].top==101 &&
  input.hits[1].left==260 && input.hits[1].top==101 && input.click(235,101).has_value() &&
  input.click(283,120).has_value() && !input.click(235,75) && !input.click(284,121) &&
  input.key('y').has_value() && input.key('n').has_value() && input.key(InputKey::Escape).has_value(),
  "Compact YesNo keyboard or mouse hit rectangles differ from DOS");
 check(draws.size()==5 && draws[3].name=="confirm.icn" && draws[3].x==235 && draws[3].y==101 &&
  draws[4].x==260 && draws[4].y==101,"YesNo sprites do not match hit rectangles");
}
void originalCues(XeenAssetSource &assets,const std::vector<XeenCdExtent> &extents) {
 XeenEventLoader loader([&](const std::string &name)->std::optional<std::vector<std::uint8_t>> {
  if(!assets.hasInitialResource(name))return {};return assets.readInitialResource(name);
 });
 unsigned refused=0,crossed=0,gunther=0,decoded=0;
 std::chrono::microseconds longest{};
 for(unsigned map:{28,30,49})for(const auto &r:loader.load(map).records)if(r.opcode==60) {
  check(r.parameters.size()==5,"Original PlayCD operand width");
  const XeenEventCdSpeech cue{r.parameters[0],std::uint16_t(r.parameters[1]|r.parameters[2]<<8),std::uint16_t(r.parameters[3]|r.parameters[4]<<8)};
  const auto request=mapXeenCdCue(extents,1,cue);
  if(request.refused){check(map==30 && cue.track==2 && cue.start==24 && cue.finish==416,"Unexpected original refusal");++refused;continue;}
  check(request.error.empty(),"Original cue failed absolute mapping");
  const auto start=std::chrono::steady_clock::now();const auto pcm=decodeXeenCdCue(request);
  longest=std::max(longest,std::chrono::duration_cast<std::chrono::microseconds>(std::chrono::steady_clock::now()-start));++decoded;
  check(!pcm.empty(),"Original cue decoded no PCM");
  if(map==49 && request.windows.size()==2){++crossed;check(request.duration==5053333334ULL,"Original map49 duration");}
  if(map==28 && r.x==14 && r.y==5 && r.direction==3 && cue.start==370){++gunther;check(cue.track==19 && cue.start==370 && cue.finish==2161,"Original Gunther operands");}
 }
 check(refused==2 && crossed==1 && gunther==1,"Original Gunther/map30/map49 inventory changed");
 std::cout<<"Original cues decoded: "<<decoded<<"; longest bounded decode microseconds: "<<longest.count()<<'\n';
}

}
int main(int argc,char **argv) {try {
 check(argc==4,"usage: cd_audio_tests <GOG directory> <scratch> <synthetic OGG>");
 const auto root=std::filesystem::u8path(argv[1]),scratch=std::filesystem::u8path(argv[2]);
 std::filesystem::create_directories(scratch);
 auto tracks=readXeenCdTracks(root/"GAME1.INS",1);
 const auto disc2=readXeenCdTracks(root/"GAME2.INS",2);
 check(tracks.size()==31 && disc2.size()==30,"Both discs must retain all tracks");
 check(tracks[20].number==21 && tracks[20].audio,"TRACK 21AUDIO rejected");
 check(tracks[1].number==disc2[1].number && tracks[1].file!=disc2[1].file,"Repeated track numbers lost disc identity");
 tracks.insert(tracks.end(),disc2.begin(),disc2.end());
 const auto extents=inspectXeenCdTracks(tracks);
 for(const auto &extent:extents) {
  if(!extent.error.empty())throw std::runtime_error(extent.error);
  check(extent.start.has_value(),"Cumulative absolute address absent");
  if(!extent.track.audio)continue;
  // Validate the complete original stream, not only a header or one witness.
  for(std::uint64_t first=0;first<extent.samples;first+=60*44100) {
   const auto count=std::min<std::uint64_t>(60*44100,extent.samples-first);
   XeenCdRequest request;request.windows.push_back({extent.track.file,first,count});
   check(decodeXeenCdCue(request).size()==count*2,"Original track is truncated");
  }
 }
 auto gunther=mapXeenCdCue(extents,1,{19,370,2161});
 check(gunther.error.empty() && !gunther.refused && gunther.windows.size()==1,"Gunther mapping failed");
 check(gunther.windows[0].first==158760 && gunther.windows[0].count==803208,"Gunther must decode 3.6 through 21.813333 seconds");
 check(gunther.duration==18213333334ULL,"Gunther logical duration differs");
 check(gunther.absoluteStart==*extents[18].start+270,"Absolute MSF bias/preroll differs");
 const auto began=std::chrono::steady_clock::now();const auto pcm=decodeXeenCdCue(gunther);
 std::cout<<"Gunther cue decode milliseconds: "<<std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now()-began).count()<<'\n';
 check(std::any_of(pcm.begin(),pcm.end(),[](auto s){return s!=0;}),"Gunther PCM is silent");
 auto cross=mapXeenCdCue(extents,1,{3,20,474});
 check(cross.error.empty() && cross.windows.size()==2 && cross.duration==5053333334ULL,"Map 49 cross-file request differs");
 check(cross.windows[0].first==6767292 && cross.windows[0].count==2940 && cross.windows[1].first==0 && cross.windows[1].count==219912,"Map 49 file sample positions differ");
 check(decodeXeenCdCue(cross).size()==379*588*2,"Map 49 duration lost at EOF");
 // Synthetic disc extents use real decodable files but independent short
 // sector boundaries, exposing absolute-start traversal and early EOF.
 auto shortDisc=std::vector<XeenCdExtent>{extents[0],extents[1],extents[2]};
 shortDisc[0].start=0;shortDisc[0].sectors=100;
 shortDisc[1].start=100;shortDisc[1].sectors=100;
 shortDisc[2].start=200;shortDisc[2].sectors=100;
 auto later=mapXeenCdCue(shortDisc,1,{2,200,250}); // start125 sectors after T
 check(later.windows.size()==1 && later.windows[0].file==shortDisc[2].track.file && later.windows[0].first==25*588,
       "Absolute start was incorrectly restricted to named track");
 auto earlyEof=mapXeenCdCue(shortDisc,1,{2,130,200}); // starts80, plays20, remaining50
 check(earlyEof.error.empty() && earlyEof.windows.size()==1 && earlyEof.windows[0].count==20*588 && earlyEof.duration==933333334ULL,
       "Short EOF remainder discarded the playable tail or changed logical timing");
 check(decodeXeenCdCue(earlyEof).size()==20*588*2,"Short EOF tail was not decoded");
 auto finalEof=mapXeenCdCue(shortDisc,1,{3,130,400}); // starts80, plays20, remaining200
 check(finalEof.error.empty() && finalEof.windows.size()==1 && finalEof.windows[0].count==20*588 && finalEof.duration==2933333334ULL,
       "Disc lead-out discarded a started cue's prefix or changed logical timing");
 check(decodeXeenCdCue(finalEof).size()==20*588*2,"Final audio track tail was not decoded");
 check(mapXeenCdCue(shortDisc,1,{3,300,400}).refused,"Lead-out start must be refused");
 // A named data track can address audio; only the absolute start type matters.
 auto fromData=mapXeenCdCue(shortDisc,1,{1,200,250});
 check(!fromData.refused && fromData.error.empty() && fromData.windows[0].file==shortDisc[1].track.file,
       "Refusal used the named track instead of the absolute starting track");
 check(!mapXeenCdCue({},1,{1,200,250}).refused,"Unknown disc layout guessed named-track refusal");
 // Synthetic 600-frame stereo silence, generated without any game data:
 // ffmpeg -f lavfi -i anullsrc=r=44100:cl=stereo -af atrim=end_sample=600 -c:a libvorbis -q:a 2 cd-silence.ogg
 const auto partialTrack=inspectXeenCdTracks({{1,2,true,std::filesystem::u8path(argv[3]),0}}).at(0);
 check(partialTrack.error.empty() && partialTrack.samples==600 && partialTrack.sectors==2,"Synthetic partial-sector fixture");
 XeenCdRequest partial;partial.windows.push_back({partialTrack.track.file,600,576});
 const auto silence=decodeXeenCdCue(partial);
 check(silence.size()==1152 && std::all_of(silence.begin(),silence.end(),[](auto v){return v==0;}),"Rounded partial sector was not silent");
 partial.windows[0].first=599;partial.windows[0].count=578;
 rejects([&]{decodeXeenCdCue(partial);});
 const auto refused=mapXeenCdCue(extents,1,{2,24,416});
 check(refused.refused && !refused.duration && refused.windows.empty(),"Map 30 must refuse without wait");
 check(mapXeenCdCue({},1,{2,24,416}).refused,"Plain GAME changed refused-start rule");
 const auto missing=mapXeenCdCue({},1,{19,370,2161});
 check(!missing.error.empty() && missing.duration==gunther.duration,"Plain GAME fallback lost timing");
 rejects([&]{decodeXeenCdCue(missing);});
 auto damaged=tracks;damaged[18].file=scratch/"missing.ogg";
 auto absent=inspectXeenCdTracks(damaged);
 check(!mapXeenCdCue(absent,1,{19,370,2161}).error.empty(),"Missing track accepted");
 {std::ofstream file(scratch/"broken.ogg",std::ios::binary);file<<"not Vorbis";}
 damaged[18].file=scratch/"broken.ogg";
 check(!mapXeenCdCue(inspectXeenCdTracks(damaged),1,{19,370,2161}).error.empty(),"Malformed track accepted");
 auto beyond=gunther;beyond.windows[0].first=extents[18].sectors*588;
 rejects([&]{decodeXeenCdCue(beyond);});
 beyond.windows[0].first=extents[18].samples-1;
 beyond.windows[0].count=extents[18].sectors*588-beyond.windows[0].first+1;
 rejects([&]{decodeXeenCdCue(beyond);});
 for(const auto &index:{"00:60:00","00:00:75"}) {
  {std::ofstream cue(scratch/"bad.ins");cue<<"FILE \"broken.ogg\" MP3\n TRACK 02 AUDIO\n INDEX 01 "<<index<<'\n';}
  rejects([&]{readXeenCdTracks(scratch/"bad.ins",1);});
 }
 const auto installation=xeenTestInstallationDetector().detect(root);
 check(bool(installation),"No original installation");XeenAssetSource assets(*installation);
 npcPolling(*installation,assets,extents);
 fallbackAndReaders(*installation,assets,extents);
 npcGeometry(assets);
 originalCues(assets,extents);
 std::ifstream moduleFile(installation->uiModule->physicalPath,std::ios::binary);
 const std::vector<std::uint8_t> module((std::istreambuf_iterator<char>(moduleFile)),{});
 const auto loader=[&](const std::string &name,XeenSceneArchive archive){return assets.readArchiveResource(name,archive);};
 failurePolling(*installation,assets,extents,module);
 // A deliberately blocked decoder proves cue start does not wait for PCM and
 // a stopped request cannot publish after the worker completes.
 std::promise<void> release,entered;auto gate=release.get_future().share();auto started=entered.get_future();
 XeenAudio async(0,{},loader,module,100,false,{}, {},[&](const XeenCdRequest &) {
  entered.set_value(); // A second invocation deliberately throws future_error.
  gate.wait();return std::vector<std::int16_t>(44100*2,1234);
 });
 async.cdTracks(extents);async.retainCdCue({19,370,2161});async.prepareRender(44100);
 const auto accepted=async.startCd(100);
 const auto workerStarted=started.wait_for(std::chrono::seconds(5))==std::future_status::ready;
 const auto sampledDeadline=async.cdDeadline(100);
 async.stopCd(200);release.set_value();
 check(accepted && workerStarted && sampledDeadline==100+gunther.duration,"Worker blocked cue start or moved logical start");
 // A subsequent generation must finish after the cancelled decode has returned.
 // Its intentional failure also checks once-only asynchronous diagnostics.
 async.retainCdCue({19,370,2161});async.startCd(200);
 decoded(async);
 check(!async.takeCdNotice().empty() && async.takeCdNotice().empty(),"Worker error notice was lost or repeated");
 async.stopCd(200);
 std::vector<std::int16_t> stopped(512*2);async.render(stopped.data(),512,44100);
 check(!async.cdBusy(200) && std::all_of(stopped.begin(),stopped.end(),[](auto v){return v==0;}),"Cancelled worker resurrected stopped PCM");
 XeenAudio audio(0,{},loader,module,100,false);audio.cdTracks(extents);
 check(!audio.cdBusy(100) && !audio.retainedCdCue(),"Fresh process must start idle");
 audio.retainCdCue({19,370,2161});check(audio.startCd(100),"Gunther playback refused");
 check(audio.cdBusy(100+gunther.duration-1) && !audio.cdBusy(100+gunther.duration),"Busy reads the wrong clock/boundary");
 audio.advance(100+1000000000);audio.rebaseClock(900000000000ULL);
 check(audio.cdDeadline(900000000000ULL)==900000000000ULL+gunther.duration-1000000000,"Rebase lost remaining duration");
 auto options=XeenSoundOptions{};options.speech=false;audio.observeOptions(options);
 check(!audio.startCd(900000000000ULL) && audio.cdBusy(900000000000ULL),"Speech off interrupted the running cue");
 audio.selectCdDisc(2,900000000000ULL);check(audio.retainedCdCue().has_value(),"Speech-off disc check cleared operands");
 options.speech=true;audio.observeOptions(options);
 audio.requestSong("town.m",XeenSceneArchive::Clouds,207,900000000000ULL);audio.advance(900000000001ULL);
 check(audio.pending(),"Pending FM load ignored CD busy");
 audio.stopCd(900000000001ULL);audio.advance(900000000002ULL);
 check(!audio.pending() && audio.retainedCdCue().has_value(),"Stop cleared operands or retained song wait");
 check(audio.startCd(900000000002ULL),"Later NPC cue start failed");
 audio.selectCdDisc(2,900000000002ULL);
 check(!audio.cdBusy(900000000002ULL) && !audio.retainedCdCue(),"Actual disc change did not stop/clear");
 // Missing/undecodable delivery produces one consumable notice per start.
 XeenCdTrack badSheet;badSheet.disc=1;badSheet.error="Unsupported audio metadata";
 {
  XeenAudio bad(0,{},loader,module,0,false);bad.cdTracks(inspectXeenCdTracks({badSheet}));
  // Both map 30 sites issue this cue. Neither may acquire a logical wait.
  for(unsigned site=0;site<2;++site) {
   bad.retainCdCue({2,24,416});check(!bad.startCd(site) && !bad.cdBusy(site) && !bad.cdDeadline(site),"Unparsed disc metadata created map30 wait");
  }
 }
 for(auto media:{std::vector<XeenCdExtent>{},absent,inspectXeenCdTracks(damaged),inspectXeenCdTracks({badSheet})}) {
  XeenAudio unavailable(0,{},loader,module,0,false);unavailable.cdTracks(media);unavailable.retainCdCue({19,370,2161});
  check(!unavailable.startCd(0) && unavailable.cdBusy(0),"Unavailable start must keep the logical interval");
  check(!unavailable.takeCdNotice().empty() && unavailable.takeCdNotice().empty(),"Unavailable notice repeated within one cue");
 }
 // Actual title/session calls, independent clock epochs and a pending song.
 XeenAudio handoff(0,{},loader,module,100,false);handoff.cdTracks(extents);handoff.retainCdCue({19,370,2161});
 check(handoff.startCd(100),"Handoff cue did not play");
 handoff.advance(100+1000000000ULL);handoff.rebaseClock(700000000000ULL);
 handoff.enterTitle(700000000000ULL);handoff.leaveTitle(701000000000ULL);
 check(handoff.pending() && handoff.retainedCdCue().has_value(),"Title discarded process operands or skipped CD song barrier");
 handoff.rebaseClock(42);
 const auto remaining=gunther.duration-2000000000ULL;
 check(handoff.cdDeadline(42)==42+remaining,"Title/session epoch lost remaining CD duration");
 handoff.advance(42+remaining-1);check(handoff.pending(),"Song loaded before surviving CD interval ended");
 handoff.advance(42+remaining);check(!handoff.pending(),"Song stayed pending after logical CD idle");
 check(handoff.retainedCdCue().has_value(),"Elapsed playback cleared retained operands");
 XeenAudio restored(0,{},loader,module,900,false);
 check(!restored.cdBusy(900) && !restored.retainedCdCue(),"Fresh process imported passive playback state");
 for(int rate:{22050,44100,48000}) {
  std::vector<std::int16_t> expected;
  for(unsigned block:{1,127,512,2048}) {
   XeenAudio mixer(0,{},loader,module,0,false);mixer.cdTracks(extents);mixer.retainCdCue({19,370,2161});
   check(mixer.prepareRender(rate) && mixer.startCd(0),"Mixer preparation failed");
   const auto readyUntil=std::chrono::steady_clock::now()+std::chrono::seconds(5);
   while(XeenCdAudioTestAccess::pending(mixer) && std::chrono::steady_clock::now()<readyUntil)std::this_thread::yield();
   check(!XeenCdAudioTestAccess::pending(mixer),"CD worker did not finish");
   std::vector<std::int16_t> output(4096*2);
   for(std::size_t f=0;f<4096;f+=block)mixer.render(output.data()+f*2,std::min<std::size_t>(block,4096-f),rate);
   if(expected.empty())expected=output;else check(output==expected,"CD PCM depends on callback size");
   if(rate==44100) {
    const auto raw=decodeXeenCdCue(gunther);
    for(unsigned i=0;i<output.size();++i)check(std::abs(output[i]*3-int(raw[i]))<=3,"CD gain was omitted or applied twice");
   }
   mixer.stopCd(0);mixer.render(output.data(),4096,rate);
   check(std::all_of(output.begin(),output.end(),[](auto s){return s==0;}),"CD stop retained resampler tail");
  }
 }
 for(unsigned block:{1,73,512,2049}) {
  XeenAudio mixer(0,{},loader,module,0,false,{}, {},[](const auto &) {
   std::vector<std::int16_t> pcm(44100*4*2);
   for(unsigned f=0;f<44100*4;++f)pcm[f*2]=pcm[f*2+1]=std::lround(16000*std::sin(f*100.0*6.283185307179586/44100));
   return pcm;
  });
  mixer.cdTracks(extents);mixer.retainCdCue({19,370,2161});check(mixer.startCd(0),"Synthetic cue refused");decoded(mixer);
  for(int rate:{48000,44100,96000,32000,48000}) {
   check(mixer.prepareRender(rate),"Live CD rate change failed");
   unsigned largestStep=0,peak=0;int previous=0;
   for(unsigned f=0;f<5000;) {
    const auto n=std::min(block,5000-f);std::vector<std::int16_t> pcm(n*2);mixer.render(pcm.data(),n,rate);
    for(unsigned i=0;i<n;++i) {
     // A single startup discontinuity is accepted. Once its short filter
     // transient has passed, a 100 Hz tone must have no block-sized gaps.
     if(f+i>100) {
      largestStep=std::max(largestStep,unsigned(std::abs(int(pcm[i*2])-previous)));
      peak=std::max(peak,unsigned(std::abs(int(pcm[i*2]))));
     }
     previous=pcm[i*2];
    }
    f+=n;
   }
   if(largestStep>=250 || peak<5320 || peak>5345)throw std::runtime_error("CD rate="+std::to_string(rate)+" step="+std::to_string(largestStep)+" peak="+std::to_string(peak));
  }
 }
 // Every source reaches the real mixer exactly once, including the simultaneous
 // high-amplitude case (the separate extrema check covers coherent full scale).
 {
  const auto constantLoader=[&](const std::string &name,XeenSceneArchive archive) {
   if(name=="constant.voc") {std::vector<std::uint8_t> sample(60032,255);sample[27]=60009&255;sample[28]=60009>>8;return sample;}
   return loader(name,archive);
  };
  const auto driver=std::make_shared<const std::vector<std::uint8_t>>(loader("promus",XeenSceneArchive::IntroOnly));
  auto fm=std::make_shared<XeenFmSource>(),baseFm=std::make_shared<XeenFmSource>();
  XeenAudio mix(3,driver,constantLoader,module,0,false,{},fm,[](const auto &){return std::vector<std::int16_t>(44100*2,32767);});
  XeenAudio baseline(3,driver,constantLoader,module,0,false,{},baseFm);
  mix.enterTitle(0);baseline.enterTitle(0);mix.cdTracks(extents);mix.retainCdCue({19,370,2161});mix.startCd(0);decoded(mix);
  mix.startDigital("constant.voc",XeenSceneArchive::Clouds,0);mix.prepareRender(44100);baseline.prepareRender(44100);
  std::vector<std::int16_t> actual(20000),music(20000);mix.render(actual.data(),10000,44100);baseline.render(music.data(),10000,44100);
  const auto added=std::lrint(32512*mix.mixerGains()[2]+10922);
  for(unsigned i=0;i<actual.size();++i)check(actual[i]==music[i]+added && std::abs(int(actual[i]))<=28000,"Simultaneous sources clipped or gained twice");
 }
 std::cout<<"CD mapping, decoding, lifecycle and callback tests passed\n";return 0;
}catch(const std::exception &e){std::cerr<<e.what()<<'\n';return 1;}}
