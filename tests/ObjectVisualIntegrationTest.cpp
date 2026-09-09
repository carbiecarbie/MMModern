#define FORBIDDEN_SYMBOL_ALLOW_ALL
#include "formats/xeen/XeenAssetSource.h"
#include "formats/xeen/XeenObjectSpriteSafety.h"
#include "games/xeen/XeenObjectVisual.h"
#include "games/xeen/XeenInstallationDetector.h"
#include "games/xeen/XeenMapLoader.h"
#include "common/memstream.h"
#include "mm/shared/xeen/sprites.h"
#include <algorithm>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <stdexcept>

using namespace mmodern;
namespace {
void check(bool value,const char *message) {if(!value) throw std::runtime_error(message);}
unsigned word(const std::vector<std::uint8_t> &b,std::size_t p) {return b.at(p)|(unsigned(b.at(p+1))<<8);}
void saveBmp(const IndexedFrame &frame,const std::filesystem::path &path) {
	std::ofstream out(path,std::ios::binary);
	auto u16=[&](unsigned n) {out.put(n&255);out.put((n>>8)&255);};
	auto u32=[&](unsigned n) {u16(n&65535);u16(n>>16);};
	const unsigned stride=(frame.width*3+3)&~3;
	out.put('B');out.put('M');u32(54+stride*frame.height);u32(0);u32(54);
	u32(40);u32(frame.width);u32(frame.height);u16(1);u16(24);u32(0);u32(stride*frame.height);
	u32(0);u32(0);u32(0);u32(0);
	for(int y=frame.height-1;y>=0;--y) {
		for(int x=0;x<frame.width;++x) {
			const auto p=frame.pixels[y*frame.width+x]*3;
			out.put(frame.palette[p+2]);out.put(frame.palette[p+1]);out.put(frame.palette[p]);
		}
		for(unsigned x=frame.width*3;x<stride;++x) out.put(0);
	}
	check(bool(out),"BMP output failed");
}
std::uint64_t hash(const IndexedFrame &frame) {
	std::uint64_t h=14695981039346656037ull;
	for(auto pixel:frame.pixels) {h^=pixel;h*=1099511628211ull;}return h;
}
// Test-only instrumentation of the reused rasterizer: disable one directory
// cell in an in-memory decoder instance. Never alter resource bytes or expose
// a cell-selection feature in production. This proves both cells contribute.
class CellProbe : public MM::Shared::Xeen::SpriteResource {
public:
	CellProbe(const std::vector<std::uint8_t> &bytes,std::size_t frame,unsigned cell) {
		validateXeenObjectSprite(bytes,frame);
		Common::MemoryReadStream input(bytes.data(),static_cast<uint32>(bytes.size()));
		load(input);
		if(cell==2) _index[frame]._offset1=_index[frame]._offset2;
		if(cell) _index[frame]._offset2=0;
	}
	std::vector<std::uint8_t> pixels(std::size_t frame,bool flip) {
		MM::Shared::Xeen::XSurface surface;surface.create(320,200);surface.clear(0);
		draw(surface,static_cast<int>(frame),Common::Point(0,0),flip?MM::Shared::Xeen::SPRFLAG_HORIZ_FLIPPED:0);
		std::vector<std::uint8_t> result;
		for(int y=0;y<200;++y) {
			const auto *p=static_cast<const std::uint8_t *>(surface.getBasePtr(0,y));
			result.insert(result.end(),p,p+320);
		}return result;
	}
};
}

