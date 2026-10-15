#include "games/xeen/XeenMonsterTreasure.h"
#include "games/xeen/XeenJourneyContent.h"
#include "games/xeen/XeenItemRewards.h"
#include <algorithm>
#include <limits>
#include <vector>
#include <stdexcept>
namespace mmodern {
void xeenValidateMonsterTreasure(const XeenMonsterTreasure &value) {
	const auto require = [](bool valid) {
		if (!valid) throw std::invalid_argument("Invalid pending monster treasure");
	};

	require((value.pendingMask & ~0xfffu) == 0);
	unsigned count = 0;
	for (unsigned i = 0; i < 12; ++i) count += (value.pendingMask >> i) & 1;
	require(value.pendingGold == 10 * count &&
		std::uint64_t(value.gold) + value.pendingGold <= std::numeric_limits<std::uint32_t>::max());
	std::uint32_t sources = 0;
	for (unsigned category = 0; category < 3; ++category) {
		bool empty = false;
		for (const auto &entry : category == 2 ? value.miscellaneous : category ? value.armor : value.weapons) {
			if (!entry.item.id) {
				require(entry == XeenMonsterTreasureItem{});
				empty = true;
				continue;
			}
			require(!empty && entry.source < 12 && entry.item.frame == 0);
			if (category == 2)
				require(entry.item.material >= 1 && entry.item.material <= 9 && entry.item.id <= 15 &&
					entry.item.state >= 1 && entry.item.state <= 8);
			else require(entry.item.id <= (category ? 7 : 33) && entry.item.material == 0 && entry.item.state == 0);
			const auto bit = 1u << entry.source;
			require(!(sources & bit));
			sources |= bit;
		}
	}
	// Every referenced slot names a distinct monster; the others are empty.
	const auto referenced = value.referencedSources();
	for (unsigned slot = 0; slot < value.sources.size(); ++slot) {
		const auto id = value.sources[slot];
		if (!(referenced & (1u << slot))) { require(id == XeenMonsterIdentity{}); continue; }
		require(bool(id.mapId));
		for (unsigned other = 0; other < slot; ++other) require(!(referenced & (1u << other)) || !(value.sources[other] == id));
	}
}
namespace {
// A new source's slot: its record when below 12 and free, else the lowest free
// slot. Current content's F3 Orcs keep their record as slot, live and restored.
unsigned freeSourceSlot(const XeenMonsterTreasure &value, XeenMonsterIdentity id, std::uint32_t taken) {
	const auto preferred = std::size_t(id.recordIndex);
	if (preferred < value.sources.size() && !(taken & (1u << preferred))) return unsigned(preferred);
	for (unsigned slot = 0; slot < value.sources.size(); ++slot) if (!(taken & (1u << slot))) return slot;
	throw std::invalid_argument("Monster treasure has no free source slot");
}
template<class Map> XeenMonsterTreasure remapSources(const XeenMonsterTreasure &value, const std::array<int,12> &slot, Map identity) {
	auto result = value;result.sources = {};
	std::uint32_t mask = 0;
	for (unsigned k = 0; k < 12; ++k) if (value.pendingMask & (1u << k)) mask |= 1u << slot[k];
	result.pendingMask = mask;
	for (auto *entries : {&result.weapons, &result.armor, &result.miscellaneous})
		for (auto &entry : *entries) if (entry.item.id) entry.source = static_cast<std::uint8_t>(slot.at(entry.source));
	for (unsigned k = 0; k < 12; ++k) if (slot[k] >= 0) result.sources[slot[k]] = identity(k);
	return result;
}
}
void xeenPruneMonsterTreasureSources(XeenMonsterTreasure &value) noexcept {
	const auto referenced = value.referencedSources();
	for (unsigned slot = 0; slot < value.sources.size(); ++slot)
		if (!(referenced & (1u << slot)) && !(value.sources[slot] == XeenMonsterIdentity{})) value.sources[slot] = {};
}
XeenMonsterTreasure xeenCanonicalMonsterTreasure(const XeenMonsterTreasure &value) {
	xeenValidateMonsterTreasure(value);
	const auto referenced = value.referencedSources();
	std::vector<unsigned> order;
	for (unsigned k = 0; k < 12; ++k) if (referenced & (1u << k)) order.push_back(k);
	std::sort(order.begin(), order.end(), [&](unsigned a, unsigned b) { return value.sources[a] < value.sources[b]; });
	std::array<int,12> slot; slot.fill(-1);
	for (unsigned n = 0; n < order.size(); ++n) slot[order[n]] = int(n);
	return remapSources(value, slot, [&](unsigned k) { return value.sources[k]; });
}
XeenMonsterTreasure xeenLiveMonsterTreasure(const XeenMonsterTreasure &value) {
	xeenValidateMonsterTreasure(value);
	const auto referenced = value.referencedSources();
	std::array<int,12> slot; slot.fill(-1);
	std::uint32_t taken = 0;
	for (unsigned k = 0; k < 12; ++k) if (referenced & (1u << k)) {
		const auto free = freeSourceSlot(value, value.sources[k], taken);
		slot[k] = int(free); taken |= 1u << free;
	}
	return remapSources(value, slot, [&](unsigned k) { return value.sources[k]; });
}
}

