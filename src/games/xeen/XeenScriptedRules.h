#ifndef MMODERN_XEEN_SCRIPTED_RULES_H
#define MMODERN_XEEN_SCRIPTED_RULES_H
#include "games/xeen/XeenEventValues.h"
#include "games/xeen/XeenActor.h"
namespace mmodern {
// Detached death-rule result. The combat/encounter owner publishes it with XP.
struct XeenScriptedDeath {
 XeenGameFlags flags;
 XeenCloudsQuestFlags quests;
 XeenCloudsQuestItems items;
 std::array<std::array<std::uint8_t,128>,6> awards{};
 void publish(XeenPartyState &,XeenGameFlags &) const noexcept;
};
XeenScriptedDeath xeenPrepareScriptedDeath(const XeenPartyState &,const XeenGameFlags &,
 const XeenCamera &,const XeenActor &);
void xeenRegenerateIceTrolls(std::vector<XeenActor> &,XeenMapIdentity);
// Derived on map load, never serialized or used as a second flag owner.
std::uint16_t xeenMapRuleView(const XeenGameFlags &) noexcept;
namespace xeen_scripted_rules_detail {
struct ObjectRule {unsigned flag,map,slot;};
inline constexpr ObjectRule objects[]{{63,7,5},{39,4,6},{64,12,11},{65,11,10},{66,15,11},
 {67,10,9},{68,16,9},{69,15,14},{49,19,14}};
}
// Kept inline so geometry-only World clients need no party/Event implementation.
inline XeenMapEntity xeenMapRuleObject(XeenMapEntity object,XeenObjectIdentity id,std::uint16_t view) noexcept {
 if(id.mapId.side!=XeenSide::Clouds)return object;
 const auto &objects=xeen_scripted_rules_detail::objects;
 for(unsigned n=0;n<9;++n)if((view&(1u<<n)) && id.mapId.number==objects[n].map) {
  if(id.recordIndex==objects[n].slot)object.x=object.y=-128;
  break; // DOS else-if chain: the first flag/map match suppresses later entries.
 }
 if((view&(1u<<9)) && id.mapId.number==12 && object.resourceId==118)object.resourceId=85;
 return object;
}
inline bool xeenDrainedCave(XeenMapIdentity map,std::uint16_t view) noexcept {
 return map.side==XeenSide::Clouds && (view&(1u<<10)) && map.number>=42 && map.number<=44;
}
bool xeenLakeDragonsGone(const std::vector<XeenActor> &) noexcept;
}
#endif
