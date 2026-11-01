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
  // Accept the legacy one/two-key files and the complete current four-key file.
  XeenSettings settings;unsigned seen=0;std::size_t at=0;
  while(at<text.size()) {
   const auto end=text.find('\n',at);if(end==std::string::npos)return {};
   auto line=text.substr(at,end-at);at=end+1;if(!line.empty() && line.back()=='\r')line.pop_back();
   const auto eq=line.find('=');if(eq==std::string::npos)return {};
   const auto key=line.substr(0,eq), value=line.substr(eq+1);
   if(value!="0" && value!="1")return {};
   bool *target=nullptr;unsigned bit=0;
   if(key=="won"){target=&settings.won;bit=1;}
   else if(key=="darkside_intro_seen"){target=&settings.darksideIntroSeen;bit=2;}
   else if(key=="darkside_won"){target=&settings.darksideWon;bit=4;}
   else if(key=="world_won"){target=&settings.worldWon;bit=8;}
   else return {};
   if(seen&bit)return {};seen|=bit;*target=value=="1";
  }
  if(seen==1 || seen==3 || seen==15)return settings;
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
   output<<"won="<<(settings.won?1:0)<<"\ndarkside_intro_seen="<<(settings.darksideIntroSeen?1:0)<<"\ndarkside_won="<<(settings.darksideWon?1:0)<<"\nworld_won="<<(settings.worldWon?1:0)<<"\n";
   if(!output.flush()){output.close();std::filesystem::remove(temporary,error);return false;}
  }
  if(MoveFileExW(temporary.c_str(),path.c_str(),MOVEFILE_REPLACE_EXISTING|MOVEFILE_WRITE_THROUGH))return true;
  std::filesystem::remove(temporary,error);return false;
 } catch(...) {return false;}
}
}
bool xeenReadWon(const std::filesystem::path &path) noexcept {return xeenReadSettings(path).won;}
bool xeenWriteWon(const std::filesystem::path &path,XeenWonKind kind) noexcept {
 auto s=xeenReadSettings(path);
 if(kind==XeenWonKind::Clouds)s.won=true;else if(kind==XeenWonKind::Darkside)s.darksideWon=true;else s.worldWon=true;
 return writeSettings(path,s);
}
bool xeenWriteSeen(const std::filesystem::path &path,bool seen) noexcept {auto s=xeenReadSettings(path);s.darksideIntroSeen=seen;return writeSettings(path,s);}
}
