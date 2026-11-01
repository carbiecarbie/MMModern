#ifndef MMODERN_FORMATS_XEEN_XEEN_EVENT_FORMAT_H
#define MMODERN_FORMATS_XEEN_XEEN_EVENT_FORMAT_H

#include <cstddef>
#include <cstdint>
#include <optional>
#include <vector>

namespace mmodern {
struct XeenSaveMapState;

struct XeenEventRecord {
	std::size_t fileOffset = 0;
	std::uint8_t lengthField = 0;
	std::uint8_t x = 0;
	std::uint8_t y = 0;
	std::uint8_t direction = 0;
	std::uint8_t line = 0;
	std::uint8_t opcode = 0;
	std::vector<std::uint8_t> parameters;
	// DOS FallToMap reads beyond its operands into the following record length.
	// Absent at the end of the resource (dev-fall-damage-zero).
	std::optional<std::uint8_t> followingByte;

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
	static void writeDos(std::vector<std::uint8_t> &, const XeenSaveMapState &);
};

} // namespace mmodern

#endif
