#ifndef MMODERN_TOOLS_XEEN_TEST_SAVE_H
#define MMODERN_TOOLS_XEEN_TEST_SAVE_H

#include "core/GameInstallation.h"
#include <string>

namespace mmodern::developer {
// Developer-only: never linked into mmodern. The caller supplies the sole output path.
void generateTestSave(const GameInstallation &installation, const std::string &preset,
    const std::filesystem::path &output);
}
#endif
