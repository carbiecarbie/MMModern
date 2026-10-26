#ifndef MMODERN_XEEN_COMPRESSED_FRAME_H
#define MMODERN_XEEN_COMPRESSED_FRAME_H
#include <cstdint>
#include <vector>
namespace mmodern {
// XEEN.EXE 0x18c70 -> 0x1cd42: adaptive Huffman/LZ full-screen resource.
// The last word may contain only its high byte; missing bits are never usable.
std::vector<std::uint8_t> decodeXeenCompressedFrame(const std::vector<std::uint8_t> &);
}
#endif
