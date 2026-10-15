#include "games/xeen/XeenWorld.h"
#include "games/xeen/XeenRestoreGuard.h"
#include "games/xeen/XeenActorApproach.h"
#include "games/xeen/XeenEventPublication.h"
#include "games/xeen/XeenStateEquality.h"
#include "games/xeen/XeenGameFlags.h"
#include "games/xeen/XeenSkills.h"
#include "games/xeen/XeenCombatRules.h"

#include <atomic>
#include <stdexcept>
#include <limits>
#include <utility>
#include <algorithm>
#include <exception>
#include <map>

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

std::size_t XeenSessionWorldState::disabledObjectCount() const noexcept {
	std::size_t count=0;for (const auto &entry:_mapStates) count+=entry.second.removedObjects.size();return count;
}
std::size_t XeenSessionWorldState::disabledEventCount() const noexcept {
	std::size_t count=0;for (const auto &entry:_mapStates) count+=entry.second.disabledEvents.size();return count;
}
std::set<XeenObjectIdentity> XeenSessionWorldState::disabledObjects() const {
	std::set<XeenObjectIdentity> values;
	for (const auto &entry:_mapStates) values.insert(entry.second.removedObjects.begin(),entry.second.removedObjects.end());
	return values;
}
std::set<XeenEventIdentity> XeenSessionWorldState::disabledEvents() const {
	std::set<XeenEventIdentity> values;
	for (const auto &entry:_mapStates) values.insert(entry.second.disabledEvents.begin(),entry.second.disabledEvents.end());
	return values;
}
std::vector<XeenBarrierOverride> XeenSessionWorldState::barriers() const {
	std::vector<XeenBarrierOverride> values;
	for (const auto &entry:_mapStates) values.insert(values.end(),entry.second.cells.begin(),entry.second.cells.end());
	return values;
}
std::vector<XeenObjectOverride> XeenSessionWorldState::objectOverrides() const {
	std::vector<XeenObjectOverride> values;
	for (const auto &entry:_mapStates)
		values.insert(values.end(),entry.second.objectOverrides.begin(),entry.second.objectOverrides.end());
	return values;
}
std::vector<XeenExploredTile> XeenSessionWorldState::exploredTiles() const {
	std::vector<XeenExploredTile> values;
	for (const auto &entry:_mapStates) if (entry.second.explored) values.push_back({entry.first,*entry.second.explored});
	return values;
}
void XeenSessionWorldState::stageMonsters(XeenMapIdentity mapId, std::vector<XeenActor> actors,
		std::size_t originalCount, bool outdoors) {
	if (originalCount>actors.size() || originalCount>std::numeric_limits<std::uint16_t>::max())
		throw std::invalid_argument("Staged monster table loses original records");
	auto &state=_mapStates.state(mapId);
	state.monsters.emplace(std::move(actors));
	state.originalMonsterCount=static_cast<std::uint16_t>(originalCount);state.outdoors=outdoors;
}
void XeenSessionWorldState::reserveTiles(const std::vector<XeenExploredTile> &values) {
	for (const auto &tile:values) static_cast<void>(_mapStates.state(tile.mapId));
}
void XeenSessionWorldState::publishExploration(const std::vector<XeenExploredTile> &values) noexcept {
	// Every value names a reserved entry; entries absent from the complete
	// values have no explored bits.
	for (const auto &tile:values) if (!_mapStates.find(tile.mapId)) std::terminate();
	auto next=values.begin();
	for (const auto &entry:_mapStates) {
		auto &state=*_mapStates.find(entry.first);
		if (next!=values.end() && next->mapId==entry.first) state.explored=next++->cells;
		else state.explored.reset();
	}
}
void XeenSessionWorldState::assignCells(const std::vector<XeenBarrierOverride> &values) {
	std::map<XeenMapIdentity,std::vector<XeenBarrierOverride>> tiles;
	for (const auto &cell:values) {
		tiles[cell.tile].push_back(cell);
		static_cast<void>(_mapStates.state(cell.tile));
	}
	for (const auto &entry:_mapStates) {
		auto &state=*_mapStates.find(entry.first);
		const auto found=tiles.find(entry.first);
		if (found!=tiles.end()) state.cells.swap(found->second);else state.cells.clear();
	}
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
		if (!prepared._mapStates.state(id.mapId).removedObjects.insert(id).second)
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
		if (!prepared._mapStates.state(id.mapId).disabledEvents.insert(id).second)
			throw std::invalid_argument("duplicate restored event identity");
	}
	// Replace both sets on every entry, keeping the entries' other values.
	for (const auto &entry:_sessionState._mapStates) static_cast<void>(prepared._mapStates.state(entry.first));
	for (const auto &entry:prepared._mapStates) static_cast<void>(_sessionState._mapStates.state(entry.first));
	if (hasEncounterState()) throw std::logic_error("encounter appeared before overlay publication");
	XeenMutationWatch::write(this);
	for (const auto &entry:prepared._mapStates) {
		auto &live=*_sessionState._mapStates.find(entry.first);auto &value=*prepared._mapStates.find(entry.first);
		live.removedObjects.swap(value.removedObjects);live.disabledEvents.swap(value.disabledEvents);
	}
	++_ownerRevision;
}