#include "games/xeen/XeenCombatRules.h"
namespace mmodern {
XeenMonsterDropCandidate::XeenMonsterDropCandidate(const XeenMonsterTreasure &before,XeenMonsterIdentity id) : treasure(before) {
	xeenValidateMonsterTreasure(before);
	const auto referenced = before.referencedSources();
	for (unsigned slot = 0; slot < 12; ++slot)
		if ((referenced & (1u << slot)) && before.sources[slot] == id) throw std::invalid_argument("Duplicate or invalid Orc production");
	if (!id.mapId) throw std::invalid_argument("Duplicate or invalid Orc production");
	if (std::uint64_t(before.gold)+before.pendingGold+10>std::numeric_limits<std::uint32_t>::max())
		throw std::overflow_error("Monster gold overflow");
	const auto source = freeSourceSlot(before, id, referenced);
	treasure.sources[source]=id;
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
	case Step::Charges: { const auto n=draw.draw(1,8);if (n) { generated.item.state=static_cast<std::uint8_t>(*n);step=Step::Store; } break; }
	case Step::Store: {
		outcome=XeenMonsterDropOutcome::CategoryCapacityLoss;
		// DOS WORLD/XEEN.DAT 0x2c4f5..0x2c535 stages Misc in its own array.
		for (auto &entry:category>85 ? treasure.miscellaneous : armor ? treasure.armor : treasure.weapons)
			if (!entry.item.id) { entry=generated;outcome=XeenMonsterDropOutcome::Item;break; }
		step=Step::Done;break;
	}
	case Step::Done: break;
	}
	return step==Step::Done;
}
XeenMonsterDeliveryCandidate xeenPrepareMonsterDelivery(const XeenMonsterTreasure &before,
		const std::array<XeenCharacter,6> &characters) {
	xeenValidateMonsterTreasure(before);
	if (!before.ready()) throw std::invalid_argument("Monster treasure is not ready for delivery");
	XeenMonsterDeliveryCandidate result; result.characters=characters;result.treasure=before;
	std::array<XeenCharacter *,6> owners{};
	for (std::size_t i=0;i<owners.size();++i) owners[i]=&result.characters[i];
	result.globallyFull=xeenPacksGloballyFull(owners.data(),owners.size());
	for (unsigned category=0;category<3;++category) for (auto &entry:category==2 ? result.treasure.miscellaneous : category ? result.treasure.armor : result.treasure.weapons) {
		if (!entry.item.id) continue;
		auto &record=result.records.at(result.count++);record.production=entry;record.armor=category==1;record.miscellaneous=category==2;
		const auto kind=category==2 ? XeenRewardCategory::Miscellaneous : category ? XeenRewardCategory::Armor : XeenRewardCategory::Weapons;
		const auto recipient=xeenDeliverRewardItem(owners.data(),owners.size(),record.production.item,kind);
		if (recipient) record.recipient=result.characters[*recipient].rosterId;
		else record.loss=XeenMonsterDeliveryLoss::GloballyFull;
		entry={};
	}
	xeenPruneMonsterTreasureSources(result.treasure);
	return result;
}
XeenMonsterTreasure xeenPrepareMonsterGoldForfeiture(const XeenMonsterTreasure &before) {

	xeenValidateMonsterTreasure(before);
	auto after = before; after.pendingGold = after.pendingMask = 0; xeenPruneMonsterTreasureSources(after); return after;
}
XeenMonsterTreasure xeenPrepareMonsterGoldCredit(const XeenMonsterTreasure &before) {
	xeenValidateMonsterTreasure(before);
	for (const auto &entries : {before.weapons, before.armor, before.miscellaneous}) for (const auto &entry : entries)
		if (entry.item.id) throw std::invalid_argument("Monster items must be delivered before gold credit");
	auto after=before;after.gold+=after.pendingGold;after.pendingGold=after.pendingMask=0;xeenPruneMonsterTreasureSources(after);return after;
}
}
