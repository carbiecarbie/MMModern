#define FORBIDDEN_SYMBOL_ALLOW_ALL
#include "compat/scummvm/PictureFilterLabel.h"
#include "graphics/fonts/bdf.h"
#include "graphics/pixelformat.h"
#include "graphics/surface.h"

namespace Graphics {
FORWARD_DECLARE_FONT(g_sysfont);
}

namespace mmodern {

PictureFilterLabelImage pictureFilterLabelImage(std::string_view text) {
	PictureFilterLabelImage image;
	if (text.empty()) return image;
	if (!Graphics::g_sysfont) Graphics::create_g_sysfont();
	const Graphics::Font &font = *Graphics::g_sysfont;
	constexpr int padding = 2;
	constexpr std::uint32_t box = 0xe0101010u, ink = 0xfff0f0f0u;
	int textWidth = 0;
	for (const unsigned char c : text) textWidth += font.getCharWidth(c);
	image.width = textWidth + 2 * padding;
	image.height = font.getFontHeight() + 2 * padding;
	image.pixels.assign(static_cast<std::size_t>(image.width) * image.height, box);
	Graphics::Surface surface;
	surface.init(image.width, image.height, image.width * 4, image.pixels.data(),
		Graphics::PixelFormat(4, 8, 8, 8, 8, 16, 8, 0, 24));
	int x = padding;
	for (const unsigned char c : text) {
		font.drawChar(&surface, c, x, padding, ink);
		x += font.getCharWidth(c);
	}
	return image;
}

} // namespace mmodern
