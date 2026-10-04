#ifndef MMODERN_GAMES_XEEN_INDOOR_SCENE_H
#define MMODERN_GAMES_XEEN_INDOOR_SCENE_H

#include "games/xeen/XeenNavigation.h"
#include "games/xeen/XeenObjectVisual.h"
#include "formats/xeen/XeenSpriteDrawOptions.h"
#include "games/xeen/XeenActorApproach.h"
#include "formats/xeen/XeenMonsterAppearance.h"

#include <array>
#include <cstddef>
#include <cstdint>
#include <optional>
#include <string>
#include <variant>
#include <vector>

namespace mmodern {

class XeenWorld;

// Map::_sidePictures: Clouds art is also used by these World of Xeen mazes.
inline XeenSceneArchive xeenPictureArchive(XeenMapIdentity map) {
	return map.side==XeenSide::Clouds || (map.number>=125 && map.number<=127)
		? XeenSceneArchive::Clouds : XeenSceneArchive::Darkside;
}

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
	bool animated=false;
};

struct XeenIndoorObjectDraw {
	XeenObjectVisual visual;
	int scaleIndex = 0;
	bool bottomClipped = false;
};

struct XeenIndoorActorDraw {
	XeenMonsterIdentity identity;
	std::uint8_t image=0,frame=0;
	XeenMonsterSpriteKind kind=XeenMonsterSpriteKind::Normal;
	int selectedSlot=0,scaleIndex=0;
	unsigned effectFlags=0;
	std::uint32_t effectSeed=0;
	bool bottomClipped=false;
};

struct XeenIndoorProjectileDraw { bool enemy=false;unsigned row=0,lane=0,pow=11; };
struct XeenWallItemDraw {
	std::size_t recordIndex=0, frame=0;
	std::string resourceName;
	int scaleIndex=0;
	bool animated=false;
	XeenSceneArchive archive=XeenSceneArchive::Current;
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
	std::variant<XeenIndoorGeometryDraw, XeenIndoorObjectDraw, XeenIndoorActorDraw, XeenIndoorProjectileDraw, XeenWallItemDraw, XeenHitSplatDraw> content;
	XeenIndoorGeometryDraw &geometry() { return std::get<XeenIndoorGeometryDraw>(content); }
	const XeenIndoorGeometryDraw &geometry() const {
		return std::get<XeenIndoorGeometryDraw>(content);
	}
	const XeenIndoorObjectDraw *object() const {
		return std::get_if<XeenIndoorObjectDraw>(&content);
	}
	const XeenIndoorActorDraw *actor() const { return std::get_if<XeenIndoorActorDraw>(&content); }
	const XeenIndoorProjectileDraw *projectile() const { return std::get_if<XeenIndoorProjectileDraw>(&content); }
 const XeenHitSplatDraw *splat() const {return std::get_if<XeenHitSplatDraw>(&content);}
	const XeenWallItemDraw *wallItem() const { return std::get_if<XeenWallItemDraw>(&content); }
	XeenSpriteDrawOptions drawOptions() const {
        if(const auto *s=splat()) {XeenSpriteDrawOptions r;r.scaleIndex=s->damage<10?5:0;r.enlarge=s->damage>=100;r.sceneClipped=r.bottomClipped=true;return r;}
		if (const auto *wall=wallItem()) {XeenSpriteDrawOptions result;result.scaleIndex=wall->scaleIndex;result.sceneClipped=true;result.archive=wall->archive;return result;}
        if(const auto *p=projectile()){XeenSpriteDrawOptions result;result.sceneClipped=true;result.scaleIndex=4*p->row+(p->enemy?3:0);result.horizontalFlip=p->lane%2;return result;}
		if (const auto *draw=actor()) {
			XeenSpriteDrawOptions result;
			result.scaleIndex=draw->scaleIndex;result.sceneClipped=true;
			result.bottomClipped=draw->bottomClipped;result.monsterEffectFlags=draw->effectFlags;
			result.monsterEffectSeed=draw->effectSeed;
			return result;
		}
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
		XeenWorld &world, const XeenCamera &camera, bool sceneView = false) const;
	XeenActorView classifyActors(XeenWorld &world, const XeenCamera &camera,
		const std::vector<XeenActor> &actors) const;
	std::vector<XeenIndoorDrawCommand> buildActors(XeenWorld &world,
		const XeenCamera &camera, const std::vector<XeenActor> &actors,
		std::optional<std::uint64_t> ordinaryPhase = std::nullopt,
		std::optional<XeenMonsterAppearance> actorFrame = std::nullopt) const;
	std::vector<XeenIndoorDrawCommand> build(XeenWorld &world,
		const XeenCamera &camera,
		const XeenObjectVisualResolver *resolver = nullptr,
		std::vector<XeenObjectVisual> *diagnostics = nullptr,
		std::optional<std::uint64_t> ordinaryPhase = std::nullopt,
		std::optional<XeenMonsterAppearance> actorFrame = std::nullopt,
		bool night = false,
		std::function<std::size_t(const std::string &)> wallFrames = {}) const;
private:
	XeenActorView classifySceneActors(XeenWorld &,const XeenCamera &,
		const std::vector<XeenActor> &,bool sceneView) const;
};

} // namespace mmodern

#endif
