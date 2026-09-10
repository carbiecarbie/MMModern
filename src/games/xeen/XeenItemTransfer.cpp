#include "games/xeen/XeenItemTransfer.h"
#include "games/xeen/XeenCharacterRules.h"
#include "games/xeen/XeenPartyLoader.h"
#include <stdexcept>
#include <type_traits>

namespace mmodern {
namespace {
template<class Character> auto items(Character &c, XeenInventoryCategory category) noexcept
	-> decltype(&c.weapons) {
	switch (category) {
	case XeenInventoryCategory::Weapons: return &c.weapons;
	case XeenInventoryCategory::Armor: return &c.armor;
	case XeenInventoryCategory::Accessories: return &c.accessories;
	case XeenInventoryCategory::Miscellaneous: return &c.miscellaneous;
	}
	return nullptr;
}
}
XeenItemCategory *xeenInventoryItems(XeenCharacter &c, XeenInventoryCategory category) noexcept { return items(c, category); }
const XeenItemCategory *xeenInventoryItems(const XeenCharacter &c, XeenInventoryCategory category) noexcept { return items(c, category); }
bool xeenSameItem(XeenItem a, XeenItem b) noexcept {
	return a.material == b.material && a.id == b.id && a.state == b.state && a.frame == b.frame;
}
XeenTransferResult xeenTransferItem(XeenPartyState &party, std::size_t source,
		std::size_t destination, XeenInventoryCategory category, std::size_t slot) {
	using Status = XeenTransferStatus;
	const auto &ids = party.party.activeRosterIds();
	if (ids.empty()) return {Status::EmptyParty};
	if (source >= ids.size() || destination >= ids.size()) return {Status::InvalidParticipant};
	if (ids[source] >= XeenRoster::kCharacterCount || ids[destination] >= XeenRoster::kCharacterCount)
		return {Status::InvalidOwner};
	auto &src = party.roster.at(ids[source]);
	auto &dst = party.roster.at(ids[destination]);
	if (src.rosterId != ids[source] || dst.rosterId != ids[destination]) return {Status::InvalidOwner};
	if (ids[source] == ids[destination]) return {Status::SameOwner};
	auto *sourceItems = xeenInventoryItems(src, category);
	auto *destinationItems = xeenInventoryItems(dst, category);
	if (!sourceItems) return {Status::InvalidCategory};
	if (slot >= sourceItems->size()) return {Status::InvalidSlot};
	const auto item = (*sourceItems)[slot];
	if (!item.id) return {Status::EmptySource};
	if (item.state & 0x40) return {Status::Cursed};
	if (!xeenItemHasTailCapacity(*destinationItems)) return {Status::DestinationFull};
	auto candidateSource = src;
	auto candidateDestination = dst;
	auto &s = *xeenInventoryItems(candidateSource, category);
	auto &d = *xeenInventoryItems(candidateDestination, category);
	d.back() = item;
	s[slot] = {};
	d.back().frame = 0;
	xeenCompactItems(s);
	xeenCompactItems(d);
	try {
		XeenCharacterRules::validateForUse(candidateSource, {kCloudsInitialYear});
		XeenCharacterRules::validateForUse(candidateDestination, {kCloudsInitialYear});
	} catch (const std::invalid_argument &) { return {Status::UnsafeRules}; }
	XeenTransferResult result{Status::Success, ids[source], ids[destination], 0, item};
	for (const auto &record : *destinationItems) if (record.id) ++result.destinationSlot;
	static_assert(std::is_nothrow_copy_assignable_v<XeenItemCategory>);
	static_assert(std::is_nothrow_copy_constructible_v<XeenTransferResult>);
	// Unobservable publication segment. The second assignment is the commit point.
	*sourceItems = s;
	*destinationItems = d;
	return result;
}
const char *xeenTransferMessage(XeenTransferStatus status) noexcept {
	switch (status) {
	case XeenTransferStatus::Success: return "Item transferred";
	case XeenTransferStatus::EmptyParty: return "No active characters";
	case XeenTransferStatus::InvalidParticipant: return "No active member at that F-key";
	case XeenTransferStatus::InvalidOwner: return "Invalid roster owner";
	case XeenTransferStatus::SameOwner: return "Same owner";
	case XeenTransferStatus::InvalidCategory: return "Invalid category";
	case XeenTransferStatus::InvalidSlot: return "Invalid slot";
	case XeenTransferStatus::EmptySource: return "Select an occupied item";
	case XeenTransferStatus::StaleSelection: return "Selection changed; select again";
	case XeenTransferStatus::Cursed: return "Cannot transfer a cursed item";
	case XeenTransferStatus::DestinationFull: return "Destination category tail is full";
	case XeenTransferStatus::UnsafeRules: return "Transfer would exceed safe character rules";
	}
	return "Invalid transfer";
}
}
