#define FORBIDDEN_SYMBOL_ALLOW_ALL
#include "formats/xeen/XeenAssetSource.h"
#include "formats/xeen/XeenObjectSpriteSafety.h"
#include "common/memstream.h"
#include "mm/shared/xeen/sprites.h"
#include "games/xeen/CloudsMapComposer.h"
#include "games/xeen/CloudsUiComposer.h"
#include "games/xeen/XeenCharacterRules.h"
#include "games/xeen/XeenInstallationDetector.h"
#include "games/xeen/XeenMapLoader.h"
#include "games/xeen/XeenPartyLoader.h"
#include "games/xeen/XeenWorld.h"
#include <algorithm>
#include <filesystem>
#include <fstream>
#include <iostream>

using namespace mmodern;
namespace {
void check(bool b,const char *s){if(!b)throw std::runtime_error(s);}
void save(const IndexedFrame &f,const std::filesystem::path &path){
	std::ofstream out(path,std::ios::binary);
	auto u16=[&](unsigned n){out.put(n&255);out.put((n>>8)&255);};
	auto u32=[&](unsigned n){u16(n&65535);u16(n>>16);};
	out.put('B');out.put('M');u32(54+320*200*3);u32(0);u32(54);
	u32(40);u32(320);u32(200);u16(1);u16(24);u32(0);u32(320*200*3);u32(0);u32(0);u32(0);u32(0);
	for(int y=199;y>=0;--y)for(int x=0;x<320;++x){const auto p=f.pixels[y*320+x]*3;out.put(f.palette[p+2]);out.put(f.palette[p+1]);out.put(f.palette[p]);}
	check(bool(out),"frame output failed");
}
const XeenOutdoorDrawCommand &find(const std::vector<XeenOutdoorDrawCommand> &commands,XeenObjectIdentity id){
	for(const auto &c:commands)if(c.object() && c.object()->visual.identity==id)return c;
	throw std::runtime_error("checkpoint object command missing");
}
// Same test-only directory-cell instrumentation as 16A, this time over the
// real scene underlay and with the real projection/clipping options.
class SceneCellProbe : public MM::Shared::Xeen::SpriteResource {
public:
	std::vector<std::uint8_t> drawCell(const std::vector<std::uint8_t> &bytes,
		const XeenOutdoorDrawCommand &command,const IndexedFrame &underlay,unsigned cell) {
		const auto frame=command.object()->visual.frame;
		validateXeenObjectSprite(bytes,frame);
		Common::MemoryReadStream input(bytes.data(),static_cast<uint32>(bytes.size()));load(input);
		check(_index[frame]._offset2!=0,"Air / Corner second cell missing");
		if(cell==2)_index[frame]._offset1=_index[frame]._offset2;
		_index[frame]._offset2=0;
		MM::Shared::Xeen::XSurface surface;surface.create(320,200);
		for(int y=0;y<200;++y)std::copy_n(underlay.pixels.data()+y*320,320,static_cast<std::uint8_t *>(surface.getBasePtr(0,y)));
		const auto options=command.drawOptions();
		unsigned flags=MM::Shared::Xeen::SPRFLAG_SCENE_CLIPPED;
		if(options.horizontalFlip)flags|=MM::Shared::Xeen::SPRFLAG_HORIZ_FLIPPED;
		if(options.bottomClipped){setClippedBottom(140);flags|=MM::Shared::Xeen::SPRFLAG_BOTTOM_CLIPPED;}
		draw(surface,static_cast<int>(frame),Common::Point(command.x,command.y),flags,options.scaleIndex);
		std::vector<std::uint8_t> result;
		for(int y=0;y<200;++y){const auto *p=static_cast<const std::uint8_t *>(surface.getBasePtr(0,y));result.insert(result.end(),p,p+320);}
		return result;
	}
};
}
int main(int argc,char **argv){try{
	check(argc==3,"usage: mmodern_outdoor_object_smoke <game-directory> <ignored-output-directory>");
	const auto install=XeenInstallationDetector().detect(argv[1]);check(install && install->hasXeen(),"Clouds unavailable");
	const std::filesystem::path output=argv[2];std::filesystem::create_directories(output);
	XeenAssetSource assets(*install,320,200);const XeenMapLoader loader;
	const auto party=XeenPartyLoader().loadInitialCloudsParty(assets);
	const XeenCharacterRulesContext context{kCloudsInitialYear};const CloudsMapComposer composer;
	const auto resolver=XeenObjectVisualResolver::load(assets);
	struct Case{const char *name;XeenCamera camera;std::size_t record;int sample,order,x,y,scale,frame;bool flip;};
	const Case cases[]={
		{"phirna-current",{23,8,2,XeenDirection::North},13,2,111,-5,2,0,0,false},
		{"phirna-depth1",{23,8,3,XeenDirection::South},13,7,87,-7,25,7,0,false},
		{"phirna-depth2",{23,8,4,XeenDirection::South},13,14,66,-8,50,12,0,false},
		{"phirna-depth3-mirrored",{23,5,2,XeenDirection::East},13,27,37,-9,58,14,0,true},
		{"air-corner",{1,1,14,XeenDirection::West},4,2,111,-5,2,0,0,false},
		{"117-north",{23,12,2,XeenDirection::North},11,2,111,-5,2,0,1,false},
		{"117-east",{23,12,2,XeenDirection::East},11,2,111,-5,2,0,0,false},
		{"117-south",{23,12,2,XeenDirection::South},11,2,111,-5,2,0,3,false},
		{"117-west",{23,12,2,XeenDirection::West},11,2,111,-5,2,0,2,false}
	};
	std::size_t occludedCases=0, visibleCases=0;
	for(const auto &c:cases){
		std::map<XeenMapIdentity,int> mobs;
		XeenWorld world([&](auto id){return loader.loadGeometryMap(assets,id);},[&](auto id){++mobs[id];return loader.loadObjects(assets,id);});
		std::vector<XeenObjectVisual> diagnostics;
		const auto commands=XeenOutdoorScene().build(world,c.camera,&resolver,&diagnostics);
		const auto id=XeenObjectIdentity{c.camera.mapId,c.record};const auto &cmd=find(commands,id);
		const auto &visual=cmd.object()->visual;const auto options=cmd.drawOptions();
		check(cmd.sampleIndex==c.sample && cmd.originalOrder==c.order && cmd.x==c.x && cmd.y==c.y &&
			options.scaleIndex==c.scale && options.sceneClipped && options.bottomClipped==(c.sample==2) && !options.enlarge &&
			visual.frame==static_cast<std::size_t>(c.frame) && visual.horizontalFlip==c.flip,"real command differs from approved checkpoint");
		check(mobs.size()==1 && mobs.begin()->first==c.camera.mapId,"neighbor object load");
		const auto full=composer.compose(assets,world,party,c.camera,context,&diagnostics);save(full,output/(std::string(c.name)+".bmp"));
		// Trace the same production command stream before border/UI to distinguish
		// terrain occlusion from frame clipping or interface overlap.
		CloudsUiComposer().loadBackground(assets);IndexedFrame before,after;bool reached=false;
		std::vector<XeenOutdoorDrawCommand> laterTerrain;
		for(const auto &draw:commands){
			const bool target=draw.object() && draw.object()->visual.identity==id;
			if(target)before=assets.snapshot();
			composer.drawOutdoorCommands(assets,{draw});
			if(target){after=assets.snapshot();reached=true;}
			else if(reached && !draw.object())laterTerrain.push_back(draw);
		}
		if(visual.spriteName=="054.obj"){
			const auto bytes=assets.readArchiveResource(visual.spriteName);
			check(SceneCellProbe().drawCell(bytes,cmd,before,1)!=after.pixels &&
				SceneCellProbe().drawCell(bytes,cmd,before,2)!=after.pixels,"both Air / Corner cells must contribute inside scene");
		}
		// Recreate the through-target surface, then only its later terrain.
		CloudsUiComposer().loadBackground(assets);
		for(const auto &draw:commands){composer.drawOutdoorCommands(assets,{draw});if(draw.object() && draw.object()->visual.identity==id)break;}
		composer.drawOutdoorCommands(assets,laterTerrain);const auto terrainAfter=assets.snapshot();
		std::size_t touched=0,occluded=0;
		for(std::size_t p=0;p<before.pixels.size();++p)if(before.pixels[p]!=after.pixels[p]){++touched;if(terrainAfter.pixels[p]!=after.pixels[p])++occluded;}
		if(occluded && occluded<touched)++occludedCases;
		world.disableObject(id);const auto disabled=composer.compose(assets,world,party,c.camera,context);
		// The approved Phirna depth-1/2 views contain intervening ltree.wal
		// commands. A valid object can be fully hidden in the composed frame.
		check(touched>0,"object sprite did not draw before occlusion");
		if(full.pixels!=disabled.pixels)++visibleCases;
		else check(occluded>0,"invisible checkpoint without terrain occlusion");
		std::size_t changed=0;
		for(int y=0;y<200;++y)for(int x=0;x<320;++x){const auto p=y*320+x;if(full.pixels[p]!=disabled.pixels[p]){++changed;check(x>=8 && x<223 && y>=8 && y<141,"object changed interface outside scene");}}
		const auto rebuilt=XeenOutdoorScene().build(world,c.camera,&resolver);
		check(std::none_of(rebuilt.begin(),rebuilt.end(),[&](const auto &v){return v.object() && v.object()->visual.identity==id;}),"disabled command remained");
		world.discardMapCache();check(composer.compose(assets,world,party,c.camera,context).pixels==disabled.pixels && mobs[c.camera.mapId]==2,"base reload revived disabled object");
		if(c.record==13 && c.sample==2)save(disabled,output/"phirna-explicit-rebuild-disabled.bmp");
		std::cout<<c.name<<" frame="<<visual.frame<<" flip="<<visual.horizontalFlip<<" anchor="<<cmd.x<<','<<cmd.y<<" scale="<<options.scaleIndex<<" changed="<<changed<<" touched="<<touched<<" terrain-occluded="<<occluded<<" diagnostics="<<diagnostics.size()<<'\n';
		if(!changed)for(const auto &draw:laterTerrain)std::cout<<"  later terrain order="<<draw.originalOrder<<' '<<draw.terrain().resourceName<<" source="<<draw.sourceX<<','<<draw.sourceY<<'\n';
	}
	std::cout<<"Partially terrain-occluded checkpoint views="<<occludedCases<<'\n';
	check(occludedCases>0 && visibleCases>=7,"missing visible/partially occluded real coverage");
	std::cout<<"Outdoor real checkpoints and explicit disabled reconstruction passed\n";return 0;
}catch(const std::exception &e){std::cerr<<e.what()<<'\n';return 1;}}
