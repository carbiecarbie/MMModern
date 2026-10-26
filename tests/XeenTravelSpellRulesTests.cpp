#include "XeenEncounterTestSupport.h"
#include "games/xeen/XeenLearnedSpellRules.h"
#include "games/xeen/XeenItemServiceRules.h"
#include "games/xeen/XeenCombat.h"
#include "games/xeen/XeenEventTrigger.h"
#include <iostream>
using namespace encounter_test;
namespace {
using R=XeenLearnedSpellRules;using S=XeenLearnedSpell;
void mapTravel() {
    for(auto spell:{S::LloydsBeacon,S::TownPortal}) {
        const unsigned flag=spell==S::LloydsBeacon?0x800:0x100;
        const auto loader=[&](auto id){auto m=map();m.geometry.id=id.number;m.geometry.flags=id==XeenMapIdentity(23)?flag:0;
            m.geometry.runX=18;m.geometry.runY=4;
            m.geometry.flags2=0;m.geometry.neighbors.fill(0);if(id.number>=28 && id.number<=32)m.geometry.neighbors[1]=109;return m;};
        XeenWorld blocked(loader);XeenCharacter caster;const XeenCamera here{23,9,11,XeenDirection::West};
        const auto refused=R::travel(blocked,here,spell,1,&caster,false);
        check(refused.failed && !refused.beacon && !refused.destination && !caster.lloydMap,"Restriction precedes dialog/default write");
        XeenWorld w([&](auto id){if(id==XeenMapIdentity(99))throw std::runtime_error("Missing DAT");auto m=loader(id);m.geometry.flags=0;return m;});
        w.markEncounterSession(XeenEncounterEntry::Journey);
        const auto cancel=R::travel(w,here,spell,0,&caster,false);
        check(!cancel.failed && !cancel.destination && (spell!=S::LloydsBeacon || cancel.beacon==std::array<std::uint8_t,4>{28,18,4,0}),"Paid dialog cancel/default initialization");
        const auto cost=R::cost(unsigned(spell),17);
        check(cost.sp==(spell==S::LloydsBeacon?6u:30u) && cost.gems==(spell==S::LloydsBeacon?2u:5u),"Exact travel costs");
        check(!R::supportedIn(unsigned(spell),true),"Learned combat and Quick Fight forbid map travel");
        if(spell==S::LloydsBeacon) {
            const auto set=R::travel(w,here,spell,1,&caster,false);
            check(set.beacon==std::array<std::uint8_t,4>{23,9,11,0} && !set.destination,"Set records caster location only");
            caster.lloydMap=23;caster.lloydX=9;caster.lloydY=11;
            const auto same=R::travel(w,here,spell,2,&caster,false);
            check(same.destination && !same.reload && same.destination->direction==here.direction,"Same map Return never reloads or turns");
            const auto cross=R::travel(w,{28,18,4,XeenDirection::South},spell,2,&caster,false);
            check(cross.destination && cross.reload && cross.destination->direction==XeenDirection::South,"Cross map Return loads and keeps facing");
            for(unsigned side:{0u,1u}) {caster.lloydMap=side?23:99;caster.lloydSide=side;
                check(R::travel(w,here,spell,2,&caster,false).unsupported,"Absent or Darkside beacon must refuse before payment");}
        } else {
            const auto town=R::travel(w,here,spell,1,&caster,false);
            const auto same=R::travel(w,{28,18,4,XeenDirection::East},spell,1,&caster,false);
            check(town.reload && town.destination && town.destination->mapId==XeenMapIdentity(28) && town.destination->x==18 && town.destination->y==4 && same.reload,"Portal always loads Vertigo");
            for(unsigned choice=2;choice<=5;++choice) {
                const auto town=R::travel(w,here,spell,choice,&caster,false);
                check(!town.unsupported && town.reload && town.destination && town.destination->mapId==XeenMapIdentity(27+choice) &&
                    town.destination->x==18 && town.destination->y==4 && town.destination->direction==here.direction,"All five towns use their own run location and retain facing");
            }
        }
    }
    for(auto pair:{std::pair{28,S::LloydsBeacon},{55,S::TownPortal}}) {
        check(xeenItemUseSpell(pair.first)==pair.second,"Travel item exploration dispatch");
        check(bool(xeenItemUseSpell(pair.first,true))==(pair.second==S::TownPortal),"Combat item travel admission (Portal continues, Beacon refuses)");
    }
}
void handlers() {
    for(bool outdoor:{false,true})for(auto spell:{S::SuperShelter,S::Etherealize,S::TimeDistortion,S::Teleport}) {
        auto source=map();source.geometry.id=outdoor?23:28;source.geometry.flags2=outdoor?0x8000:0;
        source.geometry.runX=10;source.geometry.runY=12;
        for(auto &cell:source.geometry.cells) {cell.rawWord=outdoor?0x0001:0x1111;cell.surfaceIndex=1;
            cell.geometry=outdoor?decltype(cell.geometry){XeenOutdoorLayers{1,0,0,0}}:decltype(cell.geometry){XeenIndoorWalls{{1,1,1,1}}};}
        const XeenCamera camera{outdoor?23u:28u,9,11,XeenDirection::North};
        const unsigned flag=spell==S::SuperShelter?0x200:spell==S::Etherealize?0x40:spell==S::TimeDistortion?0x400:0x1000;
        {XeenWorld w([&](auto){return source;});const auto work=R::travel(w,camera,spell,2,nullptr,false);
            check(!work.failed && !work.unsupported,"Travel ignores intervening walls");
            if(spell==S::SuperShelter)check(work.shelter && !work.destination,"Shelter delegates to Rest");
            else check(work.destination && work.destination->direction==camera.direction && work.destination->mapId==camera.mapId &&
                work.destination->x==(spell==S::TimeDistortion?10:9) && work.destination->y==(spell==S::TimeDistortion?12:spell==S::Etherealize?12:13),"DOS destination and facing");}
        source.geometry.flags=flag;
        {XeenWorld w([&](auto){return source;});const auto work=R::travel(w,camera,spell,2,nullptr,false);check(work.failed && !work.destination && !work.shelter,"Own map restriction fails");}
        source.geometry.flags=0;
        auto &ahead=source.geometry.cells[12*16+9];ahead.rawWord=0x8888;
        {XeenWorld w([&](auto){return source;});check(R::travel(w,camera,S::Etherealize,0,nullptr,false).failed,"Etherealize tests whole 0x8888 word");}
        ahead.rawWord=0x8887;
        {XeenWorld w([&](auto){return source;});check(!R::travel(w,camera,S::Etherealize,0,nullptr,false).failed,"Etherealize accepts any other whole word");}
        {XeenWorld w([&](auto){return source;});const auto cancel=R::travel(w,camera,S::Teleport,0,nullptr,false);check(!cancel.failed && !cancel.destination,"Teleport zero/empty cancellation");}
        auto &destination=source.geometry.cells[13*16+9];destination.rawWord=0x8888;destination.surfaceIndex=8;
        {XeenWorld w([&](auto){return source;});check(R::travel(w,camera,S::Teleport,2,nullptr,false).failed,"Teleport indoor word / outdoor water guard");}
        destination.rawWord=1;destination.surfaceIndex=0;
        {XeenWorld w([&](auto){return source;});check(R::travel(w,camera,S::Teleport,2,nullptr,false).failed==outdoor,"Teleport outdoor surface zero only");}
    }
    for(auto pair:{std::pair{48,S::SuperShelter},{58,S::Etherealize},{39,S::TimeDistortion},{42,S::Teleport}}) {
        check(xeenItemUseSpell(pair.first)==pair.second,"Exploration power dispatch");
        check(bool(xeenItemUseSpell(pair.first,true))==(pair.first==39),"DOS combat item admission");
        check(bool(R::supportedIn(unsigned(pair.second),true))==(pair.first==39),"Learned/Quick Fight combat admission");
    }
    XeenWorld seams([](auto id){auto m=map();m.geometry.id=id.number;m.geometry.flags2=0;
        m.geometry.neighbors.fill(0);if(id.number>=28 && id.number<=32)m.geometry.neighbors[1]=109;
        m.geometry.runX=18;m.geometry.runY=4;
        for(auto &cell:m.geometry.cells){cell.rawWord=0x1111;cell.surfaceIndex=1;cell.geometry=XeenIndoorWalls{{1,1,1,1}};}
        return m;});
    const XeenCamera seam{28,14,4,XeenDirection::East};
    const auto teleported=R::travel(seams,seam,S::Teleport,4,nullptr,false);
    check(teleported.destination && teleported.destination->mapId==XeenMapIdentity(28) && teleported.destination->x==18 &&
        seams.sampleCell(28,18,4)->mapId==XeenMapIdentity(109),"Teleport resolves resident neighbor and preserves logical map");
    const auto itemCombat=R::travel(seams,seam,*xeenItemUseSpell(39,true),0,nullptr,false);
    check(itemCombat.destination && itemCombat.destination->x==18 && itemCombat.destination->y==4,"Combat item power 39 dispatches primary run X/Y");
    check(R::cost(66,7).sp==15 && R::cost(66,7).gems==5 && R::cost(21,7).sp==30 && R::cost(21,7).gems==10 &&
        R::cost(70,7).sp==8 && !R::cost(70,7).gems && R::cost(69,7).sp==10 && !R::cost(69,7).gems,"C1 exact fixed costs");
}
void runDestinations() {
    // Combat Run lands on the current map's own run position, indoors or out.
    for(bool outdoor:{false,true}) {
        auto source=map();source.geometry.id=outdoor?23:28;source.geometry.flags2=outdoor?0x8000:0;
        source.geometry.runX=10;source.geometry.runY=12;source.geometry.neighbors.fill(0);
        const XeenCamera camera{outdoor?23u:28u,3,4,XeenDirection::South};
        XeenEventFile events;events.mapId=camera.mapId;events.resourcePresent=true;
        {XeenWorld w([&](auto){return source;});const auto to=xeenRunDestination(w,events,camera);
            check(to && to->mapId==camera.mapId && to->x==10 && to->y==12 && to->direction==camera.direction,"Run uses the map's run X/Y and keeps facing");
            const auto absent=xeenRunDestination(w,XeenEventFile{},camera);check(absent && absent->x==10 && absent->y==12,"An absent Event file has no destination Event");}
        XeenEventRecord record;record.x=10;record.y=12;record.direction=4;events.records.push_back(record);
        {XeenWorld w([&](auto){return source;});check(bool(xeenRunDestination(w,events,camera)),"A non-automatic destination Event does not block Run");}
        source.geometry.cells[12*16+10].rawAttributes=kXeenAutomaticEventFlag;
        {XeenWorld w([&](auto){return source;});// M85 Part 0: post-combat callers test the landing; its Event does not refuse Run.
         check(bool(xeenRunDestination(w,events,camera)),"Run admits an automatic destination Event");}
        events.records.back().direction=unsigned(XeenDirection::North);
        {XeenWorld w([&](auto){return source;});check(bool(xeenRunDestination(w,events,camera)),"An automatic Event for another facing does not block Run");}
        events.records.clear();
        {XeenWorld w([&](auto){return source;});check(bool(xeenRunDestination(w,events,camera)),"An automatic flag without an Event does not block Run");}
        source.geometry.cells[12*16+10].rawAttributes=0;source.geometry.runX=outdoor?16:32;
        {XeenWorld w([&](auto){return source;});check(!xeenRunDestination(w,events,camera),"A run position outside the map refuses Run");}
    }
}
}
int main(){try{mapTravel();handlers();runDestinations();std::cout<<"M76 shelter and travel handler rules passed\n";return 0;}catch(const std::exception &e){std::cerr<<e.what()<<'\n';return 1;}}
