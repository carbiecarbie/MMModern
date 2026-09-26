#ifndef MMODERN_XEEN_ARMOR_REPAIR_H
#define MMODERN_XEEN_ARMOR_REPAIR_H
#include "games/xeen/XeenItemTransfer.h"
#include "games/xeen/XeenGameplayContext.h"
#include <algorithm>
#include <optional>
namespace mmodern {
enum class XeenSmithBoundary { BeforeAdmission, AfterAdmission, Quote, BeforeRepair, AfterRepair, BeforeDeparture, AfterDeparture, Return };
enum class XeenArmorRepairOutcome { Quoted, Repaired, Empty, Unsupported, Intact, InsufficientGold, Cancelled };
struct XeenArmorRepairCandidate {
	XeenArmorRepairOutcome outcome=XeenArmorRepairOutcome::Unsupported;
	XeenItem before{}, after{};
	std::uint32_t price=0, goldBefore=0, goldAfter=0;
};
// Pure detached values. Publication and owner/frame authority belong to Flow.
// Armor costs and divisor adapted from ScummVM developers' GPL-3.0-or-later
// constants.cpp / ItemsDialog::calcItemCost, revision
// 6814ee9ba54582f5b5adcffab49efbbd8f589edd.
inline XeenArmorRepairCandidate xeenQuoteArmorRepair(XeenInventoryCategory category,
		const XeenItem &item, std::uint32_t gold) noexcept {
	XeenArmorRepairCandidate result;
	result.before=item;result.after=item;result.goldBefore=result.goldAfter=gold;
	if (category!=XeenInventoryCategory::Armor) return result;
	if (!item.id) { result.outcome=XeenArmorRepairOutcome::Empty;return result; }
	if (item.id>13 || (item.material!=0 && item.material!=38)) return result;
	if (!(item.state&0x80)) { result.outcome=XeenArmorRepairOutcome::Intact;return result; }
	static constexpr std::array<std::uint32_t,13> costs{{20,100,200,400,600,1000,2000,100,60,40,250,200,100}};
	const std::uint64_t base=costs[item.id-1];
	result.price=static_cast<std::uint32_t>(std::max<std::uint64_t>(1,(item.material==38?base/4:base)/10));
	result.outcome=XeenArmorRepairOutcome::Quoted;
	return result;
}
inline XeenArmorRepairCandidate xeenPrepareArmorRepair(const XeenItem &item,std::uint32_t gold) noexcept {
	auto result=xeenQuoteArmorRepair(XeenInventoryCategory::Armor,item,gold);
	if (result.outcome!=XeenArmorRepairOutcome::Quoted) return result;
	if (gold<result.price) {result.outcome=XeenArmorRepairOutcome::InsufficientGold;return result;}
	result.after.state=item.state&0x7f;result.goldAfter=gold-result.price;
	result.outcome=XeenArmorRepairOutcome::Repaired;
	return result;
}
inline std::optional<XeenGameplayContext> xeenPrepareSmithDeparture(
		const XeenGameplayContext &before,std::uint16_t content) {
	if (content!=9 || !xeenRegionalContext(before) || before.year!=610 ||
		(before.day!=8 && before.day!=9)) return {};
	auto after=before;after.day=static_cast<std::uint16_t>(before.day+1);
	return after;
}
}
#endif
