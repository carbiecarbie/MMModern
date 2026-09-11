#ifndef MMODERN_ENCOUNTER_TEST_SUPPORT_H
#define MMODERN_ENCOUNTER_TEST_SUPPORT_H
#include "XeenSaveTestSupport.h"
#include "games/xeen/XeenActorApproach.h"
#include "games/xeen/XeenSaveState.h"
#include "formats/xeen/XeenGameplayContextFormat.h"
#include "formats/xeen/XeenMapFormat.h"

namespace encounter_test {
using namespace mmodern;
using save_test::check;
using save_test::rejects;
using Bytes = std::vector<std::uint8_t>;
inline Bytes pty() {
	Bytes b(812,0); b[0]=b[1]=6;
	const Bytes ids{0,18,14,11,1,6,255,255};
	std::copy(ids.begin(),ids.end(),b.begin()+2);
	b[612]=1; b[614]=0x62; b[615]=2; b[616]=0xe0; b[617]=1;
	return b;
}
inline XeenPartyState party() {
	Bytes b(30*354,0);
	for (std::size_t i=0;i<30;++i) {
		b[i*354]='A'; b[i*354+35]=1; b[i*354+342]=12;
		b[i*354+346]=0x50; b[i*354+347]=2;
	}
	return XeenPartyLoader().loadFromResources(b,pty());
}
inline XeenMap map() {
	XeenMap m; m.geometry.id=20; m.geometry.flags2=0x8000; m.geometry.surfaceTypes[1]=1;
	for (auto &c:m.geometry.cells) { c.rawWord=0x31; c.surfaceIndex=1; c.geometry=XeenOutdoorLayers{1,3,0,0}; }
	for (auto index : {13, 28}) {
		auto &c=m.geometry.cells[index]; c.rawWord=15; c.surfaceIndex=15; c.geometry=XeenOutdoorLayers{15,0,0,0};
	}
	m.geometry.surfaceTypes[15]=15;
	return m;
}
inline std::vector<XeenMonsterRecord> stats() {
	std::vector<XeenMonsterRecord> s(12);
	for (auto &r:s) { r.raw[20]=20; r.raw[47]=42; }
	return s;
}
inline XeenObjectFile mob() {
	XeenObjectFile m{20,"synthetic.mob",true,{}};
	m.entities.objects.push_back({13,1,0,0,7});
	for (int i=0;i<6;++i) m.entities.monsters.push_back({-128,i,0,0,8});
	m.entities.monsters[5]={13,2,0,0,8};
	return m;
}
inline XeenEventFile events() { XeenEventFile e; e.mapId=20; e.resourcePresent=true; return e; }
inline void sameActors(const std::vector<XeenActor> &a,const std::vector<XeenActor> &b) {
	check(a.size()==b.size(),"actor count changed");
	for (std::size_t i=0;i<a.size();++i) {
		check(a[i].id==b[i].id && a[i].x==b[i].x && a[i].y==b[i].y && a[i].hp==b[i].hp &&
			a[i].activated==b[i].activated && a[i].lifecycle==b[i].lifecycle && a[i].status==b[i].status,
			"live actor facts changed");
		check(a[i].original.x==b[i].original.x && a[i].original.y==b[i].original.y &&
			a[i].original.tableIndex==b[i].original.tableIndex && a[i].original.direction==b[i].original.direction &&
			a[i].original.resourceId==b[i].original.resourceId,"original actor metadata changed");
		check(bool(a[i].statistics)==bool(b[i].statistics),"actor metadata presence changed");
		if(a[i].statistics) check(a[i].statistics->raw==b[i].statistics->raw,"actor statistics changed");
	}
}
inline void sameParty(const XeenPartyState &a,const XeenPartyState &b) {
	for (std::size_t i=0;i<30;++i) remove_test::checkSameCharacter(a.roster.at(i),b.roster.at(i));
	check(a.party.activeRosterIds()==b.party.activeRosterIds() && a.questItems.counts()==b.questItems.counts() &&
		a.questFlags.values()==b.questFlags.values() && a.firstSerializedCount==b.firstSerializedCount &&
		a.effectiveSerializedCount==b.effectiveSerializedCount && a.diagnostics==b.diagnostics,
		"non-context party facts changed");
}
struct Fixture {
	XeenMap terrain=map(); XeenObjectFile objects=mob(); XeenEventFile evt=events();
	XeenPartyState p=party(); XeenCamera camera=XeenActorApproach::kEntry;
	std::vector<XeenMonsterRecord> statistics=stats();
	XeenGameplayContext context=XeenGameplayContextFormat::parse(pty());
	XeenEncounterState state;
	unsigned mapLoads=0,mobLoads=0;
	bool failMap=false;
	XeenWorld world{[&](XeenMapIdentity id) {
		++mapLoads; if(failMap) throw std::runtime_error("injected resource preparation failure");
		auto m=terrain;m.geometry.id=id.number;return m;
	},[&](XeenMapIdentity id) { ++mobLoads; auto m=objects;m.mapId=id;return m; }};
	XeenEncounterResult start() { return XeenActorApproach::initialize(world,p,camera,state,statistics,context,evt); }
	XeenEncounterResult action(XeenEncounterAction a) { return XeenActorApproach::action(world,p,camera,state,a,evt); }
	XeenEncounterResult pulse() { return XeenActorApproach::pulse(world,p,camera,state,evt); }
	XeenEncounterResult input(XeenEncounterAction a) { action(a);return pulse(); }
	const XeenActor &anchor() const { return world.sessionState().actors().at(5); }
};
}
#endif
