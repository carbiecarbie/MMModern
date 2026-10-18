#ifndef MMODERN_TESTS_XEEN_M49_COMBAT_SUPPORT_H
#define MMODERN_TESTS_XEEN_M49_COMBAT_SUPPORT_H
#include "games/xeen/XeenCombat.h"
#include "core/PlayerAction.h"
#include "core/DialogInput.h"
#include "games/xeen/XeenRegionalRules.h"
#include "games/xeen/XeenJourneyContent.h"
#include <queue>
#include <algorithm>
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
inline std::optional<mmodern::PlayerAction> finishSpell(const mmodern::XeenCombatCastView &cast,
        const mmodern::XeenPartyState &party,unsigned spell) {
    using namespace mmodern;
    const auto slot=knownSlot(party.roster.at(cast.owner),spell);
    if(!slot)throw std::runtime_error("Witness caster lost learned spell");
    if(cast.phase==XeenCombatCastPhase::Learned) {
        const auto rows=XeenLearnedSpellRules::rows(party.roster.at(cast.owner));
        const auto it=std::find(rows.begin(),rows.end(),*slot);
        if(it==rows.end())throw std::runtime_error("Witness learned row absent");
        const auto row=std::size_t(it-rows.begin());
        if(row<cast.top)return DialogKeyAction{InputKey::Up};
        if(row>=cast.top+10)return DialogKeyAction{InputKey::Down};
        if(cast.selection!=row)return DialogKeyAction{unsigned(row-cast.top==9?'0':'1'+row-cast.top)};
        return DialogKeyAction{'s'};
    }
    if(cast.phase==XeenCombatCastPhase::Confirm)return DialogKeyAction{unsigned(cast.slot==*slot?'c':'n')};
    if(cast.phase==XeenCombatCastPhase::Result && cast.result.failed)return AcknowledgeAction{};
    return {};
}
inline std::optional<mmodern::PlayerAction> finishAwaken(const mmodern::XeenCombatCastView &cast,
        const mmodern::XeenPartyState &party) {return finishSpell(cast,party,1);}
inline bool canAwaken(const mmodern::XeenPartyState &party,int participant) {
    if(participant<0 || participant>=6)return false;
    const auto &c=party.roster.at(mmodern::kXeenCombatOwners[participant]);
    if(c.currentSp<1 || !knownSlot(c,1))return false;
    for(auto owner:mmodern::kXeenCombatOwners)if(party.roster.at(owner).conditions[8])return true;
    return false;
}
// M75 DOS turn order: a woken member may wait until next round. Once combat
// has earned an armor injury, keep every available fighter attacking.
inline mmodern::PlayerAction servicesCombatAction(const mmodern::XeenPartyState &party,
        int participant,bool earnArmor,unsigned &openingAttacks,bool unarmedSmith=false) {
    using namespace mmodern;
    // Smith's route retains its unarmed fighter and First Aid until the injury.
    if(unarmedSmith && earnArmor && party.roster.at(18).currentHp<1 && participant==4)
        return CastSpellAction{};
    if(canAwaken(party,participant))return CastSpellAction{};
    if(unarmedSmith && earnArmor && participant==1)return AttackAction{};
    bool broken=false;
    for(auto owner:kXeenCombatOwners)for(const auto &armor:party.roster.at(owner).armor)
        broken=broken || (armor.id && (armor.state&128));
    if(earnArmor && !broken && openingAttacks++>=1)return BlockAction{};
    return AttackAction{};
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
