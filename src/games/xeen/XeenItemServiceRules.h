#ifndef MMODERN_XEEN_ITEM_SERVICE_RULES_H
#define MMODERN_XEEN_ITEM_SERVICE_RULES_H
#include "games/xeen/XeenItemTransfer.h"
#include "games/xeen/XeenLearnedSpellRules.h"
#include <optional>
namespace mmodern {
enum class XeenItemService { Buy, Sell, Identify, Fix };
std::optional<std::uint32_t> xeenItemPrice(XeenInventoryCategory,const XeenItem &,
    XeenItemService,unsigned merchant=0) noexcept;
enum class XeenItemUseAdmission { Admitted, InvalidCategory, NoWorld, CannotAct, NoAbility };
XeenItemUseAdmission xeenItemUseAdmission(XeenInventoryCategory,const XeenItem &,bool world,bool canAct) noexcept;
// Original ordinary-spell destination; invalid and special handlers have no ID.
std::optional<std::uint8_t> xeenItemPowerSpellId(std::uint8_t power) noexcept;
std::optional<XeenLearnedSpell> xeenItemUseSpell(std::uint8_t power,bool combat=false) noexcept;
bool xeenItemUseSupported(const XeenItem &) noexcept;
std::optional<XeenSpellCost> xeenItemCancellationRefund(std::uint8_t spell,unsigned level) noexcept;
// DOS refuses cursed items and weapon ID exactly 34 before confirmation.
std::optional<XeenItemCategory> xeenDiscardItems(const XeenItemCategory &,std::size_t slot,XeenInventoryCategory) noexcept;
}
#endif
