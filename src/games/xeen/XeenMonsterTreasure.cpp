#include "games/xeen/XeenMonsterTreasure.h"
#include "games/xeen/XeenJourneyContent.h"
#include "games/xeen/XeenItemRewards.h"
#include <stdexcept>
namespace mmodern {
namespace {
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
bool XeenTreasureStaging::emptySlot(const XeenItem &item, XeenRewardCategory category) noexcept {
	return category == XeenRewardCategory::Miscellaneous ? !item.material : !item.id;
}
bool XeenTreasureStaging::full(XeenRewardCategory category) const noexcept {
	for (const auto &item : items(category)) if (emptySlot(item, category)) return false;
	return true;
}
bool XeenTreasureStaging::insert(XeenRewardCategory category, const XeenItem &item) noexcept {
	for (auto &entry : items(category))
		if (emptySlot(entry, category)) { entry = item; ready = true; return true; }
	return false;
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
		// DOS WORLD/XEEN.DAT 0x2c402..0x2c540: first empty slot of the generated
		// category; readiness is set only on insertion.
		outcome=treasure.insert(category,generated)?XeenMonsterDropOutcome::Item:XeenMonsterDropOutcome::CategoryCapacityLoss;
		step=Step::Done;break;
	}
	case Step::Done: break;
	}
	return step==Step::Done;
}
XeenPurse xeenCreditTreasure(const XeenPurse &purse, const XeenTreasureStaging &staged) noexcept {
	XeenPurse result=purse;
	result.gold+=std::uint32_t(staged.gold);result.gems+=std::uint32_t(staged.gems);
	return result;
}
}
