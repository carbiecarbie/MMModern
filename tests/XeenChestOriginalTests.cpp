// M96 on original data: a picked chest opens at the end of its Event run, is
// drawn open, and keeps its MOB table index through leaving the map, MoveObj,
// Save and Load (DOS 0x28d92..0x28e87, 0x266c8; plan acceptance item 6). The
// selection those runs use also takes a picture-less record (map 47).
#include "XeenTrainingTestSupport.h"
#include "XeenTestInstallation.h"
#include "games/xeen/XeenActorApproach.h"
#include "games/xeen/XeenItemRewards.h"
#include "games/xeen/XeenOutdoorScene.h"
#include "games/xeen/XeenIndoorScene.h"
#include "games/xeen/XeenRestoreGuard.h"
#include "games/xeen/CloudsMapComposer.h"
#include <filesystem>
#include <fstream>
#include <iostream>
using namespace training_test;
namespace mmodern {
struct XeenMonsterTestAccess {
 static std::optional<XeenPresentationRequest> request(const XeenEventFlow &f) {
  return f._pending && f._pending->state.pendingPresentation ?
   std::optional<XeenPresentationRequest>(f._pending->state.pendingPresentation->request) : std::nullopt;
 }
 static bool fatal(const XeenEventFlow &f){return f._fatal || f._death;}
 // A staircase's map load, published as a Quiet transition.
 static void travel(Fixture &f,XeenCamera destination) {
  auto &flow=*f.flow;auto &encounter=*flow._encounter;
  check(encounter.journeyQuiet(),"A map load requires Quiet");
  encounter.journeySavePreimage().check();auto candidate=f.w.transitionCandidate();
  XeenPartyState party;candidate->copyEventParty(f.p,party);
  const auto flags=candidate->loadEventDestination(f.c.mapId,destination.mapId,true,f.f);
  auto events=f.in.events.load(destination.mapId);const auto arrival=candidate->prepareTransitionArrival(destination);
  auto prepared=std::make_shared<XeenRestoreGuard>(f.w,f.p,f.c,f.f);
  prepared->retainResources(encounter.journeySavePreimage());prepared->prepareSpellMapPublication(*candidate);
  prepared->cameraValue=destination;prepared->flagValues=flags.values();
  encounter.journeySavePreimage().check();f.w.checkTransitionTreasure(*candidate);
  f.w.publishTransition(*candidate);f.c=destination;f.f=flags;encounter._journeyEvents=std::move(events);
  encounter._journeyPreimage.swap(prepared);++encounter._generation;encounter._journeyPreimage->adoptMutationBoundary();
  encounter.publishArrival(arrival);encounter.holdJourneyFrame();flow._encounterFrame=encounter.ticket();f.present(flow.refresh(true));
 }
 // Scene builds through the real resolver, so drawing reads the effective index.
 static void scenes(Fixture &f,const XeenObjectVisualResolver &resolver) {
  auto draw=[&resolver](XeenWorld &w,const XeenPartyState &,const XeenCamera &c,std::uint64_t,XeenMonsterAppearance) {
   if(w.map(c.mapId).geometry.isOutdoors())XeenOutdoorScene().build(w,c,&resolver,nullptr,0);
   else XeenIndoorScene().build(w,c,&resolver,nullptr,0);
   return XeenEventFlow::Composition{frame(),false};
  };
  f.flow->_transitionCompose=draw;
  f.flow->_encounterCompose=[&f,draw](auto phase,auto appearance){return draw(f.w,f.p,f.c,phase,appearance);};
 }
};
}
namespace {
void saveBmp(const IndexedFrame &frame,const std::filesystem::path &path) {
 std::ofstream out(path,std::ios::binary);
 const auto u16=[&](unsigned n){out.put(char(n&255));out.put(char((n>>8)&255));};
 const auto u32=[&](unsigned n){u16(n&65535);u16(n>>16);};
 const unsigned stride=(frame.width*3+3)&~3u;
 out.put('B');out.put('M');u32(54+stride*frame.height);u32(0);u32(54);
 u32(40);u32(frame.width);u32(frame.height);u16(1);u16(24);u32(0);u32(stride*frame.height);u32(0);u32(0);u32(0);u32(0);
 for(int y=int(frame.height)-1;y>=0;--y) {
  for(unsigned x=0;x<frame.width;++x) {
   const auto p=frame.pixels[y*frame.width+x]*3;
   out.put(char(frame.palette[p+2]));out.put(char(frame.palette[p+1]));out.put(char(frame.palette[p]));
  }
  for(unsigned x=frame.width*3;x<stride;++x)out.put(0);
 }
 check(bool(out),"BMP output failed");
}
struct Visit:Fixture {
 Visit(Inputs &in,const XeenSaveSnapshot &source,const XeenObjectVisualResolver &resolver):Fixture(in,source) {
  XeenMonsterTestAccess::scenes(*this,resolver);
 }
 void pulse(){now+=25;flow->beginCycle(++cycle);if(auto image=flow->updatePresentation())present(*image);}
 // Each distinct request the route meets: kind, response and text.
 std::vector<std::string> *log=nullptr;
 // Acknowledges every notice, answers Yes and names member 0 in WhoWill.
 std::string settle() {
  std::string last;
  for(unsigned n=0;n<2400;++n) {
   if(flow->canSave())return "quiet";
   if(XeenMonsterTestAccess::fatal(*flow))return "fatal";
   if(flow->encounter()->combat())return "combat";
   const auto request=XeenMonsterTestAccess::request(*flow);
   if(log && request) {
    auto line="kind "+std::to_string(int(request->kind))+" response "+std::to_string(int(request->response))+
     (request->lockPicked?std::string(" lockPicked ")+(*request->lockPicked?"1":"0"):std::string())+" text "+request->text;
    for(auto &c:line)if(c=='\n' || c=='\r')c='|';
    if(line!=last)log->push_back(line);last=line;
   }
   const auto input=flow->inputContext(flow->frame().presentation());
   if(input.readyForAction && input.dialog) {
    if(request && request->response==XeenPresentationResponseRequirement::CharacterSelection)act(SelectMemberAction{0});
    else if(request && request->response==XeenPresentationResponseRequirement::YesNo)act(YesAction{});
    else if(input.dialog->anyKey)act(AcknowledgeAction{});
   }
   pulse();
  }
  return "cap";
 }
 std::string interact(){now+=125;act(InteractionAction{});return settle();}
};
// The Clouds party on `camera`, seeded with `seed`: the map's monsters are
// defeated and the first member can pick its lock (Thievery, level 30).
XeenSaveSnapshot source(Inputs &in,XeenCamera camera,std::uint32_t seed) {
 auto s=in.base();s.camera=camera;s.journey->random->state=seed;
 // The Cave of Illusion's chests (maps 42-45) stay under water until it is drained (flag 25).
 if(camera.mapId.number>=42 && camera.mapId.number<=45)s.gameFlags[25]=true;
 auto &picker=s.characters[s.activeRosterIds.front()];picker.skills[0]=1;picker.permanentLevel=30;
 picker.currentHp=XeenCharacterRules::maxHp(picker,{std::uint32_t(s.journey->context->year)});
 // The outdoor skills of the m78-playtest party (Mountaineer, Pathfinder, Swimmer).
 for(auto owner:s.activeRosterIds) {auto &c=s.characters[owner];c.skills[9]=c.skills[11]=c.skills[14]=1;}
 const auto actors=XeenActorApproach::actorsFromResources(in.maps.loadObjects(in.assets,camera.mapId),in.statistics);
 auto &table=s.map(camera.mapId).monsters;table.emplace(XeenSaveMonsterTable{std::uint16_t(actors.size()),{}});
 for(const auto &a:actors) {
  if(a.lifecycle==XeenActorLifecycle::Present)table->slots.push_back({-128,-128,XeenActorLifecycle::Defeated,a.status,true,-1});
  else table->slots.push_back({a.x,a.y,a.lifecycle,a.status,false,-1});
 }
 return s;
}
struct Chest {XeenCamera camera;std::size_t record;std::uint8_t closed,open;int closedSprite,openSprite;};
// Purse, quest items and every carried item, to see a chest's treasure arrive.
std::vector<unsigned> possessions(Visit &v) {
 std::vector<unsigned> result{v.p.purse->gold,v.p.purse->gems,v.p.food};
 for(auto n:v.p.questItems.counts())result.push_back(n);
 for(auto owner:v.p.party.activeRosterIds())for(unsigned category=0;category<4;++category)
  for(const auto &item:xeenRewardItems(v.p.roster.at(owner),static_cast<XeenRewardCategory>(category))) {
   result.push_back(item.material);result.push_back(item.id);result.push_back(item.state);result.push_back(item.frame);
  }
 return result;
}
const XeenObjectOverride *override(const XeenWorld &w,XeenObjectIdentity id) {
 const auto *s=w.sessionState().mapState(id.mapId);if(!s)return nullptr;
 for(const auto &v:s->objectOverrides)if(v.id==id)return &v;
 return nullptr;
}
// The object the scene draws for `id` from `camera`, through the real scenes.
std::optional<XeenObjectVisual> drawn(XeenWorld &w,const XeenCamera &camera,const XeenObjectVisualResolver &resolver,XeenObjectIdentity id) {
 if(w.map(camera.mapId).geometry.isOutdoors()) {
  for(const auto &c:XeenOutdoorScene().build(w,camera,&resolver,nullptr,0))
   if(const auto *o=c.object())if(o->visual.identity==id)return o->visual;
 } else for(const auto &c:XeenIndoorScene().build(w,camera,&resolver,nullptr,0))
  if(const auto *o=c.object())if(o->visual.identity==id)return o->visual;
 return std::nullopt;
}
std::string spriteName(int id) {const auto n=std::to_string(id);return std::string(3-n.size(),'0')+n+(id<100?".obj":".0bj");}
// The first seed whose pick succeeds, its Visit after the Event.
std::unique_ptr<Visit> picked(Inputs &in,const XeenObjectVisualResolver &visuals,const Chest &chest) {
 const XeenObjectIdentity id{chest.camera.mapId,chest.record};
 for(std::uint32_t seed=1;seed<40;++seed) {
  auto v=std::make_unique<Visit>(in,source(in,chest.camera,seed),visuals);
  check(v->w.effectiveObject(id).tableIndex==chest.closed && v->w.effectiveObject(id).resourceId==chest.closedSprite,
   "The chest is not closed");
  const auto before=possessions(*v);
  check(v->interact()=="quiet","The chest Event did not finish");
  if(v->w.effectiveObject(id).resourceId!=chest.openSprite)continue;
  check(possessions(*v)!=before,"The picked chest's treasure was not received");
  const auto *value=override(v->w,id);
  check(value && value->tableIndex==chest.open && value->x==chest.camera.x && value->y==chest.camera.y &&
   v->w.effectiveObject(id).tableIndex==chest.open,"The picked chest did not take its map's open slot in place");
  std::cout<<"map "<<chest.camera.mapId<<" record "<<chest.record<<": seed "<<seed<<" picks the lock; index "
   <<unsigned(chest.closed)<<" -> "<<unsigned(chest.open)<<'\n';
  return v;
 }
 throw std::runtime_error("No seed picks the chest");
}
IndexedFrame compose(Inputs &in,Visit &v,XeenCamera camera) {
 bool animated=false;
 return CloudsMapComposer().compose(in.assets,v.w,v.p,camera,{std::uint32_t(v.p.encounterContext->year)},nullptr,0,&animated);
}
XeenCamera facing(const XeenCamera &chest,XeenDirection direction) {
 constexpr int dx[]{0,1,0,-1},dy[]{1,0,-1,0};const auto d=static_cast<unsigned>(direction);
 return {chest.mapId,chest.x-dx[d],chest.y-dy[d],direction};
}
// The sprites that can hold a frame entry on Clouds: the ones giveExt picks
// (16, 58, 73) and the open chests (62, 119). Each view of each is static in
// clouds.dat, so an entry that reaches its view's start frame stays there, as
// DOS's per-object byte does (0x10cf9..0x10d55), and can be dropped.
void framedSpritesAreStatic(const XeenObjectVisualResolver &visuals) {
 for(const int sprite:{16,58,62,73,119}) {
  XeenObjectFile file;file.mapId=XeenMapIdentity(9);file.resourcePresent=true;
  file.entities.objectTable[0]=std::uint8_t(sprite);file.entities.objects.push_back({0,0,0,0,sprite});
  for(const auto facing:{XeenDirection::North,XeenDirection::East,XeenDirection::South,XeenDirection::West}) {
   const auto v=visuals.resolve(file,0,facing);
   check(v.status==XeenObjectVisualStatus::SupportedStatic && v.endFrame<=v.initialFrame,
    "A sprite that can hold a frame entry is animated");
  }
 }
}
// Acceptance 6: pick, receive the treasure, see the chest open; leave and
// return, then Save and Load with a byte-identical re-save.
void pickLeaveReturnSave(Inputs &in,const Chest &chest,const std::filesystem::path &frames) {
 const auto visuals=XeenObjectVisualResolver::load(in.assets);
 framedSpritesAreStatic(visuals);
 const XeenObjectIdentity id{chest.camera.mapId,chest.record};
 auto v=picked(in,visuals,chest);
 std::cout<<"map "<<chest.camera.mapId<<(v->w.map(chest.camera.mapId).geometry.isOutdoors()?" is outdoors\n":" is indoors\n");
 const auto view=facing(chest.camera,chest.camera.direction);
 const auto open=drawn(v->w,view,visuals,id);
 check(open && open->spriteName==spriteName(chest.openSprite) && open->resourceId==chest.openSprite,"The opened chest is not drawn open");
 // Leave the map and return: the index is the record's, not the visit's.
 XeenMonsterTestAccess::travel(*v,{28,18,4,XeenDirection::West});check(v->settle()=="quiet","Departure not Quiet");
 check(!v->w.objectFrame(id),"Leaving the map kept giveExt's frame");
 XeenMonsterTestAccess::travel(*v,chest.camera);check(v->settle()=="quiet","Return not Quiet");
 check(v->w.effectiveObject(id).resourceId==chest.openSprite && override(v->w,id) && override(v->w,id)->tableIndex==chest.open,
  "The chest closed after leaving and returning");
 // Save and Load: still open, and the save re-saves byte-identically.
 const auto saved=v->snapshot();const auto bytes=XeenSaveFormat::encode(saved);
 bool entry=false;
 for(const auto &m:saved.maps)if(m.id==id.mapId)for(const auto &o:m.objects)
  entry=entry || (o.record==chest.record && o.tableIndex==chest.open && o.x==chest.camera.x && o.y==chest.camera.y);
 check(entry,"The save does not hold the opened chest's index");
 Visit loaded(in,XeenSaveFormat::decode(bytes),visuals);
 const auto restored=drawn(loaded.w,view,visuals,id);
 check(loaded.w.effectiveObject(id).resourceId==chest.openSprite && restored && restored->spriteName==open->spriteName &&
  restored->frame==open->frame && restored->horizontalFlip==open->horizontalFlip &&
  XeenSaveFormat::encode(loaded.snapshot())==bytes,"The loaded chest is closed or does not re-save byte-identically");
 check(loaded.interact()=="quiet" && loaded.w.effectiveObject(id).resourceId==chest.openSprite &&
  override(loaded.w,id)->tableIndex==chest.open,"A second visit changed the opened chest");
 // The lid while a picked lock's counter is positive, driven as play drives
 // it: giveExt writes frame 1 (0x235f6), the pick holds the selection, and
 // each draw3d updates the byte before its composition (0x10c83..0x10cf6).
 Visit picking(in,source(in,chest.camera,1),visuals);
 const auto lidView=picking.w.map(id.mapId).geometry.isOutdoors()?chest.camera:view;
 auto &scene=picking.w.scenePresentation();
 picking.w.objectFrameDrawn(id,1);picking.w.objectFrameLocked(id);
 std::vector<std::pair<unsigned,IndexedFrame>> lids;
 for(const unsigned lid:{2u,3u,4u,1u}) {
  scene.composeFromRetained(false);scene.beginDraw();
  auto image=compose(in,picking,lidView);
  const auto command=drawn(picking.w,lidView,visuals,id);
  check(command && command->spriteName==spriteName(chest.closedSprite) && command->frame==lid,
   "The picked lid does not draw frames 1..4, one per draw3d");
  lids.emplace_back(lid,std::move(image));
 }
 for(std::size_t a=0;a<lids.size();++a)for(std::size_t b=a+1;b<lids.size();++b)
  check(lids[a].second.pixels!=lids[b].second.pixels,"Two lid frames render the same picture");
 if(frames.empty())return;
 // Rendered frames, closed and open from each side, for inspection.
 std::filesystem::create_directories(frames);
 Visit closed(in,source(in,chest.camera,1),visuals);
 const auto prefix="map"+std::to_string(unsigned(id.mapId.number))+"-";
 for(const auto &[lid,image]:lids)saveBmp(image,frames/(prefix+"lid-"+std::to_string(lid)+".bmp"));
 for(const auto direction:{XeenDirection::North,XeenDirection::East,XeenDirection::South,XeenDirection::West}) {
  const auto from=facing(chest.camera,direction);const char side="NESW"[static_cast<unsigned>(direction)];
  if(from.x<0 || from.y<0 || from.x>=16 || from.y>=16 || !drawn(loaded.w,from,visuals,id))continue;
  saveBmp(compose(in,closed,from),frames/(prefix+"closed-"+side+".bmp"));
  saveBmp(compose(in,loaded,from),frames/(prefix+"open-"+side+".bmp"));
 }
 // Standing on the chest's cell (outdoors the only place it is drawn over the terrain).
 for(const auto direction:{XeenDirection::North,XeenDirection::East,XeenDirection::South,XeenDirection::West}) {
  const XeenCamera on{chest.camera.mapId,chest.camera.x,chest.camera.y,direction};const char side="NESW"[static_cast<unsigned>(direction)];
  saveBmp(compose(in,closed,on),frames/(prefix+"on-closed-"+side+".bmp"));
  saveBmp(compose(in,loaded,on),frames/(prefix+"on-open-"+side+".bmp"));
 }
}
// One MoveObj case (map 60's (11,22) lies off its tile and never runs): map
// 44's 73 chest at (8,2) is picked open; the Event at (14,1) then moves it onto
// its own cell (line 4), which flips it. The index stays, live and saved.
void movedInPlace(Inputs &in) {
 const auto visuals=XeenObjectVisualResolver::load(in.assets);
 const Chest chest{{44,8,2,XeenDirection::North},3,1,3,73,119};
 auto v=picked(in,visuals,chest);
 const XeenObjectIdentity id{44,3};
 const bool flip=v->w.effectiveObjectFlip(id);
 XeenMonsterTestAccess::travel(*v,{44,14,1,XeenDirection::North});check(v->settle()=="quiet","(14,1) arrival not Quiet");
 const auto drawnFlip=v->w.effectiveObjectFlip(id);
 check(v->interact()=="quiet","The (14,1) Event did not finish");
 const auto *value=override(v->w,id);
 check(value && value->x==8 && value->y==2 && value->tableIndex==std::uint8_t(3) && value->flipped!=drawnFlip &&
  v->w.effectiveObject(id).resourceId==119,"MoveObj lost the opened chest's index");
 std::cout<<"map 44 record 3: flip "<<flip<<" -> drawn "<<drawnFlip<<" -> moved "<<v->w.effectiveObjectFlip(id)<<'\n';
 const auto bytes=XeenSaveFormat::encode(v->snapshot());
 Visit loaded(in,XeenSaveFormat::decode(bytes),visuals);
 value=override(loaded.w,id);
 check(value && value->tableIndex==std::uint8_t(3) && loaded.w.effectiveObject(id).resourceId==119 &&
  XeenSaveFormat::encode(loaded.snapshot())==bytes,"The moved opened chest did not round-trip");
}
}
// The m96-chest play-test route on map 9. Outdoors an object is drawn over the
// terrain only on the party's own cell (order 111; the cell ahead's mountain,
// order 105, hides it), so the party starts on the chest's cell, facing North:
// search (Space) to run its Event; East into map 13 and back; Save and Load.
void playTestRoute(Inputs &in) {
 const auto visuals=XeenObjectVisualResolver::load(in.assets);
 const XeenObjectIdentity chest{9,1};
 const auto step=[](Visit &v,auto action){v.now+=125;v.act(action);return v.settle();};
 const auto at=[](const Visit &v,int map,int x,int y,XeenDirection d) {
  return v.c.mapId==XeenMapIdentity(map) && v.c.x==x && v.c.y==y && v.c.direction==d;
 };
 for(std::uint32_t seed=1;seed<40;++seed) {
  Visit v(in,source(in,{9,15,11,XeenDirection::North},seed),visuals);
  std::vector<std::string> requests;v.log=&requests;
  const auto closed=drawn(v.w,v.c,visuals,chest);
  check(closed && closed->spriteName=="016.obj","The closed chest is not drawn on the party's cell");
  check(step(v,InteractionAction{})=="quiet","The search did not finish");
  if(v.w.effectiveObject(chest).resourceId!=62)continue;
  for(const auto &line:requests)std::cout<<"route request: "<<line<<'\n';
  const auto picked=drawn(v.w,v.c,visuals,chest);
  check(picked && picked->spriteName=="062.obj","The picked chest is not drawn open");
  check(step(v,NavigationAction::TurnRight)=="quiet" && step(v,NavigationAction::MoveForward)=="quiet" &&
   v.c.mapId==XeenMapIdentity(13),"East of the chest is not map 13");
  std::cout<<"route: seed "<<seed<<"; map 13 at ("<<v.c.x<<","<<v.c.y<<")\n";
  check(step(v,NavigationAction::TurnRight)=="quiet" && step(v,NavigationAction::TurnRight)=="quiet" &&
   step(v,NavigationAction::MoveForward)=="quiet" && at(v,9,15,11,XeenDirection::West),"West of map 13 is not the chest's cell");
  const auto open=drawn(v.w,v.c,visuals,chest);
  check(open && open->spriteName=="062.obj","The chest is not open after leaving and returning");
  const auto bytes=XeenSaveFormat::encode(v.snapshot());
  Visit loaded(in,XeenSaveFormat::decode(bytes),visuals);
  const auto reloaded=drawn(loaded.w,loaded.c,visuals,chest);
  check(reloaded && reloaded->spriteName=="062.obj" && reloaded->frame==open->frame &&
   XeenSaveFormat::encode(loaded.snapshot())==bytes,"The chest is not open after Load");
  return;
 }
 throw std::runtime_error("No seed opens the play-test chest");
}
// Volcano Cave Level 2 (map 47): record 0 stands on (8,0) and every slot of
// the map's picture table is FF; no Event record lies on the cell. DOS selects
// it anyway (indoor 0x156f0..0x15836), so a search there shows "Nothing here"
// in every facing, where skipping it showed nothing; nothing draws it.
void pictureLessSearch(Inputs &in) {
 const auto visuals=XeenObjectVisualResolver::load(in.assets);
 const XeenObjectIdentity id{47,0};
 const auto objects=in.maps.loadObjects(in.assets,id.mapId);
 check(objects.entities.objects.size()==1 && objects.entities.objects[0].x==8 && objects.entities.objects[0].y==0 &&
  !objects.entities.objects[0].hasResource() && objects.entities.objectTable[objects.entities.objects[0].tableIndex]==0xff,
  "Map 47's picture-less record changed");
 for(const auto &r:in.events.load(id.mapId).records)check(r.x!=8 || r.y!=0,"An Event record lies on map 47's (8,0)");
 for(const auto d:{XeenDirection::North,XeenDirection::East,XeenDirection::South,XeenDirection::West}) {
  Visit v(in,source(in,{47,8,0,d},1),visuals);std::vector<std::string> requests;v.log=&requests;
  check(v.w.selectObject(v.c)==id,"The picture-less record is not selected");
  check(!drawn(v.w,v.c,visuals,id),"The picture-less record is drawn");
  check(v.interact()=="quiet" && v.c.x==8 && v.c.y==0 && !v.w.isObjectDisabled(id),"The search changed the cell");
  check(requests.size()==1 && requests[0].rfind("kind "+std::to_string(int(XeenPresentationKind::NothingHere))+" ",0)==0,
   "The search did not show exactly Nothing here");
 }
}
int main(int argc,char **argv)try {
 check(argc>=3,"usage: chest-original <installation> <case> [frame directory]");
 const auto install=xeenTestInstallationDetector().detect(argv[1]);check(bool(install),"Missing installation");Inputs in(*install);
 const std::string mode=argv[2];const std::filesystem::path frames=argc>3?argv[3]:"";
 if(mode=="map70")pickLeaveReturnSave(in,{{70,2,14,XeenDirection::East},24,2,4,16,62},frames);
 else if(mode=="map9")pickLeaveReturnSave(in,{{9,15,11,XeenDirection::North},1,3,4,16,62},frames);
 else if(mode=="map44")movedInPlace(in);
 else if(mode=="route")playTestRoute(in);
 else if(mode=="map47")pictureLessSearch(in);
 else throw std::runtime_error("Unknown chest case");
 std::cout<<"Chest original "<<mode<<" passed\n";return 0;
}catch(const std::exception &e){std::cerr<<e.what()<<'\n';return 1;}
