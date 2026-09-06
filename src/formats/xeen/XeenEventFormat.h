#ifndef MMODERN_FORMATS_XEEN_XEEN_EVENT_FORMAT_H
#define MMODERN_FORMATS_XEEN_XEEN_EVENT_FORMAT_H

#include <cstddef>
#include <cstdint>
#include <vector>

namespace mmodern {

struct XeenEventRecord {
	std::size_t fileOffset = 0;
	std::uint8_t lengthField = 0;
	std::uint8_t x = 0;
	std::uint8_t y = 0;
	std::uint8_t direction = 0;
	std::uint8_t line = 0;
	std::uint8_t opcode = 0;
	std::vector<std::uint8_t> parameters;

	std::size_t totalSerializedSize() const {
		return static_cast<std::size_t>(lengthField) + 1;
	}
};

// Parses decoded maze*.evt bytes only. Opcode semantics, directions, and the
// distinction between physical coordinates and logical call targets belong to
// the game layer.
class XeenEventFormat {
public:
	static std::vector<XeenEventRecord> parse(const std::vector<std::uint8_t> &bytes);
};

} // namespace mmodern

#endif
