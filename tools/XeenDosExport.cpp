#include "tools/XeenDosExport.h"
#include "compat/scummvm/ScummVmXeenBridge.h"
#include "formats/xeen/XeenAssetSource.h"
#include "formats/xeen/XeenCharacterFormat.h"
#include "formats/xeen/XeenGameplayContextFormat.h"
#include "formats/xeen/XeenQuestFlagFormat.h"
#include "formats/xeen/XeenEventFormat.h"
#include "formats/xeen/XeenMapFormat.h"
#include "games/xeen/CloudsMapComposer.h"
#include "games/xeen/XeenPartyLoader.h"
#include "games/xeen/XeenEventLoader.h"
#include "games/xeen/XeenMapLoader.h"
#include "games/xeen/XeenSaveState.h"
#include "platform/XeenSaveFile.h"
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#include <algorithm>
#include <atomic>
#include <fstream>
#include <iomanip>
#include <sstream>

namespace mmodern::developer {
namespace {
namespace fs=std::filesystem;
DosMember &member(DosMembers &members,const std::string &name) {
	const auto it=std::find_if(members.begin(),members.end(),[&](const auto &m){return m.name==name;});
	if(it==members.end()) throw std::runtime_error("Missing DOS New Game member: "+name);
	return *it;
}
void validateExportState(const XeenSaveSnapshot &s) {
	if(s.camera.mapId.side!=XeenSide::Clouds) throw std::invalid_argument("Darkside input saves cannot be exported");
	if(s.insideInn || !s.camera.mapId.number) throw std::invalid_argument("Cannot export a party inside an Inn (map 0)");
	for(const auto &m:s.maps) if(m.id.side!=XeenSide::Clouds) throw std::invalid_argument("Darkside map history cannot be exported");
}
// Local resource adapters, matching normal startup; no extra gameplay owner.
void restore(const GameInstallation &installation,const XeenSaveSnapshot &s) {
	XeenAssetSource assets(installation,320,200); XeenMapLoader maps;
	XeenEventLoader events([&](const auto &name)->std::optional<DosBytes> {
		if(!assets.hasInitialResource(name)) return {}; return assets.readInitialResource(name);
	});
	XeenEventTextLoader texts([&](const auto &name)->std::optional<DosBytes> {
		if(!assets.hasArchiveResource(name)) return {}; return assets.readArchiveResource(name);
	});
	XeenSaveState::Resources r;
	r.signature=XeenSaveFile::fingerprint(installation);
	r.loadInitialParty=[&]{return XeenPartyLoader().loadInitialCloudsParty(assets);};
	r.loadInitialCharacters=[&]{return assets.readInitialResource("maze.chr");};
	r.loadInitialContext=[&]{return XeenGameplayContextFormat::parse(assets.readInitialResource("maze.pty"));};
	r.loadInitialPurse=[&]{return XeenCharacterFormat::parseMonsterPurse(assets.readInitialResource("maze.pty"));};
	r.loadInitialBankBalances=[&]{return XeenCharacterFormat::parseBankBalances(assets.readInitialResource("maze.pty"));};
	r.loadInitialWorldFlags=[&]{return XeenQuestFlagFormat::parseWorldFlags(assets.readInitialResource("maze.pty"));};
	r.loadEvents=[&](auto id){return events.load(id);};
	r.loadRegionalText=[&](auto id){return texts.load(id);};
	r.loadMonsterStatistics=[&]{
		const auto b=assets.readCloudsMonsterStatisticsFromDarkArchive();
		if(!b) throw std::runtime_error("Missing DARK.CC/xeen.mon"); return XeenMonsterFormat::parse(*b);
	};
	r.loadLearnedSpellNames=[&]{
		const auto b=assets.readLearnedSpellNamesFromDarkArchive();
		if(!b) throw std::runtime_error("Missing DARK.CC/spells.xen"); return XeenLearnedSpellNames::parse(*b);
	};
	r.regionalManifest=[&](const auto &map,const auto &mob,const auto &evt,const auto &mon) {
		xeenValidateRegionalManifest(map,mob,evt,mon,assets.readInitialResource("maze0023.dat"),
			assets.readInitialResource("maze0023.mob"),assets.readInitialResource("maze0023.evt"));
	};
	XeenWorld world([&](auto id){return maps.loadGeometryMap(assets,id);},[&](auto id){return maps.loadObjects(assets,id);});
	XeenPartyState party; XeenCamera camera; XeenGameFlags flags;
	XeenSaveState::restoreBeforeGameplay(s,r,party,camera,flags,world,[&](auto &w,const auto &p,const auto &c,const auto &) {
		(void)CloudsMapComposer().compose(assets,w,p,c,{p.encounterContext->year});
	});
}
fs::path outputPath(const fs::path &requested,const GameInstallation &installation) {
	if(requested.empty()) throw std::invalid_argument("Output folder is empty");
	const bool extended=requested.native().rfind(L"\\\\?\\",0)==0;
	if(extended) {
		if(requested.native().find(L'/')!=std::wstring::npos) throw std::invalid_argument("Extended DOS output path requires native separators");
		for(const auto &part:requested) if(part==L"." || part==L"..") throw std::invalid_argument("Extended DOS output path cannot contain relative components");
	}
	auto ancestor=extended?requested:fs::absolute(requested).lexically_normal().make_preferred(); fs::path suffix;
	while(!fs::exists(ancestor)) {
		if(ancestor==ancestor.root_path()) throw std::runtime_error("Output folder has no existing ancestor");
		suffix=ancestor.filename()/suffix; ancestor=ancestor.parent_path();
	}
	// Reuse the save platform's Windows handle resolution, including junctions,
	// links, case and 8.3 names, and all installation protected directories.
	fs::path resolved;
	try {resolved=XeenSaveFile::resolve(ancestor/L".dos-export-guard.mmsave",installation).parent_path();}
	catch(const std::exception &e) {throw std::runtime_error("Resolve DOS output folder "+ancestor.u8string()+": "+e.what());}
	// Extended Windows paths require native separators; lexical normalization
	// after handle resolution would turn those back into forward slashes in MinGW.
	auto result=(resolved/suffix).make_preferred();
	while(result.has_relative_path() && result.filename().empty()) result=result.parent_path();
	return result;
}
void ordinaryTarget(const fs::path &p,bool overwrite) {
	const auto a=GetFileAttributesW(p.c_str());
	if(a==INVALID_FILE_ATTRIBUTES) {
		if(GetLastError()==ERROR_FILE_NOT_FOUND || GetLastError()==ERROR_PATH_NOT_FOUND) return;
		throw std::runtime_error("Cannot inspect DOS slot target (Windows error "+std::to_string(GetLastError())+")");
	}
	if(!overwrite) throw std::runtime_error("DOS slot file already exists; use --overwrite");
	if(a&(FILE_ATTRIBUTE_REPARSE_POINT|FILE_ATTRIBUTE_DIRECTORY|FILE_ATTRIBUTE_DEVICE))
		throw std::runtime_error("DOS slot target must be an ordinary file");
}
void write(const fs::path &path,const DosBytes &b) {
	std::ofstream f(path,std::ios::binary|std::ios::trunc);
	f.exceptions(std::ios::badbit|std::ios::failbit); f.write(reinterpret_cast<const char *>(b.data()),b.size()); f.close();
}
void removeStage(const fs::path &stage) {
	// The staging directory is flat and owned by this invocation. Windows may
	// retain a delete-pending entry briefly after its file handle closes.
	for(unsigned attempt=0;attempt<50;++attempt) {
		for(const auto *name:{"xeen.new","dark.new","xeen.old","dark.old"}) DeleteFileW((stage/name).c_str());
		if(RemoveDirectoryW(stage.c_str())) return;
		const auto error=GetLastError();
		if(error==ERROR_FILE_NOT_FOUND || error==ERROR_PATH_NOT_FOUND) return;
		if(error!=ERROR_ACCESS_DENIED && error!=ERROR_SHARING_VIOLATION && error!=ERROR_DIR_NOT_EMPTY)
			throw std::runtime_error("Cannot remove DOS staging folder (Windows error "+std::to_string(error)+"): "+stage.u8string());
		Sleep(100);
	}
	throw std::runtime_error("Cannot remove DOS staging folder: "+stage.u8string());
}
}
bool validDosExportName(const std::string &name) noexcept {
	return !name.empty() && name.size()<=kDosExportNameLimit && name[0]!=' ' &&
		std::all_of(name.begin(),name.end(),[](unsigned char c){return c>=32 && c<=126;});
}
DosBytes dosExportNam(const std::string &name) {
	if(!validDosExportName(name)) throw std::invalid_argument("DOS export name requires 1..15 printable ASCII characters and no leading space");
	DosBytes b(31,0); std::copy(name.begin(),name.end(),b.begin()); b[30]=1; return b;
}
DosExportPair dosNewGame(const GameInstallation &installation) {
	if(!installation.hasXeen() || !installation.hasDarkside()) throw std::runtime_error("DOS export requires both XEEN and DARK archives");
	ScummVmXeenBridge bridge(installation); DosExportPair pair;
	for(unsigned side=0;side<2;++side) {
		DosBytes bytes;
		for(const auto *block:{"2a0c","2a1c","2a2c","2a3c","284c","2a5c"}) {
			// Missing tail blocks are normal. The first block always carries the index.
			const auto selection=side?XeenSceneArchive::DarksideOnly:XeenSceneArchive::Clouds;
			if(side ? !bridge.hasSceneResource(block,selection) : !bridge.hasArchiveResource(block)) continue;
			const auto b=bridge.readArchiveResource(block,selection); bytes.insert(bytes.end(),b.begin(),b.end());
		}
		auto members=readDosArchive(bytes); nameDosMembers(members);
		if(members.size()!=(side?444:301)) throw std::runtime_error("Unexpected DOS New Game member count");
		for(const auto &m:members) {
			const auto checked=side?bridge.readInitialDarkResource(m.name):bridge.readInitialResource(m.name);
			if(checked!=m.bytes) throw std::runtime_error("DOS New Game member disagrees with bridge reader");
		}
		(side?pair.darkside:pair.clouds)=std::move(members);
	}
	return pair;
}
DosExportPair projectDosSave(const GameInstallation &installation,const XeenSaveSnapshot &s,const std::string &name) {
	const auto nam=dosExportNam(name); validateExportState(s); restore(installation,s);
	auto pair=dosNewGame(installation);
	member(pair.clouds,"maze.nam").bytes=nam; member(pair.darkside,"maze.nam").bytes=nam;
	XeenCharacterFormat::writeDosRoster(member(pair.darkside,"maze.chr").bytes,s);
	XeenCharacterFormat::writeDosParty(member(pair.darkside,"maze.pty").bytes,s);
	for(const auto &m:s.maps) {
		XeenMapFormat::writeDosDat(member(pair.clouds,dosMapName(m.id.number,"dat")).bytes,m);
		if(m.monsters || !m.objects.empty() || !m.wallItems.empty())
			XeenMapFormat::writeDosMob(member(pair.clouds,dosMapName(m.id.number,"mob")).bytes,m);
		if(!m.events.empty()) XeenEventFormat::writeDos(member(pair.clouds,dosMapName(m.id.number,"evt")).bytes,m);
	}
	return pair;
}
std::string dosProjectionSummary() {
	return "New Game projection: XEEN party replica; DARK maps, flags, quests and character flags; "
		"prior map, PTY16, unused party-ID suffix, unmodeled counters/flags and CHR name padding; DAT seen bits.\n"
		"DOS does not store monster HP/status, Spawn types/extra slots, object flips, head cache, MMModern RNG or process state.\n";
}
void exportDosSave(const GameInstallation &installation,const fs::path &input,const fs::path &requested,
	unsigned slot,const std::string &name,bool overwrite) {
	if(slot<1 || slot>10) throw std::invalid_argument("DOS slot must be in 1..10");
	(void)dosExportNam(name);
	const auto out=outputPath(requested,installation);
	std::ostringstream number; number<<std::setw(2)<<std::setfill('0')<<slot;
	const std::array<fs::path,2> targets{out/("XEEN"+number.str()+".WOX"),out/("DARK"+number.str()+".WOX")};
	for(const auto &p:targets) ordinaryTarget(p,overwrite);
	const auto s=XeenSaveFile::read(fs::absolute(input));
	const auto pair=projectDosSave(installation,s,name);
	const std::array<DosBytes,2> bytes{writeDosArchive(pair.clouds),writeDosArchive(pair.darkside)};
	fs::create_directories(out);
	if(outputPath(out,installation)!=out) throw std::runtime_error("Output folder changed during export");
	static std::atomic<unsigned> sequence{0}; fs::path stage;
	for(unsigned attempts=0;attempts<100;++attempts) {
		stage=out/(".dos-export-"+std::to_string(GetCurrentProcessId())+"-"+std::to_string(++sequence));
		if(fs::create_directory(stage)) break;
		stage.clear();
	}
	if(stage.empty()) throw std::runtime_error("Cannot create DOS pair staging folder");
	std::array<bool,2> backed{},published{};
	try {
		for(unsigned i=0;i<2;++i) write(stage/(i?"dark.new":"xeen.new"),bytes[i]);
		for(const auto &p:targets) ordinaryTarget(p,overwrite);
		for(unsigned i=0;i<2;++i) if(fs::exists(targets[i])) {
			fs::rename(targets[i],stage/(i?"dark.old":"xeen.old")); backed[i]=true;
		}
		for(unsigned i=0;i<2;++i) {fs::rename(stage/(i?"dark.new":"xeen.new"),targets[i]); published[i]=true;}
	} catch(...) {
		for(unsigned i=0;i<2;++i) {
			if(published[i]) fs::remove(targets[i]);
			if(backed[i]) fs::rename(stage/(i?"dark.old":"xeen.old"),targets[i]);
		}
		removeStage(stage); throw;
	}
	removeStage(stage);
}
}
