// M69 per-map state owner: table exchange, absent/empty equivalence, bounds and
// the world's per-map distribution behind the flat (v12) projections.
#include "games/xeen/XeenWorld.h"
#include "games/xeen/XeenStateEquality.h"
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
		exchange();equivalence();bounds();distribution();mapLoad();
		std::cout<<"Per-map table exchange, equivalence, bounds and distribution passed\n";
	} catch(const std::exception &e) {std::cerr<<e.what()<<'\n';return 1;}
}
