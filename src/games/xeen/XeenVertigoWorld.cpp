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
	if (_sessionState._journeyContract!=8 || _sessionState._entry!=XeenEncounterEntry::Ordinary ||
		_sessionState._vertigoActors || mob.mapId!=XeenMapIdentity(28))
		throw std::logic_error("Vertigo actor staging is unavailable");
	auto actors=XeenActorApproach::actorsFromResources(mob,statistics);
	if (actors.size()!=46 || statistics.empty() || actors[35].original.resourceId!=0 ||
		actors[35].original.x!=15 || actors[35].original.y!=4 ||
		statistics[0].image()!=0 || statistics[0].baseHp()!=2)
		throw std::invalid_argument("Original Vertigo actor catalog changed");
	statistics[0].validateSlime();
	_vertigoSpawnSlime=statistics[0];
	xeenValidateVertigoActors(*this,actors);
	_sessionState._vertigoActors.emplace(std::move(actors));
}

void XeenWorld::applySpawn(std::uint8_t slot, int x, int y, std::uint8_t) {
	XeenMutationWatch::write(this);
	if (_sessionState._journeyContract!=8 || _sessionState._entry!=XeenEncounterEntry::Ordinary ||
		!_sessionState._vertigoActors || x<0 || x>=32 || y<0 || y>=32 ||
		!(slot<=40 || slot==50 || slot==51))
		throw std::invalid_argument("Spawn is outside the admitted city reset");
	auto &actors=*_sessionState._vertigoActors;
	if (slot>=50 && actors.size()==46) {
		if (!_vertigoSpawnSlime) throw std::logic_error("Script-created Slime descriptor is absent");
		actors.reserve(52);
		for (unsigned i=46;i<52;++i) {
			XeenActor a;a.id={28,i};a.original={};a.original.x=a.original.y=0;
			a.x=a.y=0;
			if (i>=50) {a.original.resourceId=0;a.statistics=*_vertigoSpawnSlime;}
			actors.push_back(std::move(a));
		}
	}
	if (slot>=actors.size()) throw std::invalid_argument("Spawn slot is unavailable");
	auto &a=actors[slot];
	if (!a.statistics) throw std::invalid_argument("Spawn has no original monster type");
	a.x=x;a.y=y;a.hp=a.statistics->baseHp();a.activated=false;
	a.lifecycle=XeenActorLifecycle::Present;a.status=XeenActorStatus::Physical;
	_sessionState._accountedMonsters.erase(a.id);
}

