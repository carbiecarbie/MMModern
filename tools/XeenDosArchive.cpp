#define FORBIDDEN_SYMBOL_ALLOW_ALL
#include "tools/XeenDosArchive.h"
#include "formats/xeen/XeenMapFormat.h"
#include "common/memstream.h"
#include "common/path.h"
#include "mm/shared/xeen/cc_archive.h"
#include <cstdio>
#include <set>
#include <stdexcept>

namespace mmodern::developer {
namespace {
class Archive final : public MM::Shared::Xeen::BaseCCArchive {
public:
	Common::SeekableReadStream *createReadStreamForMember(const Common::Path &) const override {return nullptr;}
	DosMembers read(const DosBytes &b) {
		if(b.size()<2) throw std::invalid_argument("Truncated DOS CC archive");
		const unsigned count=b[0]|unsigned(b[1])<<8;
		const std::size_t header=2+count*8;
		if(!count || header>b.size()) throw std::invalid_argument("Invalid DOS CC index extent");
		for(unsigned i=7;i<count*8;i+=8) {
			const unsigned v=b[2+i];
			if((((v<<2)|(v>>6))+0xac+i*0x67)%256) throw std::invalid_argument("Invalid DOS CC reserved byte");
		}
		Common::MemoryReadStream stream(b.data(),b.size()); loadIndex(stream);
		DosMembers result; std::set<std::uint16_t> ids;
		std::size_t end=header;
		for(const auto &e:_index) {
			const auto start=static_cast<std::size_t>(e._offset);
			if(start<end || start>b.size() || e._size>b.size()-start || !ids.insert(e._id).second)
				throw std::invalid_argument("Invalid DOS CC member extent or duplicate ID");
			result.push_back({{},e._id,{b.begin()+start,b.begin()+start+e._size}}); end=start+e._size;
		}
		return result;
	}
	DosBytes write(const DosMembers &members) {
		if(members.empty() || members.size()>65535) throw std::invalid_argument("Invalid DOS CC member count");
		std::size_t offset=2+members.size()*8; std::set<std::uint16_t> ids;
		for(const auto &m:members) {
			if(m.bytes.size()>65535 || (!m.name.empty() && dosMemberId(m.name)!=m.id) || !ids.insert(m.id).second)
				throw std::invalid_argument("Invalid DOS CC member size, name or duplicate ID");
			MM::Shared::Xeen::CCEntry entry(m.id,offset,m.bytes.size()); entry._writeOffset=offset;
			_index.push_back(entry); offset+=m.bytes.size();
		}
		if(offset>0xffffff) throw std::invalid_argument("DOS CC archive exceeds 24-bit offsets");
		Common::MemoryWriteStreamDynamic out(DisposeAfterUse::YES);
		// The pinned index writer; SaveArchive::save adds incompatible ScummVM framing.
		saveIndex(out);
		for(const auto &m:members) out.write(m.bytes.data(),m.bytes.size());
		if(out.err()) throw std::runtime_error("Cannot encode DOS CC archive");
		return {out.getData(),out.getData()+out.size()};
	}
};
}
std::uint16_t dosMemberId(const std::string &name) {
	if(name.empty()) throw std::invalid_argument("Empty DOS member name");
	return MM::Shared::Xeen::BaseCCArchive::convertNameToId(Common::Path(name.c_str(),Common::Path::kNoSeparator));
}
std::string dosMapName(unsigned id,const char *ext) {
	char name[32]; std::snprintf(name,sizeof(name),id>=100?"mazex%03u.%s":"maze%04u.%s",id,ext); return name;
}
DosMembers readDosArchive(const DosBytes &b) {return Archive().read(b);}
DosBytes writeDosArchive(const DosMembers &m) {return Archive().write(m);}
void nameDosMembers(DosMembers &members) {
	std::vector<unsigned> maps;
	for(auto &m:members) {
		for(const auto *name:{"maze.nam","maze.chr","maze.pty"}) if(dosMemberId(name)==m.id) m.name=name;
		if(!m.name.empty() || m.bytes.size()!=892) continue;
		const unsigned id=m.bytes[768]|unsigned(m.bytes[769])<<8;
		if(id && id<=999 && dosMemberId(dosMapName(id,"dat"))==m.id) {
			(void)XeenMapFormat::parseDat(m.bytes); m.name=dosMapName(id,"dat"); maps.push_back(id);
		}
	}
	for(auto &m:members) if(m.name.empty()) {
		for(unsigned id:maps) for(const auto *ext:{"mob","evt"}) {
			const auto name=dosMapName(id,ext);
			if(dosMemberId(name)==m.id) {
				if(!m.name.empty()) throw std::runtime_error("Ambiguous canonical DOS map member");
				m.name=name;
			}
		}
		if(m.name.empty()) throw std::runtime_error("Unrecognized New Game member ID " + std::to_string(m.id));
	}
}
}
