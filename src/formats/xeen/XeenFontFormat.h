#ifndef MMODERN_FORMATS_XEEN_FONT_FORMAT_H
#define MMODERN_FORMATS_XEEN_FONT_FORMAT_H

#include <array>
#include <cstddef>
#include <cstdint>
#include <vector>

#include "formats/xeen/XeenLanguage.h"

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
	// M110: the Russian fnt adds a second bank of 128 glyphs and widths for
	// bytes 0x80-0xff, in the layout of the pinned font.cpp:48-57.
	static constexpr std::size_t kTwoBankSize = 0x2200;

	// English masks every byte to 7 bits; Russian requires the two-bank font
	// and draws bytes 0x80-0xff from its second bank.
	explicit XeenFontFormat(std::vector<std::uint8_t> bytes,
		XeenLanguage language = XeenLanguage::English);

	XeenFontGlyph glyph(std::uint8_t character, XeenFontSize size) const;
	std::uint8_t advance(std::uint8_t character, XeenFontSize size) const;
	XeenLanguage language() const { return _language; }

private:
	std::vector<std::uint8_t> _bytes;
	XeenLanguage _language;
};

} // namespace mmodern

#endif
