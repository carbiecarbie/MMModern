#include "platform/XeenWonStore.h"
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#include <shlobj.h>
#include <fstream>
#include <iterator>
namespace mmodern {
std::optional<std::filesystem::path> xeenSettingsFile() {
 const auto size=GetEnvironmentVariableW(L"MMODERN_SETTINGS_FILE",nullptr,0);
 if(size) {
  std::wstring value(size,L'\0');GetEnvironmentVariableW(L"MMODERN_SETTINGS_FILE",value.data(),size);value.resize(size-1);
  const std::filesystem::path path(value);return path.is_absolute()?std::optional<std::filesystem::path>{path}:std::nullopt;
 }
 PWSTR value=nullptr;
 if(FAILED(SHGetKnownFolderPath(FOLDERID_LocalAppData,0,nullptr,&value))){CoTaskMemFree(value);return {};}
 const std::filesystem::path base(value);CoTaskMemFree(value);
 if(base.empty() || !base.is_absolute())return {};
 return base/"MMModern"/"settings.ini";
}
XeenSettings xeenReadSettings(const std::filesystem::path &path) noexcept {
 try {
  std::ifstream input(path,std::ios::binary);if(!input)return {};
  const std::string text((std::istreambuf_iterator<char>(input)),{});
  if(input.bad())return {};
  if(text=="won=1\n" || text=="won=1\r\n")return {true,false};
  // Only the complete two-key format is accepted; duplicate or unknown content is damaged.
  for(bool won:{false,true})for(bool seen:{false,true}) {
   const auto w=std::string("won=")+(won?"1":"0");
   const auto d=std::string("darkside_intro_seen=")+(seen?"1":"0");
   for(const auto *newline:{"\n","\r\n"})
    if(text==w+newline+d+newline || text==d+newline+w+newline)return {won,seen};
  }
 } catch(...) {}
 return {};
}
namespace {
bool writeSettings(const std::filesystem::path &path,XeenSettings settings) noexcept {
 try {
  std::error_code error;std::filesystem::create_directories(path.parent_path(),error);if(error)return false;
  auto temporary=path;temporary+=".tmp";
  {
   std::ofstream output(temporary,std::ios::binary|std::ios::trunc);
   output<<"won="<<(settings.won?1:0)<<"\ndarkside_intro_seen="<<(settings.darksideIntroSeen?1:0)<<"\n";
   if(!output.flush()){output.close();std::filesystem::remove(temporary,error);return false;}
  }
  if(MoveFileExW(temporary.c_str(),path.c_str(),MOVEFILE_REPLACE_EXISTING|MOVEFILE_WRITE_THROUGH))return true;
  std::filesystem::remove(temporary,error);return false;
 } catch(...) {return false;}
}
}
bool xeenReadWon(const std::filesystem::path &path) noexcept {return xeenReadSettings(path).won;}
bool xeenWriteWon(const std::filesystem::path &path) noexcept {auto s=xeenReadSettings(path);s.won=true;return writeSettings(path,s);}
bool xeenWriteSeen(const std::filesystem::path &path,bool seen) noexcept {auto s=xeenReadSettings(path);s.darksideIntroSeen=seen;return writeSettings(path,s);}
}
