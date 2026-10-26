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
bool xeenReadWon(const std::filesystem::path &path) noexcept {
 try {
  std::ifstream input(path,std::ios::binary);if(!input)return false;
  const std::string text((std::istreambuf_iterator<char>(input)),{});
  return !input.bad() && (text=="won=1\n" || text=="won=1\r\n");
 } catch(...) {return false;}
}
bool xeenWriteWon(const std::filesystem::path &path) noexcept {
 try {
  std::error_code error;std::filesystem::create_directories(path.parent_path(),error);if(error)return false;
  auto temporary=path;temporary+=".tmp";
  {
   std::ofstream output(temporary,std::ios::binary|std::ios::trunc);output<<"won=1\n";
   if(!output.flush()){output.close();std::filesystem::remove(temporary,error);return false;}
  }
  // Sibling publication, with replacement and write-through on Windows.
  if(MoveFileExW(temporary.c_str(),path.c_str(),MOVEFILE_REPLACE_EXISTING|MOVEFILE_WRITE_THROUGH))return true;
  std::filesystem::remove(temporary,error);return false;
 } catch(...) {return false;}
}
}
