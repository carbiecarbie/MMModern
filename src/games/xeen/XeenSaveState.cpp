#include "games/xeen/XeenSaveState.h"

#include "formats/xeen/XeenSaveFormat.h"
#include "games/xeen/CloudsUiComposer.h"
#include "games/xeen/XeenCharacterRules.h"
#include "games/xeen/XeenPartyLoader.h"
#include "games/xeen/XeenActorApproach.h"
#include "games/xeen/XeenCombatRules.h"
#include "games/xeen/XeenRestoreGuard.h"
#include "formats/xeen/XeenCharacterFormat.h"
#include "games/xeen/XeenJourneyCapture.h"
#include "games/xeen/XeenMovement.h"
#include "games/xeen/XeenIndoorScene.h"
#include "games/xeen/XeenTraining.h"

#include <stdexcept>
#include <algorithm>
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
	require(s.barriers.empty() || j.vertigoActors.has_value());
	require(j.schema==XeenSaveFormat::kJourneySchema && j.content==XeenSaveFormat::kJourneyContent);
	require(j.context && xeenRegionalContext(*j.context));
	require(bool(j.serviceEconomy));
	if (j.serviceEconomy) {
		xeenValidateCurrentServiceEconomy(*j.serviceEconomy);
	}
	{
		require(s.resources.darkside && j.initializedMap==XeenMapIdentity(23) && j.originalActorCount>=1 && j.originalActorCount<=XeenActorApproach::kCapacity && j.actors.size()==j.originalActorCount &&
			((s.camera.mapId==XeenMapIdentity(23) && s.camera.x>=0 && s.camera.x<16 && s.camera.y>=0 && s.camera.y<16) ||
			 (s.camera.mapId==XeenMapIdentity(28) && xeenIndoorCoordinate(s.camera.x,s.camera.y) && j.vertigoActors)));
		if (j.vertigoActors) {
			require(j.vertigoActors->size()<=XeenActorApproach::kCapacity && j.cityOriginalActorCount<=j.vertigoActors->size());
			for (unsigned i=0;i<j.vertigoActors->size();++i) {
				const auto &a=(*j.vertigoActors)[i];
				require(a.id==XeenMonsterIdentity{28,i});
				require(s.camera.mapId!=XeenMapIdentity(28) || a.lifecycle!=XeenActorLifecycle::Present ||
					a.x!=s.camera.x || a.y!=s.camera.y);
			}
		}
		{
			for (const auto &id:s.disabledObjects) require(id.mapId!=XeenMapIdentity(28));
			for (const auto &id:s.disabledEvents)
				require(id.mapId!=XeenMapIdentity(28) || j.vertigoActors.has_value());
		}
		for (unsigned i=0;i<j.actors.size();++i) {
			const auto &a=j.actors[i];require(a.id==XeenMonsterIdentity{23,i} && a.status==XeenActorStatus::Physical);
			if (a.lifecycle==XeenActorLifecycle::Present) require(a.hp>0 && !a.accounted && a.x>=0 && a.x<16 && a.y>=0 && a.y<16 &&
				(s.camera.mapId!=XeenMapIdentity(23) || a.x!=s.camera.x || a.y!=s.camera.y));
			else require(a.lifecycle==XeenActorLifecycle::Defeated && !a.hp && !a.activated && a.accounted && a.x==-128 && a.y==-128);
		}
		return; // Topology/profile/closure validation needs the installation during restore.
	}
}

