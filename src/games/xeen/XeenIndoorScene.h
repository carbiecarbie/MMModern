#ifndef MMODERN_GAMES_XEEN_INDOOR_SCENE_H
#define MMODERN_GAMES_XEEN_INDOOR_SCENE_H

#include "games/xeen/XeenNavigation.h"
#include "formats/xeen/XeenSpriteDrawOptions.h"

#include <array>
#include <cstddef>
#include <cstdint>
#include <optional>
#include <string>
#include <vector>

namespace mmodern {

class XeenWorld;

struct XeenIndoorWallSample {
	std::size_t queryIndex = 0;
	XeenMapIdentity sourceMapId = 0;
	int sourceX = 0;
	int sourceY = 0;
	XeenDirection sourceFace = XeenDirection::North;
	std::optional<std::uint8_t> wallValue;
};

struct XeenIndoorDrawCommand {
	int originalOrder = 0;
	std::string resourceName;
	std::size_t frame = 0;
	int x = 0;
	int y = 0;
	XeenMapIdentity sourceMapId = 0;
	int sourceX = -1;
	int sourceY = -1;
	XeenDirection sourceFace = XeenDirection::North;
	XeenSpriteDrawOptions options;
};

class XeenIndoorScene {
public:
	static constexpr std::size_t kWallSampleCount = 44;

	std::array<XeenIndoorWallSample, kWallSampleCount> sampleWalls(
		XeenWorld &world, const XeenCamera &camera) const;
	std::vector<XeenIndoorDrawCommand> build(XeenWorld &world,
		const XeenCamera &camera) const;
};

} // namespace mmodern

#endif
