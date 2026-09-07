#include "games/xeen/XeenObjectVisual.h"
#include "formats/xeen/XeenAssetSource.h"
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
	const auto bytes = assets.readCloudsVisualMetadataFromDarkArchive();
	if (!bytes) result._diagnostic = "Clouds visual metadata unavailable: DARK.CC/clouds.dat is required";
	else result._metadata = XeenCloudsVisualMetadata::parse(*bytes);
	return result;
}

XeenObjectVisual XeenObjectVisualResolver::resolve(const XeenObjectFile &objects,
		std::size_t recordIndex, XeenDirection cameraDirection) const {
	XeenObjectVisual result;
	result.identity = {objects.mapId, recordIndex};
	if (!objects.mapId || !objects.resourcePresent || recordIndex >= objects.entities.objects.size()) {
		result.diagnostic = "object visual: invalid map, missing MOB, or original record index";
		return result;
	}
	if (objects.mapId.side != XeenSide::Clouds) {
		result.status = XeenObjectVisualStatus::UnsupportedSide;
		result.diagnostic = "object visuals support Clouds only";
		return result;
	}
	const auto &object = objects.entities.objects[recordIndex];
	const auto camera = static_cast<unsigned>(cameraDirection);
	if (object.direction >= 4 || camera >= 4) {
		result.diagnostic = "object visual: invalid object/camera direction";
		return result;
	}
	try {
		result.spriteName = xeenObjectSpriteName(object.resourceId);
		if (!_metadata) {
			result.status = XeenObjectVisualStatus::MetadataUnavailable;
			result.diagnostic = _diagnostic;
			return result;
		}
		const auto &entry = _metadata->at(static_cast<std::size_t>(object.resourceId));
		// Pinned DIRECTION_ANIM_POSITIONS[object][camera]. Not the MOB table slot.
		const auto relative = (camera + 4 - object.direction) % 4;
		result.frame = entry.initialFrames[relative];
		result.horizontalFlip = entry.flipFlags[relative] != 0;
		// drawScene increments then resets when frame >= limit. The base frame
		// remains unchanged precisely when initial + 1 >= limit (including 0).
		if (result.frame + 1 < entry.frameLimits[relative]) {
			result.status = XeenObjectVisualStatus::UnsupportedAnimation;
			result.diagnostic = "object visual requires unsupported temporal frame advancement";
		} else result.status = XeenObjectVisualStatus::SupportedStatic;
	} catch (const std::runtime_error &error) {
		result.diagnostic = error.what();
	}
	return result;
}
} // namespace mmodern
