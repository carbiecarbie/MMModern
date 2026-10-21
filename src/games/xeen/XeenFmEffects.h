#ifndef MMODERN_XEEN_FM_EFFECTS_H
#define MMODERN_XEEN_FM_EFFECTS_H
#include <optional>
#include <functional>
namespace mmodern {
inline void xeenEmitFmEffect(const std::function<void(unsigned)> &sink,unsigned id) noexcept {
    if(sink)try {sink(id);}catch(...) {} // Presentation cannot abort a rule.
}
// Original giveCharDamage/doCharDamage selectors (DAT 0x23e18, 0x2d2ab).
constexpr unsigned xeenDamageFmEffect(unsigned type) noexcept {
    constexpr unsigned ids[]{29,27,22,23,24,26,25};
    return type<7?ids[type]:38;
}
// Successful monster secondary attacks. Stone deliberately uses 37 in DOS.
constexpr std::optional<unsigned> xeenSecondaryFmEffect(unsigned special) noexcept {
    switch(special) {
    case 5:case 7:return 26;
    case 8:case 16:return 28;
    case 9:case 18:return 36;
    case 10:case 12:case 13:case 14:case 15:case 17:case 19:case 20:case 22:return 37;
    case 21:return 38;
    default:return {};
    }
}
// rangedAttack checks the next wall before testing whether any missile remains.
constexpr bool xeenMissileReachesFmWall(unsigned wallRow,bool remaining,unsigned lastHitRow) noexcept {
    return wallRow<4 && (remaining || lastHitRow+1==wallRow);
}
constexpr unsigned xeenRangedFmEffect(unsigned type) noexcept {
    constexpr unsigned ids[]{49,18,13,14,15,17,16};
    return type<7?ids[type]:49;
}
}
#endif
