#ifndef MMODERN_FORMATS_XEEN_MONSTER_APPEARANCE_H
#define MMODERN_FORMATS_XEEN_MONSTER_APPEARANCE_H
#include <cstdint>
namespace mmodern {
enum class XeenMonsterSpriteKind { Normal, Attack };
// Disposable presentation value, without gameplay authority.
struct XeenMonsterAppearance {
	XeenMonsterSpriteKind kind = XeenMonsterSpriteKind::Normal;
	std::uint8_t frame = 0;
	constexpr XeenMonsterAppearance(std::uint8_t normalFrame = 0) : frame(normalFrame) {}
	constexpr XeenMonsterAppearance(XeenMonsterSpriteKind k, std::uint8_t f) : kind(k), frame(f) {}
	constexpr bool valid() const {
		return (kind == XeenMonsterSpriteKind::Normal && frame < 8) ||
			(kind == XeenMonsterSpriteKind::Attack && frame < 4);
	}
};
}
#endif
