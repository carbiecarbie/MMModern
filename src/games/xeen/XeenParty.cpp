#include "games/xeen/XeenParty.h"

#include <stdexcept>
#include <limits>
#include <utility>
#include <type_traits>

namespace mmodern {

void XeenRoster::requireOrdinary(const XeenRoster &r) {
	if (r._combatMarked) throw std::logic_error("marked combat roster cannot be copied, moved or replaced");
}
XeenRoster::XeenRoster(const XeenRoster &r) { requireOrdinary(r); _characters = r._characters; }
XeenRoster::XeenRoster(XeenRoster &&r) { requireOrdinary(r); _characters = std::move(r._characters); }
void XeenRoster::swapOrdinary(XeenRoster &r) noexcept {
	static_assert(std::is_nothrow_swappable_v<decltype(_characters)>);
	_characters.swap(r._characters);
}
void XeenRoster::swap(XeenRoster &r) { requireOrdinary(*this); requireOrdinary(r); swapOrdinary(r); }
XeenRoster &XeenRoster::operator=(const XeenRoster &r) {
	requireOrdinary(*this); requireOrdinary(r); XeenRoster prepared(r); swapOrdinary(prepared); return *this;
}
XeenRoster &XeenRoster::operator=(XeenRoster &&r) {
	requireOrdinary(*this); requireOrdinary(r);
	if (this != &r) { XeenRoster prepared(std::move(r)); swapOrdinary(prepared); } return *this;
}
XeenPartyState::XeenPartyState(const XeenPartyState &p) {
	XeenRoster::requireOrdinary(p.roster);
	roster = p.roster; encounterContext = p.encounterContext; party = p.party;
	questItems = p.questItems; questFlags = p.questFlags;
	firstSerializedCount = p.firstSerializedCount; effectiveSerializedCount = p.effectiveSerializedCount;
	diagnostics = p.diagnostics;
}
XeenPartyState::XeenPartyState(XeenPartyState &&p) {
	XeenRoster::requireOrdinary(p.roster); swapOrdinary(p);
}
void XeenPartyState::swapOrdinary(XeenPartyState &p) noexcept {
	using std::swap;
	static_assert(std::is_nothrow_swappable_v<XeenParty> && std::is_nothrow_swappable_v<decltype(encounterContext)>);
	roster.swapOrdinary(p.roster); swap(encounterContext,p.encounterContext); swap(party,p.party);
	swap(questItems,p.questItems); swap(questFlags,p.questFlags);
	swap(firstSerializedCount,p.firstSerializedCount); swap(effectiveSerializedCount,p.effectiveSerializedCount);
	diagnostics.swap(p.diagnostics);
}
void XeenPartyState::swap(XeenPartyState &p) {
	XeenRoster::requireOrdinary(roster); XeenRoster::requireOrdinary(p.roster); swapOrdinary(p);
}
XeenPartyState &XeenPartyState::operator=(const XeenPartyState &p) {
	XeenRoster::requireOrdinary(roster); XeenRoster::requireOrdinary(p.roster);
	XeenPartyState prepared(p); swapOrdinary(prepared); return *this;
}
XeenPartyState &XeenPartyState::operator=(XeenPartyState &&p) {
	XeenRoster::requireOrdinary(roster); XeenRoster::requireOrdinary(p.roster);
	if (this != &p) { XeenPartyState prepared(std::move(p)); swapOrdinary(prepared); } return *this;
}

XeenParty XeenParty::fromRosterIds(std::vector<std::uint8_t> ids) {
	if (ids.size() > kMaximumVisibleMembers)
		throw std::invalid_argument("active party has more than six members");
	for (const auto id : ids)
		if (id >= XeenRoster::kCharacterCount)
			throw std::invalid_argument("active party has an invalid roster identity");
	XeenParty party;
	party._activeRosterIds = std::move(ids);
	return party;
}

bool XeenCloudsQuestFlags::validIndex(std::int64_t index) {
	return index >= 0 && index < static_cast<std::int64_t>(kCount);
}

std::size_t XeenCloudsQuestFlags::checkedIndex(std::int64_t index) {
	if (!validIndex(index))
		throw std::out_of_range("Clouds quest flag index outside 0..29");
	return static_cast<std::size_t>(index);
}

bool XeenCloudsQuestFlags::isSet(std::int64_t index) const {
	return _values[checkedIndex(index)];
}

void XeenCloudsQuestFlags::set(std::int64_t index) {
	_values[checkedIndex(index)] = true;
}

void XeenCloudsQuestFlags::clear(std::int64_t index) {
	_values[checkedIndex(index)] = false;
}

bool XeenCloudsQuestItems::decrement(std::size_t index) {
	auto &count = _counts.at(index);
	if (!count)
		return false;
	--count;
	return true;
}

bool XeenCloudsQuestItems::increment(std::size_t index) {
	auto &count = _counts.at(index);
	if (count == std::numeric_limits<std::uint32_t>::max())
		return false;
	++count;
	return true;
}

std::optional<std::size_t> XeenCloudsQuestItems::indexForItemId(std::int64_t itemId) {
	if (itemId < kFirstItemId || itemId >= kFirstItemId + static_cast<std::int64_t>(kCount))
		return std::nullopt;
	return static_cast<std::size_t>(itemId - kFirstItemId);
}

const XeenCharacter &XeenRoster::at(std::size_t rosterId) const {
	if (rosterId >= _characters.size())
		throw std::out_of_range("Xeen roster identity out of range");
	return _characters[rosterId];
}

XeenCharacter &XeenRoster::at(std::size_t rosterId) {
	if (rosterId >= _characters.size())
		throw std::out_of_range("Xeen roster identity out of range");
	return _characters[rosterId];
}

const XeenCharacter &XeenParty::member(const XeenRoster &roster,
		std::size_t partyIndex) const {
	if (partyIndex >= _activeRosterIds.size())
		throw std::out_of_range("indice fora da Party de Xeen");
	return roster.at(_activeRosterIds[partyIndex]);
}

} // namespace mmodern
