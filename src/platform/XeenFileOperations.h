#ifndef MMODERN_PLATFORM_XEEN_FILE_OPERATIONS_H
#define MMODERN_PLATFORM_XEEN_FILE_OPERATIONS_H
#include <filesystem>
#include <exception>
namespace mmodern {
bool xeenIsOutOfSpace(const std::exception &error);
// Small-file operations retain each caller's temporary naming and flushing.
bool xeenReplaceSmallFile(const std::filesystem::path &temporary,const std::filesystem::path &target);
#ifndef _WIN32
void xeenSyncDirectory(const std::filesystem::path &path);
void xeenSyncDirectory(int directory);
#endif
}
#endif
