#include "XeenSaveTestSupport.h"
#include "platform/XeenSaveFile.h"
#include "platform/XeenUserFolders.h"
#include "platform/XeenWonStore.h"
#include "platform/XeenFileOperations.h"
#include "platform/sdl/SdlWindowPlacement.h"
#include "app/XeenPlayerSupport.h"
#include "compat/scummvm/XeenMt32.h"
#include "games/xeen/XeenInstallationDetector.h"
#include "core/PathText.h"
#include "TestTreeRemoval.h"
#include <fstream>
#include <iostream>
#include <iterator>
#include <cstdlib>
#ifdef _WIN32
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#include <shlobj.h>
#include <cstring>
#else
#include "platform/XeenSaveIo.h"
#include <sys/stat.h>
#include <unistd.h>
#include <cerrno>
#endif
#ifdef _WIN32
namespace {std::optional<std::filesystem::path> appDataOverride;HRESULT appDataResult=S_OK;}
extern "C" HRESULT __real_SHGetKnownFolderPath(REFKNOWNFOLDERID,DWORD,HANDLE,PWSTR *);
extern "C" HRESULT __wrap_SHGetKnownFolderPath(REFKNOWNFOLDERID folder,DWORD flags,HANDLE token,PWSTR *out) {
 if(FAILED(appDataResult)){*out=nullptr;return appDataResult;}
 if(!appDataOverride)return __real_SHGetKnownFolderPath(folder,flags,token,out);
 const auto value=appDataOverride->wstring();
 *out=static_cast<PWSTR>(CoTaskMemAlloc((value.size()+1)*sizeof(wchar_t)));
 if(!*out)return E_OUTOFMEMORY;
 std::memcpy(*out,value.c_str(),(value.size()+1)*sizeof(wchar_t));return S_OK;
}
#endif
#ifndef _WIN32
namespace {bool interruptSync=false;unsigned syncCalls=0;}
extern "C" int __real_fsync(int);
extern "C" int __wrap_fsync(int fd) {
 ++syncCalls;
 if(interruptSync){interruptSync=false;errno=EINTR;return -1;}
 return __real_fsync(fd);
}
#endif
using namespace mmodern;
using namespace save_test;
namespace fs=std::filesystem;
namespace {
struct Environment {
 std::string key;fs::path previous;
 explicit Environment(const char *name,const fs::path &value):key(name),previous(xeenEnvironment(name)){set(value);}
 void set(const fs::path &value) {
#ifdef _WIN32
  std::wstring name(key.begin(),key.end());SetEnvironmentVariableW(name.c_str(),value.empty()?nullptr:value.c_str());
#else
  if(value.empty())::unsetenv(key.c_str());else ::setenv(key.c_str(),value.c_str(),1);
#endif
 }
 ~Environment(){set(previous);}
};
Bytes raw(const fs::path &path){std::ifstream input(path,std::ios::binary);return {std::istreambuf_iterator<char>(input),{}};}
void noTemporary(const fs::path &path) {
 for(const auto &entry:fs::directory_iterator(path))check(entry.path().filename().u8string().find(".tmp-")==std::string::npos,"temporary leaked");
}
}
int main(int argc,char **argv) {try {
 check(argc==2,"platform tests <scratch>");
 const auto scratch=fs::absolute(fs::u8path(argv[1]));check(removeTestTree(scratch),"cannot clear platform scratch");fs::create_directories(scratch);
#ifndef _WIN32
 syncCalls=0;interruptSync=true;xeenSyncDirectory(scratch);
 check(syncCalls==2 && !interruptSync,"directory fsync did not retry EINTR");
 Environment home("HOME",scratch/"home"),xdg("XDG_DATA_HOME",scratch/"xdg");
 check(xeenUserFolder()==scratch/"xdg/MMModern","absolute XDG base");
 check(xeenPrivatePaths(xeenUserFolder().native()+"/settings.ini")=="$XDG_DATA_HOME/MMModern/settings.ini","XDG diagnostic base label");
 xdg.set(scratch/"stray/../normalized");
 check(xeenUserFolder()==scratch/"normalized/MMModern" && !fs::exists(scratch/"stray"),"XDG normalization created stray directory");
 xdg.set(scratch/"xdg");
 struct stat mode{};check(::stat(xeenUserFolder().c_str(),&mode)==0 && (mode.st_mode&0777)==0700,"private new user base mode");
 xdg.set({});check(xeenUserFolder()==scratch/"home/.local/share/MMModern","unset XDG fallback");
 check(xeenPrivatePaths(xeenUserFolder().native()+"/settings.ini")=="~/.local/share/MMModern/settings.ini","fallback diagnostic base label");
 xdg.set("relative");check(xeenUserFolder()==scratch/"home/.local/share/MMModern","relative XDG fallback");
 xdg.set(scratch/"xdg");home.set({});check(xeenUserFolder()==scratch/"xdg/MMModern","absolute XDG needs no HOME");
 xdg.set({});rejects([]{xeenUserFolder();},"HOME");
 home.set(scratch/"home");xdg.set(scratch/"xdg");
#endif
 const auto base=xeenUserFolder();
 Environment settings("MMODERN_SETTINGS_FILE",{}),player("MMODERN_PLAYER_DIR",{}),log("MMODERN_LOG_FILE",{}),roms("MMODERN_MT32_ROM_DIR",{});
#ifdef _WIN32
 appDataResult=E_FAIL;rejects([]{xeenPlayerDirectory();},"Cannot resolve Local AppData");
 rejects([]{xeenPrivatePaths("diagnostic");},"Cannot resolve Local AppData");appDataResult=S_OK;
 appDataOverride=fs::path{};
 check(xeenPlayerDirectory()==fs::path("MMModern"),"empty AppData player result changed");
 check(xeenPrivatePaths("diagnostic")=="diagnostic","empty AppData diagnostic result changed");
 check(XeenMt32::identify({}).directory.empty(),"empty AppData ROM result changed");
 appDataOverride=fs::path("relative-AppData");
 check(xeenPlayerDirectory()==fs::path("relative-AppData/MMModern"),"relative AppData player result changed");
 check(xeenPrivatePaths("relative-AppData/MMModern/settings.ini")=="%LOCALAPPDATA%\\MMModern\\settings.ini","relative AppData diagnostic result changed");
 check(XeenMt32::identify({}).directory==fs::path("relative-AppData/MMModern/roms"),"relative AppData ROM result changed");
 appDataOverride.reset();
 SetEnvironmentVariableW(L"MMODERN_SETTINGS_FILE",L"");
 check(!xeenSettingsFile(),"empty Windows settings override must be nullopt");settings.set({});
#endif
 check(xeenPlayerDirectory()==base && xeenLogFile()==base/"logs/mmodern.log","default player/log scopes");
 check(xeenSettingsFile()==base/"settings.ini" && windowPlacementFile()==base/"window.ini","default settings/window scopes");
 check(XeenMt32::identify({}).directory==base/"roms","default ROM scope");
 player.set(scratch/"player");
 check(xeenPlayerDirectory()==scratch/"player" && xeenLogFile()==scratch/"player/logs/mmodern.log","player override log scope");
 check(xeenSettingsFile()==base/"settings.ini" && windowPlacementFile()==base/"window.ini" && XeenMt32::identify({}).directory==base/"roms","player override leaked");
 check(!xeenPlayerCfg(),"owned CFG override should start absent");
 std::vector<std::uint8_t> cfg(30);xeenWriteOwnedCfg(scratch/"player/XEEN.CFG",cfg);
 check(xeenPlayerCfg()==scratch/"player/XEEN.CFG","player override CFG scope");
 settings.set(scratch/"settings.ini");log.set(scratch/"chosen.log");roms.set(scratch/"roms");
 check(xeenSettingsFile()==scratch/"settings.ini" && xeenLogFile()==scratch/"chosen.log","individual overrides");
 check(xeenPlayerDirectory()==scratch/"player" && windowPlacementFile()==base/"window.ini" && XeenMt32::identify({}).directory==base/"roms","individual overrides leaked to base");
 check(XeenMt32::identify(scratch/"roms").directory==scratch/"roms","explicit ROM override scope");
 GameInstallation installation;installation.root=scratch/"data";fs::create_directories(installation.root);
 installation.xeenArchive=installation.root/"xeen.cc";std::ofstream(installation.xeenArchive)<<"synthetic archive fingerprint";
 const auto saveBase=scratch/"save-base";fs::create_directory(saveBase);
 const auto managed=XeenSaveFile::createSlotDirectory(installation,scratch/"repository",saveBase);
 const auto id=XeenSaveFile::dataId(XeenSaveFile::fingerprint(installation));
 check(managed==XeenSaveFile::resolve(saveBase/"MMModern/Saves"/id/"check.mmsave",installation).parent_path(),"save seam scope");
 check(!fs::exists(scratch/"player/Saves"),"player override leaked into saves");
 fs::create_directories(scratch/"tracks");std::ofstream(scratch/"tracks/mixed.ogg")<<"metadata fixture";
 const auto cue=scratch/"case.ins";
 std::ofstream(cue)<<"FILE \"TrAcKs\\MiXeD.OgG\" OGG\n  TRACK 02 AUDIO\n    INDEX 01 00:00:00\n";
 const auto tracks=readXeenCdTracks(cue,1);
 check(tracks.size()==1 && tracks[0].file==scratch/"tracks/mixed.ogg" && tracks[0].error.empty(),"backslash case-insensitive cue FILE path");
 settings.set("relative");check(!xeenSettingsFile(),"relative settings override");
 player.set("relative");rejects([]{xeenPlayerDirectory();},"absolute");player.set(scratch/"player");
 auto first=sample(),next=first;next.questItems[17]=34;
 const auto path=XeenSaveFile::resolve(scratch/"save.MMSAVE",scratch/"commercial");
 XeenSaveFile::write(path,first);
 using Op=XeenSaveFile::Operation;
 for(const auto op:{Op::Open,Op::Write,Op::ShortWrite,Op::Flush,Op::Close,Op::Replace,Op::Revalidate,Op::Publish}) {
  XeenSaveFile::write(path,first);
  rejects([&]{XeenSaveFile::write(path,next,[&](Op current){return current==op;});},op==Op::Publish?"Publish save file":"");
  check(raw(path)==XeenSaveFormat::encode(op==Op::Publish?next:first),"failure changed wrong save bytes");noTemporary(scratch);
 }
 rejects([&]{XeenSaveFile::write(path,first,[](Op op){return op==Op::Replace || op==Op::Cleanup;});},"cleanup failed");
 unsigned leftovers=0;
 for(const auto &entry:fs::directory_iterator(scratch))if(entry.path().filename().u8string().find(".tmp-")!=std::string::npos){++leftovers;fs::remove(entry.path());}
 check(leftovers==1,"cleanup fault evidence");sameSnapshot(XeenSaveFile::read(path),next);
#ifndef _WIN32
 const auto game=scratch/"commercial";fs::create_directories(game/"nested");
 const auto leaf=scratch/"leaf.mmsave";
 std::ofstream(game/"inside.dat")<<"synthetic game file";
 fs::create_symlink(game/"inside.dat",leaf);
 rejects([&]{XeenSaveFile::resolve(leaf,game);},"ordinary local file");fs::remove(leaf);
 fs::create_directory_symlink(game,scratch/"protected-root-link");
 rejects([&]{XeenSaveFile::resolve(game/"nested/save.mmsave",scratch/"protected-root-link");},"installation");
 for(const bool savesLink:{false,true}) {
  const auto linkBase=scratch/(savesLink?"managed-saves-link":"managed-base-link");fs::create_directory(linkBase);
  if(savesLink)fs::create_directory(linkBase/"MMModern");
  fs::create_directory_symlink(installation.root,savesLink?linkBase/"MMModern/Saves":linkBase/"MMModern");
  rejects([&]{XeenSaveFile::createSlotDirectory(installation,scratch/"repository",linkBase);},"installation");
  check(!fs::exists(installation.root/(savesLink?id:"Saves")),"managed link wrote into game directory");
 }
 // A stale, symlinked parent is refused before any save I/O.
 fs::create_directory(scratch/"pinned");fs::create_directory_symlink(scratch/"pinned",scratch/"alias");
 rejects([&]{XeenSaveFile::write(scratch/"alias/save.mmsave",first);},"Save parent directory identity changed");
 // Once locked, validation/publication and cleanup stay with the same directory
 // even when its former pathname is replaced with a link into game data.
 for(const bool cleanup:{false,true}) {
  const auto live=scratch/(cleanup?"cleanup-live":"publish-live"),pinned=scratch/(cleanup?"cleanup-pinned":"publish-pinned");
  fs::create_directory(live);XeenSaveFile::write(live/"save.mmsave",first);XeenSaveFile::write(game/"save.mmsave",first);
  fs::path sentinel;
  const auto swap=[&](Op op) {
   if(op!=(cleanup?Op::Write:Op::Revalidate))return false;
   if(cleanup)for(const auto &entry:fs::directory_iterator(live))if(entry.path().filename().native().find(".tmp-")!=std::string::npos)sentinel=entry.path().filename();
   fs::rename(live,pinned);fs::create_directory_symlink(game,live);
   if(cleanup){check(!sentinel.empty(),"temporary fixture missing");std::ofstream(game/sentinel)<<"protected sentinel";}
   return cleanup;
  };
  if(cleanup)rejects([&]{XeenSaveFile::write(live/"save.mmsave",next,swap);},"Injected write failure");
  else XeenSaveFile::write(live/"save.mmsave",next,swap);
  check(raw(pinned/"save.mmsave")==XeenSaveFormat::encode(cleanup?first:next),"directory-relative publication changed wrong target");
  check(raw(game/"save.mmsave")==XeenSaveFormat::encode(first),"path swap wrote into game data");
  if(cleanup){check(raw(game/sentinel)==Bytes({'p','r','o','t','e','c','t','e','d',' ','s','e','n','t','i','n','e','l'}),"cleanup followed swapped pathname");fs::remove(game/sentinel);}
  noTemporary(pinned);noTemporary(game);
 }
 fs::create_symlink(path,leaf);rejects([&]{XeenSaveFile::resolve(leaf,game);});rejects([&]{XeenSaveFile::read(leaf);});fs::remove(leaf);
 fs::create_symlink(scratch/"absent",leaf);rejects([&]{XeenSaveFile::resolve(leaf,game);});fs::remove(leaf);
 fs::create_directory_symlink(game,scratch/"parent");
 rejects([&]{XeenSaveFile::resolve(scratch/"parent/save.mmsave",game);},"installation");
 fs::create_directory_symlink(scratch/"parent",scratch/"second");
 rejects([&]{XeenSaveFile::resolve(scratch/"second/nested/save.mmsave",game);},"installation");
 rejects([&]{XeenSaveFile::resolve(game/"nested/save.mmsave",game);},"installation");
 check(::mkfifo(leaf.c_str(),0600)==0,"FIFO fixture");
 rejects([&]{XeenSaveFile::resolve(leaf,game);});rejects([&]{XeenSaveFile::read(leaf);});
 check(XeenSaveFile::inspectSlot(leaf,first.resources).state==XeenSaveFile::Slot::State::Protected,"FIFO load menu");fs::remove(leaf);
 // Swap after the precheck. The validation read must refuse promptly too.
 XeenSaveFile::write(leaf,first);
 rejects([&]{XeenSaveFile::write(leaf,next,[&](Op op){if(op==Op::Revalidate){fs::remove(leaf);check(::mkfifo(leaf.c_str(),0600)==0,"race FIFO");}return false;});},"ordinary local file");
 fs::remove(leaf);noTemporary(scratch);
 if(::geteuid()==0)std::cout<<"SKIP: unwritable-directory case; root bypasses directory write permissions (geteuid()==0)\n";
 else {
  fs::create_directory(scratch/"unwritable");check(::chmod((scratch/"unwritable").c_str(),0500)==0,"chmod");
  rejects([&]{XeenSaveFile::write(scratch/"unwritable/save.mmsave",first);},"Create temporary save failed");
  check(::chmod((scratch/"unwritable").c_str(),0700)==0,"restore chmod");
 }
 const auto bytes=scratch/fs::path(std::string("non-utf8-\xff",10));fs::create_directory(bytes);
 const auto bytePath=XeenSaveFile::resolve(bytes/"save.mmsave",game);XeenSaveFile::write(bytePath,first);sameSnapshot(XeenSaveFile::read(bytePath),first);
 xdg.set(bytes);check(xeenUserFolder()==bytes/"MMModern","byte-preserving XDG base");
 check(xeenPathText(bytes).find("non-utf8-\\xff")!=std::string::npos,"non-UTF-8 diagnostic escaping");
 check(xeenDiagnosticText(u8"valid \u00e9 \u4e2d")==u8"valid \u00e9 \u4e2d","valid UTF-8 diagnostics");
 check(xeenDiagnosticText(std::string("\xc0\x80\xed\xa0\x80",5))=="\\xc0\\x80\\xed\\xa0\\x80","invalid UTF-8 diagnostics");
 xdg.set(installation.root/"missing-base");
 rejects([&]{XeenSaveFile::createSlotDirectory(installation,scratch/"repository");},"installation");
 check(!fs::exists(installation.root/"missing-base"),"protected XDG base was created before validation");
 for(const auto type:{0x6969UL,0x517bUL,0xff534d42UL,0xfe534d42UL,0x5346414fUL,0x6b414653UL,0x73757245UL,0x00c36400UL,0x564cUL,0x01021997UL,0x786f4256UL})check(saveio::remoteType(type),"remote statfs type");
 check(!saveio::remoteType(0xef53) && !saveio::remoteType(0x65735546),"local statfs type");
 for(const auto *subtype:{"sshfs","rclone","gvfsd-fuse","s3fs","davfs","curlftpfs"}) {
  const auto line=std::string("42 1 0:7 / /mnt/path\\040space rw shared:1 - fuse.")+subtype+" source rw\n";
  check(saveio::remoteFuseMount(line,0,7),"network FUSE mountinfo");check(!saveio::remoteFuseMount(line,0,8),"unrelated FUSE mount");
 }
 check(!saveio::remoteFuseMount("42 1 0:7 / /mnt rw - fuse.local local rw\n",0,7),"local FUSE subtype");
 check(xeenIsOutOfSpace(std::runtime_error("Write save file failed (POSIX error "+std::to_string(ENOSPC)+")")),"ENOSPC");
 check(xeenIsOutOfSpace(std::runtime_error("Write save file failed (POSIX error "+std::to_string(EDQUOT)+")")),"EDQUOT");
#else
 check(xeenIsOutOfSpace(std::runtime_error("Windows error 112")) && xeenIsOutOfSpace(std::runtime_error("Windows error 39")),"Windows disk full");
#endif
 check(!xeenIsOutOfSpace(std::runtime_error("Other save failure")),"unrelated error");
 check(removeTestTree(scratch),"cannot remove platform scratch");std::cout<<"Platform save failures and folder scopes passed\n";return 0;
}catch(const std::exception &e){std::cerr<<e.what()<<'\n';return 1;}}
