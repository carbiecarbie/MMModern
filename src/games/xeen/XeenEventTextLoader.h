#ifndef MMODERN_GAMES_XEEN_XEEN_EVENT_TEXT_LOADER_H
#define MMODERN_GAMES_XEEN_XEEN_EVENT_TEXT_LOADER_H

#include <cstdint>
#include <functional>
#include <optional>
#include <string>
#include <vector>

namespace mmodern {

struct XeenEventTextFile {
	std::uint16_t mapId = 0;
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

	static std::string resourceNameForMap(std::uint16_t mapId);
	XeenEventTextFile load(std::uint16_t mapId) const;

private:
	ResourceReader _resourceReader;
};

} // namespace mmodern

#endif
