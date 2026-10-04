#ifndef MMODERN_GAMES_XEEN_CLOUDS_UI_COMPOSER_H
#define MMODERN_GAMES_XEEN_CLOUDS_UI_COMPOSER_H

#include "core/IndexedFrame.h"
#include "games/xeen/XeenParty.h"

#include <cstddef>
#include <cstdint>
#include <optional>
#include <string>
#include <vector>

namespace mmodern {

class XeenAssetSource;
struct XeenCharacterRulesContext;
class XeenScenePresentation;

class CloudsUiComposer {
public:
	struct PortraitPlacement {
		std::string resourceName;
		std::size_t frame = 0;
		int x = 0;
		int y = 0;
	};

	struct HpPlacement {
		std::size_t partySlot = 0;
		std::uint8_t rosterId = 0;
		std::size_t frame = 0;
		int x = 0;
		int y = 0;
	};

	static constexpr int kWidth = 320;
	static constexpr int kHeight = 200;

	static std::vector<PortraitPlacement> buildPortraitPlacements(
		const XeenPartyState &partyState, unsigned memberMask=0x3f);
	static std::vector<HpPlacement> buildHpPlacements(
		const XeenPartyState &partyState,
		const XeenCharacterRulesContext &context, unsigned memberMask=0x3f);
	static std::optional<std::size_t> partyMemberAtSlot(const XeenPartyState &, unsigned memberMask, std::size_t slot);
	static std::vector<PortraitPlacement> buildPartyFeedbackPlacements(const XeenPartyState &,
		const XeenScenePresentation &, unsigned memberMask, int actingMember);
	void loadBackground(XeenAssetSource &assets) const;
	// Replace the already composed main-screen controls with ICONS_COMBAT.
	void drawCombatButtons(XeenAssetSource &assets, IndexedFrame &frame) const;
	void drawInterface(XeenAssetSource &assets, const XeenPartyState &partyState,
		const XeenCharacterRulesContext &context) const;
	IndexedFrame compose(XeenAssetSource &assets, const XeenPartyState &partyState,
		const XeenCharacterRulesContext &context) const;
};

} // namespace mmodern

#endif
