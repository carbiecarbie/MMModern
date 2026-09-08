#include "games/xeen/XeenParty.h"

#include <stdexcept>
#include <limits>
#include <utility>

namespace mmodern {

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
		throw std::out_of_range("ID fora do roster de Xeen");
	return _characters[rosterId];
}

XeenCharacter &XeenRoster::at(std::size_t rosterId) {
	if (rosterId >= _characters.size())
		throw std::out_of_range("ID fora do roster de Xeen");
	return _characters[rosterId];
}

const XeenCharacter &XeenParty::member(const XeenRoster &roster,
		std::size_t partyIndex) const {
	if (partyIndex >= _activeRosterIds.size())
		throw std::out_of_range("indice fora da Party de Xeen");
	return roster.at(_activeRosterIds[partyIndex]);
}

} // namespace mmodern
