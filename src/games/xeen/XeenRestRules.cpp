#include "games/xeen/XeenRestRules.h"
#include <limits>
#include <stdexcept>
namespace mmodern {
// Adapted from Interface::rest at ScummVM 6814ee9b, interface.cpp:1144-1238;
// GPL-3.0-or-later, ScummVM developers (COPYRIGHT). Weak/Drunk follows the
// maintainer's approved original-behavior decision in the M51 plan.
bool xeenRestDanger(const XeenConsequenceCharacters &characters,const XeenConsequenceInputs &inputs,unsigned year) {
 for(unsigned i=0;i<characters.size();++i)for(unsigned attribute=0;attribute<7;++attribute)
  if(XeenCharacterRules::sheetStat(characters[i],&inputs[i],attribute,{year})<1)return true;
 return false;
}
bool xeenRestRangedWake(const XeenConsequenceCharacters &characters) {
 for(const auto &c:characters) {
  const auto condition=c.worstCondition();
  if(condition==XeenCondition::Depressed || condition==XeenCondition::Confused || condition==XeenCondition::Good)return true;
 }
 return false;
}
XeenRestRecovery::XeenRestRecovery(const XeenConsequenceCharacters &p,const XeenConsequenceInputs &i,
 const XeenGameplayContext &c,std::uint16_t f):characters(p),inputs(i),context(c),food(f) {
 // The approved original Weak/Drunk replacement occurs in changeTime, before
 // maxima. Do not reproduce the pin's paired post-refill Weak workaround.
 xeenResetPartyTemps(context);
 for(unsigned n=0;n<characters.size();++n)xeenResetCharacterTemps(characters[n],inputs[n]);
 for(auto &member:characters) {
  member.conditions[8]=0;
  if(!food) {starving=true;continue;}
  context.rested=true;
  const auto condition=member.worstCondition();
  if(condition>=XeenCondition::Dead && condition<=XeenCondition::Eradicated)continue;
  --food;++consumed;member.conditions[12]=0;
  const auto hp=XeenCharacterRules::maxHp(member,{context.year}),sp=XeenCharacterRules::maxSp(member,{context.year});
  if(hp>std::numeric_limits<std::int16_t>::max() || sp>std::numeric_limits<std::int16_t>::max())
   throw std::overflow_error("Rest recovery exceeds character storage");
  member.currentHp=hp;member.currentSp=sp;
 }
}
}
