#ifndef MMODERN_GAMES_XEEN_XEEN_EVENT_TEXT_LOADER_H
#define MMODERN_GAMES_XEEN_XEEN_EVENT_TEXT_LOADER_H

#include "games/xeen/XeenMapIdentity.h"

#include <cstdint>
#include <functional>
#include <optional>
#include <string>
#include <vector>

namespace mmodern {

struct XeenMirrorDestination {
    std::string name;
    std::uint8_t map=0;
    int x=0,y=0,direction=-1;
    bool operator==(const XeenMirrorDestination &o) const {
        return name==o.name && map==o.map && x==o.x && y==o.y && direction==o.direction;
    }
};

struct XeenEventTextFile {
	XeenMapIdentity mapId = 0;
	std::string resourceName;
	bool resourcePresent = false;
	std::vector<std::string> strings;

	std::vector<XeenMirrorDestination> mirrors;
	const std::string *stringAt(std::size_t index) const;
};

class XeenEventTextLoader {
public:
	using ResourceReader = std::function<std::optional<std::vector<std::uint8_t>>(
		const std::string &resourceName)>;

	using SideReader = std::function<std::optional<std::vector<std::uint8_t>>(const std::string &,XeenSide)>;
	explicit XeenEventTextLoader(SideReader,ResourceReader mirrorReader={});
	explicit XeenEventTextLoader(ResourceReader resourceReader,ResourceReader mirrorReader={});

	static std::string resourceNameForMap(XeenMapIdentity mapId);
	static std::vector<XeenMirrorDestination> parseMirrors(const std::vector<std::uint8_t> &);
	XeenEventTextFile load(XeenMapIdentity mapId) const;

private:
 SideReader _sideReader;
	ResourceReader _resourceReader, _mirrorReader;
};

} // namespace mmodern

#endif
