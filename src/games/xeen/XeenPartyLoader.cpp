#include "games/xeen/XeenPartyLoader.h"

#include "formats/xeen/XeenAssetSource.h"
#include "formats/xeen/XeenCharacterFormat.h"
#include "formats/xeen/XeenQuestItemFormat.h"
#include "formats/xeen/XeenQuestFlagFormat.h"

#include <array>
#include <stdexcept>
#include <string>

namespace mmodern {

XeenPartyState XeenPartyLoader::loadInitialCloudsParty(XeenAssetSource &assets) const {
	const auto rosterBytes = assets.readInitialResource("maze.chr");
	const auto partyBytes = assets.readInitialResource("maze.pty");
	return loadFromResources(rosterBytes, partyBytes);
}

XeenPartyState XeenPartyLoader::loadFromResources(
		const std::vector<std::uint8_t> &rosterBytes,
		const std::vector<std::uint8_t> &partyBytes) const {
	XeenPartyState state;
	state.roster = XeenCharacterFormat::parseRoster(rosterBytes);
	const auto header = XeenCharacterFormat::parsePartyHeader(partyBytes);
	state.questItems = XeenQuestItemFormat::parseClouds(partyBytes);
	state.questFlags = XeenQuestFlagFormat::parseClouds(partyBytes);
	state.firstSerializedCount = header.firstCount;
	state.effectiveSerializedCount = header.effectiveCount;
	if (partyBytes.size() < 620) throw std::runtime_error("maze.pty truncated before food");
	// Clouds overlays only its short template; the CD tail defaults survive.
	state.soundOptions=XeenSoundOptions{partyBytes.at(14)!=0,partyBytes.at(15)!=0,false,true};
	state.food = std::uint16_t(partyBytes[618]) | (std::uint16_t(partyBytes[619])<<8);

	if (header.firstCount != header.effectiveCount) {
		state.diagnostics.push_back("maze.pty: different member counts (" +
			std::to_string(header.firstCount) + " and " +
			std::to_string(header.effectiveCount) + "); using the active first count");
	}
	if (header.firstCount > XeenParty::kMaximumVisibleMembers)
		throw std::runtime_error("maze.pty declares more than six active members");

	std::array<bool, XeenRoster::kCharacterCount> seen{};
	for (std::size_t i = 0; i < header.rosterIds.size(); ++i) {
		const int rosterId = header.rosterIds[i];
		if (rosterId < -1 || rosterId >= static_cast<int>(XeenRoster::kCharacterCount))
			throw std::runtime_error("maze.pty contains invalid roster ID at slot " +
				std::to_string(i));
		if (i >= header.firstCount || rosterId == -1)
			continue;

		if (seen[static_cast<std::size_t>(rosterId)]) {
			state.diagnostics.push_back("maze.pty: duplicate active member at roster " +
				std::to_string(rosterId));
		}
		seen[static_cast<std::size_t>(rosterId)] = true;
		state.party._activeRosterIds.push_back(static_cast<std::uint8_t>(rosterId));

		const XeenCharacter &character = state.roster.at(static_cast<std::size_t>(rosterId));
		if (character.name.empty()) {
			state.diagnostics.push_back("maze.pty: active member references empty slot " +
				std::to_string(rosterId));
		}
		if (!character.portraitResourceName()) {
			state.diagnostics.push_back("active member at roster " + std::to_string(rosterId) +
				" has no supported individual portrait");
		}
	}
	return state;
}

} // namespace mmodern
