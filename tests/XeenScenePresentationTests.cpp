#include "XeenRegionalTestSupport.h"
#include "games/xeen/XeenIndoorScene.h"
#include "games/xeen/XeenOutdoorScene.h"
#include "games/xeen/XeenIndoorSceneTables.h"
#include "formats/xeen/XeenAssetSource.h"
#include "games/xeen/CloudsMapComposer.h"
#include "SyntheticXeenArchive.h"
#include <chrono>
#include <iostream>
#include <set>
using namespace regional_test;
namespace {
template<class F> void rejects(F f) {bool rejected=false;try {f();}catch(const std::exception &) {rejected=true;}check(rejected,"Malformed presentation metadata admitted");}
XeenMap indoor(XeenMapIdentity id) {XeenMap m;m.geometry.id=id.number;for(auto &c:m.geometry.cells)c.geometry=XeenIndoorWalls{};return m;}
void archiveResolution() {
	using namespace sprite_test;
	const auto root=std::filesystem::temp_directory_path()/("mmodern-scene-archives-"+std::to_string(std::chrono::steady_clock::now().time_since_epoch().count()));
	std::filesystem::create_directories(root);
	struct Cleanup {std::filesystem::path path;~Cleanup() {std::error_code error;std::filesystem::remove_all(path,error);}} cleanup{root};
	const auto image=[](unsigned color) {return sprite(cell(0,1,0,1,{3,0,0,std::uint8_t(color)}));};
	GameInstallation installation;installation.root=root;installation.xeenArchive=root/"XEEN.CC";installation.darkArchive=root/"DARK.CC";installation.edition=GameEdition::WorldOfXeen;
	archive(installation.xeenArchive,{{"shared.srf",image(3)},{"broken.srf",{0}},{"cloudsonly.srf",image(12)}});
	archive(root/"INTRO.CC",{{"shared.srf",image(4)},{"fallback.srf",image(5)},{"intro.srf",image(6)}});
 installation.introData=ReadOnlyDataFile::plain(root/"INTRO.CC");
	std::map<std::string,Bytes> dark{{"shared.srf",image(7)},{"fallback.srf",image(8)},{"companion.srf",image(9)},{"broken.srf",image(10)}};
	archive(installation.darkArchive,dark);
	{
		XeenAssetSource assets(installation,320,200);
		CloudsMapComposer composer;
		const auto draw=[&](const char *name,unsigned color,XeenSceneArchive selection=XeenSceneArchive::Current) {
			check(assets.hasSceneResource(name,selection),"Installed scene resource was reported absent");
			check(assets.spriteFrameCount(name,selection)==1,"Scene frame validation did not use archive resolution");
			XeenIndoorDrawCommand command;command.x=100;command.y=100;command.content=XeenIndoorGeometryDraw{name,0};
			command.geometry().options.archive=selection;
			composer.drawIndoorCommands(assets,{command});
			check(assets.snapshot().pixels[32100]==color,"Scene archive precedence differs from selected/current then INTRO order");
			XeenOutdoorDrawCommand outdoor;outdoor.x=101;outdoor.y=100;outdoor.content=XeenOutdoorTerrainDraw{name,0};
			outdoor.terrain().options.archive=selection;
			composer.drawOutdoorCommands(assets,{outdoor});
			check(assets.snapshot().pixels[32101]==color,"Outdoor scene bypassed archive resolution");
			command.x=102;command.content=XeenWallItemDraw{0,0,name,0,false,selection};
			composer.drawIndoorCommands(assets,{command});
			check(assets.snapshot().pixels[32102]==color,"Wall item bypassed archive resolution");
		};
		draw("shared.srf",3);draw("fallback.srf",5);draw("intro.srf",6);
		check(!assets.hasSceneResource("companion.srf"),"DARK was searched without explicit archive selection");
		rejects([&] {assets.spriteFrameCount("companion.srf");});
		draw("shared.srf",7,XeenSceneArchive::Darkside);draw("shared.srf",3,XeenSceneArchive::Clouds);
		check(!assets.hasSceneResource("cloudsonly.srf",XeenSceneArchive::Darkside),"Explicit DARK selection fell back to opposite-side archive");
		draw("fallback.srf",8,XeenSceneArchive::Darkside);draw("intro.srf",6,XeenSceneArchive::Darkside);
		draw("companion.srf",9,XeenSceneArchive::Darkside);
		assets.discardSpriteCache();draw("companion.srf",9,XeenSceneArchive::Darkside);
		check(!assets.hasArchiveResource("companion.srf"),"Presentation lookup broadened Clouds gameplay resources");
		rejects([&] {assets.readArchiveResource("companion.srf");});
		rejects([&] {assets.drawSprite("companion.srf",0,100,100);});
		check(!assets.hasSceneResource("absent.srf"),"Absent scene resource accepted");
		rejects([&] {assets.drawSceneSprite("absent.srf",0,100,100);});
		// Presence in a higher-priority archive must not be bypassed when invalid.
		rejects([&] {assets.spriteFrameCount("broken.srf");});
		assets.discardSpriteCache();dark["companion.srf"]=image(11);archive(installation.darkArchive,dark);
		rejects([&] {assets.spriteFrameCount("companion.srf",XeenSceneArchive::Darkside);});
		dark["companion.srf"]=image(9);archive(installation.darkArchive,dark);assets.discardSpriteCache();
		rejects([&] {assets.spriteFrameCount("companion.srf",XeenSceneArchive::Darkside);});
	}
	// The same scene API also works with no optional companion archives installed.
	std::filesystem::remove(root/"INTRO.CC");installation.introData.reset();installation.darkArchive.clear();
	XeenAssetSource clouds(installation,320,200);
	check(clouds.hasSceneResource("shared.srf")&&!clouds.hasSceneResource("companion.srf"),"Clouds-only scene lookup failed");
}
}
int main() {try {
	archiveResolution();
	XeenMonsterRecord data;data.raw[20]=10;data.raw[47]=42;
	data.validatePresentation();data.raw[48]=1;data.raw[49]=15;data.raw[46]=1;
	data.validatePresentation();check(!data.supportsAdmittedMechanicsRendering(),"Presentation broadened mechanics admission");
	data.raw[49]=16;rejects([&] {data.validatePresentation();});data.raw[49]=0;
	data.raw[47]=255;rejects([&] {data.validatePresentation();});data.raw[47]=42;
	XeenMonsterAnimation cycle;data.raw[48]=0;
	for(unsigned i=1;i<=24;++i) {cycle.advance(data);check(cycle.frame==i%8,"Normal cycle");}
	data.raw[48]=1;cycle={};
	constexpr unsigned loop[]{1,2,3,4,5,6,7,6,5,4,3,2,1,0,1};
	for(auto frame:loop) {cycle.advance(data);check(cycle.frame==frame,"Loop reversal differs from reference");}
	cycle.attack(2);for(auto frame:{9u,10u,10u,0u}) {cycle.advance(data);check(cycle.frame==frame,"Attack delay/recovery");}
	cycle.hit(2);cycle.advance(data);check(cycle.frame==11,"Hit delay");cycle.advance(data);check(cycle.frame==0,"Hit recovery");
	for(unsigned effect=1;effect<=15;++effect) {
		data.raw[49]=effect;
		for(unsigned phase=0;phase<8;++phase) {
			cycle={};cycle.effect1=cycle.effect2=effect;cycle.effect3=phase;
			check(cycle.flags()<=0xfff,"Effect flags bounds");
			cycle.advance(data);check(cycle.effect3<3,"Initial random effect phase did not enter original oscillation");
			for(unsigned n=0;n<32;++n) cycle.advance(data);
		}
	}
	cycle={};cycle.effect2=1;cycle.effect3=7;data.raw[49]=3;cycle.advance(data);
	check(cycle.effect1==3&&cycle.effect2==3&&cycle.effect3==0,"Temporary effect recovery");
	XeenScenePresentation action(77);
	action.navigation(NavigationAction::MoveForward,false);check(!action.ground&&!action.sky,"Blocked movement flips");
	action.navigation(NavigationAction::MoveForward,true);check(action.ground&&action.defaultGround&&!action.sky,"Accepted movement flips");
	action.navigation(NavigationAction::TurnLeft,false);check(!action.ground&&!action.defaultGround&&action.sky,"Exploration turn flips");
	action.navigation(NavigationAction::TurnRight,false,true);check(!action.ground&&!action.sky,"Combat turn ground flip");
	action.wait();check(!action.ground,"Wait double flip cancellation");
	for(unsigned i=0;i<3;++i) action.advance(99);check(!action.water,"Premature water flip");
	action.advance(99);check(action.water&&action.overallFrame==4&&action.floatPhase==4,"Periodic water/overall/float phases");
	XeenObjectFile mob{99,"synthetic.mob",true,{}};mob.entities.monsters.push_back({8,8,0,0,0});
	data.raw[49]=1;data.raw[48]=1;std::vector<XeenMonsterRecord> stats{data};
	XeenWorld w([&](auto id) {return indoor(id);},[&](auto id) {auto m=mob;m.mapId=id;return m;});
	w.scenePresentation()=XeenScenePresentation(77);
	const auto actors=w.sceneActors(99,[&] {return stats;});
	check(!w.hasEncounterState()&&w.sessionState().actors().empty(),"Render-only observations acquired encounter state");
	const XeenCamera camera{99,8,8,XeenDirection::North};
	auto commands=XeenIndoorScene().build(w,camera);
	const auto selected=std::find_if(commands.begin(),commands.end(),[](const auto &c) {return c.actor()!=nullptr;});
	check(selected!=commands.end()&&selected->actor()->image==42&&selected->x==-7&&selected->y==0,"Generic image/floating production command");
	check(selected->drawOptions().monsterEffectFlags>=0x104,"Effect metadata absent from command");
	const auto phase=w.scenePresentation().animation({99,0})->effect3;
	w.discardMapCache();w.sceneActors(99,[&] {return stats;});
	check(w.scenePresentation().animation({99,0})->effect3==phase,"Cache reload rerandomized phases");
	XeenMonsterAppearance hit{XeenMonsterSpriteKind::Attack,3};hit.identity=XeenMonsterIdentity{99,0};
	w.scenePresentation().appearance(hit);
	w.sceneActors(100,[&] {return stats;});w.scenePresentation().advance(100);
	check(w.scenePresentation().wallFrame(7)==1,"Wall phase did not restart on map entry");
	w.discardMapCache();w.sceneActors(100,[&] {return stats;});
	check(w.scenePresentation().wallFrame(7)==1,"Cache reload reset wall phase");
	w.sceneActors(99,[&] {return stats;});
	check(w.scenePresentation().animation({99,0})->frame<8&&
		w.scenePresentation().animation({99,0})->postAttackDelay==0&&w.scenePresentation().wallFrame(7)==0,"Map reentry retained hit/wall phases");
	std::set<unsigned> phases,initialFrames;
	XeenScenePresentation seeded(123),repeated(123);
	std::vector<XeenActor> many;
	for(unsigned i=0;i<64;++i) {auto a=actors[0];a.id.recordIndex=i;many.push_back(a);}
	seeded.include(many);repeated.include(many);
	for(const auto &a:many) {const auto p=seeded.animation(a.id)->effect3;check(p<8&&p==repeated.animation(a.id)->effect3,"Seeded presentation RNG");phases.insert(p);
		const auto frame=seeded.animation(a.id)->frame;check(frame<8&&frame==repeated.animation(a.id)->frame,"Seeded initial normal frame");initialFrames.insert(frame);}
	check(phases.size()==8,"Initial effect phases lack variation");
	check(initialFrames.size()==8,"Initial normal frames lack variation");
	// Invalid resolved metadata/statistics fail explicitly; inactive and table
	// sentinels remain observations rather than silently becoming live actors.
	mob.entities.monsters[0].resourceId=7;
	rejects([&] {w.discardMapCache();w.sceneActors(99,[&] {return stats;});});
	mob.entities.monsters[0].resourceId=0;stats[0].raw[49]=16;
	XeenWorld invalid([&](auto id) {return indoor(id);},[&](auto) {return mob;});
	rejects([&] {invalid.sceneActors(99,[&] {return stats;});});
	stats[0].raw[49]=1;mob.entities.monsters[0].resourceId=-1;
	XeenWorld sentinel([&](auto id) {return indoor(id);},[&](auto) {return mob;});
	check(!sentinel.sceneActors(99,[&] {return stats;})[0].statistics,"Unresolved table sentinel acquired statistics");
	// Production terrain commands: action flips use alternate frames and water
	// is independent. Night selects the original sky in both projections.
	for(bool outdoors:{false,true}) {
		const XeenCamera terrainCamera{33,8,8,XeenDirection::North};
		XeenMap map=indoor(33);map.geometry.surfaceTypes[4]=outdoors?8:0;
		if(outdoors) map.geometry.flags2=0x8000;
		for(auto &cell:map.geometry.cells) {
			cell.surfaceIndex=4;
			if(outdoors) cell.geometry=XeenOutdoorLayers{4,0,0};
		}
		XeenWorld terrain([&](auto) {return map;});
		terrain.scenePresentation().sky=true;terrain.scenePresentation().water=true;
		if(outdoors) {
			const auto draw=XeenOutdoorScene().build(terrain,terrainCamera,nullptr,nullptr,{}, {},true);
			check(draw[0].terrain().resourceName=="night.sky"&&draw[0].drawOptions().horizontalFlip,"Outdoor night/sky flip");
			check(draw[2].drawOptions().horizontalFlip,"Outdoor default water flip");
			check(draw[3].terrain().frame==24&&draw[3].drawOptions().horizontalFlip,"Outdoor water alternate frame");
		} else {
			const auto draw=XeenIndoorScene().build(terrain,terrainCamera,nullptr,nullptr,{}, {},true);
			check(draw[0].geometry().resourceName=="night.sky"&&draw[0].drawOptions().horizontalFlip,"Indoor night/sky flip");
			const auto water=std::find_if(draw.begin(),draw.end(),[](const auto &v) {return !v.actor()&&!v.wallItem()&&v.geometry().resourceName=="water.srf";});
			check(water!=draw.end()&&water->geometry().frame==24&&water->drawOptions().horizontalFlip,"Indoor water alternate frame");
		}
		map.geometry.surfaceTypes[4]=1;terrain.discardMapCache();
		terrain.scenePresentation().ground=true;terrain.scenePresentation().defaultGround=true;
		if(outdoors) {
			const auto draw=XeenOutdoorScene().build(terrain,terrainCamera);
			check(draw[3].terrain().resourceName=="dirt.srf"&&draw[3].terrain().frame==24&&draw[3].drawOptions().horizontalFlip,"Outdoor action ground alternate frame");
		} else {
			const auto draw=XeenIndoorScene().build(terrain,terrainCamera);
			check(draw[2].drawOptions().horizontalFlip,"Indoor default ground flip");
			const auto dirt=std::find_if(draw.begin(),draw.end(),[](const auto &v) {return !v.actor()&&!v.wallItem()&&v.geometry().resourceName=="dirt.srf";});
			check(dirt!=draw.end()&&dirt->geometry().frame==24&&dirt->drawOptions().horizontalFlip,"Indoor action ground alternate frame");
		}
	}
	// Every reference wall-picture projection, independent of commercial IDs.
	for(unsigned direction=0;direction<4;++direction) {
		XeenObjectFile walls{99,"walls.mob",true,{}};
		for(unsigned q:{2u,7u,5u,9u,14u,12u,16u,27u,25u,23u,29u,31u})
			walls.entities.wallItems.push_back({8+xeen_indoor_scene_tables::kScreenPositioningX[direction][q],8+xeen_indoor_scene_tables::kScreenPositioningY[direction][q],0,std::uint8_t(direction),17});
		XeenWorld wallWorld([&](auto id) {return indoor(id);},[&](auto) {return walls;});
		const XeenCamera c{99,8,8,static_cast<XeenDirection>(direction)};
		auto draw=XeenIndoorScene().build(wallWorld,c,nullptr,nullptr,{}, {},false,[](auto) {return 3;});
		check(std::count_if(draw.begin(),draw.end(),[](const auto &v) {return v.wallItem()!=nullptr;})==18,"Missing reference wall-picture projection");
		for(unsigned tick=0;tick<7;++tick) {
			draw=XeenIndoorScene().build(wallWorld,c,nullptr,nullptr,{}, {},false,[](auto) {return 3;});
			for(const auto &v:draw) if(auto wall=v.wallItem()) check(wall->frame==tick%3&&v.drawOptions().sceneClipped,"Wall frame count/clipping");
			wallWorld.scenePresentation().advance(99);
		}
		const auto other=XeenIndoorScene().build(wallWorld,{99,8,8,static_cast<XeenDirection>((direction+1)%4)},nullptr,nullptr,{}, {},false,[](auto) {return 3;});
		check(std::none_of(other.begin(),other.end(),[](const auto &v) {return v.wallItem()!=nullptr;}),"Wall direction ignored");
		// A wall immediately in front sets wo[27]. Preserve the original extreme
		// lateral exceptions, while hiding the other depths behind this wall.
		auto blocked=indoor(99);
		const auto shift=xeen_indoor_scene_tables::kWallShifts[direction][2];
		const unsigned face=shift==12?0:shift==8?1:shift==4?2:3;
		xeenGet<XeenIndoorWalls>(blocked.geometry.cells[8*16+8].geometry).walls[face]=5;
		XeenWorld obstruction([&](auto) {return blocked;},[&](auto) {return walls;});
		const auto hidden=XeenIndoorScene().build(obstruction,c,nullptr,nullptr,{}, {},false,[](auto) {return 3;});
		std::set<int> orders;for(const auto &v:hidden) if(v.wallItem()) orders.insert(v.originalOrder);
		check(orders==std::set<int>{46,54,96,122,124,148},"Wall item occlusion differs from pinned near-wall cases");
		// Original near entries keep the last duplicate; distant entries keep
		// the first. Empty sprite resources are explicit failures.
		walls.entities.wallItems.push_back(walls.entities.wallItems[0]);
		wallWorld.discardMapCache();
		const auto duplicates=XeenIndoorScene().build(wallWorld,c,nullptr,nullptr,{}, {},false,[](auto) {return 3;});
		const auto near=std::find_if(duplicates.begin(),duplicates.end(),[](const auto &v) {return v.wallItem()&&v.originalOrder==148;});
		check(near!=duplicates.end()&&near->wallItem()->recordIndex==walls.entities.wallItems.size()-1,"Near duplicate did not use last record");
		rejects([&] {XeenIndoorScene().build(wallWorld,c,nullptr,nullptr,{}, {},false,[](auto) {return 0;});});
		walls.entities.wallItems[0].direction=4;wallWorld.discardMapCache();
		rejects([&] {XeenIndoorScene().build(wallWorld,c,nullptr,nullptr,{}, {},false,[](auto) {return 3;});});
	}
	XeenWorld neighbors([](auto id) {auto m=indoor(id);if(id==99)m.geometry.neighbors[0]=100;return m;});
	const auto gameplayNorth=neighbors.sampleCell(99,8,16);
	const auto north=neighbors.sceneCell(99,8,16);
	check(north&&north->mapId==100&&north->x==8&&north->y==0,"Generic indoor scene neighbor sampling");
	check(gameplayNorth&&gameplayNorth->cell==north->cell&&gameplayNorth->mapId==north->mapId,
		"Gameplay and presentation must resolve the same indoor tile");
	XeenWorld tables([](auto id) {auto m=indoor(id);m.geometry.surfaceTypes[4]=id==33?1:12;
		if(id==33)m.geometry.neighbors[0]=34;for(auto &cell:m.geometry.cells)cell.surfaceIndex=4;return m;});
	const auto rootTables=XeenIndoorScene().build(tables,{33,8,15,XeenDirection::North});
	check(std::none_of(rootTables.begin(),rootTables.end(),[](const auto &v) {return !v.actor()&&!v.wallItem()&&v.geometry().resourceName=="sewer.srf";}),"Neighbor replaced root surface sprite table");
	// The current save bytes, RNG, time and live actor state survive cosmetic work.
	Fixture fixture;
	const auto before=XeenSaveFormat::encode(fixture.snapshot());
	fixture.w.sceneActors(fixture.camera.mapId);
	for(unsigned i=0;i<128;++i) {fixture.w.scenePresentation().advance(fixture.camera.mapId);fixture.w.scenePresentation().navigation(NavigationAction::TurnLeft,false);}
	fixture.w.discardMapCache();
	const auto after=XeenSaveFormat::encode(fixture.snapshot());
	check(before==after,"Cosmetic ticks/cache reload changed save bytes or gameplay RNG");
	std::cout<<"Scene animation, effects, action/water flips, all wall placements, RNG and save isolation passed\n";
	return 0;
} catch(const std::exception &e) {std::cerr<<e.what()<<'\n';return 1;}}
