#ifndef MMODERN_TEST_SYNTHETIC_XEEN_ARCHIVE_H
#define MMODERN_TEST_SYNTHETIC_XEEN_ARCHIVE_H
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <map>
#include <stdexcept>
#include <string>
#include <vector>

namespace sprite_test {
using Bytes = std::vector<std::uint8_t>;
inline void word(Bytes &b, unsigned n) { b.push_back(n & 255); b.push_back((n >> 8) & 255); }
inline void setWord(Bytes &b, std::size_t at, unsigned n) { b.at(at)=n&255; b.at(at+1)=(n>>8)&255; }
inline unsigned nameId(std::string name) {
	for(auto &c:name) if(c>='a' && c<='z') c-=32;
	unsigned id=static_cast<unsigned char>(name.at(0));
	for(std::size_t i=1;i<name.size();++i)
		id=(((id & 127)<<9)|((id & 0xff80)>>7))+static_cast<unsigned char>(name[i]);
	return id&65535;
}
// Synthetic CC container writer solely to exercise production archive access.
inline void archive(const std::filesystem::path &path, const std::map<std::string,Bytes> &files) {
	Bytes index, payload, output;
	std::size_t offset=2+files.size()*8;
	for(const auto &[name,bytes]:files) {
		word(index,nameId(name)); word(index,offset); index.push_back((offset>>16)&255);
		word(index,bytes.size()); index.push_back(0);
		for(auto b:bytes) payload.push_back(b^0x35);
		offset+=bytes.size();
	}
	word(output,files.size());
	for(std::size_t i=0;i<index.size();++i) {
		const unsigned b=(index[i]-(0xac+i*0x67))&255;
		output.push_back((b>>2)|(b<<6));
	}
	output.insert(output.end(),payload.begin(),payload.end());
	std::ofstream stream(path,std::ios::binary);
	stream.write(reinterpret_cast<const char *>(output.data()),output.size());
	if(!stream) throw std::runtime_error("synthetic archive write failed");
}
inline Bytes sprite(const Bytes &first, const Bytes &second = {}) {
	Bytes b; word(b,1); word(b,6); word(b,second.empty()?0:6+first.size());
	b.insert(b.end(),first.begin(),first.end()); b.insert(b.end(),second.begin(),second.end()); return b;
}
inline Bytes cell(unsigned x,unsigned width,unsigned y,unsigned height,const Bytes &rows) {
	Bytes b; word(b,x);word(b,width);word(b,y);word(b,height);
	b.insert(b.end(),rows.begin(),rows.end());return b;
}
} // namespace sprite_test
#endif