void XeenSaveState::restoreJourney(const XeenSaveSnapshot &source, const Resources &resources,
		XeenPartyState &party, XeenCamera &camera, XeenGameFlags &flags, XeenWorld &world, const Preflight &preflight) {
	const auto snapshot = source;
	validateJourneyValues(snapshot);
	const auto monsters = resources.loadMonsterStatistics;
	const auto eventProvider = resources.loadEvents;
	const auto regionalManifest = resources.regionalManifest;
	const auto presentation = preflight;
	if (!monsters || !eventProvider || !presentation || !world._objectLoader)
		throw std::invalid_argument("Journey restoration requires MON/MOB/EVT and presentation providers");
	if (world._gameplayBorrow.borrowed() || party._gameplayBorrow.borrowed() || party.roster._gameplayBorrow.borrowed() ||
		camera.gameplayBorrow.borrowed() || flags._gameplayBorrow.borrowed() || world._combatCheck || world._combatAuthorized ||
		world._journeyRestoration || !world._journeyCapture.expired() || world._sessionState._combatOwner ||
		world._sessionState._combatApproachState || world._sessionState._journeyOwner ||
		world._sessionState._encounterRevision)
		throw std::logic_error("Journey restoration requires fresh unborrowed destinations");
	for (unsigned i = 0; i < 30; ++i)
		if (party.roster.combatInputs(i)) throw std::logic_error("Journey destination has detached supplements");
	XeenRestoreGuard destination(world,party,camera,flags,true);
	if (!resources.loadInitialParty) throw std::invalid_argument("Restoration requires original character display data");
	XeenPartyState p = resources.loadInitialParty();
	destination.check();
	XeenCamera c = snapshot.camera;
	XeenGameFlags f(snapshot.gameFlags);
	XeenWorld w(world._baseLoader,world._baseObjectLoader);
	for (unsigned i = 0; i < 30; ++i) {
		const auto details = p.roster.at(i)._originalDetails;
		p.roster.at(i) = snapshot.characters[i];
		p.roster.at(i)._originalDetails = details;
	}
	p.party = XeenParty::fromRosterIds(snapshot.activeRosterIds);
	p.food=snapshot.food;
	p.questItems = XeenCloudsQuestItems(snapshot.questItems); p.questFlags = XeenCloudsQuestFlags(snapshot.questFlags);
	p.regionalRecovery = snapshot.journey->regionalRecovery;
	p.firstSerializedCount = p.effectiveSerializedCount = 6;
	p.encounterContext = snapshot.journey->context;
	p.monsterTreasure=snapshot.journey->treasure;
	p.serviceEconomy=snapshot.journey->serviceEconomy;
	p.roster._combatMarked = true;
	for (const auto &r : snapshot.journey->supplements) p.roster._combatInputs[r.owner] = r.inputs;
	w._sessionState._skeletonSeed = snapshot.journey->skeletonSeed;
	xeenValidateJourneyParty(p);
	std::optional<XeenRestoreGuard> prepared;
	const auto adopt = [&] {
		XeenRestoreGuard next(w,p,c,f);
		if (prepared) next.retainResources(*prepared);
		prepared.emplace(std::move(next));
	};
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
	if(!snapshot.barriers.empty()) {w.restoreBarriers(snapshot.barriers);adopt();}
	auto statistics = callback(monsters);
	const auto &policy=xeenJourneyContent();
	auto evt = events(policy.entry.mapId);
	{
		if(!resources.loadInitialCharacters)
			throw std::invalid_argument("Missing Training restoration resources");
		xeenValidateTrainingSource(callback(resources.loadInitialCharacters));
		// Bind resources even when the saved Journey has never entered the city.
		adopt();
		const auto cityEvents=events(28);
		if(!cityEvents.resourcePresent || cityEvents.mapId!=XeenMapIdentity(28))
			throw std::invalid_argument("Missing city Event resource");
	}
	auto actors = XeenActorApproach::actorsFromResources(w.objectFile(policy.entry.mapId),statistics);
	{
		if (!resources.loadRegionalText) throw std::invalid_argument("Missing regional text restoration provider");
		prepared->admitRegionalText(callback([&] { return resources.loadRegionalText(23); }));
	}
	{
		if (!resources.loadLearnedSpellNames) throw std::invalid_argument("Missing learned spell names restoration provider");
		prepared->admitLearnedSpellNames(callback(resources.loadLearnedSpellNames));
	}
	{
		if (!regionalManifest) throw std::invalid_argument("Missing regional restoration manifest");
		callback([&] { regionalManifest(w.map(23),w.objectFile(23),evt,statistics);return true; });
		if (c.mapId==XeenMapIdentity(23) &&
			!XeenMovement::component(w.map(23),9,11,policy.traversal)[c.y*16+c.x])
			throw std::invalid_argument("Regional camera outside mainland");
	}
	if (actors.size()!=snapshot.journey->originalActorCount || actors.size()!=snapshot.journey->actors.size()) throw std::invalid_argument("Journey requires complete original actor collection");
	for (const auto &live:snapshot.journey->actors) {
		auto &a=actors.at(live.id.recordIndex);
		a.x=live.x; a.y=live.y; a.hp=live.hp; a.activated=live.activated; a.lifecycle=live.lifecycle; a.status=live.status;
	}
	static_cast<void>(CloudsUiComposer::buildPortraitPlacements(p));
	auto &s = w._sessionState;
	w._cityStatistics=statistics;
	s._actors.swap(actors); s._entry = XeenEncounterEntry::Journey;
	s._encounterMarked = s._encounterInitialized = true; s._encounterRevision = 1;
	s._skeletonSeed = snapshot.journey->skeletonSeed;
	 s._journeyRandom=snapshot.journey->random;
	for (const auto &live:snapshot.journey->actors) if (live.accounted) s._accountedMonsters.insert(live.id);
	adopt(); // Base Journey values are complete before optional regional resources.
	if (snapshot.journey->vertigoActors) {
		const auto cityEvents=events(28);
		const auto cityMob=callback([&] { return w.objectFile(28); });
		auto city=XeenActorApproach::actorsFromResources(cityMob,statistics);
		if(city.size()!=snapshot.journey->cityOriginalActorCount)
			throw std::invalid_argument("Saved city original actor count differs from MOB");
		w._cityStatistics=statistics;
		w._cityOriginalActorCount=snapshot.journey->cityOriginalActorCount;
		const auto savedCount=snapshot.journey->vertigoActors->size();
		if(savedCount<city.size() || savedCount>XeenActorApproach::kCapacity)
			throw std::invalid_argument("Saved city actor collection loses original records");
		while(city.size()<savedCount) {
			XeenActor a;a.id={28,city.size()};a.original={};
			const auto type=(*snapshot.journey->vertigoActors)[city.size()].spawnedType;
			if(type!=-1) {
				if(type!=0 || statistics.empty()) throw std::invalid_argument("Invalid saved script-slot MON binding");
				a.original.resourceId=type;a.statistics=statistics.at(type);
			}
			city.push_back(std::move(a));
		}
		for (const auto &live:*snapshot.journey->vertigoActors) {
			auto &a=city.at(live.id.recordIndex);
			a.x=live.x;a.y=live.y;a.hp=live.hp;a.activated=live.activated;a.lifecycle=live.lifecycle;a.status=live.status;
			if(live.accounted)s._accountedMonsters.insert(live.id);
		}
		s._vertigoActors.emplace(std::move(city));
		adopt(); // The complete saved actor graph precedes checked geometry callbacks.
		xeenValidateVertigoActors(w,*s._vertigoActors);
		adopt();
	}
	adopt(); // Complete saved domain, deliberately unbound and unavailable.
	XeenActorApproach::validateEnvironment(w,s._actors,evt);
	const bool cityActive=c.mapId==XeenMapIdentity(28);
	std::optional<XeenEventFile> activeCityEvents;
	if(cityActive) {
		activeCityEvents=events(28);
	}
	const auto &active=cityActive ? s._vertigoActors.value() : s._actors;
	const auto view=cityActive ? XeenIndoorScene().classifyActors(w,c,active) : XeenActorApproach::classify(active,c);
	if (view.engaged()) throw std::invalid_argument("Quiet restore has active contact");
	if(p.monsterTreasure && p.monsterTreasure->ready() && std::none_of(view.slots.begin(),view.slots.end(),[](const auto &v){return bool(v);})) throw std::invalid_argument("Quiet treasure is immediately collectable");
	for (unsigned i=0;i<active.size();++i) if (view.activation[i] && !active[i].activated) throw std::invalid_argument("Quiet actor activation missing");
	for (auto n:XeenActorApproach::occupancy(s._actors)) if (n>3) throw std::invalid_argument("Quiet actor occupancy exceeded");
	if(s._vertigoActors)for(auto n:XeenActorApproach::occupancy(*s._vertigoActors))if(n>3)throw std::invalid_argument("Quiet city actor occupancy exceeded");
	try { presentation(w,p,c,f); }
	catch (...) { destination.check(); prepared->check(); throw; }
	destination.check(); prepared->check();
	// Prepare the complete handoff and its FINAL-owner preimage before any stores.
	auto binding = std::make_shared<XeenJourneyRestoration>();
	binding->capture.reset(new XeenJourneyCapture);
	binding->capture->admittedActors = s._actors;
	binding->statistics = std::move(statistics); binding->events = cityActive ? std::move(*activeCityEvents) : std::move(evt);
	binding->learnedNamesProvider=resources.loadLearnedSpellNames;
	binding->guard = std::make_shared<XeenRestoreGuard>(world,party,camera,flags);
	binding->guard->prepareJourneyPublication(*prepared);
	destination.check(); prepared->check();
	party.publishCompleted(p); // Same private full supplement publication; public copy rules remain closed.
	camera = c; flags = f;
	world.swapPreparedState(w);
	auto &out = world._sessionState;
	out._actors.swap(s._actors); out._vertigoActors.swap(s._vertigoActors);
	out._accountedMonsters.swap(s._accountedMonsters);
	world._cityStatistics.swap(w._cityStatistics);
	std::swap(world._cityOriginalActorCount,w._cityOriginalActorCount);
	out._entry = XeenEncounterEntry::Journey; out._encounterMarked = out._encounterInitialized = true;
	out._encounterRevision = 1; out._skeletonSeed = snapshot.journey->skeletonSeed;
	 out._journeyRandom=snapshot.journey->random;
	world._journeyRestoration.swap(binding);
	world._journeyRestoration->guard->adoptMutationBoundary();
}

