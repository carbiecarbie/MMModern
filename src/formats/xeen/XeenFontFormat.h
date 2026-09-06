#ifndef MMODERN_FORMATS_XEEN_FONT_FORMAT_H
#define MMODERN_FORMATS_XEEN_FONT_FORMAT_H

#include <array>
#include <cstddef>
#include <cstdint>
#include <vector>

namespace mmodern {

enum class XeenFontSize {
	Normal,
	Reduced
};

struct XeenFontGlyph {
	static constexpr int kWidth = 8;
	static constexpr int kHeight = 8;

	std::array<std::uint8_t, kWidth * kHeight> pixels{};
	std::uint8_t advance = 0;
};

class XeenFontFormat {
public:
	static constexpr std::size_t kGlyphCount = 128;
	static constexpr std::size_t kGlyphBytes = 16;
	static constexpr std::size_t kMinimumSize = 0x1100;

	explicit XeenFontFormat(std::vector<std::uint8_t> bytes);

	XeenFontGlyph glyph(std::uint8_t character, XeenFontSize size) const;
	std::uint8_t advance(std::uint8_t character, XeenFontSize size) const;

private:
	std::vector<std::uint8_t> _bytes;
};

} // namespace mmodern

#endif
