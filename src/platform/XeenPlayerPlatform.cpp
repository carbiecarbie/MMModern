#include "core/PathText.h"
#include "platform/XeenPlayerPlatform.h"
#include "platform/XeenFileOperations.h"
#include "app/XeenPlayerSupport.h"
#include "formats/xeen/XeenSoundSetup.h"
#include <algorithm>
#include <stdexcept>
#ifdef _WIN32
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#else
#include <sys/utsname.h>
#include <fcntl.h>
#include <unistd.h>
#include <cerrno>
#endif
namespace mmodern {
using Path=std::filesystem::path;
#ifdef _WIN32
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
void xeenOpenLog(std::ofstream &file) {
   const auto path=xeenLogFile();std::filesystem::create_directories(path.parent_path());
   auto previous=path.parent_path()/(path.stem().wstring()+L"-previous"+path.extension().wstring());
   if(std::filesystem::exists(path)) {
    if(!std::filesystem::is_regular_file(path))return;
    if(!MoveFileExW(path.c_str(),previous.c_str(),MOVEFILE_REPLACE_EXISTING))return;
   }
   file.open(path,std::ios::binary|std::ios::trunc);if(!file)return;
}
void xeenLocalTime(std::tm &date,const std::time_t &now){localtime_s(&date,&now);}
#else
std::string windowsVersion() {
 struct utsname value{};if(::uname(&value)<0)return "Linux unknown";
 return std::string(value.sysname)+" "+value.release+" "+value.machine;
}
void replacePath(std::string &text,const Path &path,const char *replacement) {
 const auto &needle=path.native();if(needle.empty())return;
 for(std::size_t pos=0;(pos=text.find(needle,pos))!=std::string::npos;) {
  const auto end=pos+needle.size();
  if(end<text.size() && text[end]!='/' && text[end]!='\n' && text[end]!='\r' && text[end]!='"' && text[end]!='\'' && text[end]!=';' && text[end]!=' ') {pos=end;continue;}
  text.replace(pos,needle.size(),replacement);pos+=std::char_traits<char>::length(replacement);
 }
}
void xeenWriteOwnedCfg(const Path &path,const std::vector<std::uint8_t> &bytes) {
 xeenValidateSetupBase(bytes);std::filesystem::create_directories(path.parent_path());
 Path temporary;int file=-1;
 for(unsigned n=0;n<100 && file<0;++n) {
  temporary=path;temporary+=".tmp-"+std::to_string(::getpid())+"-"+std::to_string(n);
  file=::open(temporary.c_str(),O_WRONLY|O_CREAT|O_EXCL|O_CLOEXEC,0666);
  if(file<0 && errno!=EEXIST)break;
 }
 if(file<0)throw std::runtime_error("Cannot create temporary CFG beside "+xeenPathText(path));
 bool ok=true;std::size_t offset=0;
 while(offset<bytes.size()) {
  const auto count=::write(file,bytes.data()+offset,bytes.size()-offset);
  if(count<0 && errno==EINTR)continue;
  if(count<=0){ok=false;break;}offset+=count;
 }
 if(::fsync(file)<0)ok=false;
 if(::close(file)<0)ok=false;
 if(ok)ok=xeenReplaceSmallFile(temporary,path);
 if(!ok){::unlink(temporary.c_str());throw std::runtime_error("Cannot replace owned CFG: "+xeenPathText(path));}
}
void xeenResetOwnedCfg(const Path &path) {
 if(::unlink(path.c_str())<0 && errno!=ENOENT && errno!=ENOTDIR)
  throw std::runtime_error("Cannot reset owned CFG: "+xeenPathText(path));
}
void xeenOpenLog(std::ofstream &file) {
   const auto path=xeenLogFile();std::filesystem::create_directories(path.parent_path());
   auto previous=path.parent_path()/(path.stem().native()+std::string("-previous")+path.extension().native());
   if(std::filesystem::exists(path)) {
    if(!std::filesystem::is_regular_file(path))return;
    if(::rename(path.c_str(),previous.c_str())<0)return;
   }
   file.open(path,std::ios::binary|std::ios::trunc);if(!file)return;
}
void xeenLocalTime(std::tm &date,const std::time_t &now){if(!localtime_r(&now,&date))throw std::runtime_error("Host wall clock error");}
#endif
}
