#ifndef MMODERN_GAMES_XEEN_OUTDOOR_SCENE_H
#define MMODERN_GAMES_XEEN_OUTDOOR_SCENE_H

#include "formats/xeen/XeenSpriteDrawOptions.h"
#include "games/xeen/XeenNavigation.h"

#include <cstddef>
#include <string>
#include <vector>

namespace mmodern {

class XeenWorld;

struct XeenOutdoorDrawCommand {
	int originalOrder = 0;
	std::string resourceName;
	std::size_t frame = 0;
	int x = 0;
	int y = 0;
	XeenMapIdentity sourceMapId = 0;
	int sourceX = -1;
	int sourceY = -1;
	XeenSpriteDrawOptions options;
};

class XeenOutdoorScene {
public:
	static constexpr XeenCamera kAreaA1Camera{1, 9, 6, XeenDirection::South};

	std::vector<XeenOutdoorDrawCommand> build(XeenWorld &world,
		const XeenCamera &camera = kAreaA1Camera) const;
};

} // namespace mmodern

#endif
