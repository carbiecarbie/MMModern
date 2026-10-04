#include "games/xeen/CloudsUiComposer.h"
#include "games/xeen/XeenPartyVisualState.h"
#include "games/xeen/XeenScenePresentation.h"

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

std::optional<std::size_t> CloudsUiComposer::partyMemberAtSlot(const XeenPartyState &party, unsigned mask, std::size_t slot) {
	for(std::size_t member=0;member<party.party.size();++member) if(mask&(1u<<member)) {
		if(!slot)return member;
		--slot;
	}
	return {};
}

std::vector<CloudsUiComposer::PortraitPlacement> CloudsUiComposer::buildPartyFeedbackPlacements(
		const XeenPartyState &party, const XeenScenePresentation &presentation, unsigned mask, int actingMember) {
	std::vector<PortraitPlacement> result;
	unsigned slot=0;
	for(std::size_t member=0;member<party.party.size();++member) if(mask&(1u<<member)) {
		if(int(member)==actingMember) result.push_back({"global.icn",8,kFaceX.at(slot)-1,149});
		++slot;
	}
	slot=0;
	for(std::size_t member=0;member<party.party.size();++member) if(mask&(1u<<member)) {
		const auto &effect=presentation.portraits.at(party.party.activeRosterIds()[member]);
		if(effect.damageTicks)result.push_back({"charpow.icn",effect.damageFrame,kFaceX.at(slot),150});
		if(effect.spellFrame<4)result.push_back({"spellfx.icn",effect.spellFrame,kFaceX.at(slot),150});
		++slot;
	}
	return result;
}

std::vector<CloudsUiComposer::PortraitPlacement> CloudsUiComposer::buildPortraitPlacements(
		const XeenPartyState &partyState, unsigned memberMask) {
	static constexpr std::array<std::size_t, 17> kConditionFrames = {
		2, 2, 2, 1, 1, 4, 4, 4, 3, 2, 4, 3, 3, 5, 6, 7, 0
	};

	if (partyState.party.size() > kFaceX.size())
		throw std::runtime_error("Clouds interface supports at most six members");

	std::vector<PortraitPlacement> placements;
	placements.reserve(partyState.party.size());
	for (std::size_t i = 0; i < partyState.party.size(); ++i) {
		if(!(memberMask&(1u<<i)))continue;
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
		placement.x = kFaceX[placements.size()];
		placement.y = kPortraitY;
		placements.push_back(std::move(placement));
	}
	return placements;
}

std::vector<CloudsUiComposer::HpPlacement> CloudsUiComposer::buildHpPlacements(
		const XeenPartyState &partyState,
		const XeenCharacterRulesContext &context, unsigned memberMask) {
	if (partyState.party.size() > kHpX.size())
		throw std::runtime_error("Clouds interface supports at most six members");

	std::vector<HpPlacement> placements;
	placements.reserve(partyState.party.size());
	for (std::size_t i = 0; i < partyState.party.size(); ++i) {
		if(!(memberMask&(1u<<i)))continue;
		const XeenCharacter &character = partyState.party.member(partyState.roster, i);
		HpPlacement placement;
		placement.partySlot = placements.size();
		placement.rosterId = character.rosterId;
		placement.frame = XeenPartyVisualState::hpFrame(character, context);
		placement.x = kHpX[placements.size()];
		placement.y = kHpY;
		placements.push_back(placement);
	}
	return placements;
}

} // namespace mmodern
