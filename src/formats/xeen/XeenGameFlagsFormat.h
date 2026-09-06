#ifndef MMODERN_FORMATS_XEEN_XEEN_GAME_FLAGS_FORMAT_H
#define MMODERN_FORMATS_XEEN_XEEN_GAME_FLAGS_FORMAT_H

#include <array>
#include <cstddef>
#include <cstdint>
#include <vector>

namespace mmodern {

class XeenGameFlagsFormat {
public:
	// Derived from the original serialization order in:
	//   engines/mm/xeen/party.cpp, Party::synchronize() and
	//   BlacksmithWares::synchronize()
	//   engines/mm/xeen/item.cpp, XeenItem::synchronize()
	//   engines/mm/shared/xeen/file.cpp, File::syncBitFlags()
	// Before this block maze.pty has 28 bytes of party/configuration fields,
	// 576 bytes of Clouds shop inventory, 34 bytes of uint16 values, 20 bytes
	// of uint32 values, and one rested byte: 28 + 576 + 34 + 20 + 1 = 659.
	static constexpr std::size_t kCloudsFlagsOffset = 659;
	static constexpr std::size_t kCloudsFlagCount = 256;
	static constexpr std::size_t kCloudsFlagsSerializedSize = 32;
	static constexpr std::size_t kCloudsRequiredPartySize =
		kCloudsFlagsOffset + kCloudsFlagsSerializedSize;

	using CloudsFlags = std::array<bool, kCloudsFlagCount>;

	static CloudsFlags parseClouds(const std::vector<std::uint8_t> &partyBytes);
};

} // namespace mmodern

#endif
