#include "games/xeen/XeenItemRewards.h"
#include <algorithm>
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
bool xeenPacksGloballyFull(XeenCharacter *const *owners, std::size_t count) noexcept {
	if (!count) return false;
	for (std::size_t i=0;i<count;++i) {
		const auto &c=*owners[i];
		if (xeenItemHasTailCapacity(c.weapons) || xeenItemHasTailCapacity(c.armor) ||
			xeenItemHasTailCapacity(c.accessories) || xeenItemHasTailCapacity(c.miscellaneous)) return false;
	}
	return true;
}
bool xeenPacksGloballyFull(const XeenPartyState &party) {
	const auto &ids=party.party.activeRosterIds();
	if (ids.empty()) return false;
	for (const auto id:ids) {
		const auto &c=party.roster.at(id);
		if (xeenItemHasTailCapacity(c.weapons) || xeenItemHasTailCapacity(c.armor) ||
			xeenItemHasTailCapacity(c.accessories) || xeenItemHasTailCapacity(c.miscellaneous)) return false;
	}
	return true;
}
std::optional<std::size_t> xeenDeliverRewardItem(XeenCharacter *const *owners,
		std::size_t count, XeenItem &item, XeenRewardCategory category,
		std::optional<std::size_t> preferred, bool sort, bool quest) noexcept {
	if (!count || !item.id || xeenPacksGloballyFull(owners,count)) return {};
	const auto capacity=[&](std::size_t i) {return xeenItemHasTailCapacity(xeenRewardItems(*owners[i],category));};
	std::optional<std::size_t> recipient;
	// DOS 0x243d5..0x24565: only a valid preference enters the eligible passes.
	if (preferred && *preferred<count) {
		if (owners[*preferred]->canAct() && capacity(*preferred)) recipient=preferred;
		for (std::size_t i=0;!recipient && i<count;++i)
			if (owners[i]->canAct() && capacity(i)) recipient=i;
	}
	for (std::size_t i=0;!recipient && i<count;++i) if (capacity(i)) recipient=i;
	// DOS 0x24567: category-only overflow replaces the first member's tail.
	if (!recipient) recipient=0;
	// Quest counters were already granted by GiveTake. Their Weapons staging
	// entry only supplies the recipient/found line; it is never inventory data.
	if(quest)return recipient;
	auto &items=xeenRewardItems(*owners[*recipient],category);
	if (items.back().id) item.frame=items.back().frame; // 0x245aa..0x245cc leaves frame intact.
	items.back()=item;
	if(sort)xeenCompactItems(items);
	return recipient;
}
XeenRewardReceipt xeenPrepareRewardReceipt(const XeenPendingRewards &pending) noexcept {
	XeenRewardReceipt result;result.count=pending.size();result.overflow=pending.overflow();result.invalid=pending.invalid();
	result.gold=pending.gold();result.gems=pending.gems();std::size_t next=0;
	for(unsigned category=0;category<4;++category)for(std::size_t i=0;i<pending.size();++i)
		if(static_cast<unsigned>(pending.categoryAt(i))==category) {
			auto &entry=result.entries[next++];entry.item=pending.at(i);entry.category=pending.categoryAt(i);entry.quest=pending.questAt(i);
		}
	return result;
}
void xeenDeliverRewardEntry(XeenRewardReceipt &receipt,std::size_t index,XeenPartyState &party,
		std::optional<std::size_t> preferred,bool sort) {
	if(index>=receipt.count)throw std::out_of_range("Reward receipt index");
	auto &entry=receipt.entries[index];
	if(entry.owner || entry.loss!=XeenRewardLoss::None)throw std::logic_error("Reward entry already published");
	const auto &ids=party.party.activeRosterIds();std::array<XeenCharacter *,XeenParty::kMaximumVisibleMembers> owners{};
	for(std::size_t i=0;i<ids.size();++i)owners[i]=&party.roster.at(ids[i]);
	const auto recipient=xeenDeliverRewardItem(owners.data(),ids.size(),entry.item,entry.category,preferred,sort,entry.quest);
	if(recipient) {entry.owner=ids[*recipient];++receipt.delivered;}
	else {entry.loss=ids.empty()?XeenRewardLoss::EmptyParty:XeenRewardLoss::GloballyFull;++receipt.lost;}
}
void xeenSortRewardEntry(const XeenRewardEntry &entry,XeenPartyState &party) {
	if(entry.owner)xeenCompactItems(xeenRewardItems(party.roster.at(*entry.owner),entry.category));
}
void xeenCreditRewardCurrency(XeenPendingRewards &pending,const XeenRewardReceipt &receipt,XeenPartyState &party) noexcept {
	if(party.purse){party.purse->gold+=receipt.gold;party.purse->gems+=receipt.gems;}
	pending={};
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
			auto &entry=result.entries[entryIndex++];entry.item=pending.at(i);entry.category=pending.categoryAt(i);entry.quest=pending.questAt(i);
		}
	// Everything below is bounded, allocation-free and nonthrowing.
	for (std::size_t i = 0; i < result.count; ++i) {
		auto &entry = result.entries[i];
		const auto recipient=xeenDeliverRewardItem(owners.data(),ids.size(),entry.item,entry.category,preferred,true,entry.quest);
		if (recipient) {
			entry.owner = ids[*recipient]; ++result.delivered;
		} else {
			entry.loss = ids.empty() ? XeenRewardLoss::EmptyParty : XeenRewardLoss::GloballyFull;
			++result.lost;
		}
	}
	// DOS 0x2433e..0x2437b clears only items; 0x2486a..0x24884 still credits money.
	result.gold=pending.gold();result.gems=pending.gems();
	if(party.purse) {party.purse->gold+=result.gold;party.purse->gems+=result.gems;}
	pending = {};
	return result;
}
void xeenDiscardRewards(XeenPendingRewards &pending, XeenRewardReceipt &receipt,
		XeenRewardDiscard reason) noexcept {
	if (pending.hasWork()) {
		const auto published=std::min(receipt.count,receipt.delivered+receipt.lost);
		receipt.discarded = receipt.count ? receipt.count-published : pending.size(); receipt.overflow = pending.overflow();
		receipt.invalid = pending.invalid(); receipt.discardReason = reason;
	}
	pending = {};
}
std::string xeenRewardReceiptText(const XeenRewardReceipt &receipt, const XeenRoster &roster) {
	std::ostringstream out;
	out << "Reward receipt\nDelivered=" << receipt.delivered << " Lost=" << receipt.lost
		<< " Overflow=" << receipt.overflow << " Invalid empty=" << receipt.invalid << '\n';
	out << "M=material S=state F=frame\n";
	if(receipt.gold || receipt.gems)
		out<<"Gold="<<receipt.gold<<" Gems="<<receipt.gems<<'\n';
	for (std::size_t i = 0; i < receipt.count; ++i) {
		const auto &e = receipt.entries[i];
		out << i + 1 << ". ";
		if (e.owner) out << "Owner " << unsigned(*e.owner) << " " << diagnosticName(roster.at(*e.owner).name);
		else out << "Lost: " << (e.loss == XeenRewardLoss::EmptyParty ? "empty party" :
			"all packs full");
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
