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
		const bool skillWall=middle==1 || middle==7 || middle==9 || middle==10 || middle==12 || middle==3 || middle==6;
		const bool checks=middle==0 || middle==2 || middle==4 || middle==5 || middle==8 || middle==11 || middle==13 || middle==14;
		const auto expected=skillWall ? XeenMovementResult::BlockedByTerrain : checks && (surface==0 || surface==8 || surface==15) ? XeenMovementResult::BlockedBySurface : XeenMovementResult::Moved;
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
	check(!a.statistics->supportsMovement() && xeenMonsterMovementSupported(*a.statistics,6),"Ranged profile independent movement admission");
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
// M68 Part B: the original monster terrain test (WORLD/XEEN.DAT 0x1fc91).
void monsterTerrainMatrix() {
	constexpr unsigned water=0,deepWater=8,space=15;
	for (const auto side:{XeenSide::Clouds,XeenSide::Darkside})
	for (unsigned middle=0;middle<16;++middle) for (unsigned surface=0;surface<16;++surface)
	for (const bool flying:{false,true}) for (const int species:{6,59}) for (const unsigned wallNoPass:{0u,6u,7u,15u,16u}) {
		bool expected=false;
		switch (middle) {
		case 0:case 2:case 3:case 4:case 5:case 6:case 8:case 11:case 13:case 14:
			expected=surface==water || surface==deepWater ? flying || species==59 :
				surface==space ? flying : side==XeenSide::Darkside || species!=59;
			break;
		case 15: expected=15<wallNoPass;break;
		default: expected=false; // 1, 7, 9, 10, 12: refused before any threshold, flyers too
		}
		check(xeenOutdoorMonsterTerrain(middle,surface,wallNoPass,flying,species,side)==
			(expected ? XeenMonsterTerrain::Allowed : XeenMonsterTerrain::Blocked),"Outdoor monster terrain matrix");
	}
	check(xeenOutdoorMonsterTerrain(0,16,0,true,6,XeenSide::Clouds)==XeenMonsterTerrain::Unsupported,"Unresolved surface type");
	check(xeenOutdoorMonsterTerrain(16,2,0,true,6,XeenSide::Clouds)==XeenMonsterTerrain::Unsupported,"Middle outside a nibble");
	// The named cases of the plan and of the DOS correction.
	check(xeenOutdoorMonsterTerrain(0,water,0,true,1,XeenSide::Clouds)==XeenMonsterTerrain::Allowed,"Giant Bat over water");
	check(xeenOutdoorMonsterTerrain(0,water,0,false,6,XeenSide::Clouds)==XeenMonsterTerrain::Blocked,"Walker refused by water");
	check(xeenOutdoorMonsterTerrain(0,deepWater,0,false,59,XeenSide::Clouds)==XeenMonsterTerrain::Allowed,"Water Dragon in deep water");
	check(xeenOutdoorMonsterTerrain(0,2,0,false,59,XeenSide::Clouds)==XeenMonsterTerrain::Blocked,"Clouds species 59 stays off land");
	check(xeenOutdoorMonsterTerrain(0,2,0,true,59,XeenSide::Darkside)==XeenMonsterTerrain::Allowed,"Darkside species 59 (Vulture Roc) on land");
	check(xeenOutdoorMonsterTerrain(0,space,0,false,59,XeenSide::Darkside)==XeenMonsterTerrain::Blocked,"Space admits only flyers");
	check(xeenOutdoorMonsterTerrain(1,2,15,true,6,XeenSide::Clouds)==XeenMonsterTerrain::Blocked,"Mountain refuses flyers above the threshold (DOS)");
	check(xeenOutdoorMonsterTerrain(1,2,6,false,6,XeenSide::Clouds)==XeenMonsterTerrain::Blocked,"Clouds map 2: middle 1 under no-pass 6 still blocks (DOS)");
	check(xeenOutdoorMonsterTerrain(15,2,15,true,6,XeenSide::Clouds)==XeenMonsterTerrain::Blocked &&
		xeenOutdoorMonsterTerrain(15,2,16,true,6,XeenSide::Clouds)==XeenMonsterTerrain::Allowed,"Middle 15 strict threshold");
	for (unsigned wall=0;wall<16;++wall) check(xeenMonsterPassesWall(wall,7)==(wall<7),"Strict wall threshold");

	// The regional query resolves the cell's surface index and keeps Darkside closed.
	XeenMap map;map.geometry.id=23;map.geometry.flags2=0x8000;
	for (unsigned i=0;i<16;++i) map.geometry.surfaceTypes[i]=i;
	for (auto &c:map.geometry.cells) c.geometry=XeenOutdoorLayers{2,0,0,0};
	map.geometry.cells[5*16+7].geometry=XeenOutdoorLayers{water,0,0,0};
	XeenActor walker;walker.id={23,0};walker.original.x=6;walker.original.y=5;walker.original.resourceId=6;
	walker.x=6;walker.y=5;walker.statistics=XeenMonsterRecord{};walker.statistics->raw[20]=25;
	walker.hp=17;walker.activated=true;walker.lifecycle=XeenActorLifecycle::Present;
	auto flyer=walker;flyer.original.resourceId=1;flyer.statistics->raw[46]=1;
	check(xeenRegionalActorTerrain(map,walker,7,5)==XeenMonsterTerrain::Blocked && xeenRegionalActorTerrain(map,flyer,7,5)==XeenMonsterTerrain::Allowed,"Regional water edge");
	check(xeenActorClosure(map,flyer).count()==256 && xeenActorClosure(map,walker).count()==255,"Flyer closure includes water");
	auto zero=flyer;zero.statistics->raw[20]=0;
	check(!xeenMonsterMovementSupported(*zero.statistics,0) && xeenRegionalActorTerrain(map,zero,7,5)==XeenMonsterTerrain::Unsupported,"Zero-HP record has no movement");
	map.side=XeenSide::Darkside;
	check(xeenRegionalActorTerrain(map,flyer,7,5)==XeenMonsterTerrain::Unsupported,"Darkside gameplay stays closed");
}
// Indoors there is no flight rule: walkers and flyers share the strict threshold.
void indoorMonsterWalls() {
	XeenMap m;m.geometry.id=28;m.geometry.difficulties[0]=7;
	for (auto &cell:m.geometry.cells) cell.geometry=XeenIndoorWalls{};
	XeenActor walker;walker.id={28,0};walker.original.x=5;walker.original.y=5;walker.original.resourceId=6;
	walker.x=5;walker.y=5;walker.statistics=XeenMonsterRecord{};walker.statistics->raw[20]=25;
	walker.hp=17;walker.activated=true;walker.lifecycle=XeenActorLifecycle::Present;
	auto flyer=walker;flyer.original.resourceId=1;flyer.statistics->raw[46]=1;
	for (unsigned wall:{0u,6u,7u,8u,15u}) {
		auto tile=m;auto &cell=tile.geometry.cells[5*16+5];
		xeenGet<XeenIndoorWalls>(cell.geometry).walls[1]=std::uint8_t(wall);cell.rawWord=wall<<8;
		XeenWorld world([&](auto){return tile;});
		const auto expected=wall<7 ? XeenMonsterTerrain::Allowed : XeenMonsterTerrain::Blocked;
		check(xeenIndoorActorTerrain(world,walker,6,5)==expected && xeenIndoorActorTerrain(world,flyer,6,5)==expected,
			"Indoor wall threshold is strict and the same for flyers (DOS 0x1fd8f)");
	}
}
// Walkers next to flyers keep the shared approach: scan order, primary and
// fallback, the three-actor cap without a flight layer, contact.
void walkersNextToFlyers() {
	XeenMap map;map.geometry.id=23;map.geometry.flags2=0x8000;
	for (unsigned i=0;i<16;++i) map.geometry.surfaceTypes[i]=i;
	for (auto &c:map.geometry.cells) c.geometry=XeenOutdoorLayers{2,0,0,0};
	map.geometry.cells[5*16+7].geometry=XeenOutdoorLayers{0,0,0,0};
	const auto terrain=[&](const XeenActor &a,int x,int y) { return xeenRegionalActorTerrain(map,a,x,y); };
	const auto actor=[](unsigned record,int x,int y,bool flying,bool activated) {
		XeenActor a;a.id={23,record};a.original.x=x;a.original.y=y;a.original.resourceId=flying?1:6;a.x=x;a.y=y;
		a.statistics=XeenMonsterRecord{};a.statistics->raw[20]=25;a.statistics->raw[46]=flying;
		a.hp=17;a.activated=activated;a.lifecycle=XeenActorLifecycle::Present;return a;
	};
	// Party (9,7) facing North; from (6,5) the primary step is East onto water at
	// (7,5) and the fallback is North to (6,6).
	const XeenCamera north{23,9,7,XeenDirection::North};
	auto moved=XeenActorApproach::move({actor(0,6,5,false,true)},north,terrain,true);
	check(moved[0].x==6 && moved[0].y==6,"Walker refused by water takes the terrain fallback");
	moved=XeenActorApproach::move({actor(0,6,5,true,true)},north,terrain,true);
	check(moved[0].x==7 && moved[0].y==5,"Flyer takes the primary step onto water");
	// Three idle flyers hovering on the water fill the shared cap: the flyer's
	// allowed primary step fails on occupancy and never tries the fallback,
	// while the walker's terrain refusal still falls back.
	for (const bool flying:{false,true}) {
		std::vector<XeenActor> actors{actor(0,6,5,flying,true)};
		for (unsigned r=1;r<4;++r) actors.push_back(actor(r,7,5,true,false));
		moved=XeenActorApproach::move(actors,north,terrain,true);
		check(flying ? moved[0].x==6 && moved[0].y==5 : moved[0].x==6 && moved[0].y==6,"Terrain versus occupancy failure");
		check(XeenActorApproach::occupancy(moved)[5*32+7]==3,"Flyers share the ground cap");
	}
	// Pass order: the earlier walker is refused by the full cell its later flyer
	// neighbours vacate; the second pass retries it once. The flyers reach the
	// party's cell and engage, exactly like walkers.
	const XeenCamera camera{23,10,10,XeenDirection::North};
	std::vector<XeenActor> actors{actor(0,9,11,false,true)};
	for (unsigned r=1;r<4;++r) actors.push_back(actor(r,10,11,true,true));
	moved=XeenActorApproach::move(actors,camera,terrain,true);
	check(moved[0].x==10 && moved[0].y==11 && moved[1].y==10 && moved[2].y==10 && moved[3].y==10,"Two-pass order with flyers");
	check(XeenActorApproach::classify(moved,camera).engaged(),"Flyers enter contact");
	// The approach is a pure function of its inputs: no random draw is involved.
	check(XeenActorApproach::move(actors,camera,terrain,true).size()==moved.size(),"Repeatable approach");
	for (unsigned i=0;i<4;++i) check(XeenActorApproach::move(actors,camera,terrain,true)[i].x==moved[i].x,"Approach is deterministic");
}
void events() {
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
	// The mainland classifies by its bound records, as every other map does:
	// any bound entry is an Event, and only the Vertigo gate stays distinct.
	sign.parameters={16};
	file.records.push_back({510,8,0,1,4,0,0x0c,{0,0,8,25,0,0,0}}); // A fountain-like give.
	file.records.push_back({520,8,12,2,4,0,0x05,{0,0,0,1,0}});    // A statue-like NPC.
	file.records.push_back({530,6,14,4,1,0,0x07,{28,1,1}});        // A teleport to Vertigo.
	const auto kind=[&](int x,int y,XeenDirection d){return xeenRegionalInteraction(file,{23,x,y,d});};
	check(kind(5,9,XeenDirection::North)==XeenRegionalInteraction::Event,"Mainland sign is an ordinary Event");
	check(kind(5,9,XeenDirection::East)==XeenRegionalInteraction::None,"Unbound mainland facing has no Event");
	for(unsigned d=0;d<4;++d) {
		check(kind(0,1,XeenDirection(d))==XeenRegionalInteraction::Event,"Mainland fountain is an Event");
		check(kind(12,2,XeenDirection(d))==XeenRegionalInteraction::Event,"Mainland statue is an Event");
	}
	check(kind(14,4,XeenDirection::East)==XeenRegionalInteraction::Event,"Vertigo gate uses the generic Event transition");
	check(kind(14,4,XeenDirection::North)==XeenRegionalInteraction::None,"Gate binds only its facing");
	check(kind(3,3,XeenDirection::North)==XeenRegionalInteraction::None,"Empty mainland cell has no Event");
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
	try { geometry();calendar();actors();monsterTerrainMatrix();indoorMonsterWalls();walkersNextToFlyers();events();structuralResources();indoorRays();std::cout << "Regional geometry, time, actor and event rules passed\n";return 0; }
	catch (const std::exception &e) { std::cerr << e.what() << '\n';return 1; }
}
