#include "XeenSaveTestSupport.h"
#include "platform/XeenSaveFile.h"
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#include <winioctl.h>
#include <cstring>
#include <fstream>
#include <iostream>
using namespace mmodern;
using namespace save_test;
namespace fs = std::filesystem;
Bytes raw(const fs::path &p) {
 HANDLE file=CreateFileW(p.c_str(),GENERIC_READ,FILE_SHARE_READ,nullptr,OPEN_EXISTING,FILE_ATTRIBUTE_NORMAL,nullptr);
 check(file!=INVALID_HANDLE_VALUE,"cannot open raw save bytes");
 const DWORD size=GetFileSize(file,nullptr);Bytes bytes(size);DWORD count=0;
 const bool read=ReadFile(file,bytes.data(),size,&count,nullptr)!=0;
 const bool closed=CloseHandle(file)!=0;check(read && closed && count==size,"cannot read/close raw save bytes");return bytes;
}
std::vector<std::wstring> nativeEntries(const fs::path &directory) {
 WIN32_FIND_DATAW data{};
 HANDLE search=FindFirstFileW((directory/L"*").c_str(),&data);
 check(search!=INVALID_HANDLE_VALUE,"cannot enumerate native fixture directory");
 std::vector<std::wstring> entries;
 do {if(std::wstring(data.cFileName)!=L"." && std::wstring(data.cFileName)!=L"..")entries.emplace_back(data.cFileName);}
 while(FindNextFileW(search,&data));
 const DWORD last=GetLastError();const bool closed=FindClose(search)!=0;
 check(last==ERROR_NO_MORE_FILES && closed,"cannot finish/close native directory enumeration");return entries;
}
void put(const fs::path &p,const Bytes &b) {
 HANDLE file=CreateFileW(p.c_str(),GENERIC_WRITE,0,nullptr,CREATE_ALWAYS,FILE_ATTRIBUTE_NORMAL,nullptr);
 check(file!=INVALID_HANDLE_VALUE,"cannot create fixture file");DWORD count=0;
 const bool written=WriteFile(file,b.data(),static_cast<DWORD>(b.size()),&count,nullptr)!=0;
 const bool closed=CloseHandle(file)!=0;check(written && closed && count==b.size(),"cannot write/close fixture file");
}
struct TestJunction {
 fs::path path;
 explicit TestJunction(const fs::path &link, const fs::path &target):path(link) {
  check(CreateDirectoryW(path.c_str(),nullptr)!=0,"could not create synthetic junction directory");
  HANDLE handle=CreateFileW(path.c_str(),GENERIC_WRITE,0,nullptr,OPEN_EXISTING,
   FILE_FLAG_OPEN_REPARSE_POINT|FILE_FLAG_BACKUP_SEMANTICS,nullptr);
  const auto substitute=L"\\??\\"+target.wstring(), printed=target.wstring();
  // Mount-point reparse buffer: eight-byte header, four WORD offsets/lengths, UTF-16 names.
  const WORD substituteBytes=static_cast<WORD>(substitute.size()*sizeof(wchar_t));
  const WORD printBytes=static_cast<WORD>(printed.size()*sizeof(wchar_t));
  const WORD printOffset=substituteBytes+sizeof(wchar_t);
  const WORD payload=8+printOffset+printBytes+sizeof(wchar_t);
  Bytes buffer(8+payload,0);
  const DWORD tag=IO_REPARSE_TAG_MOUNT_POINT;
  std::memcpy(buffer.data(),&tag,4); std::memcpy(buffer.data()+4,&payload,2);
  std::memcpy(buffer.data()+10,&substituteBytes,2); std::memcpy(buffer.data()+12,&printOffset,2);
  std::memcpy(buffer.data()+14,&printBytes,2);
  std::memcpy(buffer.data()+16,substitute.c_str(),substituteBytes);
  std::memcpy(buffer.data()+16+printOffset,printed.c_str(),printBytes);
  DWORD returned=0;
  const bool created=handle!=INVALID_HANDLE_VALUE && DeviceIoControl(handle,FSCTL_SET_REPARSE_POINT,
   buffer.data(),static_cast<DWORD>(buffer.size()),nullptr,0,&returned,nullptr);
  const DWORD failure=GetLastError();
  if(handle!=INVALID_HANDLE_VALUE) CloseHandle(handle);
  if(!created) {
   RemoveDirectoryW(path.c_str());
   throw std::runtime_error("Real Windows junction creation failed (Windows error "+std::to_string(failure)+")");
  }
 }
 ~TestJunction(){if(!path.empty()) RemoveDirectoryW(path.c_str());}
 void remove(){check(RemoveDirectoryW(path.c_str())!=0,"synthetic junction cleanup failed");path.clear();}
};
void directoryAliasTests(const fs::path &directory) {
 const auto fixture=directory/("aliases-"+std::to_string(GetCurrentProcessId())+"-"+std::to_string(GetTickCount64()));
 const auto protectedRoot=fixture/"protected", outside=fixture/"protected-other";
 fs::create_directories(protectedRoot/"child"); fs::create_directories(outside);
 TestJunction junction(fixture/"junction",protectedRoot);
 const fs::path dotted(protectedRoot.wstring()+L".");
 unsigned refused=0;
 for(const auto &parent:std::vector<fs::path>{protectedRoot,protectedRoot/"child",dotted,dotted/"child",
   junction.path,junction.path/"child"}) {
  unsigned writes=0;
  rejects([&]{
   const auto resolved=XeenSaveFile::resolve(parent/"blocked.mmsave",protectedRoot);
   ++writes; XeenSaveFile::write(resolved,sample());
  },"Cannot save inside the commercial game installation");
  check(writes==0,"protected alias reached save write");
  for(const auto &entry:fs::recursive_directory_iterator(protectedRoot))
   check(entry.is_directory(),"refused alias created destination or temporary file");
  ++refused;
 }
 // The installation argument itself must also be resolved through handles.
 for(const auto &root:std::vector<fs::path>{dotted,junction.path})
  rejects([&]{XeenSaveFile::resolve(protectedRoot/"blocked.mmsave",root);},"Cannot save inside");
 for(const auto &leaf:std::vector<fs::path>{"plain.mmsave","with spaces.mmsave",fs::path(L"\u00e7 \u6e38.mmsave")}) {
  const auto resolved=XeenSaveFile::resolve(outside/leaf,protectedRoot);
  auto first=sample(),replacement=first; replacement.questItems[17]=34;
  XeenSaveFile::write(resolved,first); sameSnapshot(XeenSaveFile::read(resolved),first);
  XeenSaveFile::write(resolved,replacement); sameSnapshot(XeenSaveFile::read(resolved),replacement);
 }
 const auto normalized=XeenSaveFile::resolve(fs::path(outside.wstring()+L".")/"alias.mmsave",protectedRoot);
 check(normalized==XeenSaveFile::resolve(outside/"alias.mmsave",protectedRoot),"returned parent retains directory alias");
 XeenSaveFile::write(normalized,sample()); sameSnapshot(XeenSaveFile::read(normalized),sample());
 junction.remove();
 std::cout<<refused<<" protected directory/alias cases refused before any write; real junction removed\n";
}
BY_HANDLE_FILE_INFORMATION directoryIdentity(const fs::path &path) {
 HANDLE handle=CreateFileW(path.c_str(),0,FILE_SHARE_READ|FILE_SHARE_WRITE|FILE_SHARE_DELETE,
  nullptr,OPEN_EXISTING,FILE_FLAG_BACKUP_SEMANTICS,nullptr);
 check(handle!=INVALID_HANDLE_VALUE,"cannot open identity fixture directory");
 BY_HANDLE_FILE_INFORMATION info{};
 const bool ok=GetFileInformationByHandle(handle,&info)!=0;
 const bool closed=CloseHandle(handle)!=0;
 check(ok && closed,"cannot inspect/close identity fixture directory");return info;
}
bool sameDirectory(const BY_HANDLE_FILE_INFORMATION &a,const BY_HANDLE_FILE_INFORMATION &b) {
 return a.dwVolumeSerialNumber==b.dwVolumeSerialNumber && a.nFileIndexHigh==b.nFileIndexHigh && a.nFileIndexLow==b.nFileIndexLow;
}
void extendedIdentityTests(const fs::path &directory) {
 const auto fixture=directory/("identity-"+std::to_string(GetCurrentProcessId())+"-"+std::to_string(GetTickCount64()));
 const auto protectedRoot=fixture/"protected";
 fs::create_directories(protectedRoot);
 const fs::path literal(L"\\\\?\\"+protectedRoot.wstring()+L".");
 check(CreateDirectoryW(literal.c_str(),nullptr)!=0,"cannot create literal trailing-dot fixture directory");
 TestJunction junction(fixture/"literal-junction",fs::path(protectedRoot.wstring()+L"."));
 const auto normalId=directoryIdentity(protectedRoot), literalId=directoryIdentity(literal);
 check(!sameDirectory(normalId,literalId),"fixture directories must be distinct");
 check(sameDirectory(directoryIdentity(junction.path),literalId),"junction must reach literal directory");
 const auto sentinel=protectedRoot/"keep.mmsave";
 const auto sentinelBytes=XeenSaveFormat::encode(sample());put(sentinel,sentinelBytes);
 const auto resolved=XeenSaveFile::resolve(junction.path/"identity-probe.mmsave",protectedRoot);
 check(sameDirectory(directoryIdentity(resolved.parent_path()),literalId),"resolved path changed checked directory identity");
 check(resolved.native().rfind(L"\\\\?\\",0)==0,"resolved path lost extended semantics");
 check(XeenSaveFile::resolve(resolved,protectedRoot)==resolved,"re-resolving stored path changed identity");
 check(resolved.filename()=="identity-probe.mmsave","save leaf changed");
 const auto actual=literal/"identity-probe.mmsave", forbidden=protectedRoot/"identity-probe.mmsave";
 const auto checkProtected=[&] {
  check(raw(sentinel)==sentinelBytes,"protected preexisting bytes changed");
  check(GetFileAttributesW(forbidden.c_str())==INVALID_FILE_ATTRIBUTES && GetLastError()==ERROR_FILE_NOT_FOUND,"save appeared in normalized protected sibling");
  unsigned count=0;
  for(const auto &entry:fs::directory_iterator(protectedRoot)) {
   ++count;check(entry.path().filename()=="keep.mmsave","unexpected protected sibling file/temp");
  }
  check(count==1,"protected sentinel disappeared");
 };
 const auto noTemps=[&] {
  for(const auto &entry:nativeEntries(literal))
   check(entry==L"identity-probe.mmsave","unexpected literal-directory temporary file");
  checkProtected();
 };
 auto first=sample(),next=first;next.questItems[17]=1234;
 XeenSaveFile::write(resolved,first);check(GetFileAttributesW(actual.c_str())!=INVALID_FILE_ATTRIBUTES,"save missing from checked literal directory");
 sameSnapshot(XeenSaveFile::read(actual),first);noTemps();
 XeenSaveFile::write(resolved,next);sameSnapshot(XeenSaveFile::read(actual),next);noTemps();
 const auto oldBytes=raw(actual);
 put(actual,Bytes{1,2,3});
 rejects([&]{XeenSaveFile::write(resolved,first);});
 check(raw(actual)==Bytes({1,2,3}),"malformed save in literal directory was overwritten");noTemps();
 put(actual,oldBytes);
 for(auto failure:{XeenSaveFile::Operation::Write,XeenSaveFile::Operation::Replace}) {
  bool sawTemporary=false;
  rejects([&]{XeenSaveFile::write(resolved,first,[&](auto op){
   if(op!=failure)return false;
   for(const auto &entry:nativeEntries(literal))
    if(entry.find(L"identity-probe.mmsave.tmp-")==0)sawTemporary=true;
   checkProtected();return true;
  });},"Injected");
  check(sawTemporary,"temporary was not in checked literal directory");
  check(raw(actual)==oldBytes,"failed replacement changed existing literal save");
  sameSnapshot(XeenSaveFile::read(actual),next);noTemps();
 }
 junction.remove();
 auto upper=protectedRoot.wstring();CharUpperBuffW(upper.data(),static_cast<DWORD>(upper.size()));
 rejects([&]{XeenSaveFile::resolve(forbidden,fs::path(upper));},"Cannot save inside");
 rejects([&]{XeenSaveFile::resolve(forbidden,fixture.root_path());},"Cannot save inside");
 rejects([&]{XeenSaveFile::resolve(forbidden,fs::path(L"\\\\?\\"+fixture.root_path().wstring()));},"Cannot save inside");
 DWORD handlesBefore=0,handlesAfter=0;
 check(GetProcessHandleCount(GetCurrentProcess(),&handlesBefore)!=0,"cannot count handles");
 for(unsigned i=0;i<32;++i) {
  check(XeenSaveFile::resolve(resolved,protectedRoot)==resolved,"stored path identity changed");
  rejects([&]{XeenSaveFile::resolve(forbidden,protectedRoot);},"Cannot save inside");
 }
 check(GetProcessHandleCount(GetCurrentProcess(),&handlesAfter)!=0 && handlesAfter==handlesBefore,"directory handles leaked");
 std::cout<<"Distinct protected/protected. junction identity, creation, replacement and failure cleanup passed\n";
}
int main(int argc,char **argv) {
 try {
  const auto directory=fs::current_path()/"save-file-tests";
  fs::create_directories(directory/"commercial");
  extendedIdentityTests(directory);
  if(argc==2 && std::string(argv[1])=="--identity-only")return 0;
  directoryAliasTests(directory);
  const auto path=XeenSaveFile::resolve(directory/fs::path(L"space \u00e7 \u6e38.mmsave"),directory/"commercial");
  check(XeenSaveFile::resolve(fs::path("save-file-tests")/fs::path(L"space \u00e7 \u6e38.mmsave"),directory/"commercial")==path,"relative save path resolution");
  rejects([&]{XeenSaveFile::resolve(directory/"CON.mmsave",directory/"commercial");},"device");
  fs::remove(path);
  auto old=sample(), next=old; next.questItems[17]=34;
  XeenSaveFile::write(path,old); sameSnapshot(XeenSaveFile::read(path),old);
  XeenSaveFile::write(path,next); sameSnapshot(XeenSaveFile::read(path),next);
  const auto prior=raw(path);
  using Op=XeenSaveFile::Operation;
  for (auto op:{Op::Open,Op::Write,Op::ShortWrite,Op::Flush,Op::Close,Op::Replace}) {
   rejects([&]{XeenSaveFile::write(path,old,[&](Op current){return current==op;});});
   check(raw(path)==prior,"failed save damaged old bytes"); sameSnapshot(XeenSaveFile::read(path),next);
  }
  rejects([&]{XeenSaveFile::write(path,old,[](Op op){return op==Op::Replace || op==Op::Cleanup;});},"cleanup failed");
  check(raw(path)==prior,"cleanup failure damaged target"); sameSnapshot(XeenSaveFile::read(path),next);
  unsigned leftovers=0;
  for(const auto &entry:fs::directory_iterator(directory)) if(entry.path().wstring().find(L".tmp-")!=std::wstring::npos) {
   ++leftovers; fs::remove(entry.path());
  }
  check(leftovers==1,"unexpected temporary cleanup behavior");
  HANDLE locked=CreateFileW(path.c_str(),GENERIC_READ,FILE_SHARE_READ,nullptr,OPEN_EXISTING,FILE_ATTRIBUTE_NORMAL,nullptr);
  check(locked!=INVALID_HANDLE_VALUE,"could not lock target");
  bool failed=false; try{XeenSaveFile::write(path,old);}catch(const std::exception&){failed=true;}
  CloseHandle(locked);check(failed && raw(path)==prior,"Windows locked target was replaced");sameSnapshot(XeenSaveFile::read(path),next);
  locked=CreateFileW(path.c_str(),GENERIC_READ,0,nullptr,OPEN_EXISTING,FILE_ATTRIBUTE_NORMAL,nullptr);
  check(locked!=INVALID_HANDLE_VALUE,"could not exclusively lock target");
  rejects([&]{XeenSaveFile::read(path);}); CloseHandle(locked);
  for(auto bad:std::vector<Bytes>{{1,2,3},Bytes(XeenSaveFormat::kMaximumSize+1),prior}) {
   if(bad==prior) bad[8]=2;
   put(path,bad); rejects([&]{XeenSaveFile::write(path,old);});check(raw(path)==bad,"unknown file overwritten");
  }
  put(path,prior);
  rejects([&]{XeenSaveFile::read(directory/"missing.mmsave");});
  for(const auto &bad:std::vector<fs::path>{directory/"bad.txt",directory/"missing"/"a.mmsave",directory/"commercial"/"a.mmsave",directory/"a.mmsave:stream.mmsave"})
   rejects([&]{XeenSaveFile::resolve(bad,directory/"commercial");});
  fs::create_directories(directory/"folder.mmsave");rejects([&]{XeenSaveFile::resolve(directory/"folder.mmsave",directory/"commercial");});
  const auto missing=directory/"uncreated.mmsave"; fs::remove(missing);
  rejects([&]{XeenSaveFile::write(missing,old,[](Op op){return op==Op::Write;});});check(!fs::exists(missing),"failed creation published");
  // Filesystem side of archive signatures, including Unicode, content and presence.
  const auto archive=directory/fs::path(L"\u00e7.cc");put(archive,Bytes{'1','2','3','4','5','6','7','8','9'});
  GameInstallation install{directory,archive,{},GameEdition::CloudsOfXeen};
  const auto signature=XeenSaveFile::fingerprint(install);check(signature.clouds==XeenArchiveFingerprint{9,0xcbf43926},"archive fingerprint");
  install.darkArchive=archive;check(XeenSaveFile::fingerprint(install).darkside==signature.clouds,"darkside presence");
  install.xeenArchive=directory/"missing.cc";rejects([&]{XeenSaveFile::fingerprint(install);});
  std::cout<<"Local Windows save replacement, faults, Unicode and signatures passed\n";return 0;
 }catch(const std::exception &e){std::cerr<<e.what()<<'\n';return 1;}
}
