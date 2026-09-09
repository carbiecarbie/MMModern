#include "SyntheticXeenArchive.h"
#include "formats/xeen/XeenAssetSource.h"
#include "games/xeen/CloudsMapComposer.h"
#include "games/xeen/XeenWorld.h"
#include "games/xeen/XeenParty.h"
#include "games/xeen/XeenCharacterRules.h"
#include <algorithm>
#include <chrono>
#include <iostream>

using namespace mmodern;
using namespace sprite_test;
namespace {
void check(bool b,const char *s){if(!b)throw std::runtime_error(s);}
Bytes frames(const Bytes &c,unsigned count) {
	Bytes b;word(b,count);for(unsigned i=0;i<count;++i){word(b,2+count*4);word(b,0);}
	b.insert(b.end(),c.begin(),c.end());return b;
}
Bytes solid(unsigned width,unsigned height,unsigned color) {
	Bytes rows;
	for(unsigned y=0;y<height;++y) {
		Bytes payload{0};
		for(unsigned x=0;x<width;) {const auto n=std::min(32u,width-x);payload.push_back(n-1);payload.insert(payload.end(),n,color);x+=n;}
		rows.push_back(payload.size());rows.insert(rows.end(),payload.begin(),payload.end());
	}return cell(0,width,0,height,rows);
}
}
int main(){try {
	const auto directory=std::filesystem::temp_directory_path()/("mmodern-composer-"+std::to_string(std::chrono::steady_clock::now().time_since_epoch().count()));
	std::filesystem::create_directories(directory);
	struct Cleanup{std::filesystem::path p;~Cleanup(){std::error_code e;std::filesystem::remove_all(p,e);}} cleanup{directory};
	const auto empty=frames(cell(0,0,0,0,{}),41);
	std::map<std::string,Bytes> files;
	for(const char *name:{"sky.sky","water.out","water.srf","space.srf","global.icn","border.icn","fecp.brd","bless.icn","restorex.icn","main.icn"})files[name]=empty;
	files["back.raw"]=Bytes(64000,99);files["mm4.pal"]=Bytes(768);
	files["111.0bj"]=sprite(solid(200,100,7));
	files["mount.wal"]=frames(solid(80,90,42),3);
	// A real border draw after the scene must win at this pixel.
	files["global.icn"]=sprite(solid(1,1,55));
	archive(directory/"xeen.cc",files);
	GameInstallation installation;installation.xeenArchive=directory/"xeen.cc";
	installation.darkArchive=directory/"dark.cc";archive(installation.darkArchive,{{"clouds.dat",Bytes(1452)}});
	auto geometry=[](XeenMapIdentity id){XeenMap m;m.geometry.id=id.number;m.side=id.side;m.geometry.flags2=0x8000;
		m.geometry.wallTypes[1]=1;for(auto &c:m.geometry.cells)c.geometry=XeenOutdoorLayers{};
		// Far terrain before the object, near terrain after it.
		std::get<XeenOutdoorLayers>(m.geometry.cells[12*16+8].geometry).middle=1;
		std::get<XeenOutdoorLayers>(m.geometry.cells[9*16+8].geometry).middle=1;return m;};
	auto objectFile=[](XeenMapIdentity id){XeenObjectFile f;f.mapId=id;f.resourcePresent=true;f.entities.objects={{8,9,0,0,111},{8,9,0,0,111}};return f;};
	const XeenCamera camera{23,8,8,XeenDirection::North};
	{
		XeenAssetSource assets(installation,320,200);XeenWorld world(geometry,objectFile);
		const auto r=XeenObjectVisualResolver::load(assets);
		const auto commands=XeenOutdoorScene().build(world,camera,&r);
		const CloudsMapComposer composer;
		assets.loadRawFramebuffer("back.raw");
		IndexedFrame beforeObject,afterObject;
		for(const auto &c:commands){if(c.object())beforeObject=assets.snapshot();composer.drawOutdoorCommands(assets,{c});if(c.object())afterObject=assets.snapshot();}
		const auto scene=assets.snapshot();
		std::size_t coversEarlier=0,coveredLater=0,objectSurvives=0;
		for(std::size_t p=0;p<scene.pixels.size();++p){
			if(beforeObject.pixels[p]==42 && afterObject.pixels[p]==7)++coversEarlier;
			if(afterObject.pixels[p]==7 && scene.pixels[p]==42)++coveredLater;
			if(scene.pixels[p]==7)++objectSurvives;
		}
		check(coversEarlier && coveredLater && objectSurvives,"terrain/object pixel interleaving");
		std::cout<<"Pixels: object covers earlier terrain="<<coversEarlier<<", later terrain covers object="<<coveredLater<<", object survives="<<objectSurvives<<'\n';
		std::vector<XeenObjectVisual> diagnostics;
		const auto full=composer.compose(assets,world,{},camera,{},&diagnostics);
		bool presence=true;
		for(std::uint64_t phase:{0,1,999}) {
			presence=true;check(composer.compose(assets,world,{},camera,{},nullptr,phase,&presence).pixels==full.pixels && !presence,"static scene phase invariance/presence");
		}
		check(diagnostics.empty() && full.pixels[8*320+8]==55,"composer border or diagnostics");
		for(int y=8;y<141;++y)for(int x=8;x<223;++x)if(x!=8 || y!=8)
			check(full.pixels[y*320+x]==scene.pixels[y*320+x],"composer did not execute ordered stream");
		world.disableObject({23,0});world.disableObject({23,1});
		const auto disabled=composer.compose(assets,world,{},camera,{});
		check(full.pixels!=disabled.pixels && std::count(disabled.pixels.begin(),disabled.pixels.end(),7)==0,"explicit reconstruction left object pixels");
		world.discardMapCache();check(composer.compose(assets,world,{},camera,{}).pixels==disabled.pixels,"cache reconstruction restored pixels");
		for(int y=0;y<200;++y)for(int x=0;x<320;++x)if(x<8 || x>=223 || y<8 || y>=141)
			check(full.pixels[y*320+x]==disabled.pixels[y*320+x],"object escaped scene clipping");
	}
	// Distinct streams and changing coverage exercise actual frame selection and clean underlays.
	files["110.0bj"]=multiFrameSprite({{solid(200,100,17),{}},{solid(100,60,18),{}},{solid(150,80,19),{}}});
	files["111.0bj"]=sprite(solid(20,20,7));
	Bytes coveredRows;for(int y=0;y<8;++y)coveredRows.insert(coveredRows.end(),{3,0,0x45,17});
	files["109.0bj"]=frames(cell(80,8,40,8,coveredRows),2);
	files["town.sky"]=empty;files["town.gnd"]=empty;files["ftown1.fwl"]=empty;
	archive(directory/"xeen.cc",files);
	Bytes animatedMetadata(1452);animatedMetadata[110*12+8]=3;animatedMetadata[109*12+8]=2;
	archive(installation.darkArchive,{{"clouds.dat",animatedMetadata}});
	{
		XeenAssetSource assets(installation,320,200);
		int maps=0,mobs=0;
		std::vector<XeenMapEntity> records={{8,9,0,0,110},{8,8,0,0,111}};
		XeenWorld world([&](auto id){++maps;return geometry(id);},[&](auto id){++mobs;auto f=objectFile(id);f.entities.objects=records;return f;});
		const CloudsMapComposer composer;bool presence=true;
		const auto omitted=composer.compose(assets,world,{},camera,{},nullptr,std::nullopt,&presence);
		check(!presence,"omitted phase presence");
		std::vector<IndexedFrame> rendered;
		for(std::uint64_t phase:{0,1,2,3}) {
			presence=false;rendered.push_back(composer.compose(assets,world,{},camera,{},nullptr,phase,&presence));
			check(presence && rendered.back().pixels[8*320+8]==55,"animated presence/border");
		}
		check(rendered[0].pixels!=rendered[1].pixels && rendered[0].pixels==rendered[3].pixels,"phase forwarding/wrap pixels");
		for(std::uint64_t phase:{1,0,2,1}) check(composer.compose(assets,world,{},camera,{},nullptr,phase).pixels==rendered[phase].pixels,"repeat/reorder left trails");
		for(std::size_t p=0;p<omitted.pixels.size();++p) if(omitted.pixels[p]==7)
			for(const auto &frame:rendered) check(frame.pixels[p]==7,"isolated static foreground contribution changed");
		const auto resolver=XeenObjectVisualResolver::load(assets);
		for(std::uint64_t phase:{0,1,2}) {
			const auto commands=XeenOutdoorScene().build(world,camera,&resolver,nullptr,phase);
			const auto animated=std::find_if(commands.begin(),commands.end(),[](const auto &c){return c.object() && c.object()->visual.status==XeenObjectVisualStatus::SupportedAnimated;});
			check(animated!=commands.end() && animated->object()->visual.frame==phase,"composer fixture logical frame");
		}
		const auto oldMaps=maps,oldMobs=mobs;const auto loads=assets.spriteLoadCount();
		world.discardMapCache();assets.discardSpriteCache();
		check(composer.compose(assets,world,{},camera,{},nullptr,1).pixels==rendered[1].pixels && maps==oldMaps+1 && mobs==oldMobs+1 && assets.spriteLoadCount()>loads,"same phase cache reconstruction");
		world.disableObject({23,0});presence=true;
		const auto removed=composer.compose(assets,world,{},camera,{},nullptr,2,&presence);
		check(!presence && removed.pixels==omitted.pixels,"removed animation presence/underlay");
		world.discardMapCache();assets.discardSpriteCache();presence=true;
		check(composer.compose(assets,world,{},camera,{},nullptr,2,&presence).pixels==removed.pixels && !presence,"removed cache reconstruction");
		// First static or invalid record suppresses a later animated overlap.
		for(int first:{111,121}) {
			XeenWorld overlap(geometry,[&](auto id){auto f=objectFile(id);f.entities.objects={{8,9,0,0,first},{8,9,0,0,110}};return f;});
			presence=true;composer.compose(assets,overlap,{},camera,{},nullptr,1,&presence);check(!presence,"suppressed animation presence");
		}
		XeenWorld offscreen(geometry,[&](auto id){auto f=objectFile(id);f.entities.objects={{1,1,0,0,110}};return f;});
		presence=true;composer.compose(assets,offscreen,{},camera,{},nullptr,1,&presence);check(!presence,"offscreen animation presence");
		XeenWorld covered(geometry,[&](auto id){auto f=objectFile(id);f.entities.objects={{8,9,0,0,109}};return f;});
		assets.loadRawFramebuffer("back.raw");
		const auto coveredCommands=XeenOutdoorScene().build(covered,camera,&resolver,nullptr,1);
		bool touched=false;
		for(const auto &command:coveredCommands) {
			composer.drawOutdoorCommands(assets,{command});
			if(command.object()) {const auto frame=assets.snapshot();touched=std::count(frame.pixels.begin(),frame.pixels.end(),17)>0;}
		}
		presence=false;const auto coveredFrame=composer.compose(assets,covered,{},camera,{},nullptr,1,&presence);
		check(touched && presence && std::count(coveredFrame.pixels.begin(),coveredFrame.pixels.end(),17)==0,"terrain-covered emitted animation must count");
		XeenWorld indoor([](auto id){XeenMap m;m.side=id.side;m.geometry.id=id.number;for(auto &c:m.geometry.cells)c.geometry=XeenIndoorWalls{};return m;});
		presence=true;composer.compose(assets,indoor,{},camera,{},nullptr,1,&presence);check(!presence,"indoor animation presence");
	}
	installation.darkArchive.clear();
	{
		XeenAssetSource assets(installation,320,200);XeenWorld world(geometry,objectFile);std::vector<XeenObjectVisual> diagnostics;
		const auto full=CloudsMapComposer().compose(assets,world,{},camera,{},&diagnostics);
		check(full.isValid() && full.pixels[8*320+8]==55 && std::count(full.pixels.begin(),full.pixels.end(),42)>0,"metadata absence broke terrain/interface");
		check(diagnostics.size()==1 && diagnostics[0].status==XeenObjectVisualStatus::MetadataUnavailable && diagnostics[0].identity==XeenObjectIdentity{23,0},"missing metadata precedence/diagnostic");
		check(std::count(full.pixels.begin(),full.pixels.end(),7)==0,"fabricated object metadata");
		bool presence=true;check(CloudsMapComposer().compose(assets,world,{},camera,{},nullptr,1,&presence).pixels==full.pixels && !presence,"missing metadata explicit phase/presence");
	}
	installation.darkArchive=directory/"dark.cc";archive(installation.darkArchive,{{"clouds.dat",Bytes(12)}});
	{
		XeenAssetSource assets(installation,320,200);XeenWorld world(geometry,objectFile);bool rejected=false;
		bool presence=true;
		try{CloudsMapComposer().compose(assets,world,{},camera,{},nullptr,1,&presence);}catch(const std::runtime_error &e){rejected=std::string(e.what()).find("truncated")!=std::string::npos;}
		check(rejected && !presence,"present corrupt metadata was hidden or presence leaked");
	}
	// Failure after emitting an animated command must not publish presence.
	archive(installation.darkArchive,{{"clouds.dat",animatedMetadata}});
	files["110.0bj"]=multiFrameSprite({{solid(10,10,17),{}},{cell(0,8,0,1,{2,0,1}),{}}});archive(directory/"xeen.cc",files);
	{
		XeenAssetSource assets(installation,320,200);XeenWorld world(geometry,[&](auto id){auto f=objectFile(id);f.entities.objects={{8,9,0,0,110}};return f;});
		bool presence=true,rejected=false;
		try{CloudsMapComposer().compose(assets,world,{},camera,{},nullptr,1,&presence);}catch(const std::runtime_error &){rejected=true;}
		check(rejected && !presence,"failed animated draw published presence");
	}
	std::cout<<"Outdoor composition tests passed\n";return 0;
}catch(const std::exception &e){std::cerr<<e.what()<<'\n';return 1;}}
