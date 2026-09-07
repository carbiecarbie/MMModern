#ifndef MMODERN_XEEN_CLOUDS_VISUAL_METADATA_H
#define MMODERN_XEEN_CLOUDS_VISUAL_METADATA_H

#include <array>
#include <cstddef>
#include <cstdint>
#include <vector>

namespace mmodern {

struct XeenObjectVisualEntry {
	std::array<std::uint8_t, 4> initialFrames{}, flipFlags{}, frameLimits{};
};

// Immutable base table from DARK.CC/clouds.dat in the validated WoX layout.
class XeenCloudsVisualMetadata {
public:
	static constexpr std::size_t kEntryCount = 121, kEntrySize = 12;
	static XeenCloudsVisualMetadata parse(const std::vector<std::uint8_t> &bytes);
	const XeenObjectVisualEntry &at(std::size_t resourceId) const;
private:
	std::array<XeenObjectVisualEntry, kEntryCount> _entries{};
};

} // namespace mmodern
#endif
