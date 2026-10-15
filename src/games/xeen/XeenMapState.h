#ifndef MMODERN_XEEN_MAP_STATE_H
#define MMODERN_XEEN_MAP_STATE_H
#include "games/xeen/XeenActor.h"
#include "games/xeen/XeenBarrierState.h"
#include "games/xeen/XeenExploration.h"
#include "games/xeen/XeenObjectState.h"
#include "games/xeen/XeenRecordIdentity.h"
#include <array>
#include <cstddef>
#include <exception>
#include <stdexcept>
#include <iterator>
#include <map>
#include <optional>
#include <set>
#include <utility>
#include <vector>

namespace mmodern {

// Logical MOB wall-item positions and EVT opcodes, keyed by original record.
// Zero opcodes use the existing disabledEvents owner; these replacements are
// nonzero. Position changes never alter wall-item facing or sprite metadata.
struct XeenWallItemOverride {
	XeenWallItemIdentity id;
	XeenMutable<int> x=0,y=0;
	friend bool operator==(const XeenWallItemOverride &a,const XeenWallItemOverride &b) noexcept {
		return a.id==b.id && a.x==b.x && a.y==b.y;
	}
};
struct XeenEventOverride {
	XeenEventIdentity id;
	XeenMutable<std::uint8_t> opcode=0;
	friend bool operator==(const XeenEventOverride &a,const XeenEventOverride &b) noexcept {
		return a.id==b.id && a.opcode==b.opcode;
	}
};

// Live state of one original map, keyed by its identity. A logical map owns
// its MOB and EVT state (monsters, objects, Events); every physical DAT tile,
// including a neighbour tile of an indoor map, owns its own cell and explored
// state, as the original writes each tile's DAT back separately. Original
// resources remain immutable; every value is validated against them.
struct XeenMapState {
	// Every MOB record followed by script-created slots. Absence (never
	// staged) is distinct from an empty table.
	std::optional<std::vector<XeenActor>> monsters;
	XeenMutable<std::uint16_t> originalMonsterCount=0;
	// Geometry of the map the monster table belongs to, bound at staging.
	XeenMutable<bool> outdoors=false;
	std::set<XeenObjectIdentity> removedObjects;
	std::set<XeenEventIdentity> disabledEvents;
	// Resource-relative object positions and effective flips, by record.
	std::vector<XeenObjectOverride> objectOverrides;
	XeenMutableVector<XeenWallItemOverride> wallItemOverrides;
	XeenMutableVector<XeenEventOverride> eventOverrides;
	// This tile's changed cells, by cell, with their original preimage.
	std::vector<XeenBarrierOverride> cells;
	// This tile's explored bits (y*16+x). Presence is retained even when zero.
	std::optional<std::array<std::uint8_t,32>> explored;

