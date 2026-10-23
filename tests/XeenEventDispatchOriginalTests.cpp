#include "XeenTrainingTestSupport.h"
#include "XeenTestInstallation.h"
#include <iostream>
using namespace training_test;
namespace {
XeenSaveSnapshot at(Inputs &in,unsigned map,int x,int y,XeenDirection direction) {
 auto save=in.base();save.camera={map,x,y,direction};
 if(map!=23) {
  const auto objects=in.maps.loadObjects(in.assets,map);
  const auto actors=XeenActorApproach::actorsFromResources(objects,in.statistics);
  save.map(map).monsters.emplace(XeenSaveMonsterTable{std::uint16_t(actors.size()),{}});
  for(const auto &a:actors)save.map(map).monsters->slots.push_back({a.x,a.y,a.lifecycle,a.status,false,-1});
 }
 for(auto &m:save.maps)if(m.monsters)for(auto &a:m.monsters->slots)if(a.lifecycle==XeenActorLifecycle::Present) {
  a.x=a.y=-128;a.lifecycle=XeenActorLifecycle::Defeated;a.accounted=true;
 }
 for(auto owner:save.activeRosterIds){save.characters[owner].currentHp=1000;save.characters[owner].conditions.fill(0);}
 return save;
}
void finish(Fixture &f,bool answer=false) {
 for(unsigned n=0;!f.flow->canSave();++n) {
  check(n<1500,"original-data pass settlement bounded");f.now+=25;
  // The SDL loop accepts queued input before idle renews the NPC deadline.
  const auto input=f.flow->inputContext(f.flow->frame().presentation());
  if(input.readyForAction && input.dialog && f.flow->presentationGeneration()) {
   if(f.flow->canCancelInteraction())f.act(CancelInteractionAction{});
   else if(input.dialog->anyKey)f.act(AcknowledgeAction{});
   else if(answer)f.act(NoAction{});
  }
  f.flow->beginCycle(++f.cycle);
  if(auto frame=f.flow->updatePresentation())f.present(*frame);
 }
}
void pass(Fixture &f){f.present(f.flow->completeExplorationInput());finish(f,true);}
int health(const Fixture &f){int hp=0;for(auto owner:f.p.party.activeRosterIds())hp+=f.p.roster.at(owner).currentHp;return hp;}
void trap(Inputs &in) {
 const auto source=at(in,26,8,3,XeenDirection::North);
 for(unsigned kind=0;kind<4;++kind) {
  Fixture f(in,source);const auto before=health(f);unsigned calls=0;
  f.flow->reportManual=[&](const auto &r){if(std::holds_alternative<XeenManualEventCompleted>(r))++calls;};
  if(kind==0)pass(f);
  else if(kind==1){f.act(WaitAction{});finish(f);}
  else if(kind==2){f.act(InfoAction{});check(!f.flow->canSave() && health(f)==before,"Info retains the trap pass");
   f.act(CancelInteractionAction{});finish(f);}
  else {f.act(CancelInteractionAction{});finish(f);}
  check(calls==1 && health(f)<before,"original map 26 damage on entry, Wait, Info close and Escape, one pass each");
 }
}
void label(Inputs &in) {
 auto save=at(in,28,13,4,XeenDirection::West);Fixture f(in,save);unsigned labels=0;
 f.flow->reportManual=[&](const auto &r){if(const auto *p=std::get_if<XeenEventExecutionSuspended>(&r)) {
  if(p->request.kind==XeenPresentationKind::SceneLabelReduced || p->request.kind==XeenPresentationKind::SceneLabelNormal)++labels;
 }};
 pass(f);check(labels>0,"Vertigo original door label at entry");const auto before=labels;
 f.act(WaitAction{});finish(f);check(labels>before,"Vertigo West label returns after Wait clears it");
}
void combatTrap(Inputs &in) {
 auto save=at(in,55,7,8,XeenDirection::East);
 const auto actors=XeenActorApproach::actorsFromResources(in.maps.loadObjects(in.assets,55),in.statistics);
 const auto &original=actors.at(8);
 check(original.x==10 && original.y==8,"original Tower trap witness monster");
 save.map(55).monsters->slots[8]={original.x,original.y,original.lifecycle,original.status,false,-1};
 for(auto owner:save.activeRosterIds) {
  save.characters[owner].permanentLevel=100;
  save.journey->supplements[owner].inputs.might={200,0};save.journey->supplements[owner].inputs.accuracy={200,0};
 }
 Fixture f(in,save);unsigned postFightDamage=0;bool fought=false;
 f.flow->reportManual=[&](const auto &r){if(const auto *p=std::get_if<XeenEventExecutionSuspended>(&r))
  if(fought && p->request.portrait && p->request.portrait->stage==XeenEventPortraitStage::DamageHold)++postFightDamage;};
 for(unsigned n=0;!f.flow->encounter()->combat();++n) {
  check(n<500,"original trap monster reaches party");
  if(f.flow->canSave())f.act(WaitAction{});
  f.now+=25;f.flow->beginCycle(++f.cycle);if(auto frame=f.flow->updatePresentation())f.present(*frame);
 }
 check(f.c.mapId==XeenMapIdentity(55) && f.c.x==7 && f.c.y==8,"trap terrain retains the party");
 check(f.programState.eventCallSkipped,"flagged trap call skips as monster engages");fought=true;
 for(unsigned n=0;!f.flow->canSave();++n) {
  check(n<5000,"trap fight and caller settle");
  auto *combat=f.flow->encounter()->combat();
  if(combat && combat->phase()==XeenCombatPhase::PlayerReady)f.act(AttackAction{});
  else if(!combat) {
   const auto input=f.flow->inputContext(f.flow->frame().presentation());
   if(input.readyForAction && input.dialog && input.dialog->anyKey)f.act(AcknowledgeAction{});
  }
  f.now+=25;f.flow->beginCycle(++f.cycle);if(auto frame=f.flow->updatePresentation())f.present(*frame);
 }
 check(!f.programState.eventCallSkipped && postFightDamage==1,"post-combat trap scans and hurts exactly once");
}
void combatLabels(Inputs &in) {
 for(unsigned exit=0;exit<4;++exit) {
  auto save=at(in,28,13,4,XeenDirection::East);save.journey->purse->gems=25;
  const auto actors=XeenActorApproach::actorsFromResources(in.maps.loadObjects(in.assets,28),in.statistics);
  const auto &original=actors.at(35);
  check(original.x==15 && original.y==4,"Part 0 original Vertigo monster 35");
  save.map(28).monsters->slots[35]={original.x,original.y,original.lifecycle,original.status,false,-1};
  for(auto owner:save.activeRosterIds) {
   save.characters[owner].permanentLevel=100;
   save.journey->supplements[owner].inputs.might={200,0};
   save.journey->supplements[owner].inputs.accuracy={200,0};
  }
  Fixture f(in,save);unsigned labels=0,calls=0;bool wizard=false;
  std::vector<XeenCamera> wizardCalls,wizardLabels;
  f.flow->reportManual=[&](const auto &r) {
   if(std::holds_alternative<XeenManualEventCompleted>(r)){++calls;if(wizard)wizardCalls.push_back(f.c);}
   if(const auto *p=std::get_if<XeenEventExecutionSuspended>(&r))
    if(p->request.source.opcode==2){++labels;if(wizard)wizardLabels.push_back(f.c);}
  };
  f.act(NavigationAction::TurnLeft);finish(f);
  f.act(NavigationAction::TurnLeft);finish(f);
  check(f.c.direction==XeenDirection::West,"turn to original witness label");
  for(unsigned n=0;!f.flow->encounter()->combat();++n) {
   check(n<200,"original monster reaches labelled party cell");
   if(f.flow->canSave())f.act(WaitAction{});
   f.now+=25;f.flow->beginCycle(++f.cycle);
   if(auto frame=f.flow->updatePresentation())f.present(*frame);
  }
  check(f.w.sessionState().regionalActors(28).at(35).x==13,"original monster walked to party without coordinate edits");
  check(labels && f.programState.eventCallSkipped,"label draw exposes contact to next line head");
  const auto before=labels;
  for(unsigned n=0;!f.flow->canSave();++n) {
   check(n<5000,"original label fight and caller settle");
   if(auto *combat=f.flow->encounter()->combat()) {
    if(combat->phase()==XeenCombatPhase::PlayerReady) {
     if(exit==3) {
      const auto moved=f.flow->mrWizard();check(bool(moved),"Mr Wizard admitted at the original label fight");
      wizard=true;f.present(*moved);
      check(f.p.purse->gems==0 && f.c.x==13 && f.c.y==4,"Mr Wizard zeroes gems before leaving the fight cell");
     }
     else if(exit==2 && f.c.direction!=XeenDirection::East)f.act(NavigationAction::TurnLeft);
     else f.act(exit?PlayerAction{RunAction{}}:PlayerAction{AttackAction{}});
    }
   } else {
    const auto input=f.flow->inputContext(f.flow->frame().presentation());
    if(input.readyForAction && input.dialog)f.act(input.dialog->anyKey?PlayerAction{AcknowledgeAction{}}:PlayerAction{NoAction{}});
   }
   f.now+=25;f.flow->beginCycle(++f.cycle);
   if(auto frame=f.flow->updatePresentation())f.present(*frame);
  }
  check(labels==before+(exit==1 || exit==3?0:1),"caller 4 redraws matching label at fight or Run landing");
  if(exit==3) {
   // Monster 35 still holds slot 0 at the fight cell: caller 4 skips there.
   // The load builds no slots (DOS 0xf542), so the draw's caller 3 at
   // Vertigo's run position tests the same slot 0 and skips too.
   const auto at=[](const XeenCamera &c,int x,int y,XeenDirection d){return c.mapId==XeenMapIdentity(28) && c.x==x && c.y==y && c.direction==d;};
   check(wizardCalls.size()==2 && at(wizardCalls[0],13,4,XeenDirection::West) && at(wizardCalls[1],18,4,XeenDirection::East),
    ("Mr Wizard caller 4 at the fight cell precedes caller 3 at the landing; calls="+std::to_string(wizardCalls.size())).c_str());
   check(wizardLabels.empty(),"Neither skipped caller draws a label");
   check(f.programState.eventCallSkipped && f.p.purse->gems==0,"Both callers skip on the fight's slot 0 and keep the flag");
   check(f.w.sessionState().regionalActors(28).at(35).x==13,"Mr Wizard leaves the original monster at the fight cell");
  } else check(!f.programState.eventCallSkipped,"scanning caller 4 suppresses caller 3");
  if(exit)check(f.c.x==18 && f.c.y==4,"Run and Mr Wizard reach the original Vertigo landing");
 }
}
void guardedNpc(Inputs &in) {
 auto source=at(in,7,6,3,XeenDirection::North);
 Fixture f(in,source,false,nullptr,{}, {},[&](auto &image,auto portrait,auto frame){in.assets.drawNpc(image,portrait,frame);});
 const auto guardedText=in.texts.load(7).strings.at(7);
 unsigned prompts=0;f.flow->reportManual=[&](const auto &r){if(const auto *p=std::get_if<XeenEventExecutionSuspended>(&r))
  if(p->request.kind==XeenPresentationKind::NpcAcknowledgment && p->request.text==guardedText)++prompts;};
 f.act(WaitAction{});finish(f,true);f.act(WaitAction{});finish(f,true);const auto first=prompts;
 check(first>0,"original guarded NPC branch reached");
 f.act(WaitAction{});finish(f,true);check(prompts>first,"guarded original NPC repeats while its guard passes");
 source.gameFlags[39]=true;
 Fixture guarded(in,source,false,nullptr,{}, {},[&](auto &image,auto portrait,auto frame){in.assets.drawNpc(image,portrait,frame);});
 unsigned blockedPrompts=0;guarded.flow->reportManual=[&](const auto &r){if(const auto *p=std::get_if<XeenEventExecutionSuspended>(&r))
  if(p->request.kind==XeenPresentationKind::NpcAcknowledgment && p->request.text==guardedText)++blockedPrompts;};
 guarded.act(WaitAction{});finish(guarded,true);guarded.act(WaitAction{});finish(guarded,true);
 check(!blockedPrompts,"game flag 39 selects another NPC branch instead of repeating this guarded prompt");
}
}
int main(int argc,char **argv){try {
 check(argc==2,"original dispatch test requires game directory");auto installation=xeenTestInstallationDetector().detect(argv[1]);check(bool(installation),"original installation missing");
 Inputs in(*installation);trap(in);label(in);guardedNpc(in);combatLabels(in);combatTrap(in);std::cout<<"M84 original-data Event passes OK\n";return 0;
}catch(const std::exception &e){std::cerr<<e.what()<<'\n';return 1;}}



