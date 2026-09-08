#include "games/xeen/CloudsUiComposer.h"
#include "games/xeen/XeenPartyVisualState.h"

#include <array>
#include <stdexcept>
#include <utility>

namespace mmodern {
namespace {

constexpr std::array<int, XeenParty::kMaximumVisibleMembers> kFaceX = {
	10, 45, 81, 117, 153, 189
};

constexpr std::array<int, XeenParty::kMaximumVisibleMembers> kHpX = {
	13, 50, 86, 122, 158, 194
};

constexpr int kPortraitY = 150;
constexpr int kHpY = 182;

} // namespace

std::vector<CloudsUiComposer::PortraitPlacement> CloudsUiComposer::buildPortraitPlacements(
		const XeenPartyState &partyState) {
	static constexpr std::array<std::size_t, 17> kConditionFrames = {
		2, 2, 2, 1, 1, 4, 4, 4, 3, 2, 4, 3, 3, 5, 6, 7, 0
	};

	if (partyState.party.size() > kFaceX.size())
		throw std::runtime_error("a interface de Clouds suporta no maximo seis membros");

	std::vector<PortraitPlacement> placements;
	placements.reserve(partyState.party.size());
	for (std::size_t i = 0; i < partyState.party.size(); ++i) {
		const XeenCharacter &character = partyState.party.member(partyState.roster, i);
		const auto portrait = character.portraitResourceName();
		if (!portrait) {
			throw std::runtime_error("Active roster member " +
				std::to_string(character.rosterId) + " has no supported portrait");
		}
		const std::size_t visualFrame = kConditionFrames[
			static_cast<std::size_t>(character.worstCondition())];
		PortraitPlacement placement;
		placement.resourceName = visualFrame > 4 ? "dse.fac" : *portrait;
		placement.frame = visualFrame > 4 ? visualFrame - 5 : visualFrame;
		placement.x = kFaceX[i];
		placement.y = kPortraitY;
		placements.push_back(std::move(placement));
	}
	return placements;
}

std::vector<CloudsUiComposer::HpPlacement> CloudsUiComposer::buildHpPlacements(
		const XeenPartyState &partyState,
		const XeenCharacterRulesContext &context) {
	if (partyState.party.size() > kHpX.size())
		throw std::runtime_error("a interface de Clouds suporta no maximo seis membros");

	std::vector<HpPlacement> placements;
	placements.reserve(partyState.party.size());
	for (std::size_t i = 0; i < partyState.party.size(); ++i) {
		const XeenCharacter &character = partyState.party.member(partyState.roster, i);
		HpPlacement placement;
		placement.partySlot = i;
		placement.rosterId = character.rosterId;
		placement.frame = XeenPartyVisualState::hpFrame(character, context);
		placement.x = kHpX[i];
		placement.y = kHpY;
		placements.push_back(placement);
	}
	return placements;
}

} // namespace mmodern
