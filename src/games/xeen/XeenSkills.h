#ifndef MMODERN_GAMES_XEEN_SKILLS_H
#define MMODERN_GAMES_XEEN_SKILLS_H
#include "games/xeen/XeenParty.h"
namespace mmodern {
// A skill is one stored byte. Granting or removing one only writes that byte;
// effects are read where the rules use them. Arms Master (1), Direction Sense (6),
// Merchant (8), Mountaineer (9), Swimmer (14), Tracker (15), Spot Secret Doors (16)
// and Danger Sense (17) have no implemented effect yet (docs/project-status.md).
// Crusader (5) and Linguist (7) are only read by Event tests: the DOS
// checkSkill callers (0xc983..0x1bdba) test skills 4, 6, 9, 10, 11, 14, 16, 17.
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
 if(skill>=character.skills.size())return false;
 // A byte that wraps (DOS increment at 0x2272b..0x22731).
 character.skills[skill]=static_cast<std::uint8_t>(character.skills[skill]+1);
 return true;
}
inline bool xeenRemoveSkill(XeenCharacter &character,unsigned skill) {
 if(skill>=character.skills.size())return false;
 character.skills[skill]=0;return true;
}
}
#endif
