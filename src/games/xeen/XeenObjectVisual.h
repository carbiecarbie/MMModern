#ifndef MMODERN_XEEN_OBJECT_VISUAL_H
#define MMODERN_XEEN_OBJECT_VISUAL_H

#include "formats/xeen/XeenCloudsVisualMetadata.h"
#include "games/xeen/XeenMap.h"
#include "games/xeen/XeenRecordIdentity.h"
#include <optional>
#include <cstdint>
#include <utility>

namespace mmodern {
class XeenAssetSource;

enum class XeenObjectVisualStatus { SupportedStatic, UnsupportedAnimation, MetadataUnavailable, Invalid, UnsupportedSide, SupportedAnimated };

struct XeenObjectVisual {
	XeenObjectIdentity identity;
	std::string spriteName;
	int resourceId = -1; // The sprite drawn: the record's, or its table index's.
	std::size_t frame = 0;
	// The view's start and end frames (clouds.dat), for the framed-object update.
	std::size_t initialFrame = 0, endFrame = 0;
	bool horizontalFlip = false;
	XeenObjectVisualStatus status = XeenObjectVisualStatus::Invalid;
	std::string diagnostic;
};

std::string xeenObjectSpriteName(int resourceId);

// No session state, visibility, projection, or animation clock is retained.
class XeenObjectVisualResolver {
public:
	explicit XeenObjectVisualResolver(XeenCloudsVisualMetadata metadata) : _metadata{std::move(metadata),std::nullopt} {}
	static XeenObjectVisualResolver load(XeenAssetSource &assets);
	// `tableIndex` is the object's effective picture-table index; one that
	// differs from the record's own names the picture, frames and flip.
	XeenObjectVisual resolve(const XeenObjectFile &objects, std::size_t recordIndex,
		XeenDirection cameraDirection,
		std::optional<std::uint64_t> ordinaryPhase = std::nullopt,
		std::optional<std::uint8_t> tableIndex = std::nullopt) const;
private:
	XeenObjectVisualResolver() = default;
	std::array<std::optional<XeenCloudsVisualMetadata>,2> _metadata;
	std::string _diagnostic;
};

class XeenWorld;
// draw3d's object update before the composition (DOS 0x10c32..0x10dc7) for the
// world's framed objects, with each one's view for the camera's facing.
void xeenAdvanceObjectFrames(XeenWorld &world, const XeenCamera &camera, const XeenObjectVisualResolver &resolver);
} // namespace mmodern
#endif
