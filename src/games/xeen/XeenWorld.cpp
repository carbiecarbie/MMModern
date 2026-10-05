#include "games/xeen/XeenWorld.h"
#include "games/xeen/XeenRestoreGuard.h"
#include "games/xeen/XeenActorApproach.h"
#include "games/xeen/XeenEventPublication.h"
#include "games/xeen/XeenStateEquality.h"
#include "games/xeen/XeenGameFlags.h"

#include <atomic>
#include <stdexcept>
#include <limits>
#include <utility>

namespace mmodern {
XeenWorld::GameplayBorrow::GameplayBorrow(XeenWorld &w, XeenPartyState &p,
		XeenCamera &c, const XeenGameFlags &f) : owners{w._gameplayBorrow.retain(),
		p._gameplayBorrow.retain(), p.roster._gameplayBorrow.retain(),
		c.gameplayBorrow.retain(), f._gameplayBorrow.retain()} {
	for (const auto &state : owners) { ++state->references; ++state->revision; }
}
namespace {
template<class Visit> void resourceEntityRanges(const XeenMapEntities &entities,const Visit &visit) {
	visit(entities.objects.data(),entities.objects.size()*sizeof(XeenMapEntity));
	visit(entities.monsters.data(),entities.monsters.size()*sizeof(XeenMapEntity));
	visit(entities.wallItems.data(),entities.wallItems.size()*sizeof(XeenMapEntity));
}
template<class Visit> void resourceRanges(const XeenMap &map,const Visit &visit) {
	visit(&map,sizeof(map));resourceEntityRanges(map.entities,visit);
	visit(map.instructions.data(),map.instructions.size()*sizeof(XeenEventInstruction));
	for(const auto &instruction:map.instructions)
		visit(instruction.parameters.data(),instruction.parameters.size()*sizeof(XeenMutable<std::uint8_t>));
}
template<class Visit> void resourceRanges(const XeenObjectFile &objects,const Visit &visit) {
	visit(&objects,sizeof(objects));resourceEntityRanges(objects.entities,visit);
}
std::uint64_t nextWorldIncarnation() {
	static std::atomic<std::uint64_t> next{1};
	auto value = next.load(std::memory_order_relaxed);
	for (;;) {
		if (value == std::numeric_limits<std::uint64_t>::max())
			throw std::overflow_error("world incarnation exhausted");
		if (next.compare_exchange_weak(value, value + 1,
				std::memory_order_relaxed, std::memory_order_relaxed)) return value;
	}
}
using namespace xeen_state;
}

XeenWorld::XeenWorld(MapLoader loader, ObjectLoader objectLoader) :
	_incarnation(nextWorldIncarnation()), _baseLoader(loader), _baseObjectLoader(objectLoader),
	_loader(std::move(loader)), _objectLoader(std::move(objectLoader)) {
	if (!_loader)
		throw std::invalid_argument("XeenWorld requires a map loader");
}

void XeenWorld::restoreSessionState(const std::vector<XeenObjectIdentity> &objects,
		const std::vector<XeenEventIdentity> &events, const EventLoader &eventLoader) {
	if (hasEncounterState()) throw std::logic_error("cannot restore an encounter world");
	XeenSessionWorldState prepared;
	for (const auto id : objects) {
		static_cast<void>(map(id.mapId));
		if (hasEncounterState()) throw std::logic_error("encounter appeared during overlay map loading");
		validateObject(id);
		if (hasEncounterState()) throw std::logic_error("encounter appeared during overlay resource loading");
		if (!prepared._objects.insert(id).second)
			throw std::invalid_argument("duplicate restored object identity");
	}
	std::map<XeenMapIdentity, XeenEventFile> files;
	for (const auto id : events) {
		static_cast<void>(map(id.mapId));
		if (hasEncounterState()) throw std::logic_error("encounter appeared during overlay map loading");
		if (!eventLoader) throw std::invalid_argument("restoration requires an event loader");
		auto found = files.find(id.mapId);
		if (found == files.end()) found = files.emplace(id.mapId, eventLoader(id.mapId)).first;
		if (hasEncounterState()) throw std::logic_error("encounter appeared during overlay event loading");
		const auto &file = found->second;
		if (file.mapId != id.mapId || !file.resourcePresent || id.recordIndex >= file.records.size())
			throw std::invalid_argument("restored original event identity does not exist");
		if (!prepared._events.insert(id).second)
			throw std::invalid_argument("duplicate restored event identity");
	}
	if (hasEncounterState()) throw std::logic_error("encounter appeared before overlay publication");
	XeenMutationWatch::write(this);
	_sessionState._objects.swap(prepared._objects);
	_sessionState._events.swap(prepared._events);
	++_ownerRevision;
}

void XeenWorld::swapPreparedState(XeenWorld &candidate) noexcept {
	XeenMutationWatch::write(this);XeenMutationWatch::write(&candidate);
	// Private to guarded save publication. Never swaps/clears encounter authority.
	_sessionState._objects.swap(candidate._sessionState._objects);
	_sessionState._events.swap(candidate._sessionState._events);
	_sessionState._barriers.swap(candidate._sessionState._barriers);
	_maps.swap(candidate._maps);
	_objects.swap(candidate._objects);
	++_ownerRevision; ++candidate._ownerRevision;
}

const XeenObjectFile &XeenWorld::objectFile(XeenMapIdentity mapId) {
	if (!mapId) throw std::invalid_argument("invalid object map identity");
	const auto found = _objects.find(mapId);
	if (found != _objects.end()) return found->second;
	// Geometry-only clients may omit the provider; this represents no MOB source.
	XeenObjectFile loaded = _objectLoader ? _objectLoader(mapId) :
		XeenObjectFile{mapId, {}, false, {}};
	if (_combatCheck) _combatCheck();
	XeenRestoreGuard::admitLoadedResource(*this,mapId,loaded,loaded.mapId==mapId,
		[](XeenRestoreGuard &g)->auto & {return g.objects;},xeen_state::sameObjectFile);
	XeenMutationWatch::prepareOwned(this,4);
	const auto result = _objects.emplace(mapId, std::move(loaded));
	++_cacheRevision;
	XeenRestoreGuard::loadedResourceInserted(*this);
	resourceRanges(result.first->second,[this](const void *p,std::size_t n) { XeenMutationWatch::addOwned(this,p,n); });
	return result.first->second;
}

void XeenWorld::validateObject(XeenObjectIdentity id) {
	const auto &file = objectFile(id.mapId);
	if (!file.resourcePresent || id.recordIndex >= file.entities.objects.size())
		throw std::invalid_argument("selected original object index does not exist");
}

bool XeenWorld::isObjectDisabled(XeenObjectIdentity id) {
	validateObject(id);
	return objectFile(id.mapId).entities.objects[id.recordIndex].isDisabled() ||
		_sessionState.isObjectDisabled(id);
}

std::optional<XeenObjectIdentity> XeenWorld::selectObject(const XeenCamera &camera) {
	const bool city=camera.mapId==XeenMapIdentity(28) && regionalJourney();
	if (!camera.mapId || camera.x < 0 || camera.x >= (city?32:16) ||
		camera.y < 0 || camera.y >= (city?32:16))
		throw std::invalid_argument("invalid physical object-selection cell");
	const auto &file = objectFile(camera.mapId);
	if (!file.resourcePresent) return std::nullopt;
	for (std::size_t i = 0; i < file.entities.objects.size(); ++i) {
		const auto &object = file.entities.objects[i];
		XeenObjectIdentity id{camera.mapId, i};
		if (object.x == camera.x && object.y == camera.y && object.isActive() &&
				!_sessionState.isObjectDisabled(id)) return id;
	}
	return std::nullopt;
}

XeenEventRecord XeenWorld::effectiveEvent(XeenEventIdentity id, const XeenEventRecord &base) const {
	XeenEventRecord result = base;
	if (isEventDisabled(id)) result.opcode = 0;
	return result;
}

void XeenWorld::disableObject(XeenObjectIdentity id) {
	validateObject(id);
	XeenMutationWatch::write(this);
	_sessionState._objects.insert(id);
}

void XeenWorld::validateEventCell(const XeenCamera &physical, const XeenEventFile &events) {
	if (!physical.mapId || physical.x < 0 || physical.x > 15 || physical.y < 0 || physical.y > 15 ||
			events.mapId != physical.mapId)
		throw std::invalid_argument("event mutation requires the physical map/cell");
	static_cast<void>(map(physical.mapId));
}

void XeenWorld::disableEventsAtCell(const XeenCamera &physical, const XeenEventFile &events) {
	validateEventCell(physical, events);
	XeenMutationWatch::write(this);
	for (std::size_t i = 0; i < events.records.size(); ++i) {
		const auto &record = events.records[i];
		if (record.x == physical.x && record.y == physical.y)
			_sessionState._events.insert({physical.mapId, i});
	}
}

void XeenWorld::applyRemove(const XeenCamera &physical,
		std::optional<XeenObjectIdentity> selected, const XeenEventFile &events, const XeenEventPublication *publication) {
	if (_sessionState.journey() && !publication) throw std::logic_error("Journey Remove requires live event authority");
	if (publication) publication->check();
	// Validate and allocate both overlays before publishing either one. Remove is
	// atomic on its own; an earlier script grant remains independently published.
	validateEventCell(physical, events);
	if (selected) {
		if (selected->mapId != physical.mapId)
			throw std::invalid_argument("selected object belongs to another physical map/side");
		validateObject(*selected);
	}
	auto disabledObjects = _sessionState._objects;
	auto disabledEvents = _sessionState._events;
	if (selected) disabledObjects.insert(*selected);
	for (std::size_t i = 0; i < events.records.size(); ++i) {
		const auto &record = events.records[i];
		if (record.x == physical.x && record.y == physical.y)
			disabledEvents.insert({physical.mapId, i});
	}
	if (publication) publication->prepareRemove(physical, selected, events);
	static_assert(noexcept(_sessionState._objects.swap(disabledObjects)));
	static_assert(noexcept(_sessionState._events.swap(disabledEvents)));
	XeenMutationWatch::write(this);
	_sessionState._objects.swap(disabledObjects);
	_sessionState._events.swap(disabledEvents);
	if (publication) publication->removed();
}

const XeenMap &XeenWorld::map(XeenMapIdentity mapId) {
	if (!mapId)
		throw std::invalid_argument("ID zero does not represent a Xeen map");
	const auto cached = _maps.find(mapId);
	if (cached != _maps.end())
		return cached->second;

	XeenMap loaded = _loader(mapId);
	if (_combatCheck) _combatCheck();
	XeenRestoreGuard::admitLoadedResource(*this,mapId,loaded,loaded.identity()==mapId,
		[](XeenRestoreGuard &g)->auto & {return g.maps;},xeen_state::sameMap);
	XeenMutationWatch::prepareOwned(this,5+loaded.instructions.size());
	const auto result = _maps.emplace(mapId, std::move(loaded));
	++_cacheRevision;
	XeenRestoreGuard::loadedResourceInserted(*this);
	resourceRanges(result.first->second,[this](const void *p,std::size_t n) { XeenMutationWatch::addOwned(this,p,n); });
	return result.first->second;
}

void XeenWorld::discardMapCache() {
	// The immutable value preimages remain in guards. Retire only the addresses
	// being freed, preserving any mutation already observed before eviction.
	const auto retire=[](const void *p,std::size_t n) { XeenMutationWatch::retire(p,n); };
	for(const auto &entry:_maps) resourceRanges(entry.second,retire);
	for(const auto &entry:_objects) resourceRanges(entry.second,retire);
	_maps.clear();_objects.clear();_sceneActors.clear();++_cacheRevision;
}

std::optional<XeenCellSample> XeenWorld::sampleCell(
		XeenMapIdentity mapId, int x, int y) {
	// Gameplay, scene projection and rays share Map::getCell's Y-before-X
	// neighbor resolution. The sample retains its physical tile identity; the
	// caller's logical camera/actor map is never changed by this lookup.
	if(x < -16 || x >=32 || y < -16 || y>=32) return std::nullopt;
	const auto *current=&map(mapId);
	if(y<0 || y>=16) {
		const auto next=current->geometry.neighbors[y<0?2:0];
		if(!next) return std::nullopt;
		y+=y<0?16:-16;current=&map({mapId.side,next});
	}
	if(x<0 || x>=16) {
		const auto next=current->geometry.neighbors[x<0?3:1];
		if(!next) return std::nullopt;
		x+=x<0?16:-16;current=&map({mapId.side,next});
	}
	XeenCellSample result{current->identity(),x,y,&current->geometry,&current->geometry.cells[y*16+x],{}};
	for(const auto &v:_sessionState._barriers) if(v.tile==result.mapId && v.cell==y*16+x) {
		auto cell=std::make_shared<XeenMapCell>(*result.cell);
		if(cell->rawWord!=v.originalWord || cell->rawAttributes!=v.originalAttributes)
			throw std::logic_error("Barrier original resource preimage changed");
		auto &walls=xeenGet<XeenIndoorWalls>(cell->geometry).walls;
		for(unsigned d=0;d<4;++d)if(v.mask&(1u<<d)) {
			walls[d]=v.walls[d];const auto shift=12-4*d;
			cell->rawWord=(cell->rawWord&~(15u<<shift))|(unsigned(v.walls[d])<<shift);
		}
		if(v.unlocked) {cell->rawAttributes|=0x80;cell->flags|=0x80;}
		result.effectiveCell=std::move(cell);result.cell=result.effectiveCell.get();break;
	}
	return result;
}

void XeenWorld::setBarrier(const XeenCamera &camera,std::uint8_t wall,bool unlock) {
	if(!detachedEventCandidate() || unsigned(camera.direction)>3 || wall>15)
		throw std::logic_error("Barrier changes require detached capability");
	constexpr int dx[]{0,1,0,-1},dy[]{1,0,-1,0};const unsigned direction=unsigned(camera.direction);
	const auto a=sampleCell(camera.mapId,camera.x,camera.y);
	const auto b=sampleCell(camera.mapId,camera.x+dx[direction],camera.y+dy[direction]);
	if(!a || !b || a->geometry->isOutdoors() || b->geometry->isOutdoors())
		throw std::invalid_argument("Barrier has no two indoor faces");
	auto values=_sessionState._barriers;
	const auto set=[&](const XeenCellSample &s,unsigned d) {
		auto i=std::find_if(values.begin(),values.end(),[&](const auto &v){return v.tile==s.mapId && v.cell==s.y*16+s.x;});
		if(i==values.end()) {
			const auto &base=map(s.mapId).geometry.cells[s.y*16+s.x];
			XeenBarrierOverride v;v.tile=s.mapId;v.cell=s.y*16+s.x;
			v.originalWord=base.rawWord;v.originalAttributes=base.rawAttributes;
			values.push_back(v);i=std::prev(values.end());
		}
		i->mask|=1u<<d;i->walls[d]=wall;i->unlocked=i->unlocked || unlock;
	};
	set(*a,direction);set(*b,direction^2);
	std::sort(values.begin(),values.end(),[](const auto &a,const auto &b){return a.tile<b.tile || (a.tile==b.tile && a.cell<b.cell);});
	XeenMutationWatch::write(this);_sessionState._barriers.swap(values);
}

void XeenWorld::restoreBarriers(const std::vector<XeenBarrierOverride> &values) {
	if(_sessionState._journeyOwner || _sessionState.journey())throw std::logic_error("Barrier restore requires unpublished World");
	std::set<std::pair<XeenMapIdentity,unsigned>> admitted;
	// Bind the playable logical town's complete physical tile identities.
	for(int y=0;y<32;++y)for(int x=0;x<32;++x) {
		const auto s=sampleCell(28,x,y);if(!s)throw std::invalid_argument("Missing barrier geometry tile");
		admitted.emplace(s->mapId,s->y*16+s->x);
	}
	for(unsigned n=0;n<values.size();++n) {
		const auto &v=values[n];
		if(!admitted.count({v.tile,v.cell}) || !v.mask || v.mask>15 ||
			(n && !(values[n-1].tile<v.tile || (values[n-1].tile==v.tile && values[n-1].cell<v.cell))))
			throw std::invalid_argument("Invalid barrier tile/cell/order");
		const auto &base=map(v.tile).geometry.cells[v.cell];
		if(map(v.tile).geometry.isOutdoors() || base.rawWord!=v.originalWord || base.rawAttributes!=v.originalAttributes)
			throw std::invalid_argument("Saved barrier original resource mismatch");
		constexpr int dx[]{0,1,0,-1},dy[]{1,0,-1,0};
		for(unsigned d=0;d<4;++d) {
			if(!(v.mask&(1u<<d))) {if(v.walls[d])throw std::invalid_argument("Noncanonical unused barrier face");continue;}
			if(v.walls[d]!=1 && v.walls[d]!=3 && v.walls[d]!=6 && v.walls[d]!=9 && v.walls[d]!=13)
				throw std::invalid_argument("Unsupported saved barrier wall");
			const auto other=sampleCell(v.tile,v.cell%16+dx[d],v.cell/16+dy[d]);
			const auto match=other?std::find_if(values.begin(),values.end(),[&](const auto &b){return b.tile==other->mapId && b.cell==other->y*16+other->x;}):values.end();
			if(match==values.end() || !(match->mask&(1u<<(d^2))) || match->walls[d^2]!=v.walls[d])
				throw std::invalid_argument("Saved barrier lacks matching opposite face");
		}
	}
	auto prepared=values;XeenMutationWatch::write(this);_sessionState._barriers.swap(prepared);
}

std::unique_ptr<XeenWorld> XeenWorld::transitionCandidate() const {
	if (!regionalJourney())
		throw std::logic_error("Vertigo candidate requires admitted city content");
	auto candidate=std::make_unique<XeenWorld>(_baseLoader,_baseObjectLoader);
	candidate->_sessionState=_sessionState;
	// The interpreter operates only on detached values; live Journey authority
	// remains with the source world and its Flow lease.
	candidate->_sessionState._entry=XeenEncounterEntry::Ordinary;
	candidate->_detachedEventCandidate=true;
	candidate->_maps=_maps;candidate->_objects=_objects;
	candidate->_cityStatistics=_cityStatistics;
	candidate->_cityOriginalActorCount=_cityOriginalActorCount;
	candidate->_scenePresentation=_scenePresentation;
	return candidate;
}

void XeenWorld::copyEventParty(const XeenPartyState &source, XeenPartyState &candidate) const {
	if(!_detachedEventCandidate || &source==&candidate || candidate._gameplayBorrow.borrowed() ||
		candidate.roster._gameplayBorrow.borrowed() || candidate.roster._combatMarked || candidate.party.size())
		throw std::logic_error("Event party preparation requires fresh detached storage");
	for(unsigned owner=0;owner<XeenRoster::kCharacterCount;++owner) {
		if(candidate.roster._combatInputs[owner])throw std::logic_error("Event party supplements already exist");
		candidate.roster._characters[owner]=source.roster._characters[owner];
		candidate.roster._combatInputs[owner]=source.roster._combatInputs[owner];
	}
	candidate.roster._combatMarked=source.roster._combatMarked;
	candidate.party=source.party;
	candidate.encounterContext=source.encounterContext;candidate.monsterTreasure=source.monsterTreasure;
	candidate.serviceEconomy=source.serviceEconomy;candidate.questItems=source.questItems;
	candidate.questFlags=source.questFlags;candidate.regionalRecovery=source.regionalRecovery;
	candidate._originalFood=source._originalFood;
	candidate.firstSerializedCount=source.firstSerializedCount;candidate.effectiveSerializedCount=source.effectiveSerializedCount;
	candidate.diagnostics=source.diagnostics;
}

void XeenWorld::applyAlterEvent(const XeenCamera &physical, std::uint8_t line,
		std::uint8_t replacement, const XeenEventFile &events) {
	XeenMutationWatch::write(this);
	if (!regionalJourney() || _sessionState._entry!=XeenEncounterEntry::Ordinary ||
		physical.mapId!=events.mapId || replacement!=0)
		throw std::invalid_argument("AlterEvent replacement is unsupported");
	bool found=false;
	for (std::size_t i=0;i<events.records.size();++i) {
		const auto &r=events.records[i];
		if (r.x==physical.x && r.y==physical.y && r.line==line &&
			(r.direction==static_cast<std::uint8_t>(physical.direction) || r.direction==4)) {
			_sessionState._events.insert({physical.mapId,i});found=true;
		}
	}
	if (!found) throw std::invalid_argument("AlterEvent physical line was not found");
}

void XeenWorld::publishTransition(XeenWorld &candidate) noexcept {
	XeenMutationWatch::write(this);XeenMutationWatch::write(&candidate);
	_sessionState._actors.swap(candidate._sessionState._actors);
	_sessionState._vertigoActors.swap(candidate._sessionState._vertigoActors);
	_sessionState._accountedMonsters.swap(candidate._sessionState._accountedMonsters);
	_sessionState._events.swap(candidate._sessionState._events);
	_sessionState._barriers.swap(candidate._sessionState._barriers);
	_sessionState._objects.swap(candidate._sessionState._objects);
	_cityStatistics.swap(candidate._cityStatistics);
	std::swap(_cityOriginalActorCount,candidate._cityOriginalActorCount);
	_maps.swap(candidate._maps);_objects.swap(candidate._objects);
	++_ownerRevision;
}

} // namespace mmodern
