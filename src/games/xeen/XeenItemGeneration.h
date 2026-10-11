#ifndef MMODERN_XEEN_ITEM_GENERATION_H
#define MMODERN_XEEN_ITEM_GENERATION_H
#include "games/xeen/XeenCombatRules.h"
#include <tuple>
namespace mmodern {
// Detached candidates have no publication or live-owner authority.
class XeenItemCandidate {
public:
	explicit XeenItemCandidate(unsigned level);
	bool service(XeenConsequenceDraw &);
	bool complete() const noexcept { return step==Step::Done; }
	unsigned category() const noexcept { return selectedCategory; }
	const XeenItem &item() const noexcept { return generated; }
friend bool operator==(const XeenItemCandidate &a,const XeenItemCandidate &b) noexcept {
        return std::tie(a.step,a.enchantment,a.level,a.categoryRoll,a.subcategory,a.selectedCategory,a.lo,a.hi,a.offset,a.generated.material,a.generated.id,a.generated.state,a.generated.frame)==
               std::tie(b.step,b.enchantment,b.level,b.categoryRoll,b.subcategory,b.selectedCategory,b.lo,b.hi,b.offset,b.generated.material,b.generated.id,b.generated.state,b.generated.frame);
    }
private:
	enum class Step { Category, Subcategory, Resolve, Id, Enchantment, Selector, Value, Effectiveness, EffectivenessValue, Special, Charges, Done };
	enum class Enchantment { None, Material, Element, Attribute, Usable };
	Step step=Step::Category;
	Enchantment enchantment=Enchantment::None;
	unsigned level,categoryRoll=0,subcategory=0,selectedCategory=0,lo=0,hi=0,offset=0;
	XeenItem generated;
};
}
#endif