void XeenWorld::swapPreparedState(XeenWorld &candidate) noexcept {
	XeenMutationWatch::write(this);XeenMutationWatch::write(&candidate);
	// Private to guarded save publication. Never swaps/clears encounter authority.
	// The whole per-map table, monster tables included, moves as one value.
	_sessionState._mapStates.swap(candidate._sessionState._mapStates);
	_sessionState._headCache.swap(candidate._sessionState._headCache);
	_scenePresentation.objectFlips.swap(candidate._scenePresentation.objectFlips);
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
	return effectiveObject(id).isDisabled() ||
		_sessionState.isObjectDisabled(id);
}

XeenMapEntity XeenWorld::effectiveObject(XeenObjectIdentity id) {
	validateObject(id);
	auto value=objectFile(id.mapId).entities.objects[id.recordIndex];
	if(const auto *state=_sessionState.mapState(id.mapId))
		for(const auto &v:state->objectOverrides)if(v.id==id) {value.x=v.x;value.y=v.y;break;}
	return value;
}

bool XeenWorld::effectiveObjectFlip(XeenObjectIdentity id) const {
	const auto drawn=_scenePresentation.objectFlips.find(id);
	if(drawn!=_scenePresentation.objectFlips.end())return drawn->second;
	if(const auto *state=_sessionState.mapState(id.mapId))
		for(const auto &v:state->objectOverrides)if(v.id==id)return v.flipped;
	return false; // MazeObject constructor, before the first drawScene.
}

void XeenWorld::applyMoveObject(XeenObjectIdentity id,int x,int y) {
	if(!detachedEventCandidate() || x<-128 || x>127 || y<-128 || y>127)
		throw std::invalid_argument("MoveObj requires a detached Event and signed-byte coordinates");
	const auto before=effectiveObject(id);
	bool flip=effectiveObjectFlip(id);
	if(before.x==x && before.y==y)flip=!flip;
	auto &state=_sessionState._mapStates.state(id.mapId);
	auto prepared=state.objectOverrides;
	const auto at=std::lower_bound(prepared.begin(),prepared.end(),id,[](const auto &v,auto key){return v.id<key;});
	if(at!=prepared.end() && at->id==id)*at={id,x,y,flip};
	else prepared.insert(at,{id,x,y,flip});
	if(_sessionState.objectOverrides().size()-state.objectOverrides.size()+prepared.size()>65536)
		throw std::invalid_argument("Too many object overrides");
	// Allocate the disposable draw-state entry before the owned stores.
	_scenePresentation.objectFlips[id]=flip;
	XeenMutationWatch::write(this);state.objectOverrides.swap(prepared);
}

void XeenWorld::restoreObjectOverrides(const std::vector<XeenObjectOverride> &values) {
	if(hasEncounterState() || values.size()>65536)throw std::invalid_argument("Invalid object restoration owner/count");
	for(unsigned n=0;n<values.size();++n) {
		const auto &v=values[n];validateObject(v.id);
		if(hasEncounterState() || v.x<-128 || v.x>127 || v.y<-128 || v.y>127 ||
			(n && !(values[n-1].id<v.id)))throw std::invalid_argument("Invalid or duplicate object override");
	}
	std::map<XeenMapIdentity,std::vector<XeenObjectOverride>> prepared;
	for(const auto &v:values) {prepared[v.id.mapId].push_back(v);static_cast<void>(_sessionState._mapStates.state(v.id.mapId));}
	XeenMutationWatch::write(this);
	for(const auto &entry:_sessionState._mapStates) {
		auto &state=*_sessionState._mapStates.find(entry.first);const auto found=prepared.find(entry.first);
		if(found!=prepared.end())state.objectOverrides.swap(found->second);else state.objectOverrides.clear();
	}
}

