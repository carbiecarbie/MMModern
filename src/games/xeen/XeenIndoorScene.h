#ifndef MMODERN_GAMES_XEEN_INDOOR_SCENE_H
#define MMODERN_GAMES_XEEN_INDOOR_SCENE_H

#include "games/xeen/XeenNavigation.h"
#include "games/xeen/XeenObjectVisual.h"
#include "formats/xeen/XeenSpriteDrawOptions.h"

#include <array>
#include <cstddef>
#include <cstdint>
#include <optional>
#include <string>
#include <variant>
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

struct XeenIndoorGeometryDraw {
	std::string resourceName;
	std::size_t frame = 0;
	XeenSpriteDrawOptions options;
};

struct XeenIndoorObjectDraw {
	XeenObjectVisual visual;
	int scaleIndex = 0;
	bool bottomClipped = false;
};

struct XeenIndoorDrawCommand {
	int originalOrder = 0;
	int x = 0;
	int y = 0;
	XeenMapIdentity sourceMapId = 0;
	int sourceX = -1;
	int sourceY = -1;
	XeenDirection sourceFace = XeenDirection::North;
	int queryIndex = -1;
	std::variant<XeenIndoorGeometryDraw, XeenIndoorObjectDraw> content;
	XeenIndoorGeometryDraw &geometry() { return std::get<XeenIndoorGeometryDraw>(content); }
	const XeenIndoorGeometryDraw &geometry() const {
		return std::get<XeenIndoorGeometryDraw>(content);
	}
	const XeenIndoorObjectDraw *object() const {
		return std::get_if<XeenIndoorObjectDraw>(&content);
	}
	XeenSpriteDrawOptions drawOptions() const {
		if (const auto *draw = object()) {
			XeenSpriteDrawOptions result;
			result.scaleIndex = draw->scaleIndex;
			result.horizontalFlip = draw->visual.horizontalFlip;
			result.sceneClipped = true;
			result.bottomClipped = draw->bottomClipped;
			return result;
		}
		return geometry().options;
	}
};

class XeenIndoorScene {
public:
	static constexpr std::size_t kWallSampleCount = 44;

	std::array<XeenIndoorWallSample, kWallSampleCount> sampleWalls(
		XeenWorld &world, const XeenCamera &camera) const;
	std::vector<XeenIndoorDrawCommand> build(XeenWorld &world,
		const XeenCamera &camera,
		const XeenObjectVisualResolver *resolver = nullptr,
		std::vector<XeenObjectVisual> *diagnostics = nullptr) const;
};

} // namespace mmodern

#endif
