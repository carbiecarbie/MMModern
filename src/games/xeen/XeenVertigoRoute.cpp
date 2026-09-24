#include "games/xeen/XeenVertigoRoute.h"
#include <array>
#include <stdexcept>
#include <vector>

namespace mmodern {
namespace {
struct Site {unsigned index,offset,x,y,direction,line,opcode;std::vector<std::uint8_t> bytes;};
void exact(const XeenEventFile &file,const Site &s) {
	if(s.index>=file.records.size())throw std::invalid_argument("Vertigo Event record is absent");
	const auto &r=file.records[s.index];
	if(r.fileOffset!=s.offset || r.lengthField!=5+s.bytes.size() || r.x!=s.x || r.y!=s.y ||
		r.direction!=s.direction || r.line!=s.line || r.opcode!=s.opcode || r.parameters!=s.bytes)
		throw std::invalid_argument("Vertigo original Event graph changed");
	for(unsigned i=0;i<file.records.size();++i)if(i!=s.index) {
		const auto &other=file.records[i];
		if(other.x==s.x && other.y==s.y && other.direction==s.direction && other.line==s.line)
			throw std::invalid_argument("Vertigo Event address has a duplicate record");
	}
}
}
void xeenValidateVertigoRoute(const XeenEventFile &mainland,const XeenEventFile &city) {
	if(!mainland.resourcePresent || mainland.mapId!=XeenMapIdentity(23) || mainland.records.size()!=170 ||
		!city.resourcePresent || city.mapId!=XeenMapIdentity(28) || city.records.size()!=847)
		throw std::invalid_argument("Vertigo Event catalog changed");
	for(const Site &s:{
		Site{136,1141,10,13,4,0,0x01,{33}},Site{137,1148,10,13,4,1,0x09,{44,0,3}},
		Site{138,1157,10,13,4,2,0x12,{}},Site{139,1163,10,13,4,3,0x07,{28,15,0}}
	})exact(mainland,s);
	for(const Site &s:{
		Site{539,4471,13,4,3,0,0x02,{33}},
		Site{760,6468,15,0,2,0,0x19,{75,76,0}},Site{761,6477,15,0,2,1,0x01,{57}},
		Site{762,6484,15,0,2,2,0x09,{44,0,4}},Site{763,6493,15,0,2,3,0x12,{}},
		Site{764,6499,15,0,2,4,0x2f,{}},Site{765,6505,15,0,2,5,0x18,{4,0}},
		Site{766,6513,15,0,2,6,0x09,{20,9,8}},Site{767,6522,15,0,2,7,0x19,{100,100,0}},
		Site{768,6531,15,0,4,8,0x1b,{84,2}},Site{769,6539,15,0,4,9,0x07,{23,10,12}},
		Site{816,6998,75,76,4,0,0x09,{20,231,2}},Site{817,7007,75,76,4,1,0x08,{9,0,3}},
		Site{818,7016,75,76,4,2,0x1a,{}},Site{819,7022,75,76,4,3,0x0c,{0,0,20,231}},
		Site{846,7292,75,76,4,30,0x1a,{}},Site{813,6978,100,100,4,43,0x1a,{}}
	})exact(city,s);
	for(unsigned line=4;line<=29;++line) {
		const unsigned flag=line<=27 ? 232+line-4 : line==28 ? 57 : 58;
		exact(city,{816+line,7032+10*(line-4),75,76,4,line,0x0c,{20,static_cast<std::uint8_t>(flag),0,0}});
	}
	struct Spawn {unsigned slot,x,y,unused;};
	static constexpr std::array<Spawn,43> reset{{
		{0,1,11,0},{1,1,11,0},{2,2,9,0},{3,3,10,0},{4,3,11,0},
		{5,3,11,0},{6,3,13,0},{7,3,13,0},{8,3,27,0},{9,4,27,0},
		{10,4,26,0},{11,4,25,0},{12,4,12,0},{13,4,7,0},{14,4,7,0},
		{15,4,3,0},{16,4,3,0},{17,4,3,0},{18,5,12,0},{19,9,18,0},
		{20,25,14,0},{21,28,9,0},{22,30,9,0},{23,30,6,0},{24,29,15,0},
		{25,8,24,0},{26,8,24,0},{27,7,23,0},{28,7,23,0},{29,8,27,0},
		{30,8,27,0},{31,9,18,0},{32,6,2,1},{33,7,1,1},{34,6,6,1},
		{35,7,7,1},{36,15,4,0},{37,22,9,0},{38,21,1,0},{39,22,1,0},
		{40,30,1,0},{50,7,24,0},{51,6,27,0}
	}};
	for(unsigned line=0;line<reset.size();++line) {
		const auto s=reset[line];
		exact(city,{770+line,6548+10*line,100,100,4,line,0x10,
			{static_cast<std::uint8_t>(s.slot),static_cast<std::uint8_t>(s.x),
			static_cast<std::uint8_t>(s.y),static_cast<std::uint8_t>(s.unused)}});
	}
}
} // namespace mmodern
