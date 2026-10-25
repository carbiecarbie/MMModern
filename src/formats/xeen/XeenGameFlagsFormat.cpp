#include "formats/xeen/XeenGameFlagsFormat.h"

#include <stdexcept>
#include <string>

namespace mmodern {

XeenGameFlagsFormat::CloudsFlags XeenGameFlagsFormat::parseClouds(
		const std::vector<std::uint8_t> &partyBytes) {
	if (partyBytes.size() < kCloudsRequiredPartySize) {
		throw std::runtime_error("maze.pty has " + std::to_string(partyBytes.size()) +
			" bytes; at least " + std::to_string(kCloudsRequiredPartySize) +
			" are required");
	}

	CloudsFlags result{};
	// DOS packs every flag bank most-significant bit first: the getter at
	// 0x24f07 tests byte[n >> 3] & (0x80 >> (n & 7)). The pinned reference's
	// File::syncBitFlags() is least-significant bit first.
	for (std::size_t index = 0; index < result.size(); ++index) {
		const std::uint8_t byte = partyBytes[kCloudsFlagsOffset + index / 8];
		result[index] = (byte & (0x80U >> (index % 8))) != 0;
	}
	return result;
}

} // namespace mmodern
