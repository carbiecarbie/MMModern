#include "formats/xeen/XeenGameFlagsFormat.h"

#include <stdexcept>
#include <string>

namespace mmodern {

XeenGameFlagsFormat::CloudsFlags XeenGameFlagsFormat::parseClouds(
		const std::vector<std::uint8_t> &partyBytes) {
	if (partyBytes.size() < kCloudsRequiredPartySize) {
		throw std::runtime_error("maze.pty possui " + std::to_string(partyBytes.size()) +
			" bytes; minimo necessario: " +
			std::to_string(kCloudsRequiredPartySize));
	}

	CloudsFlags result{};
	// Party::synchronize() serializes these through File::syncBitFlags():
	// each byte stores flags least-significant bit first.
	for (std::size_t index = 0; index < result.size(); ++index) {
		const std::uint8_t byte = partyBytes[kCloudsFlagsOffset + index / 8];
		result[index] = ((byte >> (index % 8)) & 1U) != 0;
	}
	return result;
}

} // namespace mmodern
