#include "app/XeenCutsceneFlow.h"
#include "app/XeenAudio.h"
#include "formats/xeen/XeenAssetSource.h"
#include "formats/xeen/XeenVoc.h"
#include "compat/scummvm/ScummVmXeenBridge.h"
#include "XeenTestInstallation.h"
#include <algorithm>
#include <cmath>
#include <fstream>
#include <iostream>
#include <set>
using namespace mmodern;
namespace {
void check(bool ok,const char *message){if(!ok)throw std::runtime_error(message);}
std::vector<std::uint8_t> read(const ReadOnlyDataFile &file) {
 auto in=file.open();std::vector<std::uint8_t> b(in->size());check(in->read(b.data(),b.size())==b.size(),"test source read");return b;
}
struct Run {
 std::uint64_t now=17000000;XeenAssetSource &assets;const XeenFontFormat &font;
 std::shared_ptr<XeenAudio> audio;
 std::unique_ptr<XeenCutsceneFlow> flow;
 std::vector<XeenCutsceneFlow::Trace> trace;
 std::vector<XeenCutscene::Audio> requests;
 std::vector<std::string> heard;
 bool slow=false;unsigned prepareCost=0;
 std::filesystem::path pictures;std::set<std::string> pictured;
 Run(const GameInstallation &installation,XeenAssetSource &a,const XeenFontFormat &f,const XeenCutsceneData &data,bool slowPublication=false,unsigned loadCost=0):assets(a),font(f),slow(slowPublication),prepareCost(loadCost) {
  audio=XeenAudio::installed(installation,assets,now,false);
  // Same EXE handoff as Application: saved party switches are all off.
  audio->observeOptions({false,false,true,false});audio->observeOptions({});
  audio->digitalSink([&](const auto &name,auto){heard.push_back(name);});
  auto services=XeenCutsceneFlow::original(assets,font,*audio,[&]{return now;},data.digitalDevice>=2);
  const auto action=services.audio;services.audio=[&,action](const auto &a,auto t){requests.push_back(a);action(a,t);};
  const auto prepare=services.prepare;services.prepare=[&,prepare](const auto &name){now+=prepareCost;prepare(name);};
  services.trace=[&](const auto &e){trace.push_back(e);};
  const auto story=XeenCutsceneData::firstCaption(assets.readArchiveResource(data.resources[XeenCutsceneData::Caption],XeenSceneArchive::CloudsProgram));
  flow=std::make_unique<XeenCutsceneFlow>(XeenCutscene::clouds(data,story),std::move(services));
  acquire();
 }
 void acquire() {
  flow->presented(flow->frame().presentation());flow->completeInput(flow->frame().presentation());
  const auto name=flow->segmentName();
  if(!pictures.empty() && (name=="narration 0" || name=="lake flight" || name=="wizard") && pictured.insert(name).second) {
   std::ofstream out(pictures/(name+".ppm"),std::ios::binary);out<<"P6\n320 200\n255\n";
   const auto &image=flow->frame();for(auto p:image.pixels)out.write(reinterpret_cast<const char *>(image.palette.data()+3*p),3);
  }
 }
 void step() {
  const auto next=flow->nextPresentationDeadline();check(bool(next),"missing running cutscene deadline");
  if(*next>now)now=*next;
  // Explicit fake publication latency makes the first pan's measured stride 3.
  if(slow && flow->segmentName()=="stars descent" && flow->cutscene().observations->starsOffsets.size()==1 &&
     flow->cutscene().observations->panStride==0)now=PresentationClock::after(now,3);
  audio->advance(now);if(auto frame=flow->update())acquire();
 }
 void finish(){for(unsigned guard=0;guard<200000 && !flow->done();++guard)step();check(flow->done(),"cutscene did not finish");}
 void until(const std::string &name){for(unsigned guard=0;guard<200000 && (flow->segmentName()!=name || !flow->inputContext(flow->frame().presentation()).readyForAction);++guard)step();check(flow->segmentName()==name,"scene not reached");}
 void key(unsigned code){check(flow->inputContext(flow->frame().presentation()).readyForAction,"test key before acquired input");flow->handle(DialogKeyAction{code},flow->frame().presentation());}
 std::uint64_t first(const std::string &name,const std::string &event="flip") const {
  auto i=std::find_if(trace.begin(),trace.end(),[&](const auto &e){return e.segment==name && e.event==event;});
  check(i!=trace.end(),"missing timing marker");return i->nanoseconds;
 }
};
void model() {
 check(XeenCutscene::elapsed(7,9,2) && !XeenCutscene::elapsed(7,8,2),"capture and subtract hold");
 check(XeenCutscene::stride(0)==1 && XeenCutscene::stride(4)==4,"measured pan stride");
 const auto phase=PresentationClock::boundary(9)-1;
 check(PresentationClock::after(phase,2)==PresentationClock::boundary(10),"BIOS reset lost phase");
 std::uint64_t now=phase;unsigned sound=0;
 XeenCutscene scene;
 for(unsigned ticks:{2,3}) {
  XeenCutscene::Segment s;s.name=std::to_string(ticks);s.ticks=ticks;
  s.compose=[ticks](const auto &)->XeenCutscene::Frame{return {XeenCutscene::Level{ticks}};};scene.segments.push_back(s);
 }
 XeenCutsceneFlow::Services services;services.clock=[&]{return now;};services.read=[](const auto &){return std::vector<std::uint8_t>{};};
 services.draw=[](auto &,const auto &){};services.audio=[&](const auto &,auto){++sound;};services.digitalBusy=[](auto){return false;};
 XeenCutsceneFlow flow(std::move(scene),services);flow.presented(flow.frame().presentation());flow.completeInput(flow.frame().presentation());
 check(!flow.inputContext(flow.frame().presentation()).dialog->click(1,1),"cutscene mouse admitted");
 auto stale=flow.frame().presentation();
 now=PresentationClock::boundary(10)-1;check(!flow.update() && flow.segmentName()=="2","early tick hold");
 now=PresentationClock::boundary(10);flow.update();check(flow.segmentName()=="3","segment order");
 for(unsigned guard=0;guard<30 && !flow.done();++guard) {
  now=std::max(now,*flow.nextPresentationDeadline());if(flow.update())flow.presented(flow.frame().presentation());
 }
 check(flow.done() && sound==0 && !flow.acceptsInput(stale),"done signal or stale input");
 for(unsigned step:{128,8,4,2}) {
  now=0;XeenCutscene fades;XeenCutscene::Segment f;f.name="fade";f.frames=128/step+1;
  f.compose=[step](const auto &c)->XeenCutscene::Frame{return {XeenCutscene::Level{step*c.iteration}};};fades.segments.push_back(f);
  unsigned flips=0;std::uint64_t previous=0;services.trace=[&](const auto &t){if(t.event=="flip"){if(flips)check(t.nanoseconds>=PresentationClock::retraceAfter(previous),"fade skipped retrace");previous=t.nanoseconds;++flips;}};
  XeenCutsceneFlow fade(std::move(fades),services);fade.presented(fade.frame().presentation());
  for(unsigned guard=0;guard<400 && !fade.done();++guard) {
   now=std::max(now,*fade.nextPresentationDeadline());
   if(auto image=fade.update()) {
    const auto origin=image->presentation();now+=1000000;
    check(!fade.update() && fade.frame().presentation()==origin,"unacquired level skipped or reissued");
    fade.presented(origin);
   }
  }
  check(fade.done() && flips==128/step+1,"fade endpoint/write count");
 }
}
void resourceAudit(XeenAssetSource &assets,const XeenCutsceneData &data) {
 unsigned sprites=0,frames=0;
 std::vector<std::string> names;
 for(auto r:{XeenCutsceneData::Logo,XeenCutsceneData::LogoTail,XeenCutsceneData::Stars,XeenCutsceneData::Title,
  XeenCutsceneData::Lake,XeenCutsceneData::Xeen,XeenCutsceneData::Tower,XeenCutsceneData::Wizard,
  XeenCutsceneData::LakeFlight,XeenCutsceneData::LakeDescent,XeenCutsceneData::Burst,
  XeenCutsceneData::GroupBack,XeenCutsceneData::Group,XeenCutsceneData::Crodo,XeenCutsceneData::Box})names.push_back(data.resources[r]);
 for(unsigned i=0;i<16;++i)names.push_back(data.hand(i));for(unsigned i=1;i<=4;++i)names.push_back(data.marble(i));
 for(const auto &name:names) {
  const auto count=assets.spriteFrameCount(name,XeenSceneArchive::CloudsProgram);++sprites;frames+=count;
  for(unsigned frame=0;frame<count;++frame) {
   IndexedFrame image{320,200,std::vector<std::uint8_t>(64000)};
   assets.drawDialogSprite(image,name.c_str(),frame,0,0,XeenSceneArchive::CloudsProgram);
  }
 }
 check(sprites==35 && frames==325,"Clouds compiled sprite inventory");
 for(auto r:{XeenCutsceneData::LogoBack,XeenCutsceneData::Jvc,XeenCutsceneData::Blank})
  check(assets.readArchiveResource(data.resources[r],XeenSceneArchive::CloudsProgram).size()==64000,"Clouds RAW size");
 for(auto r:{XeenCutsceneData::LogoPalette,XeenCutsceneData::JvcPalette,XeenCutsceneData::ScenePalette})
  check(assets.readArchiveResource(data.resources[r],XeenSceneArchive::CloudsProgram).size()==768,"Clouds palette size");
 std::set<std::string> voices(data.voices.begin(),data.voices.end());voices.insert(data.resources[XeenCutsceneData::Fire]);voices.insert(data.resources[XeenCutsceneData::Growl]);
 check(voices.size()==14,"Clouds unique VOC inventory");
 for(const auto &name:voices)check(!decodeXeenVoc(assets.readArchiveResource(name,XeenSceneArchive::CloudsProgram)).pcm.empty(),"Clouds VOC decode");
 check(!decodeXeenVoc(assets.readArchiveResource(data.resources[XeenCutsceneData::Scratch],XeenSceneArchive::IntroOnly)).pcm.empty(),"Startup scratch VOC decode");
}
std::vector<unsigned> mixers(const Run &run) {
 std::vector<unsigned> result;for(const auto &r:run.requests)if(r.kind==XeenCutscene::Audio::Mixer)result.push_back(r.value);return result;
}
void whole(const GameInstallation &installation,XeenAssetSource &assets,const XeenFontFormat &font,const XeenCutsceneData &data,const std::filesystem::path &output) {
 std::filesystem::create_directories(output);
 Run run(installation,assets,font,data);run.pictures=output;run.finish();
 check(run.heard.size()==16 && run.heard[0]==data.resources[XeenCutsceneData::Fire] && run.heard[1]==data.resources[XeenCutsceneData::Growl],"logo digital order");
 check(std::equal(data.voices.begin(),data.voices.end(),run.heard.begin()+2),"fourteen voice order");
 std::vector<unsigned> effects;unsigned songs=0;bool fade50=false,fade0=false;
 for(const auto &r:run.requests) {
  if(r.kind==XeenCutscene::Audio::Fm)effects.push_back(r.value);
  if(r.kind==XeenCutscene::Audio::Song){++songs;check(r.resource==data.resources[XeenCutsceneData::Song],"CD promus song choice");}
  if(r.kind==XeenCutscene::Audio::FadeSong){fade50|=r.value==50;fade0|=r.value==0;}
 }
 check(effects==std::vector<unsigned>{1,0,3,3,2,52,0} && songs==1 && fade50 && fade0,"FM/song request order");
 check(mixers(run)==std::vector<unsigned>{75,95} && std::abs(run.audio->mixerGains()[0]-std::pow(10.0f,-0.8f))<1e-5f,"Natural completion mixer 75 then 95 (EXE 0x8481, 0x88c2)");
 const auto &o=*run.flow->cutscene().observations;check(o.panStride==1 && o.starsOffsets.front()==0 && o.starsOffsets.back()==199 &&
  o.lakeOffsets.front()==1 && o.lakeOffsets.back()==200 && o.lakeOffsets.size()==200,"stride-one pan endpoints");
 for(const auto &e:run.trace)if(e.event=="flip" && (e.segment=="NWC logo" || e.segment=="wizard")) {
  if(e.segment=="NWC logo")check(e.tick>=e.reset,"logo pre-composition reset");
 }
 const auto count=[&](const std::string &name){return std::count_if(run.trace.begin(),run.trace.end(),[&](const auto &e){return e.segment==name && e.event=="flip";});};
 check(count("NWC logo")==80 && count("wizard")==39 && count("lake flight")>100,"animation loop semantics");
 for(const auto &e:run.trace)if(e.event=="compose" && e.iteration>0 && (e.segment=="NWC logo" || e.segment=="wizard")) {
  auto last=std::find_if(run.trace.rbegin(),run.trace.rend(),[&](const auto &p){return p.segment==e.segment && p.iteration+1==e.iteration && p.event==(e.segment=="wizard"?"flip reset":"compose");});
  check(last!=run.trace.rend() && e.tick-last->reset>=2,"per-frame BIOS hold/reset point");
 }
 const auto seconds=[](auto ns){return double(ns)/1e9;};
 std::ofstream timing(output/"timing.txt");
 timing<<"Fake clock: initial phase 17 ms, zero resource latency, publication on the next 70 Hz retrace; real VOC lengths.\n";
 const auto stars=run.first("stars descent"),opening=run.first("opening scroll"),tableau=run.first("tableau"),closing=run.first("closing scroll"),black=run.first("black");
 timing<<"Stars appear -> hands appear (opening scroll): "<<seconds(opening-stars)<<" s; DOS about 33.4 s\n";
 timing<<"Crodo tableau -> closing scroll: "<<seconds(closing-tableau)<<" s; DOS about 34.4 s\n";
 timing<<"Closing scroll + black -> fresh title handoff: "<<seconds(run.now-closing)<<" s; DOS about 1.1 s\n";
 timing<<"Black published at "<<seconds(black-closing)<<" s after closing starts. Pan stride "<<o.panStride<<".\n";
 std::ofstream trace(output/"trace.tsv");for(const auto &e:run.trace)trace<<e.event<<'\t'<<e.segment<<'\t'<<e.iteration<<'\t'<<e.nanoseconds<<'\t'<<e.tick<<'\t'<<e.reset<<'\n';
 Run slow(installation,assets,font,data,true);slow.finish();
 check(slow.flow->cutscene().observations->panStride>=3 && slow.flow->cutscene().observations->lakeOffsets.back()==200,"pan stride not measured on publication");
 Run loading(installation,assets,font,data,false,100000000);loading.finish();
 auto reset=std::find_if(loading.trace.begin(),loading.trace.end(),[](const auto &e){return e.segment=="JVC hold" && e.event=="entry reset";});
 check(reset!=loading.trace.end() && reset->tick>reset->reset,"JVC loading reset must precede sprite loads");
 check(loading.first("JVC fade out to stars","entry reset")-PresentationClock::boundary(reset->reset)<3500000000ULL,"JVC hold added after loading");
}
}
int main(int argc,char **argv) {try {
 check(argc==3,"cutscene test paths");model();auto installation=*xeenTestInstallationDetector().detect(argv[1]);
 auto data=XeenCutsceneData::installed(installation);XeenAssetSource assets(installation,320,200);
 const XeenFontFormat font(assets.readArchiveResource("fnt",XeenSceneArchive::DarksideOnly));resourceAudit(assets,data);
 whole(installation,assets,font,data,argv[2]);
 for(const auto &name:{"NWC logo","lake descent","narration 0"}) {
  Run skip(installation,assets,font,data);skip.until(name);skip.key(27);skip.finish();
  check(skip.flow->done() && std::any_of(skip.trace.begin(),skip.trace.end(),[](const auto &e){return e.event=="escape";}),"Escape did not finish through closing scroll");
  check(mixers(skip)==(std::string(name)=="narration 0"?std::vector<unsigned>{75}:std::vector<unsigned>{}),"Escape must bypass the 95 restoration");
 }
 for(unsigned key:{27u,32u}) {
  Run opening(installation,assets,font,data);opening.until("opening scroll");opening.key(key);opening.until("narration 0");
  check(std::none_of(opening.trace.begin(),opening.trace.end(),[](const auto &e){return e.event=="escape";}),"opening key aborted intro");
  opening.key(27);opening.finish();
 }
 Run space(installation,assets,font,data);space.key(32);space.until("narration 0");space.key(32);space.finish();check(space.heard.size()==16,"ordinary key ended intro");
 auto exe=read(ReadOnlyDataFile::plain(installation.uiModule->physicalPath.parent_path()/"XEEN.EXE"));auto cfg=read(*installation.configuration);
 for(unsigned device:{0u,1u}) {
  cfg[0]=device;cfg[1]=0;const auto noDigital=XeenCutsceneData::parse(exe,cfg);
  Run captions(installation,assets,font,noDigital);captions.finish();check(captions.heard.empty(),"synthetic CFG started digital samples");
  const auto &observed=*captions.flow->cutscene().observations;
  check(!observed.captionWidths.empty() && *std::max_element(observed.captionWidths.begin(),observed.captionWidths.end())==40,"caption strip width");
  check(observed.captionAdvances.size()==389 && std::adjacent_find(observed.captionAdvances.begin(),observed.captionAdvances.end(),[](auto a,auto b){return b-a<2;})==observed.captionAdvances.end(),"caption timer/wrap");
  check(std::none_of(captions.trace.begin(),captions.trace.end(),[](const auto &e){return e.segment=="narration 1";}),"caption wrap did not end single narration pass");
 }
 exe.back()^=1;bool refused=false;try{XeenCutsceneData::parse(exe,cfg);}catch(const std::exception &e){refused=std::string(e.what()).find("different WORLD/XEEN.EXE")!=std::string::npos;}
 check(refused,"different EXE accepted");installation.uiModule->physicalPath=std::filesystem::path(argv[2])/"missing"/"XEEN.DAT";
 refused=false;try{XeenCutsceneData::installed(installation);}catch(const std::exception &e){refused=std::string(e.what()).find("Clouds intro unavailable")!=std::string::npos;}check(refused,"missing EXE notice");
 {
  std::ofstream loose(std::filesystem::path(argv[2])/"blank.raw",std::ios::binary);loose<<"test";loose.close();
  installation.uiModule->physicalPath=std::filesystem::path(argv[2])/"XEEN.DAT";
  ScummVmXeenBridge source(installation,320,200);
  check(source.readArchiveResource("blank.raw",XeenSceneArchive::CloudsProgram)==std::vector<std::uint8_t>{'t','e','s','t'},"EXE loose-first lookup");
  refused=false;try{source.readArchiveResource("pyramid1.raw",XeenSceneArchive::CloudsProgram);}catch(const std::exception &){refused=true;}
  check(refused,"EXE silently fell back from XEEN to INTRO");
 }
 std::cout<<"Clouds resources, segments, clock, audio order, input and CFG captions passed\n";return 0;
}catch(const std::exception &e){std::cerr<<e.what()<<'\n';return 1;}}
