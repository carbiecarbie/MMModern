#ifndef MMODERN_COMPLETED_GAMEPLAY_ORACLE_H
#define MMODERN_COMPLETED_GAMEPLAY_ORACLE_H
#include "XeenCombatGameplayTestSupport.h"
#include "formats/xeen/XeenCharacterFormat.h"
#include "games/xeen/XeenStateEquality.h"
namespace completed_test {
using namespace combat_gameplay_test;
struct Oracle {
 XeenSaveSnapshot expected;
 std::vector<XeenActor> actors;
 XeenPartyState initial;
 Oracle(Harness &h, const XeenGameplayServices &s, unsigned seed) : initial(s.resources.loadInitialParty()) {
  // Independent resource baseline plus literal accepted outcomes. No capture,
  // codec, gameplay publication or resource-to-actor constructor supplies this oracle.
  expected.resources=s.resources.signature;expected.camera=XeenActorApproach::kEntry;
  expected.characters=initial.roster.characters();expected.activeRosterIds=initial.party.activeRosterIds();
  expected.questItems=initial.questItems.counts();expected.questFlags=initial.questFlags.values();
  expected.gameFlags=s.initialFlags().values();
  XeenSaveCompletedEncounter end;end.context=s.resources.loadInitialContext();
  end.context.minutes=seed==1?493:492;end.context.ctr24=1;
  const auto chr=s.resources.loadInitialCharacters();
  constexpr unsigned owners[]{0,1,6,11,14,18};
  for(unsigned i=0;i<6;++i){
   auto &v=end.supplements[i];v.owner=owners[i];v.inputs=XeenCharacterFormat::parseCombatInputs(chr,owners[i]);
   v.inputs.experience+=(seed==1?82u:owners[i]==1?0u:100u);
  }
  auto &rebecca=expected.characters[1];rebecca.currentHp=seed==1?-4:-17;
  rebecca.conditions[12]=1;rebecca.conditions[13]=seed==56?1:0;
  if(seed==56){
   check(xeenSameItem(initial.roster.at(1).armor[0],{0,2,0,3})&&xeenSameItem(initial.roster.at(1).armor[1],{38,10,0,9}),"independent initial Rebecca armor");
   rebecca.armor[0]={0,2,128,3};rebecca.armor[1]={38,10,128,9};
   check(xeenSameItem(initial.roster.at(6).accessories[1],{86,1,0,0})&&xeenSameItem(initial.roster.at(0).accessories[1],{}),"independent initial Speed ring");
   expected.characters[6].accessories[1]={};expected.characters[0].accessories[1]={86,1,0,8};
  }
  expected.completedEncounter=end;
  const auto mob=s.objects(20);const auto mon=s.resources.loadMonsterStatistics();
  check(mob.entities.monsters.size()==27,"all 27 original records required");
  for(unsigned i=0;i<27;++i){
   XeenActor a;a.id={20,i};a.original=mob.entities.monsters[i];a.x=a.original.x;a.y=a.original.y;
   if(a.original.hasResource()) {a.statistics=mon.at(a.original.resourceId);a.hp=a.statistics->baseHp();a.lifecycle=XeenActorLifecycle::Present;}
   if(a.original.isDisabled())a.lifecycle=XeenActorLifecycle::Disabled;
   if(i==5){a.x=a.y=-128;a.hp=0;a.lifecycle=XeenActorLifecycle::Defeated;}
   actors.push_back(a);
  }
 }
 void live(const XeenWorld &w,const XeenPartyState &p,const XeenCamera &c,const XeenGameFlags &f) const {
  const auto &s=w.sessionState();
  check(s.completion()==XeenEncounterCompletion::VictoryQuiescent&&s.combatAccounted()&&s.encounterEntry()==XeenEncounterEntry::Diagnostic27&&
   s.encounterMarked()&&s.encounterInitialized()&&s.encounterTerminal()&&s.completedMonster()==XeenMonsterIdentity{20,5},"completed authority facts");
  check(p.roster.combatMarked()&&p.encounterContext==expected.completedEncounter->context,"exact completed context and marker");
  check(p.party.activeRosterIds()==expected.activeRosterIds&&p.firstSerializedCount==initial.firstSerializedCount&&
   p.effectiveSerializedCount==initial.effectiveSerializedCount&&p.diagnostics==initial.diagnostics,"membership/metadata");
  for(unsigned owner=0;owner<30;++owner){
   remove_test::checkSameCharacter(expected.characters[owner],p.roster.at(owner));
   const XeenCombatInputs *expectedInputs=nullptr;
   for(const auto &v:expected.completedEncounter->supplements)if(v.owner==owner)expectedInputs=&v.inputs;
   check(bool(p.roster.combatInputs(owner))==bool(expectedInputs),"all 30 supplement presence values");
   if(expectedInputs)check(xeen_state::sameInputs(*expectedInputs,*p.roster.combatInputs(owner)),"all supplement fields and XP");
  }
  check(xeen_state::sameCamera(expected.camera,c)&&p.questItems.counts()==expected.questItems&&p.questFlags.values()==expected.questFlags&&
   f.values()==expected.gameFlags,"exact camera/quest/game flags");
  check(std::vector<XeenObjectIdentity>(s.disabledObjects().begin(),s.disabledObjects().end())==expected.disabledObjects&&
   std::vector<XeenEventIdentity>(s.disabledEvents().begin(),s.disabledEvents().end())==expected.disabledEvents,"independent ordinary overlays");
  sameActors(actors,s.actors());
 }
 void live(const Harness &h) const {live(*h.world,*h.party,*h.camera,*h.flags);}
 void disk(const std::filesystem::path &path) const {
  const auto saved=XeenSaveFile::read(path);save_test::sameSnapshot(expected,saved);
  std::ifstream in(path,std::ios::binary);Bytes bytes(std::istreambuf_iterator<char>(in),{});
  check(bytes.size()>263&&bytes[8]==3&&bytes[9]==0,"actual disk v3 header");
  const auto b=bytes.size()-243;
  const unsigned prefix[]{1,2,1,1,0,20,0,5,0,0,0,0,0};
  for(unsigned i=0;i<13;++i)check(bytes[b+i]==prefix[i],"literal completion wire field");
  check(bytes[b+44]==6,"literal six-owner suffix");
  for(unsigned i=0;i<6;++i){const auto &v=expected.completedEncounter->supplements[i];const auto at=b+45+i*33;
   check(bytes[at]==v.owner,"literal owner wire order");
   const unsigned xp=bytes[at+29]|(unsigned(bytes[at+30])<<8)|(unsigned(bytes[at+31])<<16)|(unsigned(bytes[at+32])<<24);
   check(xp==v.inputs.experience,"literal disk XP");}
 }
};
inline Bytes diskBytes(const std::filesystem::path &p){std::ifstream in(p,std::ios::binary);check(bool(in),"disk evidence read");return {std::istreambuf_iterator<char>(in),{}};}
}
#endif
