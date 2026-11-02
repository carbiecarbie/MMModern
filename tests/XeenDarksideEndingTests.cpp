#include "app/XeenCutsceneFlow.h"
#include "app/XeenAudio.h"
#include "formats/xeen/XeenAssetSource.h"
#include "XeenTestInstallation.h"
#include <algorithm>
#include <fstream>
#include <iostream>
#include <map>
#include <cstdlib>
#include <cmath>
#include <zlib.h>

using namespace mmodern;
namespace {
void check(bool ok,const std::string &message){if(!ok)throw std::runtime_error(message);}
void picture(const IndexedFrame &frame,const std::filesystem::path &path) {
 std::ofstream out(path,std::ios::binary);out<<"P6\n320 200\n255\n";
 for(auto p:frame.pixels)out.write(reinterpret_cast<const char *>(frame.palette.data()+3*p),3);
 check(bool(out),"Cannot write ending frame");
}
// Golden RGB values at VGA's six-bit precision, from video0004.avi.
// The first page is acknowledged before phase 0 recurs; page two covers
// all 32 poses. Original extraction and pixel diffs stay outside the repo.
void originalScoreFrame(const IndexedFrame &frame,unsigned page,unsigned phase) {
 static const std::uint32_t expected[2][32]{
  {3579482612u,313040246u,3785280307u,3130673894u,1033307913u,2245978344u,1389348040u,1121082271u,2395711668u,3487198742u,463491007u,2556859835u,193351687u,111740062u,221787096u,2258677334u,355189559u,2682617363u,48831693u,3099811835u,2853163229u,1116447418u,1762244074u,48831693u,3099811835u,2853163229u,2082121928u,4097037085u,409384371u,637090060u,1078141776u,0u},
  {3553511596u,2168786214u,3661729011u,1565311260u,3842322173u,840172253u,2565225920u,1426050795u,367921225u,2431378790u,3824148523u,228056867u,3640564142u,1920703269u,4192405163u,3498560384u,2042495419u,3836896101u,1587028051u,1280618357u,2765151506u,2404567618u,3836896101u,1587028051u,1280618357u,2588006240u,832560042u,3721051396u,3763679675u,2247044071u,64549372u,2984422542u}
 };
 const auto hash=expected[page][phase%32];if(!hash)return;
 std::vector<std::uint8_t> rgb;rgb.reserve(192000);
 for(auto p:frame.pixels)for(unsigned c=0;c<3;++c)rgb.push_back(frame.palette[3*p+c]/4);
 check(crc32(0,rgb.data(),rgb.size())==hash,"Darkside score frame differs from DOS capture");
}
struct Run {
 std::uint64_t now=17000000;std::shared_ptr<XeenAudio> audio;std::unique_ptr<XeenCutsceneFlow> flow;
 std::vector<XeenCutsceneFlow::Trace> trace;std::vector<XeenCutscene::Audio> requests;
 std::map<std::string,std::uint64_t> first;std::map<std::string,unsigned> frames;
 std::filesystem::path out;bool keys=false;
 std::ofstream atlas,atlasTimes,audioTimes;
 Run(const GameInstallation &installation,XeenAssetSource &assets,const XeenFontFormat &font,const XeenCutsceneData &data,
  const XeenCutsceneData::Captions &captions,std::filesystem::path output,bool input=false,std::uint32_t score=1992200000):out(std::move(output)),keys(input) {
  if(!out.empty() && std::getenv("MMODERN_ENDING_ATLAS")){atlas.open(out/"frames.rgb",std::ios::binary);atlasTimes.open(out/"frames.tsv");audioTimes.open(out/"audio.tsv");}
  audio=XeenAudio::installed(installation,assets,now,false,std::filesystem::path("mmodern-nonexistent-rom-fixture"));audio->observeOptions({});
  auto s=XeenCutsceneFlow::original(assets,font,*audio,[&]{return now;},data.digitalDevice>=2,XeenSceneArchive::DarksideProgram);
  const auto play=s.audio;s.audio=[&,play](const auto &a,auto t){requests.push_back(a);if(audioTimes.is_open())audioTimes<<t<<'\t'<<a.kind<<'\t'<<a.resource<<'\t'<<a.value<<'\n';play(a,t);};s.trace=[&](const auto &t){trace.push_back(t);};
  flow=std::make_unique<XeenCutsceneFlow>(XeenCutscene::darksideEnding(data,captions,XeenWallClock::fixed(1782878400),score),std::move(s));acquire();
 }
 void acquire() {
  flow->presented(flow->frame().presentation());flow->completeInput(flow->frame().presentation());
  const auto stage=flow->segmentName();const auto n=frames[stage]++;
  if(!first.count(stage))first[stage]=now;
  if(!out.empty() && stage.find("Darkside score page ")==0)originalScoreFrame(flow->frame(),stage.back()-'1',n);
  if(atlas.is_open()) {
   atlasTimes<<now<<'\t'<<stage<<'\t'<<n<<'\n';
   const auto &f=flow->frame();for(auto p:f.pixels){const char rgb[]{char(f.palette[3*p]/4),char(f.palette[3*p+1]/4),char(f.palette[3*p+2]/4)};atlas.write(rgb,3);}
  }
  if(!out.empty() && (n==0 || (stage.find("Darkside score page")==0 && n<=32)))picture(flow->frame(),out/(stage+"-"+std::to_string(n)+".ppm"));
  if(keys && stage.find("score page")==std::string::npos && stage.find("closing scroll")==std::string::npos) {
   flow->handle(DialogKeyAction{32},flow->frame().presentation());flow->handle(CancelInteractionAction{},flow->frame().presentation());
  }
  check(!flow->inputContext(flow->frame().presentation()).dialog->click(80,80),"Ending admits mouse input");
 }
 void finish() {
  for(unsigned guard=0;guard<100000 && !flow->done();++guard) {
   const auto stage=flow->segmentName();
   if(stage.find("Darkside score page")==0 && frames[stage]>32)flow->handle(DialogKeyAction{27},flow->frame().presentation());
   auto next=flow->nextPresentationDeadline();check(bool(next),"Darkside ending lost deadline at "+stage);now=std::max(now,*next);
   audio->advance(now);if(flow->update())acquire();
  }
  if(!flow->done())throw std::runtime_error("Darkside ending stalled at "+flow->segmentName());
 }
};
void capturedCadence(const Run &run) {
 // Unique whole-frame matches in video0004.avi (native ZMBV, VGA6 colors).
 // A sampled picture identifies a displayed pose, not its exact first scanout.
 const auto flip=[&](const char *stage,unsigned iteration) {
  for(const auto &t:run.trace)if(t.event=="flip" && t.segment==stage && t.iteration==iteration)
   return double(t.nanoseconds)/1e9;
  throw std::runtime_error(std::string("Missing captured pose: ")+stage);
 };
 struct Pose {const char *stage;unsigned iteration;double native;};
 const double origin=8.617723-flip("Alamar rises",11);
 check(std::abs(double(run.first.at("Darkside scene 1"))/1e9+origin-5.0)<=.25,"Opening before Alamar anchor changed");
 for(const char *name:{"scene 1 fade in","scene 1 fade out","throne fade in"})check(run.frames.at(name)==33,"Opening fade removed or shortened");
 for(const auto &p:std::vector<Pose>{{"disk flight",49,20.774164},{"disk opens",16,33.943641},
  {"Corak arrives",26,44.074008},{"helmet rises",12,61.295633},{"fight",13,76.491184},
  {"cloak",9,82.569404},{"Corak bleeding",18,92.699771},{"shield",12,109.921395},
  {"ricochet",18,114.986579},{"admit defeat",0,117.012653},{"I do",20,122.077836}})
  check(std::abs(flip(p.stage,p.iteration)+origin-p.native)<=.4,
   std::string("Pre-boundary DOS capture bracket: ")+p.stage);
 // Maintainer-approved exception ONLY at I do -> What no, 2026-11-01:
 // docs/original-quirks.md#dev-m106-ido-boundary. Keep the unsplit sound
 // deadline (modeled 4.5 s, about 0.65 s longer than DOS), then realign
 // the capture comparison here. Both regional checks remain strict +/-0.4 s;
 // no runtime wait or guessed load duration is added.
 const double after=124.103910-flip("what no",2);
 for(const auto &p:std::vector<Pose>{{"what no",8,125.116946},{"vortex",9,128.156057},
  {"vortex",21,130.182130},{"vortex",40,133.221240},{"castle explodes",8,136.260350},
  {"castle explodes",21,138.286424},{"castle explodes",45,142.338571},{"Darkside score page 1",0,148.040000}})
  check(std::abs(flip(p.stage,p.iteration)+after-p.native)<=.4,
   std::string("Post-boundary relative DOS capture bracket: ")+p.stage);
 std::cout<<"Capture anchors: pre-boundary origin "<<origin<<" s; post-boundary origin "<<after
  <<" s; approved I do model "<<double(run.first.at("what background")-run.first.at("I do background"))/1e9
  <<" s (dev-m106-ido-boundary); downstream cadence re-anchored\n";
}
void scoreInput(const XeenCutsceneData &data,const XeenCutsceneData::Captions &captions) {
 // Isolate the production page's input/tick properties without art or I/O.
 auto scene=XeenCutscene::darksideEnding(data,captions,XeenWallClock::fixed(0),1234567890);
 const auto it=std::find_if(scene.segments.begin(),scene.segments.end(),[](const auto &s){return s.name=="Darkside score page 1";});
 auto page=*it;page.enter={};page.entering={};page.compose=[](const auto &)->XeenCutscene::Frame{return {};};
 scene.segments={page};scene.closing=1;std::uint64_t now=0;XeenCutsceneFlow::Services services;
 services.clock=[&]{return now;};services.read=[](const auto &){return std::vector<std::uint8_t>{};};services.draw=[](auto &,const auto &){};
 services.audio=[](const auto &,auto){};services.digitalBusy=[](auto){return false;};
 XeenCutsceneFlow flow(std::move(scene),services);flow.presented(flow.frame().presentation());flow.completeInput(flow.frame().presentation());
 flow.handle(DialogKeyAction{27},flow.frame().presentation());now=PresentationClock::boundary(3)-1;flow.update();check(!flow.done(),"Score key bypasses three-tick wait");
 check(!flow.controlQuitReady(),"Score Ctrl-Q polled before three ticks");
 now=PresentationClock::boundary(3);flow.update();check(flow.done(),"Score page ignores keyboard after three ticks");
 // DOS clears before the two-page loop, not between its pages (0x6f15).
 // Preserve a second queued key and the repeat context across that boundary.
 XeenCutscene grouped;grouped.segments={page,page};grouped.closing=2;now=0;
 XeenCutsceneFlow pair(std::move(grouped),services);pair.presented(pair.frame().presentation());pair.completeInput(pair.frame().presentation());
 const auto group=pair.inputContext(pair.frame().presentation()).contextId;
 pair.handle(DialogKeyAction{32},pair.frame().presentation());pair.handle(DialogKeyAction{27},pair.frame().presentation());
 now=PresentationClock::boundary(3);pair.update();now=*pair.nextPresentationDeadline();pair.update();pair.presented(pair.frame().presentation());pair.completeInput(pair.frame().presentation());
 check(!pair.done() && pair.inputContext(pair.frame().presentation()).contextId==group,"Score page discarded DOS key-repeat context");
 now=PresentationClock::boundary(6)-1;pair.update();check(!pair.done(),"Queued second key bypassed its page's three-tick wait");
 now=PresentationClock::boundary(6);pair.update();check(pair.done(),"Second score page discarded its queued keyboard result");
}
void quitPolls() {
 using C=XeenCutscene;using I=C::Segment::Input;
 for(const auto input:{I::Ignore,I::Unpolled}) {
  C scene;C::Segment s;s.name="quit poll";s.compose=[](const auto &)->C::Frame{return {};};s.frames=2;s.ticks=3;s.input=input;
  scene.segments={s};scene.closing=1;std::uint64_t now=0;XeenCutsceneFlow::Services services;
  services.clock=[&]{return now;};services.read=[](const auto &){return std::vector<std::uint8_t>{};};services.draw=[](auto &,const auto &){};
  services.audio=[](const auto &,auto){};services.digitalBusy=[](auto){return false;};XeenCutsceneFlow flow(scene,services);
  flow.presented(flow.frame().presentation());flow.completeInput(flow.frame().presentation());
  check(flow.controlQuitReady()==(input==I::Ignore),"EXE quit poll boundary");flow.update();
  check(!flow.controlQuitReady(),"EXE quit repeated inside unpolled fixed hold");
  now=PresentationClock::boundary(3);flow.update();now=*flow.nextPresentationDeadline();flow.update();flow.presented(flow.frame().presentation());flow.completeInput(flow.frame().presentation());
  check(flow.controlQuitReady()==(input==I::Ignore),"EXE quit did not reach next frame poll");
 }
}
void firstFrameFade(const XeenCutsceneData &data,const XeenCutsceneData::Captions &captions) {
 using C=XeenCutscene;
 auto intro=C::darkside(data,captions,XeenWallClock::fixed(0),C::DarksidePath::Full);
 auto ending=C::darksideEnding(data,captions,XeenWallClock::fixed(0),0);
 for(const auto &entry:std::vector<std::pair<C *,const char *>>{{&intro,"pyramid approach"},{&ending,"helmet closeup"}}) {
  auto it=std::find_if(entry.first->segments.begin(),entry.first->segments.end(),[&](const auto &x){return x.name==entry.second;});
  check(it!=entry.first->segments.end(),"Missing fade-order site");auto stage=*it;
  const bool immediate=stage.name=="helmet closeup";
  check((stage.firstFadeOrder==C::Segment::FirstFadeOrder::AfterPublish)==immediate,"EXE stage fade order changed");
  stage.enter={};stage.entering={};stage.published={};stage.finished={};stage.compose=[](const auto &)->C::Frame{return {};};stage.frames=2;
  C scene;scene.segments={stage};scene.closing=1;std::uint64_t now=17000000;XeenCutsceneFlow::Services services;
  services.clock=[&]{return now;};services.read=[](const auto &){return std::vector<std::uint8_t>{};};services.draw=[](auto &,const auto &){};
  services.audio=[](const auto &,auto){};services.digitalBusy=[](auto){return false;};XeenCutsceneFlow flow(scene,services);
  flow.presented(flow.frame().presentation());flow.completeInput(flow.frame().presentation());
  const auto hold=PresentationClock::boundary(PresentationClock::tick(now)+stage.ticks);
  check(flow.nextPresentationDeadline()==(immediate?now:hold),"First fade deadline violates EXE order");
  flow.update();
  if(!immediate){check(flow.nextPresentationDeadline()==hold,"Early input cycle bypassed pre-fade hold");now=hold;flow.update();}
  check(flow.nextPresentationDeadline()==PresentationClock::retraceAfter(now),"Fade did not begin at its selected boundary");
 }
}
void boundaryPreparation(const XeenCutsceneData &data,const XeenCutsceneData::Captions &captions) {
 // Give reads/preparation real fake-clock cost. Speech must start after
 // every EXE preload, rather than allowing lazy page reads during speech.
 struct Case {const char *background,*speech,*voice;std::vector<std::string> pages;};
 for(const auto &c:std::vector<Case>{{"defeat background","admit defeat","admit2.voc",{"sc25a.end","sc25b.end"}},
  {"I do background","I do","ido2.voc",{"sc26a.end","sc26b.end","sc26c.end","sc26d.end"}},
  {"what background","what no","what3.voc",{"sc27.end"}}}) {
  auto original=XeenCutscene::darksideEnding(data,captions,XeenWallClock::fixed(0),0);XeenCutscene scene;
  for(const char *name:{c.background,c.speech}) {
   const auto it=std::find_if(original.segments.begin(),original.segments.end(),[&](const auto &s){return s.name==name;});
   check(it!=original.segments.end(),"Missing ending boundary segment");scene.segments.push_back(*it);
  }
  scene.closing=scene.segments.size();std::uint64_t now=17000000;bool retained=false,started=false;
  std::vector<std::string> prepared;XeenCutsceneFlow::Services services;
  services.clock=[&]{return now;};services.digitalBusy=[](auto){return false;};
  services.read=[&](const auto &){now+=50000000;return std::vector<std::uint8_t>(64000);};
  services.prepare=[&](const auto &name){check(retained && !started,"Sprite load crossed the EXE voice-start boundary");prepared.push_back(name);now+=100000000;};
  services.draw=[](auto &,const auto &){};
  services.audio=[&](const auto &a,auto) {
   if(a.kind==XeenCutscene::Audio::RetainDigital){check(a.resource==c.voice && prepared.empty() && !started,"EXE voice preload order");retained=true;now+=50000000;}
   if(a.kind==XeenCutscene::Audio::Digital){check(retained && prepared==c.pages && !started,"Speech started before all EXE sprite pages were loaded");started=true;}
  };
  XeenCutsceneFlow flow(std::move(scene),std::move(services));
  for(unsigned guard=0;guard<1000 && !flow.done();++guard) {
   flow.presented(flow.frame().presentation());flow.completeInput(flow.frame().presentation());
   const auto next=flow.nextPresentationDeadline();check(bool(next),"Boundary preparation lost deadline");now=std::max(now,*next);flow.update();
  }
  check(flow.done() && started,"Boundary preparation did not reach speech");
 }
}
}
int main(int argc,char **argv) {try {
 check(argc==4 && (std::string(argv[3])=="--main" || std::string(argv[3])=="--captions"),"Darkside ending test requires original data, output directory and --main/--captions");
 const auto installation=*xeenTestInstallationDetector().detect(argv[1]);XeenAssetSource assets(installation,320,200);
 const auto data=XeenCutsceneData::installed(installation);const auto captions=XeenCutsceneData::captions(assets.readArchiveResource(data.darkside.resources[XeenCutsceneData::Darkside::Caption],XeenSceneArchive::DarksideProgram));
 const XeenFontFormat font(assets.readArchiveResource("fnt",XeenSceneArchive::DarksideOnly));
 const std::filesystem::path out=argv[2];std::filesystem::create_directories(out);
 if(std::string(argv[3])=="--main") {
  scoreInput(data,captions);quitPolls();firstFrameFade(data,captions);boundaryPreparation(data,captions);
  Run run(installation,assets,font,data,captions,out);run.finish();capturedCadence(run);
  std::ofstream trace(out/"trace.tsv");for(const auto &t:run.trace)trace<<t.event<<'\t'<<t.segment<<'\t'<<t.iteration<<'\t'<<t.nanoseconds<<'\t'<<t.tick<<'\t'<<t.reset<<'\n';
  std::ofstream timing(out/"timing.tsv");for(const auto &t:run.trace)if(t.event=="flip" && t.iteration==0)timing<<t.segment<<'\t'<<double(t.nanoseconds)/1e9<<'\n';
  for(const auto &hold:std::vector<std::pair<const char *,unsigned>>{{"scene 1 hold",30},{"throne room hold",30},{"disk opening hold",20},{"disk opening minimum",30},{"disk flight tail",10},{"come to me",27},{"Corak question",30},{"Sheltem hold",30},{"Corak settle",2}}) {
   const auto begin=std::find_if(run.trace.begin(),run.trace.end(),[&](const auto &t){return t.segment==hold.first && t.event=="entry reset";});
   check(begin!=run.trace.end(),std::string("Missing hold ")+hold.first);
   const auto end=std::find_if(begin+1,run.trace.end(),[&](const auto &t){return t.event=="entry reset" && t.segment!=hold.first;});
   check(end!=run.trace.end() && end->tick-begin->reset>=hold.second,std::string("Minimum elapsed hold ticks: ")+hold.first);
  }
  const auto &o=*run.flow->cutscene().observations;
  const unsigned maximum[]{2,8,1,3};const unsigned counts[]{13,15,15,15};
  for(unsigned i=0;i<4;++i)check(o.endingMouths[i].size()==counts[i] && std::all_of(o.endingMouths[i].begin(),o.endingMouths[i].end(),[&](auto n){return n<=maximum[i];}),"DOS ending mouth draws/range");
  for(const char *stage:{"Alamar rises","disk flight","Corak arrives","fight","I do","vortex","castle explodes","Darkside score page 1","Darkside score page 2","Darkside ending shutdown"})check(run.first.count(stage),std::string("Missing stage ")+stage);
  std::uint64_t previous=0;
  for(const char *stage:{"Darkside scene 1","Alamar rises","come to me","throne room hold","door opens","party enters","disk flight","disk opens","beam","Corak arrives","Corak question","Corak yes","nowhere to run","nor do you","helmet closeup","helmet rises","Sheltem hold","ready","fight challenge","fight","cannot fail","force travels","cloak","Sheltem fires","Corak bleeding","Corak waves","bolts","Sheltem laughs","sphere forms","sphere travels","shield","ricochet","admit defeat","I do","what no","vortex","castle explodes","Darkside score page 1","Darkside score page 2","Darkside ending shutdown"}) {
   check(run.first.at(stage)>previous,std::string("DOS ending stage order: ")+stage);previous=run.first.at(stage);
  }
  // Calibrated EXE wait sites, including the two waits the pin changes.
  for(const auto &wait:std::vector<std::pair<const char *,unsigned>>{{"Alamar rises",1},{"door opens",4},{"party enters",4},{"disk flight",1},{"disk opens",3},{"beam",3},{"Corak arrives",2},{"Corak yes",3},{"nowhere to run",3},{"nor do you",3},{"helmet closeup",3},{"helmet rises",3},{"ready",3},{"fight challenge",3},{"fight",3},{"cannot fail",4},{"force travels",2},{"cloak",3},{"Sheltem fires",3},{"Corak bleeding",3},{"Corak waves",3},{"bolts",2},{"sphere forms",4},{"sphere travels",2},{"shield",3},{"ricochet",3},{"admit defeat",3},{"I do",2},{"what no",3},{"vortex",3},{"castle explodes",3},{"Darkside score page 1",3},{"Darkside score page 2",3}}) {
   std::optional<std::uint64_t> tick;
   for(const auto &t:run.trace)if(t.segment==wait.first && t.event=="compose") {
    if(tick)check(t.tick-*tick>=wait.second,std::string("DOS ending frame wait: ")+wait.first);tick=t.tick;
   }
  }
  for(const auto &pair:std::vector<std::pair<const char *,unsigned>>{{"Alamar rises",27},{"disk flight",160},{"Corak arrives",45},{"admit defeat",18},{"what no",12},{"castle explodes",74}})check(run.frames[pair.first]==pair.second,"DOS fixed frame count");
  Run input(installation,assets,font,data,captions,{},true,1234567890);input.finish();
  auto replay=XeenCutscene::darksideEnding(data,captions,XeenWallClock::fixed(0),1234567890);
  const auto page=std::find_if(replay.segments.begin(),replay.segments.end(),[](const auto &s){return s.name=="Darkside score page 1";});
  const auto composition=page->compose({0,0,0,false});
  check(std::get<XeenCutscene::Text>(composition.back()).value.find("1234567890")!=std::string::npos,"E replay did not display the fixed DOS score");
  std::vector<std::string> songs;std::vector<unsigned> mixers;
  for(const auto &a:run.requests){if(a.kind==XeenCutscene::Audio::Song)songs.push_back(a.resource);if(a.kind==XeenCutscene::Audio::Mixer)mixers.push_back(a.value);}
  check(songs==std::vector<std::string>({"dngon3.m","dngon2.m","sf17.m","dungeon2.m","outday3.m"}),"Darkside ending song sequence");
  check(mixers==std::vector<unsigned>({95,48,95,95}),"Darkside ending raw mixer commands");
  std::map<std::string,unsigned> sounds;for(const auto &a:run.requests)if(a.kind==XeenCutscene::Audio::Digital)++sounds[a.resource];
  check(sounds["explosio.voc"]==9 && sounds["crash.voc"]==1 && sounds["laff1.voc"]==1,"Original EXE sound-handle reuse/castle smoke boundary");
  check(input.now==run.now && input.flow->cutscene().observations->endingMouths==o.endingMouths,"Ignored input/replay score changed timing or presentation RNG");
  std::cout<<"Darkside ending main/input/replay, fixed frames, random mouths and keyboard score waits passed; "<<double(run.now)/1e9<<" s\n";
 } else {
  auto silent=data;silent.digitalDevice=0;Run run(installation,assets,font,silent,captions,{});run.finish();
  const auto &o=*run.flow->cutscene().observations;
  check(o.captionRecords==std::vector<unsigned>({21,22,23,24,25,26,27,28,29,30,31}),"Darkside ending caption selection order");
  check(!o.captionAdvances.empty() && std::all_of(o.captionWidths.begin(),o.captionWidths.end(),[](auto n){return n<=40;}),"Ending caption width/cadence");
  check(!run.audio->digitalBusy(run.now),"No-device ending started digital sound");
 }
 return 0;
}catch(const std::exception &e){std::cerr<<e.what()<<'\n';return 1;}}
