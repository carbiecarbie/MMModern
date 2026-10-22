#ifndef MMODERN_XEEN_FM_EFFECTS_H
#define MMODERN_XEEN_FM_EFFECTS_H
#include <optional>
#include <functional>
#include "games/xeen/XeenLearnedSpellRules.h"
namespace mmodern {
inline void xeenEmitFmEffect(const std::function<void(unsigned)> &sink,unsigned id) noexcept {
    if(sink)try {sink(id);}catch(...) {} // Presentation cannot abort a rule.
}
// DOS spell handlers request this before rangedAttack, after scalar/type draws.
// Ordinary impacts still use the retained attack-weapon selector (C1).
constexpr std::optional<unsigned> xeenSpellLaunchFmEffect(XeenLearnedSpell spell,unsigned type,unsigned itemPower=0) noexcept {
    using S=XeenLearnedSpell;
    switch(spell) {
    case S::FlyingFist:case S::Sleep:case S::Pain:case S::TurnUndead:case S::FingerOfDeath:
    case S::DancingSword:case S::PrismaticLight:case S::MassDistortion:case S::HolyWord:
    case S::Implosion:case S::BeastMaster:case S::DragonSleep:case S::Hypnotize:return 18;
    case S::Sparks:case S::LightningBolt:case S::MegaVolts:return 14;
    case S::EnergyBlast:case S::MoonRay:case S::SunRay:case S::GolemStopper:case S::Shrapmetal:return 16;
    case S::ToxicCloud:case S::AcidSpray:case S::InsectSpray:return 17;
    case S::Fireball:case S::DeadlySwarm:case S::FieryFlail:case S::Inferno:case S::StarBurst:return 13;
    case S::ColdRay:case S::FantasticFreeze:return 15;
    case S::Frostbite:return itemPower==33?std::optional<unsigned>{15}:std::nullopt;
    case S::Incinerate:return itemPower==64?13:22;
    case S::ElementalStorm:return type==2?13:type==3?14:type==4?15:17;
    case S::PoisonVolley:return 49;
    default:return {};
    }
}
// Member cancellation bypasses the cue, except Protection from Elements:
// its selector rejoins the FX20 tail even on Escape (DAT 0x2f6d4).
constexpr std::optional<unsigned> xeenSpellResultFmEffect(XeenLearnedSpell spell,bool failed,bool cancelled=false) noexcept {
    using S=XeenLearnedSpell;
    if(spell==S::ProtectionFromElements)return 20;
    if(failed || cancelled)return {};
    switch(spell) {
    case S::Light:return 39;
    case S::Awaken:case S::FirstAid:case S::Revitalize:case S::CureWounds:case S::NaturesCure:
    case S::CurePoison:case S::CureDisease:case S::CureParalysis:case S::StoneToFlesh:
    case S::RaiseDead:case S::Resurrection:case S::Bless:case S::HolyBonus:case S::Heroism:return 30;
    case S::SuppressPoison:case S::SuppressDisease:case S::Levitate:case S::WizardEye:
    case S::WalkOnWater:case S::PowerShield:case S::CreateFood:case S::DivineIntervention:
    case S::Clairvoyance:case S::DayOfProtection:case S::DayOfSorcery:return 20;
    default:return {};
    }
}
constexpr bool xeenTravelFmBeforeRelocation(XeenLearnedSpell spell) noexcept {
    return spell==XeenLearnedSpell::LloydsBeacon || spell==XeenLearnedSpell::TownPortal || spell==XeenLearnedSpell::SuperShelter;
}
constexpr std::optional<unsigned> xeenTravelFmEffect(XeenLearnedSpell spell,bool failed,bool destination,unsigned choice=0) noexcept {
    if(failed)return {};
    using S=XeenLearnedSpell;
    switch(spell) {
    case S::LloydsBeacon:if(choice==1)return 20;return choice==2 && destination?std::optional<unsigned>{51}:std::nullopt;
    case S::SuperShelter:return 30;
    case S::Jump:case S::TimeDistortion:case S::Teleport:case S::Etherealize:case S::TownPortal:
        return destination?std::optional<unsigned>{51}:std::nullopt;
    default:return {};
    }
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
