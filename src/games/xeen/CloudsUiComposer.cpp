#include "games/xeen/CloudsUiComposer.h"

#include "formats/xeen/XeenAssetSource.h"

#include <cstddef>
#include <algorithm>
#include <stdexcept>

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

void CloudsUiComposer::drawCombatButtons(XeenAssetSource &assets, IndexedFrame &frame) const {
    // Interface::setMainButtons(ICONS_COMBAT), pinned ScummVM
    // 6814ee9ba54582f5b5adcffab49efbbd8f589edd. GPL-3.0-or-later,
    // attributed to the ScummVM developers listed in upstream COPYRIGHT.
    if (!frame.isValid() || frame.width!=320 || frame.height!=200)
        throw std::runtime_error("Invalid combat interface framebuffer");
    for (const auto &placement : kInterfaceButtons)
        assets.drawSprite("combat.icn",placement.frame,placement.x,placement.y);
    const auto buttons=assets.snapshot();
    // The asset surface retains the current composition. Copy just controls,
    // preserving Flow's text layers and the rest of the supplied framebuffer.
    for (const auto &placement : kInterfaceButtons) {
        const int width=placement.frame==18 ? 13 : 24;
        const int height=placement.frame==18 ? 10 : 20;
        for(int y=placement.y;y<placement.y+height;++y) {
            const auto offset=static_cast<std::size_t>(y)*320+placement.x;
            std::copy_n(buttons.pixels.begin()+offset,width,frame.pixels.begin()+offset);
        }
    }
}

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
