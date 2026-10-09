#ifndef MMODERN_GAMES_XEEN_INSTALLATION_DETECTOR_H
#define MMODERN_GAMES_XEEN_INSTALLATION_DETECTOR_H

#include "core/GameInstallation.h"

#include <filesystem>
#include <optional>
#include <utility>
#include <array>

namespace mmodern {

// Explicit cue entrypoint for independent disc/source validation.
std::array<ReadOnlyDataFile,3> readCueCdArchives(const std::filesystem::path &cue);

class XeenInstallationDetector {
public:
 explicit XeenInstallationDetector(std::filesystem::path uiData = {}) : _uiData(std::move(uiData)) {}
	std::optional<GameInstallation> detect(const std::filesystem::path &root) const;
private:
 std::filesystem::path _uiData;
};

} // namespace mmodern

#endif
