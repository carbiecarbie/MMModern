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
std::vector<XeenCdTrack> readXeenCdTracks(const std::filesystem::path &cue,unsigned disc);

class XeenInstallationDetector {
public:
 explicit XeenInstallationDetector(std::filesystem::path uiData = {},std::filesystem::path cfg = {})
  : _uiData(std::move(uiData)),_cfg(std::move(cfg)) {}
	std::optional<GameInstallation> detect(const std::filesystem::path &root) const;
private:
 std::filesystem::path _uiData;
 std::filesystem::path _cfg;
};

} // namespace mmodern

#endif
