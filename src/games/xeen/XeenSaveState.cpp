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

#include <iostream>
#include <stdexcept>
#include <algorithm>
#include <type_traits>
#include <map>
#include <optional>
#include <utility>

namespace mmodern {

bool XeenWorld::journeyCaptureEligible(const XeenPartyState &party, const XeenCamera &camera) const noexcept {
	if (_developerEventMap) return false;
	const auto retained = _journeyCapture.lock();
	return retained && retained->current(party,camera);
}

namespace {
// Older builds kept Spawn slots past a map's MOB count, so their saves may hold
// pending treasure from a monster DOS never had (count byte 0x9c59, which Spawn
// never raises). Restoration drops those slots; drop what they left pending too.
XeenMonsterTreasure withoutBeyondCountSources(const XeenSaveSnapshot &snapshot, XeenMonsterTreasure value) {
	std::uint32_t dropped = 0;
	for (unsigned k = 0; k < value.sources.size(); ++k) {
		const auto id = value.sources[k];
		const auto *map = id.mapId ? snapshot.find(id.mapId) : nullptr;
		if (!map || !map->monsters || id.recordIndex < map->monsters->originalCount) continue;
		if (!(value.referencedSources() & (1u << k))) continue;
		dropped |= 1u << k;
		std::cerr << "Warning: discarded pending monster treasure from "
			<< (id.mapId.side == XeenSide::Clouds ? "Clouds" : "Darkside") << " map " << id.mapId.number
			<< " slot " << id.recordIndex << ", past its MOB monster count\n";
	}
	if (!dropped) return value;
	for (unsigned k = 0; k < 12; ++k) if ((dropped & value.pendingMask) & (1u << k)) {
		value.pendingMask = value.pendingMask & ~(1u << k);
		value.pendingGold = value.pendingGold - 10;
	}
	for (auto *entries : {&value.weapons, &value.armor, &value.miscellaneous}) {
		std::array<XeenMonsterTreasureItem,10> kept{};
		unsigned n = 0;
		for (const auto &entry : *entries)
			if (entry.item.id && !(entry.source < 32 && (dropped & (1u << entry.source)))) kept[n++] = entry;
		*entries = kept;
	}
	xeenPruneMonsterTreasureSources(value);
	return value;
}

// A barrier override as the generic effective word/attribute pair of its cell.
XeenSaveCellOverride savedCell(const XeenBarrierOverride &v) {
	return {v.cell, v.word(false), v.attributes(), v.originalWord, v.originalAttributes};
}

// The live cell owner for doors and world Events. The wire remains v13.
XeenBarrierOverride restoredBarrier(XeenMapIdentity tile, const XeenSaveCellOverride &v) {
	XeenBarrierOverride result;
	result.tile = tile; result.cell = v.cell; result.originalWord = v.originalWord; result.originalAttributes = v.originalAttributes;
	for (unsigned d = 0; d < 4; ++d) {
		const auto shift = 12 - 4 * d;
		const auto wall = (v.word >> shift) & 15u, original = (v.originalWord >> shift) & 15u;
		if (wall != original) { result.mask = static_cast<std::uint8_t>(result.mask | (1u << d)); result.walls[d] = static_cast<std::uint8_t>(wall); }
	}
	if (v.attributes != v.originalAttributes) {
		if ((v.attributes&0x78)!=(v.originalAttributes&0x78) || ((v.originalAttributes&0x80) && !(v.attributes&0x80)))
			throw std::invalid_argument("Saved cell flag change is not supported yet");
		result.unlocked=(v.attributes&0x80) && !(v.originalAttributes&0x80);
		result.surfaceChanged=(v.attributes&7)!=(v.originalAttributes&7);
		if(result.surfaceChanged)result.surface=v.attributes&7;
	}
	return result;
}
}

void XeenSaveState::validateJourneyValues(const XeenSaveSnapshot &s) {
	const auto &j = *s.journey;
	const auto require = [](bool ok) { if (!ok) throw std::invalid_argument("Unsupported Journey durable state"); };
	require(j.schema==XeenSaveFormat::kJourneySchema && j.content==XeenSaveFormat::kJourneyContent);
	require(j.context && xeenRegionalContext(*j.context));
	require(bool(j.serviceEconomy));
	if (j.serviceEconomy) {
		xeenValidateCurrentServiceEconomy(*j.serviceEconomy);
	}
	require(bool(s.resources.darkside));
	// Decision 6: Darkside identities are representable, not loadable.
	if (s.camera.mapId.side!=XeenSide::Clouds)
		throw std::invalid_argument("Darkside map state cannot be loaded yet");
	for (const auto &m:s.maps) if (m.id.side!=XeenSide::Clouds)
		throw std::invalid_argument("Darkside map state cannot be loaded yet");
	// Decision 2: the Warzone (Clouds 85) is never written back.
	for (const auto &m:s.maps) if (!xeenRetainsMapState(m.id))
		throw std::invalid_argument("Saved Warzone state is not part of a save");
	// The camera map's own table; no live monster stands on the camera cell.
	const auto *camera=s.find(s.camera.mapId);
	require(camera && camera->monsters);
	for (const auto &m:s.maps) if (m.monsters) for (const auto &a:m.monsters->slots) {
		require(a.status==XeenActorStatus::Physical);
		require(m.id!=s.camera.mapId || a.lifecycle!=XeenActorLifecycle::Present || a.x!=s.camera.x || a.y!=s.camera.y);
	}
	// Topology/profile/closure validation needs the installation during restore.
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
	// The saved treasure names its sources in identity order; the live owner
	// keeps each in its production slot.
	p.monsterTreasure=xeenLiveMonsterTreasure(withoutBeyondCountSources(snapshot,*snapshot.journey->treasure));
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
	// The per-map overlays, flattened in identity order; the World binds each
	// to its own map's resources. A removed object is its (-128,-128) entry.
	std::vector<XeenObjectIdentity> removedObjects;
	std::vector<XeenObjectOverride> movedObjects;
	std::vector<XeenEventIdentity> disabledEvents;
	std::vector<XeenEventOverride> replacedEvents;
	std::vector<XeenWallItemOverride> wallItems;
	std::vector<XeenBarrierOverride> cells;
	std::vector<XeenExploredTile> explored;
	for (const auto &m:snapshot.maps) {
		for (const auto &v:m.objects) {
			const XeenObjectIdentity id{m.id,v.record};
			if (v.x==-128 && v.y==-128) removedObjects.push_back(id);
			else movedObjects.push_back({id,v.x,v.y,v.flipped});
		}
		for (const auto &v:m.events) {
			if(v.opcode)replacedEvents.push_back({{m.id,v.record},v.opcode});
			else disabledEvents.push_back({m.id,v.record});
		}
		for (const auto &v:m.wallItems) wallItems.push_back({{m.id,v.record},v.x,v.y});
		for (const auto &v:m.cells) cells.push_back(restoredBarrier(m.id,v));
		if (m.explored) explored.push_back({m.id,*m.explored});
	}
	w.restoreSessionState(removedObjects,disabledEvents,events);
	adopt(); // Only checked overlay preparation changed candidate gameplay values.
	if(!wallItems.empty()) {w.restoreWallItemOverrides(wallItems);adopt();}
	if(!replacedEvents.empty()) {w.restoreEventOverrides(replacedEvents,events);adopt();}
	if(!movedObjects.empty()) {w.restoreObjectOverrides(movedObjects);adopt();}
	if(!cells.empty()) {w.restoreBarriers(cells);adopt();}
	if(!explored.empty()) {w.restoreExploredTiles(explored);adopt();}
	for (const auto &m:snapshot.maps) if (m.tavernTips) {
		// Resolve all resource callbacks against the retained preimage before
		// publishing this candidate's map-owned cursor; never advance a tip.
		static_cast<void>(callback([&] { return w.map(m.id).geometry.id; }));
		w.setTavernTipIndex(m.id,*m.tavernTips);adopt();
	}
	w.restoreHeads(snapshot.headCache,c);adopt();
	auto statistics = callback(monsters);
	const auto &policy=xeenJourneyContent();
	// The Vertigo transition's two destinations remain the only admitted
	// camera maps until whole maps are admitted.
	const XeenMapIdentity city(28);
	auto evt = events(policy.entry.mapId);
	{
		if(!resources.loadInitialCharacters)
			throw std::invalid_argument("Missing Training restoration resources");
		xeenValidateTrainingSource(callback(resources.loadInitialCharacters));
		// Bind resources even when the saved Journey has never entered the city.
		adopt();
		const auto cityEvents=events(city);
		if(!cityEvents.resourcePresent || cityEvents.mapId!=city)
			throw std::invalid_argument("Missing city Event resource");
	}
	{
		if (!resources.loadRegionalText) throw std::invalid_argument("Missing regional text restoration provider");
		prepared->admitRegionalText(callback([&] { return resources.loadRegionalText(policy.entry.mapId); }));
	}
	{
		if (!resources.loadLearnedSpellNames) throw std::invalid_argument("Missing learned spell names restoration provider");
		prepared->admitLearnedSpellNames(callback(resources.loadLearnedSpellNames));
	}
	{
		if (!regionalManifest) throw std::invalid_argument("Missing regional restoration manifest");
		callback([&] { regionalManifest(w.map(policy.entry.mapId),w.objectFile(policy.entry.mapId),evt,statistics);return true; });
		if (c.mapId!=policy.entry.mapId && c.mapId!=city)
			throw std::invalid_argument("Saved camera map is not admitted");
		if (w.map(c.mapId).geometry.isOutdoors() ? (c.x>15 || c.y>15) : !w.sampleCell(c.mapId,c.x,c.y))
			throw std::invalid_argument("Saved camera outside its map");
		if (c.mapId==policy.entry.mapId &&
			!XeenMovement::component(w.map(policy.entry.mapId),9,11,{true,true,false})[c.y*16+c.x])
			throw std::invalid_argument("Regional camera outside mainland");
	}
	const auto *entryState=snapshot.find(policy.entry.mapId);
	if (!entryState || !entryState->monsters) throw std::invalid_argument("Journey requires complete original actor collection");
	static_cast<void>(CloudsUiComposer::buildPortraitPlacements(p));
	auto &s = w._sessionState;
	w._cityStatistics=statistics;
	s._entry = XeenEncounterEntry::Journey;
	s._encounterMarked = s._encounterInitialized = true; s._encounterRevision = 1;
	s._skeletonSeed = snapshot.journey->skeletonSeed;
	 s._journeyRandom=snapshot.journey->random;
	adopt(); // Base Journey values are complete before the retained monster tables.
	// Every retained monster table: its MOB records. HP
	// and wake state are rebuilt as a map load does (decision 8). All provider
	// calls precede the stores, so one adoption covers every staged table.
	struct Loaded { const XeenSaveMapState *saved; XeenObjectFile mob; bool outdoors; };
	std::vector<Loaded> tables;
	for (const auto &m:snapshot.maps) if (m.monsters) {
		auto mob=callback([&] { return w.objectFile(m.id); });
		const bool outdoors=callback([&] { return w.map(m.id).geometry.isOutdoors(); });
		tables.push_back({&m,std::move(mob),outdoors});
	}
	for (const auto &table:tables) {
		const auto &m=*table.saved;
		auto actors=XeenActorApproach::actorsFromResources(table.mob,statistics);
		if (actors.size()!=m.monsters->originalCount)
			throw std::invalid_argument("Saved original monster count differs from MOB");
		// Slots past the MOB count never survive a DOS save or map load (writer
		// 0x266e3..0x2671c, load 0x0f5be..0x0f5c7); drop any an older build saved.
		for (std::size_t i=0;i<actors.size();++i) {
			const auto &saved=m.monsters->slots[i];auto &a=actors[i];
			a.x=saved.x;a.y=saved.y;a.lifecycle=saved.lifecycle;a.status=saved.status;a.activated=false;
			a.hp=(saved.lifecycle==XeenActorLifecycle::Present || saved.lifecycle==XeenActorLifecycle::Disabled) && a.statistics ?
				std::int32_t(a.statistics->baseHp()) : 0;
			if (saved.accounted) s._accountedMonsters.insert(a.id);
		}
		s.stageMonsters(m.id,std::move(actors),m.monsters->originalCount,table.outdoors);
	}
	adopt(); // The complete saved tables precede checked geometry callbacks.
	for (const auto &table:tables) {
		const auto id=table.saved->id;
		const auto mapEvents=id==policy.entry.mapId ? evt : events(id);
		XeenActorApproach::validateEnvironment(w,s.mutableMonsters(id),mapEvents);
	}
	adopt();
	// Each treasure source is a defeated, accounted monster of its own map.
	for (const auto &id:p.monsterTreasure->sources) if (id.mapId) {
		const auto &a=s.mutableMonsters(id.mapId).at(id.recordIndex);
		if (a.lifecycle!=XeenActorLifecycle::Defeated || !s._accountedMonsters.count(a.id))
			throw std::invalid_argument("Noncanonical monster treasure source");
	}
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

XeenSaveSnapshot XeenSaveState::captureImpl(const XeenSaveResourceSignature &resources,
		const XeenPartyState &party, const XeenCamera &camera,
		const XeenGameFlags &flags, const XeenWorld &world, bool initializing) {
	if (world.developerEventMap()) throw std::logic_error("Saving is disabled in the Event developer session");
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
	snapshot.headCache=state.headCache();
	// v13: every retained map's state, in identity order; an empty entry is
	// equivalent to an absent one and is not written.
	for (const auto &entry:state.mapStates()) {
		if (!xeenRetainsMapState(entry.first)) continue; // Decision 2: never written back.
		const auto &live=entry.second;
		XeenSaveMapState saved;saved.id=entry.first;
		const auto record=[](std::size_t index) {
			if (index>XeenSaveFormat::kMaximumOverrides) throw std::logic_error("MMModern save: record index exceeds the save representation");
			return static_cast<std::uint16_t>(index);
		};
		if (live.monsters) {
			XeenSaveMonsterTable table;table.originalCount=live.originalMonsterCount;
			// Only MOB records persist, as the DOS writer stores; no script slots.
			if (live.monsters->size()!=table.originalCount) throw std::logic_error("Monster table differs from its MOB records");
			for (const auto &a:*live.monsters) {
				if (!xeenActorStatusSupported(a.status)) throw std::invalid_argument("Unsupported live monster status");
				table.slots.push_back({a.x,a.y,a.lifecycle,XeenActorStatus::Physical,state.accountedMonsters().count(a.id)!=0,-1});
			}
			saved.monsters=std::move(table);
		}
		std::map<std::size_t,XeenSaveObjectOverride> objects;
		for (const auto &v:live.objectOverrides)
			objects[v.id.recordIndex]={record(v.id.recordIndex),v.x,v.y,world.effectiveObjectFlip(v.id)};
		for (const auto &id:live.removedObjects) objects[id.recordIndex]={record(id.recordIndex),-128,-128,false};
		for (const auto &value:objects) saved.objects.push_back(value.second);
		for (const auto &v:live.cells) {
			const auto cell=savedCell(v);
			if (cell.word!=cell.originalWord || cell.attributes!=cell.originalAttributes) saved.cells.push_back(cell);
		}
		for (const auto &v:live.wallItemOverrides) saved.wallItems.push_back({record(v.id.recordIndex),v.x,v.y});
		std::map<std::size_t,std::uint8_t> events;
		for (const auto &v:live.eventOverrides) events[v.id.recordIndex]=v.opcode;
		for (const auto &id:live.disabledEvents) events[id.recordIndex]=0;
		for (const auto &v:events) saved.events.push_back({record(v.first),v.second});
		saved.explored=live.explored;
		if(live.tavernTips)saved.tavernTips=std::uint8_t(*live.tavernTips);
		if (!saved.empty()) snapshot.maps.push_back(std::move(saved));
	}

	if (state.journey()) {
		xeenValidateJourneyParty(party);
		XeenSaveJourney j;
		j.context = party.encounterContext; j.skeletonSeed = state.skeletonSeed();
		j.content=XeenSaveFormat::kJourneyContent; j.schema=XeenSaveFormat::kJourneySchema; j.random=state.journeyRandom();j.treasure=party.monsterTreasure;j.worldFlags=party.worldFlags;
		j.serviceEconomy=party.serviceEconomy;
		for (unsigned i = 0; i < 30; ++i) j.supplements[i] = {static_cast<std::uint8_t>(i), *party.roster.combatInputs(i)};
		// Admitted MOB topology survives authorized eviction of resource caches.
		if (state.monsters(xeenJourneyContent().entry.mapId).size()!=authority->admittedActors.size())
			throw std::logic_error("Journey actor collection changed");
		// The live treasure names its sources by monster identity.
		if (j.treasure) j.treasure=xeenCanonicalMonsterTreasure(*j.treasure);
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
