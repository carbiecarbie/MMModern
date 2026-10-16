#include "XeenTestInstallation.h"
#include "XeenTrainingTestSupport.h"
#include <iostream>
using namespace training_test;
using save_test::rejects;
int main(int argc,char **argv) {
    try {
        check(argc==2,"usage: temple-original <original-installation>");
        const auto installation=xeenTestInstallationDetector().detect(argv[1]);
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
			auto &slime=slimeSave.map(28).monsters->slots.at(35);
			slime.x=15;slime.y=21;
			slime.lifecycle=XeenActorLifecycle::Present;slime.accounted=false;
			slimeSave.journey->content=14;
			Fixture admitted(in,slimeSave);
			check(admitted.snapshot().map(28).monsters->slots.at(35).y==21,
				"M43 Temple-closure Slime save was not restored");
			slimeSave.journey->content=13;
			rejects([&]{Fixture legacy(in,slimeSave);});
		}
        auto base=in.service();base.journey->content=14;base.camera={28,15,28,XeenDirection::North};
        const auto original=XeenActorApproach::actorsFromResources(in.maps.loadObjects(in.assets,28),in.statistics);
        for(bool reset:{false,true}) {
            auto actors=original;auto source=base;
            if(reset) {
                // Reset Spawns of slots 50 and 51 are past the MOB count: inert (DOS 0x9c59).
                for(const auto &record:in.city.records) {if(record.x!=100 || record.y!=100 || record.opcode!=0x10)continue;check(record.parameters.size()==4,"reset Spawn operand structure");const auto &p=record.parameters;if(p[0]>=actors.size())continue;auto &a=actors[p[0]];
                    a.x=p[1];a.y=p[2];a.hp=a.statistics->baseHp();a.activated=false;a.lifecycle=XeenActorLifecycle::Present;}
                // CD maze0028.evt removes exit protection/self-disable at (15,0,South,4/5); reset creates no overlay.
            }
            source.map(28).monsters.emplace(XeenSaveMonsterTable{std::uint16_t(original.size()),{}});
            for(const auto &a:actors)source.map(28).monsters->slots.push_back({a.x,a.y,a.lifecycle,a.status,false,-1});
            Fixture fixture(in,source);
            xeenValidateIndoorActors(fixture.w,28,actors);
            for(unsigned selected=0;selected<original.size();++selected) {
                auto changed=actors;changed[selected].hp=changed[selected].statistics->baseHp()+1;
                rejects([&]{xeenValidateIndoorActors(fixture.w,28,changed);});
                changed=actors;changed[selected].statistics->raw[20]^=1;
                rejects([&]{xeenValidateIndoorActors(fixture.w,28,changed);});
            }
        }
        std::cout<<"Original Temple state, city resource bindings and structural actor checks passed\n";
        return 0;
    } catch(const std::exception &error){std::cerr<<error.what()<<'\n';return 1;}
}
