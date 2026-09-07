#include "SyntheticXeenArchive.h"
#include "formats/xeen/XeenAssetSource.h"
#include "formats/xeen/XeenObjectSpriteSafety.h"
#include "games/xeen/XeenObjectVisual.h"
#include <chrono>
#include <iostream>

using namespace mmodern;
using namespace sprite_test;
namespace {
void check(bool v,const char *s) { if(!v) throw std::runtime_error(s); }
template<class F> void rejects(F f) {
	try { f(); } catch(const std::runtime_error &e) { check(*e.what(),"empty diagnostic"); return; }
	throw std::runtime_error("malformed/unsupported sprite accepted");
}
}
int main() {
	try {
		const auto first=cell(0,8,0,2,{8,0,1,0,7,0xa1,1,8,9,0,0});
		const auto second=cell(1,3,0,1,{5,0,2,12,13,14});
		const auto one=sprite(first), two=sprite(first,second);
		validateXeenObjectSprite(one,0); validateXeenObjectSprite(two,0);
		std::vector<Bytes> malformed={{},{1},{0,0},{1,0},{1,0,0,0,0,0}};
		auto bad=one;setWord(bad,2,65535);malformed.push_back(bad);
		bad=one;setWord(bad,4,one.size()-7);malformed.push_back(bad);
		bad=one;setWord(bad,8,321);malformed.push_back(bad);
		bad=one;bad.pop_back();malformed.push_back(bad);
		bad=one;bad[14]=255;malformed.push_back(bad);
		malformed.push_back(sprite(cell(0,8,0,1,{2,0,1}))); // literal missing operands
		malformed.push_back(sprite(cell(0,8,0,1,{3,0,0x60,0}))); // copy missing operand
		malformed.push_back(sprite(cell(0,8,0,1,{4,0,0x60,255,255}))); // copy underflow
		malformed.push_back(sprite(cell(0,8,0,1,{4,0,0x7f,0,0}))); // copy beyond EOF
		malformed.push_back(sprite(cell(0,8,0,1,{3,0,0x5f,42}))); // expanded row overflow
		malformed.push_back(sprite(cell(0,8,0,1,{0,1}))); // skip past height
		malformed.push_back(sprite(cell(0,8,0,1,{1,9}))); // offset past width
		for (auto opcode : {0x40,0x80,0xc0,0xe0})
			malformed.push_back(sprite(cell(0,8,0,1,{2,0,static_cast<std::uint8_t>(opcode)})));
		for(const auto &b:malformed) rejects([&] {validateXeenObjectSprite(b,0);});
		rejects([&] {validateXeenObjectSprite(one,1);});
		// Exercise each opcode family, including a bounded source copy.
		const auto commands=sprite(cell(0,128,0,1,{16,0,0,5,0x40,6,0x60,3,0,0x80,7,8,0xa0,0xc0,9,0xe0,10}));
		validateXeenObjectSprite(commands,0);
		Bytes longLiteralRow{35,0,0x20};
		for (int i=0;i<33;++i) longLiteralRow.push_back(i);
		const auto longLiteral=sprite(cell(0,33,0,1,longLiteralRow));
		Bytes solidRows;
		for(int i=0;i<16;++i) solidRows.insert(solidRows.end(),{3,0,0x4d,42});
		const auto solid=sprite(cell(0,16,0,16,solidRows));
		const auto directory=std::filesystem::temp_directory_path()/
			("mmodern-sprite-"+std::to_string(std::chrono::steady_clock::now().time_since_epoch().count()));
		std::filesystem::create_directories(directory);
		struct Cleanup { std::filesystem::path p; ~Cleanup() {std::error_code e;std::filesystem::remove_all(p,e);} } cleanup{directory};
		std::map<std::string,Bytes> files={{"one.obj",one},{"two.obj",two},{"solid.obj",solid},
			{"commands.obj",commands},{"literal.obj",longLiteral},{"base.raw",Bytes(320*200,99)},{"clouds.dat",Bytes(12,7)}};
		for(std::size_t i=0;i<malformed.size();++i) files["bad"+std::to_string(i)+".obj"]=malformed[i];
		archive(directory/"xeen.cc",files);
		GameInstallation installation;installation.xeenArchive=directory/"xeen.cc";
		XeenObjectFile objects;objects.mapId=23;objects.resourcePresent=true;objects.entities.objects.resize(1);
		objects.entities.objects[0].resourceId=111;
		XeenObjectVisual visual;visual.identity={23,0};visual.status=XeenObjectVisualStatus::SupportedStatic;
		{
			XeenAssetSource assets(installation,320,200);
			const auto unavailable=XeenObjectVisualResolver::load(assets).resolve(objects,0,XeenDirection::North);
			check(unavailable.status==XeenObjectVisualStatus::MetadataUnavailable && !unavailable.diagnostic.empty(),"missing DARK metadata");
			check(assets.readArchiveResource("clouds.dat")==Bytes(12,7),"generic Clouds origin changed");
			auto draw=[&](const std::string &name,int x=0,int y=0,XeenSpriteDrawOptions options={}) {
				assets.loadRawFramebuffer("base.raw");visual.spriteName=name;assets.drawObjectVisual(visual,x,y,options);return assets.snapshot();
			};
			const auto normal=draw("one.obj");
			check(normal.pixels[0]==0 && normal.pixels[1]==7,"zero is an opaque palette index");
			check(normal.pixels[2]==99 && normal.pixels[3]==99 && normal.pixels[320]==99,"transparent skips changed underlay");
			check(normal.pixels[4]==8 && normal.pixels[5]==9,"literal pixels");
			const auto composite=draw("two.obj");
			check(composite.pixels[0]==0 && composite.pixels[1]==12 && composite.pixels[2]==13 && composite.pixels[3]==14 && composite.pixels[4]==8,"two cells/order");
			visual.horizontalFlip=true;const auto flipped=draw("one.obj");
			check(flipped.pixels[7]==0 && flipped.pixels[6]==7 && flipped.pixels[3]==8 && flipped.pixels[2]==9 && flipped.pixels[4]==99,"horizontal flip");
			visual.horizontalFlip=false;
			XeenSpriteDrawOptions options;options.scaleIndex=8;const auto scaled=draw("solid.obj",0,0,options);
			for(int y=0;y<16;++y) for(int x=0;x<16;++x)
				check(scaled.pixels[y*320+x]==((y<8 && x>=4 && x<12)?42:99),"scale pixels");
			options={};options.sceneClipped=true;auto clipped=draw("solid.obj",0,0,options);
			check(clipped.pixels[8*320+8]==42 && clipped.pixels[7*320+8]==99 && clipped.pixels[8*320+7]==99,"scene top/left clipping");
			clipped=draw("solid.obj",220,138,options);
			check(clipped.pixels[140*320+222]==42 && clipped.pixels[140*320+223]==99 && clipped.pixels[141*320+222]==99,"scene bottom/right clipping");
			options={};options.bottomClipped=true;clipped=draw("solid.obj",10,138,options);
			check(clipped.pixels[139*320+10]==42 && clipped.pixels[140*320+10]==99,"bottom clipping");
			const auto operations=draw("commands.obj");
			check(operations.pixels[0]==5 && operations.pixels[1]==6 && operations.pixels[3]==6 &&
				operations.pixels[4]==0x60 && operations.pixels[6]==0 && operations.pixels[8]==7 &&
				operations.pixels[9]==8 && operations.pixels[12]==99 && operations.pixels[13]==9 &&
				operations.pixels[16]==10 && operations.pixels[18]==9,"compressed opcode pixels");
			const auto literal=draw("literal.obj");
			for(int i=0;i<33;++i) check(literal.pixels[i]==i,"long literal command");
			rejects([&]{draw("missing.obj");});
			for(std::size_t i=0;i<malformed.size();++i) rejects([&]{draw("bad"+std::to_string(i)+".obj");});
			check(assets.snapshot().pixels==Bytes(320*200,99),"preflight failure changed framebuffer");
			visual.frame=1;rejects([&]{draw("one.obj");});visual.frame=0;
			options={};options.enlarge=true;rejects([&]{draw("one.obj",0,0,options);});
			options={};options.scaleIndex=16;rejects([&]{draw("one.obj",0,0,options);});
			visual.status=XeenObjectVisualStatus::UnsupportedAnimation;rejects([&]{draw("one.obj");});
			visual=unavailable;rejects([&]{draw("one.obj");});
		}
		installation.darkArchive=directory/"dark.cc";
		archive(installation.darkArchive,{{"other.dat",{1}}});
		{ XeenAssetSource assets(installation,320,200);check(!assets.readCloudsVisualMetadataFromDarkArchive(),"missing member"); }
		archive(installation.darkArchive,{{"clouds.dat",{}}});
		{ XeenAssetSource assets(installation,320,200);check(assets.readCloudsVisualMetadataFromDarkArchive()->empty(),"empty vs missing");rejects([&]{XeenObjectVisualResolver::load(assets);}); }
		archive(installation.darkArchive,{{"clouds.dat",Bytes(1452)}});
		{ XeenAssetSource assets(installation,320,200);
			const auto resolved=XeenObjectVisualResolver::load(assets).resolve(objects,0,XeenDirection::North);
			check(resolved.status==XeenObjectVisualStatus::SupportedStatic && resolved.identity==XeenObjectIdentity{23,0},"physical archive changed gameplay identity");
			check(assets.readArchiveResource("clouds.dat")==Bytes(12,7),"archive origin ambiguity");
		}
		std::cout<<"M16 preflight, production rasterizer and archive-origin tests passed\n";return 0;
	} catch(const std::exception &e) {std::cerr<<e.what()<<'\n';return 1;}
}
