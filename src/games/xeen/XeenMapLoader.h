#ifndef MMODERN_GAMES_XEEN_MAP_LOADER_H
#define MMODERN_GAMES_XEEN_MAP_LOADER_H

#include "games/xeen/XeenMap.h"

#include <cstdint>
#include <functional>
#include <optional>

namespace mmodern {

class XeenAssetSource;

class XeenMapLoader {
public:
	using ResourceReader = std::function<std::optional<std::vector<std::uint8_t>>(const std::string &)>;
	XeenObjectFile loadObjects(XeenAssetSource &assets, XeenMapIdentity mapId) const;
	XeenObjectFile loadObjects(const ResourceReader &reader, XeenMapIdentity mapId) const;

	// Loads immutable geometry for either an outdoor or indoor Clouds map.
	// Gameplay data (.mob/.evt) deliberately remains outside this operation.
	XeenMap loadGeometryMap(XeenAssetSource &assets, XeenMapIdentity mapId) const;

	// Loads immutable geometry from the initial Clouds container. Gameplay data
	// (.mob/.evt) deliberately remains outside geometry loading.
	XeenMap loadOutdoorMap(XeenAssetSource &assets, XeenMapIdentity mapId) const;

	// Milestone 6 supports only the original Clouds Area A1, without save overlays.
	XeenMap loadAreaA1(XeenAssetSource &assets) const;
};

} // namespace mmodern

#endif
