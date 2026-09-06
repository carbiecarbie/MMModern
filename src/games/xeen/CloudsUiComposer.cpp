#include "games/xeen/CloudsUiComposer.h"

#include "formats/xeen/XeenAssetSource.h"

#include <cstddef>

namespace mmodern {
namespace {

struct SpritePlacement {
	const char *resourceName;
	std::size_t frame;
	int x;
	int y;
};

// Static Clouds of Xeen buttons taken from Interface::setMainButtons() and
// ButtonContainer's normal-frame rule. Party portraits are data-driven.
constexpr SpritePlacement kInterfaceButtons[] = {
	{ "main.icn",     0, 235,  75 },
	{ "main.icn",     2, 260,  75 },
	{ "main.icn",     4, 286,  75 },
	{ "main.icn",     6, 235,  96 },
	{ "main.icn",     8, 260,  96 },
	{ "main.icn",    10, 286,  96 },
	{ "main.icn",    12, 235, 117 },
	{ "main.icn",    14, 260, 117 },
	{ "main.icn",    16, 286, 117 },
	{ "main.icn",    18, 109, 137 },
	{ "main.icn",    20, 235, 148 },
	{ "main.icn",    22, 260, 148 },
	{ "main.icn",    24, 286, 148 },
	{ "main.icn",    26, 235, 169 },
	{ "main.icn",    28, 260, 169 },
	{ "main.icn",    30, 286, 169 }
};

} // namespace

void CloudsUiComposer::loadBackground(XeenAssetSource &assets) const {
	assets.loadPalette("mm4.pal");
	assets.loadRawFramebuffer("back.raw");
}

void CloudsUiComposer::drawInterface(XeenAssetSource &assets,
		const XeenPartyState &partyState,
		const XeenCharacterRulesContext &context) const {
	// PartyDrawer::drawParty() restores all six slots before drawing active faces.
	assets.drawSprite("restorex.icn", 0, 8, 149);
	for (const PortraitPlacement &placement : buildPortraitPlacements(partyState)) {
		assets.drawSprite(placement.resourceName, placement.frame,
			placement.x, placement.y);
	}
	// The original draws every face first, then the classic HP indicator pass.
	for (const HpPlacement &placement : buildHpPlacements(partyState, context)) {
		assets.drawSprite("hpbars.icn", placement.frame,
			placement.x, placement.y);
	}
	for (const SpritePlacement &placement : kInterfaceButtons) {
		assets.drawSprite(placement.resourceName, placement.frame,
			placement.x, placement.y);
	}
}

IndexedFrame CloudsUiComposer::compose(XeenAssetSource &assets,
		const XeenPartyState &partyState,
		const XeenCharacterRulesContext &context) const {
	loadBackground(assets);
	drawInterface(assets, partyState, context);
	return assets.snapshot();
}

} // namespace mmodern
