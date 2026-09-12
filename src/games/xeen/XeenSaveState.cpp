#include "games/xeen/XeenSaveState.h"

#include "formats/xeen/XeenSaveFormat.h"
#include "games/xeen/CloudsUiComposer.h"
#include "games/xeen/XeenCharacterRules.h"
#include "games/xeen/XeenPartyLoader.h"

#include <stdexcept>
#include <type_traits>
#include <utility>

namespace mmodern {

bool XeenSaveState::canCapture(const XeenPartyState &party, const XeenCamera &camera,
		const XeenWorld &world) noexcept {
	return (!world.hasEncounterState() && !party.encounterContext && !party.roster.combatMarked()) ||
		world.completedCaptureEligible(party, camera);
}

XeenSaveSnapshot XeenSaveState::capture(const XeenSaveResourceSignature &resources,
		const XeenPartyState &party, const XeenCamera &camera,
		const XeenGameFlags &flags, const XeenWorld &world) {
	const bool completed = world.sessionState().completion() == XeenEncounterCompletion::VictoryQuiescent;
	if (!canCapture(party, camera, world))
		throw std::logic_error("MMModern save: encounter sessions cannot be captured");
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
	if (completed) {
		XeenSaveCompletedEncounter value;
		value.entry = state.encounterEntry();
		value.victory = state.completion() == XeenEncounterCompletion::VictoryQuiescent;
		value.accountingConsumed = state.combatAccounted();
		value.monster = state.completedMonster();
		value.context = *party.encounterContext;
		constexpr std::array<std::uint8_t, 6> owners{0, 1, 6, 11, 14, 18};
		for (std::size_t i = 0; i < owners.size(); ++i) {
			value.supplements[i].owner = owners[i];
			value.supplements[i].inputs = *party.roster.combatInputs(owners[i]);
		}
		snapshot.completedEncounter = std::move(value);
	}
	XeenSaveFormat::validate(snapshot);
	return snapshot;
}

void XeenSaveState::restoreBeforeGameplay(const XeenSaveSnapshot &snapshot,
		const Resources &resources, XeenPartyState &party, XeenCamera &camera,
		XeenGameFlags &flags, XeenWorld &world, const Preflight &preflight) {
	if (world.hasEncounterState() || party.encounterContext || party.roster.combatMarked())
		throw std::logic_error("MMModern save: cannot restore into encounter owners");
	XeenSaveFormat::validate(snapshot);
	if (snapshot.completedEncounter)
		throw std::logic_error("MMModern save: v3 completed encounter restoration is not supported before Milestone 28B");
	if (!(snapshot.resources == resources.signature))
		throw std::runtime_error("MMModern save: original archive contents are incompatible");
	if (!resources.loadInitialParty || !preflight)
		throw std::invalid_argument("save preparation requires initial party and presentation providers");

	// Initial records supply metadata and only the fields absent from v1.
	// Resolve locally by roster slot before replacing the complete character.
	XeenPartyState candidateParty = resources.loadInitialParty();
	if (candidateParty.encounterContext || candidateParty.roster.combatMarked() ||
		world.hasEncounterState() || party.encounterContext || party.roster.combatMarked())
		throw std::logic_error("MMModern save: ordinary provider supplied encounter context");
	for (std::size_t i = 0; i < snapshot.characters.size(); ++i) {
		if (candidateParty.roster.at(i).rosterId != i)
			throw std::runtime_error("initial roster source has an inconsistent slot identity");
		auto saved = snapshot.characters[i];
		if (snapshot.itemState == XeenSaveItemState::LegacyV1MissingFields) {
			const auto &initial = candidateParty.roster.at(i);
			for (std::size_t slot = 0; slot < XeenCharacter::kEquipmentSlotsPerCategory; ++slot) {
				saved.weapons[slot].id = initial.weapons[slot].id;
				saved.armor[slot].id = initial.armor[slot].id;
				saved.accessories[slot].id = initial.accessories[slot].id;
			}
			saved.miscellaneous = initial.miscellaneous;
		}
		candidateParty.roster.at(i) = std::move(saved);
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
	const auto guard = [&] {
		if (world.hasEncounterState() || party.encounterContext || party.roster.combatMarked() ||
			candidateWorld.hasEncounterState() || candidateParty.encounterContext || candidateParty.roster.combatMarked())
			throw std::logic_error("MMModern save: encounter state appeared during preparation");
	};
	// Guard destination and candidate after each resource callback, before another
	// provider or ordinary candidate mutation can run.
	const auto mapProvider=candidateWorld._loader;
	const auto objectProvider=candidateWorld._objectLoader;
	candidateWorld._loader=[&](XeenMapIdentity id){auto value=mapProvider(id);guard();return value;};
	if(objectProvider)candidateWorld._objectLoader=[&](XeenMapIdentity id){auto value=objectProvider(id);guard();return value;};
	const auto eventProvider=[&](XeenMapIdentity id){
		if(!resources.loadEvents)throw std::invalid_argument("restoration requires an event loader");
		auto value=resources.loadEvents(id);guard();return value;
	};
	static_cast<void>(candidateWorld.map(candidateCamera.mapId));
	guard();
	candidateWorld.restoreSessionState(snapshot.disabledObjects, snapshot.disabledEvents, eventProvider);
	guard();
	preflight(candidateWorld, candidateParty, candidateCamera, candidateFlags);
	// Providers/preflight are fallible external calls. Recheck both graphs before stores.
	guard();

	static_assert(std::is_nothrow_copy_assignable<XeenCamera>::value, "camera publication must not throw");
	static_assert(std::is_nothrow_copy_assignable<XeenGameFlags>::value, "flag publication must not throw");
	using std::swap;
	// Both graphs were checked above. This private publication never exchanges markers.
	party.swapOrdinary(candidateParty);
	camera = candidateCamera;
	flags = candidateFlags;
	world.swapPreparedState(candidateWorld);
}

} // namespace mmodern
