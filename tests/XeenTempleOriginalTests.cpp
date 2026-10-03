#include "XeenTrainingTestSupport.h"
#include <iostream>
using namespace training_test;
using save_test::rejects;
namespace {
std::bitset<2048> independentClosure(XeenWorld &world,std::vector<XeenActor> actors,
        unsigned selected,bool occupancy) {
    if(!occupancy)for(unsigned n=0;n<actors.size();++n)if(n!=selected) {
        actors[n].lifecycle=XeenActorLifecycle::Defeated;actors[n].activated=false;
    }
    std::bitset<2048> reached;std::vector<unsigned> queue;
    const auto add=[&](int x,int y,bool active) {
        check(x>=0 && x<32 && y>=0 && y<32,"Temple actor left city geometry");
        const unsigned state=y*32+x+(active?1024:0);
        if(!reached.test(state)){reached.set(state);queue.push_back(state);}
    };
    add(actors[selected].x,actors[selected].y,false);
    for(unsigned cursor=0;cursor<queue.size();++cursor) {
        const unsigned state=queue[cursor];
        actors[selected].x=state%32;actors[selected].y=state%1024/32;
        actors[selected].activated=state>=1024;
        for(int y=0;y<32;++y)for(int x=0;x<32;++x)if(xeenJourneyContent().vertigoCell(x,y))
            for(unsigned d=0;d<4;++d) {
                const XeenCamera camera{28,x,y,static_cast<XeenDirection>(d)};
                const auto view=XeenIndoorScene().classifyActors(world,camera,actors);
                const unsigned large=actors.size()==52?36:35,small=actors.size()==52?35:34;
                for(unsigned n=0;n<actors.size();++n)if(n!=large && n!=small)
                    check(!view.activation[n],"another original city actor influenced Temple route");
                if(view.activation[selected])add(actors[selected].x,actors[selected].y,true);
                if(actors[selected].activated || view.activation[selected]) {
                    auto moving=actors;moving[selected].activated=true;
                    const auto next=XeenActorApproach::move(moving,camera,[&](const auto &a,int mx,int my) {
                        return xeenIndoorActorTerrain(world,a,mx,my);
                    });
                    add(next[selected].x,next[selected].y,true);
                }
            }
    }
    return reached;
}
}
int main(int argc,char **argv) {
    try {
        check(argc==2,"usage: temple-original <original-installation>");
        const auto installation=XeenInstallationDetector().detect(argv[1]);
        check(bool(installation),"original installation unavailable");Inputs in(*installation);
        const auto &policy=xeenJourneyContent();
        unsigned cells=0;
        for(int y=0;y<32;++y)for(int x=0;x<32;++x)if(policy.vertigoCell(x,y)) {
            ++cells;check(y<=28 && (y<=11 || x==15),"M43 route has an extra cell");
        }
        check(cells==49 && policy.vertigoCell(15,8) &&
            policy.vertigoCell(15,28),"exact 49-cell route differs");
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
        xeenValidateVertigoRoute(in.mainland,in.city);
        for(unsigned index:{6u,543u})for(unsigned field=0;field<8;++field) {
            auto changed=in.city;auto &r=changed.records[index];
            switch(field){case 0:++r.fileOffset;break;case 1:++r.x;break;case 2:++r.y;break;
                case 3:r.direction^=1;break;case 4:++r.line;break;case 5:r.opcode^=1;break;
                case 6:++r.lengthField;break;case 7:r.parameters.push_back(1);break;}
            rejects([&]{xeenValidateVertigoRoute(in.mainland,changed);});
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
            for(const auto &a:actors)source.journey->vertigoActors->push_back({a.id,a.x,a.y,a.hp,a.activated,a.lifecycle,a.status,false});
            Fixture fixture(in,source);const unsigned large=reset?36:35,small=reset?35:34;
            const auto big=independentClosure(fixture.w,actors,large,true);
            const auto little=independentClosure(fixture.w,actors,small,true);
            check(big.count()==103 && little.count()==4 && (big&little).none(),
                "M43 original actor closure counts differ");
            check(big==independentClosure(fixture.w,actors,large,false) &&
                little==independentClosure(fixture.w,actors,small,false),
                "city occupancy changed actor closure");
            for(unsigned key=0;key<2048;++key)for(unsigned selected:{large,small}) {
                auto changed=actors;auto &a=changed[selected];
                a.x=key%32;a.y=key%1024/32;a.activated=key>=1024;
                if((selected==large?big:little).test(key))xeenValidateVertigoActors(fixture.w,changed);
                else rejects([&]{xeenValidateVertigoActors(fixture.w,changed);});
            }
            for(const char *name:{"tmpl1.twn","002.obj","012.obj"})
                rejects([&]{xeenValidateVertigoManifest(fixture.w,in.city,in.statistics,[&](const auto &resource) {
                    auto bytes=in.reader()(resource);if(resource==name)bytes[0]^=1;return bytes;});});
        }
        std::cout<<"Original Temple route, Event identities, resource manifest and independent actor closures passed\n";
        return 0;
    } catch(const std::exception &error){std::cerr<<error.what()<<'\n';return 1;}
}
