#include "XeenTestInstallation.h"
#include "XeenTrainingTestSupport.h"
#include "games/xeen/CloudsMapComposer.h"
#include <chrono>
#include <iostream>
using namespace training_test;
using save_test::rejects;
namespace {

}
int main(int argc,char **argv) {
    try {
        check(argc==2,"usage: training-original <original-installation>");const auto installation=xeenTestInstallationDetector().detect(argv[1]);
        check(bool(installation),"original installation absent");Inputs in(*installation);auto base=in.base();
        const auto &successor=base;
        check(successor.journey->schema==9 && successor.journey->content==14 &&
            *successor.journey->random==XeenJourneyRandomState{1,1652828136u,901} &&
            successor.journey->treasure->gold==800 && successor.journey->treasure->gems==10 &&
            successor.journey->serviceEconomy->bank==XeenBankBalances{0,0},"fresh successor changed independently recorded seed-7 initialization");
        const auto original=XeenActorApproach::actorsFromResources(in.maps.loadObjects(in.assets,28),in.statistics);
        xeenValidateTrainingSource(in.chr);
        for(bool reset:{false,true}) {
            auto actors=original;auto source=base;
            if(reset) {
                actors.resize(52);
                for(unsigned n=46;n<52;++n){auto &a=actors[n];a.id={28,n};a.original={};a.original.x=a.original.y=0;a.x=a.y=0;
                    if(n>=50){a.original.resourceId=0;a.statistics=in.statistics[0];}}
                for(const auto &record:in.city.records) {if(record.x!=100 || record.y!=100 || record.opcode!=0x10)continue;check(record.parameters.size()==4,"reset Spawn operand structure");const auto &p=record.parameters;auto &a=actors[p[0]];
                    a.x=p[1];a.y=p[2];a.hp=a.statistics->baseHp();a.activated=false;a.lifecycle=XeenActorLifecycle::Present;}
                // CD maze0028.evt removes exit protection/self-disable at (15,0,South,4/5); reset creates no overlay.
            }
            source.map(28).monsters.emplace(XeenSaveMonsterTable{std::uint16_t(original.size()),{}});
            for(const auto &a:actors)source.map(28).monsters->slots.push_back({a.x,a.y,a.lifecycle,a.status,false,
                a.id.recordIndex>=original.size() && a.original.hasResource()?std::int16_t(0):std::int16_t(-1)});
            Fixture fixture(in,source);const unsigned large=reset?36:35,small=reset?35:34;
            for(unsigned actor=0;actor<actors.size();++actor)for(unsigned field=0;field<5;++field) {
                auto changed=actors;auto &a=changed[actor];
                switch(field){case 0:++a.id.recordIndex;break;case 1:++a.hp;break;case 2:a.status=static_cast<XeenActorStatus>(1);break;
                    case 3:a.lifecycle=XeenActorLifecycle::Defeated;break;case 4:a.x=-128;break;}
                rejects([&]{xeenValidateIndoorActors(fixture.w,28,changed);});
            }
            auto changed=actors;changed[small].hp=changed[small].statistics->baseHp()+1;
            rejects([&]{xeenValidateIndoorActors(fixture.w,28,changed);});
            auto atWall=actors[small];atWall.x=8;atWall.y=7;atWall.activated=true;
            check(xeenIndoorActorTerrain(fixture.w,atWall,9,7)==XeenMonsterTerrain::Blocked,"small Slime crossed wall 9");
            // All cells/facings have an independent geometry oracle in
            // XeenVertigoOriginal. Raster every resource-defined Event site
            // through all existing animation phases without a route list. The
            // reset pass changes only the six reset actors, so it rasters every
            // site and facing once (phase 0); the eight phases of the same
            // objects were already composed by the initial pass above.
            const auto rasterBegan=std::chrono::steady_clock::now();unsigned rasters=0;
            const unsigned phases=reset?1:8;
            for(int y=0;y<32;++y)for(int x=0;x<32;++x)
                if(std::any_of(in.city.records.begin(),in.city.records.end(),[&](const auto &r){return r.x==x && r.y==y;}))
                for(unsigned d=0;d<4;++d)for(unsigned phase=0;phase<phases;++phase) {
                    const auto image=CloudsMapComposer().compose(in.assets,fixture.w,fixture.p,{28,x,y,static_cast<XeenDirection>(d)},
                        {610},nullptr,phase);
                    check(image.isValid(),"Training route geometry/object raster invalid");++rasters;
                }
            std::cerr<<"Event-site raster: "<<rasters<<" compositions in "<<std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now()-rasterBegan).count()<<" ms\n";
            const auto encoded=XeenSaveFormat::encode(fixture.snapshot());
            check(encoded==XeenSaveFormat::encode(XeenSaveFormat::decode(encoded)),"9/14 city form codec changed bytes");
            for(unsigned length:{0u,1u,16u,static_cast<unsigned>(encoded.size()-1)})
                rejects([&]{XeenSaveFormat::decode(std::vector<std::uint8_t>(encoded.begin(),encoded.begin()+length));});
            std::cout<<"M41 "<<(reset?"reset":"initial")<<" complete actor/canonical/resource union passed\n";
        }
        auto rendered=frame();in.assets.drawTraining(rendered);check(rendered.isValid(),"original Training frames invalid");
        std::cout<<"M41 original route, resources, actor mutations and codec passed\n";return 0;
    }catch(const std::exception &e){std::cerr<<e.what()<<'\n';return 1;}
}
