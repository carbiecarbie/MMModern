#ifndef MMODERN_FORMATS_XEEN_XEEN_EVENT_TEXT_FORMAT_H
#define MMODERN_FORMATS_XEEN_XEEN_EVENT_TEXT_FORMAT_H

#include <cstdint>
#include <string>
#include <vector>

namespace mmodern {

class XeenEventTextFormat {
public:
	// Xeen event text is an uncounted sequence of NUL-terminated byte strings.
	// Strings are deliberately kept byte-for-byte: event text contains original
	// font and layout control bytes which are interpreted by a later stage.
	static std::vector<std::string> parse(const std::vector<std::uint8_t> &bytes);
};

} // namespace mmodern

#endif
