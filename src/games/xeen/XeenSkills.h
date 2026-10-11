#ifndef MMODERN_GAMES_XEEN_SKILLS_H
#define MMODERN_GAMES_XEEN_SKILLS_H
#include "games/xeen/XeenParty.h"
namespace mmodern {
// Effects implemented by shared rules, or by the M58 exploration owner.
inline bool xeenSkillSupported(unsigned skill) noexcept {
 switch(skill) {case 0:case 2:case 3:case 4:case 10:case 11:case 12:case 13:return true;default:return false;}
}
inline bool xeenCharacterHasSkill(const XeenCharacter &character,unsigned skill) noexcept {
 return skill<character.skills.size() && character.skills[skill]!=0;
}
inline bool xeenPartyHasSkill(const XeenPartyState &party,unsigned skill) {
 if(skill>=18 || party.party.size()==0)return false;
 unsigned holders=0;
 // Party::checkSkill counts every active member, without a condition filter.
 for(auto owner:party.party.activeRosterIds())holders+=xeenCharacterHasSkill(party.roster.characters().at(owner),skill);
 const auto required=(skill==9 || skill==11)?2u:(skill==5 || skill==14)?party.party.size():1u;
 return holders>=required;
}
inline bool xeenGrantSkill(XeenCharacter &character,unsigned skill) {
 if(!xeenSkillSupported(skill))return false;
 // follows the original byte storage; not confirmed in DOS
 character.skills[skill]=static_cast<std::uint8_t>(character.skills[skill]+1);
 return true;
}
inline bool xeenRemoveSkill(XeenCharacter &character,unsigned skill) {
 if(!xeenSkillSupported(skill))return false;
 character.skills[skill]=0;return true;
}
}
#endif
