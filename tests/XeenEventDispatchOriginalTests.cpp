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
 Inputs in(*installation);trap(in);label(in);guardedNpc(in);std::cout<<"M84 original-data Event passes OK\n";return 0;
}catch(const std::exception &e){std::cerr<<e.what()<<'\n';return 1;}}



