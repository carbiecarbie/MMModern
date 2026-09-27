#ifndef MMODERN_XEEN_MERCHANT_GENERATION_H
#define MMODERN_XEEN_MERCHANT_GENERATION_H
#include "games/xeen/XeenServiceEconomy.h"
#include "games/xeen/XeenCombatRules.h"
#include <optional>
namespace mmodern {
// Detached candidates have no publication or live-owner authority.
class XeenMerchantItemCandidate {
public:
	explicit XeenMerchantItemCandidate(unsigned level);
	bool service(XeenConsequenceDraw &);
	bool complete() const noexcept { return step==Step::Done; }
	unsigned category() const noexcept { return selectedCategory; }
	const XeenItem &item() const noexcept { return generated; }
private:
	enum class Step { Category, Subcategory, Resolve, Id, Enchantment, Selector, Value, Effectiveness, EffectivenessValue, Special, Charges, Done };
	enum class Enchantment { None, Material, Element, Attribute, Usable };
	Step step=Step::Category;
	Enchantment enchantment=Enchantment::None;
	unsigned level,categoryRoll=0,subcategory=0,selectedCategory=0,lo=0,hi=0,offset=0;
	XeenItem generated;
};
class XeenMerchantStockCandidate {
public:
	XeenMerchantStockCandidate()=default;
	bool service(XeenConsequenceDraw &);
	bool complete() const noexcept { return side==2; }
	const XeenMerchantWares &wares() const noexcept { return generated; }
	unsigned generatedItems() const noexcept { return totalItems; }
	unsigned discardedItems() const noexcept { return discarded; }
private:
	XeenMerchantWares generated;
	std::array<unsigned,4> counts{};
	unsigned side=0,shop=0,band=0,ordinal=0,totalItems=0,discarded=0;
	std::optional<XeenMerchantItemCandidate> item;
};
}
#endif
