#include "formats/xeen/XeenQuestFlagFormat.h"

#include <stdexcept>

namespace mmodern {
XeenQuestFlags XeenQuestFlagFormat::parseClouds(const std::vector<std::uint8_t> &bytes) {
	if (bytes.size() < kRequiredSize)
		throw std::runtime_error("maze.pty truncated before complete quest-flag field (747 bytes required)");
	XeenQuestFlags::Values values{};
	for (std::size_t i = 0; i < XeenQuestFlags::kCount; ++i)
		values[i] = (bytes[kOffset + i / 8] & (0x80u >> (i % 8))) != 0; // DOS 0x24f07: MSB first.
	return XeenQuestFlags(values);
}
XeenWorldFlags XeenQuestFlagFormat::parseWorldFlags(const std::vector<std::uint8_t> &bytes) {
	constexpr std::size_t offset = 659 + 32 + 32;
	if (bytes.size() < offset + 16)
		throw std::runtime_error("maze.pty truncated before complete world-flag block");
	XeenWorldFlags flags;
	for (std::size_t i=0;i<flags.kCount;++i) flags.values[i]=(bytes[offset+i/8] & (0x80u>>(i%8)))!=0;
	return flags;
}
}