int main(int argc,char **argv) {
	try {
		check(argc==3,"usage: mmodern_object_visual_smoke <game-directory> <ignored-output-directory>");
		const auto installation=XeenInstallationDetector().detect(argv[1]);
		check(installation && installation->hasXeen(),"Clouds installation missing");
		std::filesystem::create_directories(argv[2]);
		const char *directions[]={"north","east","south","west"};
		struct Checkpoint {unsigned map,index;int x,y,id,direction;const char *name;};
		const Checkpoint cases[]={{23,13,8,2,111,0,"phirna"},{1,4,1,14,54,3,"air-corner"},{23,11,12,2,117,1,"directional-117"}};
		for(const auto &c:cases) for(unsigned camera=0;camera<4;++camera) {
			XeenAssetSource assets(*installation,320,200);
			const auto rawMetadata=assets.readCloudsVisualMetadataFromDarkArchive();
			check(rawMetadata && rawMetadata->size()==1452,"real clouds.dat layout differs from approved table");
			const auto metadata=XeenCloudsVisualMetadata::parse(*rawMetadata);
			const auto objects=XeenMapLoader().loadObjects(assets,XeenMapIdentity{static_cast<std::uint16_t>(c.map)});
			const auto &object=objects.entities.objects.at(c.index);
			check(object.x==c.x && object.y==c.y && object.resourceId==c.id && object.direction==c.direction,"real MOB checkpoint mismatch");
			const auto resolver=XeenObjectVisualResolver::load(assets);
			const auto v=resolver.resolve(objects,c.index,static_cast<XeenDirection>(camera));
			check(v.status==XeenObjectVisualStatus::SupportedStatic,"real checkpoint is not supported static");
			check(v.identity==XeenObjectIdentity{objects.mapId,c.index},"real Clouds identity changed");
			const auto &entry=metadata.at(c.id);
			if(camera==0) {
				std::cout<<c.name<<" metadata initial/flip/limit:";
				for(unsigned d=0;d<4;++d) std::cout<<' '<<unsigned(entry.initialFrames[d])<<'/'<<unsigned(entry.flipFlags[d])<<'/'<<unsigned(entry.frameLimits[d]);
				std::cout<<'\n';
			}
			if(c.id==111) {
				check(object.tableIndex==8 && v.spriteName=="111.0bj" && v.frame==0 && v.horizontalFlip==(camera%2==1),"Phirna resolution mismatch");
				for(unsigned d=0;d<4;++d) check(entry.initialFrames[d]==0 && entry.flipFlags[d]==d%2 && entry.frameLimits[d]==0,"Phirna metadata mismatch");
			}
			if(c.id==117) {
				const unsigned expected[]={1,0,3,2};
				check(v.spriteName=="117.0bj" && v.frame==expected[camera] && !v.horizontalFlip,"117 expected frame/flip mismatch");
			}
			if(c.id==54) check(v.spriteName=="054.obj","Air/Corner resource name mismatch");
			const auto bytes=assets.readArchiveResource(v.spriteName);
			validateXeenObjectSprite(bytes,v.frame);
			const auto first=word(bytes,2+v.frame*4), second=word(bytes,4+v.frame*4);
			if(c.id==111) check(bytes.size()==748 && word(bytes,0)==1 && second==0,"Phirna sprite structure mismatch");
			else check(second!=0,"expected two populated cells");
			assets.loadPalette("mm4.pal");
			assets.drawObjectVisual(v,0,0);
			const auto output=assets.snapshot();
			check(std::count_if(output.pixels.begin(),output.pixels.end(),[](auto p){return p!=0;})>0,"empty isolated output");
			check(output.pixels==CellProbe(bytes,v.frame,0).pixels(v.frame,v.horizontalFlip),"production draw differs from original rasterizer");
			if(second) {
				check(output.pixels!=CellProbe(bytes,v.frame,1).pixels(v.frame,v.horizontalFlip),"second cell did not contribute");
				check(output.pixels!=CellProbe(bytes,v.frame,2).pixels(v.frame,v.horizontalFlip),"first cell did not contribute");
			}
			assets.drawObjectVisual(v,0,0);check(assets.snapshot().pixels==output.pixels,"cached draw is not deterministic");
			for(std::uint64_t phase:{0,1,99}) {
				const auto explicitVisual=resolver.resolve(objects,c.index,static_cast<XeenDirection>(camera),phase);
				check(explicitVisual.status==v.status && explicitVisual.frame==v.frame && explicitVisual.horizontalFlip==v.horizontalFlip && explicitVisual.identity==v.identity,"real static phase invariance");
				assets.loadRawFramebuffer("back.raw");assets.drawObjectVisual(v,0,0);const auto staticControl=assets.snapshot();
				assets.loadRawFramebuffer("back.raw");assets.drawObjectVisual(explicitVisual,0,0);
				check(assets.snapshot().pixels==staticControl.pixels,"real static pixels changed under phase");
			}
			saveBmp(output,std::filesystem::path(argv[2])/(std::string(c.name)+"-"+directions[camera]+".bmp"));
			std::cout<<c.name<<' '<<directions[camera]<<" sprite="<<v.spriteName<<" frame="<<v.frame<<" flip="<<v.horizontalFlip<<" bytes="<<bytes.size()<<" frames="<<word(bytes,0)<<" cells="<<(second?2:1)<<" hash="<<hash(output)<<" headers:";
			for(auto offset:{first,second}) if(offset) std::cout<<" ["<<word(bytes,offset)<<','<<word(bytes,offset+2)<<','<<word(bytes,offset+4)<<','<<word(bytes,offset+6)<<']';
			std::cout<<'\n';
		}
		{
			XeenAssetSource assets(*installation,320,200);
			const auto objects=XeenMapLoader().loadObjects(assets,23);
			const auto &myra=objects.entities.objects.at(1);
			check(objects.resourceName=="maze0023.mob" && myra.x==9 && myra.y==11 && myra.direction==3 && myra.resourceId==9 && myra.tableIndex==1,"Myra original record mismatch");
			const auto metadata=XeenCloudsVisualMetadata::parse(*assets.readCloudsVisualMetadataFromDarkArchive());
			const auto &entry=metadata.at(9);
			check(entry.initialFrames==std::array<std::uint8_t,4>{0,4,4,4} && entry.frameLimits==std::array<std::uint8_t,4>{3,7,7,7} && entry.flipFlags==std::array<std::uint8_t,4>{0,0,1,0},"Myra original metadata mismatch");
			const auto bytes=assets.readArchiveResource("009.obj");check(word(bytes,0)==8,"Myra actual directory count");
			const auto resolver=XeenObjectVisualResolver::load(assets);
			for(unsigned camera=0;camera<4;++camera) {
				const auto direction=static_cast<XeenDirection>(camera);
				const auto omitted=resolver.resolve(objects,1,direction);
				check(omitted.status==XeenObjectVisualStatus::UnsupportedAnimation && !omitted.diagnostic.empty(),"Myra omitted phase must remain unsupported");
				std::vector<IndexedFrame> renders;
				for(std::uint64_t phase:{0,1,2,3}) {
					const auto v=resolver.resolve(objects,1,direction,phase);
					check(v.identity==XeenObjectIdentity{23,1} && v.spriteName=="009.obj" && v.status==XeenObjectVisualStatus::SupportedAnimated && v.frame==(camera==3?0:4)+phase%3 && v.horizontalFlip==(camera==1),"Myra directional explicit frame");
					assets.loadPalette("mm4.pal");assets.loadRawFramebuffer("back.raw");assets.drawObjectVisual(v,0,0);
					renders.push_back(assets.snapshot());
					saveBmp(renders.back(),std::filesystem::path(argv[2])/(std::string("myra-")+directions[camera]+"-phase-"+std::to_string(phase)+".bmp"));
				}
				check(renders[0].pixels==renders[3].pixels,"Myra isolated wrap");
				check(renders[0].pixels!=renders[1].pixels || renders[1].pixels!=renders[2].pixels,"Myra isolated complete-cycle motion");
				assets.loadRawFramebuffer("back.raw");assets.drawObjectVisual(resolver.resolve(objects,1,direction,1),0,0);
				check(assets.snapshot().pixels==renders[1].pixels,"Myra repeated phase underlay");
			}
			std::cout<<"Myra {Clouds,23,1}, (9,11), West, 009.obj: eight directory frames; initial=[0,4,4,4], limit=[3,7,7,7], flip=[0,0,1,0]; West frames=0/1/2/0; four directional cycles passed\n";
		}
		std::cout<<"All three real static controls and explicit-phase Myra passed\n";return 0;
	} catch(const std::exception &e) {std::cerr<<e.what()<<'\n';return 1;}
}
