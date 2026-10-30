#include "app/XeenPlayerSupport.h"
#include "MMModernVersion.h"
#include "compat/scummvm/XeenMt32.h"
#include "core/DiagnosticNotice.h"
#include "formats/xeen/XeenSoundSetup.h"
#include "formats/xeen/XeenCutsceneData.h"
#include "games/xeen/XeenInstallationDetector.h"
#include "platform/XeenSaveFile.h"
#include "platform/XeenWonStore.h"
#include "platform/sdl/SdlWindowPlacement.h"
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#include <shlobj.h>
#include <algorithm>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <iterator>
#include <mutex>
#include <set>
#include <sstream>
#include <ctime>
namespace mmodern {
namespace {
using Path=std::filesystem::path;
Path environment(const wchar_t *name) {
 const auto size=GetEnvironmentVariableW(name,nullptr,0);if(!size)return {};
 std::wstring value(size,L'\0');GetEnvironmentVariableW(name,value.data(),size);value.resize(size-1);return Path(value).make_preferred();
}
Path localAppData() {
 PWSTR value=nullptr;
 if(FAILED(SHGetKnownFolderPath(FOLDERID_LocalAppData,0,nullptr,&value)))throw std::runtime_error("Cannot resolve Local AppData");
 Path path(value);CoTaskMemFree(value);return path;
}
std::vector<std::uint8_t> readCfg(const Path &path) {
 std::ifstream input(path,std::ios::binary);
 if(!input)throw std::runtime_error("Cannot read XEEN.CFG: "+path.u8string());
 std::vector<std::uint8_t> bytes((std::istreambuf_iterator<char>(input)),{});
 if(input.bad())throw std::runtime_error("Cannot read XEEN.CFG: "+path.u8string());
 try{xeenValidateSetupBase(bytes);}catch(const std::exception &e){throw std::runtime_error(path.u8string()+": "+e.what());}
 return bytes;
}
std::string hex(unsigned value) {std::ostringstream s;s<<"0x"<<std::hex<<value;return s.str();}
std::string windowsVersion() {
 // RtlGetVersion is independent of the executable's compatibility manifest.
 using Version=LONG (WINAPI *)(OSVERSIONINFOW *);
 const auto function=reinterpret_cast<Version>(GetProcAddress(GetModuleHandleW(L"ntdll.dll"),"RtlGetVersion"));
 OSVERSIONINFOW v{};v.dwOSVersionInfoSize=sizeof(v);
 if(!function || function(&v))return "unknown";
 return std::to_string(v.dwMajorVersion)+"."+std::to_string(v.dwMinorVersion)+" (build "+std::to_string(v.dwBuildNumber)+")";
}
void replacePath(std::string &text,const Path &path,const char *replacement) {
 if(path.empty())return;
 const auto count=MultiByteToWideChar(CP_UTF8,0,text.data(),static_cast<int>(text.size()),nullptr,0);
 std::wstring wide(count,L'\0');
 MultiByteToWideChar(CP_UTF8,0,text.data(),static_cast<int>(text.size()),wide.data(),count);
 auto needle=path.wstring(),normalized=wide;
 std::replace(needle.begin(),needle.end(),L'/',L'\\');
 std::replace(normalized.begin(),normalized.end(),L'/',L'\\');
 std::wstring token;for(const char *p=replacement;*p;++p)token+=wchar_t(*p);
 for(std::size_t pos=0;pos+needle.size()<=normalized.size();) {
  if(CompareStringOrdinal(normalized.data()+pos,static_cast<int>(needle.size()),needle.data(),static_cast<int>(needle.size()),TRUE)!=CSTR_EQUAL) {++pos;continue;}
  const auto end=pos+needle.size();
  if(end<normalized.size() && normalized[end]!=L'\\' && normalized[end]!=L'\n' && normalized[end]!=L'\r' && normalized[end]!=L'"' && normalized[end]!=L'\'' && normalized[end]!=L';' && normalized[end]!=L' ') {pos=end;continue;}
  wide.replace(pos,needle.size(),token);normalized.replace(pos,needle.size(),token);pos+=token.size();
  // Use Windows separators in abbreviated profile paths, even for UTF-8 argv.
  for(auto i=pos;i<wide.size() && wide[i]!=L'\n' && wide[i]!=L'\r' && wide[i]!=L'"';++i)if(wide[i]==L'/')wide[i]=L'\\';
 }
 const auto size=WideCharToMultiByte(CP_UTF8,0,wide.data(),static_cast<int>(wide.size()),nullptr,0,nullptr,nullptr);
 text.resize(size);WideCharToMultiByte(CP_UTF8,0,wide.data(),static_cast<int>(wide.size()),text.data(),size,nullptr,nullptr);
}
}
Path xeenPlayerDirectory() {
 auto path=environment(L"MMODERN_PLAYER_DIR");
 if(path.empty())return localAppData()/"MMModern";
 if(!path.is_absolute())throw std::runtime_error("MMODERN_PLAYER_DIR must be absolute");
 return path;
}
Path xeenLogFile() {
 auto path=environment(L"MMODERN_LOG_FILE");
 return path.empty()?xeenPlayerDirectory()/"logs"/"mmodern.log":path;
}
std::string xeenPrivatePaths(std::string text,const Path &local,const Path &profile) {
 replacePath(text,local,"%LOCALAPPDATA%");replacePath(text,profile,"%USERPROFILE%");return text;
}
std::string xeenPrivatePaths(std::string text) {
 return xeenPrivatePaths(std::move(text),localAppData(),environment(L"USERPROFILE"));
}

std::optional<Path> xeenPlayerCfg(const std::optional<Path> &explicitCfg) {
 if(explicitCfg)return explicitCfg;
 const auto path=xeenPlayerDirectory()/"XEEN.CFG";
 // exists() follows links; symlink_status also catches a broken owned link.
 if(std::filesystem::symlink_status(path).type()==std::filesystem::file_type::not_found)return {};
 readCfg(path);return path;
}
void xeenWriteOwnedCfg(const Path &path,const std::vector<std::uint8_t> &bytes) {
 xeenValidateSetupBase(bytes);std::filesystem::create_directories(path.parent_path());
 // Exclusive sibling creation avoids clobbering another setup process's temp.
 Path temporary;HANDLE file=INVALID_HANDLE_VALUE;
 for(unsigned n=0;n<100 && file==INVALID_HANDLE_VALUE;++n) {
  temporary=path;temporary+=L".tmp-"+std::to_wstring(GetCurrentProcessId())+L"-"+std::to_wstring(n);
  file=CreateFileW(temporary.c_str(),GENERIC_WRITE,0,nullptr,CREATE_NEW,FILE_ATTRIBUTE_NORMAL,nullptr);
  if(file==INVALID_HANDLE_VALUE && GetLastError()!=ERROR_FILE_EXISTS)break;
 }
 if(file==INVALID_HANDLE_VALUE)throw std::runtime_error("Cannot create temporary CFG beside "+path.u8string());
 DWORD written=0;bool ok=WriteFile(file,bytes.data(),static_cast<DWORD>(bytes.size()),&written,nullptr) && written==bytes.size();
 ok=FlushFileBuffers(file) && ok;ok=CloseHandle(file) && ok;
 if(ok)ok=MoveFileExW(temporary.c_str(),path.c_str(),MOVEFILE_REPLACE_EXISTING|MOVEFILE_WRITE_THROUGH)!=0;
 if(!ok) {DeleteFileW(temporary.c_str());throw std::runtime_error("Cannot replace owned CFG: "+path.u8string());}
}
void xeenResetOwnedCfg(const Path &path) {
 if(!DeleteFileW(path.c_str()) && GetLastError()!=ERROR_FILE_NOT_FOUND && GetLastError()!=ERROR_PATH_NOT_FOUND)
  throw std::runtime_error("Cannot reset owned CFG: "+path.u8string());
}
std::string xeenDiagnostics(const Path &root,const std::optional<Path> &explicitCfg,const Path &ui) {
 std::ostringstream out;
 const auto line=[&](const char *key,const auto &value){out<<key<<": "<<value<<'\n';};
 line("version",std::string("MMModern ")+MMODERN_VERSION);line("windows",windowsVersion());line("game.folder",root.u8string());
 try {
  // Detection of the data is independent of whether the owned CFG is damaged.
  auto selected=explicitCfg;std::string source=selected?"explicit":"installed";
  try {selected=xeenPlayerCfg(explicitCfg);if(selected && !explicitCfg)source="owned";}
  catch(const std::exception &e){source="owned";line("cfg.error",e.what());selected=xeenPlayerDirectory()/"XEEN.CFG";}
  line("cfg.source",source);if(selected)line("cfg.file",selected->u8string());
  auto installation=XeenInstallationDetector(ui).detect(root);
  if(!installation)throw std::runtime_error("No usable World of Xeen data found");
  line("game.status","usable");line("game.edition","World of Xeen (CD)");
  line("game.archives",installation->sourceOrigin);
  const char *names[]{"archive.clouds","archive.darkside","archive.intro"};
  unsigned index=0;for(const auto role:{XeenArchiveRole::Clouds,XeenArchiveRole::Darkside,XeenArchiveRole::Intro}) {
   const auto f=archiveDataFile(*installation,role);line(names[index++],f.physicalPath.u8string()+" ("+std::to_string(f.logicalSize)+" bytes)");
  }
  if(!selected)line("cfg.file",installation->configuration->physicalPath.u8string());
  try {
   const auto path=selected?*selected:installation->configuration->physicalPath;
   const auto bytes=readCfg(path);const auto cfg=XeenAudioConfig::parse(bytes);
   line("sound.speech",xeenSoundCaption(cfg.digital,false)+"; "+xeenSoundStatus(cfg,false));
   line("sound.music",xeenSoundCaption(cfg.music,true)+"; "+xeenSoundStatus(cfg,true));
   line("sound.digital-port",hex(cfg.digitalPort));line("sound.music-port",hex(cfg.musicPort));
   line("sound.irq",cfg.irq);line("sound.dma",cfg.dma);line("sound.automatic-volume",cfg.mixer?"yes":"no");
  }catch(const std::exception &e){line("sound.error",e.what());}
  try{XeenCutsceneData::installed(*installation);line("game.exe","usable for intros and Clouds ending");}
  catch(const std::exception &e){line("game.exe",std::string("unusable: ")+e.what());}
  const auto id=XeenSaveFile::dataId(XeenSaveFile::fingerprint(*installation));line("game.data-id",id);
  const auto saves=localAppData()/"MMModern"/"Saves"/id;line("saves.folder",saves.u8string());
  unsigned used=0;for(unsigned n=0;n<10;++n)if(std::filesystem::exists(XeenSaveFile::slotPath(saves,n)))++used;
  line("saves.used-slots",used);
 }catch(const std::exception &e){line("game.status",std::string("unusable: ")+e.what());}
 try {
  const auto roms=XeenMt32::identify(environment(L"MMODERN_MT32_ROM_DIR"));
  line("roms.folder",roms.directory.u8string());line("roms.control",roms.control);line("roms.pcm",roms.pcm);
 }catch(const std::exception &e){line("roms.error",e.what());}
 line("settings.file",xeenSettingsFile().value_or(Path{}).u8string());
 line("window.file",windowPlacementFile().value_or(Path{}).u8string());line("log.file",xeenLogFile().u8string());
 return xeenPrivatePaths(out.str());
}
int xeenSoundSetupCommand(const std::vector<std::string> &args) {
 if(args.size()<3)throw std::runtime_error("Usage: mmodern --sound-setup <game-dir> [--reset | --speech <key> --music <key> [--digital-port <n>] [--music-port <n>] [--irq <n>] [--dma <n>] [--volume 0|1]]");
 const auto owned=xeenPlayerDirectory()/"XEEN.CFG";
 if(args.size()==4 && args[3]=="--reset") {xeenResetOwnedCfg(owned);std::cout<<"cfg.source: installed\n";return 0;}
 const auto selected=xeenPlayerCfg();auto installation=XeenInstallationDetector({},selected.value_or(Path{})).detect(std::filesystem::u8path(args[2]));
 if(!installation)throw std::runtime_error("No usable World of Xeen data found");
 const auto bytes=readCfg(installation->configuration->physicalPath);const auto cfg=XeenAudioConfig::parse(bytes);
 if(args.size()>3) {
  XeenSoundAnswers a;bool speech=false,music=false;std::set<std::string> seen;
  for(std::size_t n=3;n<args.size();n+=2) {
   if(n+1==args.size() || !seen.insert(args[n]).second)throw std::runtime_error("Missing or duplicate sound answer");
   const auto &s=args[n+1];std::size_t consumed=0;
   const auto value=std::stoul(s,&consumed,0);if(consumed!=s.size() || value>65535 || s[0]=='-')throw std::runtime_error("Invalid sound answer");
   const auto key=[&] {if(s.size()!=1 || s[0]<'0' || s[0]>'9')throw std::runtime_error("Invalid installer key");return s[0]=='0'?9u:unsigned(s[0]-'1');};
   if(args[n]=="--speech"){a.speech=key();speech=true;}else if(args[n]=="--music"){a.music=key();music=true;}
   else if(args[n]=="--digital-port")a.digitalPort=value;else if(args[n]=="--music-port")a.musicPort=value;
   else if(args[n]=="--irq")a.irq=value;else if(args[n]=="--dma")a.dma=value;else if(args[n]=="--volume")a.volume=value;
   else throw std::runtime_error("Unknown sound answer: "+args[n]);
  }
  if(!speech || !music)throw std::runtime_error("Both --speech and --music are required");
  xeenWriteOwnedCfg(owned,xeenApplySoundSetup(bytes,a));std::cout<<"cfg.source: owned\n";return 0;
 }
 std::cout<<"cfg.source: "<<(selected?"owned":"installed")<<'\n';
 const auto menu=[&](const char *name,const auto &rows,bool music) {
  for(unsigned n=0;n<rows.size();++n) {
   const auto key=n==9?0:n+1;const auto prefix=std::string(name)+"."+std::to_string(key);const auto &r=rows[n];
   auto c=cfg;if(music)c.music=r.id;else c.digital=r.id;
   std::cout<<prefix<<".caption: "<<r.caption<<"; "<<xeenSoundStatus(c,music)<<'\n';
   std::cout<<prefix<<".id: "<<r.id<<'\n';
   const auto values=[&](const char *q,const auto &v,bool port=false){if(v.empty())return;std::cout<<prefix<<'.'<<q<<": ";bool first=true;for(auto x:v){if(!first)std::cout<<',';first=false;std::cout<<(port?hex(x):std::to_string(x));}std::cout<<'\n';};
   values(music?"music-port":"digital-port",r.ports,true);values("irq",r.irqs);values("dma",r.dmas);
   if(r.volume)std::cout<<prefix<<".volume: 0,1\n";
  }
 };
 menu("speech",xeenSpeechMenu(),false);menu("music",xeenMusicMenu(),true);
 const auto current=[&](const char *name,const auto &rows,unsigned id){for(unsigned n=0;n<rows.size();++n)if(rows[n].id==id){std::cout<<"current."<<name<<": "<<(n==9?0:n+1)<<'\n';return;}};
 current("speech",xeenSpeechMenu(),cfg.digital);current("music",xeenMusicMenu(),cfg.music);
 std::cout<<"current.digital-port: "<<hex(cfg.digitalPort)<<"\ncurrent.music-port: "<<hex(cfg.musicPort)<<"\ncurrent.irq: "<<cfg.irq<<"\ncurrent.dma: "<<cfg.dma<<"\ncurrent.volume: "<<(cfg.mixer?1:0)<<'\n';
 return 0;
}
namespace {
class Tee final:public std::streambuf {
 std::streambuf *terminal;std::ofstream &file;std::string pending;std::mutex &mutex;
 const Path &local,&profile;
 void flushLine() noexcept {try {if(file) {file<<xeenPrivatePaths(pending,local,profile);file.flush();}pending.clear();}catch(...) {pending.clear();}}
 std::streamsize xsputn(const char *text,std::streamsize size) override {
  std::lock_guard<std::mutex> lock(mutex);
  // Keep bulk diagnostics bulk at the console too. Character-at-a-time writes
  // to the launcher's hidden console stall New/Load and gameplay inspection.
  terminal->sputn(text,size);
  const auto *end=text+size;
  while(text!=end) {
   const auto *newline=std::find(text,end,'\n');
   const auto *next=newline==end?end:newline+1;
   pending.append(text,static_cast<std::size_t>(next-text));
   if(newline!=end)flushLine();
   text=next;
  }
  return size;
 }
 int overflow(int c) override {
  if(c==traits_type::eof())return traits_type::not_eof(c);
  const char value=char(c);xsputn(&value,1);return c;
 }
 int sync() override {std::lock_guard<std::mutex> lock(mutex);terminal->pubsync();return 0;}
public:
 Tee(std::streambuf *t,std::ofstream &f,std::mutex &m,const Path &l,const Path &p):terminal(t),file(f),mutex(m),local(l),profile(p){}
 ~Tee(){flushLine();}
};
void noticeToLog(const std::string &text) {
 // Animation may render the same notice every frame; log each distinct notice once.
 static std::set<std::string> shown;
 if(shown.insert(text).second)std::cerr<<"Notice: "<<text<<'\n';
}
}
struct XeenPlayerLog::Impl {
 Path local,profile;
 std::ofstream file;std::mutex mutex;std::streambuf *out=std::cout.rdbuf(),*err=std::cerr.rdbuf();
 std::unique_ptr<Tee> output,error;
 Impl() {
  try {
   const auto path=xeenLogFile();std::filesystem::create_directories(path.parent_path());
   auto previous=path.parent_path()/(path.stem().wstring()+L"-previous"+path.extension().wstring());
   if(std::filesystem::exists(path)) {
    if(!std::filesystem::is_regular_file(path))return;
    if(!MoveFileExW(path.c_str(),previous.c_str(),MOVEFILE_REPLACE_EXISTING))return;
   }
   file.open(path,std::ios::binary|std::ios::trunc);if(!file)return;
   local=localAppData();profile=environment(L"USERPROFILE");
   output=std::make_unique<Tee>(out,file,mutex,local,profile);error=std::make_unique<Tee>(err,file,mutex,local,profile);
   std::cout.rdbuf(output.get());std::cerr.rdbuf(error.get());diagnosticNoticeSink=noticeToLog;
   const auto now=std::time(nullptr);std::tm date{};localtime_s(&date,&now);
   std::cout<<"date: "<<std::put_time(&date,"%Y-%m-%d %H:%M:%S")<<'\n';
  }catch(...) {}
 }
 ~Impl(){diagnosticNoticeSink=nullptr;std::cout.rdbuf(out);std::cerr.rdbuf(err);}
};
XeenPlayerLog::XeenPlayerLog():_impl(new Impl){}
XeenPlayerLog::~XeenPlayerLog()=default;
}
