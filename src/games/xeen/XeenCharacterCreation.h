#ifndef MMODERN_XEEN_CHARACTER_CREATION_H
#define MMODERN_XEEN_CHARACTER_CREATION_H
#include "games/xeen/XeenParty.h"
#include "games/xeen/XeenCombat.h"
namespace mmodern {
using XeenCreateAttributes=std::array<int,7>;
using XeenCreateClasses=std::array<bool,10>;
// A display length of npos runs through the original string's terminator.
struct XeenCreateSkill {int id;std::size_t offset,length;};
inline constexpr std::array<XeenCreateSkill,10> kXeenCreateClassSkills{{
    {1,0,11},{5,0,8},{-1,0,0},{-1,0,0},{4,0,12},
    {0,0,8},{0,0,8},{-1,0,0},{6,0,9},{11,0,11}
}};
inline constexpr std::array<XeenCreateSkill,5> kXeenCreateRaceSkills{{
    {14,0,std::string::npos},{-1,0,0},{17,0,std::string::npos},
    {16,5,std::string::npos},{-1,0,0}
}};
XeenCreateClasses xeenCreateClasses(const XeenCreateAttributes &);
std::optional<unsigned> xeenCreateArrow(const XeenCreateClasses &);
std::optional<unsigned> xeenMoveCreateArrow(const XeenCreateClasses &,std::optional<unsigned>,int direction);
std::size_t xeenMoveCreateSlot(std::size_t index,std::size_t count,int direction);
std::vector<std::uint8_t> xeenCreateSlots(const XeenRoster &);
inline XeenRace xeenCreateRace(unsigned id) {return XeenRace((id/4)%5);}
inline XeenSex xeenCreateSex(unsigned id) {return XeenSex(id&1);}
XeenCreateAttributes xeenCreateRoll(XeenCombatRandom &);
// Both halves start as a completely cleared CHR, with every optional present.
std::pair<XeenCharacter,XeenCombatInputs> xeenCreatedCharacter(std::uint8_t id,
    const std::string &name,unsigned characterClass,const XeenCreateAttributes &,
    unsigned town,const XeenGameplayContext &);
struct XeenCreateDie {unsigned frame;int x,y,dx,dy;};
using XeenCreateDice=std::array<XeenCreateDie,3>;
inline XeenCreateDice xeenInitialCreateDice() {return {{{0,20,17,10,-10},{2,112,35,-10,-10},{4,61,50,-10,10}}};}
void xeenAdvanceCreateDice(XeenCreateDice &);
}
#endif
