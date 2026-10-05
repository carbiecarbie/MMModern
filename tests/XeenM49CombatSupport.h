#ifndef MMODERN_TESTS_XEEN_M49_COMBAT_SUPPORT_H
#define MMODERN_TESTS_XEEN_M49_COMBAT_SUPPORT_H
#include "games/xeen/XeenCombat.h"
#include "core/PlayerAction.h"
#include "games/xeen/XeenRegionalRules.h"
#include "games/xeen/XeenJourneyContent.h"
#include <queue>
namespace m49_combat {
// Ordinary book/confirmation input only. No mutable gameplay reference escapes.
inline std::optional<unsigned> knownSlot(const mmodern::XeenCharacter &c,unsigned spell) {
    const auto category=mmodern::XeenLearnedSpellRules::categoryForClass(c.characterClass);
    if(!category)return {};
    for(unsigned slot=0;slot<39;++slot)
        if(mmodern::XeenLearnedSpellRules::known(c,slot) &&
            mmodern::XeenLearnedSpellRules::spellForSlot(*category,slot)==spell)return slot;
    return {};
}
inline std::optional<mmodern::PlayerAction> finishAwaken(const mmodern::XeenCombatCastView &cast,
        const mmodern::XeenPartyState &party) {
    using namespace mmodern;
    if(cast.phase==XeenCombatCastPhase::Learned) {
        const auto slot=knownSlot(party.roster.at(cast.owner),1);
        if(!slot)throw std::runtime_error("Recovery caster lost learned Awaken");
        if(cast.slot<*slot)return NavigationAction::MoveBackward;
        if(cast.slot>*slot)return NavigationAction::MoveForward;
        return AcknowledgeAction{};
    }
    if(cast.phase==XeenCombatCastPhase::Confirm || cast.phase==XeenCombatCastPhase::Result)
        return AcknowledgeAction{};
    return {};
}
inline bool canAwaken(const mmodern::XeenPartyState &party,int participant) {
    if(participant<0 || participant>=6)return false;
    const auto &c=party.roster.at(mmodern::kXeenCombatOwners[participant]);
    if(c.currentSp<1 || !knownSlot(c,1))return false;
    for(auto owner:mmodern::kXeenCombatOwners)if(party.roster.at(owner).conditions[8])return true;
    return false;
}
inline std::optional<mmodern::PlayerAction> toward(mmodern::XeenWorld &world,
        const mmodern::XeenCamera &camera,int x,int y,mmodern::XeenDirection facing) {
    using namespace mmodern;
    if(camera.mapId!=XeenMapIdentity(23))throw std::runtime_error("Mainland route left its region");
    if(camera.x==x && camera.y==y) {
        if(camera.direction==facing)return {};
        return NavigationAction::TurnRight;
    }
    const auto &map=world.map(23);constexpr int dx[]{0,1,0,-1},dy[]{1,0,-1,0};
    std::array<int,256> first;first.fill(-1);std::queue<int> pending;
    const int start=camera.y*16+camera.x;first[start]=4;pending.push(start);
    while(!pending.empty() && first[y*16+x]<0) {
        const auto cell=pending.front();pending.pop();const int cx=cell%16,cy=cell/16;
        for(unsigned direction=0;direction<4;++direction) {
            const int nx=cx+dx[direction],ny=cy+dy[direction];
            if(nx<0 || nx>=16 || ny<0 || ny>=16 || first[ny*16+nx]>=0 ||
                XeenMovement::localOutdoor(map,cx,cy,nx,ny,xeenJourneyContent().traversal)!=XeenMovementResult::Moved)continue;
            first[ny*16+nx]=cell==start ? int(direction) : first[cell];pending.push(ny*16+nx);
        }
    }
    const auto direction=first[y*16+x];
    if(direction<0)throw std::runtime_error("No reachable mainland route to original combat/service target");
    if(unsigned(camera.direction)==unsigned(direction))return NavigationAction::MoveForward;
    return NavigationAction::TurnRight;
}
}
#endif
