#ifndef MMODERN_GAMES_XEEN_CLOUDS_UI_COMPOSER_H
#define MMODERN_GAMES_XEEN_CLOUDS_UI_COMPOSER_H

#include "core/IndexedFrame.h"
#include "games/xeen/XeenParty.h"

#include <cstddef>
#include <cstdint>
#include <string>
#include <vector>

namespace mmodern {

class XeenAssetSource;
struct XeenCharacterRulesContext;

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
		const XeenPartyState &partyState);
	static std::vector<HpPlacement> buildHpPlacements(
		const XeenPartyState &partyState,
		const XeenCharacterRulesContext &context);
	void loadBackground(XeenAssetSource &assets) const;
	void drawInterface(XeenAssetSource &assets, const XeenPartyState &partyState,
		const XeenCharacterRulesContext &context) const;
	IndexedFrame compose(XeenAssetSource &assets, const XeenPartyState &partyState,
		const XeenCharacterRulesContext &context) const;
};

} // namespace mmodern

#endif
