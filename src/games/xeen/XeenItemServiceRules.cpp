#include "games/xeen/XeenItemServiceRules.h"
#include "games/xeen/XeenNavigation.h"
#include "games/xeen/detail/XeenItemTables.h"
#include <algorithm>
namespace mmodern {
// Spells::castItemSpell, pinned ScummVM 6814ee9b. ScummVM contributors,
// upstream COPYRIGHT; GPL-3.0-or-later. Support comes from the shared spells.
std::optional<std::uint8_t> xeenItemPowerSpellId(std::uint8_t power) noexcept {
    // Power 33 shares Frostbite's effect; its projectile is selected by power.
    constexpr std::uint8_t spells[74]{76,42,1,45,26,27,20,61,59,10,62,60,38,72,55,50,
        40,2,4,73,41,75,3,34,43,31,52,49,44,53,30,33,74,28,15,25,5,9,0,70,17,
        7,69,24,8,29,51,14,66,12,13,6,23,57,22,71,64,56,21,11,48,46,54,19,36,
        32,58,18,47,37,65,35,63,16};
    if(!power || power>=std::size(spells) || spells[power]==76)return {};
    return spells[power];
}
std::optional<XeenLearnedSpell> xeenItemUseSpell(std::uint8_t power,bool combat) noexcept {
    const auto id=xeenItemPowerSpellId(power);
    // DOS admits item Enchant in combat, independently of learned admission.
    if(combat && power==63)return XeenLearnedSpell::EnchantItem;
    if(combat && power==55)return XeenLearnedSpell::TownPortal;
    return id?XeenLearnedSpellRules::supportedIn(*id,combat):std::nullopt;
}
bool xeenItemUseSupported(const XeenItem &item) noexcept {
    return xeenItemUseAdmission(XeenInventoryCategory::Miscellaneous,item,true,true)==XeenItemUseAdmission::Admitted &&
        (item.id==37 || xeenItemUseSpell(item.id).has_value());
}
std::optional<XeenSpellCost> xeenItemCancellationRefund(std::uint8_t spell,unsigned level) noexcept {
    // Only the cancellation routes established by M61, M75 and M76.
    switch(spell) {
    case 19:case 57:case 39:case 55:case 26:case 7:case 8:case 9:case 49:case 52:case 56:case 58:case 64:
        return XeenLearnedSpellRules::cost(spell,level);
    default:return {};
    }
}
std::optional<std::uint32_t> xeenItemPrice(XeenInventoryCategory category,const XeenItem &item,
        XeenItemService action,unsigned merchant) noexcept {
    using namespace item_tables;
    if(!item.id)return {};
    std::uint32_t value=0;
    if(category==XeenInventoryCategory::Miscellaneous) {
        if(!item.material || item.material>=std::size(miscMaterial) || item.id>=std::size(misc))return {};
        value=miscMaterial[item.material]+misc[item.id];
    } else {
        if(item.material>130)return {};
        if(category==XeenInventoryCategory::Weapons && item.id<std::size(weapons))value=weapons[item.id];
        else if(category==XeenInventoryCategory::Armor && item.id<std::size(armor))value=armor[item.id];
        else if(category==XeenInventoryCategory::Accessories && item.id<std::size(accessories))value=accessories[item.id];
        else return {};
        const auto m=item.material;
        if(m>=37 && m<=40)value/=m==37?10:m==39?2:4;
        else if(m>40 && m<59)value*=metal[m-37];
        if(m<37)value+=elemental[m]*100;
        // Calibrated DOS WORLD/XEEN.DAT 0x3c7fd/0x3c802, table 0x4f9e6.
        // The reference's ELEMENTAL_DAMAGE[m-52] is not the DOS table.
        else if(m>=59)value+=attributes[m-59]*100;
    }
    unsigned divisor=1;
    switch(action) {
    case XeenItemService::Buy:break;
    case XeenItemService::Sell:divisor=(merchant&0x7f)?1:2;break;
    case XeenItemService::Identify:divisor=100;break;
    case XeenItemService::Fix:divisor=10;break;
    default:return {};
    }
    return std::max(1u,value/divisor);
}
bool xeenDosMapZero(const XeenPartyState &party,const XeenCamera &camera) noexcept {
    return party.insideInn || camera.mapId.number==0;
}
XeenItemUseAdmission xeenItemUseAdmission(XeenInventoryCategory category,const XeenItem &item,bool world,bool canAct) noexcept {
    if(category!=XeenInventoryCategory::Miscellaneous)return XeenItemUseAdmission::InvalidCategory;
    if(!world)return XeenItemUseAdmission::NoWorld;
    if(!canAct)return XeenItemUseAdmission::CannotAct;
    if(!item.id || !item.material || (item.state&0xc0) || !(item.state&63))return XeenItemUseAdmission::NoAbility;
    return XeenItemUseAdmission::Admitted;
}
std::optional<XeenItemCategory> xeenDiscardItems(const XeenItemCategory &items,std::size_t slot,XeenInventoryCategory category) noexcept {
    if(slot>=items.size() || !items[slot].id || (items[slot].state&0x40) ||
        (category==XeenInventoryCategory::Weapons && items[slot].id==34))return {};
    auto after=items;after[slot]={};xeenCompactItems(after);return after;
}
}