void xeenValidateVertigoActors(XeenWorld &world,const std::vector<XeenActor> &actors) {
	if(!world.regionalContract8() || (actors.size()!=46 && actors.size()!=52))
		throw std::invalid_argument("Vertigo actor collection has an invalid shape: "+std::to_string(actors.size())+" contract "+std::to_string(world.sessionState().journeyContract()));
	const bool reset=actors.size()==52;
	if(reset && !world.isEventDisabled({28,764}))
		throw std::invalid_argument("Reset actors require the original protection overlay");
	const auto &mob=world.objectFile(28);
	if(!mob.resourcePresent || mob.entities.monsters.size()!=46 || mob.entities.objects.size()!=143)
		throw std::invalid_argument("Original Vertigo MOB catalog changed");
	static constexpr std::array<std::array<int,2>,43> resetAt{{
		{{1,11}},{{1,11}},{{2,9}},{{3,10}},{{3,11}},{{3,11}},{{3,13}},{{3,13}},
		{{3,27}},{{4,27}},{{4,26}},{{4,25}},{{4,12}},{{4,7}},{{4,7}},{{4,3}},
		{{4,3}},{{4,3}},{{5,12}},{{9,18}},{{25,14}},{{28,9}},{{30,9}},{{30,6}},
		{{29,15}},{{8,24}},{{8,24}},{{7,23}},{{7,23}},{{8,27}},{{8,27}},{{9,18}},
		{{6,2}},{{7,1}},{{6,6}},{{7,7}},{{15,4}},{{22,9}},{{21,1}},{{22,1}},
		{{30,1}},{{7,24}},{{6,27}}
	}};
	const unsigned selected=reset?36:35;
	for(unsigned i=0;i<actors.size();++i) {
		const auto &a=actors[i];
		if(!(a.id==XeenMonsterIdentity{28,i}) || a.status!=XeenActorStatus::Physical)
			throw std::invalid_argument("Vertigo actor identity/status changed");
		if(i>=46 && i<=49) {
			if(a.statistics || a.x || a.y || a.hp || a.activated ||
				a.lifecycle!=XeenActorLifecycle::Unresolved || world.sessionState().accountedMonsters().count(a.id))
				throw std::invalid_argument("Vertigo gap slot is materialized");
			continue;
		}
		if(!a.statistics)throw std::invalid_argument("Vertigo actor statistics are absent");
		if(i<46) {
			const auto &original=mob.entities.monsters[i];
			if(a.original.x!=original.x || a.original.y!=original.y ||
				a.original.direction!=original.direction || a.original.tableIndex!=original.tableIndex ||
				a.original.resourceId!=original.resourceId)
				throw std::invalid_argument("Original Vertigo actor identity changed");
		} else if(i>=50) a.statistics->validateSlime();
		const int x=reset && (i<=40 || i>=50) ? resetAt[i>=50 ? i-9 : i][0] : int(a.original.x);
		const int y=reset && (i<=40 || i>=50) ? resetAt[i>=50 ? i-9 : i][1] : int(a.original.y);
		const bool accounted=world.sessionState().accountedMonsters().count(a.id)!=0;
		if(i==selected) {
			a.statistics->validateSlime();
			if(a.lifecycle==XeenActorLifecycle::Defeated) {
				if(a.x!=-128 || a.y!=-128 || a.hp || a.activated || !accounted)
					throw std::invalid_argument("Defeated Vertigo Slime is noncanonical");
			} else if(a.lifecycle!=XeenActorLifecycle::Present || a.hp<1 || a.hp>a.statistics->baseHp() ||
				accounted || a.x<0 || a.x>=32 || a.y<0 || a.y>=32 ||
				(!a.activated && (a.x!=x || a.y!=y)))
				throw std::invalid_argument("Live Vertigo Slime is noncanonical");
		} else if(a.x!=x || a.y!=y || a.hp!=a.statistics->baseHp() || a.activated || accounted ||
			a.lifecycle!=XeenActorLifecycle::Present)
			throw std::invalid_argument("Dormant Vertigo actor changed");
	}
	for(const auto id:world.sessionState().accountedMonsters())
		if(id.mapId==XeenMapIdentity(28) && id.recordIndex!=selected)
			throw std::invalid_argument("Vertigo accounting source changed");
	for(const auto count:XeenActorApproach::occupancy(actors))if(count>3)
		throw std::invalid_argument("Vertigo occupancy exceeded");
	// A saved activated Slime must be reachable from the checked original/reset
	// spawn under every admitted player cell and facing. Keep every other original
	// slot in the simulation; a newly influencing actor invalidates admission.
	auto &closure=world._vertigoClosure[reset?1:0];
	if(!closure) {
	std::vector<XeenCamera> cameras;
	for(int y=0;y<=4;++y)for(int x=13;x<=16;++x)if(xeenJourneyContent(8).vertigoCell(x,y))
		for(unsigned facing=0;facing<4;++facing)
			cameras.push_back({28,x,y,static_cast<XeenDirection>(facing)});
	std::bitset<2048> reachable;
	std::vector<unsigned> queue;
	auto enqueue=[&](int px,int py,bool activated) {
		if(px<0 || px>=32 || py<0 || py>=32)
			throw std::invalid_argument("Vertigo Slime left the original map");
		const unsigned index=unsigned(py*32+px)+(activated?1024:0);
		if(!reachable.test(index)){reachable.set(index);queue.push_back(index);}
	};
	enqueue(reset?resetAt[36][0]:int(actors[35].original.x),
		reset?resetAt[36][1]:int(actors[35].original.y),false);
	auto simulated=actors;
	auto &slime=simulated[selected];
	slime.lifecycle=XeenActorLifecycle::Present;
	slime.hp=slime.statistics->baseHp();
	for(std::size_t q=0;q<queue.size();++q) {
		const unsigned state=queue[q];
		slime.x=int(state%1024%32);slime.y=int(state%1024/32);
		slime.activated=state>=1024;
		for(const auto &camera:cameras) {
			const auto view=XeenIndoorScene().classifyActors(world,camera,simulated);
			for(unsigned i=0;i<simulated.size();++i)if(i!=selected && view.activation[i])
				throw std::invalid_argument("Unsupported Vertigo actor influences the route");
			if(view.activation[selected])enqueue(slime.x,slime.y,true);
			if(slime.activated || view.activation[selected]) {
				auto active=simulated;
				active[selected].activated=true;
				const auto moved=XeenActorApproach::move(active,camera,[&](const XeenActor &a,int mx,int my) {
					return xeenIndoorActorTerrain(world,a,mx,my);
				});
				enqueue(moved[selected].x,moved[selected].y,true);
			}
		}
	}
	closure=reachable;
	}
	if(actors[selected].lifecycle==XeenActorLifecycle::Present &&
		!closure->test(unsigned(actors[selected].y*32+actors[selected].x)+(actors[selected].activated?1024:0)))
		throw std::invalid_argument("Vertigo Slime position is outside its movement closure");
}

} // namespace mmodern
