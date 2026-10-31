#include "games/xeen/XeenMonsterTreasure.h"
#include "games/xeen/XeenCombatRules.h"
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
namespace {
// Drop chance by field (lookup 0x4d650): 10/20/50%, then 100% for fields 4-6.
constexpr std::array<unsigned,8> kDropChance{0,10,20,50,100,100,100,0};
}
// Arena map 85 leaves after XP, before any currency or draw (0x2c283..0x2c28a).
// Otherwise fixed gold and gems accumulate before the chance draw
// (0x2c2be..0x2c307), in 32-bit sums that wrap without refusal.
XeenMonsterDropCandidate::XeenMonsterDropCandidate(const XeenTreasureStaging &before, const XeenMonsterRecord &record,
		XeenMapIdentity map, std::uint64_t revision, unsigned resourceId) : treasure(before), base(revision) {
	const auto field=record.dropLevel();
	if (map.number==85) return;
	treasure.gold+=std::uint32_t(record.gold());treasure.gems+=std::uint32_t(record.gems());
	// DOS 0x2c325..0x2c337: overwrite slot zero before the ordinary drop.
	if(map.side==XeenSide::Clouds && resourceId==89) {
        auto &scepter=treasure.weapons[0];scepter.material=0;scepter.id=90;scepter.state=0;treasure.ready=true;
    }
	if (field) { level=field;item=XeenItemCandidate(level);step=Step::Drop; }
}
bool XeenMonsterDropCandidate::service(XeenConsequenceDraw &draw) {
	while (step!=Step::Done && draw.remaining) switch (step) {
	// Every nonzero field draws once, even at 100% (0x2c38b..0x2c39a).
	case Step::Drop: { const auto n=draw.draw(1,100);if (n) step=*n<=kDropChance[level] ? Step::Generate : Step::Done;break; }
	case Step::Generate:
		// DOS 0x2c3e2..0x2c3ec passes the same drop field to makeItem as its
		// level before checking capacity.
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
