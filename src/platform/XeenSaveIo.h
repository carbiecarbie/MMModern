#ifndef MMODERN_PLATFORM_XEEN_SAVE_IO_H
#define MMODERN_PLATFORM_XEEN_SAVE_IO_H
#include "platform/XeenSaveFile.h"
namespace mmodern::saveio {
using Path=std::filesystem::path;
bool ordinary(const Path &,bool absent);
Path resolve(const Path &,const Path &protectedRoot);
std::vector<std::uint8_t> read(const Path &);
#ifdef _WIN32
void write(const Path &,const std::vector<std::uint8_t> &,const std::function<void()> &validate,const XeenSaveFile::Fault &);
#else
// The file layer reads through its locked directory; the owner validates bytes.
void write(const Path &,const std::vector<std::uint8_t> &,const std::function<void(const std::vector<std::uint8_t> &)> &validate,const XeenSaveFile::Fault &);
#endif
Path createSlotDirectory(const std::function<Path(const Path &)> &check,const std::function<std::string()> &dataId,const std::optional<Path> &override);
// Pure classifier seams: mountinfo is used only for remote FUSE subtypes.
bool remoteType(unsigned long type);
bool remoteFuseMount(const std::string &mountinfo,unsigned deviceMajor,unsigned deviceMinor);
}
#endif
