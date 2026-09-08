#include "formats/xeen/XeenQuestFlagFormat.h"

#include <stdexcept>

namespace mmodern {
XeenCloudsQuestFlags XeenQuestFlagFormat::parseClouds(const std::vector<std::uint8_t> &bytes) {
	if (bytes.size() < kRequiredSize)
		throw std::runtime_error("maze.pty truncated before complete quest-flag field (747 bytes required)");
	XeenCloudsQuestFlags::Values values{};
	for (std::size_t i = 0; i < values.size(); ++i)
		values[i] = (bytes[kOffset + i / 8] & (1u << (i % 8))) != 0;
	return XeenCloudsQuestFlags(values);
}
}
