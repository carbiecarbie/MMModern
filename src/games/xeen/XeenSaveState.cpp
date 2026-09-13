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
#include "games/xeen/XeenJourneyCapture.h"

#include <stdexcept>
#include <type_traits>
#include <utility>

namespace mmodern {

bool XeenWorld::journeyCaptureEligible(const XeenPartyState &party, const XeenCamera &camera) const noexcept {
	const auto retained = _journeyCapture.lock();
	return retained && retained->current(party,camera);
}

void XeenSaveState::validateJourneyValues(const XeenSaveSnapshot &s) {
	const auto &j = *s.journey;
	const auto require = [](bool ok) { if (!ok) throw std::invalid_argument("Unsupported Journey durable state"); };
	const auto &policy=xeenJourneyContent(j.contract);
	require(s.resources.darkside.has_value() && j.initializedMap==XeenMapIdentity(20) && j.originalActorCount==27 && j.actors.size()==policy.count && s.camera.mapId==XeenMapIdentity(20) && policy.contains(s.camera.x,s.camera.y));
	for (unsigned i=0;i<policy.count;++i) {
		const auto &a=j.actors[i]; const auto id=policy.records[i]; const auto admission=policy.actor(id);
		require(a.id==XeenMonsterIdentity{20,id} && a.status==XeenActorStatus::Physical);
		if (a.lifecycle==XeenActorLifecycle::Present) {
			require(a.hp==admission.hp && !a.accounted && policy.movementContains(a.x,a.y) && (a.x!=s.camera.x || a.y!=s.camera.y));
			if (j.contract==1) require(a.activated);
			else {
				require(!policy.blockedTerrain(a.x,a.y) && admission.contains(a.x,a.y));
				if (!a.activated) require(a.x==admission.spawnX && a.y==admission.spawnY);
			}
		} else require(a.lifecycle==XeenActorLifecycle::Defeated && a.hp==0 && !a.activated && a.accounted && a.x==-128 && a.y==-128);
	}
	if (j.contract==2) {
		for (auto id:s.disabledObjects) require(!(id==XeenObjectIdentity{20,1}));
		for (auto id:s.disabledEvents) require(!(id.mapId==XeenMapIdentity(20) && id.recordIndex>=1 && id.recordIndex<=5));
	}

}

void XeenSaveState::restoreJourney(const XeenSaveSnapshot &source, const Resources &resources,
		XeenPartyState &party, XeenCamera &camera, XeenGameFlags &flags, XeenWorld &world, const Preflight &preflight) {
	const auto snapshot = source;
	validateJourneyValues(snapshot);
	const auto monsters = resources.loadMonsterStatistics;
	const auto eventProvider = resources.loadEvents;
	const auto presentation = preflight;
	if (!monsters || !eventProvider || !presentation || !world._objectLoader)
		throw std::invalid_argument("Journey restoration requires MON/MOB/EVT and presentation providers");
	if (world._gameplayBorrow.borrowed() || party._gameplayBorrow.borrowed() || party.roster._gameplayBorrow.borrowed() ||
		camera.gameplayBorrow.borrowed() || flags._gameplayBorrow.borrowed() || world._combatCheck || world._combatAuthorized ||
		world._journeyRestoration || !world._journeyCapture.expired() || world._sessionState._combatOwner ||
		world._sessionState._combatApproachState || world._sessionState._journeyOwner ||
		world._sessionState._encounterRevision || world._sessionState._completedAuthority || world._sessionState._completedLease)
		throw std::logic_error("Journey restoration requires fresh unborrowed destinations");
	for (unsigned i = 0; i < 30; ++i)
		if (party.roster.combatInputs(i)) throw std::logic_error("Journey destination has detached supplements");
	XeenRestoreGuard destination(world,party,camera,flags,true);
	XeenPartyState p;
	XeenCamera c = snapshot.camera;
	XeenGameFlags f(snapshot.gameFlags);
	XeenWorld w(world._loader,world._objectLoader);
	for (unsigned i = 0; i < 30; ++i) p.roster.at(i) = snapshot.characters[i];
	p.party = XeenParty::fromRosterIds(snapshot.activeRosterIds);
	p.questItems = XeenCloudsQuestItems(snapshot.questItems); p.questFlags = XeenCloudsQuestFlags(snapshot.questFlags);
	p.firstSerializedCount = p.effectiveSerializedCount = 6;
	p.encounterContext = snapshot.journey->context;
	p.roster._combatMarked = true;
	for (const auto &r : snapshot.journey->supplements) p.roster._combatInputs[r.owner] = r.inputs;
	w._sessionState._skeletonSeed = snapshot.journey->skeletonSeed;
	xeenValidateJourneyParty(p,snapshot.journey->contract);
	std::optional<XeenRestoreGuard> prepared;
	const auto adopt = [&] { prepared.emplace(w,p,c,f); };
	adopt();
	const auto callback = [&](auto &&provider) {
		destination.check(); prepared->check();
		try {
			auto returned = provider(); destination.check(); prepared->check();
			const auto detached = returned; return detached;
		} catch (...) { destination.check(); prepared->check(); throw; }
	};
	const auto mapProvider = w._loader; const auto mobProvider = w._objectLoader;
	w._loader = [&](XeenMapIdentity id) {
		auto value = callback([&] { return mapProvider(id); }); prepared->admitMap(id,value); return value;
	};
	w._objectLoader = [&](XeenMapIdentity id) {
		auto value = callback([&] { return mobProvider(id); }); prepared->admitObjects(id,value); return value;
	};
	const auto events = [&](XeenMapIdentity id) { return callback([&] { return eventProvider(id); }); };
	static_cast<void>(w.map(c.mapId));
	w.restoreSessionState(snapshot.disabledObjects,snapshot.disabledEvents,events);
	adopt(); // Only checked overlay preparation changed candidate gameplay values.
	auto statistics = callback(monsters);
	auto actors = XeenActorApproach::actorsFromResources(w.objectFile(20),statistics);
	auto evt = events(20);
	if (actors.size()!=27) throw std::invalid_argument("Journey requires complete original actor collection");
	const auto &policy=xeenJourneyContent(snapshot.journey->contract);
	for (unsigned i=0;i<policy.count;++i) {
		const auto &a=actors.at(policy.records[i]);
		const auto admission=policy.actor(policy.records[i]);
		if (!a.statistics || a.original.resourceId!=admission.resourceId) throw std::invalid_argument("Journey original species mismatch");
		if (snapshot.journey->contract==2) admission.validateStatistics(*a.statistics);
		else a.statistics->validateCombat();
	}
	for (const auto &live:snapshot.journey->actors) {
		auto &a=actors.at(live.id.recordIndex);
		a.x=live.x; a.y=live.y; a.hp=live.hp; a.activated=live.activated; a.lifecycle=live.lifecycle; a.status=live.status;
	}
	static_cast<void>(CloudsUiComposer::buildPortraitPlacements(p));
	auto &s = w._sessionState;
	s._actors.swap(actors); s._entry = XeenEncounterEntry::Journey;
	s._encounterMarked = s._encounterInitialized = true; s._encounterRevision = 1;
	s._skeletonSeed = snapshot.journey->skeletonSeed;
	s._journeyContract=snapshot.journey->contract; s._journeyRandom=snapshot.journey->random;
	for (const auto &live:snapshot.journey->actors) if (live.accounted) s._accountedMonsters.insert(live.id);
	adopt(); // Complete saved domain, deliberately unbound and unavailable.
	XeenActorApproach::validateEnvironment(w,s._actors,evt,s._journeyContract);
	const auto view=XeenActorApproach::classify(s._actors,c);
	for (unsigned i=0;i<s._actors.size();++i) if (view.activation[i] && !s._actors[i].activated) throw std::invalid_argument("Quiet actor activation missing");
	for (auto n:XeenActorApproach::occupancy(s._actors)) if (n>3) throw std::invalid_argument("Quiet actor occupancy exceeded");
	try { presentation(w,p,c,f); }
	catch (...) { destination.check(); prepared->check(); throw; }
	destination.check(); prepared->check();
	// Prepare the complete handoff and its FINAL-owner preimage before any stores.
	auto binding = std::make_shared<XeenJourneyRestoration>();
	binding->capture.reset(new XeenJourneyCapture);
	binding->capture->admittedActors = s._actors;
	binding->statistics = std::move(statistics); binding->events = std::move(evt);
	binding->guard = std::make_shared<XeenRestoreGuard>(world,party,camera,flags);
	binding->guard->prepareJourneyPublication(*prepared);
	destination.check(); prepared->check();
	party.publishCompleted(p); // Same private full supplement publication; public copy rules remain closed.
	camera = c; flags = f;
	world.swapPreparedState(w);
	auto &out = world._sessionState;
	out._actors.swap(s._actors); out._accountedMonsters.swap(s._accountedMonsters);
	out._entry = XeenEncounterEntry::Journey; out._encounterMarked = out._encounterInitialized = true;
	out._encounterRevision = 1; out._skeletonSeed = snapshot.journey->skeletonSeed;
	out._journeyContract=snapshot.journey->contract; out._journeyRandom=snapshot.journey->random;
	world._journeyRestoration.swap(binding);
}

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
	if (!world.hasEncounterState() && !party.encounterContext && !party.roster.combatMarked()) {
		for (unsigned owner = 0; owner < XeenRoster::kCharacterCount; ++owner)
			if (party.roster.combatInputs(owner)) return false;
		return true;
	}
	return world.completedCaptureEligible(party, camera) || world.journeyCaptureEligible(party,camera);
}

