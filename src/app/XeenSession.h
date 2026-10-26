#ifndef MMODERN_APP_XEEN_SESSION_H
#define MMODERN_APP_XEEN_SESSION_H
#include "games/xeen/XeenSaveSnapshot.h"
#include <memory>
#include <filesystem>
namespace mmodern {
// Values crossing Application's outer loop; never borrowed gameplay owners.
struct XeenSessionEntry {
 enum class Kind { Title, New, Load, Exit, Retry, CancelNew, Save, MrWizard, CloudsIntro, CloudsEnding };
 Kind kind=Kind::Title;
 unsigned slot=0;
 std::filesystem::path path;
 std::string name;
 XeenDifficulty difficulty=XeenDifficulty::Adventurer;
 std::shared_ptr<const XeenSaveSnapshot> snapshot;
 std::uint32_t score=0;
};
struct XeenSessionOutcome {
 enum class Kind { Title, Exit, Failure, Load, CloudsEnding };
 Kind kind=Kind::Exit;
 int status=0;
 XeenSessionEntry entry;
 std::uint32_t score=0;
};
}
#endif
