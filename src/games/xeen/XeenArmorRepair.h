#ifndef MMODERN_XEEN_ARMOR_REPAIR_H
#define MMODERN_XEEN_ARMOR_REPAIR_H
#include "games/xeen/XeenItemTransfer.h"
#include "games/xeen/XeenGameplayContext.h"
#include "games/xeen/XeenItemServiceRules.h"
#include <algorithm>
#include <optional>
#include <limits>
namespace mmodern {
enum class XeenSmithBoundary { BeforeAdmission, AfterAdmission, Quote, BeforeRepair, AfterRepair, BeforeDeparture, AfterDeparture, Return,
	BeforeReservation, AfterReservation, StockComplete, BankPrepared, BeforeEventSettlement, AfterEventSettlement,
	DeparturePublished, BeforePurchase, AfterPurchase, BeforeRebind, AfterRebind,
	BeforeHealUpgrade, AfterHealUpgrade, BeforeHeal, AfterHeal };
// Checked finite authority arithmetic; this confers neither frame nor owner
// authority. Reserve the entire owed suffix before consuming another revision.
inline bool xeenSmithAuthorityRoom(std::uint64_t generation,unsigned remaining) noexcept {
	return generation<=std::numeric_limits<std::uint64_t>::max()-remaining;
}
enum class XeenArmorRepairOutcome { Quoted, Repaired, Empty, Unsupported, Intact, InsufficientGold, Cancelled, Sold, Identified, Refused };
struct XeenArmorRepairCandidate {
	XeenArmorRepairOutcome outcome=XeenArmorRepairOutcome::Unsupported;
	XeenItem before{}, after{};
	std::uint32_t price=0, goldBefore=0, goldAfter=0;
};
// ScummVM GPL-3.0-or-later armor base costs, in physical ID order. Repair
// applies its divisor separately; the plain Buy rule uses divisor 1.
inline constexpr std::array<std::uint32_t,13> kXeenArmorBaseCosts{{20,100,200,400,600,1000,2000,100,60,40,250,200,100}};
// Pure detached values. Publication and owner/frame authority belong to Flow.
// Armor costs and divisor adapted from ScummVM developers' GPL-3.0-or-later
// constants.cpp / ItemsDialog::calcItemCost, revision
// 6814ee9ba54582f5b5adcffab49efbbd8f589edd.
inline XeenArmorRepairCandidate xeenQuoteItemService(XeenInventoryCategory category,
        const XeenItem &item, std::uint32_t gold,XeenItemService action,unsigned merchant=0) noexcept {
	XeenArmorRepairCandidate result;
	result.before=item;result.after=item;result.goldBefore=result.goldAfter=gold;
	if(static_cast<unsigned>(category)>=4 || action==XeenItemService::Buy || static_cast<unsigned>(action)>3)return result;
	if (!item.id) { result.outcome=XeenArmorRepairOutcome::Empty;return result; }
	if(action==XeenItemService::Sell && ((item.state&0x40) || (category==XeenInventoryCategory::Weapons && item.id>=34))) {
		result.outcome=XeenArmorRepairOutcome::Refused;return result;
	}
	if(action==XeenItemService::Fix && !(item.state&0x80)) {result.outcome=XeenArmorRepairOutcome::Intact;return result;}
	const auto price=xeenItemPrice(category,item,action,merchant);if(!price)return result;
	result.price=*price;
	result.outcome=XeenArmorRepairOutcome::Quoted;
	return result;
}
inline XeenArmorRepairCandidate xeenQuoteArmorRepair(XeenInventoryCategory category,const XeenItem &item,std::uint32_t gold) noexcept {
	return xeenQuoteItemService(category,item,gold,XeenItemService::Fix);
}
inline XeenArmorRepairCandidate xeenPrepareItemService(XeenInventoryCategory category,const XeenItem &item,
        std::uint32_t gold,XeenItemService action,unsigned merchant=0) noexcept {
	auto result=xeenQuoteItemService(category,item,gold,action,merchant);
	if(result.outcome!=XeenArmorRepairOutcome::Quoted)return result;
	if(action==XeenItemService::Sell) {
		result.after={};result.goldAfter=gold+result.price;result.outcome=XeenArmorRepairOutcome::Sold;return result;
	}
	if(gold<result.price){result.outcome=XeenArmorRepairOutcome::InsufficientGold;return result;}
	result.goldAfter=gold-result.price;
	if(action==XeenItemService::Identify)result.outcome=XeenArmorRepairOutcome::Identified;
	else {result.after.state&=0x7f;result.outcome=XeenArmorRepairOutcome::Repaired;}
	return result;
}
inline XeenArmorRepairCandidate xeenPrepareArmorRepair(const XeenItem &item,std::uint32_t gold) noexcept {
	return xeenPrepareItemService(XeenInventoryCategory::Armor,item,gold,XeenItemService::Fix);
}
inline std::optional<XeenGameplayContext> xeenPrepareServiceDeparture(
        const XeenGameplayContext &before,std::uint16_t minutes) {
    if(!xeenRegionalContext(before))return {};
    // DOS addTime 0x263b7..263e5 first adds into the minute WORD, then
    // normalizes unsigned. A large farewell can wrap that sum within this day.
    auto midnight=before;midnight.minutes=0;
    const auto sum=static_cast<std::uint16_t>(std::uint32_t(before.minutes)+minutes);
    auto after=xeenPrepareTime(midnight,sum).context;
    if(after.minutes>=300)after.newDay=false;
    return after;
}
inline std::optional<XeenGameplayContext> xeenPrepareSmithDeparture(
		const XeenGameplayContext &before) {
	if (!xeenRegionalContext(before)) return {};
	auto after=xeenPrepareTime(before,1440).context;
	// Script addTime consumes pending dawn without resets or weakness.
	if(after.minutes>=300)after.newDay=false;
	return after;
}
inline std::optional<XeenGameplayContext> xeenPrepareTemplePaidDeparture(
		const XeenGameplayContext &before) {
	if(!xeenRegionalContext(before))return {};
	auto after=xeenPrepareTime(before,2880).context;
	if(after.minutes>=300)after.newDay=false;
	return after;
}
}
#endif
