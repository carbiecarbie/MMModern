#include "app/XeenCutsceneFlow.h"
#include "app/XeenAudio.h"
#include "formats/xeen/XeenAssetSource.h"
#include "XeenTestInstallation.h"
#include <algorithm>
#include <cmath>
#include <cstdlib>
#include <fstream>
#include <iostream>
#include <map>
#include <zlib.h>

using namespace mmodern;
namespace {
void check(bool ok,const std::string &message){if(!ok)throw std::runtime_error(message);}
std::uint32_t vga6(const IndexedFrame &frame) {
 std::vector<std::uint8_t> rgb;rgb.reserve(192000);
 for(auto p:frame.pixels)for(unsigned c=0;c<3;++c)rgb.push_back(frame.palette[3*p+c]/4);
 return crc32(0,rgb.data(),static_cast<uInt>(rgb.size()));
}
struct Run {
 std::uint64_t now=17000000;std::shared_ptr<XeenAudio> audio;std::unique_ptr<XeenCutsceneFlow> flow;
 std::vector<XeenCutsceneFlow::Trace> trace;std::vector<XeenCutscene::Audio> requests;
 std::map<std::string,std::uint64_t> first;std::map<std::string,unsigned> frames;
 std::map<std::pair<std::string,unsigned>,std::uint32_t> pictures;std::vector<std::uint32_t> sequence;
 bool keys=false;std::ofstream atlas,atlasTimes;
 Run(const GameInstallation &installation,XeenAssetSource &assets,const XeenFontFormat &font,const XeenCutsceneData &data,
  const std::filesystem::path &out,bool input=false,std::uint32_t score=1234567890):keys(input) {
  if(!out.empty() && std::getenv("MMODERN_ENDING_ATLAS")){atlas.open(out/"frames.rgb",std::ios::binary);atlasTimes.open(out/"frames.tsv");}
  audio=XeenAudio::installed(installation,assets,now,false,std::filesystem::path("mmodern-nonexistent-rom-fixture"));audio->observeOptions({});
  auto s=XeenCutsceneFlow::original(assets,font,*audio,[&]{return now;},data.digitalDevice>=2,XeenSceneArchive::DarksideProgram);
  const auto play=s.audio;s.audio=[&,play](const auto &a,auto t){requests.push_back(a);play(a,t);};s.trace=[&](const auto &t){trace.push_back(t);};
  flow=std::make_unique<XeenCutsceneFlow>(XeenCutscene::worldEnding(data,score),std::move(s));acquire();
 }
 void acquire() {
  flow->presented(flow->frame().presentation());flow->completeInput(flow->frame().presentation());
  const auto stage=flow->segmentName();const auto n=frames[stage]++;
  if(!first.count(stage))first[stage]=now;
  const auto crc=vga6(flow->frame());pictures[{stage,n}]=crc;sequence.push_back(crc);
  if(atlas.is_open()) {
   atlasTimes<<now<<'\t'<<stage<<'\t'<<n<<'\n';
   const auto &f=flow->frame();for(auto p:f.pixels){const char rgb[]{char(f.palette[3*p]/4),char(f.palette[3*p+1]/4),char(f.palette[3*p+2]/4)};atlas.write(rgb,3);}
  }
  if(keys && stage.find(" page ")==std::string::npos) {
   flow->handle(DialogKeyAction{32},flow->frame().presentation());flow->handle(CancelInteractionAction{},flow->frame().presentation());
  }
  check(!flow->inputContext(flow->frame().presentation()).dialog->click(80,80),"World ending admits mouse input");
 }
 void finish() {
  for(unsigned guard=0;guard<200000 && !flow->done();++guard) {
   const auto stage=flow->segmentName();
   // The maintainer's capture acknowledged the opening page on its 58th
   // frame and the score page on its 36th; the same poses precede each fade.
   const unsigned last=stage=="World opening page 1"?57:stage=="World score page 1"?35:32;
   if(stage.find(" page ")!=std::string::npos && frames[stage]>last)flow->handle(DialogKeyAction{13},flow->frame().presentation());
   auto next=flow->nextPresentationDeadline();check(bool(next),"World ending lost deadline at "+stage);now=std::max(now,*next);
   audio->advance(now);if(flow->update())acquire();
  }
  if(!flow->done())throw std::runtime_error("World ending stalled at "+flow->segmentName());
 }
 double flip(const char *stage,unsigned iteration) const {
  for(const auto &t:trace)if(t.event=="flip" && t.segment==stage && t.iteration==iteration)return double(t.nanoseconds)/1e9;
  throw std::runtime_error(std::string("Missing World ending pose: ")+stage);
 }
};
// Six-bit VGA RGB CRCs of frames that match worldending.avi exactly (M107
// capture, docs/rules/evidence.md#m107-capture). Image extraction and the
// full-frame comparison stay outside the repository.
void capturedFrames(const Run &run) {
 struct Key {const char *stage;unsigned n;std::uint32_t crc;};
 for(const auto &k:std::vector<Key>{{"sky hold",0,3340066314u},{"dragon approaches",9,1991101893u},{"dragon lands",39,1475603181u},
  {"crowd scroll",30,195759539u},{"crowd gathers",28,522673272u},{"CD 1",58,2643965724u},{"CD 2",150,3335184200u},
  {"CD 3 palette",0,1379129934u},{"CD 3",267,2551832543u},{"crowd sits",25,1547510659u},{"Dragon Pharaoh",29,2248031946u},
  {"Queen Kalindra",17,3698437000u},{"Cube rises",16,2512876742u},{"Cube rises",33,2927254862u},{"Cube shines",16,974460491u},
  {"Prince Roland",14,1648611281u},{"Sceptre rises",28,4174852992u},{"Cube placed",0,2627762767u},{"hands turn",1,2238073531u},
  {"table glows",16,3101731934u},{"Sceptre placed",34,11521164u},{"light",60,4122910556u},{"beam fades",14,2816226884u},
  {"power lines",51,3403826832u},{"collectors background",0,3264478020u},{"collectors",77,2817026427u},{"arcs",88,869824882u},
  {"globe",137,2132010143u},{"flash hold",0,4045493899u},{"Xeen united",79,3957588780u}})
  check(run.pictures.at({k.stage,k.n})==k.crc,std::string("World ending frame differs from DOS capture: ")+k.stage+" "+std::to_string(k.n));
 static const std::uint32_t pages[2][32]{
  {3859735081u,2216822283u,3601417089u,2376974932u,178294715u,3001639002u,1710771322u,2653972040u,1378456931u,322458561u,2520628974u,3845766051u,188843179u,3727213094u,1957544109u,4281687331u,2271970599u,777146140u,3009293762u,158625524u,465101266u,4080833461u,3636364517u,3009293762u,158625524u,465101266u,3450706375u,1718850829u,2317808547u,3082387228u,3523437888u,1412743003u},
  {2009204109u,570368984u,3536418205u,2311976008u,247511463u,3062453830u,1637382758u,1904600369u,3181123098u,4237152440u,687337745u,2872126229u,953282217u,905886256u,1047498102u,3052867832u,645692825u,2897395901u,834576995u,2341790037u,2572709491u,1910250516u,1516112708u,834576995u,2341790037u,2572709491u,1330452070u,3345247667u,724634397u,380536738u,1930731006u,4112730085u}};
 for(unsigned phase=0;phase<32;++phase) {
  check(run.pictures.at({"World opening page 1",phase})==pages[0][phase],"World opening Pharaoh page differs from DOS capture");
  check(run.pictures.at({"World score page 1",phase})==pages[1][phase],"World score Pharaoh page differs from DOS capture");
 }
 // Capture seconds after the first complete landing frame (21.873032 s in
 // worldending.avi); measured residuals stay within 0.12 s. Final caption
 // iteration 47 is its 80th publication, after the 32 fade-in levels.
 struct Pose {const char *stage;unsigned n;double native;};
 const double origin=run.flip("dragon lands",0);
 for(const auto &p:std::vector<Pose>{{"dragon approaches",9,-4.508727},{"crowd scroll",30,6.049684},{"crowd gathers",28,10.986455},
  {"CD 1",0,11.129136},{"CD 2",150,34.100814},{"CD 3",267,76.933719},{"crowd sits",25,82.155852},{"Dragon Pharaoh",29,88.034318},
  {"Cube rises",33,93.79864},{"Prince Roland",14,103.743522},{"Sceptre rises",28,106.925313},{"Cube lowers",70,124.560713},
  {"table glows",16,128.855418},{"Sceptre placed",34,137.573241},{"light",60,151.855632},{"beam fades",14,160.15968},
  {"power lines",51,166.865697},{"collectors",77,175.426571},{"arcs",88,186.127663},{"globe",137,202.236374},
  {"Xeen united",47,209.313363},{"World score page 1",0,213.893431}})
  check(std::abs(run.flip(p.stage,p.n)-origin-p.native)<=.25,std::string("DOS capture bracket: ")+p.stage);
 // Page acknowledgement to landing, including the unpolled song-fade wait:
 // 10.287 s in the capture.
 check(std::abs(origin-run.flip("opening page fade out",0)-10.287)<=.25,"Opening song-fade wait differs from DOS capture");
}
void segmentRules(const XeenCutsceneData &data) {
 using I=XeenCutscene::Segment::Input;
 const auto scene=XeenCutscene::worldEnding(data,0);
 const auto find=[&](const char *name)->const XeenCutscene::Segment & {
  const auto it=std::find_if(scene.segments.begin(),scene.segments.end(),[&](const auto &s){return s.name==name;});
  check(it!=scene.segments.end(),std::string("Missing World ending segment ")+name);return *it;
 };
 check(scene.timedControlQuit,"World ending lost EXE Ctrl-Q polling");
 // Unpolled song-fade waits and white flash; continuously polled holds.
 for(const char *name:{"opening song fade","final song fade","flash hold"})check(find(name).input==I::Unpolled,std::string("EXE wait polls keys: ")+name);
 for(const char *name:{"opening song fade","final song fade"})check(find(name).songFadeWait,"Song-fade wait lost command 0xffe0");
 for(const auto &[name,ticks]:std::vector<std::pair<const char *,unsigned>>{{"sky hold",60},{"Sceptre hold",30},{"table hold",30}})
  check(find(name).continuousKeyPoll && find(name).ticks==ticks,std::string("EXE hold: ")+name);
 for(const char *name:{"World opening page 1","World score page 1"}) {
  const auto &page=find(name);check(page.input==I::Keyboard && page.ticks==3 && page.pollAfterHold,"Pharaoh page keyboard/three ticks");
 }
 for(const auto &s:scene.segments)if(s.frames>1 && s.ticks)check(s.input==I::Ignore,"Animation admits skip keys: "+s.name);
 const auto &w=data.worldEnding;
 check(w.cd(1,3)=="cd1d1.eg2" && w.cd(2,25)=="cd2z1.eg2" && w.cd(2,26)=="cd2a2.eg2" && w.cd(2,34)=="cd2i2.eg2" &&
  w.cd(3,0)=="cd3a1.eg2" && w.cd(3,26)=="cd3a2.eg2" && w.cd(3,66)=="cd3o3.eg2","EXE CD animation member names");
 check(w.captions[0].find("And so the call")==0 && w.captions[7].find("\nWith the prophecy")==0 && w.captions[7].back()=='.',"EXE caption texts");
 const auto score=XeenCutscene::worldEnding(data,1234567890);
 const auto page=std::find_if(score.segments.begin(),score.segments.end(),[](const auto &s){return s.name=="World score page 1";});
 check(std::get<XeenCutscene::Text>(page->compose({0,0,0,false}).back()).value.find("1234567890")!=std::string::npos,"W replay score page");
}
}
int main(int argc,char **argv) {try {
 check(argc==4 && std::string(argv[3])=="--main","World ending test requires original data, output directory and --main");
 const auto installation=*xeenTestInstallationDetector().detect(argv[1]);XeenAssetSource assets(installation,320,200);
 const auto data=XeenCutsceneData::installed(installation);
 const XeenFontFormat font(assets.readArchiveResource("fnt",XeenSceneArchive::DarksideOnly));
 const std::filesystem::path out=argv[2];std::filesystem::create_directories(out);
 segmentRules(data);
 Run run(installation,assets,font,data,out);run.finish();
 std::ofstream trace(out/"trace.tsv");for(const auto &t:run.trace)trace<<t.event<<'\t'<<t.segment<<'\t'<<t.iteration<<'\t'<<t.nanoseconds<<'\t'<<t.tick<<'\t'<<t.reset<<'\n';
 capturedFrames(run);
 std::uint64_t previous=0;
 for(const char *stage:{"World opening page 1","opening song fade","sky hold","dragon approaches","dragon lands","crowd scroll","crowd gathers",
  "CD 1","CD 2","CD 3","crowd sits","Dragon Pharaoh","Queen Kalindra","Cube rises","Cube shines","Pharaoh watches","Prince Roland","Sceptre rises",
  "Pharaoh presides","table","Cube placed","hands turn","Cube lowers","table activates","table glows","table closes","Sceptre placed","light","beam",
  "beam fades","power lines","collectors","arcs","globe","flash hold","Xeen united","final song fade","World score page 1","World ending shutdown"}) {
  check(run.first.count(stage) && run.first.at(stage)>previous,std::string("DOS World ending stage order: ")+stage);previous=run.first.at(stage);
 }
 for(const auto &[stage,count]:std::vector<std::pair<const char *,unsigned>>{{"dragon approaches",50},{"dragon lands",40},{"crowd scroll",64},
  {"crowd gathers",29},{"CD 1",59},{"CD 2",273},{"CD 3",268},{"crowd sits",26},{"Dragon Pharaoh",30},{"Queen Kalindra",18},{"Cube rises",39},
  {"Cube shines",17},{"Prince Roland",15},{"Sceptre rises",29},{"Cube placed",5},{"hands turn",16},{"Cube lowers",71},{"table activates",14},
  {"table glows",17},{"table closes",6},{"Sceptre placed",35},{"light",61},{"beam",35},{"beam fades",15},{"power lines",52},{"collectors",78},
  {"arcs",89},{"globe",138}})
  check(run.frames[stage]==count,std::string("DOS World ending frame count: ")+stage);
 check(run.frames["Xeen united"]==80+32,"DOS final caption frames and first-frame fade");
 for(const auto &[stage,ticks]:std::vector<std::pair<const char *,unsigned>>{{"dragon approaches",2},{"dragon lands",2},{"crowd scroll",1},
  {"crowd gathers",2},{"CD 1",2},{"CD 2",2},{"CD 3",2},{"crowd sits",3},{"Dragon Pharaoh",3},{"Queen Kalindra",2},{"Cube rises",2},{"Cube shines",2},
  {"Pharaoh watches",3},{"Prince Roland",2},{"Sceptre rises",2},{"Cube lowers",2},{"table activates",2},{"table glows",3},{"table closes",4},
  {"Sceptre placed",3},{"light",4},{"beam",3},{"power lines",2},{"collectors",2},{"arcs",2},{"globe",2},{"World score page 1",3}}) {
  std::optional<std::uint64_t> tick;
  for(const auto &t:run.trace)if(t.segment==stage && t.event=="compose") {
   if(tick)check(t.tick-*tick>=ticks,std::string("DOS World ending frame wait: ")+stage);tick=t.tick;
  }
 }
 std::vector<std::string> songs;std::vector<unsigned> fades;std::map<std::string,unsigned> sounds;
 for(const auto &a:run.requests) {
  if(a.kind==XeenCutscene::Audio::Song)songs.push_back(a.resource);if(a.kind==XeenCutscene::Audio::FadeSong)fades.push_back(a.value);
  if(a.kind==XeenCutscene::Audio::Digital)++sounds[a.resource];check(a.kind!=XeenCutscene::Audio::Mixer,"World ending sends no mixer command");
 }
 check(songs==std::vector<std::string>({"outday3.m","nwblksmt.m","outday3.m"}),"World ending song sequence");
 check(fades==std::vector<unsigned>({223,207,207,207,207,0}),"World ending song fades");
 check(sounds["whoosh.voc"]==4 && sounds["elect.voc"]==2 && sounds["explosio.voc"]==2 && sounds["gascompr.voc"]==5 && sounds["padspell.voc"]==4 &&
  sounds["photon.voc"]==7 && sounds.count("cast.voc") && sounds.count("comet.voc") && sounds.count("rumble.voc"),"World ending effect cues");
 // Keys and Escape during the film change nothing; the pages still need
 // the keyboard. Captions do not depend on the sound device.
 Run input(installation,assets,font,data,{},true);input.finish();
 check(input.now==run.now && input.sequence==run.sequence,"Ignored input changed World ending timing or frames");
 auto silent=data;silent.digitalDevice=0;Run quiet(installation,assets,font,silent,{});quiet.finish();
 check(quiet.pictures==run.pictures,"World ending captions depend on the sound device");
 std::cout<<"World ending order, frames, waits, capture frames and brackets, audio cues, input and device independence passed; "<<double(run.now)/1e9<<" s\n";
 return 0;
}catch(const std::exception &e){std::cerr<<e.what()<<'\n';return 1;}}
