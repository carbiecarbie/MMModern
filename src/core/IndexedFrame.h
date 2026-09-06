#ifndef MMODERN_CORE_INDEXED_FRAME_H
#define MMODERN_CORE_INDEXED_FRAME_H

#include <array>
#include <cstdint>
#include <vector>

namespace mmodern {

struct IndexedFrame {
	static constexpr std::size_t kPaletteSize = 256 * 3;

	int width = 0;
	int height = 0;
	std::vector<std::uint8_t> pixels;
	std::array<std::uint8_t, kPaletteSize> palette{};

	bool isValid() const {
		return width > 0 && height > 0 &&
			pixels.size() == static_cast<std::size_t>(width) * height;
	}
};

} // namespace mmodern

#endif
