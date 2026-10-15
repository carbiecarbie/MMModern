#include "games/xeen/XeenWorld.h"
#include "games/xeen/XeenActorApproach.h"
#include "games/xeen/XeenRegionalRules.h"
#include "games/xeen/XeenIndoorScene.h"
#include <bitset>
#include <stdexcept>

namespace mmodern {

XeenTransitionResult XeenWorld::prepareEventTransition(const XeenCamera &destination) {
	if (regionalJourney() && destination.mapId!=XeenMapIdentity(23) &&
		destination.mapId!=XeenMapIdentity(28) && destination.mapId!=_developerEventMap)
		return {XeenTransitionResult::Status::Unavailable,destination,"Destination map is not admitted yet"};
	const auto &resource=map(destination.mapId);
	const int limit=resource.geometry.isOutdoors()?16:32;
	if (unsigned(destination.direction)>3 || destination.x<0 || destination.y<0 ||
		destination.x>=limit || destination.y>=limit || !sampleCell(destination.mapId,destination.x,destination.y))
		return {XeenTransitionResult::Status::Unavailable,destination,"Destination has no map cell"};
	if (regionalJourney() && destination.mapId!=_developerEventMap && destination.mapId==XeenMapIdentity(23) &&
		!XeenMovement::component(resource,9,11,xeenJourneyContent().traversal)[destination.y*16+destination.x])
		return {XeenTransitionResult::Status::Unavailable,destination,"Destination is outside the admitted mainland"};
	return {XeenTransitionResult::Status::Available,destination,{}};
}

XeenActorView XeenWorld::prepareTransitionArrival(const XeenCamera &camera) {
	if (!_detachedEventCandidate) throw std::logic_error("Arrival requires a detached transition");
	auto &actors=_sessionState.mutableMonsters(camera.mapId);
	const auto view=!_sessionState.mapState(camera.mapId)->outdoors ? XeenIndoorScene().classifyActors(*this,camera,actors) :
		XeenActorApproach::classify(actors,camera);
	for (unsigned i=0;i<actors.size();++i) if (view.activation[i]) actors[i].activated=true;
	return view;
}

void XeenWorld::reloadMapMonsters(XeenMapIdentity mapId) {
	if (!_detachedEventCandidate) throw std::logic_error("Map load requires a detached transition");
	for (auto &actor:_sessionState.mutableMonsters(mapId)) xeenRefillLoadedMonster(actor);
}

void XeenWorld::leaveMap(XeenMapIdentity mapId) {
	if (!_detachedEventCandidate) throw std::logic_error("Map exit requires a detached transition");
	if (xeenRetainsMapState(mapId)) return;
	XeenMutationWatch::write(this);
	for (auto i=_sessionState._accountedMonsters.begin();i!=_sessionState._accountedMonsters.end();)
		i=i->mapId==mapId ? _sessionState._accountedMonsters.erase(i) : std::next(i);
	_sessionState._mapStates.leave(mapId);
}

void XeenWorld::stageIndoorMonsters(const XeenObjectFile &mob,
		const std::vector<XeenMonsterRecord> &statistics) {
	if (map(mob.mapId).geometry.isOutdoors()) throw std::logic_error("Indoor monster staging is unavailable");
	stageMapMonsters(mob,statistics);
}

void XeenWorld::stageMapMonsters(const XeenObjectFile &mob,
		const std::vector<XeenMonsterRecord> &statistics) {
	XeenMutationWatch::write(this);
	if (_sessionState._entry!=XeenEncounterEntry::Ordinary || !mob.mapId ||
		_sessionState.hasRegionalActors(mob.mapId))
		throw std::logic_error("Indoor monster staging is unavailable");
	auto actors=XeenActorApproach::actorsFromResources(mob,statistics);
	_cityStatistics=statistics;
	xeenValidateIndoorActors(*this,mob.mapId,actors);
	const bool outdoors=map(mob.mapId).geometry.isOutdoors();
	if (outdoors && mob.mapId!=_developerEventMap) throw std::logic_error("Outdoor map is not the selected developer map");
	_sessionState.stageMonsters(mob.mapId,std::move(actors),mob.entities.monsters.size(),outdoors);
}

void XeenWorld::applySpawn(XeenMapIdentity mapId, std::uint8_t slot, int x, int y, std::uint8_t) {
	XeenMutationWatch::write(this);
	auto *state=_sessionState._mapStates.find(mapId);
	if (_sessionState._entry!=XeenEncounterEntry::Ordinary ||
		!state || !state->monsters || state->outdoors || !xeenIndoorCoordinate(x,y) ||
		slot>=XeenActorApproach::kCapacity || _cityStatistics.empty())
		throw std::invalid_argument("Invalid Spawn operands or owner");
	auto &actors=*state->monsters;
	while(actors.size()<=slot) {
		XeenActor a;a.id={mapId,actors.size()};a.original={};
		actors.push_back(std::move(a));
	}
	auto &a=actors[slot];
	// cmdSpawn reuses the sprite ID; newly constructed slots use type zero.
	// Intermediate resize slots remain unresolved until explicitly spawned.
	const auto type=a.original.hasResource()?int(a.original.resourceId):0;
	if(unsigned(type)>=_cityStatistics.size()) throw std::invalid_argument("Spawn monster type is absent");
	if(!a.original.hasResource()) a.original.resourceId=type;
	a.statistics=_cityStatistics[type];
	a.x=x;a.y=y;a.hp=a.statistics->baseHp();a.activated=false;
	a.lifecycle=XeenActorLifecycle::Present;a.status=XeenActorStatus::Physical;
	_sessionState._accountedMonsters.erase(a.id);
	// Approved M48/M50 policy: cmdSpawn's random frame uses cosmetic RNG.
	_scenePresentation.spawn(a);
	_spawnedPresentationSlots.insert(a.id);
}

XeenScenePresentation XeenWorld::prepareSpawnPresentation(const XeenWorld &candidate) const {
	auto prepared=_scenePresentation;
	prepared.objectFlips=candidate._scenePresentation.objectFlips;
	for(const auto id:candidate._spawnedPresentationSlots)
		prepared.copySpawn(candidate._sessionState.regionalActors(id.mapId).at(id.recordIndex),candidate._scenePresentation);
	if(!candidate._spawnedPresentationSlots.empty())prepared.copySpawnRandom(candidate._scenePresentation);
	return prepared;
}

void xeenValidateIndoorActors(XeenWorld &world,XeenMapIdentity mapId,const std::vector<XeenActor> &actors) {
	const auto &mob=world.objectFile(mapId);
	const auto &map=world.map(mapId);
	if(!world.regionalJourney() || !mob.resourcePresent || mob.mapId!=mapId ||
		actors.size()<mob.entities.monsters.size() || actors.size()>XeenActorApproach::kCapacity ||
		world._cityStatistics.empty()) throw std::invalid_argument("Invalid resource-bound city actor collection");
	for(unsigned i=0;i<actors.size();++i) {
		const auto &a=actors[i];
		if(!(a.id==XeenMonsterIdentity{mapId,i}) || a.status!=XeenActorStatus::Physical)
			throw std::invalid_argument("Vertigo actor identity/status changed");
		if(i<mob.entities.monsters.size()) {
			const auto &original=mob.entities.monsters[i];
			if(a.original.x!=original.x || a.original.y!=original.y ||
				a.original.direction!=original.direction || a.original.tableIndex!=original.tableIndex ||
				a.original.resourceId!=original.resourceId)
				throw std::invalid_argument("Original Vertigo actor identity changed");
		} else if(a.original.x || a.original.y || a.original.direction || a.original.tableIndex ||
			(a.original.resourceId!=-1 && a.original.resourceId!=0))
			throw std::invalid_argument("Script-created city identity changed");
		if(a.lifecycle==XeenActorLifecycle::Unresolved) {
			const bool position=i<mob.entities.monsters.size() ?
				(a.x==a.original.x && a.y==a.original.y) : (!a.x && !a.y);
			if(a.statistics || !position || a.hp || a.activated ||
				a.original.hasResource() || world.sessionState().accountedMonsters().count(a.id))
				throw std::invalid_argument("Vertigo gap slot is materialized");
			continue;
		}
		if (a.lifecycle==XeenActorLifecycle::Disabled && !a.original.hasResource()) {
			if (i>=mob.entities.monsters.size() || !a.original.isDisabled() || a.x!=a.original.x || a.y!=a.original.y ||
				a.statistics || a.hp || a.activated || world.sessionState().accountedMonsters().count(a.id))
				throw std::invalid_argument("Disabled original placeholder changed");
			continue;
		}
		if(!a.statistics)throw std::invalid_argument("Vertigo actor statistics are absent");
		if (a.lifecycle==XeenActorLifecycle::Present &&
			(!xeenMonsterMovementSupported(*a.statistics,a.original.resourceId) || !a.statistics->supportsRendering()))
			throw std::invalid_argument("Map actor mechanics are unavailable");
		if(!a.original.hasResource() || unsigned(a.original.resourceId)>=world._cityStatistics.size() ||
			a.statistics->raw!=world._cityStatistics[a.original.resourceId].raw)
			throw std::invalid_argument("City actor MON resource binding changed");
		const bool accounted=world.sessionState().accountedMonsters().count(a.id)!=0;
		// Original MOBs also contain off-map parking slots for later Spawn, and
		// records beyond the available DAT footprint. Retain exact source values;
		// this does not admit a moved actor outside valid geometry.
		if (i<mob.entities.monsters.size() && a.lifecycle==XeenActorLifecycle::Present &&
			a.x==a.original.x && a.y==a.original.y &&
			(!xeenIndoorCoordinate(a.x,a.y) || !world.sampleCell(mapId,a.x,a.y)) &&
			a.hp==a.statistics->baseHp() && !a.activated && !accounted) continue;
		if(a.lifecycle==XeenActorLifecycle::Defeated) {
				if(a.x!=-128 || a.y!=-128 || a.hp || a.activated || !accounted)
					throw std::invalid_argument("Defeated city actor is noncanonical");
		} else if(a.lifecycle==XeenActorLifecycle::Disabled) {
			if(!a.original.isDisabled() || a.x!=a.original.x || a.y!=a.original.y ||
				a.hp!=a.statistics->baseHp() || a.activated || accounted)
				throw std::invalid_argument("Disabled city actor is noncanonical");
		} else if(a.lifecycle!=XeenActorLifecycle::Present || a.hp<1 || a.hp>a.statistics->baseHp() ||
			accounted || !xeenIndoorCoordinate(a.x,a.y) || !world.sampleCell(mapId,a.x,a.y))
			throw std::invalid_argument("Invalid live city actor value");
		if (map.geometry.isOutdoors() && a.lifecycle==XeenActorLifecycle::Present &&
			(a.x<0 || a.x>=16 || a.y<0 || a.y>=16 || !xeenActorClosure(map,a)[a.y*16+a.x]))
			throw std::invalid_argument("Outdoor actor outside its resource terrain closure");
	}
	for(const auto id:world.sessionState().accountedMonsters())
		if(id.mapId==mapId && (id.recordIndex>=actors.size() ||
			actors[id.recordIndex].lifecycle!=XeenActorLifecycle::Defeated))
			throw std::invalid_argument("Vertigo accounting source changed");
	for(const auto count:XeenActorApproach::occupancy(actors))if(count>3)
		throw std::invalid_argument("Vertigo occupancy exceeded");
}

} // namespace mmodern
