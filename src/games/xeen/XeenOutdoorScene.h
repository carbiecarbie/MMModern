#ifndef MMODERN_GAMES_XEEN_OUTDOOR_SCENE_H
#define MMODERN_GAMES_XEEN_OUTDOOR_SCENE_H

#include "formats/xeen/XeenSpriteDrawOptions.h"
#include "games/xeen/XeenNavigation.h"
#include "games/xeen/XeenObjectVisual.h"
#include "games/xeen/XeenActorApproach.h"

#include <cstddef>
#include <cstdint>
#include <optional>
#include <string>
#include <variant>
#include <vector>

namespace mmodern {

class XeenWorld;

struct XeenOutdoorTerrainDraw {
	std::string resourceName;
	std::size_t frame = 0;
	XeenSpriteDrawOptions options;
};

struct XeenOutdoorObjectDraw {
	XeenObjectVisual visual;
	int scaleIndex = 0;
	bool bottomClipped = false;
};

struct XeenOutdoorActorDraw {
	XeenMonsterIdentity identity;
	std::uint8_t image = 0, frame = 0;
	int selectedSlot = 0, scaleIndex = 0;
	bool bottomClipped = false;
};

struct XeenOutdoorDrawCommand {
	int originalOrder = 0;
	int x = 0;
	int y = 0;
	XeenMapIdentity sourceMapId = 0;
	int sourceX = -1;
	int sourceY = -1;
	int sampleIndex = -1;
	std::variant<XeenOutdoorTerrainDraw, XeenOutdoorObjectDraw, XeenOutdoorActorDraw> content;
	XeenOutdoorTerrainDraw &terrain() { return std::get<XeenOutdoorTerrainDraw>(content); }
	const XeenOutdoorTerrainDraw &terrain() const { return std::get<XeenOutdoorTerrainDraw>(content); }
	const XeenOutdoorObjectDraw *object() const { return std::get_if<XeenOutdoorObjectDraw>(&content); }
	const XeenOutdoorActorDraw *actor() const { return std::get_if<XeenOutdoorActorDraw>(&content); }
	XeenSpriteDrawOptions drawOptions() const {
		if (const auto *a = actor()) {
			XeenSpriteDrawOptions result;
			result.scaleIndex = a->scaleIndex;
			result.sceneClipped = true;
			result.bottomClipped = a->bottomClipped;
			return result;
		}
		if (const auto *o = object()) {
			XeenSpriteDrawOptions result;
			result.scaleIndex = o->scaleIndex;
			result.horizontalFlip = o->visual.horizontalFlip;
			result.sceneClipped = true;
			result.bottomClipped = o->bottomClipped;
			return result;
		}
		return terrain().options;
	}
};

class XeenOutdoorScene {
public:
	static constexpr XeenCamera kAreaA1Camera{1, 9, 6, XeenDirection::South};

	// A null resolver preserves terrain-only clients. Production composition
	// supplies 16A's resolver. Diagnostics retain skipped first-record results.
	std::vector<XeenOutdoorDrawCommand> build(XeenWorld &world,
		const XeenCamera &camera = kAreaA1Camera,
		const XeenObjectVisualResolver *resolver = nullptr,
		std::vector<XeenObjectVisual> *diagnostics = nullptr,
		std::optional<std::uint64_t> ordinaryPhase = std::nullopt,
		std::optional<std::uint8_t> actorFrame = std::nullopt) const;
	// Pure projection over owned observations; no activation or movement.
	static std::vector<XeenOutdoorDrawCommand> actorCommands(const std::vector<XeenActor> &,
		const XeenCamera &, std::uint8_t frame);
};

} // namespace mmodern

#endif
