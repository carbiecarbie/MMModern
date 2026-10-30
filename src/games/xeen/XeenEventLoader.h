#ifndef MMODERN_GAMES_XEEN_XEEN_EVENT_LOADER_H
#define MMODERN_GAMES_XEEN_XEEN_EVENT_LOADER_H

#include "games/xeen/XeenEventFile.h"

#include <cstdint>
#include <functional>
#include <optional>
#include <string>
#include <vector>

namespace mmodern {

class XeenEventLoader {
public:
	using ResourceReader = std::function<std::optional<std::vector<std::uint8_t>>(
		const std::string &resourceName)>;

	using SideReader = std::function<std::optional<std::vector<std::uint8_t>>(const std::string &,XeenSide)>;
	explicit XeenEventLoader(SideReader);
	explicit XeenEventLoader(ResourceReader resourceReader);

	static std::string resourceNameForMap(XeenMapIdentity mapId);
	XeenEventFile load(XeenMapIdentity mapId) const;

private:
	SideReader _sideReader;
	ResourceReader _resourceReader;
};

} // namespace mmodern

#endif
