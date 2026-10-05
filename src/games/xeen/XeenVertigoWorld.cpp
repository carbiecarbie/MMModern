#include "games/xeen/XeenWorld.h"
#include "games/xeen/XeenActorApproach.h"
#include "games/xeen/XeenRegionalRules.h"
#include "games/xeen/XeenIndoorScene.h"
#include <bitset>
#include <stdexcept>

namespace mmodern {

XeenActorView XeenWorld::prepareTransitionArrival(const XeenCamera &camera) {
	if (!_detachedEventCandidate) throw std::logic_error("Arrival requires a detached transition");
	auto &actors=camera.mapId==XeenMapIdentity(28) ? _sessionState._vertigoActors.value() : _sessionState._actors;
	const auto view=camera.mapId==XeenMapIdentity(28) ? XeenIndoorScene().classifyActors(*this,camera,actors) :
		XeenActorApproach::classify(actors,camera);
	for (unsigned i=0;i<actors.size();++i) if (view.activation[i]) actors[i].activated=true;
	return view;
}

void XeenWorld::stageVertigoActors(const XeenObjectFile &mob,
		const std::vector<XeenMonsterRecord> &statistics) {
	XeenMutationWatch::write(this);
	if (_sessionState._entry!=XeenEncounterEntry::Ordinary ||
		_sessionState._vertigoActors || mob.mapId!=XeenMapIdentity(28))
		throw std::logic_error("Vertigo actor staging is unavailable");
	auto actors=XeenActorApproach::actorsFromResources(mob,statistics);
	_cityStatistics=statistics;
	_cityOriginalActorCount=static_cast<std::uint16_t>(mob.entities.monsters.size());
	xeenValidateVertigoActors(*this,actors);
	_sessionState._vertigoActors.emplace(std::move(actors));
}

void XeenWorld::applySpawn(std::uint8_t slot, int x, int y, std::uint8_t) {
	XeenMutationWatch::write(this);
	if (_sessionState._entry!=XeenEncounterEntry::Ordinary ||
		!_sessionState._vertigoActors || !xeenIndoorCoordinate(x,y) ||
		slot>=XeenActorApproach::kCapacity || _cityStatistics.empty())
		throw std::invalid_argument("Invalid Spawn operands or owner");
	auto &actors=*_sessionState._vertigoActors;
	while(actors.size()<=slot) {
		XeenActor a;a.id={28,actors.size()};a.original={};
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
	_spawnedPresentationSlots.insert(slot);
}

XeenScenePresentation XeenWorld::prepareSpawnPresentation(const XeenWorld &candidate) const {
	auto prepared=_scenePresentation;
	for(const auto slot:candidate._spawnedPresentationSlots)
		prepared.copySpawn(candidate._sessionState._vertigoActors->at(slot),candidate._scenePresentation);
	return prepared;
}

void xeenValidateVertigoActors(XeenWorld &world,const std::vector<XeenActor> &actors) {
	const auto &mob=world.objectFile(28);
	if(!world.regionalJourney() || !mob.resourcePresent || mob.mapId!=XeenMapIdentity(28) ||
		actors.size()<mob.entities.monsters.size() || actors.size()>XeenActorApproach::kCapacity ||
		world._cityStatistics.empty()) throw std::invalid_argument("Invalid resource-bound city actor collection");
	for(unsigned i=0;i<actors.size();++i) {
		const auto &a=actors[i];
		if(!(a.id==XeenMonsterIdentity{28,i}) || a.status!=XeenActorStatus::Physical)
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
			if(a.statistics || a.x || a.y || a.hp || a.activated ||
				a.original.hasResource() || world.sessionState().accountedMonsters().count(a.id))
				throw std::invalid_argument("Vertigo gap slot is materialized");
			continue;
		}
		if(!a.statistics)throw std::invalid_argument("Vertigo actor statistics are absent");
		if(!a.original.hasResource() || unsigned(a.original.resourceId)>=world._cityStatistics.size() ||
			a.statistics->raw!=world._cityStatistics[a.original.resourceId].raw)
			throw std::invalid_argument("City actor MON resource binding changed");
		const bool accounted=world.sessionState().accountedMonsters().count(a.id)!=0;
		if(a.lifecycle==XeenActorLifecycle::Defeated) {
				if(a.x!=-128 || a.y!=-128 || a.hp || a.activated || !accounted)
					throw std::invalid_argument("Defeated city actor is noncanonical");
		} else if(a.lifecycle==XeenActorLifecycle::Disabled) {
			if(!a.original.isDisabled() || a.x!=a.original.x || a.y!=a.original.y ||
				a.hp!=a.statistics->baseHp() || a.activated || accounted)
				throw std::invalid_argument("Disabled city actor is noncanonical");
		} else if(a.lifecycle!=XeenActorLifecycle::Present || a.hp<1 || a.hp>a.statistics->baseHp() ||
			accounted || !xeenIndoorCoordinate(a.x,a.y) || !world.sampleCell(28,a.x,a.y))
			throw std::invalid_argument("Invalid live city actor value");
	}
	for(const auto id:world.sessionState().accountedMonsters())
		if(id.mapId==XeenMapIdentity(28) && (id.recordIndex>=actors.size() ||
			actors[id.recordIndex].lifecycle!=XeenActorLifecycle::Defeated))
			throw std::invalid_argument("Vertigo accounting source changed");
	for(const auto count:XeenActorApproach::occupancy(actors))if(count>3)
		throw std::invalid_argument("Vertigo occupancy exceeded");
}

} // namespace mmodern
