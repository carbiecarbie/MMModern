#include "XeenTrainingTestSupport.h"
#include "games/xeen/CloudsMapComposer.h"
#include <iostream>
using namespace training_test;
using save_test::rejects;
namespace {

}
int main(int argc,char **argv) {
    try {
        check(argc==2,"usage: training-original <original-installation>");const auto installation=XeenInstallationDetector().detect(argv[1]);
        check(bool(installation),"original installation absent");Inputs in(*installation);auto base=in.base();
        const auto &successor=base;
        check(successor.journey->schema==9 && successor.journey->content==14 &&
            *successor.journey->random==XeenJourneyRandomState{1,1652828136u,901} &&
            successor.journey->treasure->gold==800 && successor.journey->treasure->gems==10 &&
            successor.journey->serviceEconomy->bank==XeenBankBalances{0,0},"fresh successor changed independently recorded seed-7 initialization");
        const auto original=XeenActorApproach::actorsFromResources(in.maps.loadObjects(in.assets,28),in.statistics);
        xeenValidateVertigoRoute(in.mainland,in.city);xeenValidateTrainingSource(in.chr);
        std::vector<unsigned> sites{0,3,538,539};
        for(unsigned n=760;n<=813;++n)sites.push_back(n);
        for(unsigned n=816;n<=846;++n)sites.push_back(n);
        for(unsigned site:sites)for(unsigned field=0;field<8;++field) {
            auto altered=in.city;auto &r=altered.records[site];
            switch(field){case 0:++r.fileOffset;break;case 1:++r.x;break;case 2:++r.y;break;case 3:r.direction^=1;break;
                case 4:++r.line;break;case 5:r.opcode^=1;break;case 6:++r.lengthField;break;case 7:r.parameters.push_back(1);break;}
            rejects([&]{xeenValidateVertigoRoute(in.mainland,altered);});
        }
        for(unsigned n=136;n<=139;++n){auto altered=in.mainland;altered.records[n].parameters.push_back(0);
            rejects([&]{xeenValidateVertigoRoute(altered,in.city);});}
        for(bool reset:{false,true}) {
            auto actors=original;auto source=base;
            if(reset) {
                actors.resize(52);
                for(unsigned n=46;n<52;++n){auto &a=actors[n];a.id={28,n};a.original={};a.original.x=a.original.y=0;a.x=a.y=0;
                    if(n>=50){a.original.resourceId=0;a.statistics=in.statistics[0];}}
                for(unsigned n=770;n<=812;++n){const auto &p=in.city.records[n].parameters;auto &a=actors[p[0]];
                    a.x=p[1];a.y=p[2];a.hp=a.statistics->baseHp();a.activated=false;a.lifecycle=XeenActorLifecycle::Present;}
                source.disabledEvents.push_back({28,764});
            }
            source.journey->vertigoActors.emplace();
            for(const auto &a:actors)source.journey->vertigoActors->push_back({a.id,a.x,a.y,a.hp,a.activated,a.lifecycle,a.status,false});
            Fixture fixture(in,source);const unsigned large=reset?36:35,small=reset?35:34;
            for(unsigned actor=0;actor<actors.size();++actor)for(unsigned field=0;field<5;++field) {
                auto changed=actors;auto &a=changed[actor];
                switch(field){case 0:++a.id.recordIndex;break;case 1:++a.hp;break;case 2:a.status=static_cast<XeenActorStatus>(1);break;
                    case 3:a.lifecycle=XeenActorLifecycle::Defeated;break;case 4:a.x=-128;break;}
                rejects([&]{xeenValidateVertigoActors(fixture.w,changed);});
            }
            // Required immutable union is checked even without a city camera.
            for(const char *resource:{"maze0028.dat","mazex109.dat","mazex110.dat","mazex111.dat","maze0028.mob","maze0028.evt","aaze0028.txt",
                "trng1.twn","train.icn","esc.icn","004.obj","006.obj","008.obj","009.obj","010.obj","011.obj","maze.chr"})
                rejects([&]{xeenValidateVertigoManifest(fixture.w,in.city,in.statistics,[&](const auto &name){auto b=in.reader()(name);
                    if(name==resource)b[b.size()/2]^=1;return b;});});
            for(unsigned type:{0u,2u,73u}){auto statistics=in.statistics;statistics[type].raw[20]^=1;
                rejects([&]{xeenValidateVertigoManifest(fixture.w,in.city,statistics,in.reader());});}
            auto changed=actors;changed[small].hp=1;rejects([&]{xeenValidateVertigoActors(fixture.w,changed);});
            auto atWall=actors[small];atWall.x=8;atWall.y=7;atWall.activated=true;
            check(xeenIndoorActorTerrain(fixture.w,atWall,9,7)==XeenMonsterTerrain::Blocked,"small Slime crossed wall 9");
            for(int y=0;y<32;++y)for(int x=0;x<32;++x)if(xeenJourneyContent().vertigoCell(x,y))
                for(unsigned d=0;d<4;++d)for(unsigned phase=0;phase<8;++phase) {
                    const auto image=CloudsMapComposer().compose(in.assets,fixture.w,fixture.p,{28,x,y,static_cast<XeenDirection>(d)},
                        {610},nullptr,phase);
                    check(image.isValid(),"Training route geometry/object raster invalid");
                }
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
