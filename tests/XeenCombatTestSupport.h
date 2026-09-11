#ifndef MMODERN_COMBAT_TEST_SUPPORT_H
#define MMODERN_COMBAT_TEST_SUPPORT_H
#include "XeenEncounterTestSupport.h"
#include "games/xeen/XeenCombat.h"
#include "games/xeen/XeenCharacterRules.h"
namespace combat_test {
using namespace encounter_test;
using Phase=XeenCombatPhase;using Work=XeenCombatWork;using Status=XeenCombatStatus;
using Command=XeenCombatCommand;using Draw=XeenCombatRandom::Draw;
inline Bytes chr() {
	Bytes bytes(30*354,0);
	for(unsigned id=0;id<30;++id){bytes[id*354]='T';bytes[id*354+35]=1;bytes[id*354+346]=80;bytes[id*354+347]=2;}
	const int might[]{17,19,15,14,12,8},speed[]{16,16,15,15,14,14},accuracy[]{15,16,12,18,13,15};
	const int hp[]{12,16,12,10,7,5},sp[]{2,0,2,0,7,9},classes[]{1,0,9,5,3,4},weapon[]{6,2,8,12,15,7},end[]{19,25,17,15,15,13};
	for(unsigned i=0;i<6;++i){const auto b=kXeenCombatOwners[i]*354;
		bytes[b+19]=classes[i];bytes[b+20]=might[i];bytes[b+22]=bytes[b+24]=15;bytes[b+26]=end[i];
		bytes[b+28]=speed[i];bytes[b+30]=accuracy[i];bytes[b+342]=hp[i];bytes[b+344]=sp[i];
		unsigned slots[4]{};auto add=[&](unsigned cat,unsigned m,unsigned id,unsigned f){auto p=b+166+cat*36+4*slots[cat]++;bytes[p]=m;bytes[p+1]=id;bytes[p+3]=f;};
		add(0,0,weapon[i],1);if(i==2)add(0,0,30,4);if(i==3)add(0,0,12,0);
		add(1,0,i==0?3:i==5?1:2,3);if(i==0)add(1,0,8,2);if(i==1)add(1,0,9,5);
		if(i<3)add(1,0,13,6);add(1,38,10,9);if(i==3)add(1,38,11,10);
		add(2,38,2,12);if(i==3)add(2,42,1,8);if(i==4)add(2,42,5,8);if(i==5)add(2,86,1,0);
	}return bytes;
}
inline std::vector<XeenMonsterRecord> statistics() {
	auto values=encounter_test::stats();auto &r=values[8];r.raw[16]=250;r.raw[20]=20;r.raw[22]=5;r.raw[23]=10;
	r.raw[24]=1;r.raw[25]=3;r.raw[26]=2;r.raw[28]=6;r.raw[31]=4;r.raw[33]=4;
	for(unsigned i=34;i<38;++i)r.raw[i]=50;r.raw[40]=50;r.raw[47]=8;return values;
}
inline XeenObjectFile objects() {auto m=mob();while(m.entities.monsters.size()<27)m.entities.monsters.push_back({-128,0,0,0,8});return m;}
struct CombatFixture {
	Bytes bytes=chr();XeenPartyState p=XeenPartyLoader().loadFromResources(bytes,pty());
	XeenCamera camera=XeenActorApproach::kEntry;
	XeenWorld w{[](XeenMapIdentity){return map();},[](XeenMapIdentity){return objects();}};
	XeenCombatBoundary boundary{w,p,camera};std::unique_ptr<XeenCombat> combat;
	explicit CombatFixture(XeenCombatRandom rng=XeenCombatRandom(1)) {
		combat=std::make_unique<XeenCombat>(w,p,camera,boundary,bytes,XeenGameplayContextFormat::parse(pty()),statistics(),events(),std::move(rng));
	}
	void enter(bool delayed=false) {
		check(combat->beginApproach(combat->ticket()).status==Status::Accepted,"begin approach");
		if(delayed){for(auto a:{XeenEncounterAction::Right,XeenEncounterAction::Forward}){combat->approachAction(combat->ticket(),a);combat->approachPulse(combat->ticket());}
			combat->approachPulse(combat->ticket());combat->approachPulse(combat->ticket());}
		combat->approachAction(combat->ticket(),XeenEncounterAction::Wait);
		check(combat->phase()==Phase::Engaged,"real M26 engagement");
		check(combat->beginCombat(combat->ticket()).status==Status::Advanced,"one-time combat handoff");
	}
	XeenCombatResult action(Command a) {
		auto r=combat->command(combat->ticket(),a);
		while(combat->pending()==Work::Action)r=combat->service(combat->ticket());return r;
	}
	XeenCombatResult service(){return combat->service(combat->ticket());}
	void blockRound(){while(combat->phase()==Phase::PlayerReady)action(Command::Block);check(combat->pending()==Work::Enemy,"mandatory enemy");}
};
inline std::vector<Draw> mixedTape(){return {{1,2,1},{1,2,1},{1,2,1},{1,2,1},{1,20,10},
	{1,3,1},{1,3,1},{1,20,10},{1,2,1},{1,2,1},{1,20,1},{1,20,15},{1,4,4},
	{1,2,2},{1,2,2},{1,2,2},{1,2,2},{1,20,10}};}
inline std::vector<Draw> defeatTape(){std::vector<Draw> v;
	for(int round=0;round<6;++round){if(round)v.push_back({0,5,unsigned(round==5?5:round-1)});
		v.insert(v.end(),{{1,20,20},{1,6,6},{1,6,6},{1,4,4}});if(round!=1)v.insert(v.end(),{{1,6,6},{1,6,6}});
	}return v;
}
}
#endif
