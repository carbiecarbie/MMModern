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
	installation.darkArchive.clear();
	{
		XeenAssetSource assets(installation,320,200);XeenWorld world(geometry,objectFile);std::vector<XeenObjectVisual> diagnostics;
		const auto full=CloudsMapComposer().compose(assets,world,{},camera,{},&diagnostics);
		check(full.isValid() && full.pixels[8*320+8]==55 && std::count(full.pixels.begin(),full.pixels.end(),42)>0,"metadata absence broke terrain/interface");
		check(diagnostics.size()==1 && diagnostics[0].status==XeenObjectVisualStatus::MetadataUnavailable && diagnostics[0].identity==XeenObjectIdentity{23,0},"missing metadata precedence/diagnostic");
		check(std::count(full.pixels.begin(),full.pixels.end(),7)==0,"fabricated object metadata");
	}
	installation.darkArchive=directory/"dark.cc";archive(installation.darkArchive,{{"clouds.dat",Bytes(12)}});
	{
		XeenAssetSource assets(installation,320,200);XeenWorld world(geometry,objectFile);bool rejected=false;
		try{CloudsMapComposer().compose(assets,world,{},camera,{});}catch(const std::runtime_error &e){rejected=std::string(e.what()).find("truncated")!=std::string::npos;}
		check(rejected,"present corrupt metadata was hidden");
	}
	std::cout<<"Outdoor composition tests passed\n";return 0;
}catch(const std::exception &e){std::cerr<<e.what()<<'\n';return 1;}}
