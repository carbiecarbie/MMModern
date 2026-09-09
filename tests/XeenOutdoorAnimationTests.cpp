#include "XeenSaveGameplayTestSupport.h"
#include "SyntheticXeenArchive.h"
#include "formats/xeen/XeenAssetSource.h"
#include "games/xeen/CloudsMapComposer.h"
#include "games/xeen/XeenCharacterRules.h"
#define SDL_MAIN_HANDLED
#include <SDL.h>
#include <chrono>
#include <iostream>

using namespace mmodern;
using remove_test::check;
using remove_test::record;
namespace {
struct Fixture {
 gameplay_test::Fixture data;
 std::uint64_t now=0;
 bool animated=true, fail=false, blocked=false, indoor=false, automatic=false;
 std::vector<std::uint64_t> phases;
 XeenCamera camera{1,1,1,XeenDirection::North};
 XeenGameFlags flags;
 XeenWorld world{[&](XeenMapIdentity id){auto m=remove_test::map(id);
  m.geometry.surfaceTypes[0]=blocked?0:1;m.geometry.neighbors[0]=id.number==1?2:1;
  m.geometry.neighbors[2]=id.number==1?2:1;
  if(automatic)for(auto &c:m.geometry.cells)c.rawAttributes=0x10;
  if(indoor && id.number==3){m.geometry.flags2=0;for(auto &c:m.geometry.cells)c.geometry=XeenIndoorWalls{};}
  return m;},[&](XeenMapIdentity id){XeenObjectFile f{id,"test.mob",true,{}};f.entities.objects={{1,1,0,0,111}};return f;}};
 XeenEventSystem events{[&](XeenMapIdentity id){return remove_test::script(id,data.scripts[id]);},
  [&](XeenMapIdentity id){auto t=data.text;t.mapId=id;return t;}};
 XeenEventFlow flow{world,events,data.initial,camera,flags,data.font,[&](std::uint64_t phase){
  phases.push_back(phase);if(fail)throw std::runtime_error("animation composition fault");
  IndexedFrame f;f.width=320;f.height=200;f.pixels.assign(64000,static_cast<std::uint8_t>(phase%251));
  return XeenEventFlow::Composition{f,animated};},{},[&]{return now;}};
 void at(std::uint64_t time){now=time;flow.updatePresentation();}
 void phase(std::uint64_t value){flow.refresh(true);check(phases.back()==value,"unexpected ordinary phase");}
};
void deadlinesAndActions(){
 Fixture f;check(f.phases==std::vector<std::uint64_t>{0},"initial zero");
 f.at(99);check(f.phases.size()==1,"early tick");f.at(100);check(f.phases.back()==1,"first deadline");
 auto n=f.phases.size();f.at(100);check(f.phases.size()==n,"same-now double tick");
 f.at(9000);f.phase(2);f.at(9099);f.phase(2);f.at(9100);f.phase(3);
 for(int i=0;i<3;++i)f.flow.refresh(true);f.phase(3);
 f.now=9199;f.flow.handle(NavigationAction::MoveForward);f.phase(4);
 check(f.camera.y==2,"forward fixture blocked");f.at(9200);f.phase(4);f.at(9299);f.phase(5);
 f.flow.handle(NavigationAction::MoveBackward);f.phase(6);check(f.camera.y==1,"backward fixture");
 f.blocked=true;f.world.discardMapCache();f.flow.handle(NavigationAction::MoveForward);f.phase(7);
 check(f.camera.y==1,"blocked movement committed");f.flow.handle(InteractionAction{});f.phase(8);
 f.flow.handle(NavigationAction::TurnRight);f.phase(0);f.flow.handle(NavigationAction::TurnLeft);f.phase(0);
 f.flow.handle(SaveGameAction{});f.flow.handle(InspectInventoryAction{});f.phase(0);
 f.at(9398);f.phase(0);f.at(9399);f.phase(1);
 f.flow.acceptManual(XeenManualEventNoEvent{});f.flow.acceptAutomatic(XeenAutomaticEventNoTrigger{});f.flow.initial();f.phase(1);
 f.camera.x=2;f.flow.refresh();f.phase(1); // Same-map relocation is a pure refresh.
 f.camera.mapId=2;f.flow.refresh();f.phase(0);f.at(9499);f.phase(1);
 f.camera.mapId=1;f.flow.refresh();f.phase(0);
}
void staticIndoorAndFailures(){
 Fixture f;f.animated=false;f.flow.refresh(true);auto n=f.phases.size();
 f.at(100);f.at(200);check(f.phases.size()==n,"static-only view recomposed on tick");
 f.animated=true;f.camera.x=2;f.flow.refresh();check(f.phases.back()==2,"offscreen phase did not continue");
 f.indoor=true;f.camera.mapId=3;f.flow.refresh();n=f.phases.size();f.at(9999);
 check(f.phases.size()==n,"indoor timed redraw");f.flow.handle(InteractionAction{});f.phase(0);
 f.camera.mapId=1;f.flow.refresh();f.phase(0);f.at(10098);f.phase(0);f.at(10099);f.phase(1);
 f.fail=true;bool threw=false;try{f.at(10199);}catch(const std::runtime_error&){threw=true;}
 check(threw && f.phases.back()==2,"no-pending animation failure swallowed");
 f.fail=false;f.flow.refresh(true);check(f.phases.back()==2,"failed tick rolled phase back");
 f.now=10200;f.camera.mapId=2;f.fail=true;
 try{f.flow.refresh();}catch(const std::runtime_error&){}
 f.now=10299;f.fail=false;f.flow.refresh();check(f.phases.back()==0,"failed transition reset phase");
 f.at(10300);check(f.phases.back()==1,"render retry rearmed transition deadline");
 n=f.phases.size();f.world.disableObject({2,0});f.at(10400);
 check(f.phases.size()==n+1 && f.phases.back()==2,"mutation and tick composed twice");
 f.camera.mapId=1;n=f.phases.size();f.at(10500);
 check(f.phases.size()==n+1 && f.phases.back()==0,"reset and idle not coalesced");
}
void committedAndPending(){
 for(bool automatic:{false,true})for(bool fail:{false,true}){
  Fixture f;f.automatic=automatic;f.world.discardMapCache();
  const int y=automatic?2:1;
  f.data.scripts[1]={record(1,y,0,0x1f,{2,1,1})};
  f.data.scripts[2]={record(1,1,0,9,{44,0,1}),record(1,1,1,fail?0xff:0x12)};
  f.events.discardScriptCache();f.at(100);
  f.flow.handle(automatic?PlayerAction(NavigationAction::MoveForward):PlayerAction(InteractionAction{}));
  check(f.flow.blocksGameplay() && f.camera.mapId==1 && f.camera.y==y,"working teleport published");f.phase(2);
  auto n=f.phases.size();f.flow.handle(NavigationAction::TurnLeft);check(f.phases.size()==n,"pending navigation stepped");
  f.at(200);f.phase(3);f.flow.handle(NoAction{});
  check(!f.flow.blocksGameplay() && f.camera.mapId==(fail?1:2),"teleport failure publication");f.phase(fail?3:0);
 }
 // Committed movement A->B followed by immediate B->A cannot be inferred from final camera.
 Fixture f;f.camera.y=15;f.automatic=true;f.world.discardMapCache();f.flow.refresh();f.at(100);
 f.data.scripts[2]={record(1,0,0,7,{1,1,15})};f.events.discardScriptCache();
 auto n=f.phases.size();f.flow.handle(NavigationAction::MoveForward);
 check(f.camera.mapId==1 && f.camera.y==15 && f.phases.size()==n+1 && f.phases.back()==0,"round-trip map entry missed");
 // Reentrant reports and reconstruction do not consume the action again.
 f.flow.reportManual=[&](const auto&){f.flow.handle(InteractionAction{});f.flow.updatePresentation();f.flow.refresh(true);};
 f.flow.handle(InteractionAction{});f.phase(1);
 // Same-map event relocation keeps the enclosing action's single step.
 f.data.scripts[1]={record(1,15,0,7,{1,2,14})};f.events.discardScriptCache();
 f.flow.handle(InteractionAction{});check(f.camera.x==2 && f.camera.y==14,"same-map teleport fixture");f.phase(2);
}

void responseLayers(){
 for(int kind=0;kind<3;++kind){
  Fixture f;f.data.text.strings[1]=std::string(900,'W');
  auto request=record(1,1,0,1,{1});
  if(kind==1)request=record(1,1,0,9,{44,0,1});
  if(kind==2)request=record(1,1,0,0x20,{0,1});
  f.data.scripts[1]={request,record(1,1,1,0x12)};f.events.discardScriptCache();
  unsigned reports=0;std::optional<XeenPresentationResponseRequirement> response;
  f.flow.reportManual=[&](const auto&r){if(const auto *s=std::get_if<XeenEventExecutionSuspended>(&r)){++reports;response=s->request.response;}};
  f.flow.handle(InteractionAction{});check(f.flow.blocksGameplay() && response,"response layer fixture absent");
  if(kind==0)f.flow.handle(AcknowledgeAction{});
  const auto gen=f.flow.presentationGeneration();const auto page=f.flow.presenter().pageIndex();const auto count=f.phases.size();
  const auto inventory=xeenInventoryInspection(f.data.initial);const auto oldReports=reports;
  IndexedFrame base;base.width=320;base.height=200;base.pixels.assign(64000,2);
  auto oracle=f.flow.presenter();const auto expected=oracle.rebase(base);
  f.at(100);
  check(f.phases.size()==count+1 && f.phases.back()==2 && f.flow.frame().pixels==expected.pixels,"response-layer animated rebase");
  check(f.flow.presentationGeneration()==gen && f.flow.presenter().pageIndex()==page && f.flow.blocksGameplay() &&
   reports==oldReports && xeenInventoryInspection(f.data.initial)==inventory,"idle changed response or delivered rewards");
  f.flow.abandonPresentation();
  if(kind!=0)check(f.flow.frame().pixels==base.pixels,"transient layer did not reveal current animation base");
 }
}

using sprite_test::Bytes;
struct RealScene {
 std::filesystem::path dir=std::filesystem::temp_directory_path()/("mmodern-22b-"+std::to_string(std::chrono::steady_clock::now().time_since_epoch().count()));
 GameInstallation installation;
 RealScene(bool corrupt=false){
  using namespace sprite_test;std::filesystem::create_directories(dir);
  Bytes empty;word(empty,41);for(int i=0;i<41;++i){word(empty,166);word(empty,0);}auto c=cell(0,0,0,0,{});empty.insert(empty.end(),c.begin(),c.end());
  std::map<std::string,Bytes> files;
  for(const char *name:{"sky.sky","water.out","water.srf","space.srf","global.icn","border.icn","fecp.brd","bless.icn","restorex.icn","main.icn"})files[name]=empty;
  files["back.raw"]=Bytes(64000,99);files["mm4.pal"]=Bytes(768);
  auto solid=[](unsigned color){Bytes rows;for(int y=0;y<20;++y){rows.push_back(22);rows.push_back(0);rows.push_back(19);rows.insert(rows.end(),20,color);}return cell(60,20,70,20,rows);};
  files["110.0bj"]=multiFrameSprite({{solid(17),{}},{corrupt?cell(0,8,0,1,{2,0,1}):solid(18),{}},{solid(19),{}}});
  Bytes metadata(1452);for(int d=0;d<4;++d)metadata[110*12+8+d]=3;
  archive(dir/"xeen.cc",files);archive(dir/"dark.cc",{{"clouds.dat",metadata}});
  installation.xeenArchive=dir/"xeen.cc";installation.darkArchive=dir/"dark.cc";
 }
 ~RealScene(){std::error_code e;std::filesystem::remove_all(dir,e);}
};
void realComposition(bool sdl){
 RealScene resources;XeenAssetSource assets(resources.installation,320,200);CloudsMapComposer composer;
 gameplay_test::Fixture f;auto services=f.services();std::uint64_t now=0;
 std::vector<std::uint64_t> phases;std::vector<IndexedFrame> frames;std::vector<std::size_t> selected;
 services.clock=[&]{return now;};
 services.resources.loadInitialParty=[] { return XeenPartyState{}; };
 services.maps=[](auto id) { return remove_test::map(id); };
 services.objects=[](XeenMapIdentity id){XeenObjectFile o{id,"test.mob",true,{}};o.entities.objects={{1,1,0,0,110}};return o;};
 services.compose=[&](XeenWorld &w,const XeenPartyState &p,const XeenCamera &c,std::uint64_t phase){
  XeenEventFlow::Composition result;result.frame=composer.compose(assets,w,p,c,{},nullptr,phase,&result.containsOrdinaryAnimation);
  const auto resolver=XeenObjectVisualResolver::load(assets);auto commands=XeenOutdoorScene().build(w,c,&resolver,nullptr,phase);
  auto object=std::find_if(commands.begin(),commands.end(),[](const auto &v){return v.object()!=nullptr;});
  check(object!=commands.end() && result.containsOrdinaryAnimation,"real animated command absent");
  phases.push_back(phase);selected.push_back(object->object()->visual.frame);frames.push_back(result.frame);return result;
 };
 services.show=[&](const auto &first,const auto &handle,const auto &escape,const auto &idle,const auto &status){
  check(phases==std::vector<std::uint64_t>{0},"Application first composition phase");
  if(!sdl){for(int i=1;i<=3;++i){now=i*100;check(bool(idle()),"real idle did not publish");}return true;}
  const auto started=SDL_GetTicks();bool done=false;
  const bool ok=SdlWindow().showInteractive(first,"Ordinary animation SDL",handle,escape,[&]()->std::optional<IndexedFrame>{
   check(SDL_GetTicks()-started<5000,"ordinary SDL watchdog");now+=25;auto changed=idle();
   if(phases.back()>=3 && !done){done=true;SDL_Event q{};q.type=SDL_QUIT;SDL_PushEvent(&q);}return changed;
  },status);
  return ok && done;
 };
 check(Application().playGameplay(services,{1,1,1,XeenDirection::North},{},false)==0,"Application real animation run");
 check(phases==std::vector<std::uint64_t>({0,1,2,3}) && selected==std::vector<std::size_t>({0,1,2,0}),"logical idle cycle");
 check(frames[0].pixels!=frames[1].pixels && frames[1].pixels!=frames[2].pixels && frames[0].pixels==frames[3].pixels,"real cycle pixels/wrap");
 std::cout<<"Application/Flow/"<<(sdl?"SDL":"direct")<<" phase 0/1/2/3, selected frames 0/1/2/0 and native pixel cycle passed\n";
}
void warmedMalformedFrame(){
 RealScene resources(true);XeenAssetSource assets(resources.installation,320,200);CloudsMapComposer composer;
 Fixture f;std::uint64_t now=0;std::vector<std::uint64_t> phases;
 XeenWorld world([](auto id){return remove_test::map(id);},[](auto id){XeenObjectFile o{id,"test.mob",true,{}};o.entities.objects={{1,1,0,0,110}};return o;});
 XeenEventFlow flow(world,f.events,f.data.initial,f.camera,f.flags,f.data.font,[&](std::uint64_t phase){phases.push_back(phase);XeenEventFlow::Composition r;r.frame=composer.compose(assets,world,{},f.camera,{},nullptr,phase,&r.containsOrdinaryAnimation);return r;},{},[&]{return now;});
 const auto loads=assets.spriteLoadCount();now=100;bool failed=false;
 try{flow.updatePresentation();}catch(const std::runtime_error&){failed=true;}
 check(failed && phases.back()==1 && assets.spriteLoadCount()==loads,"warm malformed later frame silently accepted");
 now=200;check(bool(flow.updatePresentation()) && phases.back()==2,"later valid selected frame did not use logical decision");
}
}
int main(int argc,char **argv){try{
 const bool sdl=argc>1 && std::string(argv[1])=="sdl";
 if(!sdl){deadlinesAndActions();staticIndoorAndFailures();committedAndPending();responseLayers();warmedMalformedFrame();}
 realComposition(sdl);std::cout<<"Ordinary outdoor animation tests passed\n";return 0;
}catch(const std::exception &e){std::cerr<<e.what()<<'\n';return 1;}}
