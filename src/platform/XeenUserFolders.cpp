#include "platform/XeenUserFolders.h"
#include <stdexcept>
#ifdef _WIN32
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#include <shlobj.h>
#else
#include <cstdlib>
#include <cerrno>
#include <sys/stat.h>
#endif
namespace mmodern {
std::filesystem::path xeenEnvironment(const char *name) {
#ifdef _WIN32
 std::wstring key;while(*name)key+=wchar_t(*name++);
 const auto size=GetEnvironmentVariableW(key.c_str(),nullptr,0);if(!size)return {};
 std::wstring value(size,L'\0');GetEnvironmentVariableW(key.c_str(),value.data(),size);value.resize(size-1);
 return std::filesystem::path(value).make_preferred();
#else
 const auto *value=std::getenv(name);return value?std::filesystem::path(value):std::filesystem::path{};
#endif
}
#ifdef _WIN32
std::filesystem::path xeenWindowsAppData(bool playerFailure) {
 PWSTR value=nullptr;
 const auto status=SHGetKnownFolderPath(FOLDERID_LocalAppData,0,nullptr,&value);
 if(FAILED(status)) {
  if(playerFailure)throw std::runtime_error("Cannot resolve Local AppData");
  throw std::runtime_error("Resolve Local AppData failed (HRESULT "+std::to_string(status)+")");
 }
 struct Free {PWSTR value;~Free(){CoTaskMemFree(value);}} free{value};
 return std::filesystem::path(value);
}
#endif
const char *xeenUserFolderLabel() {
#ifdef _WIN32
 return "Local AppData/MMModern";
#else
 return xeenEnvironment("XDG_DATA_HOME").is_absolute()?"$XDG_DATA_HOME/MMModern":"~/.local/share/MMModern";
#endif
}
std::filesystem::path xeenUserFolder(bool create) {
#ifdef _WIN32
 (void)create;
 const auto base=xeenWindowsAppData();
 if(base.empty() || !base.is_absolute())throw std::runtime_error("Local AppData must be an absolute existing directory");
 return base/"MMModern";
#else
 auto base=xeenEnvironment("XDG_DATA_HOME");
 if(!base.is_absolute()) {
  base=xeenEnvironment("HOME");
  if(!base.is_absolute())throw std::runtime_error("HOME must be an absolute directory when XDG_DATA_HOME is unset or relative");
  base/=".local/share";
 }
 base/="MMModern";
 base=base.lexically_normal();
 if(!create)return base;
 // Create missing parents with the same private mode, without changing existing permissions.
 std::filesystem::path current;
 for(const auto &part:base) {
  current/=part;
  if(::mkdir(current.c_str(),0700)<0 && errno!=EEXIST) {
   const int code=errno;
   throw std::runtime_error("Create user directory failed (POSIX error "+std::to_string(code)+")");
  }
  struct stat st{};
  if(::stat(current.c_str(),&st)<0 || !S_ISDIR(st.st_mode))throw std::runtime_error("User directory is not a directory");
 }
 return base;
#endif
}
}
