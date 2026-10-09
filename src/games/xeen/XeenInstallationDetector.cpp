#include "games/xeen/XeenInstallationDetector.h"
#include <algorithm>
#include <array>
#include <cctype>
#include <cstring>
#include <fstream>
#include <limits>
#include <regex>
#include <set>
#include <stdexcept>
#include <string>
#include <vector>
namespace mmodern {
namespace {
using Path=std::filesystem::path;
std::string lower(std::string value) {
 for(auto &c:value) if(c>='A'&&c<='Z') c+=32;
 return value;
}
Path child(const Path &directory,const std::string &name,bool directoryWanted=false) {
 Path result;
 if(!std::filesystem::is_directory(directory)) return result;
 for(const auto &entry:std::filesystem::directory_iterator(directory)) {
  if(lower(entry.path().filename().u8string())!=lower(name)) continue;
  if(!result.empty()) throw std::runtime_error("Ambiguous case-insensitive source name: "+directory.u8string()+"/"+name);
  if(directoryWanted?!entry.is_directory():!entry.is_regular_file()) throw std::runtime_error("Source entry has wrong file type: "+entry.path().u8string());
  result=entry.path();
 }
 return result;
}
Path relativeImage(const Path &cue,const std::string &name) {
 const auto relative=std::filesystem::u8path(name);
 if(relative.is_absolute()) throw std::runtime_error("Cue image must be relative to its cue");
 auto resolved=cue.parent_path();
 for(auto part=relative.begin();part!=relative.end();++part) {
  const auto &component=*part;
  if(component==".") continue;
  if(component=="..") {resolved=resolved.parent_path();continue;}
  const auto found=child(resolved,component.u8string(),std::next(part)!=relative.end());
  if(found.empty()) throw std::runtime_error("Missing cue data image: "+name);
  resolved=found;
 }
 return resolved;
}
std::uint16_t both16(const std::uint8_t *p) {
 const auto le=std::uint16_t(p[0]|unsigned(p[1])<<8),be=std::uint16_t(unsigned(p[2])<<8|p[3]);
 if(le!=be) throw std::runtime_error("ISO9660 both-endian 16-bit mismatch");
 return le;
}
std::uint32_t both32(const std::uint8_t *p) {
 const auto le=std::uint32_t(p[0])|std::uint32_t(p[1])<<8|std::uint32_t(p[2])<<16|std::uint32_t(p[3])<<24;
 const auto be=std::uint32_t(p[4])<<24|std::uint32_t(p[5])<<16|std::uint32_t(p[6])<<8|p[7];
 if(le!=be) throw std::runtime_error("ISO9660 both-endian 32-bit mismatch");
 return le;
}
struct IsoEntry { std::string name; std::uint32_t block=0,size=0; bool directory=false; };
IsoEntry record(const std::uint8_t *p,std::size_t available,std::uint64_t volume) {
 if(available<34 || p[0]<34 || p[0]>available || 33u+p[32]+(p[32]%2==0?1:0)>p[0] || p[32]==0)
  throw std::runtime_error("Malformed ISO9660 directory record");
 if(p[1]!=0 || p[26]!=0 || p[27]!=0 || (p[25]&0x80)) throw std::runtime_error("Unsupported ISO9660 extended/interleaved/multi-extent record");
 if((p[25]&~3u)!=0 || both16(p+28)!=1) throw std::runtime_error("Unsupported ISO9660 directory flags or volume sequence");
 if(p[32]%2==0 && p[33+p[32]]!=0) throw std::runtime_error("Invalid ISO9660 filename padding");
 IsoEntry result; result.block=both32(p+2); result.size=both32(p+10); result.directory=(p[25]&2)!=0;
 if(std::uint64_t(result.block)*2048>volume || result.size>volume-std::uint64_t(result.block)*2048) throw std::runtime_error("ISO9660 extent outside volume");
 result.name=lower(std::string(reinterpret_cast<const char *>(p+33),p[32]));
 if(result.name.size()==1 && (result.name[0]==0||result.name[0]==1)) return result;
 const auto semicolon=result.name.find(';');
 if(semicolon!=std::string::npos) {
  if(result.directory || result.name.substr(semicolon)!=";1") throw std::runtime_error("Unsupported ISO9660 file version");
  result.name.resize(semicolon);
 }
 if(result.name.find('/')!=std::string::npos || result.name.find('\\')!=std::string::npos) throw std::runtime_error("Malformed ISO9660 filename");
 return result;
}
std::vector<IsoEntry> directory(ReadOnlyDataStream &stream,const IsoEntry &entry,std::uint64_t volume) {
 if(!entry.directory || !entry.size || entry.size>16*1024*1024) throw std::runtime_error("Invalid or oversized ISO9660 directory");
 std::vector<std::uint8_t> bytes(entry.size); stream.seek(std::uint64_t(entry.block)*2048); stream.read(bytes.data(),bytes.size());
 std::vector<IsoEntry> result;
 for(std::size_t offset=0;offset<bytes.size();) {
  const auto available=std::min<std::size_t>(2048-offset%2048,bytes.size()-offset);
  if(!bytes[offset]) {
   if(!std::all_of(bytes.begin()+offset,bytes.begin()+offset+available,[](auto b){return b==0;})) throw std::runtime_error("Nonzero ISO9660 directory padding");
   offset+=available; continue;
  }
  auto item=record(bytes.data()+offset,available,volume);
  for(const auto &known:result) if(item.name==known.name) throw std::runtime_error("Duplicate ISO9660 directory name: "+item.name);
  result.push_back(std::move(item)); offset+=bytes[offset];
 }
 return result;
}
IsoEntry require(const std::vector<IsoEntry> &entries,const char *name,bool directoryWanted) {
 const auto found=std::find_if(entries.begin(),entries.end(),[&](const auto &entry){return entry.name==name;});
 if(found==entries.end()) throw std::runtime_error(std::string("Missing ISO9660 source entry: ")+name);
 if(found->directory!=directoryWanted) throw std::runtime_error(std::string("Wrong ISO9660 entry type: ")+name);
 return *found;
}
std::array<ReadOnlyDataFile,3> iso(const ReadOnlyDataFile &track) {
 auto stream=track.open(); std::array<std::uint8_t,2048> bytes{}; bool primary=false,ended=false; IsoEntry root; std::uint64_t volume=0;
 for(unsigned block=16;block<272;++block) {
  if(!stream->seek(std::uint64_t(block)*2048)) throw std::runtime_error("Missing ISO9660 volume descriptor");
  stream->read(bytes.data(),bytes.size());
  if(std::memcmp(bytes.data()+1,"CD001",5)||bytes[6]!=1) throw std::runtime_error("Invalid ISO9660 volume descriptor");
  if(bytes[0]==255) { ended=true; break; }
  if(bytes[0]!=0&&bytes[0]!=1&&bytes[0]!=2&&bytes[0]!=3) throw std::runtime_error("Unsupported ISO9660 volume descriptor type");
  if(bytes[0]!=1) continue;
  if(primary) throw std::runtime_error("Ambiguous ISO9660 primary volumes");
  primary=true;
  if(both16(bytes.data()+128)!=2048 || both16(bytes.data()+120)!=1 || both16(bytes.data()+124)!=1 || bytes[881]!=1)
   throw std::runtime_error("Unsupported ISO9660 block size or volume layout");
  volume=std::uint64_t(both32(bytes.data()+80))*2048;
  if(!volume || volume>track.logicalSize) throw std::runtime_error("ISO9660 volume exceeds data track");
  root=record(bytes.data()+156,34,volume);
 }
 if(!primary||!ended) throw std::runtime_error("Incomplete ISO9660 volume descriptors");
 const auto game=require(directory(*stream,root,volume),"game",true);
 const auto entries=directory(*stream,game,volume);
 std::array<ReadOnlyDataFile,3> result; const char *names[]{"xeen.cc","dark.cc","intro.cc"};
 for(unsigned i=0;i<3;++i) {
  const auto entry=require(entries,names[i],false); result[i]=track;
  result[i].logicalOffset=std::uint64_t(entry.block)*2048; result[i].logicalSize=entry.size;
 }
 return result;
}
std::array<ReadOnlyDataFile,3> cueSource(const Path &cue) {
 if(std::filesystem::file_size(cue)>1024*1024) throw std::runtime_error("Oversized cue metadata");
 std::ifstream input(cue); if(!input) throw std::runtime_error("Cannot read cue metadata");
 std::regex filePattern(R"cue(^\s*FILE\s+"([^"]+)"\s+(\S+)\s*$)cue",std::regex::icase);
 std::regex trackPattern(R"(^\s*TRACK\s+([0-9]+)\s*(MODE[0-9]+/[0-9]+|AUDIO)\s*$)",std::regex::icase);
 std::regex indexPattern(R"(^\s*INDEX\s+([0-9]+)\s+([0-9]+):([0-9]+):([0-9]+)\s*$)",std::regex::icase);
 std::string line,file,type,mode,dataImage; std::smatch match; bool dataIndex=false,found=false; std::uint64_t base=0,trackEnd=0; unsigned lastTrack=0;
 while(std::getline(input,line)) {
  if(!line.empty()&&line.back()=='\r')line.pop_back();
  if(std::regex_match(line,match,filePattern)) {
   if(mode=="mode1/2352"&&!dataIndex)throw std::runtime_error("Data track has no INDEX 01 before next FILE");
   file=match[1]; type=lower(match[2]); mode.clear();
  }
  else if(std::regex_match(line,match,trackPattern)) {
   if(file.empty()) throw std::runtime_error("Cue TRACK precedes FILE");
   const unsigned number=std::stoul(match[1]); if(!number||number<=lastTrack) throw std::runtime_error("Invalid cue track order"); lastTrack=number;
   if(mode=="mode1/2352"&&!dataIndex) throw std::runtime_error("Data track has no INDEX 01");
   mode=lower(match[2]); dataIndex=false;
   if(mode!="audio"&&mode!="mode1/2352") throw std::runtime_error("Unsupported cue data track format: "+mode);
   if(mode=="mode1/2352"&&(found||type!="binary")) throw std::runtime_error("Ambiguous or non-binary cue data track");
  } else if(std::regex_match(line,match,indexPattern)) {
   if(mode.empty()) throw std::runtime_error("Cue INDEX precedes TRACK");
   const auto index=std::stoul(match[1]); const auto minutes=std::stoull(match[2]),seconds=std::stoull(match[3]),frames=std::stoull(match[4]);
   if(seconds>=60||frames>=75||minutes>(std::numeric_limits<std::uint64_t>::max()-seconds*75-frames)/4500) throw std::runtime_error("Invalid cue index time");
   const auto sectors=minutes*4500+seconds*75+frames;
   if(sectors>std::numeric_limits<std::uint64_t>::max()/2352) throw std::runtime_error("Cue offset overflow");
   if(index==1&&mode=="mode1/2352") {
    if(dataIndex) throw std::runtime_error("Duplicate data INDEX 01");
    dataIndex=true; found=true; dataImage=file; base=sectors*2352;
   } else if(found&&file==dataImage&&!trackEnd&&mode=="audio"&&(index==0||index==1)) trackEnd=sectors*2352;
  } else {
   const auto normalized=lower(line); const auto begin=normalized.find_first_not_of(" \t");
   if(begin==std::string::npos||normalized.compare(begin,3,"rem")==0)continue;
   throw std::runtime_error("Unsupported or malformed cue metadata: "+line);
  }
 }
 if(!input.eof() || !found || (mode=="mode1/2352"&&!dataIndex)) throw std::runtime_error("Incomplete cue data track");
 const auto image=relativeImage(cue,dataImage); const auto length=std::filesystem::file_size(image);
 if(!trackEnd)trackEnd=length;
 if(base>=trackEnd||trackEnd>length||(trackEnd-base)%2352)throw std::runtime_error("Invalid raw data track extent");
 ReadOnlyDataFile track;track.physicalPath=std::filesystem::weakly_canonical(image);track.transport=ReadOnlyDataFile::Transport::Mode1Raw;
 track.trackBase=base;track.logicalSize=(trackEnd-base)/2352*2048;track.physicalSize=length;
 return iso(track);
}
bool same(const ReadOnlyDataFile &left,const ReadOnlyDataFile &right) {
 if(left.logicalSize!=right.logicalSize)return false;
 auto a=left.open(),b=right.open(); std::array<std::uint8_t,65536> ab{},bb{};
 while(a->pos()<a->size()) { const auto n=a->read(ab.data(),ab.size()); if(b->read(bb.data(),n)!=n||!std::equal(ab.begin(),ab.begin()+n,bb.begin()))return false; }
 return true;
}
}
std::array<ReadOnlyDataFile,3> readCueCdArchives(const Path &cue) {return cueSource(cue);}
std::optional<GameInstallation> XeenInstallationDetector::detect(const Path &root) const {
 if(!std::filesystem::is_directory(root))return std::nullopt;
 struct Candidate {std::string origin;std::array<ReadOnlyDataFile,3> files;}; std::vector<Candidate> candidates;
 std::vector<Path> cues;
 std::set<std::string> cueNames;
 for(const auto &entry:std::filesystem::directory_iterator(root)) if(entry.is_regular_file()&&lower(entry.path().extension().u8string())==".ins") {
  if(!cueNames.insert(lower(entry.path().filename().u8string())).second)throw std::runtime_error("Ambiguous case-insensitive cue name");
  cues.push_back(entry.path());
 }
 std::sort(cues.begin(),cues.end());
 for(const auto &cue:cues)candidates.push_back({"cue "+std::filesystem::weakly_canonical(cue).u8string(),cueSource(cue)});
 // Cue-backed installs never consider co-located floppy archives.
 if(cues.empty()) {
  // A GAME folder is an explicit CD-copy source. Loose root CC files cannot
  // become a floppy fallback when a GOG installation's cues are missing.
  std::vector<Path> folders;
  if(lower(root.filename().u8string())=="game")folders.push_back(root);
  const auto game=child(root,"game",true);if(!game.empty())folders.push_back(game);
  for(const auto &folder:folders) {
   const auto xeen=child(folder,"xeen.cc"),dark=child(folder,"dark.cc"),intro=child(folder,"intro.cc");
   if(xeen.empty()&&dark.empty()&&intro.empty())continue;
   if(xeen.empty()||dark.empty()||intro.empty()) {continue;}
   candidates.push_back({"plain GAME "+std::filesystem::weakly_canonical(folder).u8string(),{ReadOnlyDataFile::plain(xeen),ReadOnlyDataFile::plain(dark),ReadOnlyDataFile::plain(intro)}});
  }
 }
 if(candidates.empty()) {
  throw std::runtime_error("No complete CD source (XEEN.CC, DARK.CC and INTRO.CC required; sources are never mixed)");
 }
 std::sort(candidates.begin(),candidates.end(),[](const auto &a,const auto &b){return a.origin<b.origin;});
 for(std::size_t i=1;i<candidates.size();++i)for(unsigned role=0;role<3;++role)if(!same(candidates[0].files[role],candidates[i].files[role]))
  throw std::runtime_error("Conflicting CD source candidates: "+candidates[0].origin+" and "+candidates[i].origin);
 Path ui=_uiData;
 if(ui.empty()) {
  std::vector<Path> modules;
  for(const auto &parent:std::vector<Path>{root,root.parent_path()}) {
   const auto world=child(parent,"world",true);if(world.empty())continue; const auto dat=child(world,"xeen.dat");
   if(!dat.empty()&&std::find(modules.begin(),modules.end(),dat)==modules.end())modules.push_back(dat);
  }
  if(modules.size()>1)throw std::runtime_error("Ambiguous WORLD/XEEN.DAT; provide --ui-data");
  if(!modules.empty())ui=modules[0];
 }
 if(ui.empty()||!std::filesystem::is_regular_file(ui))throw std::runtime_error("CD UI module missing: supply installed uncompressed English WORLD/XEEN.DAT with --ui-data");
 GameInstallation result;result.root=std::filesystem::weakly_canonical(root);result.edition=GameEdition::WorldOfXeen;
 result.cloudsData=candidates[0].files[0];result.darksideData=candidates[0].files[1];result.introData=candidates[0].files[2];
 result.xeenArchive=result.cloudsData->physicalPath;result.darkArchive=result.darksideData->physicalPath;
 result.uiModule=ReadOnlyDataFile::plain(ui);result.sourceOrigin=candidates[0].origin+"; UI "+result.uiModule->physicalPath.u8string();
 result.protectedDirectories.push_back(result.root);
 for(const auto &candidate:candidates)for(const auto &file:candidate.files)result.protectedDirectories.push_back(file.physicalPath.parent_path());
 result.protectedDirectories.push_back(result.uiModule->physicalPath.parent_path());return result;
}
}
