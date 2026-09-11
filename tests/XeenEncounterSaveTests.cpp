#include "XeenEncounterTestSupport.h"
#include <iostream>
using namespace encounter_test;

int main() {
	try {
		XeenSaveResourceSignature signature{{123,456},XeenArchiveFingerprint{789,1011}};
		XeenGameFlags flags;flags.set(7);
		Fixture ordinary;
		const auto snapshot=XeenSaveState::capture(signature,ordinary.p,ordinary.camera,flags,ordinary.world);
		for(int kind=0;kind<3;++kind) {
			Fixture f;
			if(kind==0)f.world.markEncounterSession();
			if(kind==1)f.p.encounterContext=f.context;
			if(kind==2)f.start();
			f.world.disableObject({20,0});
			XeenEventRecord event;event.x=4;event.y=4;f.evt.records.push_back(event);
			f.world.disableEventsAtCell({20,4,4,XeenDirection::North},f.evt);
			auto p=f.p;auto camera=f.camera;auto actors=f.world.sessionState().actors();
			const auto flagValues=flags.values();
			const auto objects=f.world.sessionState().disabledObjects();
			const auto events=f.world.sessionState().disabledEvents();
			const auto marked=f.world.sessionState().encounterMarked();
			unsigned calls=0;
			XeenSaveState::Resources resources{signature,[&]{++calls;return party();},[&](XeenMapIdentity){++calls;return encounter_test::events();}};
			for(int i=0;i<2;++i) {
				rejects([&]{XeenSaveState::capture(signature,f.p,f.camera,flags,f.world);},"encounter");
				rejects([&]{XeenSaveState::restoreBeforeGameplay(snapshot,resources,f.p,f.camera,flags,f.world,
					[&](XeenWorld &,const XeenPartyState &,const XeenCamera &,const XeenGameFlags &){++calls;});},"encounter");
				if(kind!=1)rejects([&]{f.world.restoreSessionState({}, {}, {});},"encounter");
				f.world.discardMapCache();
			}
			check(calls==0 && f.p.encounterContext==p.encounterContext && save_test::sameCamera(camera,f.camera) &&
				flags.values()==flagValues && f.world.sessionState().disabledObjects()==objects && f.world.sessionState().disabledEvents()==events &&
				f.world.sessionState().encounterMarked()==marked,"refused restore changed destination state");
			sameParty(p,f.p);sameActors(actors,f.world.sessionState().actors());
			if(kind==2) {
				// Simulate an inconsistent service caller clearing just party context.
				f.p.encounterContext.reset();
				rejects([&]{XeenSaveState::capture(signature,f.p,f.camera,flags,f.world);},"encounter");
				rejects([&]{f.world.restoreSessionState({}, {}, {});},"encounter");
				sameActors(actors,f.world.sessionState().actors());
			}
		}
		Fixture target;auto before=target.p;auto camera=target.camera;
		XeenSaveState::Resources bad{signature,[&]{auto p=party();p.encounterContext=target.context;return p;},
			[](XeenMapIdentity){return events();}};
		rejects([&]{XeenSaveState::restoreBeforeGameplay(snapshot,bad,target.p,target.camera,flags,target.world,
			[](XeenWorld &,const XeenPartyState &,const XeenCamera &,const XeenGameFlags &){});},"provider");
		sameParty(before,target.p);check(!target.p.encounterContext && !target.world.hasEncounterState() && save_test::sameCamera(camera,target.camera),"bad provider partially restored");
		bad.loadInitialParty=[] {return party();};
		XeenSaveState::restoreBeforeGameplay(snapshot,bad,target.p,target.camera,flags,target.world,
			[](XeenWorld &,const XeenPartyState &,const XeenCamera &,const XeenGameFlags &){});
		save_test::sameSnapshot(snapshot,XeenSaveState::capture(signature,target.p,target.camera,flags,target.world));
		// A preflight cannot smuggle a marked candidate into ordinary publication.
		rejects([&]{XeenSaveState::restoreBeforeGameplay(snapshot,bad,target.p,target.camera,flags,target.world,
			[](XeenWorld &w,const XeenPartyState &,const XeenCamera &,const XeenGameFlags &){w.markEncounterSession();});},"preparation");
		save_test::sameSnapshot(snapshot,XeenSaveState::capture(signature,target.p,target.camera,flags,target.world));
		std::cout<<"Encounter capture/restore preservation tests passed\n";return 0;
	}catch(const std::exception &e){std::cerr<<e.what()<<'\n';return 1;}
}
