#ifndef MMODERN_XEEN_OBJECT_SPRITE_SAFETY_H
#define MMODERN_XEEN_OBJECT_SPRITE_SAFETY_H
#include <cstddef>
#include <cstdint>
#include <vector>

namespace mmodern {
// Structural preflight for the normal M16 drawer only. No pixels are decoded.
// Validates the complete frame directory and the requested frame's row streams.
void validateXeenObjectSprite(const std::vector<std::uint8_t> &bytes, std::size_t frame);
} // namespace mmodern
#endif
