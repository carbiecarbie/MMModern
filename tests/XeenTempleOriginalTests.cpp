#include "XeenTrainingTestSupport.h"
#include <iostream>
using namespace training_test;
using save_test::rejects;
int main(int argc,char **argv) {
    try {
        check(argc==2,"usage: temple-original <original-installation>");
        const auto installation=XeenInstallationDetector().detect(argv[1]);
        check(bool(installation),"original installation unavailable");Inputs in(*installation);
        for(unsigned legacy=8;legacy<=13;++legacy) {
            auto rejected=in.base();rejected.journey->schema=save_test::legacyJourneySchema(legacy);rejected.journey->content=legacy;
            rejects([&]{Fixture old(in,rejected);},"no longer supported");
        }
		auto selectionSave=in.service();
		selectionSave.journey->content=14;
		Fixture templeSelection(in,selectionSave);
		templeSelection.w.selectObject({28,16,4,XeenDirection::North});
		XeenWorld ordinary(in.mapLoader(),in.objectLoader());
		rejects([&]{ordinary.selectObject({28,16,4,XeenDirection::North});},
			"invalid physical object-selection cell");
		{
			auto slimeSave=in.service();
			slimeSave.camera={28,15,4,XeenDirection::North};
			auto &slime=slimeSave.journey->vertigoActors->at(35);
			slime.x=15;slime.y=21;slime.hp=2;slime.activated=true;
			slime.lifecycle=XeenActorLifecycle::Present;slime.accounted=false;
			slimeSave.journey->content=14;
			Fixture admitted(in,slimeSave);
			check(admitted.snapshot().journey->vertigoActors->at(35).y==21,
				"M43 Temple-closure Slime save was not restored");
			slimeSave.journey->content=13;
			rejects([&]{Fixture legacy(in,slimeSave);});
		}
        auto base=in.service();base.journey->content=14;base.camera={28,15,28,XeenDirection::North};
        const auto original=XeenActorApproach::actorsFromResources(in.maps.loadObjects(in.assets,28),in.statistics);
        for(bool reset:{false,true}) {
            auto actors=original;auto source=base;
            if(reset) {
                actors.resize(52);
                for(unsigned n=46;n<52;++n) {auto &a=actors[n];a.id={28,n};a.original={};a.original.x=a.original.y=0;a.x=a.y=0;
                    if(n>=50){a.original.resourceId=0;a.statistics=in.statistics[0];}}
                for(unsigned n=770;n<=812;++n) {const auto &p=in.city.records[n].parameters;auto &a=actors[p[0]];
                    a.x=p[1];a.y=p[2];a.hp=a.statistics->baseHp();a.activated=false;a.lifecycle=XeenActorLifecycle::Present;}
                source.disabledEvents.push_back({28,764});
            }
            source.journey->vertigoActors.emplace();
            for(const auto &a:actors)source.journey->vertigoActors->push_back({a.id,a.x,a.y,a.hp,a.activated,a.lifecycle,a.status,false,
                a.id.recordIndex>=original.size() && a.original.hasResource()?std::int16_t(0):std::int16_t(-1)});
            Fixture fixture(in,source);
            xeenValidateVertigoActors(fixture.w,actors);
            for(unsigned selected=0;selected<original.size();++selected) {
                auto changed=actors;changed[selected].hp=changed[selected].statistics->baseHp()+1;
                rejects([&]{xeenValidateVertigoActors(fixture.w,changed);});
                changed=actors;changed[selected].statistics->raw[20]^=1;
                rejects([&]{xeenValidateVertigoActors(fixture.w,changed);});
            }
        }
        std::cout<<"Original Temple state, city resource bindings and structural actor checks passed\n";
        return 0;
    } catch(const std::exception &error){std::cerr<<error.what()<<'\n';return 1;}
}
