#include "games/xeen/XeenMovement.h"
#include "games/xeen/XeenGameplayContext.h"
#include "games/xeen/XeenRegionalRules.h"
#include "games/xeen/XeenIndoorScene.h"
#include "formats/xeen/XeenMapFormat.h"
#include "formats/xeen/XeenEventFormat.h"
#include <map>
#include <iostream>
#include <limits>
#include <stdexcept>

using namespace mmodern;
namespace {
void check(bool value, const char *message) { if (!value) throw std::runtime_error(message); }
template<class F> void rejects(F f) {
	bool rejected = false;
	try { f(); } catch (const std::exception &) { rejected = true; }
	check(rejected,"Invalid value was admitted");
}
void geometry() {
	XeenMap map;
	map.geometry.id=23; map.geometry.flags2=0x8000;
	for (unsigned i=0;i<16;++i) map.geometry.surfaceTypes[i]=i;
	for (auto &c:map.geometry.cells) c.geometry=XeenOutdoorLayers{2,0,0,0};
	check(XeenMovement::component(map,9,11,{}).count()==256,"Open component");
	for (int y=0;y<16;++y) map.geometry.cells[y*16+7].geometry=XeenOutdoorLayers{2,1,0,0};
	check(XeenMovement::component(map,9,11,{}).count()==128,"Disconnected halves");
	map.geometry.cells[6*16+7].geometry=XeenOutdoorLayers{2,0,0,0};
	check(XeenMovement::component(map,9,11,{}).count()==241,"Changed passage must change component");
	rejects([&]{ XeenMovement::component(map,7,7,{}); });
	for (unsigned middle=0;middle<16;++middle) for (unsigned surface=0;surface<16;++surface) {
		auto &cell=map.geometry.cells[0];
		cell.geometry=XeenOutdoorLayers{static_cast<std::uint8_t>(surface),static_cast<std::uint8_t>(middle),0,0};
		const bool mountain=middle==1 || middle==7 || middle==9 || middle==10 || middle==12;
		const bool checks=middle==0 || middle==2 || middle==4 || middle==5 || middle==8 || middle==11 || middle==13 || middle==14;
		const auto expected=mountain ? XeenMovementResult::BlockedByTerrain : checks && (surface==0 || surface==8 || surface==15) ? XeenMovementResult::BlockedBySurface : XeenMovementResult::Moved;
		check(XeenMovement::outdoorDestination(map.geometry,cell,{})==expected,"Literal collision precedence");
	}
	for (int y=0;y<16;++y) for (int x=0;x<16;++x)
		for (const auto d:{std::pair<int,int>{1,0},{-1,0},{0,1},{0,-1}}) {
			const int nx=x+d.first,ny=y+d.second;
			const auto actual=XeenMovement::localOutdoor(map,x,y,nx,ny,{});
			check(actual==(nx<0 || nx>=16 || ny<0 || ny>=16 ? XeenMovementResult::BlockedByMapBoundary :
				XeenMovement::outdoorDestination(map.geometry,map.geometry.cells[ny*16+nx],{})),"Every local edge");
		}
	map.side=XeenSide::Darkside;
	rejects([&]{ XeenMovement::component(map,9,11,{}); });
}
void calendar() {
	XeenGameplayContext c; c.year=610;c.day=8;c.minutes=480;c.ctr24=23;
	const auto ordinary=xeenPrepareTime(c,10);
	check(ordinary.context.minutes==490 && ordinary.context.ctr24==23 && !ordinary.requiresEffects(),"Time and step counter are independent");
	check(xeenPrepareTime(c,0).context==c,"Zero charge exact identity");
	for (unsigned minute:{479U,959U}) {
		c.minutes=minute;
		check(xeenPrepareTime(c,1).processing480==1,"480-minute processing");
	}
	c.minutes=1259;check(xeenPrepareTime(c,1).dusks==1,"Dusk crossing");
	c.minutes=299;check(xeenPrepareTime(c,1).dawns==1,"Dawn crossing");
	c.minutes=1439;c.day=99;
	const auto year=xeenPrepareTime(c,1);
	check(year.context.minutes==0 && year.context.day==0 && year.context.year==611 && year.context.newDay &&
		year.midnights==1 && year.yearRollovers==1,"Calendar rollover");
	c.day=8;c.minutes=480;
	const auto days=xeenPrepareTime(c,3*1440);
	check(days.processing480==1 && days.midnights==3 && days.dawns==3 && days.dusks==3,"All multi-day crossings retained");
	c.year=65535;c.day=99;c.minutes=1439;
	rejects([&]{ xeenPrepareTime(c,1); });
	rejects([&]{ xeenPrepareTime(c,std::numeric_limits<std::uint64_t>::max()); });
	c.year=611;c.day=42;c.minutes=1000;
	check(xeenRegionalContext(c),"Noninitial daytime value");
	check(!xeenPrepareTime(c,10).requiresEffects(),"Other processing interval");
	c.newDay=true;check(xeenRegionalContext(c),"Pending daily state is canonical");
	check(xeenPrepareTime(c,1).dailyProcessing==1,"Pending daily work retained");
}
void actors() {
	XeenMap map;map.geometry.id=23;map.geometry.flags2=0x8000;
	for (unsigned i=0;i<16;++i) map.geometry.surfaceTypes[i]=i;
	for (auto &c:map.geometry.cells) c.geometry=XeenOutdoorLayers{2,0,0,0};
	XeenActor a;a.id={23,0};a.original.x=0;a.original.y=5;a.original.resourceId=6;
	a.x=0;a.y=5;a.statistics=XeenMonsterRecord{};a.statistics->raw[20]=25;a.statistics->raw[32]=1;
	a.hp=17;a.activated=true;a.lifecycle=XeenActorLifecycle::Present;
	check(!a.statistics->supportsMovement() && a.statistics->supportsGroundMovement(),"Ranged profile independent movement admission");
	map.geometry.cells[5*16].geometry=XeenOutdoorLayers{2,1,0,0};
	check(xeenActorClosure(map,a).count()==256,"Mountain spawn can leave into adjacent ground");
	check(xeenRegionalActorTerrain(map,a,0,5)==XeenMonsterTerrain::Blocked,"Mountain destination blocked");
	check(xeenRegionalActorTerrain(map,a,-1,5)==XeenMonsterTerrain::Unsupported,"No actor map transfer");
	for (unsigned s:{0U,8U,15U}) {
		map.geometry.cells[0].geometry=XeenOutdoorLayers{static_cast<std::uint8_t>(s),0,0,0};
		check(xeenRegionalActorTerrain(map,a,0,0)==XeenMonsterTerrain::Blocked,"Nonflying hazard terrain blocks");
	}
	XeenCamera camera{23,5,5,XeenDirection::West};a.x=7;a.y=5;
	check(xeenOutdoorRangedRay(map,camera,a),"Clear east ray from behind");
	map.geometry.cells[5*16+6].rawWord=8;
	check(!xeenOutdoorRangedRay(map,camera,a),"East raw wall mask");
	map.geometry.cells[5*16+6].rawWord=0;
	map.geometry.cells[5*16+6].geometry=XeenOutdoorLayers{2,3,0,0};
	check(xeenOutdoorRangedRay(map,camera,a),"East ray is not a middle test");
	a.x=3;map.geometry.cells[5*16+4].geometry=XeenOutdoorLayers{2,3,0,0};
	check(!xeenOutdoorRangedRay(map,camera,a),"West ray blocks on middle 3");
	map.geometry.cells[5*16+4].geometry=XeenOutdoorLayers{2,0,0,0};
	check(xeenOutdoorRangedRay(map,camera,a),"West ray clear");
	a.x=5;a.y=7;check(xeenOutdoorRangedRay(map,camera,a),"North ray clear");
	a.y=3;check(xeenOutdoorRangedRay(map,camera,a),"South ray clear");
	a.x=4;check(!xeenOutdoorRangedRay(map,camera,a),"Nonaligned actor");
	a.x=3;a.y=5;
	unsigned calls=0;
	const auto moved=XeenActorApproach::move({a},camera,[](const XeenActor &,int,int){return XeenMonsterTerrain::Allowed;},true,
		[&](const std::vector<XeenActor> &candidate,std::size_t i){++calls;check(candidate[i].x==3,"Before movement observes candidate");});
	check(calls==1 && moved[0].x==4 && moved[0].hp==17 && a.x==3,"Moved once with wounds and detached input preserved");
	rejects([&]{XeenActorApproach::move({a},camera,[](const XeenActor &,int,int){return XeenMonsterTerrain::Allowed;},true,
		[](const std::vector<XeenActor> &,std::size_t){throw std::invalid_argument("Ranged boundary");});});
	check(a.x==3 && a.hp==17,"Refused movement leaves input unchanged");
	auto later=a;later.id.recordIndex=1;later.x=4;
	unsigned checked=0;
	rejects([&]{XeenActorApproach::move({a,later},camera,[](const XeenActor &,int,int){return XeenMonsterTerrain::Allowed;},true,
		[&](const std::vector<XeenActor> &candidate,std::size_t i){
			++checked;if(i==1){check(candidate[0].x==4,"Later actor sees earlier candidate movement");throw std::invalid_argument("Later ranged boundary");}
		});});
	check(checked==2 && a.x==3 && later.x==4,"Later failure discards whole candidate");
}
void events() {
	check(xeenWellHpAfter(2)==27 && xeenWellHpAfter(32742)==32767 &&
		!xeenWellHpAfter(32743) && !xeenWellHpAfter(32767),
		"Well HP addition is checked and unclamped");
	XeenEventFile file;file.mapId=23;file.resourcePresent=true;file.records.reserve(58);file.records.resize(57);
	for(auto &r:file.records){r.x=255;r.y=255;}
	auto &sign=file.records[56];sign={495,6,5,9,0,0,4,{16}};
	for(unsigned direction=0;direction<4;++direction) {
		XeenCamera c{23,5,9,static_cast<XeenDirection>(direction)};
		check(xeenRegionalSign(file,c)==(direction==0),"Sign exact physical facing");
		check(bool(xeenRegionalEvent(file,c))==(direction==0),"Wrong facing is actual no-event");
	}
	file.records.push_back({502,5,5,9,0,1,0,{}});
	check(!xeenRegionalSign(file,{23,5,9,XeenDirection::North}),"Extra sign continuation denied");
	file.records.pop_back();sign.opcode=0x20;
	check(!xeenRegionalSign(file,{23,5,9,XeenDirection::North}),"Grant is not sign admission");
	sign.opcode=4;sign.parameters={17};
	check(!xeenRegionalSign(file,{23,5,9,XeenDirection::North}),"Sign text index exact");
}
void structuralResources() {
	std::vector<std::uint8_t> dat(892);dat[768]=23;dat[781]=0x80;
	for(unsigned i=0;i<256;++i)dat[2*i]=1;
	XeenMap map;map.geometry=XeenMapFormat::parseDat(dat);
	std::vector<std::uint8_t> mob(48,255);mob[16]=8;
	const auto append=[&](std::initializer_list<std::uint8_t> bytes){mob.insert(mob.end(),bytes);};
	append({255,0,255,0});append({255,0,255,0}); // Empty objects.
	append({2,2,0,0});append({255,0,255,0});append({255,0,255,0});
	XeenObjectFile objects{23,"maze0023.mob",true,XeenMapFormat::parseMob(mob)};
	std::vector<XeenMonsterRecord> statistics(9);statistics[8].raw[20]=5;
	std::vector<std::uint8_t> evt{6,5,9,0,0,4,16};
	XeenEventFile events{23,"maze0023.evt",true,XeenEventFormat::parse(evt)};
	xeenValidateRegionalManifest(map,objects,events,statistics,dat,mob,evt);
	auto bad=events;bad.records[0].parameters[0]=17;
	rejects([&]{xeenValidateRegionalManifest(map,objects,bad,statistics,dat,mob,evt);});
	bad=events;bad.records[0].fileOffset=100;
	rejects([&]{xeenValidateRegionalManifest(map,objects,bad,statistics,dat,mob,evt);});
	auto changed=objects;changed.entities.monsters[0].resourceId=7;
	rejects([&]{xeenValidateRegionalManifest(map,changed,events,statistics,dat,mob,evt);});
	auto actors=XeenActorApproach::actorsFromResources(objects,statistics);
	xeenValidateRegionalActors(map,objects,actors,{},statistics);
	actors[0].statistics->raw[0]='X';
	rejects([&]{xeenValidateRegionalActors(map,objects,actors,{},statistics);});
	actors=XeenActorApproach::actorsFromResources(objects,statistics);actors.push_back(actors[0]);
	rejects([&]{xeenValidateRegionalActors(map,objects,actors,{},statistics);});
	// Shift the sign without changing its logical address, then corrupt its semantics.
	events.records.insert(events.records.begin(),{0,5,99,99,4,0,0x12,{}});events.records[1].fileOffset=6;
	check(xeenRegionalSign(events,{23,5,9,XeenDirection::North}),"sign resolves shifted resource identity");
	events.records[1].parameters={17};check(!xeenRegionalSign(events,{23,5,9,XeenDirection::North}),"shifted sign wrong operands refused");
	bad=events;bad.records[1].x=6;check(!xeenRegionalSign(bad,{23,5,9,XeenDirection::North}),"shifted sign wrong logical address refused");
}

// Artificial geometry on two logical indoor maps, each with four physical tiles.
// No production destination or original resource is changed by these controls.
void indoorRays() {
 constexpr int dx[]{0,1,0,-1},dy[]{1,0,-1,0};
 for(unsigned logical:{28u,33u})for(unsigned facing=0;facing<4;++facing)
 for(const auto origin:{std::pair<int,int>{8,8},{15,15},{16,16},{0,0},{31,31}}) {
  const XeenMapIdentity home{static_cast<std::uint16_t>(logical)};
  const XeenCamera camera{home,origin.first,origin.second,static_cast<XeenDirection>(facing)};
  const auto tiles=[&] {
   std::map<XeenMapIdentity,XeenMap> result;
   for(unsigned tile=0;tile<4;++tile) {
    const unsigned id=tile?200+tile:logical;XeenMap m;m.geometry.id=id;
    // mazeData() retains the primary map's no-pass threshold at seams.
    m.geometry.difficulties[0]=tile?1:7;
    m.geometry.neighbors=std::array<std::uint16_t,4>{
     static_cast<std::uint16_t>(tile<2?202+tile:0u),static_cast<std::uint16_t>(tile%2==0?201+tile:0u),
     static_cast<std::uint16_t>(tile>=2?(tile==2?logical:201u):0u),static_cast<std::uint16_t>(tile%2?(tile==1?logical:202u):0u)};
    for(auto &cell:m.geometry.cells)cell.geometry=XeenIndoorWalls{};
    result.emplace(XeenMapIdentity{static_cast<std::uint16_t>(id)},m);
   }
   return result;
  };
  const auto cellAt=[&](auto &maps,int x,int y)->XeenMapCell & {
   const unsigned tile=unsigned(y/16)*2+unsigned(x/16);
   return maps.at(XeenMapIdentity{static_cast<std::uint16_t>(tile?200+tile:logical)}).geometry.cells[(y%16)*16+x%16];
  };
  auto open=tiles();XeenWorld world([&](auto id){return open.at(id);});
  unsigned expected=4;
  for(unsigned row=0;row<3;++row) {
   const int x=camera.x+dx[facing]*int(row),y=camera.y+dy[facing]*int(row);
   if(x<0 || x>=32 || y<0 || y>=32){expected=row+1;break;}
  }
  check(xeenPlayerRayRows(world,camera)==expected,"Indoor empty ray/seam/edge differs");
  for(unsigned row=0;row<3;++row)for(unsigned wall:{6u,7u,8u,9u,13u}) {
   const int x=camera.x+dx[facing]*int(row),y=camera.y+dy[facing]*int(row);
   if(x<0 || x>=32 || y<0 || y>=32)continue;
   auto maps=tiles();auto &cell=cellAt(maps,x,y);
   xeenGet<XeenIndoorWalls>(cell.geometry).walls[facing]=wall;
   cell.rawWord=wall<<(12-4*facing);
   XeenWorld blocked([&](auto id){return maps.at(id);});
   check(xeenPlayerRayRows(blocked,camera)==(wall>=7?std::min(expected,row+1):expected),
    "Indoor player ray must compare each intervening query against no-pass");
  }
  std::vector<XeenActor> actors;
  for(unsigned row=0;row<4;++row)for(unsigned slot=0;slot<3;++slot) {
   const int x=camera.x+dx[facing]*int(row),y=camera.y+dy[facing]*int(row);
   if(x<0 || x>=32 || y<0 || y>=32)continue;
   XeenActor a;a.id={home,actors.size()};a.x=x;a.y=y;a.hp=5;
   a.lifecycle=XeenActorLifecycle::Present;a.statistics=XeenMonsterRecord{};
   actors.push_back(a);
  }
  const auto view=XeenIndoorScene().classifyActors(world,camera,actors);
  for(unsigned i=0;i<actors.size();++i)check(i==5?!view.slots[i]:view.slots[i]==actors[i].id,
   "Indoor row/record/slot order (including reference slot-5 predicate) differs");
  for(unsigned direction=0;direction<4;++direction)for(unsigned distance=1;distance<=3;++distance) {
   XeenActor a;a.id={home,0};a.x=camera.x+dx[direction]*int(distance);a.y=camera.y+dy[direction]*int(distance);
   if(a.x<0 || a.x>=32 || a.y<0 || a.y>=32)continue;
   check(xeenIndoorRangedRay(world,camera,a),"Clear enemy ray depends on party facing");
   constexpr unsigned masks[]{0x80,0x8,0x8000,0x800};
   for(unsigned step=1;step<=distance;++step)for(unsigned bit=0;bit<16;++bit) {
    auto maps=tiles();auto &cell=cellAt(maps,camera.x+dx[direction]*int(step),camera.y+dy[direction]*int(step));
    cell.rawWord=1u<<bit;XeenWorld blocked([&](auto id){return maps.at(id);});
    check(xeenIndoorRangedRay(blocked,camera,a)==!(masks[direction]&(1u<<bit)),
     "Enemy ray must use directional raw wall bits at every cell");
   }
  }
 }
}
}
int main() {
	try { geometry();calendar();actors();events();structuralResources();indoorRays();std::cout << "Regional geometry, time, actor and event rules passed\n";return 0; }
	catch (const std::exception &e) { std::cerr << e.what() << '\n';return 1; }
}
