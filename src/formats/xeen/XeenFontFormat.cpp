#include "formats/xeen/XeenFontFormat.h"

#include <stdexcept>
#include <utility>

namespace mmodern {
namespace {

std::size_t glyphOffset(XeenFontSize size) {
	return size == XeenFontSize::Reduced ? 0x0800 : 0x0000;
}

std::size_t widthOffset(XeenFontSize size) {
	return size == XeenFontSize::Reduced ? 0x1080 : 0x1000;
}

} // namespace

XeenFontFormat::XeenFontFormat(std::vector<std::uint8_t> bytes) :
		_bytes(std::move(bytes)) {
	if (_bytes.size() < kMinimumSize)
		throw std::invalid_argument("recurso fnt de Xeen truncado");
}

XeenFontGlyph XeenFontFormat::glyph(std::uint8_t character,
		XeenFontSize size) const {
	character &= 0x7f;
	XeenFontGlyph result;
	result.advance = advance(character, size);
	const std::size_t base = glyphOffset(size) +
		static_cast<std::size_t>(character) * kGlyphBytes;
	for (int y = 0; y < XeenFontGlyph::kHeight; ++y) {
		std::uint16_t row = static_cast<std::uint16_t>(_bytes[base + y * 2]) |
			(static_cast<std::uint16_t>(_bytes[base + y * 2 + 1]) << 8);
		for (int x = 0; x < XeenFontGlyph::kWidth; ++x) {
			result.pixels[static_cast<std::size_t>(y) * XeenFontGlyph::kWidth + x] =
				static_cast<std::uint8_t>(row & 3);
			row >>= 2;
		}
	}
	return result;
}

std::uint8_t XeenFontFormat::advance(std::uint8_t character,
		XeenFontSize size) const {
	character &= 0x7f;
	if (character == ' ')
		return size == XeenFontSize::Reduced ? 3 : 4;
	return _bytes[widthOffset(size) + character];
}

} // namespace mmodern
