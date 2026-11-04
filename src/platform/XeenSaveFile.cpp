#include "platform/XeenSaveFile.h"
#include "platform/XeenSaveIo.h"
#include "formats/xeen/XeenSaveFormat.h"
#include <array>
#include <sstream>
#include <iomanip>
#include <zlib.h>
namespace mmodern {
using Path=std::filesystem::path;
std::filesystem::path XeenSaveFile::resolve(const Path &path,const Path &installation) {return saveio::resolve(path,installation);}
XeenSaveSnapshot XeenSaveFile::read(const Path &path) {return XeenSaveFormat::decode(saveio::read(path));}
void XeenSaveFile::write(const Path &path,const XeenSaveSnapshot &snapshot,const Fault &fault,bool managed) {
 const auto bytes = XeenSaveFormat::encode(snapshot); // Before any destination I/O.
 if(managed && !snapshot.name)throw std::runtime_error("Managed save requires a name");
 const auto validatePrevious=[&](const auto &readPrevious) {
   try {
    const auto previous=readPrevious();
    if(!(previous.resources==snapshot.resources))throw std::runtime_error("Save target belongs to different game data");
    if(managed && !previous.name)throw std::runtime_error("Managed target has no validated name");
   } catch(const XeenUnsupportedSave &e) {if(!e.recognizableOlder)throw;}
 };
#ifdef _WIN32
 const auto validateTarget=[&] {
  if(saveio::ordinary(path,true))validatePrevious([&]{return read(path);});
 };
#else
 const auto validateTarget=[&](const std::vector<std::uint8_t> &previous) {
  validatePrevious([&]{return XeenSaveFormat::decode(previous);});
 };
#endif
 saveio::write(path,bytes,validateTarget,fault);
}
std::filesystem::path XeenSaveFile::resolve(const Path &path,const GameInstallation &installation) {
 auto result=resolve(path,installation.root);
 for(const auto &directory:installation.protectedDirectories)result=resolve(result,directory);
 // Manually assembled fixtures still protect all explicit archive paths.
 for(const auto &file:{installation.xeenArchive,installation.darkArchive})if(!file.empty())result=resolve(result,file.parent_path());
 if(installation.uiModule)result=resolve(result,installation.uiModule->physicalPath.parent_path());
 for(const auto &file:{installation.cloudsData,installation.darksideData,installation.introData})
  if(file)result=resolve(result,file->physicalPath.parent_path());
 return result;
}
XeenSaveResourceSignature XeenSaveFile::fingerprint(const GameInstallation &installation) {
 const auto hash=[](const ReadOnlyDataFile &file) {
  auto stream=file.open();std::array<std::uint8_t,65536> bytes{};XeenArchiveFingerprint result{};
  result.size=stream->size();uLong crc=crc32(0,nullptr,0);
  while(stream->pos()<stream->size()) { const auto count=stream->read(bytes.data(),bytes.size());crc=crc32(crc,bytes.data(),static_cast<uInt>(count)); }
  result.crc32=static_cast<std::uint32_t>(crc);return result;
 };
 XeenSaveResourceSignature result;result.clouds=hash(archiveDataFile(installation,XeenArchiveRole::Clouds));
 if(installation.hasDarkside())result.darkside=hash(archiveDataFile(installation,XeenArchiveRole::Darkside));return result;
}
std::filesystem::path XeenSaveFile::slotPath(const Path &directory,unsigned slot) {
 if(slot>=10)throw std::out_of_range("Save slot must be in 0..9");
 return directory/("slot-"+std::to_string(slot)+".mmsave");
}
XeenSaveFile::Slot XeenSaveFile::inspectSlot(const Path &path,const XeenSaveResourceSignature &signature) {
 Slot result;
 try {
  if(!saveio::ordinary(path,true))return result;
  auto snapshot=read(path);
  if(!(snapshot.resources==signature))throw std::runtime_error("Save belongs to different game data");
  if(!snapshot.name)throw std::runtime_error("Managed save has no validated name");
  result.snapshot=std::move(snapshot);result.state=Slot::State::Available;
 } catch(const XeenUnsupportedSave &e) {
  result.state=e.recognizableOlder ? Slot::State::Older : Slot::State::Protected;result.reason=e.what();
 } catch(const std::exception &e) {result.state=Slot::State::Protected;result.reason=e.what();}
 return result;
}
void XeenSaveFile::writeSlot(const Path &directory,unsigned slot,const XeenSaveSnapshot &snapshot,
 const GameInstallation &installation,const Path &repository,const Fault &fault,const std::function<void()> &check) {
 if(check)check();
 const auto target=resolve(resolve(slotPath(directory,slot),installation),repository);
 if(check)check();
 write(target,snapshot,[&](Operation op) {
  if(check)check();const bool fail=fault && fault(op);if(check)check();
  if(op==Operation::Revalidate && resolve(resolve(target,installation),repository)!=target)
   throw std::runtime_error("Managed save directory identity changed");
  return fail;
 },true);
 if(check)check();
}
std::string XeenSaveFile::dataId(const XeenSaveResourceSignature &signature) {
 std::ostringstream key;key<<std::hex<<std::setfill('0')<<std::setw(16)<<signature.clouds.size
  <<'-'<<std::setw(8)<<signature.clouds.crc32;
 if(signature.darkside)key<<'-'<<std::setw(16)<<signature.darkside->size<<'-'<<std::setw(8)<<signature.darkside->crc32;
 else key<<"-no-darkside";
 return key.str();
}

std::filesystem::path XeenSaveFile::createSlotDirectory(const GameInstallation &installation,
 const Path &repository,const std::optional<Path> &override) {
 const auto check=[&](const Path &directory) {
  auto probe=resolve(directory/"slot-directory-check.mmsave",installation);
  probe=resolve(probe,repository);
  return probe.parent_path();
 };
 return saveio::createSlotDirectory(check,[&]{return dataId(fingerprint(installation));},override);
}
}
