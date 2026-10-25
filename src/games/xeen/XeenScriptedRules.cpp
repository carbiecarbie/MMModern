#include "games/xeen/XeenScriptedRules.h"
namespace mmodern {
XeenScriptedDeath xeenPrepareScriptedDeath(const XeenPartyState &before,const XeenGameFlags &flags,
 const XeenCamera &camera,const XeenActor &actor) {
 XeenPartyState party;party.party=before.party;party.questFlags=before.questFlags;party.questItems=before.questItems;
 for(auto id:party.party.activeRosterIds())party.roster.at(id)=before.roster.at(id);
 auto nextFlags=flags;auto position=camera;XeenEventScriptValues script;
 const XeenEventValueOwners owners{party,nextFlags,position,script};
 if(camera.mapId.side==XeenSide::Clouds) {
  // DOS 0x2c191..0x2c237: MOB slots, not monster species; no living-member test.
  if(camera.mapId.number==41 && actor.id.recordIndex==28)xeenEventGive(owners,20,11,0);
  if(camera.mapId.number==78 && actor.id.recordIndex==8) {
   xeenEventGive(owners,20,60,0);xeenEventTake(owners,104,23,0);
   for(unsigned n=0;n<party.party.size();++n)xeenEventGive(owners,15,42,n);
  }
  if(camera.mapId.number==29 && actor.id.recordIndex==27)xeenEventGive(owners,20,104,0);
  // DOS 0x2c337 increments a byte, independently of the eventual delivery.
  if(camera.mapId.number!=85 && actor.original.resourceId==89)party.questItems.eventGrant(8);
 }
 XeenScriptedDeath result;result.flags=nextFlags;result.quests=party.questFlags;result.items=party.questItems;
 for(unsigned n=0;n<party.party.size();++n)result.awards[n]=party.party.member(party.roster,n).awards;
 return result;
}
void XeenScriptedDeath::publish(XeenPartyState &party,XeenGameFlags &gameFlags) const noexcept {
 gameFlags=flags;party.questFlags=quests;party.questItems=items;
 for(unsigned n=0;n<party.party.size();++n)party.roster.at(party.party.activeRosterIds()[n]).awards=awards[n];
}
void xeenRegenerateIceTrolls(std::vector<XeenActor> &actors,XeenMapIdentity map) {
 if(map.side!=XeenSide::Clouds)return;
 // DOS 0x2ea4a..0x2ea85: during new-round setup, before the next opportunity.
 for(auto &actor:actors)if(actor.lifecycle==XeenActorLifecycle::Present && actor.original.resourceId==53 && actor.statistics)
  actor.hp=actor.statistics->baseHp();
}
std::uint16_t xeenMapRuleView(const XeenGameFlags &flags) noexcept {
 const auto &objects=xeen_scripted_rules_detail::objects;
 unsigned view=0;for(unsigned n=0;n<9;++n)if(flags.isSet(objects[n].flag))view|=1u<<n;
 if(flags.isSet(43))view|=1u<<9;if(flags.isSet(25))view|=1u<<10;
 return std::uint16_t(view);
}
bool xeenLakeDragonsGone(const std::vector<XeenActor> &actors) noexcept {
 if(actors.size()<3)return false;
 // DOS tests the unsigned position bytes, not HP (0x2655d..0x26585).
 for(unsigned n=0;n<3;++n)if(std::uint8_t(actors[n].x)<=31 && std::uint8_t(actors[n].y)<=31)return false;
 return true;
}
}
