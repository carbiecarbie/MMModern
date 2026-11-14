#ifndef MMODERN_COMPAT_SCUMMVM_PICTURE_FILTER_LABEL_H
#define MMODERN_COMPAT_SCUMMVM_PICTURE_FILTER_LABEL_H

#include <cstdint>
#include <string_view>
#include <vector>

namespace mmodern {

// The F11 filter label: light text in ScummVM's built-in g_sysfont on a dark
// box, as an ARGB8888 bitmap; empty for empty text.
struct PictureFilterLabelImage {
	int width = 0, height = 0;
	std::vector<std::uint32_t> pixels;
};
PictureFilterLabelImage pictureFilterLabelImage(std::string_view text);

} // namespace mmodern

#endif
