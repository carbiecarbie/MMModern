#include "games/xeen/XeenObjectVisual.h"
#include "formats/xeen/XeenAssetSource.h"
#include "games/xeen/XeenWorld.h"
#include <stdexcept>

namespace mmodern {
std::string xeenObjectSpriteName(int resourceId) {
	// MOB IDs are bytes; FF is the absent-resource sentinel.
	if (resourceId < 0 || resourceId >= 255)
		throw std::runtime_error("object sprite: invalid resource identifier");
	const auto digits = std::to_string(resourceId);
	return std::string(3 - digits.size(), '0') + digits + (resourceId < 100 ? ".obj" : ".0bj");
}

XeenObjectVisualResolver XeenObjectVisualResolver::load(XeenAssetSource &assets) {
	XeenObjectVisualResolver result;
	for(unsigned side=0;side<2;++side) {
 const auto bytes=assets.readObjectVisualMetadata(static_cast<XeenSide>(side));
 if(bytes)result._metadata[side]=XeenCloudsVisualMetadata::parse(*bytes);
 }
 result._diagnostic="Original object animation metadata is unavailable";
	return result;
}

XeenObjectVisual XeenObjectVisualResolver::resolve(const XeenObjectFile &objects,
		std::size_t recordIndex, XeenDirection cameraDirection,
		std::optional<std::uint64_t> ordinaryPhase, std::optional<std::uint8_t> tableIndex) const {
	XeenObjectVisual result;
	result.identity = {objects.mapId, recordIndex};
	if (!objects.mapId || !objects.resourcePresent || recordIndex >= objects.entities.objects.size()) {
		result.diagnostic = "object visual: invalid map, missing MOB, or original record index";
		return result;
	}
	const auto &metadata=_metadata.at(static_cast<unsigned>(static_cast<XeenSide>(objects.mapId.side)));
	const auto &object = objects.entities.objects[recordIndex];
	const auto camera = static_cast<unsigned>(cameraDirection);
	if (object.direction >= 4 || camera >= 4) {
		result.diagnostic = "object visual: invalid object/camera direction";
		return result;
	}
	try {
		// DOS draws from the index byte (per-index resource table -0xc16; indoor
		// 0x1577e..0x1578c, outdoor 0x17119..0x17127) and animates and flips from
		// the sprite byte (0x10d0e..0x10d21, 0x10d87..0x10db6). The chest rule sets
		// both to its open sprite; every other object keeps its record's sprite,
		// so the map 12 sprite rule stays logic-only.
		int sprite = object.resourceId;
		if (tableIndex && *tableIndex != object.tableIndex) {
			if (*tableIndex >= objects.entities.objectTable.size())
				throw std::runtime_error("object visual: invalid picture-table index");
			const unsigned id = objects.entities.objectTable[*tableIndex];
			sprite = id == 0xff ? -1 : int(id);
		}
		result.resourceId = sprite;
		result.spriteName = xeenObjectSpriteName(sprite);
		if (!metadata) {
			result.status = XeenObjectVisualStatus::MetadataUnavailable;
			result.diagnostic = _diagnostic;
			return result;
		}
		const auto &entry = metadata->at(static_cast<std::size_t>(sprite));
		// Pinned DIRECTION_ANIM_POSITIONS[object][camera]. Not the MOB table slot.
		const auto relative = (camera + 4 - object.direction) % 4;
		const std::uint64_t initial = entry.initialFrames[relative];
		const std::uint64_t limit = entry.frameLimits[relative];
		// DOS 0x10d21..0x10d55 keeps the incremented frame while end >= frame, so the end
		// frame belongs to the cycle (the pin resets at frame >= end). A range is static
		// precisely when end <= start.
		const auto cycleLength = (limit > initial) ? (limit - initial + 1) : std::uint64_t{1};
		result.frame = static_cast<std::size_t>(initial);
		result.initialFrame = static_cast<std::size_t>(initial);
		result.endFrame = static_cast<std::size_t>(limit);
		result.horizontalFlip = entry.flipFlags[relative] != 0;
		if (cycleLength == 1) {
			result.status = XeenObjectVisualStatus::SupportedStatic;
		} else if (ordinaryPhase) {
			result.frame = static_cast<std::size_t>(initial + (*ordinaryPhase % cycleLength));
			result.status = XeenObjectVisualStatus::SupportedAnimated;
		} else {
			result.status = XeenObjectVisualStatus::UnsupportedAnimation;
			result.diagnostic = "object visual requires unsupported temporal frame advancement";
		}
	} catch (const std::runtime_error &error) {
		result.diagnostic = error.what();
	}
	return result;
}

void xeenAdvanceObjectFrames(XeenWorld &world, const XeenCamera &camera, const XeenObjectVisualResolver &resolver) {
	const auto &file = world.objectFile(camera.mapId);
	world.scenePresentation().advanceObjectFrames(camera.mapId, [&](XeenObjectIdentity id) -> std::optional<XeenObjectFrameView> {
		if (!file.resourcePresent || id.recordIndex >= file.entities.objects.size()) return std::nullopt;
		const auto visual = resolver.resolve(file, id.recordIndex, camera.direction, std::nullopt,
			world.effectiveObject(id).tableIndex);
		using S = XeenObjectVisualStatus;
		if (visual.status != S::SupportedStatic && visual.status != S::SupportedAnimated &&
				visual.status != S::UnsupportedAnimation)
			return std::nullopt;
		return XeenObjectFrameView{visual.resourceId, unsigned(visual.initialFrame), unsigned(visual.endFrame)};
	});
}
} // namespace mmodern
