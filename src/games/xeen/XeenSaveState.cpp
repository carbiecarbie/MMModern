#include "games/xeen/XeenSaveState.h"

#include "formats/xeen/XeenSaveFormat.h"
#include "games/xeen/CloudsUiComposer.h"
#include "games/xeen/XeenCharacterRules.h"
#include "games/xeen/XeenPartyLoader.h"
#include "games/xeen/XeenActorApproach.h"
#include "games/xeen/XeenCompletedDomain.h"
#include "games/xeen/XeenCombatRules.h"
#include "games/xeen/XeenRestoreGuard.h"
#include "formats/xeen/XeenCharacterFormat.h"

#include <stdexcept>
#include <type_traits>
#include <utility>

namespace mmodern {

void XeenSaveState::restoreCompleted(const XeenSaveSnapshot &source, const Resources &resources,
		XeenPartyState &party, XeenCamera &camera, XeenGameFlags &flags,
		XeenWorld &world, const Preflight &preflight) {
	const auto snapshot = source;
	const auto initialProvider = resources.loadInitialParty;
	const auto chrProvider = resources.loadInitialCharacters;
	const auto contextProvider = resources.loadInitialContext;
	const auto monsterProvider = resources.loadMonsterStatistics;
	const auto eventProvider = resources.loadEvents;
	const auto presentation = preflight;
	if (!initialProvider || !chrProvider || !contextProvider || !monsterProvider || !eventProvider || !presentation)
		throw std::invalid_argument("completed restoration requires encounter resource and preflight providers");
	if (!snapshot.resources.darkside)
		throw std::invalid_argument("completed restoration requires DARK.CC fingerprint");
	if (world._gameplayBorrow.borrowed() || party._gameplayBorrow.borrowed() ||
		party.roster._gameplayBorrow.borrowed() || camera.gameplayBorrow.borrowed() || flags._gameplayBorrow.borrowed() ||
		world._combatCheck || world._combatAuthorized ||
		world._sessionState._completedAuthority || world._sessionState._completedLease ||
		world._sessionState._completedLeaseKind || world._sessionState._combatOwner || world._sessionState._combatApproachState ||
		world._sessionState._completedIntegrityUnsafe || world._sessionState._completedFatal ||
		world._sessionState._completion != XeenEncounterCompletion::None || world._sessionState._encounterRevision != 0)
		throw std::logic_error("completed restoration requires fresh unborrowed destination owners");
	for (unsigned i = 0; i < XeenRoster::kCharacterCount; ++i)
		if (party.roster.combatInputs(i)) throw std::logic_error("completed restoration destination has detached supplements");
	const XeenRestoreGuard destination(world, party, camera, flags, true);
	XeenPartyState candidateParty;
	XeenCamera candidateCamera = snapshot.camera;
	XeenGameFlags candidateFlags(snapshot.gameFlags);
	XeenWorld candidate(world._loader, world._objectLoader);
	std::optional<XeenRestoreGuard> prepared;
	const auto adoptPhase = [&] { prepared.emplace(candidate, candidateParty, candidateCamera, candidateFlags); };
	adoptPhase();
	const auto callback = [&](auto &&provider) {
		destination.check(); prepared->check();
		try {
			auto value = provider();
			destination.check(); prepared->check();
			if constexpr (std::is_same_v<decltype(value), XeenPartyState>) {
				if (value.encounterContext || value.roster.combatMarked())
					throw std::logic_error("initial provider supplied encounter owners");
				for (unsigned i = 0; i < XeenRoster::kCharacterCount; ++i)
					if (value.roster.combatInputs(i)) throw std::logic_error("initial provider supplied detached supplements");
			}
			// Detach returned storage from aliases retained by an external provider.
			const auto owned = value;
			return owned;
		} catch (...) { destination.check(); prepared->check(); throw; }
	};
	const auto mapProvider = candidate._loader;
	const auto objectProvider = candidate._objectLoader;
	candidate._loader = [&](XeenMapIdentity id) {
		auto value = callback([&] { return mapProvider(id); });
		prepared->admitMap(id, value); return value;
	};
	if (objectProvider)
		candidate._objectLoader = [&](XeenMapIdentity id) {
			auto value = callback([&] { return objectProvider(id); });
			prepared->admitObjects(id, value); return value;
		};
	const auto events = [&](XeenMapIdentity id) { return callback([&] { return eventProvider(id); }); };
	auto initial = callback(initialProvider);
	if (initial.encounterContext || initial.roster.combatMarked())
		throw std::invalid_argument("completed initial party provider supplied encounter state");
	const auto chr = callback(chrProvider);
	std::array<XeenCombatInputs, 6> initialInputs;
	for (unsigned i = 0; i < initialInputs.size(); ++i)
		initialInputs[i] = XeenCharacterFormat::parseCombatInputs(chr, kXeenCombatOwners[i]);
	xeenValidateInitialCombatParty(initial, chr, initialInputs);
	const auto initialContext = callback(contextProvider);
	const auto statistics = callback(monsterProvider);
	candidateParty = initial;
	for (unsigned i = 0; i < snapshot.characters.size(); ++i) candidateParty.roster.at(i) = snapshot.characters[i];
	candidateParty.party = XeenParty::fromRosterIds(snapshot.activeRosterIds);
	candidateParty.questItems = XeenCloudsQuestItems(snapshot.questItems);
	candidateParty.questFlags = XeenCloudsQuestFlags(snapshot.questFlags);
	adoptPhase(); // Saved ordinary values are now the approved preparation phase.
	static_cast<void>(candidate.map(candidateCamera.mapId));
	candidate.restoreSessionState(snapshot.disabledObjects, snapshot.disabledEvents, events);
	adoptPhase(); // restoreSessionState publishes only its checked ordinary overlays.
	auto actors = XeenActorApproach::actorsFromResources(candidate.objectFile(20), statistics);
	const auto encounterEvents = events(20);
	// Initial admission is a resource check; restored injured owners never pass through it.
	if (initialContext.minutes != 480 || initialContext.ctr24 != 0)
		throw std::invalid_argument("completed initial PTY context mismatch");
	XeenActorApproach::validateDomain(candidate, initial, initialContext, actors, encounterEvents);
	xeenApplyCompletedOverlay(actors, snapshot.completedEncounter->monster);
	candidateParty.encounterContext = snapshot.completedEncounter->context;
	candidateParty.roster._combatMarked = true;
	for (const auto &entry : snapshot.completedEncounter->supplements)
		candidateParty.roster._combatInputs[entry.owner] = entry.inputs;
	xeenValidateCompletedParty(candidateParty, initial, initialInputs, *actors[5].statistics);
	static_cast<void>(CloudsUiComposer::buildPortraitPlacements(candidateParty));
	auto &s = candidate._sessionState;
	s._actors.swap(actors);
	s._entry = XeenEncounterEntry::Diagnostic27;
	s._diagnostic27 = s._combatEntered = s._combatAccounted = true;
	s._encounterMarked = s._encounterInitialized = s._encounterTerminal = true;
	s._completion = XeenEncounterCompletion::VictoryQuiescent;
	s._completedMonster = snapshot.completedEncounter->monster;
	s._encounterRevision = 1;
	s._completedAuthority.emplace(xeenCompletedPreimage(candidate, candidateParty, candidateCamera));
	adoptPhase(); // Private completed facts and bindings are now fixed for preflight.
	try { presentation(candidate, candidateParty, candidateCamera, candidateFlags); }
	catch (...) { destination.check(); prepared->check(); throw; }
	destination.check(); prepared->check();
	auto authority = *s._completedAuthority;
	authority.party = &party; authority.roster = &party.roster; authority.camera = &camera;
	static_assert(std::is_nothrow_move_constructible_v<XeenCompletedEncounterAuthority>);
	destination.check(); prepared->check();
	// No candidate incarnation, callback, ticket or borrowed pointer crosses publication.
	party.publishCompleted(candidateParty);
	camera = candidateCamera; flags = candidateFlags;
	world.swapPreparedState(candidate);
	auto &published = world._sessionState;
	published._actors.swap(s._actors);
	published._entry = XeenEncounterEntry::Diagnostic27;
	published._diagnostic27 = published._combatEntered = published._combatAccounted = true;
	published._encounterMarked = published._encounterInitialized = published._encounterTerminal = true;
	published._completion = XeenEncounterCompletion::VictoryQuiescent;
	published._completedMonster = snapshot.completedEncounter->monster;
	published._completedAuthority.emplace(std::move(authority));
	published._encounterRevision = 1;
	published._completedPublished = true;
}

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
	if (!(snapshot.resources == resources.signature))
		throw std::runtime_error("MMModern save: original archive contents are incompatible");
	if (snapshot.completedEncounter) {
		restoreCompleted(snapshot, resources, party, camera, flags, world, preflight);
		return;
	}
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
