#include "formats/xeen/XeenObjectSpriteSafety.h"
#include <stdexcept>
#include <string>

namespace mmodern {
namespace {
[[noreturn]] void fail(const char *message) {
	throw std::runtime_error(std::string("M16 sprite preflight: ") + message);
}
std::size_t word(const std::vector<std::uint8_t> &b, std::size_t at) {
	if (at > b.size() || b.size() - at < 2) fail("truncated word/header");
	return b[at] | (static_cast<std::size_t>(b[at + 1]) << 8);
}
void cell(const std::vector<std::uint8_t> &b, std::size_t offset) {
	const auto x = word(b, offset), width = word(b, offset + 2);
	const auto y = word(b, offset + 4), height = word(b, offset + 6);
	// The upstream normal drawer uses a 320-pixel work line and int16 positions.
	// Reserve room for the bounded M16 draw anchor; enlargement is not supported.
	if (width > 320 || x + width > 32767 - 320 || y + height > 32767 - 200)
		fail("cell dimensions/offsets exceed normal drawer bounds");
	std::size_t p = offset + 8;
	for (std::size_t row = 0; row < height;) {
		if (p >= b.size()) fail("truncated row length");
		const auto length = b[p++];
		if (!length) {
			if (p >= b.size()) fail("truncated row skip");
			const std::size_t skip = b[p++] + 1;
			if (skip > height - row) fail("row skip exceeds cell height");
			row += skip;
			continue;
		}
		if (length > b.size() - p) fail("truncated compressed row");
		const auto end = p + length;
		std::size_t pixels = b[p++]; // Initial skipped pixels, not a color key.
		if (pixels > width) fail("row offset exceeds cell width");
		while (p < end) {
			const unsigned op = b[p++], command = op >> 5, len = op & 31;
			std::size_t operands = 0, advance = 0;
			// Operand sizes and line-pointer movement from pinned sprites.cpp.
			// Colors and pattern arithmetic deliberately remain in SpriteResource.
			switch (command) {
			case 0: case 1: operands = advance = op + 1; break;
			case 2: operands = 1; advance = len + 3; break;
			case 3: operands = 2; advance = len + 4; break;
			case 4: operands = 2; advance = (len + 2) * 2; break;
			case 5: advance = len + 1; break;
			case 6: case 7: operands = 1; advance = (op & 7) + 3; break;
			}
			if (operands > end - p) fail("compressed operand crosses row boundary");
			if (command == 3) {
				const auto distance = word(b, p), resume = p + 2;
				if (distance > resume || advance > b.size() - (resume - distance))
					fail("stream-copy source outside resource");
			}
			p += operands;
			if (advance > width - pixels) fail("compressed run exceeds cell width");
			pixels += advance;
		}
		++row;
	}
}
} // namespace

void validateXeenObjectSprite(const std::vector<std::uint8_t> &bytes, std::size_t frame) {
	if (bytes.empty()) fail("empty resource");
	const auto count = word(bytes, 0);
	if (!count) fail("empty frame directory");
	const auto directoryEnd = 2 + count * 4;
	if (directoryEnd > bytes.size()) fail("truncated frame directory");
	if (frame >= count) fail("requested frame out of range");
	for (std::size_t i = 0; i < count; ++i)
		for (std::size_t c = 0; c < 2; ++c) {
			const auto offset = word(bytes, 2 + i * 4 + c * 2);
			if (!offset && c == 1) continue;
			if (offset < directoryEnd || offset > bytes.size() || bytes.size() - offset < 8)
				fail("invalid cell offset/header");
			if (i == frame) cell(bytes, offset);
		}
}
} // namespace mmodern
