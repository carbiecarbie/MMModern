#ifndef MMODERN_GAMES_XEEN_INSTALLATION_DETECTOR_H
#define MMODERN_GAMES_XEEN_INSTALLATION_DETECTOR_H

#include "core/GameInstallation.h"

#include <filesystem>
#include <optional>

namespace mmodern {

class XeenInstallationDetector {
public:
	std::optional<GameInstallation> detect(const std::filesystem::path &root) const;
};

} // namespace mmodern

#endif
