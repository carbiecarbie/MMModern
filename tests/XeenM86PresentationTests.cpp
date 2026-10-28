#include "XeenTrainingTestSupport.h"
#include "XeenTestInstallation.h"
#include "app/XeenAudio.h"
#include "games/xeen/XeenIndoorScene.h"
#include "games/xeen/XeenEventTrigger.h"
#include "games/xeen/CloudsUiComposer.h"
#include <iostream>
using namespace training_test;
namespace {
XeenAudio *observedAudio=nullptr;
std::vector<std::string> *digitalCommands=nullptr;
}
extern "C" void __real__ZN7mmodern9XeenAudio11stopDigitalEy(XeenAudio *,std::uint64_t);
extern "C" void __wrap__ZN7mmodern9XeenAudio11stopDigitalEy(XeenAudio *audio,std::uint64_t now) {
 if(audio==observedAudio && digitalCommands)digitalCommands->push_back("stop");
 __real__ZN7mmodern9XeenAudio11stopDigitalEy(audio,now);
}
namespace mmodern {
struct XeenRestTestAccess {
 static std::optional<unsigned> fall(const XeenEventFlow &f) {return f._fallSequence?std::optional<unsigned>{f._fallSequence->image}:std::nullopt;}
 static void compose(XeenEventFlow &f,XeenEventFlow::EncounterCompose scene,XeenEventFlow::TransitionCompose arrival) {
  f._encounterCompose=std::move(scene);f._transitionCompose=std::move(arrival);
 }
};
}
namespace {
void tick(Fixture &f,unsigned milliseconds=16) {
 f.now+=milliseconds;f.flow->beginCycle(++f.cycle);
 if(auto frame=f.flow->updatePresentation())f.present(*frame);
}
void quiet(Fixture &f) {
 for(unsigned n=0;n<2000 && !f.flow->canSave();++n)tick(f);
 check(f.flow->canSave(),"M86 presentation did not settle");
}
void healthy(XeenSaveSnapshot &s) {
 for(auto owner:s.activeRosterIds) {auto &c=s.characters[owner];c.conditions.fill(0);c.currentHp=1000;}
 for(auto &map:s.maps)if(map.monsters)for(auto &a:map.monsters->slots) {
  a.x=a.y=-128;a.lifecycle=XeenActorLifecycle::Defeated;a.accounted=true;
 }
}
void rules() {
 auto top=frame(),bottom=frame();
 for(unsigned y=0;y<132;++y)for(unsigned x=8;x<224;++x) {
  top.pixels[(y+8)*320+x]=y;bottom.pixels[(y+8)*320+x]=132+y;
 }
 constexpr unsigned offsets[]{0,3,7,12,18,25,33,42,52,63,75,88,102,117,132,127,132,129,132};
 for(unsigned image=0;image<19;++image) {
  const auto art=XeenScenePresentation::fallFrame(top,bottom,image);
  for(unsigned y=0;y<132;++y)check(art.pixels[(y+8)*320+8]==std::uint8_t(y+offsets[image]),"DOS fall scrolling source differs");
  check(art.pixels[0]==top.pixels[0] && art.pixels[150*320]==top.pixels[150*320],"Fall escaped scene rectangle");
 }
 for(unsigned kind=0;kind<6;++kind)for(unsigned wall:{4u,13u}) {
  XeenWorld world([&](auto id){XeenMap m;m.geometry.id=id.number;m.geometry.wallKind=kind;
   for(auto &cell:m.geometry.cells)cell.geometry=XeenIndoorWalls{};
   xeenGet<XeenIndoorWalls>(m.geometry.cells[8*16+8].geometry).walls[0]=wall;return m;});
  world.scenePresentation().openDoor=true;
  const auto commands=XeenIndoorScene().build(world,{28,8,8,XeenDirection::North});
  const auto door=std::find_if(commands.begin(),commands.end(),[](const auto &c){return c.originalOrder==145;});
  check(door!=commands.end() && door->geometry().resourceName.find("2.fwl")!=std::string::npos &&
   door->geometry().frame==(kind?1u:10u),"Open door frame/resource differs");
 }
 XeenScenePresentation scene;scene.levitateFrame=9;scene.spotDoorsFrame=5;scene.dangerSenseFrame=7;
 scene.resetServiceBorder();check(!scene.levitateFrame && !scene.spotDoorsFrame && !scene.dangerSenseFrame,"Service border reset differs");
 XeenPartyState party;std::vector<unsigned> statues;
 CloudsUiComposer::drawBorder(party,[&](const char *resource,unsigned image,int,int y){if(std::string(resource)=="border.icn" && y==32)statues.push_back(image);},&scene);
 check(statues==std::vector<unsigned>{0,8} && scene.levitateFrame==1 && scene.spotDoorsFrame==1 && scene.dangerSenseFrame==1,"Border assembly must draw inactive statues and increment after reset");
}
void door(Inputs &in,const GameInstallation &installation) {
 auto s=in.service();healthy(s);XeenWorld probe(in.mapLoader(),in.objectLoader());bool found=false;
 for(int y=0;y<16 && !found;++y)for(int x=0;x<16 && !found;++x)for(unsigned direction=0;direction<4 && !found;++direction) {
  XeenCamera from{28,x,y,XeenDirection(direction)},to=from;
  if(XeenMovement::doorAhead(probe,from) && XeenMovement().apply(probe,to,NavigationAction::MoveForward)==XeenMovementResult::Moved &&
     !(probe.sampleCell(to.mapId,to.x,to.y)->cell->rawAttributes&kXeenAutomaticEventFlag)) {s.camera=from;found=true;}
 }
 check(found,"No original Vertigo door witness");
 Fixture f(in,s);std::vector<unsigned> cues;
 f.programState.audio=XeenAudio::installed(installation,in.assets,0,false,std::filesystem::path("mmodern-nonexistent-rom-fixture"));
 f.programState.audio->effectSink([&](unsigned cue){cues.push_back(cue);});
 unsigned openViews=0;
 XeenRestTestAccess::compose(*f.flow,[&](auto,auto) {
  if(f.w.scenePresentation().openDoor) {++openViews;check(f.c.x==s.camera.x && f.c.y==s.camera.y,"Door view follows step");
   check(cues==std::vector<unsigned>{47},"Door cue must precede full redraw and step cue");}
  return XeenEventFlow::Composition{frame(),false};
 },[](auto &,const auto &,const auto &,auto,auto){return XeenEventFlow::Composition{frame(),false};});
 f.act(NavigationAction::MoveForward);
 check(openViews && f.c.x==s.camera.x && f.c.y==s.camera.y,"Door did not retain the origin view");
 const auto before=f.w.sessionState().journeyRandom()->count;
 tick(f,1);check(f.c.x==s.camera.x && f.c.y==s.camera.y,"Door two-tick hold ended early");
 quiet(f);check(f.c.x!=s.camera.x || f.c.y!=s.camera.y,"Door step never completed");
 check(f.w.sessionState().journeyRandom()->count==before,"Door presentation drew gameplay randomness");
 std::cout<<"Vertigo door "<<s.camera.x<<','<<s.camera.y<<" facing "<<unsigned(s.camera.direction)<<'\n';
}
std::vector<std::uint8_t> fall(Inputs &in,const GameInstallation &installation,bool enabled) {
 auto s=in.service();healthy(s);s.soundOptions.efx=enabled;s.journey->context->effects[XeenPartyEffect::Levitate]=0;
 XeenConsequenceCharacters characters;for(unsigned i=0;i<6;++i)characters[i]=s.characters[s.activeRosterIds[i]];
 XeenWorld probe(in.mapLoader(),in.objectLoader());XeenTerrainArrival rule;bool found=false;
 for(int y=0;y<16 && !found;++y)for(int x=0;x<16 && !found;++x) {
  const XeenCamera camera{26,x,y,XeenDirection::North};const auto cell=probe.sampleCell(26,x,y);
  const auto candidate=probe.prepareTerrainArrival(camera,characters,*s.journey->context);
  if(candidate && candidate.falling && candidate.surface==13 && !(cell->cell->rawAttributes&kXeenAutomaticEventFlag)) {s.camera=camera;rule=candidate;found=true;}
 }
 check(found,"No original cloud fall witness");
 const auto actors=XeenActorApproach::actorsFromResources(in.maps.loadObjects(in.assets,26),in.statistics);
 auto &table=s.map(26).monsters;table.emplace(XeenSaveMonsterTable{std::uint16_t(actors.size()),{}});
 for(const auto &a:actors)table->slots.push_back({a.x,a.y,a.lifecycle,a.status,false,-1});
 healthy(s);
 Fixture f(in,s);std::vector<std::string> cues;
 f.programState.audio=XeenAudio::installed(installation,in.assets,0,false,std::filesystem::path("mmodern-nonexistent-rom-fixture"));
 f.programState.audio->effectSink([&](unsigned cue){if(cue==11 || cue==31 || cue==29)cues.push_back(std::to_string(cue));});
 f.programState.audio->digitalSink([&](const std::string &name,auto){cues.push_back(name);});
 observedAudio=enabled?f.programState.audio.get():nullptr;digitalCommands=&cues;
 f.act(WaitAction{});
 for(unsigned n=0;!XeenRestTestAccess::fall(*f.flow);++n){check(n<200,"Cloud fall never started");tick(f);}
 check(enabled?cues==std::vector<std::string>{"11","scream.voc"}:cues.empty(),"Fall start cue order/Efx gate");
 std::vector<unsigned> images;
 for(unsigned n=0;XeenRestTestAccess::fall(*f.flow);++n) {
  check(n<30,"Fall image count/timing");const auto image=*XeenRestTestAccess::fall(*f.flow);images.push_back(image);
  check(f.c.mapId==s.camera.mapId && f.p.roster.at(s.activeRosterIds[0]).currentHp==1000,"Fall published damage/destination before last image");
  if(!enabled)check(cues.empty(),"Fall Efx gate");
  else if(image<15)check(cues.size()==2,"Impact sounds precede landing");
  else check(cues==std::vector<std::string>{"11","scream.voc","stop","unnh.voc","31"},"Scream stop/unnh/FM31 order");
  const auto retained=f.flow->frame().pixels;f.present(f.flow->refresh(true));
  check(f.flow->frame().pixels==retained,"Fall repaint advances image");
  tick(f);
 }
 check(images.size()==19,"Fall must present all 19 DOS images");
 for(unsigned i=0;i<19;++i)check(images[i]==i,"Fall skipped/replayed an image");
 quiet(f);check(f.c.mapId==rule.transition.destination.mapId && f.p.roster.at(s.activeRosterIds[0]).currentHp==900,"Cloud fall destination/damage changed");
 if(enabled)check(cues.size()==11 && std::count(cues.begin()+5,cues.end(),"29")==6,"Fall damage cues must follow all impact images");
 observedAudio=nullptr;digitalCommands=nullptr;
 std::cout<<"Cloud fall "<<s.camera.mapId<<' '<<s.camera.x<<','<<s.camera.y<<" -> "<<rule.transition.destination.mapId<<'\n';
 auto after=f.snapshot();after.soundOptions.efx=true;return XeenSaveFormat::encode(after);
}
void serviceBorder(Inputs &in) {
 auto s=in.service();healthy(s);Fixture f(in,s);
 auto &scene=f.w.scenePresentation();scene.levitateFrame=8;scene.spotDoorsFrame=6;scene.dangerSenseFrame=4;
 f.enter();check(!scene.levitateFrame && !scene.spotDoorsFrame && !scene.dangerSenseFrame,"Service entry did not reset indicators");
 scene.levitateFrame=8;scene.spotDoorsFrame=6;scene.dangerSenseFrame=4;
 tick(f,200);check(!scene.levitateFrame && !scene.spotDoorsFrame && !scene.dangerSenseFrame,"Service animation full redraw did not reset indicators");
 scene.levitateFrame=scene.spotDoorsFrame=scene.dangerSenseFrame=7;
 f.act(DialogKeyAction{InputKey::Escape});quiet(f);
 check(scene.levitateFrame==7 && scene.spotDoorsFrame==7 && scene.dangerSenseFrame==7,"Service exit added a DOS-absent reset");
}
}
int main(int argc,char **argv) {try {
 check(argc==2,"usage: m86-presentation <installation>");rules();
 const auto installation=xeenTestInstallationDetector().detect(argv[1]);check(bool(installation),"Original installation absent");
 Inputs in(*installation);door(in,*installation);
 check(fall(in,*installation,true)==fall(in,*installation,false),"Fall Efx setting changed gameplay snapshot");serviceBorder(in);
 std::cout<<"M86 presentation rules and original-data door/fall passed\n";return 0;
}catch(const std::exception &error){std::cerr<<error.what()<<'\n';return 1;}}