	bool empty() const noexcept {
		return !monsters && removedObjects.empty() && disabledEvents.empty() &&
			objectOverrides.empty() && wallItemOverrides.empty() && eventOverrides.empty() && cells.empty() && !explored;
	}
	void swap(XeenMapState &other) noexcept {
		monsters.swap(other.monsters);
		const std::uint16_t count=originalMonsterCount;originalMonsterCount=std::uint16_t(other.originalMonsterCount);
		other.originalMonsterCount=count;
		const bool open=outdoors;outdoors=bool(other.outdoors);other.outdoors=open;
		removedObjects.swap(other.removedObjects);disabledEvents.swap(other.disabledEvents);
		objectOverrides.swap(other.objectOverrides);cells.swap(other.cells);explored.swap(other.explored);
		wallItemOverrides.swap(other.wallItemOverrides);eventOverrides.swap(other.eventOverrides);
	}
};

// Decision 8 (DOS map load 0x0f542: the monster array is zeroed at 0x0f5c7 and
// HP refilled from MON+0x14 at 0x0faa8; the save keeps positions only, 0x266e3):
// a map load rebuilds every live monster at its MON HP and asleep.
inline void xeenRefillLoadedMonster(XeenActor &actor) noexcept {
	if(actor.lifecycle!=XeenActorLifecycle::Present || !actor.statistics)return;
	const auto hp=std::int32_t(actor.statistics->baseHp());
	if(actor.hp!=hp)actor.hp=hp;
	if(actor.activated)actor.activated=false;
}

// Decision 2: the original never writes Clouds map 85 (The Warzone) back
// (pinned map.cpp Map::saveMaze); its state lasts for one visit only.
inline bool xeenRetainsMapState(XeenMapIdentity id) noexcept {
	return !(id.side == XeenSide::Clouds && id.number == 85);
}

// The per-map table. Entries are created on first touch and dropped only when
// the party leaves a map that is never retained; an empty entry is equivalent
// to an absent one. Content exchange keeps every
// entry's storage in its owning table, so views into a live entry stay valid.
class XeenMapTable {
	using Storage=std::map<XeenMapIdentity,XeenMapState>;
public:
	// Upper bound on entries (decision 5); also bounds mutation observation.
	static constexpr std::size_t kMaximumMaps=512;
	// Node, monsters, object/wall-item/Event overrides and cells.
	static constexpr std::size_t kObservedRangesPerMap=6;
	const XeenMapState *find(XeenMapIdentity id) const noexcept {
		const auto found=_states.find(id);return found==_states.end()?nullptr:&found->second;
	}
	XeenMapState *find(XeenMapIdentity id) noexcept {
		const auto found=_states.find(id);return found==_states.end()?nullptr:&found->second;
	}
	// Creates the entry on first touch. Fallible; precedes every owned store.
	XeenMapState &state(XeenMapIdentity id) {
		if(!id)throw std::invalid_argument("Map state requires a map identity");
		const auto found=_states.find(id);
		if(found!=_states.end())return found->second;
		if(_states.size()>=kMaximumMaps)throw std::length_error("Too many map states");
		return _states.emplace(id,XeenMapState{}).first->second;
	}
	// The party leaves `id`: a map that is never retained loses its state.
	void leave(XeenMapIdentity id) noexcept {
		if (!xeenRetainsMapState(id)) _states.erase(id);
	}
	auto begin() const noexcept { return _states.cbegin(); }
	auto end() const noexcept { return _states.cend(); }
	std::size_t size() const noexcept { return _states.size(); }
	bool empty() const noexcept {
		for(const auto &entry:_states)if(!entry.second.empty())return false;
		return true;
	}
	// Fallible: creates the entries a later nonthrowing adoption needs.
	void reserve(const XeenMapTable &other) {
		for(const auto &entry:other._states)static_cast<void>(state(entry.first));
	}
	// Moves every staged monster table onto its reserved entry; never allocates.
	void adoptMonsters(XeenMapTable &staged) noexcept {
		for(auto &entry:staged._states) {
			if(!entry.second.monsters)continue;
			auto *target=find(entry.first);
			if(!target)std::terminate();
			target->monsters.swap(entry.second.monsters);
			target->originalMonsterCount=std::uint16_t(entry.second.originalMonsterCount);
			target->outdoors=bool(entry.second.outdoors);
		}
	}
	// Exchanges every entry's content. Common entries swap in place; an entry
	// present on one side only moves its node, which never allocates.
	void swap(XeenMapTable &other) noexcept {
		Storage mine;
		for(auto i=_states.begin();i!=_states.end();) {
			const auto next=std::next(i);
			const auto found=other._states.find(i->first);
			if(found!=other._states.end())i->second.swap(found->second);
			else mine.insert(_states.extract(i));
			i=next;
		}
		for(auto i=other._states.begin();i!=other._states.end();) {
			const auto next=std::next(i);
			if(!_states.count(i->first))_states.insert(other._states.extract(i));
			i=next;
		}
		other._states.merge(mine);
	}
private:
	Storage _states;
};

}
#endif
