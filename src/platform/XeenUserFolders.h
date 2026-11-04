#ifndef MMODERN_PLATFORM_XEEN_USER_FOLDERS_H
#define MMODERN_PLATFORM_XEEN_USER_FOLDERS_H
#include <filesystem>
#include <optional>
namespace mmodern {
// The per-user base only. Callers retain ownership of their existing overrides.
std::filesystem::path xeenUserFolder(bool create = true);
std::filesystem::path xeenEnvironment(const char *name);
#ifdef _WIN32
// Preserve caller-specific failure and empty/relative handling at the boundary.
std::filesystem::path xeenWindowsAppData(bool playerFailure = false);
#endif
}
#endif
