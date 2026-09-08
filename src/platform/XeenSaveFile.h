#ifndef MMODERN_PLATFORM_XEEN_SAVE_FILE_H
#define MMODERN_PLATFORM_XEEN_SAVE_FILE_H
#include "core/GameInstallation.h"
#include "games/xeen/XeenSaveSnapshot.h"
#include <functional>
namespace mmodern {
class XeenSaveFile {
public:
 enum class Operation { Open, Write, ShortWrite, Flush, Close, Replace, Cleanup };
 // Test-only failure points around the real local Windows operations.
 using Fault = std::function<bool(Operation)>;
 static std::filesystem::path resolve(const std::filesystem::path &path,
   const std::filesystem::path &installation);
 // Callers resolve once before use; these never choose a fallback/temporary path.
 static XeenSaveSnapshot read(const std::filesystem::path &absolutePath);
 static void write(const std::filesystem::path &absolutePath,
   const XeenSaveSnapshot &snapshot, const Fault &fault = {});
 static XeenSaveResourceSignature fingerprint(const GameInstallation &installation);
};
}
#endif
