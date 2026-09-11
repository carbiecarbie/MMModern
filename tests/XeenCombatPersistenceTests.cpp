#include "XeenCombatTestSupport.h"
#include <iostream>
using namespace combat_test;
struct Ordinary {
	XeenPartyState p=XeenPartyLoader().loadFromResources(chr(),pty());XeenCamera camera=XeenActorApproach::kEntry;XeenGameFlags flags;
	XeenWorld world{[](XeenMapIdentity){return map();},[](XeenMapIdentity){return objects();}};
	XeenSaveResourceSignature signature=save_test::sample().resources;
	XeenSaveSnapshot capture(){return XeenSaveState::capture(signature,p,camera,flags,world);}
	XeenSaveState::Resources resources(){return {signature,[]{return XeenPartyLoader().loadFromResources(chr(),pty());},[](XeenMapIdentity){return events();}};}
};
void persistence() {
	Ordinary source;const auto snapshot=source.capture();
	for(unsigned mode=0;mode<3;++mode) {
		Ordinary destination;const auto before=destination.capture();auto resources=destination.resources();
		XeenSaveState::Preflight preflight=[](XeenWorld &,const XeenPartyState &,const XeenCamera &,const XeenGameFlags &){};
		if(mode==0)resources.loadInitialParty=[] {
			auto p=XeenPartyLoader().loadFromResources(chr(),pty());auto c=XeenActorApproach::kEntry;
			XeenWorld w([](XeenMapIdentity){return map();},[](XeenMapIdentity){return objects();});XeenCombatBoundary b(w,p,c);
			XeenCombat combat(w,p,c,b,chr(),XeenGameplayContextFormat::parse(pty()),statistics(),events());return p;
		};
		if(mode==1)preflight=[](XeenWorld &,const XeenPartyState &party,const XeenCamera &,const XeenGameFlags &) {
			// Attack the candidate graph through a detached marked roster, leaving
			// the actual candidate world unmarked and context absent.
			auto &p=const_cast<XeenPartyState &>(party);auto c=XeenActorApproach::kEntry;
			XeenWorld w([](XeenMapIdentity){return map();},[](XeenMapIdentity){return objects();});XeenCombatBoundary b(w,p,c);
			XeenCombat combat(w,p,c,b,chr(),XeenGameplayContextFormat::parse(pty()),statistics(),events());
		};
		if(mode==2)preflight=[&](XeenWorld &,const XeenPartyState &,const XeenCamera &,const XeenGameFlags &){destination.world.markEncounterSession();};
		rejects([&]{XeenSaveState::restoreBeforeGameplay(snapshot,resources,destination.p,destination.camera,destination.flags,destination.world,preflight);});
		for(unsigned i=0;i<30;++i)remove_test::checkSameCharacter(before.characters[i],destination.p.roster.at(i));
		check(destination.camera.x==before.camera.x&&!destination.p.roster.combatMarked(),"failed restore preserves destination graph");
	}
	// Direct world restoration rechecks each map/object/event callback and never
	// publishes overlays after a callback irreversibly marks that same world.
	for(unsigned mode=0;mode<3;++mode) {
		XeenWorld *ptr=nullptr;XeenWorld w([&](XeenMapIdentity){if(mode==0)ptr->markEncounterSession();return map();},
			[&](XeenMapIdentity){if(mode==1)ptr->markEncounterSession();return objects();});ptr=&w;
		const auto load=[&](XeenMapIdentity){if(mode==2)w.markEncounterSession();auto e=events();e.records.emplace_back();return e;};
		rejects([&]{w.restoreSessionState({{20,0}},{{20,0}},load);},"encounter");
		check(w.hasEncounterState()&&w.sessionState().disabledObjectCount()==0&&w.sessionState().disabledEventCount()==0,"callback cannot overwrite encounter overlays");
	}
	// Ordinary v2 still round-trips here; existing save tests retain v1 coverage.
	const auto bytes=XeenSaveFormat::encode(snapshot);const auto decoded=XeenSaveFormat::decode(bytes);
	Ordinary destination;auto resources=destination.resources();
	XeenSaveState::restoreBeforeGameplay(decoded,resources,destination.p,destination.camera,destination.flags,destination.world,
		[](XeenWorld &,const XeenPartyState &,const XeenCamera &,const XeenGameFlags &){});
	save_test::sameSnapshot(snapshot,destination.capture());
	CombatFixture marked;marked.combat.reset();marked.p.encounterContext.reset();unsigned calls=0;
	resources.loadInitialParty=[&]{++calls;return XeenPartyLoader().loadFromResources(chr(),pty());};
	rejects([&]{XeenSaveState::restoreBeforeGameplay(snapshot,resources,marked.p,destination.camera,destination.flags,destination.world,
		[&](XeenWorld &,const XeenPartyState &,const XeenCamera &,const XeenGameFlags &){++calls;});},"encounter");
	check(calls==0,"detached destination marker refuses before providers");
}
int main(){try{persistence();std::cout<<"Combat detached-roster and restore callback guards passed\n";return 0;}catch(const std::exception &e){std::cerr<<e.what()<<'\n';return 1;}}
