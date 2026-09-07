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
	XeenMapIdentity mapId = 0;
	int x = 0;
	int y = 0;
	const XeenMapGeometry *geometry = nullptr;
	const XeenMapCell *cell = nullptr;
};

// Empty in 15A; actual session mutations are introduced by 15B.
struct XeenSessionWorldState {};

class XeenWorld {
public:
	using MapLoader = std::function<XeenMap(XeenMapIdentity)>;

	explicit XeenWorld(MapLoader loader);
	XeenWorld(const XeenWorld &) = delete;
	XeenWorld &operator=(const XeenWorld &) = delete;
	const XeenSessionWorldState &sessionState() const { return _sessionState; }
	// Invalidates map/cell references, not the session state.
	void discardMapCache() { _maps.clear(); }

	const XeenMap &map(XeenMapIdentity mapId);
	std::optional<XeenCellSample> sampleCell(XeenMapIdentity mapId, int x, int y);
	std::size_t cachedMapCount() const { return _maps.size(); }

private:
	XeenSessionWorldState _sessionState;
	MapLoader _loader;
	std::map<XeenMapIdentity, XeenMap> _maps;
};

} // namespace mmodern

#endif
