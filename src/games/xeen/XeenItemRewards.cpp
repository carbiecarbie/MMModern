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
	if (!item.id) { increment(_invalid); return XeenRewardEnqueue::InvalidEmpty; }
	if (_size == kCapacity) { increment(_overflow); return XeenRewardEnqueue::IgnoredOverflow; }
	_items[_size++] = item;
	return XeenRewardEnqueue::Accepted;
}
bool xeenInsertMiscellaneous(XeenCharacter &character, XeenItem item) noexcept {
	if (!item.id || !xeenItemHasTailCapacity(character.miscellaneous)) return false;
	character.miscellaneous.back() = item;
	xeenCompactItems(character.miscellaneous);
	return true;
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
	for (std::size_t i = 0; i < result.count; ++i) result.entries[i].item = pending.at(i);
	// Everything below is bounded, allocation-free and nonthrowing.
	for (std::size_t i = 0; i < result.count; ++i) {
		auto &entry = result.entries[i];
		std::optional<std::size_t> recipient;
		const auto eligible = [&](std::size_t index) {
			return owners[index]->canAct() && xeenItemHasTailCapacity(owners[index]->miscellaneous);
		};
		if (preferred && *preferred < ids.size() && eligible(*preferred)) recipient = preferred;
		for (std::size_t j = 0; !recipient && j < ids.size(); ++j) if (eligible(j)) recipient = j;
		if (recipient) {
			xeenInsertMiscellaneous(*owners[*recipient], entry.item);
			entry.owner = ids[*recipient]; ++result.delivered;
		} else {
			bool anyEligible = false;
			for (std::size_t j = 0; j < ids.size(); ++j) anyEligible |= owners[j]->canAct();
			entry.loss = ids.empty() ? XeenRewardLoss::EmptyParty : anyEligible ?
				XeenRewardLoss::MiscellaneousFull : XeenRewardLoss::NoEligibleMember;
			++result.lost;
		}
	}
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
	for (std::size_t i = 0; i < receipt.count; ++i) {
		const auto &e = receipt.entries[i];
		out << i + 1 << ". ";
		if (e.owner) out << "Owner " << unsigned(*e.owner) << " " << diagnosticName(roster.at(*e.owner).name);
		else out << "Lost: " << (e.loss == XeenRewardLoss::EmptyParty ? "empty party" :
			e.loss == XeenRewardLoss::NoEligibleMember ? "no eligible member" : "eligible misc tails full");
		out << "\nMiscellaneous "; fields(out, e.item, true); out << '\n';
	}
	if (receipt.overflow) out << "Overflow ignored: pending capacity 10.\n";
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
