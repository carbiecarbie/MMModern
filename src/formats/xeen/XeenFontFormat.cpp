#include "formats/xeen/XeenFontFormat.h"

#include <stdexcept>
#include <utility>

namespace mmodern {
namespace {

std::size_t glyphOffset(XeenFontSize size, XeenLanguage language, bool high) {
	if (language == XeenLanguage::Russian)
		return (size == XeenFontSize::Reduced ? 0x1000 : 0x0000) + (high ? 0x0800 : 0);
	return size == XeenFontSize::Reduced ? 0x0800 : 0x0000;
}

std::size_t widthOffset(XeenFontSize size, XeenLanguage language, bool high) {
	if (language == XeenLanguage::Russian)
		return (size == XeenFontSize::Reduced ? 0x2100 : 0x2000) + (high ? 0x0080 : 0);
	return size == XeenFontSize::Reduced ? 0x1080 : 0x1000;
}

} // namespace

XeenFontFormat::XeenFontFormat(std::vector<std::uint8_t> bytes, XeenLanguage language) :
		_bytes(std::move(bytes)), _language(language) {
	if (_bytes.size() < kMinimumSize)
		throw std::invalid_argument("truncated Xeen fnt resource");
	if (_language == XeenLanguage::Russian && _bytes.size() != kTwoBankSize)
		throw std::invalid_argument("Russian text requires the two-bank Xeen fnt resource");
}

XeenFontGlyph XeenFontFormat::glyph(std::uint8_t character,
		XeenFontSize size) const {
	XeenFontGlyph result;
	result.advance = advance(character, size);
	const bool high = _language == XeenLanguage::Russian && character >= 0x80;
	character &= 0x7f;
	const std::size_t base = glyphOffset(size, _language, high) +
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
	// Russian: only 0x20 is a space (pinned FontSurface::isSpace).
	const bool high = _language == XeenLanguage::Russian && character >= 0x80;
	character &= 0x7f;
	if (character == ' ' && !high)
		return size == XeenFontSize::Reduced ? 3 : 4;
	return _bytes[widthOffset(size, _language, high) + character];
}

} // namespace mmodern
