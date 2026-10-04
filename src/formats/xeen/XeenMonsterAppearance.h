#ifndef MMODERN_FORMATS_XEEN_MONSTER_APPEARANCE_H
#define MMODERN_FORMATS_XEEN_MONSTER_APPEARANCE_H
#include <cstdint>
#include "games/xeen/XeenRecordIdentity.h"
#include <optional>
#include <array>
#include <vector>
#include <stdexcept>
namespace mmodern {
enum class XeenMonsterSpriteKind { Normal, Attack };
struct XeenHitSplatDraw {unsigned frame=0;int damage=0;};
struct XeenProjectileAppearance {
 bool enemy=false;
 unsigned row=0,lane=0,distance=0;
 // Enemy source only; player lanes identify shooters without a fabricated target.
 std::optional<XeenMonsterIdentity> source;
 unsigned pow=11;
 bool active=true;
 std::optional<XeenMonsterIdentity> target;
 // One step for every lane, independent of whether other lanes terminate.
 void advance() noexcept {
  if(!active)return;
  if(enemy ? row==0 : row>=distance) active=false;
  else if(enemy)--row;else ++row;
 }
};
// Combat::MONSTER_SHOOT_POW, pinned ScummVM 6814ee9ba54582f5b5adcffab49efbbd8f589edd.
// GPL-3.0-or-later; ScummVM developers (upstream COPYRIGHT).
inline unsigned xeenMonsterProjectile(unsigned damageType) {
 constexpr unsigned pow[]{12,14,0,4,8,10,13};
 if(damageType>=7)throw std::invalid_argument("Unsupported monster projectile damage type");
 return pow[damageType];
}
inline unsigned xeenPortraitDamageFrame(unsigned damageType) {
 constexpr unsigned frames[]{0,6,1,2,3,4,5};
 if(damageType>=7)throw std::invalid_argument("Unsupported portrait damage type");
 return frames[damageType];
}
inline bool xeenSplatAlternatePosition(const std::array<std::optional<XeenMonsterIdentity>,3> &rows) {
 // drawIndoorsScene/drawOutdoorsScene test the raw record indexes, including
 // -1 for an absent entry. Preserve this literal test rather than counting actors.
 const int second=rows[1]?int(rows[1]->recordIndex):-1,third=rows[2]?int(rows[2]->recordIndex):-1;
 return second && !third;
}
// Disposable presentation value, without gameplay authority.
struct XeenMonsterAppearance {
	XeenMonsterSpriteKind kind = XeenMonsterSpriteKind::Normal;
	std::uint8_t frame = 0;
	// A special appearance belongs to this actor, never to a current row.
	// Absent identity retains the legacy single-monster presentation API.
	std::optional<XeenMonsterIdentity> identity;
	std::optional<XeenProjectileAppearance> projectile;
 std::vector<XeenProjectileAppearance> projectiles;
	XeenMonsterAppearance(std::uint8_t normalFrame = 0) : frame(normalFrame) {}
	XeenMonsterAppearance(XeenMonsterSpriteKind k, std::uint8_t f) : kind(k), frame(f) {}
	constexpr bool valid() const {
		return (kind == XeenMonsterSpriteKind::Normal && frame < 8) ||
			(kind == XeenMonsterSpriteKind::Attack && frame < 4);
	}
};
}
#endif
