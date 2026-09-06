#include "games/xeen/XeenParty.h"

#include <stdexcept>

namespace mmodern {

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
