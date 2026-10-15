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
	p.worldFlags = snapshot.journey->worldFlags;
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
	if(!snapshot.objectOverrides.empty()) {w.restoreObjectOverrides(snapshot.objectOverrides);adopt();}
	if(!snapshot.barriers.empty()) {w.restoreBarriers(snapshot.barriers);adopt();}
	if(!snapshot.exploredTiles.empty()) {w.restoreExploredTiles(snapshot.exploredTiles);adopt();}
	w.restoreHeads(snapshot.headCache,c);adopt();
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
		xeenRefillLoadedMonster(a); // Decision 8: saved HP and wake state are ignored.
	}
	static_cast<void>(CloudsUiComposer::buildPortraitPlacements(p));
	auto &s = w._sessionState;
	w._cityStatistics=statistics;
	// v12 projection: the Journey actors are the entry map's monster table.
	{
		const auto originalCount=actors.size();
		s.stageMonsters(policy.entry.mapId,std::move(actors),originalCount,w.map(policy.entry.mapId).geometry.isOutdoors());
	}
	s._entry = XeenEncounterEntry::Journey;
	s._encounterMarked = s._encounterInitialized = true; s._encounterRevision = 1;
	s._skeletonSeed = snapshot.journey->skeletonSeed;
	 s._journeyRandom=snapshot.journey->random;
	for (const auto &live:snapshot.journey->actors) if (live.accounted) s._accountedMonsters.insert(live.id);
	adopt(); // Base Journey values are complete before optional regional resources.
	// v12 projection: the optional city table is map 28's monster table.
	const XeenMapIdentity savedCity(28);
	if (snapshot.journey->vertigoActors) {
		const auto cityEvents=events(savedCity);
		const auto cityMob=callback([&] { return w.objectFile(savedCity); });
		const bool cityOutdoors=callback([&] { return w.map(savedCity).geometry.isOutdoors(); });
		auto city=XeenActorApproach::actorsFromResources(cityMob,statistics);
		if(city.size()!=snapshot.journey->cityOriginalActorCount)
			throw std::invalid_argument("Saved city original actor count differs from MOB");
		w._cityStatistics=statistics;
		const auto savedCount=snapshot.journey->vertigoActors->size();
		if(savedCount<city.size() || savedCount>XeenActorApproach::kCapacity)
			throw std::invalid_argument("Saved city actor collection loses original records");
		while(city.size()<savedCount) {
			XeenActor a;a.id={savedCity,city.size()};a.original={};
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
			xeenRefillLoadedMonster(a); // Decision 8: saved HP and wake state are ignored.
			if(live.accounted)s._accountedMonsters.insert(live.id);
		}
		s.stageMonsters(savedCity,std::move(city),snapshot.journey->cityOriginalActorCount,cityOutdoors);
		adopt(); // The complete saved actor graph precedes checked geometry callbacks.
		xeenValidateIndoorActors(w,savedCity,s.mutableMonsters(savedCity));
		adopt();
	}
	adopt(); // Complete saved domain, deliberately unbound and unavailable.
	XeenActorApproach::validateEnvironment(w,s.mutableMonsters(policy.entry.mapId),evt);
	// The camera map's own table and Events, as every reader resolves them.
	const bool entryActive=c.mapId==policy.entry.mapId;
	std::optional<XeenEventFile> activeEvents;
	if(!entryActive) {
		activeEvents=events(c.mapId);
	}
	auto &active=s.mutableMonsters(c.mapId);
	const auto view=!s.mapState(c.mapId)->outdoors ? XeenIndoorScene().classifyActors(w,c,active) : XeenActorApproach::classify(active,c);
	if (view.engaged()) throw std::invalid_argument("Quiet restore has active contact");
	// The game load wakes what the arrival view sees, as a map entry does.
	for (unsigned i=0;i<active.size();++i) if (view.activation[i]) active[i].activated=true;
	adopt();
	if(p.monsterTreasure && p.monsterTreasure->ready() && std::none_of(view.slots.begin(),view.slots.end(),[](const auto &v){return bool(v);})) throw std::invalid_argument("Quiet treasure is immediately collectable");
	for (unsigned i=0;i<active.size();++i) if (view.activation[i] && !active[i].activated) throw std::invalid_argument("Quiet actor activation missing");
	for (const auto &entry:s.mapStates()) if (entry.second.monsters)
		for (auto n:XeenActorApproach::occupancy(*entry.second.monsters)) if (n>3) throw std::invalid_argument("Quiet actor occupancy exceeded");
	try { presentation(w,p,c,f); }
	catch (...) { destination.check(); prepared->check(); throw; }
	destination.check(); prepared->check();
	// Prepare the complete handoff and its FINAL-owner preimage before any stores.
	auto binding = std::make_shared<XeenJourneyRestoration>();
	binding->capture.reset(new XeenJourneyCapture);
	binding->capture->admittedActors = s.monsters(policy.entry.mapId);
	binding->statistics = std::move(statistics); binding->events = entryActive ? std::move(evt) : std::move(*activeEvents);
	binding->learnedNamesProvider=resources.loadLearnedSpellNames;
	binding->guard = std::make_shared<XeenRestoreGuard>(world,party,camera,flags);
	binding->guard->prepareJourneyPublication(*prepared);
	destination.check(); prepared->check();
	party.publishCompleted(p); // Same private full supplement publication; public copy rules remain closed.
	camera = c; flags = f;
	world.swapPreparedState(w); // Includes every staged monster table.
	auto &out = world._sessionState;
	out._accountedMonsters.swap(s._accountedMonsters);
	world._cityStatistics.swap(w._cityStatistics);
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
	return captureImpl(resources,party,camera,flags,world,false);
}
XeenSaveSnapshot XeenSaveState::captureInitialized(const XeenSaveResourceSignature &resources,
		const XeenPartyState &party, const XeenCamera &camera,
		const XeenGameFlags &flags, const XeenWorld &world) {
	return captureImpl(resources,party,camera,flags,world,true);
}
namespace {
// Decision 8: HP and wake state are rebuilt on every map and game load, so the
// v12 fields carry a live monster's MON HP and false, never its live values.
XeenSaveJourneyActor projectedActor(const XeenActor &a,bool accounted) {
	auto loaded=a;xeenRefillLoadedMonster(loaded);
	return {a.id,a.x,a.y,loaded.hp,loaded.activated,a.lifecycle,a.status,accounted};
}
}