bool XeenSaveState::canCapture(const XeenPartyState &party, const XeenCamera &camera,
        const XeenWorld &world) noexcept {
    return world.journeyCaptureEligible(party,camera);
}

XeenSaveSnapshot XeenSaveState::capture(const XeenSaveResourceSignature &resources,
		const XeenPartyState &party, const XeenCamera &camera,
		const XeenGameFlags &flags, const XeenWorld &world) {
	// Reject an unrelated flag owner before observing the bound graph's integrity.
	const auto authority = world._journeyCapture.lock();
	if (world.sessionState().journey()) {
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
	snapshot.food=party.food;
	snapshot.questItems = party.questItems.counts();
	snapshot.questFlags = party.questFlags.values();
	snapshot.gameFlags = flags.values();
	const auto &state = world.sessionState();
	snapshot.disabledObjects.assign(state.disabledObjects().begin(), state.disabledObjects().end());
	snapshot.disabledEvents.assign(state.disabledEvents().begin(), state.disabledEvents().end());
	snapshot.barriers=state.barriers();

	if (state.journey()) {
		xeenValidateJourneyParty(party);
		XeenSaveJourney j;
		j.context = party.encounterContext; j.skeletonSeed = state.skeletonSeed();
		j.content=XeenSaveFormat::kJourneyContent; j.schema=XeenSaveFormat::kJourneySchema; j.random=state.journeyRandom();j.treasure=party.monsterTreasure;j.regionalRecovery=party.regionalRecovery;
		j.serviceEconomy=party.serviceEconomy;
		for (unsigned i = 0; i < 30; ++i) j.supplements[i] = {static_cast<std::uint8_t>(i), *party.roster.combatInputs(i)};
		j.initializedMap=xeenJourneyContent().entry.mapId;
		// Admitted MOB topology survives authorized eviction of resource caches.
		j.originalActorCount=authority->admittedActors.size();
		if (state.actors().size() != j.originalActorCount) throw std::logic_error("Journey actor collection changed");
		for (const auto &a:state.actors())
			j.actors.push_back({a.id,a.x,a.y,a.hp,a.activated,a.lifecycle,a.status,state.accountedMonsters().count(a.id) != 0});
		if (state._vertigoActors) {
			j.cityOriginalActorCount=world._cityOriginalActorCount;
			std::vector<XeenSaveJourneyActor> city;city.reserve(state._vertigoActors->size());
			for (const auto &a:*state._vertigoActors) {
				XeenSaveJourneyActor saved{a.id,a.x,a.y,a.hp,a.activated,a.lifecycle,a.status,state.accountedMonsters().count(a.id)!=0};
				if(a.id.recordIndex>=j.cityOriginalActorCount && a.original.hasResource())
					saved.spawnedType=static_cast<std::int16_t>(a.original.resourceId);
				city.push_back(saved);
			}
			j.vertigoActors=std::move(city);
		}
		snapshot.journey = std::move(j);
		validateJourneyValues(snapshot);
	}
	XeenSaveFormat::validate(snapshot);
	return snapshot;
}

void XeenSaveState::restoreBeforeGameplay(const XeenSaveSnapshot &snapshot,
		const Resources &resources, XeenPartyState &party, XeenCamera &camera,
		XeenGameFlags &flags, XeenWorld &world, const Preflight &preflight) {
	if (world.hasEncounterState() || party.encounterContext || party.monsterTreasure || party.serviceEconomy || party.roster.combatMarked())
		throw std::logic_error("MMModern save: cannot restore into encounter owners");
	XeenSaveFormat::validate(snapshot);
	if (!(snapshot.resources == resources.signature))
		throw std::runtime_error("MMModern save: archive/data-edition incompatibility (original archive contents differ)");
	restoreJourney(snapshot,resources,party,camera,flags,world,preflight);
}

} // namespace mmodern
