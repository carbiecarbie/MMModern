#include "XeenEncounterTestSupport.h"
#include "games/xeen/XeenLearnedSpellRules.h"
#include "games/xeen/XeenItemServiceRules.h"
#include <iostream>
using namespace encounter_test;
namespace {
using R=XeenLearnedSpellRules;using S=XeenLearnedSpell;
void handlers() {
    for(bool outdoor:{false,true})for(auto spell:{S::SuperShelter,S::Etherealize,S::TimeDistortion,S::Teleport}) {
        auto source=map();source.geometry.id=outdoor?23:28;source.geometry.flags2=outdoor?0x8000:0;
        source.geometry.runX=10;source.geometry.runY=12;
        for(auto &cell:source.geometry.cells) {cell.rawWord=outdoor?0x0001:0x1111;cell.surfaceIndex=1;
            cell.geometry=outdoor?decltype(cell.geometry){XeenOutdoorLayers{1,0,0,0}}:decltype(cell.geometry){XeenIndoorWalls{{1,1,1,1}}};}
        const XeenCamera camera{outdoor?23u:28u,9,11,XeenDirection::North};
        const unsigned flag=spell==S::SuperShelter?0x200:spell==S::Etherealize?0x40:spell==S::TimeDistortion?0x400:0x1000;
        {XeenWorld w([&](auto){return source;});const auto work=R::travel(w,camera,spell,2);
            check(!work.failed && !work.unsupported,"Travel ignores intervening walls");
            if(spell==S::SuperShelter)check(work.shelter && !work.destination,"Shelter delegates to Rest");
            else check(work.destination && work.destination->direction==camera.direction && work.destination->mapId==camera.mapId &&
                work.destination->x==(spell==S::TimeDistortion?10:9) && work.destination->y==(spell==S::TimeDistortion?12:spell==S::Etherealize?12:13),"DOS destination and facing");}
        source.geometry.flags=flag;
        {XeenWorld w([&](auto){return source;});const auto work=R::travel(w,camera,spell,2);check(work.failed && !work.destination && !work.shelter,"Own map restriction fails");}
        source.geometry.flags=0;
        auto &ahead=source.geometry.cells[12*16+9];ahead.rawWord=0x8888;
        {XeenWorld w([&](auto){return source;});check(R::travel(w,camera,S::Etherealize).failed,"Etherealize tests whole 0x8888 word");}
        ahead.rawWord=0x8887;
        {XeenWorld w([&](auto){return source;});check(!R::travel(w,camera,S::Etherealize).failed,"Etherealize accepts any other whole word");}
        {XeenWorld w([&](auto){return source;});const auto cancel=R::travel(w,camera,S::Teleport,0);check(!cancel.failed && !cancel.destination,"Teleport zero/empty cancellation");}
        auto &destination=source.geometry.cells[13*16+9];destination.rawWord=0x8888;destination.surfaceIndex=8;
        {XeenWorld w([&](auto){return source;});check(R::travel(w,camera,S::Teleport,2).failed,"Teleport indoor word / outdoor water guard");}
        destination.rawWord=1;destination.surfaceIndex=0;
        {XeenWorld w([&](auto){return source;});check(R::travel(w,camera,S::Teleport,2).failed==outdoor,"Teleport outdoor surface zero only");}
    }
    for(auto pair:{std::pair{48,S::SuperShelter},{58,S::Etherealize},{39,S::TimeDistortion},{42,S::Teleport}}) {
        check(xeenItemUseSpell(pair.first)==pair.second,"Exploration power dispatch");
        check(bool(xeenItemUseSpell(pair.first,true))==(pair.first==39),"DOS combat item admission");
        check(bool(R::supportedIn(unsigned(pair.second),true))==(pair.first==39),"Learned/Quick Fight combat admission");
    }
    XeenWorld seams([](auto id){auto m=map();m.geometry.id=id.number;m.geometry.flags2=0;
        m.geometry.neighbors.fill(0);if(id==XeenMapIdentity(28))m.geometry.neighbors[1]=109;
        m.geometry.runX=18;m.geometry.runY=4;
        for(auto &cell:m.geometry.cells){cell.rawWord=0x1111;cell.surfaceIndex=1;cell.geometry=XeenIndoorWalls{{1,1,1,1}};}
        return m;});
    const XeenCamera seam{28,14,4,XeenDirection::East};
    const auto teleported=R::travel(seams,seam,S::Teleport,4);
    check(teleported.destination && teleported.destination->mapId==XeenMapIdentity(28) && teleported.destination->x==18 &&
        seams.sampleCell(28,18,4)->mapId==XeenMapIdentity(109),"Teleport resolves resident neighbor and preserves logical map");
    const auto itemCombat=R::travel(seams,seam,*xeenItemUseSpell(39,true));
    check(itemCombat.destination && itemCombat.destination->x==18 && itemCombat.destination->y==4,"Combat item power 39 dispatches primary run X/Y");
    check(R::cost(66,7).sp==15 && R::cost(66,7).gems==5 && R::cost(21,7).sp==30 && R::cost(21,7).gems==10 &&
        R::cost(70,7).sp==8 && !R::cost(70,7).gems && R::cost(69,7).sp==10 && !R::cost(69,7).gems,"C1 exact fixed costs");
}
}
int main(){try{handlers();std::cout<<"M76 shelter and travel handler rules passed\n";return 0;}catch(const std::exception &e){std::cerr<<e.what()<<'\n';return 1;}}
