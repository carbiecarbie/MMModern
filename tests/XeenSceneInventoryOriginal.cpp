#include "formats/xeen/XeenAssetSource.h"
#include "formats/xeen/XeenMapFormat.h"
#include "games/xeen/CloudsMapComposer.h"
#include "games/xeen/XeenInstallationDetector.h"
#include "games/xeen/XeenMapLoader.h"
#include "games/xeen/XeenCharacterRules.h"
#include "games/xeen/XeenWorld.h"
#include <algorithm>
#include <cstdio>
#include <iostream>
#include <map>
#include <set>
#include <tuple>
using namespace mmodern;
namespace {
void check(bool ok,const std::string &message) {if(!ok)throw std::runtime_error(message);}
std::string numbered(unsigned n,const char *extension) {char name[20];std::snprintf(name,sizeof(name),"%03u.%s",n,extension);return name;}
XeenMap emptyIndoor(XeenMapIdentity id) {XeenMap m;m.geometry.id=id.number;for(auto &c:m.geometry.cells)c.geometry=XeenIndoorWalls{};return m;}
}
int main(int argc,char **argv) {try {
	check(argc==2,"usage: mmodern_scene_inventory_original <original-installation>");
	const auto installation=XeenInstallationDetector().detect(argv[1]);check(bool(installation),"Original installation missing");
	XeenAssetSource assets(*installation,320,200);CloudsMapComposer composer;XeenMapLoader loader;
	assets.loadPalette("mm4.pal");
	const auto bytes=assets.readCloudsMonsterStatisticsFromDarkArchive();check(bool(bytes),"Missing DARK.CC/xeen.mon");
	const auto statistics=XeenMonsterFormat::parse(*bytes);
	std::set<unsigned> images,effects,wallResources;
	unsigned looped=0,flying=0,monsterFrames=0,sentinels=0;
	for(unsigned type=0;type<statistics.size();++type) {
		const auto &s=statistics[type];
		if(s.image()==255) {++sentinels;std::cout<<"Monster image sentinel: type "<<type<<'\n';continue;}
		s.validatePresentation();images.insert(s.image());effects.insert(s.animationEffect());looped+=s.loopAnimation();flying+=s.flying();
		XeenActor actor;actor.id={99,0};actor.original.resourceId=type;actor.x=actor.y=8;actor.hp=s.baseHp();actor.statistics=s;actor.lifecycle=XeenActorLifecycle::Present;
		std::vector<XeenActor> actors{actor};XeenWorld projection([](auto id) {return emptyIndoor(id);});
		const XeenCamera camera{99,8,8,XeenDirection::North};
		// Every image's normal/attack frames pass through both production command
		// streams and the native sprite drawer, independent of combat admission.
		for(auto kind:{XeenMonsterSpriteKind::Normal,XeenMonsterSpriteKind::Attack})
		for(unsigned frame=0;frame<(kind==XeenMonsterSpriteKind::Normal?8u:4u);++frame) {
			XeenMonsterAppearance appearance{kind,std::uint8_t(frame)};appearance.identity=actor.id;
			const auto in=XeenIndoorScene().buildActors(projection,camera,actors,{},appearance);
			const auto out=XeenOutdoorScene::actorCommands(actors,camera,appearance);
			check(in.size()==1&&out.size()==1&&in[0].actor()->frame==frame&&out[0].actor()->frame==frame,"Silently skipped monster type "+std::to_string(type));
			composer.drawIndoorCommands(assets,in);composer.drawOutdoorCommands(assets,out);monsterFrames+=2;
		}
		projection.scenePresentation()=XeenScenePresentation(100+type);projection.scenePresentation().include(actors);
		std::set<unsigned> normalFrames;
		for(unsigned tick=0;tick<32;++tick) {
			const auto in=XeenIndoorScene().buildActors(projection,camera,actors);
			const auto out=XeenOutdoorScene::actorCommands(actors,camera,{0},&projection.scenePresentation());
			check(in.size()==1&&out.size()==1,"Missing data-driven effect/float commands");
			check(in[0].drawOptions().monsterEffectFlags==out[0].drawOptions().monsterEffectFlags,"Projection effect mismatch");
			normalFrames.insert(in[0].actor()->frame);
			composer.drawIndoorCommands(assets,in);composer.drawOutdoorCommands(assets,out);
			projection.scenePresentation().advance(99);
		}
		check(normalFrames.size()==8,"Incomplete original animation cycle");
		assets.discardSpriteCache();assets.validateNormalMonster(s.image());assets.validateAttackMonster(s.image());
	}
	// Enumerate the complete four-digit Clouds namespace. CC hash collisions
	// are aliases, not maps: accept only complete DATs with matching internal ID.
	std::map<unsigned,XeenMap> maps;
	unsigned aliases=0;
	for(unsigned id=1;id<=9999;++id) {
		char name[20];std::snprintf(name,sizeof(name),id>=100?"mazex%03u.dat":"maze%04u.dat",id);
		if(!assets.hasInitialResource(name))continue;
		const auto data=assets.readInitialResource(name);
		if(data.size()!=892) {++aliases;continue;}
		const auto geometry=XeenMapFormat::parseDat(data);
		if(geometry.id!=id) {++aliases;continue;}
		maps.emplace(id,loader.loadGeometryMap(assets,id));
	}
	check(!maps.empty(),"No Clouds maps inventoried");
	XeenWorld world([&](auto id) {check(maps.count(id.number),"Missing map neighbor "+std::to_string(id.number));return maps.at(id.number);},
		[&](auto id) {return loader.loadObjects(assets,id);});
	world.scenePresentation()=XeenScenePresentation(48);
	unsigned mobs=0,missingMobs=0,actors=0,disabled=0,unresolved=0,wallItems=0,inactiveWalls=0,wallFrames=0,wallCommands=0,actorCommands=0;
	std::set<std::pair<unsigned,unsigned>> uniqueWallFrames;
	std::set<std::pair<std::string,std::size_t>> sceneryFrames;
	std::set<std::pair<unsigned,std::string>> missingReferences;
	// Maintainer-approved original-data gaps: these Clouds DATs request the
	// surface from the current archive, but SEWER.SRF exists only in DARK.CC.
	// map.cpp does not select DARK for surfaces. Rendering must still fail
	// explicitly; this inventory exception never supplies replacement art.
	const std::set<std::pair<unsigned,std::string>> originalDataGaps{
		{106,"sewer.srf"},{107,"sewer.srf"},{108,"sewer.srf"}};
	std::map<std::string,bool> available;
	const auto resourcePresent=[&](unsigned map,const std::string &name) {
		const auto [entry,inserted]=available.try_emplace(name,false);
		if(inserted) entry->second=assets.hasSceneResource(name);
		if(!entry->second && missingReferences.emplace(map,name).second) {
			std::cout<<"Missing presentation resource: map "<<map<<" -> "<<name<<" (production drawing reports missing resource)\n";
			if(originalDataGaps.count({map,name})) {
				bool missingError=false;
				try {assets.drawSceneSprite(name,0,32,24);}
				catch(const std::runtime_error &e) {missingError=std::string(e.what()).find("Missing scene resource")!=std::string::npos;}
				check(missingError,"Original-data gap silently substituted art instead of reporting missing resource");
			}
		}
		return entry->second;
	};
	unsigned fullCompositions=0;
	unsigned expectedActorRecords=0,checkedActorRecords=0;
	unsigned waterSpots=0,townSpots=0;bool loopSpot=false,effectSpot=false,flyingSpot=false;
	const auto resolver=XeenObjectVisualResolver::load(assets);
	for(const auto &[id,map]:maps) {
		const auto mob=world.objectFile(id);
		if(!mob.resourcePresent) {++missingMobs;std::cout<<"Missing MOB: map "<<id<<" (geometry still rendered)\n";} else ++mobs;
		const auto view=world.sceneActors(id,[&] {return statistics;});
		check(view.size()==mob.entities.monsters.size(),"MOB actor records truncated");actors+=view.size();
		for(const auto &a:view) {disabled+=a.original.isDisabled();unresolved+=!a.original.hasResource();if(!a.original.hasResource()) std::cout<<"Unresolved monster table sentinel: map "<<id<<" record "<<a.id.recordIndex<<'\n';}
		std::set<std::size_t> expectedRecords;
		for(std::size_t record=0;record<mob.entities.monsters.size();++record) {
			const auto &raw=mob.entities.monsters[record];
			if(raw.isDisabled() || !raw.hasResource())continue;
			check(unsigned(raw.resourceId)<statistics.size(),"MOB monster resource outside statistics");
			if(statistics[raw.resourceId].image()==255)continue;
			expectedRecords.insert(record);++expectedActorRecords;
		}
		// At the party's cell the original admits the first three MOB records
		// into the front rank. Derive that expectation from MOB/statistics, never
		// from the actor view/classifier under test. Check each cell/facing, not
		// just an aggregate command total that another map could compensate.
		const auto verifyActors=[&](const auto &commands,const XeenCamera &camera) {
			std::vector<std::size_t> front;
			for(auto record:expectedRecords) {
				const auto &raw=mob.entities.monsters[record];
				if(raw.x==camera.x && raw.y==camera.y && front.size()<3)front.push_back(record);
			}
			for(const auto &c:commands)if(const auto *actor=c.actor()) {
				check(actor->identity.mapId==camera.mapId && expectedRecords.count(actor->identity.recordIndex),"Unexpected actor command outside actual MOB records");
				check(actor->image==statistics[mob.entities.monsters[actor->identity.recordIndex].resourceId].image(),"MOB record resolved to wrong drawn monster image");
			}
			for(auto record:front) {
				const auto found=std::find_if(commands.begin(),commands.end(),[&](const auto &c) {return c.actor() && c.actor()->identity==XeenMonsterIdentity{camera.mapId,record};});
				check(found!=commands.end(),"Missing expected front-rank command: map "+std::to_string(id)+" MOB record "+std::to_string(record));
			}
		};
		for(auto resource:mob.entities.wallItemTable) if(mob.resourcePresent&&resource!=255) {
			wallResources.insert(resource);const auto name=numbered(resource,"pic");
			const auto count=assets.spriteFrameCount(name);
			for(unsigned frame=0;frame<count;++frame) {
				XeenIndoorDrawCommand c;c.x=32;c.y=24;c.content=XeenWallItemDraw{0,frame,name,0};
				composer.drawIndoorCommands(assets,{c});++wallFrames;
				uniqueWallFrames.emplace(resource,frame);
			}
		}
		wallItems+=mob.entities.wallItems.size();
		// Isolate each wall record to prove resolution/animation, even where
		// ordinary scene selection hides duplicates or a sealed corridor.
		for(std::size_t record=0;record<mob.entities.wallItems.size();++record) {
			const auto &item=mob.entities.wallItems[record];
			if(!item.isActive()) {++inactiveWalls;std::cout<<"Inactive wall sentinel: map "<<id<<" record "<<record<<'\n';continue;}
			check(item.direction<4,"Invalid wall direction in original MOB");
			XeenObjectFile isolated{99,"isolated.mob",true,{}};auto copy=item;copy.x=copy.y=8;isolated.entities.wallItems.push_back(copy);
			XeenWorld isolatedWorld([](auto n) {return emptyIndoor(n);},[&](auto) {return isolated;});
			const auto count=assets.spriteFrameCount(numbered(item.resourceId,"pic"));
			for(unsigned tick=0;tick<count;++tick) {
				const auto commands=XeenIndoorScene().build(isolatedWorld,{99,8,8,static_cast<XeenDirection>(item.direction)},nullptr,nullptr,{}, {},false,[&](auto name) {return assets.spriteFrameCount(name);});
				const auto found=std::find_if(commands.begin(),commands.end(),[](auto &c) {return c.wallItem()!=nullptr;});
				check(found!=commands.end()&&found->wallItem()->frame==tick,"Silently skipped valid wall item");
				composer.drawIndoorCommands(assets,{*found});isolatedWorld.scenePresentation().advance(99);
			}
		}
		// Cover every actual cell/facing. Draw every selected actor/wall command;
		// sample full scene composition once per map through its World owner.
		bool composed=false;
		const unsigned extent=map.geometry.isOutdoors()?16:32;
		for(unsigned y=0;y<extent;++y)for(unsigned x=0;x<extent;++x) {
			const auto sample=world.sceneCell(id,x,y);if(!sample)continue;
			for(unsigned direction=0;direction<4;++direction) {
				bool complete=true;
				const XeenCamera c{std::uint16_t(id),int(x),int(y),static_cast<XeenDirection>(direction)};
				if(map.geometry.isOutdoors()) {
					const auto commands=XeenOutdoorScene().build(world,c,&resolver,nullptr,0);
					verifyActors(commands,c);
					for(const auto &command:commands) if(!command.actor()&&!command.object()&&!command.projectile()) {
						const auto &draw=command.terrain();const bool present=resourcePresent(id,draw.resourceName);complete=present&&complete;
						if(present&&sceneryFrames.emplace(draw.resourceName,draw.frame).second) composer.drawOutdoorCommands(assets,{command});
					}
					for(const auto &command:commands) if(command.actor()) {composer.drawOutdoorCommands(assets,{command});++actorCommands;}
					for(const auto &command:commands) if(const auto *actor=command.actor())
						if(!loopSpot&&view.at(actor->identity.recordIndex).statistics->loopAnimation()) {
							std::cout<<"SPOT loop: "<<id<<' '<<x<<' '<<y<<' '<<direction<<'\n';loopSpot=true;
						}
					const auto type=sample->geometry->surfaceTypes[sample->cell->surfaceIndex];
					if(id!=23&&(type==0||type==8)&&waterSpots<1) {std::cout<<"SPOT water: "<<id<<' '<<x<<' '<<y<<' '<<direction<<'\n';++waterSpots;}
				} else {
					const auto commands=XeenIndoorScene().build(world,c,&resolver,nullptr,0,{},false,[&](auto name) {return assets.spriteFrameCount(name);});
					verifyActors(commands,c);
					for(const auto &command:commands) if(!command.actor()&&!command.object()&&!command.wallItem()&&!command.projectile()) {
						const auto &draw=command.geometry();const bool present=resourcePresent(id,draw.resourceName);complete=present&&complete;
						if(present&&sceneryFrames.emplace(draw.resourceName,draw.frame).second) composer.drawIndoorCommands(assets,{command});
					}
					bool hasActor=false,hasWall=false,hasLoop=false,hasEffect=false,hasFlying=false;
					for(const auto &command:commands) if(command.actor()||command.wallItem()) {composer.drawIndoorCommands(assets,{command});actorCommands+=command.actor()!=nullptr;wallCommands+=command.wallItem()!=nullptr;hasActor|=command.actor()!=nullptr;hasWall|=command.wallItem()!=nullptr;}
					for(const auto &command:commands) if(const auto *a=command.actor()) {
						const auto &data=*view.at(a->identity.recordIndex).statistics;
						hasLoop|=data.loopAnimation();hasEffect|=data.animationEffect()!=0;hasFlying|=data.flying();
					}
					if(id!=28&&map.geometry.wallKind==0&&hasActor&&x<16&&y<16&&townSpots<1) {std::cout<<"SPOT town: "<<id<<' '<<x<<' '<<y<<' '<<direction<<'\n';++townSpots;}
					if(hasLoop&&!loopSpot&&x<16&&y<16) {std::cout<<"SPOT loop: "<<id<<' '<<x<<' '<<y<<' '<<direction<<'\n';loopSpot=true;}
					if(map.geometry.wallKind!=0&&hasActor&&hasWall&&x<16&&y<16) {
						for(auto [kind,eligible,found]:{std::tuple{"effect",hasEffect,&effectSpot},std::tuple{"flying",hasFlying,&flyingSpot}})
							if(eligible&&!*found) {std::cout<<"SPOT dungeon-"<<kind<<": "<<id<<' '<<x<<' '<<y<<' '<<direction<<'\n';*found=true;}
					}
				}
				if(!composed&&complete) {XeenPartyState party;check(composer.compose(assets,world,party,c,{},nullptr,0).isValid(),"Invalid map composition");composed=true;++fullCompositions;}
			}
		}
		check(composed,"Map silently skipped in production composition");
		// Crowded cells can legitimately hide records after the third. Isolate
		// every actual record at a front-rank cell and require its identity/image
		// through the complete production scene builder and native drawer.
		for(auto record:expectedRecords) {
			XeenObjectFile isolated{std::uint16_t(id),mob.resourceName,true,{}};
			isolated.entities=mob.entities;
			for(auto &m:isolated.entities.monsters)m.x=m.y=-128;
			isolated.entities.monsters[record]=mob.entities.monsters[record];
			isolated.entities.monsters[record].x=isolated.entities.monsters[record].y=8;
			XeenWorld single([&](auto n) {return maps.at(n.number);},[&](auto) {return isolated;});
			single.sceneActors(std::uint16_t(id),[&] {return statistics;});
			const XeenCamera camera{std::uint16_t(id),8,8,XeenDirection::North};
			const auto verify=[&](const auto &commands) {
				const auto found=std::find_if(commands.begin(),commands.end(),[&](const auto &c) {return c.actor() && c.actor()->identity==XeenMonsterIdentity{std::uint16_t(id),record};});
				check(found!=commands.end() && found->actor()->image==statistics[mob.entities.monsters[record].resourceId].image(),"Isolated actual MOB record emitted no expected command: map "+std::to_string(id)+" record "+std::to_string(record));
				++checkedActorRecords;
			};
			if(map.geometry.isOutdoors()) {const auto commands=XeenOutdoorScene().build(single,camera,&resolver,nullptr,0);verify(commands);composer.drawOutdoorCommands(assets,commands);}
			else {const auto commands=XeenIndoorScene().build(single,camera,&resolver,nullptr,0,{},false,[&](auto name) {return assets.spriteFrameCount(name);});verify(commands);composer.drawIndoorCommands(assets,commands);}
		}
		world.discardMapCache();assets.discardSpriteCache();
	}
	check(!world.hasEncounterState()&&world.sessionState().actors().empty()&&!world.sessionState().journeyRandom(),"Render inventory initialized combat/RNG");
	std::cout<<"INVENTORY monsters="<<statistics.size()<<" images="<<images.size()<<" looped="<<looped<<" flying="<<flying<<" effects="<<effects.size()<<" image-sentinels="<<sentinels<<" MON/ATT projected-frames="<<monsterFrames
		<<" maps="<<maps.size()<<" full-compositions="<<fullCompositions<<" MOBs="<<mobs<<" missing-MOBs="<<missingMobs<<" missing-presentation-references="<<missingReferences.size()<<" hash-aliases="<<aliases<<" actor-records="<<actors<<" disabled="<<disabled<<" unresolved="<<unresolved<<" wall-records="<<wallItems<<" inactive-walls="<<inactiveWalls<<" wall-resources="<<wallResources.size()<<" wall-unique-frames="<<uniqueWallFrames.size()<<" wall-decoded-draws="<<wallFrames<<" actor-commands="<<actorCommands<<" wall-commands="<<wallCommands<<'\n';
	std::cout<<"SCENERY unique-decoded-frames="<<sceneryFrames.size()<<'\n';
	std::cout<<"MOB independent expected/drawn actor records="<<expectedActorRecords<<'/'<<checkedActorRecords<<'\n';
	check(expectedActorRecords==checkedActorRecords,"MOB actor record coverage incomplete");
	for(const auto &reference:missingReferences)
		check(originalDataGaps.count(reference),"New missing presentation reference outside documented original-data gaps: map "+std::to_string(reference.first)+" -> "+reference.second);
	std::cout<<"Missing references outside approved original-data gaps=0; approved gaps="<<originalDataGaps.size()<<'\n';
	return 0;
} catch(const std::exception &e) {std::cerr<<"Scene inventory failed: "<<e.what()<<'\n';return 1;}}
