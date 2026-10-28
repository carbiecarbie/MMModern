#include "XeenTrainingTestSupport.h"
#include "XeenTestInstallation.h"
#include "games/xeen/XeenOutdoorScene.h"
#include <iostream>
using namespace training_test;
namespace mmodern {
struct XeenEventDispatchTestAccess {
 static void composer(XeenEventFlow &f,XeenEventFlow::TransitionCompose compose){f._transitionCompose=std::move(compose);}
 static void script(XeenEventFlow &f,const XeenEventFile &file){f._encounter->_journeyEvents=file;f._encounter->retainJourney();f._encounterFrame=f._encounter->ticket();}
 static void liveComposer(XeenEventFlow &f,XeenEventFlow::EncounterCompose compose){f._encounterCompose=std::move(compose);}
 static unsigned candidateLight(const XeenEventFlow &f){return f._transition->party.encounterContext->lightAndResistances[0];}
 static bool damage(const XeenEventFlow &f){return f._pending && f._pending->state.pendingPresentation &&
  f._pending->state.pendingPresentation->request.kind==XeenPresentationKind::PortraitEffect;}
};
}
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
void rubble(Inputs &in) {
 auto source=at(in,33,9,25,XeenDirection::North);Fixture f(in,source);
 auto candidate=f.w.transitionCandidate();XeenPartyState party;candidate->copyEventParty(f.p,party);
 const auto file=in.events.load(33);const auto script=[&](auto id){return XeenEventScript(id==XeenMapIdentity(33)?file:in.events.load(id));};
 const auto text=[&](auto id){return in.texts.load(id);};XeenEventInterpreter vm;
 bool slot0=false;vm.eventDraw=[&](auto &world,const auto &camera,bool){slot0=bool(world.prepareTransitionArrival(camera).slots[0]);};
 vm.eventLineSkipped=[&]{return slot0;};
 auto result=vm.begin(f.c,party,f.f,*candidate,script,text);
 check(std::holds_alternative<XeenEventExecutionSuspended>(result),"Rubble did not ask WhoWill");
 auto pending=std::get<XeenEventExecutionSuspended>(result);
 check(pending.request.kind==XeenPresentationKind::CharacterSelection,"Rubble first presentation is not WhoWill");
 result=vm.resume(pending.state,SelectedCharacter{0},party,*candidate,script,text);
 check(std::holds_alternative<XeenEventExecutionCompleted>(result),"Rubble first search did not complete");
 const auto &actors=candidate->sessionState().regionalActors(33);
 check(actors.at(9).x==9 && actors.at(9).y==25 && actors.at(10).x==9 && actors.at(10).y==25,"Rubble did not spawn both original slots");
 const auto first=std::find_if(file.records.begin(),file.records.end(),[](const auto &r){return r.x==9 && r.y==25 && r.line==0 && r.direction==4;});
 check(first!=file.records.end() && candidate->effectiveEvent({33,std::size_t(first-file.records.begin())},*first).opcode==0,"Rubble line 0 was not disabled");
 // Represent the fight having cleared the two contact slots before a second search.
 candidate->applySpawn(33,9,100,100,0);candidate->applySpawn(33,10,100,100,0);
 slot0=bool(candidate->prepareTransitionArrival(f.c).slots[0]);
 result=vm.begin(f.c,party,f.f,*candidate,script,text);
 check(std::holds_alternative<XeenEventExecutionSuspended>(result),"Empty rubble did not display its message");
 pending=std::get<XeenEventExecutionSuspended>(result);
 check(pending.request.text=="There is nothing in the rubble.","Second rubble search did not report empty rubble");
 result=vm.resume(pending.state,XeenPresentationResponse::Presented,party,*candidate,script,text);
 if(const auto *failure=std::get_if<XeenEventExecutionError>(&result))throw std::runtime_error("Second rubble search: "+failure->message);
 while(const auto *next=std::get_if<XeenEventExecutionSuspended>(&result)) {
  const auto state=next->state;result=vm.resume(state,XeenPresentationResponse::Acknowledged,party,*candidate,script,text);
 }
 const auto *done=std::get_if<XeenEventExecutionCompleted>(&result);
 check(done && done->instructionCount==4 && candidate->sessionState().regionalActors(33).at(9).x==100 && candidate->sessionState().regionalActors(33).at(10).x==100,(std::string("Second rubble search: count=")+(done?std::to_string(done->instructionCount):"not completed")).c_str());
}
void trapScene(Inputs &in,unsigned map,int x,int y,unsigned record,int sprite) {
 auto source=at(in,map,x,y,XeenDirection::North);Fixture f(in,source);const auto before=health(f);
 auto resolver=XeenObjectVisualResolver::load(in.assets);bool seen=false,sawPortrait=false,injected=false,retried=false;
 std::optional<XeenJourneyRandomState> failedRandom;
 f.flow->drawDialogSprite=[&](auto &image,const char *name,unsigned id,int px,int py) {
  if(std::string(name)=="charpow.icn" && XeenEventDispatchTestAccess::damage(*f.flow))sawPortrait=true;
  in.assets.drawDialogSprite(image,name,id,px,py);
 };
 XeenEventDispatchTestAccess::composer(*f.flow,[&](auto &world,const auto &,const auto &camera,auto,auto) {
  const auto commands=XeenIndoorScene().build(world,camera,&resolver,nullptr,0);
  if(XeenEventDispatchTestAccess::damage(*f.flow)) {
   if(map==55 && !injected) {
    injected=true;failedRandom=f.w.sessionState().journeyRandom();
    check(health(f)==before && f.w.effectiveObject({map,record}).x!=x,"Fault published the detached damage scene");
    throw std::runtime_error("Injected candidate scene failure");
   }
   if(map==55 && injected && !retried) {
    check(f.w.sessionState().journeyRandom()==failedRandom && health(f)==before &&
     f.w.effectiveObject({map,record}).x!=x,"Candidate scene retry changed live state");
    retried=true;
   }
   const auto found=std::find_if(commands.begin(),commands.end(),[&](const auto &cmd) {
    return cmd.queryIndex==2 && cmd.object() && cmd.object()->visual.identity==XeenObjectIdentity{map,record};
   });
   check(found!=commands.end() && world.effectiveObject({map,record}).resourceId==sprite,"Trap effect missing from damage scene");
   check(f.w.effectiveObject({map,record}).x!=x || f.w.effectiveObject({map,record}).y!=y,"Trap object published before Event completion");
   seen=true;
  }
  return XeenEventFlow::Composition{frame(),false};
 });
 pass(f);check(seen && sawPortrait && (map!=55 || (injected && retried)),"Trap scene, retry, or live damage portrait was not composed");
 const auto object=f.w.effectiveObject({map,record});
 check(object.x==-128 && object.y==-128 && health(f)<before,"Trap did not finish with its object hidden and party injured");
 const auto commands=XeenIndoorScene().build(f.w,f.c,&resolver,nullptr,0);
 check(std::none_of(commands.begin(),commands.end(),[&](const auto &cmd){return cmd.object() && cmd.object()->visual.identity==XeenObjectIdentity{map,record};}),"Hidden trap object remained in the scene");
}
void outdoorScene(Inputs &in) {
 auto source=at(in,23,9,6,XeenDirection::South);Fixture f(in,source);
 check(f.w.map(23).geometry.isOutdoors(),"Outdoor fixture is not outdoors");
 const auto line=[](unsigned n,unsigned op,std::vector<std::uint8_t> args) {
  XeenEventRecord r;r.x=9;r.y=6;r.direction=4;r.line=n;r.opcode=op;r.parameters=std::move(args);r.lengthField=5+r.parameters.size();return r;
 };
 XeenEventFile script{23,"maze0023.evt",true,{line(0,0x0b,{0,9,6}),line(1,0x20,{0,0}),line(2,0x12,{})}};
 f.events=XeenEventSystem([&](auto id){return XeenEventScript(id==XeenMapIdentity(23)?script:in.events.load(id));},[&](auto id){return in.texts.load(id);});
 XeenEventDispatchTestAccess::script(*f.flow,script);
 auto resolver=XeenObjectVisualResolver::load(in.assets);bool seen=false;std::vector<unsigned> phases;
 XeenEventDispatchTestAccess::composer(*f.flow,[&](auto &world,const auto &,const auto &camera,auto,auto) {
  phases.push_back(world.scenePresentation().drawingState().floatPhase);
  const auto commands=XeenOutdoorScene().build(world,camera,&resolver,nullptr,0);
  seen=seen || std::any_of(commands.begin(),commands.end(),[](const auto &cmd){return cmd.sampleIndex==2 && cmd.sourceX==9 && cmd.sourceY==6 && cmd.object() && cmd.object()->visual.identity==XeenObjectIdentity{23,0};});
  return XeenEventFlow::Composition{frame(),true};
 });
 f.act(InteractionAction{});check(seen && f.flow->canCancelInteraction(),"Outdoor Event did not draw the moved object before WhoWill");
 phases.clear();
 for(unsigned n=0;n<6;++n) {f.now+=125;f.flow->beginCycle(++f.cycle);if(auto next=f.flow->updatePresentation())f.present(*next);}
 check(phases.size()>=2 && std::adjacent_find(phases.begin(),phases.end(),std::not_equal_to<unsigned>())!=phases.end(),
  "Outdoor scene animation did not advance during WhoWill");
 f.act(SelectMemberAction{0});finish(f,true);
}
void darkPendingScene(Inputs &in) {
 auto source=at(in,28,20,20,XeenDirection::North);source.journey->context->lightAndResistances[0]=1;
 XeenWorld geometry(in.mapLoader());const auto cell=geometry.sampleCell(28,20,20);
 for(unsigned d=0;d<4;++d) {const auto wall=wallAt(*cell->cell,XeenDirection(d));
  if(wall!=1 && wall!=6 && wall!=9 && wall!=13){source.camera.direction=XeenDirection(d);break;}}
 XeenEventRecord who;who.x=20;who.y=20;who.direction=4;who.line=0;who.opcode=0x20;who.parameters={0,0};who.lengthField=7;
 XeenEventFile script{28,"maze0028.evt",true,{who}};
 auto maps=in.mapLoader();
 Fixture f(in,source,false,&script,[maps](auto id) {auto map=maps(id);
  if(id==XeenMapIdentity(28))map.geometry.flags2|=0x4000;
  for(auto &cell:map.geometry.cells)cell.rawAttributes|=0x20;return map;});
 f.w.scenePresentation().markLightingStep();bool seen=false;
 XeenEventDispatchTestAccess::composer(*f.flow,[&](auto &world,const auto &party,const auto &camera,auto,auto) {
  if(!seen) {
   check(party.encounterContext->lightAndResistances[0]==0 && world.sceneIsBlack(party,camera),"Dark pending scene used unprepared light");
   check(XeenEventDispatchTestAccess::candidateLight(*f.flow)==1 && f.p.encounterContext->lightAndResistances[0]==1,
    "Drawing the dark scene published or changed candidate light");seen=true;
  }
  return XeenEventFlow::Composition{frame(),false};
 });
 f.act(InteractionAction{});check(seen && f.flow->canCancelInteraction(),"Dark Event did not reach its pending presentation");
 f.act(SelectMemberAction{0});finish(f,true);
}
void manualRefusalScene(Inputs &in) {
 auto source=at(in,28,20,20,XeenDirection::North);
 XeenWorld geometry(in.mapLoader());const auto cell=geometry.sampleCell(28,20,20);
 for(unsigned d=0;d<4;++d) {const auto wall=wallAt(*cell->cell,XeenDirection(d));
  if(wall!=1 && wall!=6 && wall!=9 && wall!=13){source.camera.direction=XeenDirection(d);break;}}
 const auto line=[](unsigned n,unsigned op,std::vector<std::uint8_t> args) {
  XeenEventRecord r;r.x=20;r.y=20;r.direction=4;r.line=n;r.opcode=op;r.parameters=std::move(args);r.lengthField=5+r.parameters.size();return r;
 };
 XeenEventFile script{28,"maze0028.evt",true,{line(0,0x0b,{0,20,20}),line(1,0x20,{0,0}),line(2,0x21,{0,0})}};
 Fixture f(in,source,false,&script);const auto original=f.w.effectiveObject({28,0});bool seen=false;unsigned liveDraws=0;
 check(original.x!=20 || original.y!=20,"Synthetic refusal needs a moved object");
 XeenEventDispatchTestAccess::composer(*f.flow,[&](auto &world,const auto &,const auto &camera,auto,auto) {
  const auto obj=world.effectiveObject({28,0});
  if(obj.x==20 && obj.y==20) {
   check(camera.x==20 && camera.y==20 && f.w.effectiveObject({28,0}).x==original.x && f.w.effectiveObject({28,0}).y==original.y,"Manual candidate escaped before refusal");seen=true;
  }
  return XeenEventFlow::Composition{frame(),false};
 });
 XeenEventDispatchTestAccess::liveComposer(*f.flow,[&](auto,auto) {
  ++liveDraws;check(f.w.effectiveObject({28,0}).x==original.x && f.w.effectiveObject({28,0}).y==original.y,"Refused object survived in live scene");
  return XeenEventFlow::Composition{frame(),false};
 });
 f.act(InteractionAction{});check(seen && f.flow->canCancelInteraction(),(std::string("Manual Event state: seen=")+(seen?"yes":"no")+" cancel="+(f.flow->canCancelInteraction()?"yes":"no")+" notice="+f.flow->encounter()->notice()).c_str());
 f.act(SelectMemberAction{0});finish(f,true);
 const auto before=liveDraws;f.flow->refresh(true);
 check(liveDraws>before && f.w.effectiveObject({28,0}).x==original.x && f.w.effectiveObject({28,0}).y==original.y,"Refused Event did not return to live scene");
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
 Inputs in(*installation);trap(in);rubble(in);trapScene(in,55,7,8,5,87);trapScene(in,33,3,15,0,89);manualRefusalScene(in);outdoorScene(in);darkPendingScene(in);label(in);guardedNpc(in);combatLabels(in);combatTrap(in);std::cout<<"M84 original-data Event passes OK\n";return 0;
}catch(const std::exception &e){std::cerr<<e.what()<<'\n';return 1;}}



