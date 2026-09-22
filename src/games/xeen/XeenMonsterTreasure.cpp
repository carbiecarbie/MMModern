#include "games/xeen/XeenMonsterTreasure.h"
#include <limits>
#include <stdexcept>
namespace mmodern {
void xeenValidateMonsterTreasure(const XeenMonsterTreasure &value, std::uint16_t contract) {
	const auto require = [](bool valid) {
		if (!valid) throw std::invalid_argument("Invalid pending monster treasure");
	};
	require(contract == 4 || contract == 5);
	require((value.pendingMask & ~0xfffu) == 0);
	unsigned count = 0;
	for (unsigned i = 0; i < 12; ++i) count += (value.pendingMask >> i) & 1;
	require(value.pendingGold == 10 * count &&
		std::uint64_t(value.gold) + value.pendingGold <= std::numeric_limits<std::uint32_t>::max());
	std::uint32_t sources = 0;
	for (unsigned category = 0; category < 2; ++category) {
		bool empty = false;
		for (const auto &entry : category ? value.armor : value.weapons) {
			if (!entry.item.id) {
				require(entry == XeenMonsterTreasureItem{});
				empty = true;
				continue;
			}
			require(!empty && entry.source < 12 && entry.item.id <= (category ? 7 : 33) &&
				entry.item.material == 0 && entry.item.state == 0 && entry.item.frame == 0);
			const auto bit = 1u << entry.source;
			require((contract == 5 || (value.pendingMask & bit)) && !(sources & bit));
			sources |= bit;
		}
	}
}
}

#include "games/xeen/XeenCombatRules.h"
namespace mmodern {
XeenMonsterDropCandidate::XeenMonsterDropCandidate(const XeenMonsterTreasure &before,unsigned source,std::uint16_t contract) : treasure(before) {
	xeenValidateMonsterTreasure(before, contract);
	bool stored = false;
	for (const auto &entries : {before.weapons, before.armor}) for (const auto &entry : entries)
		stored = stored || (entry.item.id && entry.source == source);
	if (source>=12 || stored || (before.pendingMask & (1u<<source))) throw std::invalid_argument("Duplicate or invalid Orc production");
	if (std::uint64_t(before.gold)+before.pendingGold+10>std::numeric_limits<std::uint32_t>::max())
		throw std::overflow_error("Monster gold overflow");
	treasure.pendingMask |= 1u<<source;treasure.pendingGold+=10;generated.source=static_cast<std::uint8_t>(source);
}
bool XeenMonsterDropCandidate::service(XeenConsequenceDraw &draw) {
	while (step!=Step::Done && draw.remaining) switch (step) {
	case Step::Drop: { const auto n=draw.draw(1,100);if (n) step=*n<=10 ? Step::Category : Step::Done;break; }
	case Step::Category: { const auto n=draw.draw(0,100);if (n) { category=*n;step=Step::Subcategory; } break; }
	case Step::Subcategory: { const auto n=draw.draw(0,100);if (n) { subcategory=*n;step=Step::Id; } break; }
	case Step::Id: {
		unsigned lo=1,hi=9;
		if (category<=40) { lo=subcategory<=30 ? 1 : subcategory<=60 ? 7 : subcategory<=85 ? 18 : 30;hi=subcategory<=30 ? 6 : subcategory<=60 ? 17 : subcategory<=85 ? 29 : 33; }
		else if (category<=85) { hi=7;armor=true; }
		const auto n=draw.draw(lo,hi);if (!n) break;
		if (category>85) generated.item.material=static_cast<std::uint8_t>(*n);else generated.item.id=static_cast<std::uint8_t>(*n);
		step=Step::Enchantment;break;
	}
	case Step::Enchantment: { const auto n=draw.draw(1,100);if (n) step=category>85 ? Step::Special : Step::Store;break; }
	case Step::Special: { const auto n=draw.draw(1,15);if (n) { generated.item.id=static_cast<std::uint8_t>(*n);step=Step::Charges; } break; }
	case Step::Charges: { const auto n=draw.draw(1,8);if (n) { generated.item.state=static_cast<std::uint8_t>(*n);outcome=XeenMonsterDropOutcome::ReferenceMiscellaneousDropLoss;step=Step::Done; } break; }
	case Step::Store: {
		outcome=XeenMonsterDropOutcome::CategoryCapacityLoss;
		for (auto &entry:armor ? treasure.armor : treasure.weapons) if (!entry.item.id) { entry=generated;outcome=XeenMonsterDropOutcome::Item;break; }
		step=Step::Done;break;
	}
	case Step::Done: break;
	}
	return step==Step::Done;
}
XeenMonsterDeliveryCandidate xeenPrepareMonsterDelivery(const XeenMonsterTreasure &before,
		const std::array<XeenCharacter,6> &characters, std::uint16_t contract) {
	xeenValidateMonsterTreasure(before, contract);
	if (contract == 5 && !before.ready()) throw std::invalid_argument("Monster treasure is not ready for delivery");
	XeenMonsterDeliveryCandidate result; result.characters=characters;result.treasure=before;
	result.globallyFull=true; bool eligible=false;
	for (const auto &c:characters) {
		eligible=eligible || c.canAct();
		result.globallyFull=result.globallyFull && c.weapons.back().id && c.armor.back().id && c.accessories.back().id && c.miscellaneous.back().id;
	}
	for (unsigned category=0;category<2;++category) for (auto &entry:category ? result.treasure.armor : result.treasure.weapons) {
		if (!entry.item.id) continue;
		auto &record=result.records.at(result.count++);record.production=entry;record.armor=category!=0;
		if (result.globallyFull) record.loss=XeenMonsterDeliveryLoss::GloballyFull;
		else {
			for (auto &c:result.characters) {
				auto &items=category ? c.armor : c.weapons;
				if (c.canAct() && xeenItemHasTailCapacity(items)) { items.back()=entry.item;xeenCompactItems(items);record.recipient=c.rosterId;break; }
			}
			if (!record.recipient) record.loss=eligible ? XeenMonsterDeliveryLoss::CategoryTailsFull : XeenMonsterDeliveryLoss::NoEligibleRecipient;
		}
		entry={};
	}
	return result;
}
XeenMonsterTreasure xeenPrepareMonsterGoldForfeiture(const XeenMonsterTreasure &before, std::uint16_t contract) {
	if (contract != 5) throw std::invalid_argument("Monster gold forfeiture requires content 5");
	xeenValidateMonsterTreasure(before, contract);
	auto after = before; after.pendingGold = after.pendingMask = 0; return after;
}
XeenMonsterTreasure xeenPrepareMonsterGoldCredit(const XeenMonsterTreasure &before, std::uint16_t contract) {
	xeenValidateMonsterTreasure(before, contract);
	for (unsigned category=0;category<2;++category) for (const auto &entry:category ? before.armor : before.weapons)
		if (entry.item.id) throw std::invalid_argument("Monster items must be delivered before gold credit");
	auto after=before;after.gold+=after.pendingGold;after.pendingGold=after.pendingMask=0;return after;
}
}