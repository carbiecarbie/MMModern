#ifndef MMODERN_XEEN_OBJECT_SPRITE_SAFETY_H
#define MMODERN_XEEN_OBJECT_SPRITE_SAFETY_H
#include <cstddef>
#include <cstdint>
#include <vector>

namespace mmodern {
// Structural preflight for the normal M16 drawer only. No pixels are decoded.
// Validates the complete frame directory and the requested frame's row streams.
void validateXeenObjectSprite(const std::vector<std::uint8_t> &bytes, std::size_t frame, bool programRows = false);
// EXE normal drawer (0x1525a..0x152ba): rows outside its window are read but
// never drawn, and its unsigned row counter cannot wrap from this offset, so a
// program component with header Y at or beyond this value, or with zero height
// (offset 0 reads the directory as a header), publishes nothing.
constexpr std::size_t xeenProgramInvisibleRow = 0x4000;
inline bool xeenProgramComponentInvisible(const std::uint8_t *data, std::size_t size, std::size_t offset) {
	if (offset > size || size - offset < 8) return false;
	return (data[offset + 4] | (std::size_t(data[offset + 5]) << 8)) >= xeenProgramInvisibleRow || !(data[offset + 6] | data[offset + 7]);
}
} // namespace mmodern
#endif