XeenSaveSnapshot XeenSaveState::captureImpl(const XeenSaveResourceSignature &resources,
		const XeenPartyState &party, const XeenCamera &camera,
		const XeenGameFlags &flags, const XeenWorld &world, bool initializing) {
	// Reject an unrelated flag owner before observing the bound graph's integrity.
	const auto authority = world._journeyCapture.lock();
	if (world.sessionState().journey()) {
		if (!authority || !authority->preimage || &(**authority->preimage).f != &flags)
			throw std::logic_error("Journey capture requires the bound game-flag owner");
	}
	if (initializing ? (!authority || !authority->current(party,camera,true)) : !canCapture(party, camera, world))
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
	// v12 projection. The layout names two logical maps, the Journey entry and
	// the city (28); their physical tiles keep their own cells and explored bits.
	const auto entry=xeenJourneyContent().entry.mapId;
	const XeenMapIdentity city(28);
	for (const auto &value:state.mapStates()) {
		const auto &mapState=value.second;
		if (value.first!=entry && value.first!=city && (mapState.monsters || !mapState.removedObjects.empty() ||
			!mapState.disabledEvents.empty() || !mapState.objectOverrides.empty()))
			throw std::logic_error("MMModern save: v12 cannot hold the state of this map");
	}
	{
		const auto objects=state.disabledObjects();const auto events=state.disabledEvents();
		snapshot.disabledObjects.assign(objects.begin(),objects.end());
		snapshot.disabledEvents.assign(events.begin(),events.end());
	}
	snapshot.barriers=state.barriers();
	snapshot.objectOverrides=state.objectOverrides();
	snapshot.exploredTiles=state.exploredTiles();
	snapshot.headCache=state.headCache();
	for(auto &v:snapshot.objectOverrides)v.flipped=world.effectiveObjectFlip(v.id);

	if (state.journey()) {
		xeenValidateJourneyParty(party);
		XeenSaveJourney j;
		j.context = party.encounterContext; j.skeletonSeed = state.skeletonSeed();
		j.content=XeenSaveFormat::kJourneyContent; j.schema=XeenSaveFormat::kJourneySchema; j.random=state.journeyRandom();j.treasure=party.monsterTreasure;j.worldFlags=party.worldFlags;
		j.serviceEconomy=party.serviceEconomy;
		for (unsigned i = 0; i < 30; ++i) j.supplements[i] = {static_cast<std::uint8_t>(i), *party.roster.combatInputs(i)};
		j.initializedMap=entry;
		// Admitted MOB topology survives authorized eviction of resource caches.
		j.originalActorCount=authority->admittedActors.size();
		const auto mainland=state.monsters(entry);
		if (mainland.size() != j.originalActorCount) throw std::logic_error("Journey actor collection changed");
		for (const auto &a:mainland)
			j.actors.push_back(projectedActor(a,state.accountedMonsters().count(a.id) != 0));
		if (state.hasRegionalActors(city)) {
			const auto &cityState=*state.mapState(city);
			j.cityOriginalActorCount=cityState.originalMonsterCount;
			std::vector<XeenSaveJourneyActor> cityActors;cityActors.reserve(cityState.monsters->size());
			for (const auto &a:*cityState.monsters) {
				auto saved=projectedActor(a,state.accountedMonsters().count(a.id)!=0);
				if(a.id.recordIndex>=j.cityOriginalActorCount && a.original.hasResource())
					saved.spawnedType=static_cast<std::int16_t>(a.original.resourceId);
				cityActors.push_back(saved);
			}
			j.vertigoActors=std::move(cityActors);
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
