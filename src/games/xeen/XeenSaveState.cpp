#include "games/xeen/XeenSaveState.h"

#include "formats/xeen/XeenSaveFormat.h"
#include "games/xeen/CloudsUiComposer.h"
#include "games/xeen/XeenCharacterRules.h"
#include "games/xeen/XeenPartyLoader.h"

#include <stdexcept>
#include <type_traits>
#include <utility>

namespace mmodern {

XeenSaveSnapshot XeenSaveState::capture(const XeenSaveResourceSignature &resources,
		const XeenPartyState &party, const XeenCamera &camera,
		const XeenGameFlags &flags, const XeenWorld &world) {
	XeenSaveSnapshot snapshot;
	snapshot.resources = resources;
	snapshot.camera = camera;
	snapshot.activeRosterIds = party.party.activeRosterIds();
	snapshot.characters = party.roster.characters();
	snapshot.questItems = party.questItems.counts();
	snapshot.questFlags = party.questFlags.values();
	snapshot.gameFlags = flags.values();
	const auto &state = world.sessionState();
	snapshot.disabledObjects.assign(state.disabledObjects().begin(), state.disabledObjects().end());
	snapshot.disabledEvents.assign(state.disabledEvents().begin(), state.disabledEvents().end());
	XeenSaveFormat::validate(snapshot);
	return snapshot;
}

void XeenSaveState::restoreBeforeGameplay(const XeenSaveSnapshot &snapshot,
		const Resources &resources, XeenPartyState &party, XeenCamera &camera,
		XeenGameFlags &flags, XeenWorld &world, const Preflight &preflight) {
	XeenSaveFormat::validate(snapshot);
	if (!(snapshot.resources == resources.signature))
		throw std::runtime_error("MMModern save: original archive contents are incompatible");
	if (!resources.loadInitialParty || !preflight)
		throw std::invalid_argument("save preparation requires initial party and presentation providers");

	// Original loading provides metadata only. Every modeled gameplay value is
	// then replaced by its saved value; no scripts or default grants are replayed.
	XeenPartyState candidateParty = resources.loadInitialParty();
	for (std::size_t i = 0; i < snapshot.characters.size(); ++i) {
		if (candidateParty.roster.at(i).rosterId != i)
			throw std::runtime_error("initial roster source has an inconsistent slot identity");
		candidateParty.roster.at(i) = snapshot.characters[i];
	}
	candidateParty.party = XeenParty::fromRosterIds(snapshot.activeRosterIds);
	candidateParty.questItems = XeenCloudsQuestItems(snapshot.questItems);
	candidateParty.questFlags = XeenCloudsQuestFlags(snapshot.questFlags);
	const XeenCharacterRulesContext context{kCloudsInitialYear};
	for (std::size_t i = 0; i < candidateParty.party.size(); ++i)
		XeenCharacterRules::validateForUse(candidateParty.party.member(candidateParty.roster, i), context);
	static_cast<void>(CloudsUiComposer::buildPortraitPlacements(candidateParty));

	XeenCamera candidateCamera = snapshot.camera;
	XeenGameFlags candidateFlags(snapshot.gameFlags);
	XeenWorld candidateWorld(world._loader, world._objectLoader);
	static_cast<void>(candidateWorld.map(candidateCamera.mapId));
	candidateWorld.restoreSessionState(snapshot.disabledObjects, snapshot.disabledEvents, resources.loadEvents);
	preflight(candidateWorld, candidateParty, candidateCamera, candidateFlags);

	static_assert(std::is_nothrow_swappable<XeenPartyState>::value, "party publication must not throw");
	static_assert(std::is_nothrow_copy_assignable<XeenCamera>::value, "camera publication must not throw");
	static_assert(std::is_nothrow_copy_assignable<XeenGameFlags>::value, "flag publication must not throw");
	using std::swap;
	swap(party, candidateParty);
	camera = candidateCamera;
	flags = candidateFlags;
	world.swapPreparedState(candidateWorld);
}

} // namespace mmodern
