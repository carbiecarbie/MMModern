#ifndef MMODERN_GAMES_XEEN_XEEN_TEXT_RENDERER_H
#define MMODERN_GAMES_XEEN_XEEN_TEXT_RENDERER_H

#include "core/IndexedFrame.h"
#include "formats/xeen/XeenFontFormat.h"

#include <cstdint>
#include <string>
#include <string_view>
#include <vector>

namespace mmodern {

// M110 decision 10: a player-entered name (save slot, character) is literal
// text in both languages. xeenLiteralText prefixes each byte outside
// 0x20-0x7e with this private marker twice, a pair no original text contains
// (a single 0x1f stays an unsupported control); the renderer then draws that
// byte as a glyph and never reads it as formatting. In English the glyph is
// the one of its low 7 bits, in Russian its own.
inline constexpr char kXeenLiteralMarker = '\x1f';
std::string xeenLiteralText(std::string_view bytes);
inline std::string xeenLiteralText(const std::string &bytes) { return xeenLiteralText(std::string_view(bytes)); }
inline std::string xeenLiteralText(const char *bytes) { return xeenLiteralText(std::string_view(bytes)); }
// Characters a text shows: a marked byte counts once.
std::size_t xeenVisibleLength(std::string_view text);

struct XeenTextRect {
	int left = 0;
	int top = 0;
	int right = 0;
	int bottom = 0;
};

enum class XeenTextAlignment {
	Left,
	Center,
	Right
};

struct XeenTextRenderOptions {
	XeenTextRect bounds;
	int x = 0;
	int y = 0;
	int alignmentAnchor = -1;
	XeenFontSize size = XeenFontSize::Normal;
	XeenTextAlignment alignment = XeenTextAlignment::Left;
	std::uint8_t colorIndex = 0;
	bool drawWindow = false;
	XeenTextRect windowBounds;
	bool paginate = false;
	// Original dialog templates use absolute cursor controls and four-shade font
	// colors. Existing flowing Event/service text retains its established layout.
	bool originalControls = false;
	// FontSurface::setTextColor uses the original startup table at the title.
	bool startupColors = false;
 // XEEN.EXE 0x18813..0x18818 uses SHR/ADC for centered runs, and its
 // trailing-space deduction is one less than the DAT (0x187c4..0x187d4).
 bool exeCenterRounding = false;
 // Original Window::writeString may return the remaining text at its bottom.
 bool stopAtBottom = false;
};

struct XeenTextRenderResult {
 int writeX = 0, writeY = 0;
 XeenFontSize writeSize = XeenFontSize::Normal;
 std::uint8_t writeColor = 0;
 XeenTextAlignment writeAlignment = XeenTextAlignment::Left;
	std::vector<IndexedFrame> pages;
	std::vector<std::string> diagnostics;
	// Exclusive raw source end for each page, including intervening whitespace.
	std::vector<std::size_t> pageSourceEnds;
};

class XeenTextRenderer {
public:
	explicit XeenTextRenderer(const XeenFontFormat &font);

	XeenTextRenderResult render(const IndexedFrame &base, const std::string &text,
		const XeenTextRenderOptions &options) const;
	int textWidth(const std::string &text, XeenFontSize size,
		std::vector<std::string> *diagnostics = nullptr) const;

private:
	const XeenFontFormat &_font;
};

} // namespace mmodern

#endif
