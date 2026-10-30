#include "XeenTrainingTestSupport.h"
#include "XeenTestInstallation.h"
#include "games/xeen/CloudsMapComposer.h"
#include "games/xeen/XeenOutdoorScene.h"
#include "games/xeen/XeenSkills.h"
#include "games/xeen/XeenEventTrigger.h"
#include <iostream>
using namespace training_test;
namespace mmodern {
struct XeenEventDispatchTestAccess {
 static void composer(XeenEventFlow &f,XeenEventFlow::TransitionCompose compose){f._transitionCompose=std::move(compose);}
 static void script(XeenEventFlow &f,const XeenEventFile &file){f._encounter->_journeyEvents=file;f._encounter->retainJourney();f._encounterFrame=f._encounter->ticket();}
 static void liveComposer(XeenEventFlow &f,XeenEventFlow::EncounterCompose compose){f._encounterCompose=std::move(compose);}
 static unsigned candidateLight(const XeenEventFlow &f){return f._transition->party.encounterContext->lightAndResistances[0];}
 static std::optional<XeenPresentationRequest> request(const XeenEventFlow &f) {
  return f._pending && f._pending->state.pendingPresentation ?
   std::optional<XeenPresentationRequest>(f._pending->state.pendingPresentation->request) : std::nullopt;
 }
 static XeenWorld &candidateWorld(const XeenEventFlow &f){return *f._transition->world;}
 static const XeenPartyState &candidateParty(const XeenEventFlow &f){return f._transition->party;}
 static IndexedFrame candidateWithDialogs(const XeenEventFlow &f,const IndexedFrame &base) {
  auto presenter=f._presenter;return presenter.rebase(base);
 }
 static bool damage(const XeenEventFlow &f){return f._pending && f._pending->state.pendingPresentation &&
  f._pending->state.pendingPresentation->request.kind==XeenPresentationKind::PortraitEffect;}
 static std::string whoWillRefusal(const XeenEventFlow &f){return f._whoWillRefusal;}
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
// I2 / B2: the original rubble selection finishes and both spawned actors
// are published before the resulting combat starts.
void rubbleFlow(Inputs &in,unsigned variant=0) {
 auto source=in.base();source.camera={33,9,25,XeenDirection::North};
 if(variant==1)source.activeRosterIds.resize(1);
 if(variant==2){auto &first=source.characters[source.activeRosterIds[0]];first.conditions[13]=1;first.currentHp=0;}
 Fixture f(in,source);
 const auto visuals=XeenObjectVisualResolver::load(in.assets);
 const auto draw=[&](XeenWorld &world,const XeenPartyState &,const XeenCamera &camera,std::uint64_t,XeenMonsterAppearance) {
  XeenIndoorScene().build(world,camera,&visuals,nullptr,0);return XeenEventFlow::Composition{frame(),false};
 };
 XeenEventDispatchTestAccess::composer(*f.flow,draw);
 XeenEventDispatchTestAccess::liveComposer(*f.flow,[&](auto phase,auto appearance){return draw(f.w,f.p,f.c,phase,appearance);});
 std::string error;f.flow->reportManual=[&](const auto &result){if(const auto *bad=std::get_if<XeenEventExecutionError>(&result))error=bad->message;};
 f.now+=125;f.act(InteractionAction{});bool selected=false,refused=false;
 for(unsigned n=0;n<1800 && !f.flow->encounter()->combat();++n) {
  const auto request=XeenEventDispatchTestAccess::request(*f.flow);
  const auto input=f.flow->inputContext(f.flow->frame().presentation());
  if(input.readyForAction && input.dialog) {
   if(request && request->response==XeenPresentationResponseRequirement::CharacterSelection) {
    if(variant==2 && !refused) {
     f.act(SelectMemberAction{0});check(!XeenEventDispatchTestAccess::whoWillRefusal(*f.flow).empty(),"I6 disabled member was not refused");
     f.act(AcknowledgeAction{});refused=true;
    } else {f.act(SelectMemberAction{variant==2?1u:0u});selected=true;}
   }
   else if(input.dialog->anyKey)f.act(AcknowledgeAction{});
  }
  f.now+=25;f.flow->beginCycle(++f.cycle);if(auto image=f.flow->updatePresentation())f.present(*image);
 }
 check(error.empty() && (variant==1?!selected:selected) && (variant!=2 || refused) && f.flow->encounter()->combat(),
  "I2/B2/I6 rubble did not reach combat after owner selection");
 const auto &actors=f.w.sessionState().regionalActors(33);
 check(actors.at(9).x==9 && actors.at(9).y==25 && actors.at(10).x==9 && actors.at(10).y==25,"B2 rubble combat started before both spawns");
 const auto file=in.events.load(33);
 const auto first=std::find_if(file.records.begin(),file.records.end(),[](const auto &r){return r.x==9 && r.y==25 && r.line==0 && r.direction==4;});
 check(first!=file.records.end() && f.w.effectiveEvent({33,std::size_t(first-file.records.begin())},*first).opcode==0,"B2 rubble Event was not disabled before combat");
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
// Q01: use the original skull Event and the movement owner, while observing
// the actual displayed frame and the candidate composition of the same draw.
struct SkullRaster {IndexedFrame live,candidate;};
struct SkullObservation {
 bool seen=false;unsigned mapMismatch=0,mapDifference=0,windowFrameMismatch=0;
 std::array<unsigned,12> borderMismatch{};
 int liveWord=-1,candidateWord=-1,liveStepped=-1,candidateStepped=-1;
};
void skullRaster(Fixture &f,SkullRaster &raster) {
 const auto draw=[&f](XeenWorld &world,const XeenPartyState &party,const XeenCamera &camera,
                      std::uint64_t phase,XeenMonsterAppearance appearance) {
  bool animated=false;
  auto image=CloudsMapComposer().compose(f.in.assets,world,party,camera,
   {party.encounterContext?unsigned(party.encounterContext->year):0u},nullptr,phase,&animated,appearance);
  return XeenEventFlow::Composition{std::move(image),animated};
 };
 XeenEventDispatchTestAccess::composer(*f.flow,[&raster,draw](auto &world,const auto &party,const auto &camera,auto phase,auto appearance) {
  auto result=draw(world,party,camera,phase,appearance);raster.candidate=result.frame;return result;
 });
 XeenEventDispatchTestAccess::liveComposer(*f.flow,[&f,&raster,draw](auto phase,auto appearance) {
  auto result=draw(f.w,f.p,f.c,phase,appearance);raster.live=result.frame;return result;
 });
}
void inspectSkullWait(Fixture &f,SkullRaster &raster,SkullObservation &seen) {
 if(seen.seen)return;
 const auto image=f.flow->refresh(true);f.present(image);
 check(image.isValid() && raster.live.isValid() && raster.candidate.isValid(),"Q01 scene images are missing");
 const auto candidate=XeenEventDispatchTestAccess::candidateWithDialogs(*f.flow,raster.candidate);
 for(unsigned y=9;y<75;++y)for(unsigned x=224;x<317;++x) {
  const auto i=y*320+x;seen.mapMismatch+=image.pixels[i]!=candidate.pixels[i];
  seen.mapDifference+=raster.live.pixels[i]!=candidate.pixels[i];
 }
 // Bounding rectangles for the original border sprites. Exclude the already
 // copied viewport; the presenter may cover an element with a later dialog.
 struct Region {unsigned x1,y1,x2,y2;};
 constexpr Region elements[]={{0,82,8,120},{0,32,8,55},{224,32,232,55},
  {2,2,13,13},{219,2,230,13},{2,134,13,145},{219,134,230,145},
  {33,137,55,149},{55,137,77,149},{160,137,182,149},{182,137,203,149},
  {109,139,124,149}};
 for(unsigned e=0;e<std::size(elements);++e)for(unsigned y=elements[e].y1;y<elements[e].y2;++y)
  for(unsigned x=elements[e].x1;x<elements[e].x2;++x)if(x<8 || x>=224 || y<8 || y>=140) {
   const auto i=y*320+x;seen.borderMismatch[e]+=image.pixels[i]!=candidate.pixels[i];
  }
 // DOS also redraws window 12's frame when the bottom Event window is open.
 // Compare its perimeter after rebasing the same retained dialog layers.
 for(unsigned y=143;y<199;++y)for(unsigned x=0;x<320;++x)
  if(y<151 || y>=191 || x<8 || x>=312)
   seen.windowFrameMismatch+=image.pixels[y*320+x]!=candidate.pixels[y*320+x];
 const auto live=f.w.sampleCell(54,9,5);
 const auto pending=XeenEventDispatchTestAccess::candidateWorld(*f.flow).sampleCell(54,9,5);
 check(bool(live) && bool(pending),"Q01 changed cell missing");
 seen.liveWord=live->cell->rawWord;seen.candidateWord=pending->cell->rawWord;
 seen.liveStepped=live->cell->stepped;seen.candidateStepped=pending->cell->stepped;seen.seen=true;
}
void settleSkull(Fixture &f,const std::string &word,SkullRaster *raster=nullptr,
                 SkullObservation *observation=nullptr,unsigned line=0) {
 for(unsigned n=0;!f.flow->canSave();++n) {
  check(n<1800,"Q01 Event did not settle");
  const auto request=XeenEventDispatchTestAccess::request(*f.flow);
  if(request && observation && request->source.line==line)inspectSkullWait(f,*raster,*observation);
  const auto input=f.flow->inputContext(f.flow->frame().presentation());
  if(input.readyForAction && input.dialog) {
   if(input.dialog->anyKey)f.act(AcknowledgeAction{});
   else if(request && request->response==XeenPresentationResponseRequirement::YesNo)f.act(YesAction{});
   else if(request && request->response==XeenPresentationResponseRequirement::WordInput) {
    f.act(TextInputAction{word});f.act(DialogKeyAction{InputKey::Enter});
   }
  }
  f.now+=25;f.flow->beginCycle(++f.cycle);if(auto next=f.flow->updatePresentation())f.present(*next);
 }
}
void skullCase(Inputs &in,const std::string &which) {
 const bool eye=which=="eye",explored=which=="explored";
 check(eye || explored || which=="fresh","Unknown Q01 case");
 auto source=at(in,54,10,6,XeenDirection::East);
 source.journey->context->effects[XeenPartyEffect::WizardEye]=eye?1:0;
 source.journey->context->effects[XeenPartyEffect::AutomapOn]=eye?0:1;
 Fixture f(in,source);SkullRaster raster;skullRaster(f,raster);f.present(f.flow->refresh(true));
 check(xeenPartyHasSkill(f.p,4),"Q01 party needs Cartography");
 const auto original=f.w.sampleCell(54,9,5)->cell->rawWord;
 const auto gems=f.p.purse->gems;
 SkullObservation observation;
 f.act(InteractionAction{});settleSkull(f,"Rosebud",explored?nullptr:&raster,explored?nullptr:&observation,19);
 const auto opened=f.w.sampleCell(54,9,5)->cell->rawWord;
 check(opened!=original && f.c.mapId==XeenMapIdentity(54) && f.p.purse->gems+1==gems,
       "Rosebud did not publish the opened passage and gem payment");
 if(explored) {
  const auto move=[&](NavigationAction action){f.now+=125;f.act(action);settleSkull(f,"wrong");};
  move(NavigationAction::TurnRight);move(NavigationAction::TurnRight);move(NavigationAction::MoveForward);
  move(NavigationAction::TurnRight);move(NavigationAction::TurnRight);move(NavigationAction::TurnRight);
  move(NavigationAction::MoveForward);
  check(f.c.x==9 && f.c.y==5 && f.w.sampleCell(54,9,5)->cell->stepped,"Q01 passage was not explored by movement");
  move(NavigationAction::TurnRight);move(NavigationAction::TurnRight);move(NavigationAction::MoveForward);
  move(NavigationAction::TurnRight);move(NavigationAction::MoveForward);
  check(f.c.x==10 && f.c.y==6 && f.c.direction==XeenDirection::East,"Q01 return route missed the skull");
 }
 if(explored) {
  f.act(InteractionAction{});settleSkull(f,"wrong",&raster,&observation,10);
  check(f.c.mapId==XeenMapIdentity(51) && f.c.x==7 && f.c.y==9 && f.w.sampleCell(54,9,5)->cell->rawWord==original,
   "Wrong word did not publish the closed passage and teleport");
 }
 check(observation.seen,"Q01 expected confirmation wait was not presented");
 check(!explored ? (!observation.liveStepped && !observation.candidateStepped && observation.liveWord==original && observation.candidateWord==opened) :
       (observation.liveStepped && observation.candidateStepped && observation.liveWord==opened && observation.candidateWord==original),
       "Q01 wait has the wrong wall or exploration state");
 if(eye || explored)check(observation.mapDifference>0,"Q01 witness did not expose a changed map pixel");
 else check(observation.mapDifference==0,"Q01 fresh control unexpectedly exposed the changed tile");
 for(unsigned e=0;e<observation.borderMismatch.size();++e)
  check(observation.borderMismatch[e]==0,("Q01 "+which+" border element "+std::to_string(e)+" differs from candidate").c_str());
 check(observation.windowFrameMismatch==0,("Q01 "+which+" bottom window frame differs from candidate").c_str());
 check(observation.mapMismatch==0,("Q01 "+which+" small map differs from candidate at "+std::to_string(observation.mapMismatch)+" pixels").c_str());
 std::cout<<"Q01 "<<which<<" passed (live/candidate map pixels "<<observation.mapDifference<<")\n";
}
// The skull does not change resistances. A synthetic Event on original city
// data makes the border test distinguish candidate-only state from live state.
void pendingBorder(Inputs &in) {
 auto source=at(in,28,20,20,XeenDirection::North);
 for(unsigned i=2;i<6;++i)source.journey->context->lightAndResistances[i]=0;
 const auto line=[](unsigned n,unsigned op,std::vector<std::uint8_t> args) {
  XeenEventRecord r;r.x=20;r.y=20;r.direction=4;r.line=n;r.opcode=op;r.parameters=std::move(args);r.lengthField=5+r.parameters.size();return r;
 };
 XeenEventFile script{28,"maze0028.evt",true,{line(0,0x0c,{0,0,74,7}),line(1,0x09,{44,1,2}),line(2,0x12,{})}};
 Fixture f(in,source,false,&script);SkullRaster raster;skullRaster(f,raster);f.present(f.flow->refresh(true));
 f.act(InteractionAction{});
 const auto request=XeenEventDispatchTestAccess::request(*f.flow);
 check(request && request->kind==XeenPresentationKind::Confirmation,"Candidate border Event did not reach its wait");
 const auto &pending=XeenEventDispatchTestAccess::candidateParty(*f.flow);
 check(!f.p.encounterContext->lightAndResistances[2] && pending.encounterContext->lightAndResistances[2],
       "Candidate border Event published resistance before its wait");
 const auto shown=f.flow->refresh(true);f.present(shown);
 check(shown.isValid() && raster.live.isValid() && raster.candidate.isValid(),"Candidate border images are missing");
 const auto live=XeenEventDispatchTestAccess::candidateWithDialogs(*f.flow,raster.live);
 const auto candidate=XeenEventDispatchTestAccess::candidateWithDialogs(*f.flow,raster.candidate);
 unsigned different=0,wrong=0;
 for(unsigned y=2;y<13;++y)for(unsigned x=2;x<13;++x)if(x<8 || y<8) {
  const auto i=y*320+x;different+=live.pixels[i]!=candidate.pixels[i];wrong+=shown.pixels[i]!=candidate.pixels[i];
 }
 check(different>0 && wrong==0,"Candidate-only Fire resistance was not drawn outside the viewport");
 f.act(AcknowledgeAction{});finish(f,true);
 check(f.flow->canSave() && f.p.encounterContext->lightAndResistances[2],"Candidate border Event did not publish on completion");
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
   check(XeenEventDispatchTestAccess::candidateLight(*f.flow)==0 && f.p.encounterContext->lightAndResistances[0]==1,
    "Scene check drains only the detached Event light");seen=true;
  }
  return XeenEventFlow::Composition{frame(),false};
 });
 f.act(InteractionAction{});check(seen && f.flow->canCancelInteraction(),"Dark Event did not reach its pending presentation");
 const auto context=*XeenEventDispatchTestAccess::candidateParty(*f.flow).encounterContext;
 const auto explored=XeenEventDispatchTestAccess::candidateWorld(*f.flow).sessionState().exploredTiles();
 for(bool reconstruct:{true,false}) {
  f.present(f.flow->refresh(reconstruct));
  check(*XeenEventDispatchTestAccess::candidateParty(*f.flow).encounterContext==context &&
    XeenEventDispatchTestAccess::candidateWorld(*f.flow).sessionState().exploredTiles()==explored,
    "Reconstruction or second Event wait changed the consumed step");
 }
 f.act(SelectMemberAction{0});finish(f,true);
}
void locationArtPendingScene(Inputs &in) {
 for(bool bottom:{false,true}) {
 auto source=at(in,28,20,20,XeenDirection::North);save_test::clearExplored(source);
 source.characters[source.activeRosterIds.front()].skills[4]=1;source.soundOptions.text=false;
 source.journey->context->lightAndResistances[0]=3;
 XeenWorld geometry(in.mapLoader());const auto cell=geometry.sampleCell(28,20,20);
 for(unsigned d=0;d<4;++d){const auto wall=wallAt(*cell->cell,XeenDirection(d));
  if(wall!=1 && wall!=6 && wall!=9 && wall!=13){source.camera.direction=XeenDirection(d);break;}}
 const auto line=[](unsigned n,unsigned op,std::vector<std::uint8_t> args){
  XeenEventRecord r;r.x=20;r.y=20;r.direction=4;r.line=n;r.opcode=op;r.parameters=std::move(args);r.lengthField=5+r.parameters.size();return r;};
 // Original Vertigo NPC resources, followed by an isolated Yes/No prompt.
 XeenEventFile script{28,"maze0028.evt",true,{line(0,5,{8,9,23,2}),
  bottom?line(1,0x31,{0,9}):line(1,9,{44,0,2}),line(2,0x12,{})}};
 const auto maps=in.mapLoader();
 Fixture f(in,source,false,&script,[maps](auto id){auto map=maps(id);for(auto &c:map.geometry.cells)c.rawAttributes|=0x20;return map;},
  {},[&](auto &image,auto portrait,auto phase){in.assets.drawNpc(image,portrait,phase);});
 f.w.scenePresentation().markLightingStep();f.act(InteractionAction{});
 const auto request=XeenEventDispatchTestAccess::request(*f.flow);
 check(request && request->kind==(bottom?XeenPresentationKind::BottomWindowTwoLines:XeenPresentationKind::Confirmation) &&
  request->response==(bottom?XeenPresentationResponseRequirement::Acknowledgment:XeenPresentationResponseRequirement::YesNo),
  "Location artwork must remain under the separate prompt");
 check(f.flow->presenter().hasNpcOffer(),"Opening a prompt must retain mode-2 NPC art");
 auto &candidate=XeenEventDispatchTestAccess::candidateWorld(*f.flow);
 check(candidate.scenePresentation().lightingStepped && !candidate.sampleCell(28,20,20)->cell->stepped &&
  XeenEventDispatchTestAccess::candidateLight(*f.flow)==3,"Yes/No over location art keeps step, exploration and light");
 f.present(f.flow->refresh(true));
 check(candidate.scenePresentation().lightingStepped && XeenEventDispatchTestAccess::candidateLight(*f.flow)==3,
  "Location-art reconstruction keeps the pending step");
 finish(f,true);
 check(!f.flow->presenter().hasNpcOffer(),"Event cleanup must release mode-2 NPC art");
 }
}
void damagePendingScene(Inputs &in,bool refused=false,bool automatic=false) {
 auto source=at(in,28,20,20,XeenDirection::North);save_test::clearExplored(source);
 source.characters[source.activeRosterIds.front()].skills[4]=1;
 source.journey->context->lightAndResistances[0]=3;
 XeenWorld geometry(in.mapLoader());const auto cell=geometry.sampleCell(28,20,20);
 for(unsigned d=0;d<4;++d) {const auto wall=wallAt(*cell->cell,XeenDirection(d));
  if(wall!=1 && wall!=6 && wall!=9 && wall!=13){source.camera.direction=XeenDirection(d);break;}}
 XeenEventRecord damage;damage.x=20;damage.y=20;damage.direction=4;damage.opcode=0x16;
 damage.parameters={1,0,0};damage.lengthField=8;
 XeenEventFile script{28,"maze0028.evt",true,{damage}};
 if(refused) {
  auto repeated=damage;repeated.line=1;script.records.push_back(repeated);
  auto unsupported=damage;unsupported.line=2;unsupported.opcode=0x21;unsupported.parameters={0,0};unsupported.lengthField=7;
  script.records.push_back(unsupported);
 }
 const auto maps=in.mapLoader();
 Fixture f(in,source,false,&script,[maps,automatic](auto id){auto map=maps(id);
  for(auto &c:map.geometry.cells)c.rawAttributes|=0x20 | (automatic?kXeenAutomaticEventFlag:0);return map;});
 bool sawRefusal=false;
 f.flow->reportManual=[&](const auto &result) {
  if(std::holds_alternative<XeenEventExecutionError>(result)) {
   sawRefusal=true;
  }
 };
 f.w.scenePresentation().markLightingStep();
 if(automatic)f.present(f.flow->completeExplorationInput());else f.act(InteractionAction{!refused});
 check(XeenEventDispatchTestAccess::damage(*f.flow),"Damage must reach its portrait wait");
 auto &candidate=XeenEventDispatchTestAccess::candidateWorld(*f.flow);
 check(candidate.sampleCell(28,20,20)->cell->stepped &&
   XeenEventDispatchTestAccess::candidateLight(*f.flow)==2,"Damage pre-draw marks and drains once");
 check(!f.w.sampleCell(28,20,20)->cell->stepped && f.p.encounterContext->lightAndResistances[0]==3,
   "Damage pre-draw remains detached until Event publication");
 bool held=false;
 for(unsigned n=0;n<100 && !held;++n) {
  const auto request=XeenEventDispatchTestAccess::request(*f.flow);
  held=request && request->portrait && request->portrait->stage==XeenEventPortraitStage::DamageHold;
  if(!held){f.now+=100;f.flow->beginCycle(++f.cycle);if(auto next=f.flow->updatePresentation())f.present(*next);}
 }
 check(held && XeenEventDispatchTestAccess::candidateLight(*f.flow)==2,
   "Damage overlays and final hold must not consume the pending step twice");
 finish(f,true);
 check(!refused || sawRefusal,"The invalid damage operand must refuse after the portrait wait");
 check(f.w.sampleCell(28,20,20)->cell->stepped && f.p.encounterContext->lightAndResistances[0]==(refused?2:1),
   refused?"Repeated prefixes exclude the discarded drain; refusal consumes only the live pending step":
   "Space's separate doStep after the Event drains its own new step");
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
void cartographerEvents(Inputs &in) {
 const auto blank=[&](unsigned map,int x,int y) {
  auto s=at(in,map,x,y,XeenDirection::North);save_test::clearExplored(s);
  s.characters[s.activeRosterIds.front()].skills[4]=1;return s;
 };
 // Rivercity's boat exit clears both travel flags and teleports on the same map.
 {
  auto s=blank(30,17,28);s.gameFlags[221]=true;
  Fixture f(in,s);f.w.scenePresentation().markLightingStep();
  f.act(InteractionAction{});finish(f,true);
  check(f.c.mapId==XeenMapIdentity(30) && f.c.x==17 && f.c.y==26,"Original same-map boat exit destination");
  check(f.w.sampleCell(30,17,26)->cell->stepped && !f.w.sampleCell(30,17,28)->cell->stepped,
    "Silent same-map teleport marks only its destination");
 }
 // The original boat chain checks the consecutive map-11 stops before map 15.
 {
  auto s=blank(11,13,7);s.gameFlags[220]=true;
  Fixture f(in,s);f.w.scenePresentation().markLightingStep();
  f.act(InteractionAction{});finish(f,true);
  check(f.w.sampleCell(11,14,7)->cell->stepped && f.w.sampleCell(11,15,7)->cell->stepped,
    "Original Continue chain marks intermediate physical cells");
 }
 // Rivercity's boat NPC offers Yes/No over its artwork, then starts the chain.
 for(bool text:{false,true}) {
  auto s=blank(30,16,23);s.soundOptions.text=text;s.journey->purse->gold=10000;
  Fixture f(in,s,false,nullptr,{}, {},[&](auto &image,auto portrait,auto phase){in.assets.drawNpc(image,portrait,phase);});
  f.w.scenePresentation().markLightingStep();f.act(InteractionAction{});
  const auto request=XeenEventDispatchTestAccess::request(*f.flow);
  check(request && request->kind==XeenPresentationKind::NpcAcknowledgment,"Original boat NPC offer absent");
  const auto candidateMarked=[&]{return XeenEventDispatchTestAccess::candidateWorld(*f.flow).sampleCell(30,16,23)->cell->stepped;};
  check(candidateMarked()==!text && !f.w.sampleCell(30,16,23)->cell->stepped,
    "NPC Text gate must change only the detached source mark");
  f.present(f.flow->refresh(true));
  check(candidateMarked()==!text,"NPC reconstruction consumed a blocked pending step");
  bool answered=false;
  for(unsigned n=0;n<1000 && !answered;++n) {
   f.now+=100;const auto input=f.flow->inputContext(f.flow->frame().presentation());
   if(input.readyForAction && input.dialog && input.dialog->key('y')){f.act(YesAction{});answered=true;}
   else {f.flow->beginCycle(++f.cycle);if(auto frame=f.flow->updatePresentation())f.present(*frame);}
  }
  check(answered,"Original boat NPC never admitted Yes");finish(f,true);
  check(f.w.sampleCell(30,16,23)->cell->stepped==!text && f.w.sampleCell(30,16,24)->cell->stepped,
    "Text-on NPC keeps source blank; Text-off NPC and Continue mark both cells");
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
 check(argc==2 || argc==3,"original dispatch test requires game directory");auto installation=xeenTestInstallationDetector().detect(argv[1]);check(bool(installation),"original installation missing");
 Inputs in(*installation);
 if(argc==3) {if(std::string(argv[2])=="border")pendingBorder(in);else skullCase(in,argv[2]);return 0;}
 locationArtPendingScene(in);damagePendingScene(in);damagePendingScene(in,true);damagePendingScene(in,true,true);
 cartographerEvents(in);skullCase(in,"fresh");skullCase(in,"explored");skullCase(in,"eye");pendingBorder(in);
 trap(in);rubble(in);for(unsigned variant=0;variant<3;++variant)rubbleFlow(in,variant);trapScene(in,55,7,8,5,87);trapScene(in,33,3,15,0,89);manualRefusalScene(in);outdoorScene(in);darkPendingScene(in);label(in);guardedNpc(in);combatLabels(in);combatTrap(in);std::cout<<"M84 original-data Event passes OK\n";return 0;
}catch(const std::exception &e){std::cerr<<e.what()<<'\n';return 1;}}



