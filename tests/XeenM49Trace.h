#ifndef MMODERN_TESTS_XEEN_M49_TRACE_H
#define MMODERN_TESTS_XEEN_M49_TRACE_H
// Optional local evidence; never stores original resources in the repository.
#include "app/XeenEventFlow.h"
#include "games/xeen/XeenRegionalRules.h"
#include <cstdlib>
#include <fstream>
#include <sstream>
namespace m49_trace {
inline std::ostream *output() {
    static std::ofstream stream;
    static bool initialized=false;
    if(!initialized) {
        initialized=true;
        if(const auto path=std::getenv("MMODERN_M49_TRACE"))stream.open(path);
    }
    return stream.is_open() ? &stream : nullptr;
}
inline void draw(unsigned lo,unsigned hi,std::optional<unsigned> value,
        const mmodern::XeenJourneyRandomState &cursor) {
    if(auto out=output())*out<<"DRAW "<<cursor.count<<' '<<lo<<' '<<hi<<' '
        <<(value ? std::to_string(*value) : "reject")<<' '<<cursor.state<<'\n';
}
inline void frame(const mmodern::XeenWorld &world,const mmodern::XeenPartyState &party,
        const mmodern::XeenCamera &camera,const mmodern::XeenEventFlow &flow) {
    auto out=output();if(!out)return;
    using namespace mmodern;
    *out<<"FRAME "<<camera.mapId.number<<' '<<camera.x<<' '<<camera.y<<' '<<unsigned(camera.direction);
    if(party.encounterContext)*out<<" TIME "<<party.encounterContext->year<<' '<<party.encounterContext->day<<' '<<party.encounterContext->minutes;
    if(auto random=world.sessionState().journeyRandom())*out<<" RNG "<<random->count<<' '<<random->state;
    if(auto encounter=flow.encounter())if(auto combat=encounter->combat()) {
        if(auto cast=combat->cast())*out<<" CAST_STAGE "<<unsigned(cast->phase);
        const auto r=combat->result();
        *out<<" COMBAT "<<unsigned(combat->phase())<<' '<<combat->participant()<<' '<<unsigned(combat->participants())
            <<" RESULT "<<unsigned(r.operation)<<' '<<unsigned(r.status)<<' '<<r.revision<<' '<<r.generation
            <<' '<<r.monster.recordIndex<<' '<<unsigned(r.targetedMembers)<<' '<<r.actorHpBefore<<' '<<r.actorHpAfter;
        *out<<" COUNTDOWN "<<combat->movementCountdown()<<" STEPPED "<<combat->stepped()<<" BLOCK "<<unsigned(r.blockedMembers);
        for(unsigned i=0;i<r.injuryCount;++i)*out<<" INJURY "<<unsigned(r.injury(i).owner)<<' '<<r.injury(i).beforeHp<<' '<<r.injury(i).afterHp<<" ORDINAL "<<r.injury(i).attackOrdinal;
    }
    *out<<'\n';
    if(auto encounter=flow.encounter()) {
        const auto appearance=encounter->appearance();
        *out<<"PRESENTATION "<<unsigned(appearance.kind)<<' '<<unsigned(appearance.frame);
        if(appearance.identity)*out<<" ACTOR "<<appearance.identity->recordIndex;
        if(appearance.impactSnapshot)*out<<" RETAINED "<<appearance.impactSnapshot->id.recordIndex<<' '<<appearance.impactSnapshot->hp;
        for(const auto &p:appearance.projectiles)*out<<" LANE "<<p.enemy<<' '<<p.lane<<' '<<p.row;
        auto volley=encounter->combat() ? encounter->combat()->result().ranged : encounter->result().consequences;
        if(volley) {
            *out<<" VOLLEY_STAGE "<<unsigned(volley->stage);
            if(volley->impactSource)*out<<" SOURCE "<<volley->impactSource->recordIndex;
            if(volley->impactOwner)*out<<" OWNER "<<unsigned(*volley->impactOwner);
        }
        *out<<'\n';
    }
    if(auto encounter=flow.encounter())if(auto volley=encounter->result().consequences)
        for(unsigned source=0;source<volley->count;++source) {
            const auto &shot=volley->shots[source];
            *out<<"REGIONAL source "<<shot.source.recordIndex<<" ordinal "<<source
                <<" at "<<shot.x<<' '<<shot.y<<" distance "<<shot.distance<<" targets "<<unsigned(shot.attack.targetedMembers);
            for(unsigned i=0;i<shot.attack.injuryCount;++i) {
                const auto &injury=shot.attack.injury(i);
                *out<<" INJURY "<<unsigned(injury.owner)<<' '<<injury.beforeHp<<' '<<injury.afterHp<<" ORDINAL "<<injury.attackOrdinal;
            }
            *out<<'\n';
        }
    for(auto id:kXeenCombatOwners) {
        const auto &c=party.roster.at(id);
        *out<<"MEMBER "<<unsigned(id)<<' '<<c.currentHp<<' '<<c.currentSp;
        for(auto condition:c.conditions)*out<<' '<<unsigned(condition);
        if(auto inputs=party.roster.combatInputs(id))*out<<" XP "<<inputs->experience;
        for(const auto &a:c.armor)*out<<" ARMOR "<<unsigned(a.id)<<' '<<unsigned(a.frame)<<' '<<unsigned(a.state);
        *out<<'\n';
    }
    for(const auto &a:world.sessionState().regionalActors(camera.mapId))
        *out<<"ACTOR "<<a.id.recordIndex<<' '<<a.x<<' '<<a.y<<' '<<a.hp<<' '<<unsigned(a.lifecycle)<<' '<<a.activated<<'\n';
    if(auto treasure=party.monsterTreasure)*out<<"TREASURE "<<treasure->gold<<' '<<treasure->pendingGold<<' '<<treasure->pendingMask<<'\n';
}
}
#endif
