#include "formats/xeen/XeenQuestItemFormat.h"

#include <stdexcept>

namespace mmodern {

XeenQuestItems XeenQuestItemFormat::parseClouds(const std::vector<std::uint8_t> &bytes) {
	if (bytes.size() < kRequiredSize)
		throw std::runtime_error("maze.pty truncated before complete Clouds quest-item prefix (782 bytes required)");
	XeenQuestItems::Counts counts{};
	for (std::size_t i = 0; i < 35; ++i)
		counts[i] = bytes[kCloudsOffset + i];
	return XeenQuestItems(counts);
}

} // namespace mmodern
