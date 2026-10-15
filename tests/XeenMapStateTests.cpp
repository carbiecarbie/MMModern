// M69 per-map state owner: table exchange, absent/empty equivalence, bounds and
// the world's per-map distribution behind the flat (v12) projections.
#include "games/xeen/XeenWorld.h"
#include "games/xeen/XeenStateEquality.h"
#include "games/xeen/XeenRestoreGuard.h"
#include <iostream>
#include <stdexcept>

using namespace mmodern;
namespace {
void check(bool value,const char *message) {if(!value)throw std::runtime_error(message);}
XeenActor actor(XeenMapIdentity map,std::size_t index,int hp) {
	XeenActor a;a.id={map,index};a.x=int(index);a.y=1;a.hp=hp;a.lifecycle=XeenActorLifecycle::Present;return a;
}
XeenMap tile(XeenMapIdentity id) {XeenMap m;m.geometry.id=id.number;m.side=id.side;return m;}
XeenObjectFile objects(XeenMapIdentity id) {
	XeenObjectFile file{id,"objects",true,{}};
	for(unsigned n=0;n<4;++n){XeenMapEntity e;e.x=n;e.y=n;file.entities.objects.push_back(e);}
	file.entities.wallItems.push_back({2,3,0,1,7});
	return file;
}
XeenEventFile events(XeenMapIdentity id) {return XeenEventFile{id,"events",true,std::vector<XeenEventRecord>(4)};}

void exchange() {
	XeenMapTable live,candidate;
	live.state(23).monsters.emplace(std::vector<XeenActor>{actor(23,0,5)});
	live.state(109).explored=std::array<std::uint8_t,32>{};
	candidate.state(23).monsters.emplace(std::vector<XeenActor>{actor(23,0,4),actor(23,1,6)});
	candidate.state(28).removedObjects.insert({28,3});
	const auto *entry=live.find(23);const auto *table=&*entry->monsters;
	live.swap(candidate);
	check(live.find(23)==entry && &*live.find(23)->monsters==table && live.find(23)->monsters->size()==2,
		"A common entry must exchange its content in place");
	check(live.find(28) && live.find(28)->removedObjects.count({28,3}) && !live.find(109),
		"An entry present on one side must move to the other");
	check(candidate.find(109) && candidate.find(23)->monsters->size()==1 && !candidate.find(28),
		"The exchange must be symmetric");
}
void equivalence() {
	XeenMapTable a,b;a.state(5);
	check(xeen_state::sameMapStates(a,b) && xeen_state::sameMapStates(b,a) && a.empty(),"An empty entry must equal an absent one");
	b.state(5).disabledEvents.insert({5,1});
	check(!xeen_state::sameMapStates(a,b),"A disabled Event must be observable");
	XeenMapTable staged,absent;staged.state(7).monsters.emplace();
	check(!xeen_state::sameMapStates(staged,absent),"A staged empty monster table must differ from none");
	XeenMapTable hp,other;hp.state(9).monsters.emplace(std::vector<XeenActor>{actor(9,0,5)});
	other.state(9).monsters.emplace(std::vector<XeenActor>{actor(9,0,4)});
	check(!xeen_state::sameMapStates(hp,other),"Monster values must be compared");
	XeenMapTable changed,empty;
	changed.state(5).wallItemOverrides.push_back({{5,0},4,5});
	check(!xeen_state::sameMapStates(changed,empty) && !changed.empty(),"Wall-item overrides must be observable");
	empty=changed;empty.state(5).wallItemOverrides[0].x=6;
	check(!xeen_state::sameMapStates(changed,empty),"Wall-item coordinates must be compared");
	changed.state(5).wallItemOverrides.clear();changed.state(5).eventOverrides.push_back({{5,0},42});
	check(!xeen_state::sameMapStates(changed,XeenMapTable{}) && !changed.empty(),"Event replacements must be observable");
	empty=changed;empty.state(5).eventOverrides[0].opcode=43;
	check(!xeen_state::sameMapStates(changed,empty),"Event opcodes must be compared");
}
void bounds() {
	XeenMapTable table;
	for(unsigned n=1;n<=XeenMapTable::kMaximumMaps;++n)static_cast<void>(table.state(n));
	bool full=false,zero=false;
	try {table.state(XeenMapTable::kMaximumMaps+1);} catch(const std::length_error &) {full=true;}
	try {XeenMapTable().state(0);} catch(const std::invalid_argument &) {zero=true;}
	check(full && zero && table.size()==XeenMapTable::kMaximumMaps,"The table must stay bounded and keyed by real maps");
}
void distribution() {
	XeenWorld world(tile,objects);
	world.restoreSessionState({{2,1},{1,0}},{{2,0},{1,3}},events);
	const auto &state=world.sessionState();
	check(state.mapState(1)->removedObjects==std::set<XeenObjectIdentity>{{1,0}} &&
		state.mapState(2)->disabledEvents==std::set<XeenEventIdentity>{{2,0}},"Overlays must belong to their own map");
	check(state.disabledObjects()==std::set<XeenObjectIdentity>{{1,0},{2,1}} && state.disabledEventCount()==2 &&
		state.isEventDisabled({1,3}) && !state.isEventDisabled({2,3}),"The flat projection must keep identity order");
	world.restoreObjectOverrides({{{1,2},7,8,true},{{2,0},-128,-128,false}});
	check(state.mapState(1)->objectOverrides.size()==1 && state.mapState(2)->objectOverrides.size()==1 &&
		world.effectiveObject({1,2}).x==7 && state.objectOverrides().front().id==XeenObjectIdentity{1,2},
		"Object overrides must belong to their own map");
	XeenExploredTile a{3,{}},b{7,{}};b.cells[31]=128;
	world.restoreExploredTiles({a,b});
	const auto explored=state.exploredTiles();
	check(explored.size()==2 && explored[0].mapId==XeenMapIdentity(3) && explored[1]==b &&
		state.mapState(3)->explored.has_value(),"Explored bits must belong to their physical tile, zero presence kept");
	check(!state.hasRegionalActors(23) && state.monsters(23).empty() && !state.anyMonsters(),"No monster table is staged here");
	bool absent=false;try {static_cast<void>(state.regionalActors(23));} catch(const std::out_of_range &) {absent=true;}
	check(absent,"An absent monster table must be refused");
}
void mapOverrideGuards() {
	XeenWorld world(tile,objects);
	world.restoreWallItemOverrides({{{5,0},-7,127}});
	world.restoreEventOverrides({{{5,1},42}},events);
	const auto item=world.effectiveWallItem({5,0});
	check(item.x==-7 && item.y==127 && item.direction==1 && item.resourceId==7,
		"Effective wall items preserve resource metadata");
	check(world.objectFile(5).entities.wallItems[0].x==2 && world.effectiveEvent({5,1},events(5).records[1]).opcode==42,
		"Effective reads leave original resources immutable");
	XeenPartyState party;XeenCamera camera{5,0,0,XeenDirection::North};XeenGameFlags flags;
	auto &state=*const_cast<XeenMapState *>(world.sessionState().mapState(5));
	// Aliases precede every guard; each byte changes and returns to its initial
	// value without a guard refresh. Dynamic override storage must be watched.
	for(unsigned field=0;field<11;++field) {
		XeenRestoreGuard guard(world,party,camera,flags);
		auto &wall=state.wallItemOverrides[0];auto &event=state.eventOverrides[0];
		switch(field) {
		case 0:++wall.x;--wall.x;break;
		case 1:++wall.y;--wall.y;break;
		case 2:++wall.id.recordIndex;--wall.id.recordIndex;break;
		case 3:++wall.id.mapId.number;--wall.id.mapId.number;break;
		case 4:++event.opcode;--event.opcode;break;
		case 5:++event.id.recordIndex;--event.id.recordIndex;break;
		case 6:++event.id.mapId.number;--event.id.mapId.number;break;
		case 7:wall.id.mapId.side=XeenSide::Darkside;wall.id.mapId.side=XeenSide::Clouds;break;
		case 8:event.id.mapId.side=XeenSide::Darkside;event.id.mapId.side=XeenSide::Clouds;break;
		case 9:{const auto before=state.wallItemOverrides;state.wallItemOverrides.clear();state.wallItemOverrides=before;break;}
		case 10:{const auto before=state.eventOverrides;state.eventOverrides.clear();state.eventOverrides=before;break;}
		}
		check(!guard.current(),"A reverted map override mutation must invalidate the guard");
	}
	world.restoreSessionState({},{{5,1}},events);
	check(state.eventOverrides.empty() && world.effectiveEvent({5,1},events(5).records[1]).opcode==0,
		"Disabling a replacement leaves one durable opcode value");
	bool duplicate=false,missing=false;
	try {world.restoreWallItemOverrides({{{5,0},1,2},{{5,0},3,4}});}catch(const std::invalid_argument &){duplicate=true;}
	try {world.restoreEventOverrides({{{5,99},1}},events);}catch(const std::invalid_argument &){missing=true;}
	check(duplicate && missing && world.effectiveWallItem({5,0}).x==-7,
		"Invalid override restoration publishes no changes");
}
}
void warzone() {
	// Decision 2: Clouds 85 (The Warzone) is never retained; every other map,
	// and a Darkside map 85, keeps its state when the party leaves.
	check(!xeenRetainsMapState(85) && xeenRetainsMapState(84) && xeenRetainsMapState(28) &&
		xeenRetainsMapState({XeenSide::Darkside,85}),"Only Clouds 85 is never retained");
	XeenMapTable table;
	table.state(85).removedObjects.insert({85,1});table.state(28).removedObjects.insert({28,1});
	table.state({XeenSide::Darkside,85}).explored=std::array<std::uint8_t,32>{};
	table.leave(28);table.leave(85);table.leave({XeenSide::Darkside,85});
	check(!table.find(85) && table.find(28) && table.find({XeenSide::Darkside,85}),"Leaving drops only the Warzone");
	check(table.state(85).empty(),"Re-entering the Warzone starts fresh");
}
void mapLoad() {
	// Decision 8 (DOS map load 0x0f542): a live monster returns to its MON HP,
	// asleep; defeated, disabled and unresolved slots keep their values.
	XeenMonsterRecord record;record.raw.fill(0);record.raw[20]=25;
	XeenActor live=actor(23,0,4);live.statistics=record;live.activated=true;
	xeenRefillLoadedMonster(live);
	check(live.hp==25 && !live.activated,"A map load must rebuild a live monster asleep at MON HP");
	XeenActor dead=actor(23,1,0);dead.statistics=record;dead.lifecycle=XeenActorLifecycle::Defeated;
	xeenRefillLoadedMonster(dead);
	check(dead.hp==0 && dead.lifecycle==XeenActorLifecycle::Defeated,"A defeated monster stays defeated");
	XeenActor gap=actor(28,2,0);gap.lifecycle=XeenActorLifecycle::Unresolved;
	xeenRefillLoadedMonster(gap);check(gap.hp==0,"An unresolved slot has no MON HP");
}
int main() {
	try {
		exchange();equivalence();bounds();distribution();mapOverrideGuards();warzone();mapLoad();
		std::cout<<"Per-map table exchange, equivalence, bounds and distribution passed\n";
	} catch(const std::exception &e) {std::cerr<<e.what()<<'\n';return 1;}
}
