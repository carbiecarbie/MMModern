#include "games/xeen/XeenWorld.h"
#include "games/xeen/XeenActorApproach.h"
#include "games/xeen/XeenEventInterpreter.h"
#include "games/xeen/XeenStateEquality.h"
#include "games/xeen/XeenIndoorScene.h"
#include "games/xeen/XeenRegionalRules.h"
#include "XeenSaveTestSupport.h"
#include <algorithm>
#include <iostream>
#include <stdexcept>
using namespace mmodern;
namespace mmodern {
struct XeenTrainingTestAccess {
 static void seed(XeenWorld &w) {w._sessionState._journeyRandom=XeenJourneyRandomState{1,7,0};}
 static void developer(XeenWorld &w,XeenMapIdentity id) {w._developerEventMap=id;}
 static void defeat(XeenWorld &w,XeenMapIdentity id) {
  auto &a=w._sessionState.mutableMonsters(id).at(0);
  a.x=a.y=-128;a.hp=0;a.activated=false;a.lifecycle=XeenActorLifecycle::Defeated;
  w._sessionState._accountedMonsters.insert(a.id);
 }
};
}
namespace {
void check(bool v,const char *message) {if(!v)throw std::runtime_error(message);}
template<class F> void rejects(F f) {bool bad=false;try{f();}catch(const std::exception &){bad=true;}check(bad,"Invalid map mutation accepted");}
XeenMap tile(XeenMapIdentity id) {
 XeenMap m;m.side=id.side;m.geometry.id=id.number;
 m.geometry.neighbors=std::array<std::uint16_t,4>{std::uint16_t(id.number+2),std::uint16_t(id.number+1),0,0};
 if(id.number==54)m.geometry.neighbors.fill(0);
 const bool outdoor=id.number==23 || id.number==24;
 m.geometry.flags2=outdoor?0x8000:0;
 for(auto &c:m.geometry.cells) {
  c.rawWord=0x1234;c.rawAttributes=0x59;c.flags=0x58;
  if(outdoor){c.geometry=XeenOutdoorLayers{4,3,2,1};c.surfaceIndex=4;}
  else {XeenIndoorWalls walls;walls.walls=std::array<std::uint8_t,4>{1,2,3,4};c.geometry=walls;c.surfaceIndex=1;}
 }
 return m;
}
unsigned extraMonsters=0; // Further MOB records at (3,3+i), for stacking.
XeenObjectFile mob(XeenMapIdentity id) {
 XeenObjectFile f{id,"synthetic.mob",true,{}};
 f.entities.monsters.push_back({1,1,0,0,8});
 f.entities.monsters.push_back({100,100,0,0,8});
 for(unsigned i=0;i<extraMonsters;++i)f.entities.monsters.push_back({3,int(3+i),0,0,8});
 for(unsigned i=0;i<5;++i)f.entities.wallItems.push_back({int(i),2,0,3,7});
 f.entities.objects.push_back({20,20,0,0,7});
 return f;
}
struct Fixture {
 XeenWorld source;std::unique_ptr<XeenWorld> w;
 Fixture(XeenMapIdentity id=47,bool openOutdoor=false):source([openOutdoor](auto id){
  auto map=tile(id);if(openOutdoor){map.geometry.surfaceTypes[4]=2;for(auto &cell:map.geometry.cells)cell.geometry=XeenOutdoorLayers{4,0,0,0};}return map;
 },mob) {
  source.markEncounterSession(XeenEncounterEntry::Journey);XeenTrainingTestAccess::seed(source);
  if(id.number==23)XeenTrainingTestAccess::developer(source,id);
  w=source.transitionCandidate();std::vector<XeenMonsterRecord> stats(9);
  for(auto &s:stats){s.raw[20]=20;s.raw[24]=1;s.raw[25]=16;s.raw[26]=1;s.raw[28]=2;}
  w->stageMapMonsters(mob(id),stats);
 }
};
XeenEventRecord row(unsigned x,unsigned y,unsigned line,unsigned op,std::vector<std::uint8_t> p={}) {
 XeenEventRecord r;r.x=x;r.y=y;r.direction=4;r.line=line;r.opcode=op;r.parameters=std::move(p);r.lengthField=5+r.parameters.size();return r;
}
void operations(XeenSide side) {
 const XeenMapIdentity id{side,47};Fixture f(id);const auto random=f.w->sessionState().journeyRandom();
 // Clouds maze0047.evt @150/@159: hide record 0, put record 4 at (13,13).
 f.w->applyMoveWallObject({id,0},-128,-128);f.w->applyMoveWallObject({id,4},13,13);
 f.w->applyMoveWallObject({id,4},13,13);
 const auto item=f.w->effectiveWallItem({id,4});
 check(item.x==13 && item.y==13 && item.direction==3 && item.resourceId==7 &&
  f.w->objectFile(id).entities.wallItems[4].x==4,"MoveWallObj changed metadata, flipped or edited MOB");
 rejects([&]{f.w->applyMoveWallObject({id,5},1,1);});
 // maze0054.evt @701: neighbour DAT cell (25,5), west wall 8.
 f.w->applyAlterMap(id,25,5,3,8);
 check(f.w->sampleCell(id,25,5)->mapId==XeenMapIdentity(side,48) &&
  f.w->sampleCell(id,25,5)->cell->rawWord==0x1238 &&
  f.w->sampleCell(id,26,5)->cell->rawWord==0x1234,"AlterMap wrote an opposite face or wrong DAT");
 f.w->applyAlterMap(id,4,5,4,8);
 check(f.w->sampleCell(id,4,5)->cell->rawWord==0x8888 && f.w->sampleCell(id,4,6)->cell->rawWord==0x1234,
  "AlterMap all directions changed neighbouring cells");
 f.w->applyAlterMap({side,54},25,5,3,8);
 check(f.w->sampleCell({side,54},9,5)->cell->rawWord==0x1238,
  "Original map-54 missing-neighbour operand did not alias the selected DAT");
 // maze0047.evt @175: indoor surface 6, preserve 0x58 attribute flags.
 f.w->applyAlterCellFlag(id,12,7,6);f.w->applyAlterCellFlag(id,25,5,5);
 check(f.w->sampleCell(id,12,7)->cell->rawAttributes==0x5e && f.w->sampleCell(id,12,7)->cell->rawWord==0x1234 &&
  f.w->sampleCell({side,48},9,5)->cell->surfaceIndex==5 && f.w->sampleCell(id,9,5)->cell->surfaceIndex==1,
  "Indoor surface aliasing lost the physical DAT or flags");
 auto revisit=f.w->transitionCandidate();revisit->leaveMap(id);revisit->discardMapCache();
 check(revisit->sampleCell(id,25,5)->cell->rawWord==0x1238 && revisit->effectiveWallItem({id,4}).x==13,
  "Indoor neighbour or wall-item state lost on leave/return");
 check(f.w->sessionState().journeyRandom()==random,"Map mutations consumed Journey draws");
}
void calledDeletion() {
 Fixture f;XeenPartyState party;XeenGameFlags flags;XeenEventInterpreter vm;
 XeenEventFile file{47,"synthetic.evt",true,{row(13,13,0,25,{100,100,0}),row(13,13,1,34,{0,9,9}),
  row(100,100,0,46),row(100,100,1,34,{4,13,13}),row(100,100,2,26)}};
 auto other=row(13,13,2,18);other.direction=2;file.records.push_back(other);
 const auto result=vm.execute({47,13,13,XeenDirection::North},party,flags,*f.w,[&](auto){return XeenEventScript(file);});
 check(std::holds_alternative<XeenEventExecutionCompleted>(result),"Called MakeNothingHere did not complete");
 check(f.w->effectiveWallItem({47,4}).x==13 && f.w->effectiveWallItem({47,0}).x==0,
  "MakeNothingHere exited the called script or executed the deleted caller suffix");
 for(unsigned n:{0u,1u,5u})check(f.w->isEventDisabled({47,n}),"Physical cell facing/line survived deletion");
 check(!f.w->isEventDisabled({47,2}),"MakeNothingHere deleted the logical CallEvent cell");
 // AlterEvent also uses physical cell and facing, including nonzero re-enable.
 f.w->applyAlterEvent({47,13,13,XeenDirection::North},1,34,file);
 check(f.w->effectiveEvent({47,1},file.records[1]).opcode==34 && !f.w->isEventDisabled({47,1}),"AlterEvent did not re-enable physical line");
 f.w->applyAlterEvent({47,13,13,XeenDirection::North},2,34,file);
 check(f.w->isEventDisabled({47,5}),"AlterEvent ignored physical facing");
 f.w->applyAlterEvent({47,13,13,XeenDirection::North},99,34,file); // DOS no-op.
 const auto replaced=vm.execute({47,13,13,XeenDirection::North},party,flags,*f.w,[&](auto){return XeenEventScript(file);});
 check(std::holds_alternative<XeenEventExecutionCompleted>(replaced) && f.w->effectiveWallItem({47,0}).x==9,
  "Nonzero AlterEvent replacement was not executed with original operands");
 // DOS sign-extends the line operand against zero-extended lines: 128-255 never match.
 Fixture high;XeenEventFile highLines{47,"synthetic.evt",true,{row(13,13,127,18),row(13,13,200,18)}};
 high.w->applyAlterEvent({47,13,13,XeenDirection::North},200,0,highLines);
 high.w->applyAlterEvent({47,13,13,XeenDirection::North},127,0,highLines);
 check(!high.w->isEventDisabled({47,1}) && high.w->isEventDisabled({47,0}),"AlterEvent line sign rule changed");
 file.records.push_back(row(13,13,3,60,{1,0,0,0,0}));file.records.push_back(row(8,8,0,1,{0}));
 f.w->applyAlterEvent({47,13,13,XeenDirection::North},3,0,file);
 check(f.w->isEventDisabled({47,6}) && f.w->isEventDisabled({47,7}),"DOS PlayCD paired-record replacement was lost");
 Fixture replacement;
 XeenEventFile stored{47,"synthetic.evt",true,{row(13,13,0,16,{4,12,12,0}),row(13,13,1,18)}};
 replacement.w->applyAlterEvent({47,13,13,XeenDirection::North},0,34,stored);
 const auto changed=vm.execute({47,13,13,XeenDirection::North},party,flags,*replacement.w,[&](auto){return XeenEventScript(stored);});
 check(std::holds_alternative<XeenEventExecutionCompleted>(changed) && replacement.w->effectiveWallItem({47,4}).x==12,
  "Nonzero replacement rejected the original unused operand suffix");
}
void outdoorAndSpawn() {
 Fixture f;auto &w=*f.w;
 w.applyAlterMap(24,1,2,0,7); // Previously retained neighbour value.
 w.applyAlterMap(23,17,2,0,8);w.applyAlterCellFlag(23,3,4,6);
 check(w.sampleCell(23,17,2)->cell->rawWord==0x8234 && w.sampleCell(23,3,4)->cell->rawWord==0x1236 &&
  w.sampleCell(23,3,4)->cell->rawAttributes==0x59,"Outdoor layer preservation failed");
 rejects([&]{w.applyAlterCellFlag(23,17,2,6);});
 w.leaveMap(23);
 check(w.sampleCell(24,1,2)->cell->rawWord==0x7234 && w.sampleCell(23,3,4)->cell->rawWord==0x1236,
  "Outdoor neighbour lifetime overwrote retained DAT state");
 const auto random=w.sessionState().journeyRandom();
 w.applySpawn(47,0,7,11,0); // maze0047.evt @9, retain type 8.
 check(w.sessionState().regionalActors(47)[0].original.resourceId==8,"Spawn changed original type");
 XeenTrainingTestAccess::defeat(w,47);w.applySpawn(47,0,7,11,0);
 check(!w.sessionState().accountedMonsters().count({47,0}) && w.sessionState().regionalActors(47)[0].hp==20 &&
  !w.sessionState().regionalActors(47)[0].activated,"Revived Spawn retained defeat accounting or wake state");
 check(w.scenePresentation().animation({47,0}),"Spawn missed presentation");
 // DOS 0x9c59: a slot at or beyond the MOB count stays inert and unsaved.
 const auto before=w.sessionState().regionalActors(47);
 w.applySpawn(47,2,5,6,0);w.applySpawn(47,4,5,6,0);
 const auto &after=w.sessionState().regionalActors(47);
 check(after.size()==before.size() && std::equal(after.begin(),after.end(),before.begin(),xeen_state::sameActor) &&
  w.sessionState().mapState(47)->originalMonsterCount==2 &&
  !w.scenePresentation().animation({47,4}) && !w.sessionState().accountedMonsters().count({47,4}),
  "Spawn beyond the MOB count materialized a monster");
 check(w.sessionState().journeyRandom()==random,"Spawn used Journey RNG");
 rejects([&]{w.applySpawn(47,107,5,6,0);});
 Fixture outside(23);outside.w->applySpawn(23,0,7,11,0);outside.w->applySpawn(23,4,5,6,0);
 check(outside.w->sessionState().regionalActors(23)[0].x==7 && outside.w->sessionState().regionalActors(23).size()==2,
  "Outdoor Spawn coordinates were not staged or a beyond-count slot appeared");
 rejects([&]{outside.w->applySpawn(23,0,128,3,0);});
 Fixture parked(23);parked.w->applySpawn(23,1,100,100,0);
 xeenValidateIndoorActors(*parked.w,23,parked.w->sessionState().regionalActors(23));
 // Cave of Illusion parks on-map monsters by Spawn to (100,100) (maze0042.evt @1764).
 Fixture scripted;scripted.w->applySpawn(47,0,100,100,0);
 xeenValidateIndoorActors(*scripted.w,47,scripted.w->sessionState().regionalActors(47));
 scripted.w->applySpawn(47,0,101,100,0);
 rejects([&]{xeenValidateIndoorActors(*scripted.w,47,scripted.w->sessionState().regionalActors(47));});
 w.applyMoveWallObject({85,0},5,6);w.applyAlterMap(85,4,5,4,0);w.leaveMap(85);
 check(w.effectiveWallItem({85,0}).x==0 && w.sampleCell(85,4,5)->cell->rawWord==0x1234,"Warzone state survived its visit");
 Fixture warzone(85);XeenEventFile warEvents{85,"synthetic.evt",true,{row(4,5,0,18)}};
 warzone.w->applyAlterEvent({85,4,5,XeenDirection::North},0,34,warEvents);
 check(warzone.w->sessionState().monsters(85).empty(),"Warzone load must clear its MOB monsters");
 warzone.w->applySpawn(85,1,5,6,0);
 check(warzone.w->sessionState().monsters(85).empty(),"Spawn must not extend the empty Warzone count");
 warzone.w->generateArena({8,8,8,8,8},2);
 warzone.w->applySpawn(85,1,5,6,0);XeenTrainingTestAccess::defeat(*warzone.w,85);
 warzone.w->leaveMap(85);
 check(!warzone.w->sessionState().mapState(85) && !warzone.w->sessionState().accountedMonsters().count({85,0}),
  "Warzone Event replacements, new slots or accounting survived leave");
}
// DOS has no per-cell cap outside movement: Spawn stacks freely (Winterkill),
// each redraw fills attacker slots in slot order (0x1692c, 0x14406) and only a
// moving monster refuses a destination holding three (0x1fc24).
// Remove leaves only the (128,128) position (scripts.cpp cmdRemove): MoveObj
// shows the object again, and no signed-byte operand matches to flip it.
void removedObjectMove() {
 Fixture f;const XeenObjectIdentity id{47,0};
 f.w->disableObject(id);
 f.w->applyMoveObject(id,-128,-128);
 check(!f.w->sessionState().isObjectDisabled(id) && f.w->isObjectDisabled(id) && !f.w->effectiveObjectFlip(id),
  "MoveObj to (-128,-128) flipped a removed object or showed it");
 f.w->disableObject(id);f.w->applyMoveObject(id,5,6);
 const auto object=f.w->effectiveObject(id);
 check(!f.w->isObjectDisabled(id) && object.x==5 && object.y==6 && !f.w->effectiveObjectFlip(id) &&
  f.w->sessionState().objectOverrides().size()==1,"MoveObj did not show the removed object at its new position");
 f.w->applyMoveObject(id,5,6);
 check(f.w->effectiveObjectFlip(id) && f.w->effectiveObject(id).x==5,"MoveObj in place no longer flips");
}
void spawnDoesNotDraw() {
 Fixture f;XeenPartyState party;XeenGameFlags flags;XeenEventInterpreter vm;
 XeenEventFile file{47,"synthetic.evt",true,{row(6,6,0,16,{0,6,6,0}),
  row(6,6,1,16,{1,6,6,0}),row(6,6,2,24,{0,0})}};
 unsigned draws=0;bool slot0=false;
 vm.eventDraw=[&](auto &world,const auto &camera,bool){++draws;slot0=bool(world.prepareTransitionArrival(camera).slots[0]);};
 vm.eventLineSkipped=[&]{return slot0;};
 const auto result=vm.execute({47,6,6,XeenDirection::North},party,flags,*f.w,[&](auto){return XeenEventScript(file);});
 check(std::holds_alternative<XeenEventExecutionCompleted>(result),"Spawn sequence did not finish naturally");
 const auto &actors=f.w->sessionState().regionalActors(47);
 check(actors[0].x==6 && actors[0].y==6 && actors[1].x==6 && actors[1].y==6,
  "Spawn sequence lost an actor at the party cell");
 check(f.w->effectiveEvent({47,0},file.records[0]).opcode==0 && draws==1,
  "Spawn drew or skipped the following AlterEvent (one tail draw is expected)");
 Fixture labelFixture;slot0=false;draws=0;
 XeenEventFile label{47,"synthetic.evt",true,{row(6,6,0,16,{0,6,6,0}),
  row(6,6,1,16,{1,6,6,0}),row(6,6,2,2,{0}),row(6,6,3,24,{0,0})}};
 const auto script=[&](auto){return XeenEventScript(label);};
 const auto text=[](auto id){return XeenEventTextFile{id,"test.txt",true,{"Label"}};};
 auto pending=vm.begin({47,6,6,XeenDirection::North},party,flags,*labelFixture.w,script,text);
 check(std::holds_alternative<XeenEventExecutionSuspended>(pending) && slot0 && draws==1,"Label did not draw after both Spawns");
 const auto state=std::get<XeenEventExecutionSuspended>(pending).state;
 pending=vm.resume(state,XeenPresentationResponse::Presented,party,*labelFixture.w,script,text);
 check(std::holds_alternative<XeenEventExecutionCompleted>(pending) && !labelFixture.w->isEventDisabled({47,0}),
  "Label draw did not make the next line-head skip AlterEvent");
}
void stacking() {
 extraMonsters=4;Fixture f;extraMonsters=0;auto &w=*f.w;
 for(std::uint8_t slot=2;slot<6;++slot)w.applySpawn(47,slot,6,6,0);
 const auto &actors=w.sessionState().regionalActors(47);
 xeenValidateIndoorActors(w,47,actors);
 check(XeenActorApproach::occupancy(actors)[6*32+6]==4,"Spawn did not stack four monsters");
 const auto view=XeenIndoorScene().classifyActors(w,{47,6,6,XeenDirection::North},actors);
 check(view.slots[0]==XeenMonsterIdentity{47,2} && view.slots[1]==XeenMonsterIdentity{47,3} &&
  view.slots[2]==XeenMonsterIdentity{47,4} && view.activation[5],"Stacked contact is not slot-ordered or the fourth is not flagged");
 for(unsigned i=0;i<3;++i)check(view.slots[i]!=XeenMonsterIdentity{47,5},"Fourth stacked monster took a front slot");
 std::vector<XeenActor> movers(actors.begin(),actors.end());movers[0].x=6;movers[0].y=8;movers[0].activated=true;
 const auto moved=XeenActorApproach::move(movers,{47,6,5,XeenDirection::North},
  [](const XeenActor &,int,int){return XeenMonsterTerrain::Allowed;},true);
 check(moved[0].x==6 && moved[0].y==7,"Mover did not approach");
 const auto again=XeenActorApproach::move(moved,{47,6,5,XeenDirection::North},
  [](const XeenActor &,int,int){return XeenMonsterTerrain::Allowed;},true);
 check(again[0].x==6 && again[0].y==7,"Movement entered a cell holding four");
}
void parkedOutdoorSpawn() {
 Fixture f(23,true);auto &w=*f.w;
 w.applySpawn(23,1,6,8,0);
 xeenValidateIndoorActors(w,23,w.sessionState().monsters(23));
 const auto &before=w.sessionState().regionalActors(23);
 std::vector<XeenActor> actors(before.begin(),before.end());actors[1].activated=true;
 const auto moved=XeenActorApproach::move(actors,{23,6,5,XeenDirection::North},
  [&](const auto &a,int x,int y){return xeenRegionalActorTerrain(w.map(23),a,x,y);},true);
 check(moved[1].x==6 && moved[1].y==7,"Spawned parking slot did not move under ordinary terrain rules");
 xeenValidateIndoorActors(w,23,moved);
 auto snapshot=save_test::currentWireSnapshot();
 auto &table=snapshot.map(23).monsters;table=XeenSaveMonsterTable{2,{}};
 for(const auto &a:moved)table->slots.push_back({a.x,a.y,a.lifecycle,a.status,false,-1});
 const auto bytes=XeenSaveFormat::encode(snapshot);const auto decoded=XeenSaveFormat::decode(bytes);
 auto restored=XeenActorApproach::actorsFromResources(mob(23),w.monsterStatistics(XeenSide::Clouds));
 for(unsigned i=0;i<restored.size();++i){restored[i].x=decoded.map(23).monsters->slots[i].x;restored[i].y=decoded.map(23).monsters->slots[i].y;}
 xeenValidateIndoorActors(w,23,restored);
 check(XeenSaveFormat::encode(decoded)==bytes && restored[1].x==6 && restored[1].y==7,
  "Parked outdoor Spawn failed exact save/load");
 // Off-map originals do not authorize arbitrary off-map current positions.
 restored[1].x=101;rejects([&]{xeenValidateIndoorActors(w,23,restored);});
 // The original-anchor check still rejects an ordinary actor across a barrier.
 auto isolated=tile(23);isolated.geometry.surfaceTypes[0]=2;for(auto &cell:isolated.geometry.cells)cell.geometry=XeenOutdoorLayers{0,0,0,0};
 for(int y=0;y<16;++y)isolated.geometry.cells[16*y+4].geometry=XeenOutdoorLayers{0,1,0,0};
 restored[0].x=6;restored[0].y=1;
 check(!xeenActorClosure(isolated,restored[0])[22],"Ordinary actor lost its original terrain closure");
}
}
int main() {try{operations(XeenSide::Clouds);operations(XeenSide::Darkside);calledDeletion();outdoorAndSpawn();removedObjectMove();spawnDoesNotDraw();stacking();parkedOutdoorSpawn();
 std::cout<<"Map Event owners, DOS continuation, physical aliases, lifetimes and Spawn passed\n";return 0;
}catch(const std::exception &e){std::cerr<<e.what()<<'\n';return 1;}}