std::optional<XeenObjectIdentity> XeenWorld::selectObject(const XeenCamera &camera) {
	// An admitted indoor map (its monster table is staged) spans 32x32 cells
	// through its neighbour tiles.
	const auto *state=_sessionState.mapState(camera.mapId);
	const bool city=regionalJourney() && state && state->monsters && !state->outdoors;
	if (!camera.mapId || camera.x < 0 || camera.x >= (city?32:16) ||
		camera.y < 0 || camera.y >= (city?32:16))
		throw std::invalid_argument("invalid physical object-selection cell");
	const auto &file = objectFile(camera.mapId);
	if (!file.resourcePresent) return std::nullopt;
	for (std::size_t i = 0; i < file.entities.objects.size(); ++i) {
		XeenObjectIdentity id{camera.mapId, i};
		const auto object = effectiveObject(id);
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
	auto &state=_sessionState._mapStates.state(id.mapId);
	XeenMutationWatch::write(this);
	state.removedObjects.insert(id);
}

void XeenWorld::validateEventCell(const XeenCamera &physical, const XeenEventFile &events) {
	if (!physical.mapId || physical.x < 0 || physical.x > 15 || physical.y < 0 || physical.y > 15 ||
			events.mapId != physical.mapId)
		throw std::invalid_argument("event mutation requires the physical map/cell");
	static_cast<void>(map(physical.mapId));
}

void XeenWorld::disableEventsAtCell(const XeenCamera &physical, const XeenEventFile &events) {
	validateEventCell(physical, events);
	auto &state=_sessionState._mapStates.state(physical.mapId);
	XeenMutationWatch::write(this);
	for (std::size_t i = 0; i < events.records.size(); ++i) {
		const auto &record = events.records[i];
		if (record.x == physical.x && record.y == physical.y)
			state.disabledEvents.insert({physical.mapId, i});
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
	auto &state = _sessionState._mapStates.state(physical.mapId);
	auto disabledObjects = state.removedObjects;
	auto disabledEvents = state.disabledEvents;
	if (selected) disabledObjects.insert(*selected);
	for (std::size_t i = 0; i < events.records.size(); ++i) {
		const auto &record = events.records[i];
		if (record.x == physical.x && record.y == physical.y)
			disabledEvents.insert({physical.mapId, i});
	}
	if (publication) publication->prepareRemove(physical, selected, events);
	static_assert(noexcept(state.removedObjects.swap(disabledObjects)));
	static_assert(noexcept(state.disabledEvents.swap(disabledEvents)));
	XeenMutationWatch::write(this);
	state.removedObjects.swap(disabledObjects);
	state.disabledEvents.swap(disabledEvents);
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
	const auto *tile=_sessionState.mapState(result.mapId);
	if(tile)for(const auto &v:tile->cells) if(v.cell==y*16+x) {
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
	const unsigned index=y*16+x;
	if(!result.cell->stepped && tile && tile->explored && ((*tile->explored)[index/8]&(1u<<(index%8)))) {
		auto cell=std::make_shared<XeenMapCell>(*result.cell);cell->stepped=true;
		result.effectiveCell=std::move(cell);result.cell=result.effectiveCell.get();
	}
	return result;
}

std::vector<XeenExploredTile> XeenWorld::explorationAfterStep(const XeenPartyState &party,const XeenCamera &camera) {
	auto values=_sessionState.exploredTiles();
	if(!xeenPartyHasSkill(party,4))return values;
	const auto cell=sampleCell(camera.mapId,camera.x,camera.y);
	if(!cell)throw std::invalid_argument("Exploration has no physical occupied cell");
	// Resource defaults already participate in effective sampling. Never copy
	// seen flags or fill a visibility radius into the persistent overlay.
	if(cell->cell->stepped)return values;
	const auto at=std::lower_bound(values.begin(),values.end(),cell->mapId,[](const auto &v,auto id){return v.mapId<id;});
	const auto tile=at!=values.end() && at->mapId==cell->mapId ? at : values.insert(at,{cell->mapId,{}});
	const unsigned index=cell->y*16+cell->x;tile->cells[index/8]|=1u<<(index%8);
	if(values.size()>512)throw std::invalid_argument("Too many explored tiles");
	_sessionState.reserveTiles(values); // Publication of these values never allocates.
	return values;
}

void XeenWorld::recordExploration(const XeenPartyState &party,const XeenCamera &camera) {
	auto values=explorationAfterStep(party,camera);
	if(values==_sessionState.exploredTiles())return;
	if(_combatCheck)_combatCheck();
	XeenMutationWatch::write(this);_sessionState.publishExploration(values);
}

void XeenWorld::restoreExploredTiles(const std::vector<XeenExploredTile> &values) {
	if(hasEncounterState() || values.size()>512)throw std::invalid_argument("Invalid exploration restoration owner/count");
	for(std::size_t n=0;n<values.size();++n) {
		const auto &v=values[n];
		if(!v.mapId || v.mapId.side!=XeenSide::Clouds || (n && !(values[n-1].mapId<v.mapId)))
			throw std::invalid_argument("Invalid explored tile identity/order/bitset");
		if(map(v.mapId).identity()!=v.mapId || hasEncounterState())
			throw std::invalid_argument("Explored tile original resource mismatch");
	}
	_sessionState.reserveTiles(values);XeenMutationWatch::write(this);_sessionState.publishExploration(values);
}

void XeenWorld::setBarrier(const XeenCamera &camera,std::uint8_t wall,bool unlock) {
	if(!detachedEventCandidate() || unsigned(camera.direction)>3 || wall>15)
		throw std::logic_error("Barrier changes require detached capability");
	constexpr int dx[]{0,1,0,-1},dy[]{1,0,-1,0};const unsigned direction=unsigned(camera.direction);
	const auto a=sampleCell(camera.mapId,camera.x,camera.y);
	const auto b=sampleCell(camera.mapId,camera.x+dx[direction],camera.y+dy[direction]);
	if(!a || !b || a->geometry->isOutdoors() || b->geometry->isOutdoors())
		throw std::invalid_argument("Barrier has no two indoor faces");
	auto values=_sessionState.barriers();
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
	XeenMutationWatch::write(this);_sessionState.assignCells(values);
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
	XeenMutationWatch::write(this);_sessionState.assignCells(values);
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
	candidate.questFlags=source.questFlags;candidate.worldFlags=source.worldFlags;
	candidate.food=source.food;
	candidate.firstSerializedCount=source.firstSerializedCount;candidate.effectiveSerializedCount=source.effectiveSerializedCount;
	candidate.diagnostics=source.diagnostics;
}

void XeenWorld::applyAlterEvent(const XeenCamera &physical, std::uint8_t line,
		std::uint8_t replacement, const XeenEventFile &events) {
	XeenMutationWatch::write(this);
	if (!regionalJourney() || _sessionState._entry!=XeenEncounterEntry::Ordinary ||
		physical.mapId!=events.mapId || replacement!=0)
		throw std::invalid_argument("AlterEvent replacement is unsupported");
	auto &state=_sessionState._mapStates.state(physical.mapId);
	bool found=false;
	for (std::size_t i=0;i<events.records.size();++i) {
		const auto &r=events.records[i];
		if (r.x==physical.x && r.y==physical.y && r.line==line &&
			(r.direction==static_cast<std::uint8_t>(physical.direction) || r.direction==4)) {
			state.disabledEvents.insert({physical.mapId,i});found=true;
		}
	}
	if (!found) throw std::invalid_argument("AlterEvent physical line was not found");
}

void XeenWorld::publishTransition(XeenWorld &candidate) noexcept {
	XeenMutationWatch::write(this);XeenMutationWatch::write(&candidate);
	_sessionState._journeyRandom.swap(candidate._sessionState._journeyRandom);
	// The whole per-map table moves as one value.
	_sessionState._mapStates.swap(candidate._sessionState._mapStates);
	_sessionState._accountedMonsters.swap(candidate._sessionState._accountedMonsters);
	_sessionState._headCache.swap(candidate._sessionState._headCache);
	_cityStatistics.swap(candidate._cityStatistics);
	_maps.swap(candidate._maps);_objects.swap(candidate._objects);
	++_ownerRevision;
}

void XeenWorld::retainEventRandom(const XeenJourneyRandomState &random) {
	if(!detachedEventCandidate() || !_sessionState._journeyRandom)
		throw std::logic_error("Event RNG requires retained detached Journey state");
	_sessionState._journeyRandom=random;
}

void XeenWorld::applyEventGrant(XeenPartyState &party,std::uint8_t mode,std::uint32_t value,
		std::optional<std::size_t> member,bool entireParty,bool give,bool sparkle) {
	if(!detachedEventCandidate() || party.party.size()==0 || (!entireParty && (!member || *member>=party.party.size())))
		throw std::invalid_argument("Event recipient is unavailable");
	if(mode==34 || mode==35) {
		if(!party.monsterTreasure || (!give && mode!=34))throw std::invalid_argument("Event currency mode is unsupported");
		auto &purse=*party.monsterTreasure;
		if(!give) {
			if(purse.gold<value)throw std::invalid_argument("Insufficient Event gold");
			purse.gold-=value;
		} else if(mode==34)purse.gold+=value;else purse.gems+=value;
		return; // Party currency is granted once, independent of selector.
	}
	if(mode!=13 && mode!=15 && mode!=16)throw std::invalid_argument("Event character mode is unsupported");
	if(mode==13 && !xeenSkillSupported(value))throw std::invalid_argument("Event skill: not supported yet");
	if((mode==15 && value>=128) || (mode==16 && !give))throw std::invalid_argument("Event award/XP operand is unsupported");
	for(std::size_t i=0;i<party.party.size();++i)if(entireParty || i==*member) {
		const auto owner=party.party.activeRosterIds()[i];
		if(mode==13) {
			// follows the original byte storage; not confirmed in DOS
			auto &character=party.roster._characters.at(owner);
			if(give)xeenGrantSkill(character,value);else xeenRemoveSkill(character,value);
		} else if(mode==15)party.roster._characters.at(owner).awards[XeenCharacter::awardIndex(value)]=give?1:0;
		else {
			if(!party.roster._combatInputs[owner])throw std::invalid_argument("Event XP owner is absent");
			party.roster._combatInputs[owner]->experience+=value;
		}
		// Party::giveTake uses spellFX for skill, award and XP grants;
		// Convicted Thief (award 8) deliberately has no sparkle.
		if(sparkle && give && (mode!=15 || value!=8)) {
			_scenePresentation.spellEffect(owner,0,true);
			_scenePresentation.portraits[owner].spellDeadline=0; // Start only after publication.
		}
	}
}

void XeenWorld::publishEventParty(XeenPartyState &live,const XeenPartyState &candidate) noexcept {
	for(unsigned owner=0;owner<30;++owner) {
  auto &character=live.roster._characters[owner];const auto &after=candidate.roster._characters[owner];
  character.currentHp=after.currentHp;character.conditions=after.conditions;
  character.temporaryLevel=after.temporaryLevel;
  character.intellect.temporary=after.intellect.temporary;character.personality.temporary=after.personality.temporary;
  character.endurance.temporary=after.endurance.temporary;
		live.roster._characters[owner].skills=candidate.roster._characters[owner].skills;
		live.roster._characters[owner].awards=candidate.roster._characters[owner].awards;
		live.roster._characters[owner].weapons=candidate.roster._characters[owner].weapons;
		live.roster._characters[owner].armor=candidate.roster._characters[owner].armor;
		live.roster._characters[owner].accessories=candidate.roster._characters[owner].accessories;
		live.roster._characters[owner].miscellaneous=candidate.roster._characters[owner].miscellaneous;
		if(live.roster._combatInputs[owner])*live.roster._combatInputs[owner]=*candidate.roster._combatInputs[owner];
	}
 live.encounterContext=candidate.encounterContext;live.worldFlags=candidate.worldFlags;
	if(live.monsterTreasure) {
		live.monsterTreasure->gold=candidate.monsterTreasure->gold;
		live.monsterTreasure->gems=candidate.monsterTreasure->gems;
	}
}

XeenHeadIndicators XeenWorld::headIndicators(const XeenCamera &camera) {
 if(camera.x<0 || camera.y<0)throw std::invalid_argument("Negative head coordinate");
 const unsigned cell=(camera.y%16)*16+camera.x%16;
 const auto &cache=_sessionState._headCache;
 if(cache && cache->mapId==camera.mapId && cache->overrides[cell])return *cache->overrides[cell];
 const auto &heads=map(camera.mapId).heads;
 if(!heads)throw std::runtime_error("Primary map has no HED resource");
 return (*heads)[cell];
}
void XeenWorld::alterHeads(const XeenCamera &camera,XeenHeadIndicators heads) {
 if(!detachedEventCandidate() || camera.x<0 || camera.y<0)throw std::logic_error("Head changes require a detached Event");
 XeenMutationWatch::write(this);
 auto &cache=_sessionState._headCache;
 if(!cache || cache->mapId!=camera.mapId)cache=XeenHeadCache{camera.mapId,{}};
 cache->overrides[(camera.y%16)*16+camera.x%16]=heads;
}
void XeenWorld::reloadHeads(XeenMapIdentity destination) {
 if(!detachedEventCandidate())throw std::logic_error("Head reload requires a detached map transition");
 XeenMutationWatch::write(this);
 _sessionState._headCache.reset();
 static_cast<void>(map(destination));
}
void XeenWorld::restoreHeads(const std::optional<XeenHeadCache> &heads,const XeenCamera &camera) {
 if(hasEncounterState() || (heads && heads->mapId!=camera.mapId))throw std::invalid_argument("Invalid restored head cache");
 XeenMutationWatch::write(this);_sessionState._headCache=heads;
}

} // namespace mmodern
