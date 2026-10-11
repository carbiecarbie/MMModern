#include "games/xeen/XeenItemRewards.h"
#include <sstream>
#include <limits>

namespace mmodern {
namespace {
void increment(std::size_t &value) noexcept {
	if (value != std::numeric_limits<std::size_t>::max()) ++value;
}
// Roster names are data, not font control sequences in diagnostic presentation.
std::string diagnosticName(const std::string &name) {
	std::string result;
	for (unsigned char c : name) result += c >= 32 && c < 127 ? static_cast<char>(c) : '?';
	return result;
}
void fields(std::ostream &out, XeenItem item, bool misc) {
	out << "M=" << unsigned(item.material) << " ID=" << unsigned(item.id)
		<< " S=" << unsigned(item.state) << " F=" << unsigned(item.frame)
		<< (misc ? " charges=" : " counter=") << unsigned(item.state & 63)
		<< " cursed=" << bool(item.state & 64) << " broken=" << bool(item.state & 128);
}
}
XeenRewardEnqueue XeenPendingRewards::enqueue(XeenItem item) noexcept {
	return enqueue(item,XeenRewardCategory::Miscellaneous);
}
XeenRewardEnqueue XeenPendingRewards::enqueue(XeenItem item,XeenRewardCategory category) noexcept {
	if (!item.id) { increment(_invalid); return XeenRewardEnqueue::InvalidEmpty; }
	const auto index=static_cast<unsigned>(category);
	if(index>=4 || _counts[index]==kCapacity) {increment(_overflow);return XeenRewardEnqueue::IgnoredOverflow;}
	++_counts[index];_categories[_size]=category;_items[_size++] = item;
	return XeenRewardEnqueue::Accepted;
}
XeenItemCategory &xeenRewardItems(XeenCharacter &c,XeenRewardCategory category) noexcept {
	switch(category) {
	case XeenRewardCategory::Weapons:return c.weapons;
	case XeenRewardCategory::Armor:return c.armor;
	case XeenRewardCategory::Accessories:return c.accessories;
	case XeenRewardCategory::Miscellaneous:return c.miscellaneous;
	}
	return c.miscellaneous;
}
bool xeenInsertReward(XeenCharacter &character,XeenItem item,XeenRewardCategory category) noexcept {
	auto &items=xeenRewardItems(character,category);
	if(!item.id || !xeenItemHasTailCapacity(items))return false;
	items.back()=item;xeenCompactItems(items);return true;
}
bool xeenInsertMiscellaneous(XeenCharacter &character, XeenItem item) noexcept {
	return xeenInsertReward(character,item,XeenRewardCategory::Miscellaneous);
}
bool xeenPacksGloballyFull(const XeenPartyState &party) {
	if (!party.party.size()) return false;
	for (const auto id : party.party.activeRosterIds()) {
		const auto &c = party.roster.at(id);
		if (xeenItemHasTailCapacity(c.weapons) || xeenItemHasTailCapacity(c.armor) ||
			xeenItemHasTailCapacity(c.accessories) || xeenItemHasTailCapacity(c.miscellaneous)) return false;
	}
	return true;
}
XeenRewardReceipt xeenDeliverRewards(XeenPendingRewards &pending,
		XeenPartyState &party, std::optional<std::size_t> preferred) {
	XeenRewardReceipt result;
	result.count = pending.size(); result.overflow = pending.overflow(); result.invalid = pending.invalid();
	const auto &ids = party.party.activeRosterIds();
	// Membership is validated by XeenParty; resolve all owners before mutation.
	std::array<XeenCharacter *, XeenParty::kMaximumVisibleMembers> owners{};
	for (std::size_t i = 0; i < ids.size(); ++i) owners[i] = &party.roster.at(ids[i]);
	std::size_t entryIndex=0;
	for(unsigned category=0;category<4;++category)for(std::size_t i=0;i<pending.size();++i)
		if(static_cast<unsigned>(pending.categoryAt(i))==category) {
			auto &entry=result.entries[entryIndex++];entry.item=pending.at(i);entry.category=pending.categoryAt(i);
		}
	bool discarded=xeenPacksGloballyFull(party);
	// Everything below is bounded, allocation-free and nonthrowing.
	for (std::size_t i = 0; i < result.count; ++i) {
		auto &entry = result.entries[i];
		discarded=discarded || xeenPacksGloballyFull(party);
		std::optional<std::size_t> recipient;
		const auto eligible = [&](std::size_t index) {
			return !discarded && owners[index]->canAct() && xeenItemHasTailCapacity(xeenRewardItems(*owners[index],entry.category));
		};
		if (preferred && *preferred < ids.size() && eligible(*preferred)) recipient = preferred;
		for (std::size_t j = 0; !recipient && j < ids.size(); ++j) if (eligible(j)) recipient = j;
		if (recipient) {
			xeenInsertReward(*owners[*recipient], entry.item,entry.category);
			entry.owner = ids[*recipient]; ++result.delivered;
		} else {
			bool anyEligible = false;
			for (std::size_t j = 0; j < ids.size(); ++j) anyEligible |= owners[j]->canAct();
			entry.loss = ids.empty() ? XeenRewardLoss::EmptyParty : anyEligible ?
				XeenRewardLoss::CategoryFull : XeenRewardLoss::NoEligibleMember;
			++result.lost;
		}
	}
	// giveTreasure checks global capacity at every slot, including empty slots.
	// Once full it clears the remaining treasure and its staged currency.
	std::size_t miscCount=0;for(std::size_t i=0;i<result.count;++i)miscCount+=result.entries[i].category==XeenRewardCategory::Miscellaneous;
	if(miscCount<10)discarded=discarded || xeenPacksGloballyFull(party);
	result.currencyLost=discarded && (pending.gold() || pending.gems());
	if(!discarded) {result.gold=pending.gold();result.gems=pending.gems();}
	if(party.monsterTreasure) {party.monsterTreasure->gold+=result.gold;party.monsterTreasure->gems+=result.gems;}
	pending = {};
	return result;
}
void xeenDiscardRewards(XeenPendingRewards &pending, XeenRewardReceipt &receipt,
		XeenRewardDiscard reason) noexcept {
	if (pending.hasWork()) {
		receipt.discarded = pending.size(); receipt.overflow = pending.overflow();
		receipt.invalid = pending.invalid(); receipt.discardReason = reason;
	}
	pending = {};
}
std::string xeenRewardReceiptText(const XeenRewardReceipt &receipt, const XeenRoster &roster) {
	std::ostringstream out;
	out << "Reward receipt\nDelivered=" << receipt.delivered << " Lost=" << receipt.lost
		<< " Overflow=" << receipt.overflow << " Invalid empty=" << receipt.invalid << '\n';
	out << "M=material S=state F=frame\n";
	if(receipt.gold || receipt.gems || receipt.currencyLost)
		out<<"Gold="<<receipt.gold<<" Gems="<<receipt.gems<<(receipt.currencyLost?"; remaining treasure lost: backpacks full":"")<<'\n';
	for (std::size_t i = 0; i < receipt.count; ++i) {
		const auto &e = receipt.entries[i];
		out << i + 1 << ". ";
		if (e.owner) out << "Owner " << unsigned(*e.owner) << " " << diagnosticName(roster.at(*e.owner).name);
		else out << "Lost: " << (e.loss == XeenRewardLoss::EmptyParty ? "empty party" :
			e.loss == XeenRewardLoss::NoEligibleMember ? "no eligible member" : "eligible category tails full");
		constexpr const char *names[]={"Weapons","Armor","Accessories","Miscellaneous"};
		out << '\n'<<names[static_cast<unsigned>(e.category)]<<' '; fields(out, e.item,e.category==XeenRewardCategory::Miscellaneous); out << '\n';
	}
	if (receipt.overflow) out << "Overflow ignored: pending capacity 10 per category.\n";
	if (receipt.invalid) out << "Empty typed input rejected: ID zero.\n";
	out << "Space / Enter / Escape: next or done";
	return out.str();
}
std::string xeenInventorySummary(const XeenPartyState &party) {
	return "Inventory: " + std::to_string(party.party.size()) + " active references, 30 owners; Root=" +
		std::to_string(party.questItems.at(17)) + " Q2=" + std::to_string(party.questFlags.isSet(2));
}
std::string xeenInventoryInspection(const XeenPartyState &party) {
	std::ostringstream out;
	out << xeenInventorySummary(party) << "\nActive order:";
	std::array<bool, XeenRoster::kCharacterCount> seen{};
	std::size_t index = 0;
	for (auto id : party.party.activeRosterIds()) {
		out << " [" << index++ << "->" << unsigned(id) << (seen[id] ? " alias" : "") << ']'; seen[id] = true;
	}
	out << "\nRaw decimal M=material ID=id S=state F=frame; tails: 1=capacity\n";
	for (std::size_t id = 0; id < XeenRoster::kCharacterCount; ++id) {
		const auto &c = party.roster.at(id);
		out << "Owner " << id << " " << diagnosticName(c.name) << (seen[id] ? " active" : " inactive")
			<< " eligible=" << c.canAct() << " worst=" << xeenConditionName(c.worstCondition()) << '\n';
		const XeenItemCategory *categories[] = {&c.weapons, &c.armor, &c.accessories, &c.miscellaneous};
		const char *names[] = {"Weapons", "Armor", "Accessories", "Miscellaneous"};
		for (int category = 0; category < 4; ++category) {
			out << names[category] << " tail=" << xeenItemHasTailCapacity(*categories[category]) << '\n';
			for (std::size_t slot = 0; slot < 9; ++slot) {
				out << " " << slot << ": "; fields(out, (*categories[category])[slot], category == 3); out << '\n';
			}
		}
	}
	return out.str();
}
}
