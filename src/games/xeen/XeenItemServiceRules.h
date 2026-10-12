#ifndef MMODERN_XEEN_ITEM_SERVICE_RULES_H
#define MMODERN_XEEN_ITEM_SERVICE_RULES_H
#include "games/xeen/XeenItemTransfer.h"
#include <optional>
namespace mmodern {
enum class XeenItemService { Buy, Sell, Identify, Fix };
std::optional<std::uint32_t> xeenItemPrice(XeenInventoryCategory,const XeenItem &,
    XeenItemService,unsigned merchant=0) noexcept;
enum class XeenItemUseAdmission { Admitted, InvalidCategory, NoWorld, CannotAct, NoAbility };
XeenItemUseAdmission xeenItemUseAdmission(XeenInventoryCategory,const XeenItem &,bool world,bool canAct) noexcept;
// Detached category delta. Curse refusal follows ScummVM; not confirmed in DOS.
std::optional<XeenItemCategory> xeenDiscardItems(const XeenItemCategory &,std::size_t slot) noexcept;
}
#endif
