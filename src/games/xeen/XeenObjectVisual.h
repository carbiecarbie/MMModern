#ifndef MMODERN_XEEN_OBJECT_VISUAL_H
#define MMODERN_XEEN_OBJECT_VISUAL_H

#include "formats/xeen/XeenCloudsVisualMetadata.h"
#include "games/xeen/XeenMap.h"
#include "games/xeen/XeenRecordIdentity.h"
#include <optional>
#include <utility>

namespace mmodern {
class XeenAssetSource;

enum class XeenObjectVisualStatus { SupportedStatic, UnsupportedAnimation, MetadataUnavailable, Invalid, UnsupportedSide };

struct XeenObjectVisual {
	XeenObjectIdentity identity;
	std::string spriteName;
	std::size_t frame = 0;
	bool horizontalFlip = false;
	XeenObjectVisualStatus status = XeenObjectVisualStatus::Invalid;
	std::string diagnostic;
};

std::string xeenObjectSpriteName(int resourceId);

// No session state, visibility, projection, or animation clock is retained.
class XeenObjectVisualResolver {
public:
	explicit XeenObjectVisualResolver(XeenCloudsVisualMetadata metadata) : _metadata(std::move(metadata)) {}
	static XeenObjectVisualResolver load(XeenAssetSource &assets);
	XeenObjectVisual resolve(const XeenObjectFile &objects, std::size_t recordIndex,
		XeenDirection cameraDirection) const;
private:
	XeenObjectVisualResolver() = default;
	std::optional<XeenCloudsVisualMetadata> _metadata;
	std::string _diagnostic;
};
} // namespace mmodern
#endif