XeenSaveSnapshot XeenSaveState::capture(const XeenSaveResourceSignature &resources,
		const XeenPartyState &party, const XeenCamera &camera,
		const XeenGameFlags &flags, const XeenWorld &world) {
	const bool completed = world.sessionState().completion() == XeenEncounterCompletion::VictoryQuiescent;
	// Reject an unrelated flag owner before observing the bound graph's integrity.
	if (world.sessionState().journey()) {
		const auto authority = world._journeyCapture.lock();
		if (!authority || !authority->preimage || &(**authority->preimage).f != &flags)
			throw std::logic_error("Journey capture requires the bound game-flag owner");
	}
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
	if (state.journey()) {
		xeenValidateJourneyParty(party,state.journeyContract());
		XeenSaveJourney j;
		j.context = party.encounterContext; j.skeletonSeed = state.skeletonSeed();
		j.schema=j.contract=state.journeyContract(); j.random=state.journeyRandom();
		for (unsigned i = 0; i < 30; ++i) j.supplements[i] = {static_cast<std::uint8_t>(i), *party.roster.combatInputs(i)};
		if (state.actors().size() != 27) throw std::logic_error("Journey actor collection changed");
		for (const auto &a:state.actors()) if (xeenJourneyContent(j.contract).influences(a.id.recordIndex))
			j.actors.push_back({a.id,a.x,a.y,a.hp,a.activated,a.lifecycle,a.status,state.accountedMonsters().count(a.id) != 0});
		snapshot.journey = std::move(j);
		validateJourneyValues(snapshot);
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
	if (snapshot.journey) {
		restoreJourney(snapshot,resources,party,camera,flags,world,preflight);
		return;
	}
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
