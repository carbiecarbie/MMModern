#include "games/xeen/CloudsMapComposer.h"

#include "formats/xeen/XeenAssetSource.h"
#include "games/xeen/CloudsUiComposer.h"
#include "games/xeen/XeenIndoorScene.h"
#include "games/xeen/XeenWorld.h"
#include <algorithm>

namespace mmodern {

void CloudsMapComposer::drawOutdoorCommands(XeenAssetSource &assets,
		const std::vector<XeenOutdoorDrawCommand> &commands) const {
	for (const auto &command : commands) {
		if (const auto *object = command.object())
			assets.drawObjectVisual(object->visual, command.x, command.y, command.drawOptions());
		else
			assets.drawSprite(command.terrain().resourceName, command.terrain().frame,
				command.x, command.y, command.drawOptions());
	}
}

void CloudsMapComposer::drawIndoorCommands(XeenAssetSource &assets,
		const std::vector<XeenIndoorDrawCommand> &commands) const {
	for (const auto &command : commands) {
		if (const auto *object = command.object())
			assets.drawObjectVisual(object->visual, command.x, command.y,
				command.drawOptions());
		else
			assets.drawSprite(command.geometry().resourceName, command.geometry().frame,
				command.x, command.y, command.drawOptions());
	}
}

void CloudsMapComposer::drawInterfaceLayers(XeenAssetSource &assets,
		const XeenPartyState &partyState,
		const XeenCharacterRulesContext &context) const {
	// Interface::assembleBorder() redraws this frame after drawScene(). Parts of
	// the corner gems intentionally overlap the generic scene clipping rectangle.
	assets.drawSprite("global.icn", 0, 8, 8);
	// Resting-state border overlays from Interface::assembleBorder(). These fill
	// the intentionally transparent openings left by global.icn around the scene.
	assets.drawSprite("border.icn", 16,   0, 82); // Levitation indicator, inactive.
	assets.drawSprite("border.icn", 28, 194, 91); // Secret-door indicator, inactive.
	assets.drawSprite("border.icn", 40, 107,  9); // Danger-sense indicator, inactive.
	assets.drawSprite("border.icn",  0,   0, 32); // Left clairvoyance statue, inactive.
	assets.drawSprite("border.icn",  8, 215, 32); // Right clairvoyance statue, inactive.
	// Default (no active resistance) corner indicators from assembleBorder().
	assets.drawSprite("fecp.brd", 0,   2,   2); // Fire.
	assets.drawSprite("fecp.brd", 2, 219,   2); // Electricity.
	assets.drawSprite("fecp.brd", 4,   2, 134); // Cold.
	assets.drawSprite("fecp.brd", 6, 219, 134); // Poison.
	assets.drawSprite("bless.icn", 16, 33, 137); // Blessed indicator, inactive.

	// The compass/main button at y=137 overlaps the scene border in the original UI.
	CloudsUiComposer().drawInterface(assets, partyState, context);
}

IndexedFrame CloudsMapComposer::compose(XeenAssetSource &assets,
		XeenWorld &world, const XeenPartyState &partyState,
		const XeenCamera &camera,
		const XeenCharacterRulesContext &context,
		std::vector<XeenObjectVisual> *objectDiagnostics,
		std::optional<std::uint64_t> ordinaryPhase, bool *containsOrdinaryAnimation) const {
	if (containsOrdinaryAnimation) *containsOrdinaryAnimation = false;
	bool emittedAnimation = false;
	if (objectDiagnostics) objectDiagnostics->clear();
	CloudsUiComposer().loadBackground(assets);

	const XeenMap &map = world.map(camera.mapId);
	if (map.geometry.isOutdoors()) {
		const auto resolver = XeenObjectVisualResolver::load(assets);
		const auto commands = XeenOutdoorScene().build(world, camera, &resolver, objectDiagnostics, ordinaryPhase);
		emittedAnimation = std::any_of(commands.begin(), commands.end(), [](const auto &command) {
			return command.object() && command.object()->visual.status == XeenObjectVisualStatus::SupportedAnimated;
		});
		drawOutdoorCommands(assets, commands);
	} else {
		// Indoor darkness is deliberately ignored in Milestone 12D: the scene is
		// rendered illuminated so its geometry can be validated without gameplay.
		const auto resolver = XeenObjectVisualResolver::load(assets);
		// Indoor ordinary appearances are deliberately resolved without the
		// outdoor phase: animated visuals remain explicitly unsupported.
		const auto commands = XeenIndoorScene().build(
			world, camera, &resolver, objectDiagnostics);
		drawIndoorCommands(assets, commands);
	}

	drawInterfaceLayers(assets, partyState, context);
	auto result = assets.snapshot();
	if (containsOrdinaryAnimation) *containsOrdinaryAnimation = emittedAnimation;
	return result;
}

} // namespace mmodern
