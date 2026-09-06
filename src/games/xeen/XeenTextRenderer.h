#ifndef MMODERN_GAMES_XEEN_XEEN_TEXT_RENDERER_H
#define MMODERN_GAMES_XEEN_XEEN_TEXT_RENDERER_H

#include "core/IndexedFrame.h"
#include "formats/xeen/XeenFontFormat.h"

#include <cstdint>
#include <string>
#include <vector>

namespace mmodern {

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
};

struct XeenTextRenderResult {
	std::vector<IndexedFrame> pages;
	std::vector<std::string> diagnostics;
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
