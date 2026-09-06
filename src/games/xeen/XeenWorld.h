#ifndef MMODERN_GAMES_XEEN_WORLD_H
#define MMODERN_GAMES_XEEN_WORLD_H

#include "games/xeen/XeenMap.h"

#include <cstddef>
#include <cstdint>
#include <functional>
#include <map>
#include <optional>

namespace mmodern {

struct XeenCellSample {
	std::uint16_t mapId = 0;
	int x = 0;
	int y = 0;
	const XeenMapGeometry *geometry = nullptr;
	const XeenMapCell *cell = nullptr;
};

class XeenWorld {
public:
	using MapLoader = std::function<XeenMap(std::uint16_t)>;

	explicit XeenWorld(MapLoader loader);

	const XeenMap &map(std::uint16_t mapId);
	std::optional<XeenCellSample> sampleCell(std::uint16_t mapId, int x, int y);
	std::size_t cachedMapCount() const { return _maps.size(); }

private:
	MapLoader _loader;
	std::map<std::uint16_t, XeenMap> _maps;
};

} // namespace mmodern

#endif
