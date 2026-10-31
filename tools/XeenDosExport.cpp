#include "tools/XeenDosExport.h"
#include "compat/scummvm/ScummVmXeenBridge.h"
#include "formats/xeen/XeenAssetSource.h"
#include "formats/xeen/XeenCharacterFormat.h"
#include "formats/xeen/XeenGameplayContextFormat.h"
#include "formats/xeen/XeenQuestFlagFormat.h"
#include "formats/xeen/XeenEventFormat.h"
#include "formats/xeen/XeenMapFormat.h"
#include "formats/xeen/XeenMonsterFormat.h"
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
	if(s.insideInn || !s.camera.mapId.number) throw std::invalid_argument("Cannot export a party inside an Inn (map 0)");
}
// Local resource adapters, matching normal startup; no extra gameplay owner.
void restore(const GameInstallation &installation,const XeenSaveSnapshot &s) {
	XeenAssetSource assets(installation,320,200); XeenMapLoader maps;
	XeenEventLoader events([&](const std::string &name,XeenSide side)->std::optional<DosBytes> {
		if(!assets.hasInitialResource(name,side)) return {}; return assets.readInitialResource(name,side);
	});
	XeenSaveState::Resources r;
	r.signature=XeenSaveFile::fingerprint(installation);
	r.loadInitialParty=[&]{return XeenPartyLoader().loadInitialParty(assets);};
	r.loadInitialCharacters=[&]{return assets.readInitialResource("maze.chr");};
	r.loadInitialContext=[&]{return XeenGameplayContextFormat::parse(assets.readInitialResource("maze.pty"));};
	r.loadInitialPurse=[&]{return XeenCharacterFormat::parseMonsterPurse(assets.readInitialResource("maze.pty"));};
	r.loadInitialBankBalances=[&]{return XeenCharacterFormat::parseBankBalances(assets.readInitialResource("maze.pty"));};
	r.loadInitialWorldFlags=[&]{return XeenQuestFlagFormat::parseWorldFlags(assets.readInitialResource("maze.pty"));};
	r.loadEvents=[&](auto id){return events.load(id);};
    r.loadRegionalText=[&](auto id){
        const auto archive=xeenPictureArchive(id);
        XeenEventTextLoader loader([&](const std::string &name)->std::optional<DosBytes> {
            if(!assets.hasSceneResource(name,archive))return {};return assets.readArchiveResource(name,archive);
        });return loader.load(id);
    };
    r.loadMonsterStatisticsForSide=[&](XeenSide side){
        const auto bytes=assets.readMonsterStatistics(side);
        if(!bytes)throw std::runtime_error("Missing original side MON");return XeenMonsterFormat::parse(*bytes);
    };
	r.loadMonsterStatistics=[&]{
		const auto b=assets.readMonsterStatistics();
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
    for(auto *members:{&pair.clouds,&pair.darkside})
        member(*members,"maze.nam").bytes.at(29)=static_cast<std::uint8_t>(s.camera.mapId.side);
	XeenCharacterFormat::writeDosRoster(member(pair.darkside,"maze.chr").bytes,s);
	XeenCharacterFormat::writeDosParty(member(pair.darkside,"maze.pty").bytes,s);
	for(const auto &m:s.maps) {
        auto &members=m.id.side==XeenSide::Clouds?pair.clouds:pair.darkside;
		XeenMapFormat::writeDosDat(member(members,dosMapName(m.id.number,"dat")).bytes,m);
		if(m.monsters || !m.objects.empty() || !m.wallItems.empty())
			XeenMapFormat::writeDosMob(member(members,dosMapName(m.id.number,"mob")).bytes,m);
		if(!m.events.empty()) XeenEventFormat::writeDos(member(members,dosMapName(m.id.number,"evt")).bytes,m);
	}
	return pair;
}
void placeDosParty(const GameInstallation &installation,const XeenSaveSnapshot &s,DosExportPair &pair,const DosPlace &place) {
	if((place.side!=XeenSide::Clouds && place.side!=XeenSide::Darkside) || !place.map || place.map>255 || unsigned(place.facing)>3)
		throw std::invalid_argument("Invalid DOS destination map or facing");
	auto &members=place.side==XeenSide::Clouds?pair.clouds:pair.darkside;
	const auto data=[&](unsigned id,const char *ext)->DosBytes & {return member(members,dosMapName(id,ext)).bytes;};
	const auto geometry=[&](unsigned id) {
		const auto g=XeenMapFormat::parseDat(data(id,"dat"));
		if(g.id!=id) throw std::invalid_argument("Destination DAT has the wrong embedded map ID");
		return g;
	};
	const auto main=geometry(place.map);
	const auto entities=XeenMapFormat::parseMob(data(place.map,"mob"));
	(void)XeenEventFormat::parse(data(place.map,"evt"));
	const unsigned limit=main.isOutdoors()?16:32;
	if(place.x>=limit || place.y>=limit) throw std::invalid_argument("Destination cell outside logical map");
	// The DOS/reference load reads the main DAT and up to eight neighbours.
	constexpr unsigned prior[]={0,0,0,0,1,2,3,4}, direction[]={0,1,2,3,1,2,3,0};
	constexpr unsigned alternate[]={0,0,0,0,2,3,4,1}, alternateDirection[]={0,1,2,3,0,1,2,3};
	std::array<XeenMapGeometry,9> grid{}; grid[0]=main;
	for(unsigned i=0;i<8;++i) {
		unsigned id=grid[prior[i]].neighbors[direction[i]];
		if(!id) id=grid[alternate[i]].neighbors[alternateDirection[i]];
		if(id) grid[i+1]=geometry(id);
	}
	// A zero neighbour is an ordinary absence (reference Map::load); only the
	// tile holding an indoor logical coordinate must resolve.
	unsigned tile=place.y>=16?unsigned(grid[0].neighbors[0]):place.map;
	if(!tile) throw std::invalid_argument("Destination cell has no geometry tile");
	if(place.x>=16) {const auto g=geometry(tile);tile=g.neighbors[1];}
	if(!tile) throw std::invalid_argument("Destination cell has no geometry tile");
	ScummVmXeenBridge bridge(installation);
	const auto side=place.side==XeenSide::Clouds?XeenSceneArchive::Clouds:XeenSceneArchive::Darkside;
	const auto require=[&](const std::string &name,XeenSceneArchive selection) {
		if(!bridge.hasSceneResource(name,selection)) throw std::runtime_error("Missing destination load resource: "+name);
	};
	const auto number=[](unsigned n,const char *ext) {char b[24];std::snprintf(b,sizeof(b),"%03u.%s",n,ext);return std::string(b);};
	char txt[24],hed[24],mapname[24];
	std::snprintf(txt,sizeof(txt),place.map>=100?"aazex%03u.txt":"aaze%04u.txt",place.map);
	std::snprintf(hed,sizeof(hed),place.map>=100?"aazex%03u.hed":"aaze%04u.hed",place.map);
	std::snprintf(mapname,sizeof(mapname),place.map>=100?(place.side==XeenSide::Clouds?"xeenx%03u.txt":"darkx%03u.txt"):(place.side==XeenSide::Clouds?"xeen%04u.txt":"dark%04u.txt"),place.map);
	require(txt,side);require(hed,side);require(mapname,XeenSceneArchive::Darkside);
	const bool dark=place.side==XeenSide::Darkside;
	const bool cloudObjects=!dark || (place.map>=113 && place.map<=120) || (place.map>=124 && place.map<=128);
	const bool cloudMonsters=!dark || (place.map>=117 && place.map<=120) || place.map==124;
	const bool cloudPictures=!dark || (place.map>=125 && place.map<=127);
	const auto objectSide=cloudObjects?XeenSceneArchive::Clouds:XeenSceneArchive::Darkside;
	const auto pictureSide=cloudPictures?XeenSceneArchive::Clouds:XeenSceneArchive::Darkside;
	require(cloudObjects?"clouds.dat":"dark.dat",XeenSceneArchive::Darkside);
	require(cloudPictures?"xeenpic.dat":"darkpic.dat",XeenSceneArchive::Darkside);
	const auto monName=cloudMonsters?"xeen.mon":"dark.mon";
	require(monName,XeenSceneArchive::Darkside);
	const auto monsters=XeenMonsterFormat::parse(bridge.readArchiveResource(monName,XeenSceneArchive::Darkside));
	for(auto id:entities.objectTable) if(id!=255) require(number(id,id>=100?"0bj":"obj"),objectSide);
	for(auto id:entities.wallItemTable) if(id!=255) require(number(id,"pic"),pictureSide);
	for(auto id:entities.monsterTable) if(id!=255) {
		if(id>=monsters.size()) throw std::invalid_argument("Destination monster index outside statistics");
		const auto selection=(id==91 || cloudMonsters)?XeenSceneArchive::Clouds:XeenSceneArchive::Darkside;
		require(number(monsters[id].image(),"mon"),selection);require(number(monsters[id].image(),"att"),selection);
	}
	// Names and selection follow the pinned Xeen Map::load and resource tables.
	static const char *const terrain[]={"town","cave","towr","cstl","dung","scfi"};
	static const char *const walls[]={"","mount","ltree","dtree","grass","snotree","dsnotree","snomnt","dedltree","mount","lavamnt","palm","dmount","dedltree","dedltree","dedltree"};
	static const char *const surfaces[]={"water.srf","dirt.srf","grass.srf","snow.srf","swamp.srf","lava.srf","desert.srf","road.srf","dwater.srf","tflr.srf","sky.srf","croad.srf","sewer.srf","cloud.srf","scortch.srf","space.srf"};
	if(main.wallKind>=6) throw std::invalid_argument("Destination wall kind outside resource table");
	static const char *const music[6][7]={
		{"outday1.m","outday2.m","outday4.m","outnght1.m","outnght2.m","outnght4.m","daydesrt.m"},
		{"townday1.m","twnwlk.m","newbrigh.m","twnnitea.m","twnniteb.m","twnwlk.m","townday1.m"},
		{"cavern1.m","cavern2.m","cavern3a.m","cavern1.m","cavern2.m","cavern3a.m","cavern1.m"},
		{"dngon1.m","dngon2.m","dngon3.m","dngon1.m","dngon2.m","dngon3.m","dngon1.m"},
		{"cstl1rev.m","cstl2rev.m","cstl3rev.m","cstl1rev.m","cstl2rev.m","cstl3rev.m","cstl1rev.m"},
		{"sf05.m","sf05.m","sf05.m","sf05.m","sf05.m","sf05.m","sf05.m"}};
	const bool cloudMusic=!dark || (!main.isOutdoors() && place.map>=125 && place.map<=127);
	static const unsigned musicIndex[]={1,2,3,4,3,5};
	static const char *const cloudSongs[]={"outdoors.m","town.m","cavern.m","dungeon.m","castle.m"};
	const unsigned mi=main.isOutdoors()?(dark?unsigned(main.wallKind):0):musicIndex[unsigned(main.wallKind)];
	if(cloudMusic) {if(mi>=5)throw std::invalid_argument("Destination music index outside Clouds table");require(cloudSongs[mi],XeenSceneArchive::Clouds);}
	else for(const auto *song:music[mi]) require(song,XeenSceneArchive::Darkside);
	const auto terrainSide=dark && !main.isOutdoors() && place.map>=125 && place.map<=127?XeenSceneArchive::Clouds:side;
	const auto minutes=s.journey->context->minutes;
	const bool forcedDay=(place.map>=89 && place.map<=112) || place.map==128 || place.map==129;
	require(forcedDay || (minutes>=300 && minutes<1260)?"sky.sky":"night.sky",terrainSide);
	if(main.isOutdoors()) {
		require("water.out",side);require("outdoor.til",side);
		for(unsigned i=0;i<16;++i) {
			const unsigned wall=main.wallTypes[i], surface=main.surfaceTypes[i];
			if(wall>=16 || surface>=16) throw std::invalid_argument("Destination outdoor resource index outside table");
			if(wall)require(std::string(walls[wall])+".wal",side);
			if(i && surface)require(surfaces[surface],side);
		}
	} else {
		const std::string t=terrain[main.wallKind];
		for(const char *ext:{".sky",".gnd",".til"})require(t+ext,terrainSide);
		for(unsigned i=0;i<16;++i) {
			const unsigned index=main.surfaceTypes[i];
			if(index>=16)throw std::invalid_argument("Destination surface index outside table");
			if(index || i==4)require(surfaces[index],terrainSide);
		}
		for(unsigned i=1;i<=4;++i)require("f"+t+std::to_string(i)+".fwl",pictureSide);
		require("s"+t+".swl",pictureSide);
	}
	if(!dark) {auto placed=s;placed.camera={place.map,int(place.x),int(place.y),place.facing};restore(installation,placed);}
	for(auto *m:{&pair.clouds,&pair.darkside}) member(*m,"maze.nam").bytes.at(29)=dark?1:0;
	auto &pty=member(pair.darkside,"maze.pty").bytes;
	pty.at(10)=unsigned(place.facing);pty.at(11)=place.x;pty.at(12)=place.y;pty.at(13)=place.map;
}
std::string dosProjectionSummary() {
	return "New Game projection: XEEN party replica; prior map, PTY16, unused party-ID suffix, "
        "unmodeled counters/completion, quest-flag padding and CHR name padding; DAT seen bits.\n"
		"DOS does not store monster HP/status, Spawn types/extra slots, object flips, head cache, MMModern RNG or process state.\n";
}
void exportDosSave(const GameInstallation &installation,const fs::path &input,const fs::path &requested,
	unsigned slot,const std::string &name,bool overwrite,const std::optional<DosPlace> &place) {
	if(slot<1 || slot>10) throw std::invalid_argument("DOS slot must be in 1..10");
	(void)dosExportNam(name);
	const auto out=outputPath(requested,installation);
	std::ostringstream number; number<<std::setw(2)<<std::setfill('0')<<slot;
	const std::array<fs::path,2> targets{out/("XEEN"+number.str()+".WOX"),out/("DARK"+number.str()+".WOX")};
	for(const auto &p:targets) ordinaryTarget(p,overwrite);
	const auto s=XeenSaveFile::read(fs::absolute(input));
	auto pair=projectDosSave(installation,s,name);
	if(place) placeDosParty(installation,s,pair,*place);
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
void exportDosBoxSave(const GameInstallation &installation,const fs::path &input,const fs::path &requested,
	unsigned slot,const std::string &name,bool overwrite,const std::optional<DosPlace> &place,
	const fs::path &dosbox,const std::optional<fs::path> &cfg) {
	if(slot<1 || slot>10)throw std::invalid_argument("DOS slot must be in 1..10");
	(void)dosExportNam(name);
	if(!fs::is_regular_file(dosbox))throw std::invalid_argument("DOSBox executable is missing");
	const auto game=fs::canonical(installation.root);
	const auto disc1=game/"GAME1.INS",disc2=game/"GAME2.INS";
	if(installation.edition!=GameEdition::WorldOfXeen || !fs::is_regular_file(disc1) || !fs::is_regular_file(disc2) ||
		!fs::is_regular_file(game/"WORLD"/"XEEN.EXE"))
		throw std::invalid_argument("DOSBox requires the CD World of Xeen profile with GAME1.INS and GAME2.INS");
	const auto sourceCfg=cfg?*cfg:game/"WORLD"/"XEEN.CFG";
	if(!fs::is_regular_file(sourceCfg))throw std::invalid_argument("DOSBox XEEN.CFG is missing");
	const auto out=outputPath(requested,installation);
	const auto world=outputPath(out/"pristine"/"WORLD",installation);
	const auto overlay=outputPath(out/"overlay",installation),runs=outputPath(out/"runs",installation);
	const auto inside=[&](const fs::path &child) {
		const auto parent=out.native(), value=child.native();
		return value.size()>parent.size() && value[parent.size()]==L'\\' &&
			CompareStringOrdinal(value.data(),int(parent.size()),parent.data(),int(parent.size()),TRUE)==CSTR_EQUAL;
	};
	if(!inside(world) || !inside(overlay) || !inside(runs))throw std::invalid_argument("DOSBox subfolder resolves outside output folder");
	std::ostringstream number;number<<std::setw(2)<<std::setfill('0')<<slot;
	for(const auto &p:{world/("XEEN"+number.str()+".WOX"),world/("DARK"+number.str()+".WOX"),
		world/"XEEN.CUR",world/"DARK.CUR",out/"dosbox.conf",out/"start.ps1",out/"start.cmd"}) ordinaryTarget(p,overwrite);
	ordinaryTarget(world/"XEEN.CFG",overwrite);
	// Complete all format and placement validation before creating output.
	const auto snapshot=XeenSaveFile::read(fs::absolute(input));
	auto pair=projectDosSave(installation,snapshot,name);
	if(place)placeDosParty(installation,snapshot,pair,*place);
	const auto seed=dosNewGame(installation);
	const auto xeen=writeDosArchive(seed.clouds),dark=writeDosArchive(seed.darkside);
	DosBytes cfgBytes;
	if(cfg) {std::ifstream f(sourceCfg,std::ios::binary);if(!f)throw std::runtime_error("Cannot read selected XEEN.CFG");cfgBytes={std::istreambuf_iterator<char>(f),{}};}
	auto plain=[](fs::path path) {auto s=path.make_preferred().wstring();if(s.rfind(L"\\\\?\\",0)==0)s.erase(0,4);return fs::path(s).u8string();};
	auto psquote=[](std::string s) {std::string r;for(char c:s){r+=c;if(c=='\'')r+='\'';}return r;};
	const auto rootText=plain(out),gameText=plain(game),exeText=plain(fs::canonical(dosbox));
	for(const auto &v:{rootText,gameText,exeText})if(v.find_first_of("\r\n\"")!=std::string::npos)throw std::invalid_argument("DOSBox path cannot contain a quote or newline");
	std::ostringstream conf;
	conf<<"[sdl]\r\nfullscreen = on\r\nfullscreen_mode = forced-borderless\r\noutput = opengl\r\npause_when_inactive = yes\r\n"
		"[render]\r\naspect = auto\r\nviewport = fit\r\ninteger_scaling = off\r\n"
		"[cpu]\r\ncpu_cycles = 80000\r\n"
		"[mouse]\r\nbuiltin_dos_mouse_driver = on\r\nbuiltin_dos_mouse_driver_model = 2button\r\nbuiltin_dos_mouse_driver_move_threshold = 1\r\nbuiltin_dos_mouse_driver_options = immediate\r\nmouse_raw_input = on\r\nmouse_sensitivity = 100\r\n"
		"[autoexec]\r\n@echo off\r\nmount C \""<<gameText<<"\"\r\nmount C \""<<rootText<<"\\overlay\" -t overlay\r\nmount D \""<<plain(disc1)<<"\" \""<<plain(disc2)<<"\" -t iso\r\nC:\r\ncd WORLD\r\nXEEN.EXE\r\nexit\r\n";
	std::ostringstream script;
	script<<"$ErrorActionPreference = 'Stop'\r\n"
		"$root = $PSScriptRoot\r\n"
		"function Check-Tree([string] $p) {\r\n"
		"  if (-not (Test-Path -LiteralPath $p)) { return }\r\n"
		"  $item = Get-Item -LiteralPath $p -Force\r\n"
		"  if ($item.Attributes -band [IO.FileAttributes]::ReparsePoint) { throw \"Reparse point refused: $p\" }\r\n"
		"  if ($item.PSIsContainer) { Get-ChildItem -LiteralPath $p -Force | ForEach-Object { Check-Tree $_.FullName } }\r\n"
		"}\r\n"
		"$p = Get-Item -LiteralPath $root -Force\r\n"
		"while ($p) { if ($p.Attributes -band [IO.FileAttributes]::ReparsePoint) { throw \"Reparse point refused: $($p.FullName)\" }; $p = $p.Parent }\r\n"
		"$pristine = Join-Path $root 'pristine'; $overlay = Join-Path $root 'overlay'; $runs = Join-Path $root 'runs'\r\n"
		"Check-Tree $pristine; Check-Tree $overlay; Check-Tree $runs\r\n"
		"$world = Join-Path $overlay 'WORLD'\r\n"
		"if ((Test-Path -LiteralPath $world) -and @(Get-ChildItem -LiteralPath $world -Force).Count) {\r\n"
		"  New-Item -ItemType Directory -Force -Path $runs | Out-Null\r\n"
		"  $instant = Get-Date\r\n"
		"  do { $backup = Join-Path $runs $instant.ToString('yyyyMMdd-HHmmss-fffffff'); $instant = $instant.AddTicks(1) } while (Test-Path -LiteralPath $backup)\r\n"
		"  New-Item -ItemType Directory -Path $backup | Out-Null\r\n"
		"  Copy-Item -LiteralPath $world -Destination (Join-Path $backup 'WORLD') -Recurse\r\n"
		"}\r\n"
		"if (Test-Path -LiteralPath $overlay) { Remove-Item -LiteralPath $overlay -Recurse -Force }\r\n"
		"Copy-Item -LiteralPath $pristine -Destination $overlay -Recurse\r\n"
		"& '"<<psquote(exeText)<<"' --noprimaryconf --nolocalconf --conf (Join-Path $root 'dosbox.conf')\r\n"
		"exit $LASTEXITCODE\r\n";
	const auto asBytes=[](const std::string &s) {return DosBytes(s.begin(),s.end());};
	const auto cfgText=conf.str(),scriptText=script.str();
	const DosBytes scriptBytes=[&] {DosBytes b{0xef,0xbb,0xbf};b.insert(b.end(),scriptText.begin(),scriptText.end());return b;}();
	fs::create_directories(world);
	if(outputPath(out,installation)!=out || outputPath(world,installation)!=world)throw std::runtime_error("DOSBox output folder changed during preparation");
	struct Output {fs::path target;std::optional<DosBytes> data;bool backed=false,published=false;};
	std::vector<Output> outputs{
		{world/("XEEN"+number.str()+".WOX"),writeDosArchive(pair.clouds)},
		{world/("DARK"+number.str()+".WOX"),writeDosArchive(pair.darkside)},
		{world/"XEEN.CUR",xeen},{world/"DARK.CUR",dark},
		{world/"XEEN.CFG",cfg?std::optional<DosBytes>(cfgBytes):std::nullopt},
		{out/"dosbox.conf",asBytes(cfgText)},{out/"start.ps1",scriptBytes},
		{out/"start.cmd",asBytes("@echo off\r\npowershell.exe -NoProfile -ExecutionPolicy Bypass -File \"%~dp0start.ps1\"\r\nif errorlevel 1 pause\r\n")}};
	static std::atomic<unsigned> sequence{0};fs::path stage;
	for(unsigned attempt=0;attempt<100;++attempt) {
		stage=out/(".dosbox-export-"+std::to_string(GetCurrentProcessId())+"-"+std::to_string(++sequence));
		if(fs::create_directory(stage))break;stage.clear();
	}
	if(stage.empty())throw std::runtime_error("Cannot create DOSBox staging folder");
	const auto staged=[&](unsigned i,const char *suffix){return stage/(std::to_string(i)+suffix);};
	const auto cleanup=[&] {
		for(unsigned attempt=0;attempt<50;++attempt) {
			for(unsigned i=0;i<outputs.size();++i) {DeleteFileW(staged(i,".new").c_str());DeleteFileW(staged(i,".old").c_str());}
			if(RemoveDirectoryW(stage.c_str()) || GetLastError()==ERROR_FILE_NOT_FOUND || GetLastError()==ERROR_PATH_NOT_FOUND)return;
			Sleep(100);
		}
		throw std::runtime_error("Cannot remove DOSBox staging folder: "+stage.u8string());
	};
	try {
		for(unsigned i=0;i<outputs.size();++i)if(outputs[i].data)write(staged(i,".new"),*outputs[i].data);
		if(outputPath(out,installation)!=out || outputPath(world,installation)!=world)throw std::runtime_error("DOSBox output folder changed during preparation");
		for(const auto &v:outputs)ordinaryTarget(v.target,overwrite);
		for(unsigned i=0;i<outputs.size();++i)if(fs::exists(outputs[i].target)) {
			fs::rename(outputs[i].target,staged(i,".old"));outputs[i].backed=true;
		}
		for(unsigned i=0;i<outputs.size();++i)if(outputs[i].data) {
			fs::rename(staged(i,".new"),outputs[i].target);outputs[i].published=true;
		}
	} catch(...) {
		for(unsigned i=0;i<outputs.size();++i) {
			if(outputs[i].published)fs::remove(outputs[i].target);
			if(outputs[i].backed)fs::rename(staged(i,".old"),outputs[i].target);
		}
		cleanup();throw;
	}
	cleanup();
}
}
