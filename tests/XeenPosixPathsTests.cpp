#include "games/xeen/XeenInstallationDetector.h"
#include "formats/xeen/XeenRussianTranslation.h"
#include "platform/XeenSaveFile.h"
#include <filesystem>
#include <iostream>
#include <stdexcept>
using namespace mmodern;
namespace fs=std::filesystem;
namespace {
struct CrossDevice {};
void linkFile(const fs::path &source,const fs::path &target) {
 std::error_code error;fs::create_hard_link(source,target,error);
 if(error==std::errc::cross_device_link)throw CrossDevice{};
 if(error)throw fs::filesystem_error("Create path-test hard link",source,target,error);
}
void check(bool value,const char *message){if(!value)throw std::runtime_error(message);}
std::string lower(std::string name){for(auto &c:name)if(c>='A' && c<='Z')c+=32;return name;}
// Hard links give this read-only test independent directory names without
// duplicating the multi-gigabyte CD tracks. The test never writes file bytes.
void aliasTree(const fs::path &source,const fs::path &target) {
 fs::create_directories(target);
 for(const auto &entry:fs::directory_iterator(source)) {
  const auto child=target/lower(entry.path().filename().native());
  if(entry.is_directory())aliasTree(entry.path(),child);
  else if(entry.is_regular_file())linkFile(entry.path(),child);
 }
}
}
int main(int argc,char **argv){try {
 check(argc==4,"POSIX source paths <data> <translation-or-empty> <scratch>");
 const auto scratch=fs::absolute(argv[3]);fs::remove_all(scratch);fs::create_directories(scratch);
 const auto copy=scratch/fs::path(std::string("lowercase-\xff",11));aliasTree(argv[1],copy);
 const auto original=XeenInstallationDetector().detect(argv[1]);
 const auto installation=XeenInstallationDetector().detect(copy);
 check(bool(original) && bool(installation),"lowercase installation with non-UTF-8 folder");
 check(XeenSaveFile::fingerprint(*original)==XeenSaveFile::fingerprint(*installation),"lowercase resource identity");
 check(!installation->cdTracks.empty(),"lowercase cue tracks");
 for(const auto &track:installation->cdTracks)check(track.error.empty() && fs::exists(track.file),"lowercase/backslash cue lookup");
 if(argv[2][0]) {
  const auto ru=scratch/fs::path(std::string("russian-\xff",9));aliasTree(argv[2],ru);
  check(bool(XeenRussianTranslation::open(ru)),"non-UTF-8 Russian folder");
 }
 if(installation->uiModule) {
  const auto module=installation->uiModule->physicalPath;
  linkFile(module,module.parent_path()/"xEeN.dAt");
  bool refused=false;try{XeenInstallationDetector().detect(copy);}catch(const std::exception &e){refused=std::string(e.what()).find("Ambiguous")!=std::string::npos;}
  check(refused,"ambiguous source name accepted");
 }
 fs::remove_all(scratch);std::cout<<"Lowercase and non-UTF-8 original-data paths passed\n";return 0;
}catch(const CrossDevice &){
 std::cout<<"SKIP: POSIX original-path cases require hard links on the same filesystem (EXDEV: cross-device link)\n";
 fs::remove_all(fs::absolute(argv[3]));return 0;
}catch(const std::exception &e){std::cerr<<e.what()<<'\n';return 1;}}
