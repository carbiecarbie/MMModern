#ifndef MMODERN_GAMES_XEEN_XEEN_EVENT_TEXT_LOADER_H
#define MMODERN_GAMES_XEEN_XEEN_EVENT_TEXT_LOADER_H

#include "games/xeen/XeenMapIdentity.h"

#include <cstdint>
#include <functional>
#include <optional>
#include <string>
#include <vector>

namespace mmodern {

struct XeenEventTextFile {
	XeenMapIdentity mapId = 0;
	std::string resourceName;
	bool resourcePresent = false;
	std::vector<std::string> strings;

	const std::string *stringAt(std::size_t index) const;
};

class XeenEventTextLoader {
public:
	using ResourceReader = std::function<std::optional<std::vector<std::uint8_t>>(
		const std::string &resourceName)>;

	explicit XeenEventTextLoader(ResourceReader resourceReader);

	static std::string resourceNameForMap(XeenMapIdentity mapId);
	XeenEventTextFile load(XeenMapIdentity mapId) const;

private:
	ResourceReader _resourceReader;
};

} // namespace mmodern

#endif
