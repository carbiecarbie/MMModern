#include "games/xeen/XeenArenaRules.h"
#include <stdexcept>
namespace mmodern {
unsigned xeenArenaSpecies(unsigned level,unsigned roll) {
    if(!level || level>10 || !roll || roll>7)throw std::invalid_argument("Invalid Arena species roll");
    unsigned species=roll+(level-1)*7;
    if(species>67)species-=3;
    if(species==59)species=60;
    if(species==28)species=29;
    return species;
}
std::array<unsigned,5> xeenArenaPool(unsigned level,const XeenArenaDraw &draw) {
    std::array<unsigned,5> pool;
    for(auto &species:pool)species=xeenArenaSpecies(level,draw(1,7));
    return pool;
}
std::vector<XeenArenaMonster> xeenArenaMonsters(const std::array<unsigned,5> &pool,unsigned count,
    const std::vector<XeenMonsterRecord> &statistics,const XeenArenaDraw &draw) {
    if(!count || count>20)throw std::invalid_argument("Invalid Arena monster count");
    for(auto species:pool)if(species>=statistics.size())throw std::invalid_argument("Arena species resource is absent");
    std::vector<XeenArenaMonster> result;
    for(unsigned i=0;i<count;++i) {
        XeenArenaMonster monster;auto &a=monster.actor;auto &animation=monster.animation;
        a.id={85,i};a.x=int(draw(3,11));a.y=int(draw(2,10));
        if((a.x==5 || a.x==10) && (a.y==4 || a.y==8))a.y=5;
        a.original.tableIndex=draw(0,4);a.original.resourceId=pool[a.original.tableIndex];
        a.original.x=a.x;a.original.y=a.y;
        a.statistics=statistics[a.original.resourceId];a.hp=a.statistics->baseHp();
        animation.frame=draw(0,7);animation.effect1=animation.effect2=a.statistics->animationEffect();
        if(animation.effect2)animation.effect3=draw(0,7);
        a.activated=true;a.lifecycle=XeenActorLifecycle::Present;a.status=XeenActorStatus::Physical;
        result.push_back(std::move(monster));
    }
    return result;
}
unsigned xeenArenaSuffix(unsigned count) noexcept {return count%100/10==1?0:count%10;}
bool xeenArenaMaxed(const XeenPartyState &party) {
    for(auto owner:party.party.activeRosterIds())if(party.roster.at(owner).awards[9]>=99)return true;
    return false;
}
unsigned xeenArenaAward(XeenPartyState &party) {
    if(!party.party.size())throw std::invalid_argument("Arena requires an active party");
    for(auto owner:party.party.activeRosterIds()) {
        auto &award=party.roster.at(owner).awards[9];award=std::uint8_t(unsigned(award)+1);
    }
    return party.party.member(party.roster,0).awards[9];
}
std::optional<unsigned> xeenArenaNumber(const std::string &text,unsigned limit) {
    if(text.size()>2)return {};
    unsigned value=0;
    for(unsigned char c:text) {if(c<'0' || c>'9')return {};value=value*10+c-'0';}
    return value<=limit?std::optional<unsigned>(value):std::nullopt;
}
}
