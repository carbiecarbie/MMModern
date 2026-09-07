#include "formats/xeen/XeenQuestItemFormat.h"

#include <stdexcept>

namespace mmodern {

XeenCloudsQuestItems XeenQuestItemFormat::parseClouds(const std::vector<std::uint8_t> &bytes) {
	if (bytes.size() < kRequiredSize)
		throw std::runtime_error("maze.pty truncated before complete Clouds quest-item prefix (782 bytes required)");
	XeenCloudsQuestItems::Counts counts{};
	for (std::size_t i = 0; i < counts.size(); ++i)
		counts[i] = bytes[kCloudsOffset + i];
	return XeenCloudsQuestItems(counts);
}

} // namespace mmodern
