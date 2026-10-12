#include "games/xeen/XeenItemServiceRules.h"
#include "games/xeen/detail/XeenItemTables.h"
#include <algorithm>
namespace mmodern {
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
XeenItemUseAdmission xeenItemUseAdmission(XeenInventoryCategory category,const XeenItem &item,bool world,bool canAct) noexcept {
    if(category!=XeenInventoryCategory::Miscellaneous)return XeenItemUseAdmission::InvalidCategory;
    if(!world)return XeenItemUseAdmission::NoWorld;
    if(!canAct)return XeenItemUseAdmission::CannotAct;
    if(!item.id || !item.material || (item.state&0xc0) || !(item.state&63))return XeenItemUseAdmission::NoAbility;
    return XeenItemUseAdmission::Admitted;
}
std::optional<XeenItemCategory> xeenDiscardItems(const XeenItemCategory &items,std::size_t slot) noexcept {
    if(slot>=items.size() || !items[slot].id || (items[slot].state&0x40))return {};
    auto after=items;after[slot]={};xeenCompactItems(after);return after;
}
}
