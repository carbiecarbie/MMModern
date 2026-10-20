#include "games/xeen/XeenWorld.h"
#include "games/xeen/XeenActorApproach.h"
#include "games/xeen/XeenRegionalRules.h"
#include "games/xeen/XeenIndoorScene.h"
#include "games/xeen/XeenTerrainRules.h"
#include <bitset>
#include <algorithm>
#include <stdexcept>

namespace mmodern {

void XeenWorld::retainEventStep(XeenPartyState &party,const XeenRegionalActionCandidate &step,const std::vector<XeenActor> &actors) {
 if(!detachedEventCandidate())throw std::logic_error("Event step requires detached preparation");
 auto retained=actors;_sessionState.mutableMonsters(step.camera.mapId).swap(retained);
 for(unsigned n=0;n<6;++n) {
  party.roster.at(step.owners[n])=step.characters[n];party.roster._combatInputs[step.owners[n]]=step.inputs[n];
 }
 party.encounterContext=step.context;
 if(step.time)party.serviceEconomy=step.time->economy;
 retainEventRandom(step.random.continuation());
}

void XeenWorld::publishTerrainArrival(XeenPartyState &party,const XeenTerrainArrivalCandidate &arrival,const XeenJourneyRandomState &random) {
 if(!detachedEventCandidate())throw std::logic_error("Terrain arrival requires detached publication");
 _scenePresentation.markLightingStep();
 for(unsigned n=0;n<6;++n) {
  const auto owner=arrival.characters[n].rosterId;
  party.roster.at(owner)=arrival.characters[n];party.roster._combatInputs[owner]=arrival.inputs[n];
 }
 party.encounterContext=arrival.context;party.serviceEconomy=arrival.economy;retainEventRandom(random);
}

XeenTerrainArrival XeenWorld::prepareTerrainArrival(const XeenCamera &camera,
 const XeenConsequenceCharacters &characters,const XeenGameplayContext &context) {
 const auto cell=sampleCell(camera.mapId,camera.x,camera.y);
 if(!cell)throw std::invalid_argument("Arrival cell is absent");
 const auto &primary=map(camera.mapId).geometry;
 bool navigator=false;for(const auto &c:characters)navigator=navigator || c.skills[10]!=0;
 auto rule=xeenTerrainArrival(primary.surfaceTypes[cell->cell->surfaceIndex],primary.isOutdoors(),navigator,context.effects[XeenPartyEffect::Levitate]!=0,camera);
 if(rule.falling)rule.transition=prepareEventTransition(rule.transition.destination);
 return rule;
}

XeenTransitionResult XeenWorld::prepareEventTransition(const XeenCamera &requested,bool runLocation) {
    auto destination=requested;
	if (regionalJourney() && destination.mapId!=XeenMapIdentity(23) &&
		destination.mapId!=XeenMapIdentity(28) && destination.mapId!=_developerEventMap)
		return {XeenTransitionResult::Status::Unavailable,destination,"Destination map is not admitted yet"};
	const auto &resource=map(destination.mapId);
	if(runLocation) {destination.x=static_cast<std::int8_t>(static_cast<std::uint8_t>(resource.geometry.runX));destination.y=static_cast<std::int8_t>(static_cast<std::uint8_t>(resource.geometry.runY));}
	const int limit=resource.geometry.isOutdoors()?16:32;
	if (unsigned(destination.direction)>3 || destination.x<0 || destination.y<0 ||
		destination.x>=limit || destination.y>=limit || !sampleCell(destination.mapId,destination.x,destination.y))
		return {XeenTransitionResult::Status::Unavailable,destination,"Destination has no map cell"};
	if (regionalJourney() && destination.mapId!=_developerEventMap && destination.mapId==XeenMapIdentity(23) &&
		!XeenMovement::component(resource,9,11,kXeenAdmittedAreaTraversal)[destination.y*16+destination.x])
		return {XeenTransitionResult::Status::Unavailable,destination,"Destination is outside the admitted mainland"};
	return {XeenTransitionResult::Status::Available,destination,{}};
}

void XeenWorld::loadEventDestination(XeenMapIdentity from,XeenMapIdentity to) {
    if(!_detachedEventCandidate)throw std::logic_error("Event map load requires a detached candidate");
    if(!sessionState().hasRegionalActors(to))stageMapMonsters(objectFile(to),_cityStatistics);
    leaveMap(from);
    reloadMapMonsters(to);
    reloadHeads(to);
    _scenePresentation.markLightingStep();
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
	for(auto at=_scenePresentation.objectFrames.begin();at!=_scenePresentation.objectFrames.end();)
		at=at->first.mapId==mapId?_scenePresentation.objectFrames.erase(at):std::next(at);
	for(const auto &entry:_sessionState._mapStates) {
		auto &cells=_sessionState._mapStates.find(entry.first)->cells;
		for(auto &cell:cells)if(cell.visitMap==mapId) {
			XeenMutationWatch::write(this);cell.visitMap={};cell.visitMask=0;cell.visitWalls.fill(0);
		}
		cells.erase(std::remove_if(cells.begin(),cells.end(),[](const auto &c){
			return !c.mask && !c.unlocked && !c.surfaceChanged && !c.visitMask;
		}),cells.end());
	}
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
	if (outdoors && mob.mapId!=XeenMapIdentity(23) && mob.mapId!=_developerEventMap)
		throw std::logic_error("Outdoor map is not admitted");
	_sessionState.stageMonsters(mob.mapId,std::move(actors),mob.entities.monsters.size(),outdoors);
}

void XeenWorld::applySpawn(XeenMapIdentity mapId, std::uint8_t slot, int x, int y, std::uint8_t) {
	XeenMutationWatch::write(this);
	auto *state=_sessionState._mapStates.find(mapId);
	if (!_detachedEventCandidate || _sessionState._entry!=XeenEncounterEntry::Ordinary ||
		!state || !state->monsters || x<-128 || x>127 || y<-128 || y>127 ||
		slot>=XeenActorApproach::kCapacity || _cityStatistics.empty())
		throw std::invalid_argument("Invalid Spawn operands or owner");
	auto &actors=*state->monsters;
	// DOS bounds every monster loop, the MOB writer and map load by the MOB
	// record count, which Spawn never changes (WORLD/XEEN.DAT 0x27a59..0x27b1c,
	// 0x0fb79..0x0fb84, 0x266e3..0x2671c). A slot at or beyond it is written
	// but never drawn, moved, fought or saved, and the next map load clears it.
	if(slot>=state->originalMonsterCount) return;
	auto &a=actors.at(slot);
	// cmdSpawn reuses the sprite ID; an unbound record uses type zero.
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
	prepared.objectFrames=candidate._scenePresentation.objectFrames;
	// Indicator sources written while the detached world moved and drew.
	prepared.dangerSense=candidate._scenePresentation.dangerSense;
	prepared.thinWall=candidate._scenePresentation.thinWall;
 prepared.lightingStepped=candidate._scenePresentation.lightingStepped;
 prepared.lightingPrepared=candidate._scenePresentation.lightingPrepared;
	for(const auto id:candidate._spawnedPresentationSlots)
		prepared.copySpawn(candidate._sessionState.regionalActors(id.mapId).at(id.recordIndex),candidate._scenePresentation);
	if(!candidate._spawnedPresentationSlots.empty())prepared.copySpawnRandom(candidate._scenePresentation);
	return prepared;
}

void xeenValidateIndoorActors(XeenWorld &world,XeenMapIdentity mapId,const std::vector<XeenActor> &actors) {
	const auto &mob=world.objectFile(mapId);
	const auto &map=world.map(mapId);
	if(!world.regionalJourney() || !mob.resourcePresent || mob.mapId!=mapId ||
		actors.size()!=mob.entities.monsters.size() || actors.size()>XeenActorApproach::kCapacity ||
		world._cityStatistics.empty()) throw std::invalid_argument("Invalid resource-bound city actor collection");
	for(unsigned i=0;i<actors.size();++i) {
		const auto &a=actors[i];
		if(!(a.id==XeenMonsterIdentity{mapId,i}) || !xeenActorStatusSupported(a.status))
			throw std::invalid_argument("Vertigo actor identity/status changed");
		const auto &original=mob.entities.monsters[i];
		if(a.original.x!=original.x || a.original.y!=original.y ||
			a.original.direction!=original.direction || a.original.tableIndex!=original.tableIndex ||
			a.original.resourceId!=original.resourceId)
			throw std::invalid_argument("Original Vertigo actor identity changed");
		if(a.lifecycle==XeenActorLifecycle::Unresolved) {
			if(a.statistics || a.x!=a.original.x || a.y!=a.original.y || a.hp || a.activated ||
				a.original.hasResource() || world.sessionState().accountedMonsters().count(a.id))
				throw std::invalid_argument("Vertigo gap slot is materialized");
			continue;
		}
		if (a.lifecycle==XeenActorLifecycle::Disabled && !a.original.hasResource()) {
			if (!a.original.isDisabled() || a.x!=a.original.x || a.y!=a.original.y ||
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
		// records beyond the available DAT footprint. Scripts park monsters the
		// same way, by Spawn to (100,100) (Cave of Illusion, maps 42-45). Retain
		// exact values; this does not admit a moved actor elsewhere off geometry.
		if (a.lifecycle==XeenActorLifecycle::Present &&
			((a.x==a.original.x && a.y==a.original.y) || (a.x==100 && a.y==100)) &&
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
	// No per-cell cap: DOS Spawn may stack four or more (Winterkill); only a
	// moving monster refuses a destination holding three (0x1fc24).
}

} // namespace mmodern
