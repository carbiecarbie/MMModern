#ifndef MMODERN_FORMATS_XEEN_MONSTER_APPEARANCE_H
#define MMODERN_FORMATS_XEEN_MONSTER_APPEARANCE_H
#include <cstdint>
#include "games/xeen/XeenRecordIdentity.h"
#include <optional>
#include <array>
namespace mmodern {
enum class XeenMonsterSpriteKind { Normal, Attack };
struct XeenProjectileAppearance {
 bool enemy=false;
 unsigned row=0,lane=0,distance=0;
 // Enemy source only; player lanes identify shooters without a fabricated target.
 std::optional<XeenMonsterIdentity> source;
};
// Disposable presentation value, without gameplay authority.
struct XeenMonsterAppearance {
	XeenMonsterSpriteKind kind = XeenMonsterSpriteKind::Normal;
	std::uint8_t frame = 0;
	// A special appearance belongs to this actor, never to a current row.
	// Absent identity retains the legacy single-monster presentation API.
	std::optional<XeenMonsterIdentity> identity;
	std::optional<XeenProjectileAppearance> projectile;
	constexpr XeenMonsterAppearance(std::uint8_t normalFrame = 0) : frame(normalFrame) {}
	constexpr XeenMonsterAppearance(XeenMonsterSpriteKind k, std::uint8_t f) : kind(k), frame(f) {}
	constexpr bool valid() const {
		return (kind == XeenMonsterSpriteKind::Normal && frame < 8) ||
			(kind == XeenMonsterSpriteKind::Attack && frame < 4);
	}
};
}
#endif
