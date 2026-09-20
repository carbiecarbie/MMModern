#ifndef MMODERN_CORE_INDEXED_FRAME_H
#define MMODERN_CORE_INDEXED_FRAME_H

#include <array>
#include <cstdint>
#include <vector>
#include <memory>
#include <utility>

namespace mmodern {

class XeenEventFlow;
struct IndexedFrame {
	// The retained immutable snapshot is both the uploaded content and its identity.
	using Presentation = std::shared_ptr<const IndexedFrame>;
	IndexedFrame() = default;
	IndexedFrame(int w, int h, std::vector<std::uint8_t> p) : width(w), height(h), pixels(std::move(p)) {}
	const Presentation &presentation() const noexcept { return _presentation; }
	static constexpr std::size_t kPaletteSize = 256 * 3;

	int width = 0;
	int height = 0;
	std::vector<std::uint8_t> pixels;
	std::array<std::uint8_t, kPaletteSize> palette{};

	bool isValid() const {
		return width > 0 && height > 0 &&
			pixels.size() == static_cast<std::size_t>(width) * height;
	}
private:
	friend class XeenEventFlow;
	Presentation _presentation;
};

} // namespace mmodern

#endif
