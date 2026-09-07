#include "formats/xeen/XeenCloudsVisualMetadata.h"
#include <stdexcept>

namespace mmodern {
XeenCloudsVisualMetadata XeenCloudsVisualMetadata::parse(const std::vector<std::uint8_t> &bytes) {
	if (bytes.empty()) throw std::runtime_error("clouds.dat: empty metadata");
	if (bytes.size() < kEntryCount * kEntrySize)
		throw std::runtime_error("clouds.dat: truncated metadata table (expected 1452 bytes)");
	if (bytes.size() > kEntryCount * kEntrySize)
		throw std::runtime_error("clouds.dat: unexpected trailing metadata bytes");
	XeenCloudsVisualMetadata result;
	for (std::size_t i = 0; i < kEntryCount; ++i)
		for (std::size_t d = 0; d < 4; ++d) {
			result._entries[i].initialFrames[d] = bytes[i * 12 + d];
			result._entries[i].flipFlags[d] = bytes[i * 12 + 4 + d];
			result._entries[i].frameLimits[d] = bytes[i * 12 + 8 + d];
		}
	return result;
}

const XeenObjectVisualEntry &XeenCloudsVisualMetadata::at(std::size_t resourceId) const {
	if (resourceId >= kEntryCount) throw std::runtime_error("clouds.dat: resource index out of range");
	return _entries[resourceId];
}
} // namespace mmodern
