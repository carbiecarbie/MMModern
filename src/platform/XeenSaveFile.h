#ifndef MMODERN_PLATFORM_XEEN_SAVE_FILE_H
#define MMODERN_PLATFORM_XEEN_SAVE_FILE_H
#include "core/GameInstallation.h"
#include "games/xeen/XeenSaveSnapshot.h"
#include <functional>
namespace mmodern {
class XeenSaveFile {
public:
 enum class Operation { Open, Write, ShortWrite, Flush, Close, Replace, Revalidate, Cleanup };
 // Test-only failure points around the real local Windows operations.
 using Fault = std::function<bool(Operation)>;
 static std::filesystem::path resolve(const std::filesystem::path &path,
   const std::filesystem::path &installation);
 static std::filesystem::path resolve(const std::filesystem::path &path, const GameInstallation &installation);
 // Callers resolve once before use; these never choose a fallback/temporary path.
 static XeenSaveSnapshot read(const std::filesystem::path &absolutePath);
 static void write(const std::filesystem::path &absolutePath,
   const XeenSaveSnapshot &snapshot, const Fault &fault = {}, bool managed = false);
 // Explicit directory creation. The override is a test seam, never a fallback.
 static std::filesystem::path createSlotDirectory(const GameInstallation &,
   const std::filesystem::path &repository, const std::optional<std::filesystem::path> &localAppData = {});
 static std::filesystem::path slotPath(const std::filesystem::path &directory, unsigned slot);
 struct Slot {
  enum class State { Empty, Available, Older, Protected };
  State state = State::Empty;
  std::optional<XeenSaveSnapshot> snapshot;
  std::string reason;
 };
 static Slot inspectSlot(const std::filesystem::path &, const XeenSaveResourceSignature &);
 static void writeSlot(const std::filesystem::path &directory,unsigned slot,const XeenSaveSnapshot &,
  const GameInstallation &,const std::filesystem::path &repository,const Fault &fault={},
  const std::function<void()> &sourceCheck={});
 static XeenSaveResourceSignature fingerprint(const GameInstallation &installation);
};
}
#endif
