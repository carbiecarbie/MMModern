#include "games/xeen/XeenParty.h"

#include <stdexcept>
#include <limits>
#include <utility>
#include <type_traits>

namespace mmodern {
XeenCharacter xeenDeletedCharacter(std::uint8_t owner) {
    XeenCharacter c;c.rosterId=owner;c.currentSpell=0;
    c.learnedSpells.emplace();return c;
}
XeenCombatInputs xeenDeletedCombatInputs() {
    XeenCombatInputs inputs;inputs.luck.emplace();inputs.resistances.emplace();
    inputs.poisonResistance.emplace();return inputs;
}

void XeenRoster::requireOrdinary(const XeenRoster &r) {
	if (r._combatMarked) throw std::logic_error("marked combat roster cannot be copied, moved or replaced");
	for (const auto &input:r._combatInputs) if (input) throw std::logic_error("detached combat supplements cannot be copied, moved or replaced");
}
XeenRoster::XeenRoster(const XeenRoster &r) { requireOrdinary(r); _characters = r._characters; }
XeenRoster::XeenRoster(XeenRoster &&r) { requireOrdinary(r); _characters = std::move(r._characters); }
void XeenRoster::swapOrdinary(XeenRoster &r) noexcept {
	static_assert(std::is_nothrow_swappable_v<decltype(_characters)>);
	_characters.swap(r._characters);
	++_replacement; ++r._replacement;
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
	questItems = p.questItems; questFlags = p.questFlags; worldFlags = p.worldFlags; purse = p.purse;
	serviceEconomy = p.serviceEconomy;
    playTimeSeconds=p.playTimeSeconds;cloudsCompleted=p.cloudsCompleted;
	insideInn=p.insideInn;food = p.food; soundOptions = p.soundOptions;
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
	swap(questItems,p.questItems); swap(questFlags,p.questFlags); swap(worldFlags,p.worldFlags); swap(purse,p.purse);
	swap(serviceEconomy,p.serviceEconomy);
    swap(playTimeSeconds,p.playTimeSeconds);swap(cloudsCompleted,p.cloudsCompleted);
	swap(insideInn,p.insideInn);swap(food,p.food); swap(soundOptions,p.soundOptions);
	swap(firstSerializedCount,p.firstSerializedCount); swap(effectiveSerializedCount,p.effectiveSerializedCount);
	diagnostics.swap(p.diagnostics);
	++_replacement; ++p._replacement;
}
void XeenPartyState::swap(XeenPartyState &p) {
	XeenRoster::requireOrdinary(roster); XeenRoster::requireOrdinary(p.roster); swapOrdinary(p);
}
void XeenPartyState::publishCompleted(XeenPartyState &prepared) noexcept {
	static_assert(std::is_nothrow_swappable_v<decltype(roster._combatInputs)>);
	swapOrdinary(prepared);
	roster._combatInputs.swap(prepared.roster._combatInputs);
	roster._combatMarked = true;
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

bool XeenQuestFlags::validIndex(std::int64_t index) {
	return index >= 0 && index < static_cast<std::int64_t>(kCount);
}

std::size_t XeenQuestFlags::checkedIndex(std::int64_t index) {
	if (!validIndex(index))
		throw std::out_of_range("Quest flag index outside 0..29");
	return static_cast<std::size_t>(index);
}

namespace {
std::size_t questSlot(XeenSide side, std::int64_t index) {
 if((side!=XeenSide::Clouds && side!=XeenSide::Darkside) || !XeenQuestFlags::validIndex(index))throw std::out_of_range("Invalid quest-flag side/index");
 return static_cast<unsigned>(side)*XeenQuestFlags::kCount+static_cast<std::size_t>(index);
}
}
bool XeenQuestFlags::isSet(XeenSide side,std::int64_t index) const {return _values[questSlot(side,index)];}
void XeenQuestFlags::set(XeenSide side,std::int64_t index) {const auto slot=questSlot(side,index);XeenMutationWatch::write(this);_values[slot]=true;}
void XeenQuestFlags::clear(XeenSide side,std::int64_t index) {const auto slot=questSlot(side,index);XeenMutationWatch::write(this);_values[slot]=false;}
bool XeenQuestFlags::isSet(std::int64_t index) const {
	return _values[checkedIndex(index)];
}

void XeenQuestFlags::set(std::int64_t index) {
	const auto slot = checkedIndex(index);
	XeenMutationWatch::write(this);
	_values[slot] = true;
}

void XeenQuestFlags::clear(std::int64_t index) {
	const auto slot = checkedIndex(index);
	XeenMutationWatch::write(this);
	_values[slot] = false;
}

bool XeenQuestItems::decrement(std::size_t index) {
	auto &count = _counts.at(index);
	if (!count)
		return false;
	XeenMutationWatch::write(this);
	--count;
	return true;
}

void XeenQuestItems::eventGrant(std::size_t index) {
	auto &count = _counts.at(index);
	XeenMutationWatch::write(this);
	count = afterEventGrant(count);
}

void XeenQuestItems::eventTake(std::size_t index) {
	auto &count = _counts.at(index);
	XeenMutationWatch::write(this);
	count = afterEventTake(count);
}

bool XeenQuestItems::increment(std::size_t index) {
	auto &count = _counts.at(index);
	if (count == std::numeric_limits<std::uint32_t>::max())
		return false;
	XeenMutationWatch::write(this);
	++count;
	return true;
}

std::optional<std::size_t> XeenQuestItems::indexForItemId(std::int64_t itemId) {
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
		throw std::out_of_range("Party index outside the active Xeen party");
	return roster.at(_activeRosterIds[partyIndex]);
}

} // namespace mmodern
