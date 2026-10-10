#ifndef MMODERN_XEEN_INITIAL_RESOURCE_ORACLE_H
#define MMODERN_XEEN_INITIAL_RESOURCE_ORACLE_H
#include "games/xeen/XeenPartyLoader.h"
#include "formats/xeen/XeenCharacterFormat.h"
#include "games/xeen/XeenActorApproach.h"
#include <algorithm>
#include <stdexcept>
namespace initial_oracle {
using namespace mmodern;
inline void check(bool yes,const char *message) {if(!yes)throw std::runtime_error(message);}
// Pinned Party::synchronize: every present byte, including the partial quest
// inventory. Its absent twenty items, Darkside stock and character flags are zero.
// Prior maze (28) and the zero completion/death/win/loss/totalTime, world/
// Darkside/character flags and unused quest tail are not mutable live owners.
// Map seen/stepped bits also have no live automap owner. Supported Part A
// mechanics do not mutate them; adding such mechanics requires persistence.
inline void pty(const std::vector<std::uint8_t> &bytes) {
 std::vector<std::uint8_t> expected(812);
 const std::uint8_t prefix[]{6,6,0,18,14,11,1,6,0,0,3,18,4,28,1,1,0,28};
 std::copy(std::begin(prefix),std::end(prefix),expected.begin());
 expected[612]=1;expected[614]=98;expected[615]=2;
 expected[616]=224;expected[617]=1;expected[618]=90;
 expected[638]=32;expected[639]=3;expected[642]=10;
 check(bytes==expected,"Initial PTY differs from complete 812-byte oracle");
}
inline void roster(const XeenPartyState &party,const std::vector<std::uint8_t> &bytes,bool supplements) {
 check(bytes.size()==30*354,"Initial CHR requires thirty records");
 check(party.party.activeRosterIds()==std::vector<std::uint8_t>({0,18,14,11,1,6}),"Original active order");
 for(unsigned owner=0;owner<30;++owner) {
  const auto *r=bytes.data()+354*owner;const auto &c=party.roster.at(owner);
  unsigned length=0;while(length<16 && r[length])++length;
  check(c.name==std::string(reinterpret_cast<const char *>(r),length) && unsigned(c.sex)==r[16] &&
   unsigned(c.race)==r[17] && unsigned(c.characterClass)==r[19],"Original identity");
  check(c.maxStatSkills.astrologer==bool(r[41]) && c.maxStatSkills.bodybuilder==bool(r[42]) &&
   c.maxStatSkills.prayerMaster==bool(r[51]) && c.maxStatSkills.prestidigitation==bool(r[52]),"Original skill-derived maxima");
  check(c.intellect.permanent==r[22] && c.intellect.temporary==r[23] &&
   c.personality.permanent==r[24] && c.personality.temporary==r[25] &&
   c.endurance.permanent==r[26] && c.endurance.temporary==r[27] && c.hasSpells==bool(r[163]),"Original modeled attributes");
  check(c.rosterId==owner && c.permanentLevel==r[35] && c.temporaryLevel==r[36] &&
   c.temporaryAge==r[38] && c.currentHp==int(r[342]|r[343]<<8) &&
   c.currentSp==int(r[344]|r[345]<<8) && c.birthYear==int(r[346]|r[347]<<8),"Original CHR stats");
  for(unsigned n=0;n<16;++n)check(c.conditions[n]==r[323+n],"Original conditions");
  const XeenItemCategory *items[]{&c.weapons,&c.armor,&c.accessories,&c.miscellaneous};
  for(unsigned category=0;category<4;++category)for(unsigned slot=0;slot<9;++slot) {
   const auto &item=items[category]->at(slot);const auto *p=r+166+category*36+slot*4;
   check(item.material==p[0] && item.id==p[1] && item.state==p[2] && item.frame==p[3],"Original equipment bytes");
  }
  const auto *details=c.originalDetails();check(details,"Source-backed CHR details");
  check(details->xeenSide==r[18] && details->birthDay==r[37] && details->temporaryAc==r[34] &&
   details->lloydMap==r[160] && details->lloydX==r[161] && details->lloydY==r[162] && details->lloydSide==r[310] &&
   c.currentSpell==r[164] && c.quickOption==r[165] && details->townUnknown==unsigned(r[339]|r[340]<<8) &&
   details->savedMaze==r[341] && details->adventuringSpell==r[352] && details->combatSpell==r[353],"Source-backed beacon/quick-spell/town defaults");
  for(unsigned n=0;n<64;++n)check(details->awards[n]==(n==9?r[57+n]:r[57+n]&15) &&
   details->awards[n+64]==(n==9?0:r[57+n]>>4),"CHR award packing");
  for(unsigned n=0;n<7;++n)check(details->attributes[n]==std::array<int,2>{r[20+n*2],r[21+n*2]},"CHR attribute details");
  for(unsigned n=0;n<18;++n)check(details->skills[n]==r[39+n],"CHR skills");
  for(unsigned n=0;n<6;++n)check(details->resistances[n]==std::array<int,2>{r[311+n*2],r[312+n*2]},"CHR resistances");
  check(details->experience==std::uint32_t(r[348])+(std::uint32_t(r[349])<<8)+(std::uint32_t(r[350])<<16)+(std::uint32_t(r[351])<<24),"Source-backed experience");
  if(supplements) {
   const auto &input=party.roster.combatInputs(owner);check(bool(input) && bool(c.learnedSpells),"Complete fresh supplements");
   check(input->might.permanent==r[20] && input->might.temporary==r[21] && input->speed.permanent==r[28] &&
    input->speed.temporary==r[29] && input->accuracy.permanent==r[30] && input->accuracy.temporary==r[31] &&
    input->luck->permanent==r[32] && input->luck->temporary==r[33] && input->temporaryAc==r[34],"Original physical inputs");
   const auto &res=*input->resistances;
   check(res.firePermanent==r[311] && res.fireTemporary==r[312] && res.coldPermanent==r[313] &&
    res.coldTemporary==r[314] && res.electricalPermanent==r[315] && res.electricalTemporary==r[316] &&
    input->poisonResistance->permanent==r[317] && input->poisonResistance->temporary==r[318] &&
    res.energyPermanent==r[319] && res.energyTemporary==r[320] && res.magicPermanent==r[321] &&
    res.magicTemporary==r[322],"Original combat resistances");
   check(input->experience==std::uint32_t(r[348]|r[349]<<8|r[350]<<16|r[351]<<24),"Original XP");
   for(unsigned n=0;n<39;++n)check((*c.learnedSpells)[n]==r[121+n],"Original book");
  }
 }
}
// Literal initial setIndoorMonsters result at (18,4) West. The corridor
// contains record 35 at (15,4); all other records are outside this view.
inline void view(const XeenActorView &v) {
 for(unsigned n=0;n<v.activation.size();++n)check(v.activation[n]==(n==35),"Initial city activation oracle");
 for(unsigned n=0;n<v.slots.size();++n)check(n==9 ? v.slots[n]==XeenMonsterIdentity{28,35} : !v.slots[n],"Initial city view-slot oracle");
}
}
#endif
