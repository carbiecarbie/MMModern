#include "XeenTrainingTestSupport.h"
#include "XeenTestInstallation.h"
#include "tools/XeenTestSave.h"
#include "games/xeen/XeenScriptedRules.h"
#include "TestTreeRemoval.h"
#include <chrono>
#include <filesystem>
#include <iostream>
using namespace training_test;
namespace mmodern {
struct XeenMonsterTestAccess {
 static std::string state(const XeenEventFlow &f) {
  std::string s=" pending="+std::to_string(bool(f._pending))+" transition="+std::to_string(bool(f._transition))+" pass="+std::to_string(bool(f._explorationPass))+" rest="+std::to_string(f._encounter->_needsRestNotice)+" blocks="+std::to_string(f._presenter.blocksGameplay())+" response="+std::to_string(bool(f._npcResponse));
  if(f._npcSequence)s+=" npc acquired="+std::to_string(f._npcSequence->acquired)+" composed="+std::to_string(f._npcSequence->composed)+" deadline="+std::to_string(f._npcSequence->deadline)+" now="+std::to_string(f._presentationClock());
  if(const auto r=request(f))s+=" kind="+std::to_string(int(r->kind))+" response="+std::to_string(int(r->response))+" line="+std::to_string(r->source.line);
  return s;
 }
 static std::optional<XeenPresentationRequest> request(const XeenEventFlow &flow) {
  if(flow._pending && flow._pending->state.pendingPresentation)return flow._pending->state.pendingPresentation->request;
  return {};
 }
};
}
namespace {
namespace fs=std::filesystem;
struct Directory {
 fs::path path=fs::temp_directory_path()/("mmodern-m89-"+std::to_string(std::chrono::steady_clock::now().time_since_epoch().count()));
 Directory(){check(fs::create_directory(path),"M89 temporary directory");}
 ~Directory(){removeTestTree(path);}
};
void table(Inputs &in,XeenSaveSnapshot &snapshot,XeenMapIdentity map) {
 if(snapshot.find(map) && snapshot.find(map)->monsters)return;
 auto actors=XeenActorApproach::actorsFromResources(in.maps.loadObjects(in.assets,map),in.statistics);
 auto &saved=snapshot.map(map).monsters;saved.emplace(XeenSaveMonsterTable{std::uint16_t(actors.size()),{}});
 for(const auto &a:actors)saved->slots.push_back({a.x,a.y,a.lifecycle,a.status,false,-1});
}
void press(Fixture &f,PlayerAction action) {f.now+=125;f.act(action);f.flow->completeInputHandoff(f.flow->frame().presentation());}
void tick(Fixture &f) {
 f.now+=125;f.flow->beginCycle(++f.cycle);
 if(auto image=f.flow->updatePresentation())f.present(*image);
 f.flow->completeInputHandoff(f.flow->frame().presentation());
}
void settle(Fixture &f,std::vector<XeenPresentationRequest> *requests=nullptr,bool attack=false,bool *staged=nullptr) {
 for(unsigned n=0;n<2000;++n) {
  if(staged && f.w.treasureStaging().value().weapons[0].id==90)*staged=true;
  const auto request=XeenMonsterTestAccess::request(*f.flow);
  if(request && requests)requests->push_back(*request);
  const auto *combat=f.flow->encounter()->combat();
  if(attack && combat && combat->phase()==XeenCombatPhase::PlayerReady)press(f,AttackAction{});
  else if((request && request->response==XeenPresentationResponseRequirement::Acknowledgment) || f.flow->presenter().blocksGameplay())press(f,AcknowledgeAction{});
  try {tick(f);}catch(const std::exception &e) {
   const auto *fight=f.flow->encounter()->combat();
   throw std::runtime_error(std::string(e.what())+" at M89 tick "+std::to_string(n)+" map "+std::to_string(f.c.mapId.number)+" combat "+std::to_string(fight?int(fight->phase()):-1)+" flag60 "+std::to_string(f.f.isSet(60))+" notice "+f.flow->encounter()->notice());
  }
  if(f.flow->canSave())return;
 }
 throw std::runtime_error("M89 Flow did not return to Quiet: "+XeenMonsterTestAccess::state(*f.flow)+"\n"+f.flow->encounter()->notice());
}
void lord(Inputs &in,XeenSaveSnapshot snapshot) {
 // An active dead member exercises the DOS all-member award loop in the real kill.
 const auto dead=snapshot.activeRosterIds.back();snapshot.characters[dead].currentHp=0;
 snapshot.characters[dead].conditions[unsigned(XeenCondition::Dead)]=1;
 Fixture f(in,snapshot);check(f.w.sessionState().regionalActors(78).at(8).original.resourceId==89,"Original Lord MOB binding");
 press(f,NavigationAction::MoveForward);bool staged=false;settle(f,nullptr,true,&staged);
 check(f.w.sessionState().regionalActors(78).at(8).lifecycle==XeenActorLifecycle::Defeated,"Lord must die through the equipped sword");
 check(staged && f.p.questItems.at(8)==1 && f.f.isSet(60) && !f.p.questFlags.isSet(23),"Lord kill, staged Scepter and quest flags");
 check(!f.w.treasureStaging().value().weapons[0].id,"Ordinary treasure delivery must empty the Scepter staging");
 for(auto id:f.p.party.activeRosterIds())check(f.p.roster.at(id).awards[42]==1,"Lord award includes dead active members");
 press(f,NavigationAction::TurnLeft);settle(f);press(f,NavigationAction::MoveForward);settle(f);press(f,InteractionAction{});
 std::vector<XeenPresentationRequest> requests;settle(f,&requests);
 bool mirror=false,notice=false;
 for(const auto &r:requests) {
  if(r.text.find("You have found the Sixth Mirror!")!=std::string::npos) {check(!notice,"Mirror text must precede notice");mirror=true;}
  if(r.text=="Clouds ending: not supported yet") {check(mirror,"Ending notice skipped original mirror text");notice=true;}
 }
 if(!mirror || !notice) {std::cerr<<"Mirror debug "<<f.c.x<<","<<f.c.y<<" dir "<<unsigned(f.c.direction)<<" requests "<<requests.size()<<" input "<<f.flow->acceptsInputFrame(f.flow->frame().presentation())<<"\n";for(const auto &r:requests)std::cerr<<"request "<<unsigned(r.source.opcode)<<" "<<r.text.substr(0,90)<<"\n";}
 check(mirror && notice && f.c.mapId==78 && !f.f.isSet(75),"Sixth Mirror reaches the terminal notice without running the ending");
 press(f,NavigationAction::TurnRight);settle(f);check(f.flow->canSave(),"Game must keep running after the notice");
}
void clan(Inputs &in,XeenSaveSnapshot snapshot) {
 Fixture f(in,snapshot);check(f.w.sessionState().regionalActors(41).at(28).original.resourceId==75,"Original Clan King MOB binding");
 press(f,NavigationAction::MoveForward);settle(f,nullptr,true);
 check(f.w.sessionState().regionalActors(41).at(28).lifecycle==XeenActorLifecycle::Defeated && f.f.isSet(11),"Clan King kill must set flag 11");
 auto completed=f.snapshot();completed.camera={28,14,5,XeenDirection::West};table(in,completed,28);
 Fixture giver(in,completed,false,nullptr,{},{},[&](auto &image,auto portrait,auto phase){in.assets.drawNpc(image,portrait,phase);});press(giver,InteractionAction{});std::vector<XeenPresentationRequest> requests;settle(giver,&requests);
 bool text=false;for(const auto &r:requests)text=text || r.text.find("Red Dwarf Badge of Courage")!=std::string::npos;
 check(text && giver.f.isSet(12) && !giver.p.questFlags.isSet(1),"Gunther completion text and quest settlement");
}
void exactClanRestore(Inputs &in,Fixture &f) {
 const auto saved=f.snapshot();const auto bytes=XeenSaveFormat::encode(saved);
 Fixture restored(in,XeenSaveFormat::decode(bytes));
 check(restored.f.isSet(11) && restored.w.sessionState().regionalActors(41).at(28).lifecycle==XeenActorLifecycle::Defeated,
  "Restored Clan King hook and lifecycle");
 check(XeenSaveFormat::encode(restored.snapshot())==bytes,"Clan King kill must restore exactly without replaying XP, flags or RNG");
}
void clanDeathPaths(Inputs &in,const XeenSaveSnapshot &source) {
 for(bool spell:{false,true}) {
  auto snapshot=source;
  // Isolate the original King, retaining every MOB record and the King's
  // original position/statistics; the other placements are already defeated.
  auto &monsters=snapshot.map(41).monsters->slots;
  for(unsigned n=0;n<monsters.size();++n)if(n!=28 && monsters[n].lifecycle==XeenActorLifecycle::Present) {
   auto &a=monsters[n];a.x=a.y=-128;a.lifecycle=XeenActorLifecycle::Defeated;a.accounted=true;
  }
  unsigned caster=0;
  if(spell) {
   bool found=false;
   for(unsigned n=0;n<snapshot.activeRosterIds.size();++n) {
    const auto owner=snapshot.activeRosterIds[n];auto &character=snapshot.characters[owner];
    const auto category=XeenLearnedSpellRules::categoryForClass(character.characterClass);
    const auto slot=category?XeenLearnedSpellRules::slotForSpell(*category,unsigned(XeenLearnedSpell::Implosion)):std::nullopt;
    if(!slot)continue;
    caster=n;character.currentSpell=*slot;character.quickOption=1;character.currentSp=1000;
    snapshot.journey->supplements[owner].inputs.speed.temporary=250;found=true;break;
   }
   check(found,"Original party needs an Implosion caster");
  }
  Fixture f(in,snapshot);const auto xpBefore=f.p.roster.combatInputs(f.p.party.activeRosterIds()[0])->experience;
  press(f,spell?PlayerAction{NavigationAction::MoveForward}:PlayerAction{ShootAction{}});
  bool quiet=false,committed=false;
  for(unsigned n=0;n<2000;++n) {
   const auto *combat=f.flow->encounter()->combat();
   check(spell || !combat,"The Clan King Shoot witness must remain outside combat");
   if(combat) {
    if(const auto cast=combat->cast()) {
     committed=committed || cast->committed;
     if(cast->phase==XeenCombatCastPhase::Confirm)press(f,DialogKeyAction{'c'});
    } else if(combat->phase()==XeenCombatPhase::PlayerReady)
     press(f,combat->participant()==int(caster)?PlayerAction{QuickFightAction{}}:PlayerAction{BlockAction{}});
   }
   const auto request=XeenMonsterTestAccess::request(*f.flow);
   if((request && request->response==XeenPresentationResponseRequirement::Acknowledgment) || f.flow->presenter().blocksGameplay())press(f,AcknowledgeAction{});
   tick(f);if(f.flow->canSave()){quiet=true;break;}
  }
  check(quiet,"Clan King ranged/spell kill did not return to Quiet");
  check(f.f.isSet(11) && f.w.sessionState().regionalActors(41).at(28).lifecycle==XeenActorLifecycle::Defeated,
   spell?"Combat spell must publish Clan King flag 11":"Exploration Shoot must publish Clan King flag 11");
  check(f.p.roster.combatInputs(f.p.party.activeRosterIds()[0])->experience>xpBefore,"Ranged/spell kill must publish XP");
  if(spell)check(committed && f.p.party.member(f.p.roster,caster).currentSp<1000,"Clan King spell witness must commit and pay for Implosion");
  exactClanRestore(in,f);
 }
}
void castleObject(Inputs &in) {
 const auto file=in.maps.loadObjects(in.assets,12);unsigned castles=0;
 for(unsigned n=0;n<file.entities.objects.size();++n)if(file.entities.objects[n].resourceId==118) {
  const auto &object=file.entities.objects[n];
  check(n==0 && object.tableIndex==0 && object.x==11 && object.y==12,"Original castle sprite slot and object binding");++castles;
 }
 check(castles==1,"Only object 0 may use sprite 118 in original map 12");
 XeenGameFlags flags;flags.set(43);const auto view=xeenMapRuleView(flags);
 for(unsigned n=0;n<file.entities.objects.size();++n) {
  const auto &original=file.entities.objects[n];const auto effective=xeenMapRuleObject(original,{12,n},view);
  check(effective.resourceId==(n==0?85:int(original.resourceId)) && effective.tableIndex==original.tableIndex,
   "Castle view matches the DOS fixed object-0 ID write and retains graphics slot indices");
 }
}
void lake(Inputs &in,XeenSaveSnapshot snapshot) {
 auto &slots=snapshot.map(15).monsters->slots;
 for(unsigned n=0;n<3;++n){auto &a=slots[n];a.x=a.y=-128;a.lifecycle=XeenActorLifecycle::Defeated;a.accounted=true;}
 Fixture f(in,snapshot);check(!f.f.isSet(56),"Map-15 main load clears completion");
 auto destination=f.w.transitionCandidate();auto flags=destination->loadEventDestination(15,11,true,f.f);
 check(flags.isSet(56),"Leaving original map 15 with three dead dragons sets flag 56");
 auto back=destination->loadEventDestination(11,15,true,flags);check(!back.isSet(56),"Returning to map 15 clears flag 56");
 auto reward=f.snapshot();reward.camera={11,12,13,XeenDirection::South};reward.gameFlags=flags.values();table(in,reward,11);
 Fixture medin(in,reward,false,nullptr,{},{},[&](auto &image,auto portrait,auto phase){in.assets.drawNpc(image,portrait,phase);});press(medin,InteractionAction{});std::vector<XeenPresentationRequest> requests;settle(medin,&requests);
 bool text=false;for(const auto &r:requests)text=text || r.text.find("You have slain the monsters of the lake!")!=std::string::npos;
 check(text && !medin.p.questFlags.isSet(17),"Medin receives map-15 completion");
}
}
int main(int argc,char **argv){try {
 check(argc==2,"usage: scripted-original <installation>");const auto installation=xeenTestInstallationDetector().detect(argv[1]);check(bool(installation),"Original installation missing");
 Directory directory;Inputs in(*installation);
 const auto preset=[&](const char *name){const auto path=directory.path/(std::string(name)+".mmsave");developer::generateTestSave(*installation,name,path);return XeenSaveFile::read(path);};
 std::cout<<"Lord Xeen integration\n";lord(in,preset("m89-lord-xeen"));std::cout<<"Clan King integration\n";const auto clanSource=preset("m89-clan-king");clan(in,clanSource);clanDeathPaths(in,clanSource);castleObject(in);std::cout<<"Map 15 integration\n";lake(in,preset("m89-dragons"));
 std::cout<<"Original Clouds scripted kills, Sixth Mirror, Gunther and Medin passed\n";return 0;
}catch(const std::exception &e){std::cerr<<e.what()<<'\n';return 1;}}
