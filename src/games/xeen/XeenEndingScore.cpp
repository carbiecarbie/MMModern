#include "games/xeen/XeenEndingScore.h"
#include "games/xeen/XeenCharacterRules.h"
#include <iomanip>
#include <sstream>
#include <stdexcept>
namespace mmodern {
std::uint32_t xeenCloudsEndingScore(const XeenPartyState &party,std::uint32_t seconds) {
 if(!party.party.size())throw std::logic_error("Clouds ending requires active members");
 std::uint32_t sum=0;
 for(auto id:party.party.activeRosterIds()) {
  const auto &c=party.roster.at(id);
  const auto &input=party.roster.combatInputs(id);
  if(!input)throw std::logic_error("Clouds ending requires live experience");
  // DAT 0x1e336 counts L-11 above level 12. The existing sheet helper
  // counts L-12; retain that separate display behavior outside this ending.
  sum+=XeenCharacterRules::currentExperience(c,&*input)+(c.permanentLevel>=13?1024000u:0u);
 }
 const std::uint32_t experience=(sum/party.party.size()/10000u)*100000u;
 const auto hours=std::uint16_t(seconds/3600u);
 const auto minutes=std::uint16_t(seconds%3600u)/60u;
 const auto time=std::uint16_t(std::uint32_t(hours)*100u+minutes);
 const auto signedTime=time<0x8000?std::int32_t(time):std::int32_t(time)-0x10000;
 return experience+std::uint32_t(signedTime);
}
std::string xeenFormatEndingScore(std::uint32_t score) {
 std::ostringstream out;out<<std::setfill('0')<<std::setw(10)<<score;return out.str();
}
}
