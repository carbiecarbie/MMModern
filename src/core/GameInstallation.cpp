#include "core/GameInstallation.h"
#include <stdexcept>
namespace mmodern {
ReadOnlyDataFile archiveDataFile(const GameInstallation &installation,XeenArchiveRole role) {
 const auto &resolved=role==XeenArchiveRole::Clouds?installation.cloudsData:
  role==XeenArchiveRole::Darkside?installation.darksideData:installation.introData;
 if(resolved) return *resolved;
 const auto path=role==XeenArchiveRole::Clouds?installation.xeenArchive:
  role==XeenArchiveRole::Darkside?installation.darkArchive:std::filesystem::path{};
 if(path.empty()) throw std::runtime_error("Requested archive role is absent");
 return ReadOnlyDataFile::plain(path);
}
}
