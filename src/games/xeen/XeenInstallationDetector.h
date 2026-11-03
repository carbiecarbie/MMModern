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
 // M110: a non-null translation makes every detected installation Russian.
 explicit XeenInstallationDetector(std::filesystem::path uiData = {},std::filesystem::path cfg = {},
  std::shared_ptr<const XeenRussianTranslation> translation = {})
  : _uiData(std::move(uiData)),_cfg(std::move(cfg)),_translation(std::move(translation)) {}
	std::optional<GameInstallation> detect(const std::filesystem::path &root) const;
private:
 std::optional<GameInstallation> detectCd(const std::filesystem::path &root) const;
 std::filesystem::path _uiData;
 std::filesystem::path _cfg;
 std::shared_ptr<const XeenRussianTranslation> _translation;
};

} // namespace mmodern

#endif
