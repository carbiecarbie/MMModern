#include "games/xeen/XeenMonsterTreasure.h"
#include "games/xeen/XeenJourneyContent.h"
#include "games/xeen/XeenItemRewards.h"
#include <stdexcept>
namespace mmodern {
namespace {
// DOS tests the equipment ID byte, but the Misc container byte (0x2c4f5..0x2c53c).
bool emptySlot(const XeenItem &item, XeenRewardCategory category) noexcept {
	return category == XeenRewardCategory::Miscellaneous ? !item.material : !item.id;
}
constexpr std::array<XeenRewardCategory,4> kCategories{XeenRewardCategory::Weapons, XeenRewardCategory::Armor,
	XeenRewardCategory::Accessories, XeenRewardCategory::Miscellaneous};
}
XeenTreasureStaging::Items &XeenTreasureStaging::items(XeenRewardCategory category) noexcept {
	switch (category) {
	case XeenRewardCategory::Weapons: return weapons;
	case XeenRewardCategory::Armor: return armor;
	case XeenRewardCategory::Accessories: return accessories;
	default: return miscellaneous;
	}
}
const XeenTreasureStaging::Items &XeenTreasureStaging::items(XeenRewardCategory category) const noexcept {
	return const_cast<XeenTreasureStaging &>(*this).items(category);
}
bool XeenTreasureStaging::storedItems() const noexcept {
	for (const auto category : kCategories)
		for (const auto &item : items(category)) if (!emptySlot(item, category)) return true;
	return false;
}
void XeenTreasureStagingState::check(std::uint64_t base) const {
	if (base != _revision) throw std::logic_error("Stale pending treasure");
}
}

#include "games/xeen/XeenCombatRules.h"
namespace mmodern {
// Fixed gold accumulates before the chance draw (DOS 0x2c2be..0x2c2c6), in a
// 32-bit sum that wraps without refusal.
XeenMonsterDropCandidate::XeenMonsterDropCandidate(const XeenTreasureStaging &before, std::uint64_t revision) :
		treasure(before), base(revision) {
	treasure.gold += 10u;
}
bool XeenMonsterDropCandidate::service(XeenConsequenceDraw &draw) {
	while (step!=Step::Done && draw.remaining) switch (step) {
	case Step::Drop: { const auto n=draw.draw(1,100);if (n) step=*n<=10 ? Step::Generate : Step::Done;break; }
	case Step::Generate:
		// DOS 0x2c3e2..0x2c3ec passes the monster's drop level to makeItem
		// before checking capacity. Current admitted drops are level 1 only.
		if (item.service(draw)) {
			generated=item.item();category=static_cast<XeenRewardCategory>(item.category());step=Step::Store;
		}
		break;
	case Step::Store: {
		outcome=XeenMonsterDropOutcome::CategoryCapacityLoss;
		// DOS WORLD/XEEN.DAT 0x2c402..0x2c540: first empty slot of the generated
		// category; readiness is set only on insertion.
		for (auto &entry:treasure.items(category))
			if (emptySlot(entry,category)) { entry=generated;treasure.ready=true;outcome=XeenMonsterDropOutcome::Item;break; }
		step=Step::Done;break;
	}
	case Step::Done: break;
	}
	return step==Step::Done;
}
XeenMonsterDeliveryCandidate xeenPrepareMonsterDelivery(const XeenTreasureStaging &before,
		const std::array<XeenCharacter,6> &characters) {
	if (!before.pending()) throw std::invalid_argument("Monster treasure is not ready for delivery");
	XeenMonsterDeliveryCandidate result; result.characters=characters;result.treasure=before;
	std::array<XeenCharacter *,6> owners{};
	for (std::size_t i=0;i<owners.size();++i) owners[i]=&result.characters[i];
	result.globallyFull=xeenPacksGloballyFull(owners.data(),owners.size());
	for (const auto category:kCategories) for (auto &entry:result.treasure.items(category)) {
		if (emptySlot(entry,category)) continue;
		auto &record=result.records.at(result.count++);record.item=entry;record.category=category;
		const auto recipient=xeenDeliverRewardItem(owners.data(),owners.size(),record.item,category);
		if (recipient) record.recipient=result.characters[*recipient].rosterId;
		else record.loss=XeenMonsterDeliveryLoss::GloballyFull;
		entry={};
	}
	return result;
}
XeenMonsterCredit xeenPrepareMonsterCredit(const XeenPurse &purse, const XeenTreasureStaging &before) {
	if (before.storedItems()) throw std::invalid_argument("Monster items must be delivered before gold credit");
	XeenMonsterCredit result{purse,before};
	result.purse.gold+=std::uint32_t(before.gold);result.purse.gems+=std::uint32_t(before.gems);
	result.treasure.clearPending();
	return result;
}
}
