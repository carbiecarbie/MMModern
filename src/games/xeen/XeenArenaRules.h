#ifndef MMODERN_XEEN_ARENA_RULES_H
#define MMODERN_XEEN_ARENA_RULES_H
#include "games/xeen/XeenParty.h"
#include "games/xeen/XeenScenePresentation.h"
#include <functional>

namespace mmodern {
// Invocation locals only; neither prompts nor the generated pool are saved.
enum class XeenArenaPhase { Offer, Decline, Level, Count, Start, Pool, Unfinished, Victory, Maxed };
struct XeenArenaPrompt {
    XeenArenaPhase phase=XeenArenaPhase::Offer;
    unsigned level=0,count=0,wins=0;
    std::array<unsigned,5> pool{};
    bool operator==(const XeenArenaPrompt &o) const {
        return phase==o.phase && level==o.level && count==o.count && wins==o.wins && pool==o.pool;
    }
    bool operator!=(const XeenArenaPrompt &o) const {return !(*this==o);}
};
using XeenArenaDraw=std::function<unsigned(unsigned,unsigned)>;
unsigned xeenArenaSpecies(unsigned level,unsigned roll);
std::array<unsigned,5> xeenArenaPool(unsigned level,const XeenArenaDraw &);
struct XeenArenaMonster { XeenActor actor; XeenMonsterAnimation animation; };
std::vector<XeenArenaMonster> xeenArenaMonsters(const std::array<unsigned,5> &,unsigned count,
    const std::vector<XeenMonsterRecord> &,const XeenArenaDraw &);
unsigned xeenArenaSuffix(unsigned count) noexcept;
bool xeenArenaMaxed(const XeenPartyState &);
unsigned xeenArenaAward(XeenPartyState &);
// Invalid (>limit) inputs reprompt; empty/zero cancels. Nondecimal input is refused.
std::optional<unsigned> xeenArenaNumber(const std::string &,unsigned limit);
}
#endif
