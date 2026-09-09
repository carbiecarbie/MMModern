#include "games/xeen/XeenObjectVisual.h"
#include <iostream>
#include <limits>
#include <stdexcept>

using namespace mmodern;
namespace {
void check(bool value, const char *message) { if (!value) throw std::runtime_error(message); }
template<class F> void rejects(F f, const std::string &part) {
	try { f(); } catch (const std::runtime_error &e) {
		check(std::string(e.what()).find(part) != std::string::npos, "wrong diagnostic"); return;
	}
	throw std::runtime_error("expected rejection");
}
}
int main() {
	try {
		std::vector<std::uint8_t> bytes(1452);
		for (std::size_t i = 0; i < bytes.size(); ++i) bytes[i] = static_cast<std::uint8_t>(i);
		const auto parsed = XeenCloudsVisualMetadata::parse(bytes);
		for (std::size_t i = 0; i < 121; ++i) for (std::size_t d = 0; d < 4; ++d) {
			check(parsed.at(i).initialFrames[d] == bytes[i*12+d], "initial frames lost");
			check(parsed.at(i).flipFlags[d] == bytes[i*12+4+d], "raw flip values lost");
			check(parsed.at(i).frameLimits[d] == bytes[i*12+8+d], "frame limits lost");
		}
		rejects([&] { parsed.at(121); }, "index");
		rejects([&] { XeenCloudsVisualMetadata::parse({}); }, "empty");
		for (auto size : {1, 12, 1440, 1451})
			rejects([&] { XeenCloudsVisualMetadata::parse(std::vector<std::uint8_t>(size)); }, "truncated");
		for (auto size : {1453, 1464})
			rejects([&] { XeenCloudsVisualMetadata::parse(std::vector<std::uint8_t>(size)); }, "trailing");
		const int ids[] = {0,54,99,100,111,117,254};
		const char *names[] = {"000.obj","054.obj","099.obj","100.0bj","111.0bj","117.0bj","254.0bj"};
		for (int i=0;i<7;++i) check(xeenObjectSpriteName(ids[i]) == names[i], "sprite name");
		for (int id : {-1,255,256,1000}) rejects([&] { xeenObjectSpriteName(id); }, "identifier");

		bytes.assign(1452,0);
		// Directional base metadata; order reflects the original relative table.
		const std::uint8_t frames[] = {0,3,2,1}, flips[] = {0,1,0,255};
		for (int d=0;d<4;++d) { bytes[117*12+d]=frames[d]; bytes[117*12+4+d]=flips[d]; }
		XeenObjectFile file; file.mapId=23; file.resourcePresent=true;
		file.entities.objects.resize(14); file.entities.objects[11].resourceId=117;
		XeenObjectVisualResolver resolver(XeenCloudsVisualMetadata::parse(bytes));
		const unsigned relative[4][4]={{0,1,2,3},{3,0,1,2},{2,3,0,1},{1,2,3,0}};
		for(unsigned object=0;object<4;++object) for(unsigned camera=0;camera<4;++camera) {
			file.entities.objects[11].direction=object;
			const auto v=resolver.resolve(file,11,static_cast<XeenDirection>(camera));
			check(v.identity==XeenObjectIdentity{23,11} && v.spriteName=="117.0bj", "identity/name");
			check(v.frame==frames[relative[object][camera]] && v.horizontalFlip==(flips[relative[object][camera]]!=0), "direction/flip");
			check(v.status==XeenObjectVisualStatus::SupportedStatic, "directional frames are static");
		}
		file.entities.objects[11].direction=1;
		const unsigned expected[]={1,0,3,2};
		for(int d=0;d<4;++d) check(resolver.resolve(file,11,static_cast<XeenDirection>(d)).frame==expected[d], "117 mapping");
		file.entities.objects[11].x=-128;
		check(resolver.resolve(file,11,XeenDirection::North).status==XeenObjectVisualStatus::SupportedStatic,"base visibility leaked into visual resolver");
		file.entities.objects[11].x=0;
		// All initial/limit byte pairs follow increment-then-reset, without uint8 overflow.
		file.entities.objects[11].direction=0;
		for(unsigned initial=0;initial<256;++initial) for(unsigned limit=0;limit<256;++limit) {
			bytes[117*12]=initial; bytes[117*12+8]=limit;
			const XeenObjectVisualResolver r(XeenCloudsVisualMetadata::parse(bytes));
			const auto v=r.resolve(file,11,XeenDirection::North);
			check((v.status==XeenObjectVisualStatus::SupportedStatic)==(initial+1>=limit), "static classification");
			if(initial+1<limit) check(!v.diagnostic.empty(), "animation diagnostic missing");
			const auto originalBytes=bytes;
			for(const std::uint64_t phase : {std::uint64_t{0},std::uint64_t{1},std::uint64_t{2},std::uint64_t{255},std::numeric_limits<std::uint64_t>::max(),std::uint64_t{0}}) {
				const auto a=r.resolve(file,11,XeenDirection::North,phase);
				const bool animated=limit>initial && limit-initial>1;
				check(a.status==(animated?XeenObjectVisualStatus::SupportedAnimated:XeenObjectVisualStatus::SupportedStatic),"explicit phase classification");
				check(a.frame==(animated?initial+phase%(limit-initial):initial),"explicit frame arithmetic");
				check(a.identity==v.identity && a.spriteName==v.spriteName && a.horizontalFlip==v.horizontalFlip && a.diagnostic.empty(),"explicit phase changed identity/flip");
			}
			check(bytes==originalBytes && file.entities.objects[11].resourceId==117 && file.entities.objects[11].direction==0,"resolution mutated input");
			check(r.resolve(file,11,XeenDirection::North).status==v.status,"explicit calls changed omitted mode");
		}
		// Mixed direction ranges: nonzero initial, exclusive limit and high bytes.
		const unsigned starts[]={4,255,253,8}, limits[]={7,0,255,9};
		for(unsigned d=0;d<4;++d) {bytes[117*12+d]=starts[d];bytes[117*12+8+d]=limits[d];bytes[117*12+4+d]=flips[d];}
		const auto metadata=XeenCloudsVisualMetadata::parse(bytes);
		const XeenObjectVisualResolver mixed(metadata);
		for(unsigned object=0;object<4;++object) for(unsigned camera=0;camera<4;++camera) {
			file.entities.objects[11].direction=object;
			const auto d=relative[object][camera];
			for(std::uint64_t phase:{0,1,2,3,4,0}) {
				const auto v=mixed.resolve(file,11,static_cast<XeenDirection>(camera),phase);
				const unsigned sequence[4][5]={{4,5,6,4,5},{255,255,255,255,255},{253,254,253,254,253},{8,8,8,8,8}};
				check(v.frame==sequence[d][phase] && v.horizontalFlip==(flips[d]!=0),"mixed directional cycle");
				check(v.status==((d==0 || d==2)?XeenObjectVisualStatus::SupportedAnimated:XeenObjectVisualStatus::SupportedStatic),"mixed directional status");
				check(metadata.at(117).initialFrames[d]==starts[d] && metadata.at(117).frameLimits[d]==limits[d],"metadata mutated");
			}
		}
		check(resolver.resolve(file,14,XeenDirection::North).status==XeenObjectVisualStatus::Invalid,"record bound");
		file.entities.objects[11].direction=4;
		check(resolver.resolve(file,11,XeenDirection::North).status==XeenObjectVisualStatus::Invalid,"object direction bound");
		file.entities.objects[11].direction=0;
		check(resolver.resolve(file,11,static_cast<XeenDirection>(4)).status==XeenObjectVisualStatus::Invalid,"camera direction bound");
		file.entities.objects[11].resourceId=121;
		check(resolver.resolve(file,11,XeenDirection::North).status==XeenObjectVisualStatus::Invalid,"metadata index bound");
		file.mapId={XeenSide::Darkside,23};
		check(resolver.resolve(file,11,XeenDirection::North).status==XeenObjectVisualStatus::UnsupportedSide,"Darkside accepted");
		check(resolver.resolve(file,11,XeenDirection::North,0).status==XeenObjectVisualStatus::UnsupportedSide,"explicit phase accepted Darkside");
		file.mapId=23;
		check(resolver.resolve(file,11,XeenDirection::North,0).status==XeenObjectVisualStatus::Invalid,"explicit phase accepted missing metadata index");
		file.entities.objects[11].resourceId=117;file.entities.objects[11].direction=4;
		check(resolver.resolve(file,11,XeenDirection::North,0).status==XeenObjectVisualStatus::Invalid,"explicit phase accepted bad direction");
		file.entities.objects[11].direction=0;
		check(resolver.resolve(file,11,static_cast<XeenDirection>(4),0).status==XeenObjectVisualStatus::Invalid,"explicit phase accepted bad camera");
		check(resolver.resolve(file,14,XeenDirection::North,0).status==XeenObjectVisualStatus::Invalid,"explicit phase accepted bad record");
		std::cout << "Object metadata, naming, direction and static classification passed\n";
		return 0;
	} catch (const std::exception &e) { std::cerr << e.what() << '\n'; return 1; }
}
