#include "XeenTrainingTestSupport.h"
#include "XeenTestInstallation.h"
#include "games/xeen/XeenOutdoorScene.h"
#include "games/xeen/XeenRestoreGuard.h"
#include "games/xeen/XeenItemRewards.h"
#include "games/xeen/CloudsMapComposer.h"
#include "games/xeen/CloudsUiComposer.h"
#include "app/Application.h"
#include "app/XeenGameplayServices.h"
#define SDL_MAIN_HANDLED
#include <SDL.h>
#include <iostream>
using namespace training_test;
namespace mmodern {
struct XeenMonsterTestAccess {
 static std::optional<XeenPresentationRequest> request(const XeenEventFlow &f) {
  return f._pending && f._pending->state.pendingPresentation ?
   std::optional<XeenPresentationRequest>(f._pending->state.pendingPresentation->request) : std::nullopt;
 }
 static bool barrierSelection(const XeenEventFlow &f){return f._barrier && f._barrier->rule && f._barrier->rule->selection;}
 static bool fatal(const XeenEventFlow &f){return f._fatal || f._death;}
 static bool infoOpen(const XeenEventFlow &f){return f._summary && *f._summary==XeenEventFlow::SummaryDialog::Info;}
 static std::array<std::uint64_t,4> reward(const XeenEventFlow &f) {
  if(!f._pending)return {};const auto &s=f._pending->state;
  return {s.reward.receipt.count,s.reward.receipt.lost,s.treasure.gold,s.treasure.gems};
 }
 static void returnTo(Fixture &f,XeenCamera destination) {
  auto &flow=*f.flow;auto &encounter=*flow._encounter;
  check(encounter.journeyQuiet(),"I5 map load requires Quiet");
  encounter.journeySavePreimage().check();auto candidate=f.w.transitionCandidate();
  XeenPartyState party;candidate->copyEventParty(f.p,party);
  const auto flags=candidate->loadEventDestination(f.c.mapId,destination.mapId,true,f.f);
  auto events=f.in.events.load(destination.mapId);const auto arrival=candidate->prepareTransitionArrival(destination);
  auto prepared=std::make_shared<XeenRestoreGuard>(f.w,f.p,f.c,f.f);
  prepared->retainResources(encounter.journeySavePreimage());prepared->prepareSpellMapPublication(*candidate);
  prepared->cameraValue=destination;prepared->flagValues=flags.values();
  encounter.journeySavePreimage().check();f.w.checkTransitionTreasure(*candidate);
  f.w.publishTransition(*candidate);f.c=destination;f.f=flags;encounter._journeyEvents=std::move(events);
  encounter._journeyPreimage.swap(prepared);++encounter._generation;encounter._journeyPreimage->adoptMutationBoundary();
  encounter.publishArrival(arrival);encounter.holdJourneyFrame();flow._encounterFrame=encounter.ticket();f.present(flow.refresh(true));
 }
 static void wound(Fixture &f,unsigned index) {
  auto &encounter=*f.flow->_encounter;auto &actor=encounter._world._sessionState.mutableMonsters(23).at(index);
  actor.hp=1;actor.activated=true;actor.status=XeenActorStatus::Sleep;
  encounter.retainJourney();f.flow->_encounterFrame=encounter.ticket();
 }
 static void scenes(Fixture &f,const XeenObjectVisualResolver &resolver) {
  auto draw=[&resolver](XeenWorld &w,const XeenPartyState &,const XeenCamera &c,std::uint64_t,XeenMonsterAppearance) {
   if(w.map(c.mapId).geometry.isOutdoors())XeenOutdoorScene().build(w,c,&resolver,nullptr,0);
   else XeenIndoorScene().build(w,c,&resolver,nullptr,0);
   return XeenEventFlow::Composition{frame(),false};
  };
  f.flow->_transitionCompose=draw;
  f.flow->_encounterCompose=[&f,draw](auto phase,auto appearance){return draw(f.w,f.p,f.c,phase,appearance);};
 }
};
}
namespace {
struct Visit:Fixture {
 unsigned barrierSelections=0;
 std::string word;
 bool rewardWarning=false,rewardReceipt=false;std::uint64_t receiptLost=0;
 Visit(Inputs &in,const XeenSaveSnapshot &source,const XeenObjectVisualResolver &resolver):
  Fixture(in,source,false,nullptr,{}, {},[&in](auto &f,auto id,auto n){in.assets.drawNpc(f,id,n);}) {
  XeenMonsterTestAccess::scenes(*this,resolver);
 }
 void pulse(){now+=25;flow->beginCycle(++cycle);if(auto image=flow->updatePresentation())present(*image);}
 std::string settle(unsigned limit=1800) {
  for(unsigned n=0;n<limit;++n) {
   if(flow->canSave())return "quiet";
   if(XeenMonsterTestAccess::fatal(*flow))return "fatal";
   if(flow->encounter()->combat())return "combat";
   const auto request=XeenMonsterTestAccess::request(*flow);
   if(request && request->kind==XeenPresentationKind::RewardWarning)rewardWarning=true;
   if(request && request->kind==XeenPresentationKind::RewardReceipt){rewardReceipt=true;receiptLost=XeenMonsterTestAccess::reward(*flow)[1];}
   if(request && request->npc)now+=100;
   const auto input=flow->inputContext(flow->frame().presentation());
   if(input.readyForAction && input.dialog) {
    if(input.dialog->anyKey)act(AcknowledgeAction{});
    else if(!request && XeenMonsterTestAccess::barrierSelection(*flow)) {
     for(unsigned i=0;i<p.party.size();++i)if(p.party.member(p.roster,i).canAct()){act(SelectMemberAction{i});++barrierSelections;break;}
    } else if(request && request->response==XeenPresentationResponseRequirement::CharacterSelection) {
     const auto i=std::find_if(request->members.begin(),request->members.end(),[](auto &m){return m.eligible;});
     if(i!=request->members.end())act(SelectMemberAction{i->partyIndex});else act(CancelInteractionAction{});
    } else if(request && request->response==XeenPresentationResponseRequirement::YesNo)act(YesAction{});
    else if(request && request->response==XeenPresentationResponseRequirement::WordInput && !word.empty()) {
     act(TextInputAction{word});act(DialogKeyAction{InputKey::Enter});
    }
   }
   pulse();
  }
  return "cap";
 }
 std::string combatExit(bool attack=false) {
  for(unsigned n=0;n<5000;++n) {
   if(XeenMonsterTestAccess::fatal(*flow))return "fatal";
   auto *combat=flow->encounter()->combat();
   if(combat){if(combat->phase()==XeenCombatPhase::PlayerReady){if(attack)act(AttackAction{});else act(RunAction{});}pulse();}
   else {const auto state=settle(600);if(state!="combat")return state;}
  }
  return "cap";
 }
 std::string interact(){now+=125;act(InteractionAction{});auto state=settle();if(state=="combat")state=combatExit();return state;}
};
std::vector<std::uint64_t> rewards(const Visit &v) {
 std::vector<std::uint64_t> result{v.p.purse->gold,v.p.purse->gems,v.p.food};
 for(auto n:v.p.questItems.counts())result.push_back(n);
 for(auto owner:v.p.party.activeRosterIds()) {
  const auto &c=v.p.roster.at(owner);result.push_back(v.p.roster.combatInputs(owner)->experience);
  const auto append=[&](const auto &items){for(const auto &item:items){result.push_back(item.material);result.push_back(item.id);result.push_back(item.state);result.push_back(item.frame);}};
  append(c.weapons);append(c.armor);append(c.accessories);append(c.miscellaneous);
 }
 return result;
}
std::vector<std::uint64_t> inventory(Visit &v) {
 std::vector<std::uint64_t> result;
 for(auto owner:v.p.party.activeRosterIds())for(unsigned category=0;category<4;++category)
  for(const auto &item:xeenRewardItems(v.p.roster.at(owner),static_cast<XeenRewardCategory>(category))) {
   result.push_back(item.material);result.push_back(item.id);result.push_back(item.state);result.push_back(item.frame);
  }
 return result;
}
void secondUse(Inputs &in) {
 const auto visuals=XeenObjectVisualResolver::load(in.assets);
 for(const auto camera:std::vector<XeenCamera>{{1,0,15,XeenDirection::North},{3,8,14,XeenDirection::North}}) {
  auto source=in.base();source.camera=camera;Visit v(in,source,visuals);
  const auto before=rewards(v);check(v.interact()=="quiet","I1 first Event did not finish");
  const auto after=rewards(v);const auto flags=v.f.values();
  if(camera.mapId.number==1)check(v.f.isSet(35),"I1 map 1 did not grant its flag");
  else check(after!=before,"I1 map 3 did not deliver its reward");
  check(v.interact()=="quiet" && rewards(v)==after && v.f.values()==flags,"I1 repeated Event paid again");
 }
 auto source=in.base();source.camera={33,2,30,XeenDirection::North};Visit v(in,source,visuals);
 const auto initialGold=v.p.purse->gold;
 const auto file=in.events.load(33);
 const auto disabled=[&](){for(std::size_t i=0;i<file.records.size();++i){const auto &r=file.records[i];if(r.x==2 && r.y==30 && r.line==0 && (r.direction==0 || r.direction==4))return v.w.effectiveEvent({33,i},r).opcode==0;}return false;};
 for(unsigned n=0;n<3 && !disabled();++n)check(v.interact()=="quiet","I1 dig did not finish");
 check(disabled() && v.p.purse->gold>initialGold,"I1 dig did not succeed on its own random stream");
 const auto after=rewards(v);check(v.interact()=="quiet" && rewards(v)==after && disabled(),"I1 successful dig paid twice");
}
void exactRestore(Inputs &in) {
 const auto visuals=XeenObjectVisualResolver::load(in.assets);
 for(const auto camera:std::vector<XeenCamera>{{3,8,14,XeenDirection::North},{33,2,30,XeenDirection::North}}) {
  auto source=in.base();source.camera=camera;Visit live(in,source,visuals);
  if(camera.mapId.number==3)check(live.interact()=="quiet","I3 camp Event did not finish");
  else {
   const auto gold=live.p.purse->gold;
   for(unsigned n=0;n<3 && live.p.purse->gold==gold;++n)check(live.interact()=="quiet","I3 dig did not finish");
   check(live.p.purse->gold>gold,"I3 dig did not succeed on its own random stream");
  }
  const auto bytes=XeenSaveFormat::encode(live.snapshot());
  const auto rng=live.w.sessionState().journeyRandom();const auto skeleton=live.w.sessionState().skeletonSeed();
  Visit restored(in,XeenSaveFormat::decode(bytes),visuals);
  check(XeenSaveFormat::encode(restored.snapshot())==bytes && restored.w.sessionState().journeyRandom()==rng &&
   restored.w.sessionState().skeletonSeed()==skeleton,"I3 restore changed bytes, owners or RNG");
  check(live.interact()=="quiet" && restored.interact()=="quiet","I3 second Event did not finish");
  check(XeenSaveFormat::encode(live.snapshot())==XeenSaveFormat::encode(restored.snapshot()) &&
   live.w.sessionState().journeyRandom()==restored.w.sessionState().journeyRandom() &&
   live.w.sessionState().skeletonSeed()==restored.w.sessionState().skeletonSeed(),"I3 second Event differs after restore");
 }
}
void completion(Inputs &in) {
 const auto visuals=XeenObjectVisualResolver::load(in.assets);
 auto source=in.base();source.camera={41,26,11,XeenDirection::North};Visit v(in,source,visuals);
 check(v.interact()=="quiet" && v.barrierSelections==1,"I2 barrier selection did not complete");
}
void departureReturn(Inputs &in) {
 const auto visuals=XeenObjectVisualResolver::load(in.assets);
 auto source=in.base();source.camera={33,2,30,XeenDirection::North};Visit dig(in,source,visuals);
 const auto gold=dig.p.purse->gold;for(unsigned n=0;n<3 && dig.p.purse->gold==gold;++n)check(dig.interact()=="quiet","I5 dig did not finish");
 check(dig.p.purse->gold>gold,"I5 dig did not succeed on its own random stream");
 const auto file=in.events.load(33);
 const auto disabled=[&](){for(std::size_t i=0;i<file.records.size();++i){const auto &r=file.records[i];if(r.x==2 && r.y==30 && r.line==0 && (r.direction==0 || r.direction==4))return dig.w.effectiveEvent({33,i},r).opcode==0;}return false;};
 check(disabled(),"I5 dig Event was not disabled");
 XeenMonsterTestAccess::returnTo(dig,{28,18,4,XeenDirection::West});check(dig.settle()=="quiet","I5 departure not Quiet");
 XeenMonsterTestAccess::returnTo(dig,{33,2,30,XeenDirection::North});check(dig.settle()=="quiet" && disabled(),"I5 dig Event did not survive return");
 auto initial=in.base();Visit actor(in,initial,visuals);const auto original=actor.c;
 const auto &before=actor.w.sessionState().regionalActors(23).at(7);
 check(before.lifecycle==XeenActorLifecycle::Present && bool(before.statistics) && before.statistics->baseHp()>1,"I5 actor fixture is unsuitable");
 XeenMonsterTestAccess::wound(actor,7);
 XeenMonsterTestAccess::returnTo(actor,{28,18,4,XeenDirection::West});check(actor.settle()=="quiet","I5 actor departure not Quiet");
 XeenMonsterTestAccess::returnTo(actor,original);check(actor.settle()=="quiet","I5 actor return not Quiet");
 const auto &after=actor.w.sessionState().regionalActors(23).at(7);
 check(after.lifecycle==XeenActorLifecycle::Present && after.statistics && after.hp==after.statistics->baseHp() &&
  !after.activated && after.status==XeenActorStatus::Physical,"I5 bound actor did not reload full, asleep and Physical");
 auto skullSource=in.base();skullSource.camera={54,10,6,XeenDirection::East};Visit skull(in,skullSource,visuals);
 const auto oldWord=skull.w.sampleCell(54,9,5)->cell->rawWord;skull.word="Rosebud";
 check(skull.interact()=="quiet","I5 skull Event did not finish");
 const auto changedWord=skull.w.sampleCell(54,9,5)->cell->rawWord;
 check(changedWord!=oldWord,"I5 skull Event did not alter the wall");
 XeenMonsterTestAccess::returnTo(skull,{28,18,4,XeenDirection::West});check(skull.settle()=="quiet","I5 wall departure not Quiet");
 XeenMonsterTestAccess::returnTo(skull,{54,10,6,XeenDirection::East});
 check(skull.settle()=="quiet" && skull.w.sampleCell(54,9,5)->cell->rawWord==changedWord,"I5 Event wall did not survive return");
}
void interruptedEvent(Inputs &in) {
 const auto visuals=XeenObjectVisualResolver::load(in.assets);auto source=in.base();source.camera={28,13,4,XeenDirection::East};Visit v(in,source,visuals);
 unsigned before=0,after=0,completedAfter=0;bool engaged=false;std::string error;
 v.flow->reportManual=[&](const auto &r){
  if(const auto *bad=std::get_if<XeenEventExecutionError>(&r))error=bad->message;
  if(const auto *pending=std::get_if<XeenEventExecutionSuspended>(&r))if(pending->request.source.opcode==2){if(engaged)++after;else ++before;}
  if(engaged && std::holds_alternative<XeenManualEventCompleted>(r))++completedAfter;
 };
 v.flow->reportAutomatic=[&](const auto &r){if(const auto *bad=std::get_if<XeenEventExecutionError>(&r))error=bad->message;};
 v.now+=125;v.act(NavigationAction::TurnLeft);const auto first=v.settle();
 if(first=="quiet"){v.act(NavigationAction::TurnLeft);v.settle();}
 for(unsigned n=0;n<500 && !v.flow->encounter()->combat();++n) {
  if(v.flow->canSave()){v.act(WaitAction{});if(v.settle()=="fatal")break;}else v.pulse();
 }
 const bool fought=bool(v.flow->encounter()->combat()),skipped=v.programState.eventCallSkipped;
 check(before>0 && fought && skipped,"B1 actor did not interrupt the Event");
 engaged=true;const auto outcome=v.combatExit(true);
 check(outcome=="quiet" && error.empty() && v.w.sessionState().regionalActors(28).at(35).lifecycle==XeenActorLifecycle::Defeated &&
  after>0 && completedAfter>0 && !v.programState.eventCallSkipped,"B1 Event did not resume after combat");
}
// Exercise the native SDL queue on original frames. No outcomes or timing
// decisions are injected into the Event, projectile or encounter owners.
void queuedInput(Inputs &in,unsigned mode) {
 const bool projectile=mode==2,mouse=mode==1;
 auto source=in.base();source.camera=projectile?XeenCamera{28,18,4,XeenDirection::West}:XeenCamera{33,3,15,XeenDirection::North};
 if(projectile) {
  unsigned shooters=0;for(auto id:source.activeRosterIds)for(const auto &item:source.characters[id].weapons)shooters+=item.frame==4;
  check(shooters>0,"B2 original party has no projectile weapon");
 }
 XeenProgramState program{XeenWallClock::fixed(0)};XeenEventFlow *flow=nullptr;std::uint64_t now=0;
 unsigned loops=0,shots=0,infos=0;bool started=false,injected=false,queueable=false,ready=false,retained=false,during=false,opened=false,quitSent=false;
 XeenGameplayServices services{program,in.resources(),[]{return XeenGameFlags{};},in.mapLoader(),in.objectLoader(),
  [&](auto id){return in.texts.load(id);},in.font,[](auto &,const auto &,const auto &,auto){return XeenEventFlow::Composition{frame(),false};},{},
  [&](auto &f,const auto &){flow=&f;f.dialogText=&in.assets.uiText();
   f.drawDialogSprite=[&](auto &image,const char *name,unsigned id,int x,int y){in.assets.drawDialogSprite(image,name,id,x,y);};
   f.drawCombatButtons=[&](auto &image){CloudsUiComposer().drawCombatButtons(in.assets,image);};},{},{}};
 services.restoreSnapshot=std::make_shared<const XeenSaveSnapshot>(source);
 services.clock=[&]{return now;};
 services.composeEncounter=[&](auto &world,const auto &party,const auto &camera,auto phase,auto appearance){
  bool animated=false;auto image=CloudsMapComposer().compose(in.assets,world,party,camera,
   {party.encounterContext?unsigned(party.encounterContext->year):0u},nullptr,phase,&animated,appearance);
  return XeenEventFlow::Composition{std::move(image),animated};
 };
 services.validateEncounterSprite=[&](auto image){in.assets.validateNormalMonster(image);};
 services.validateCombatSprite=[&](auto image){in.assets.validateAttackMonster(image);};
 const auto tap=[](SDL_Keycode code){SDL_Event e{};e.type=SDL_KEYDOWN;e.key.keysym.sym=code;e.key.keysym.scancode=SDL_GetScancodeFromKey(code);
  check(SDL_PushEvent(&e)==1,"B1/B2 key enqueue failed");e.type=SDL_KEYUP;check(SDL_PushEvent(&e)==1,"B1/B2 key-up enqueue failed");};
 const auto click=[](){SDL_Event e{};e.type=SDL_MOUSEBUTTONDOWN;e.button.button=SDL_BUTTON_LEFT;e.button.x=261;e.button.y=118;
  check(SDL_PushEvent(&e)==1,"B1 click enqueue failed");e.type=SDL_MOUSEBUTTONUP;check(SDL_PushEvent(&e)==1,"B1 click-up enqueue failed");};
 const auto quit=[&](){if(!quitSent){SDL_Event e{};e.type=SDL_QUIT;SDL_PushEvent(&e);quitSent=true;}};
 services.show=[&](const auto &first,const auto &handler,const auto &escape,const auto &idle,const auto &status){
  auto native=handler;
  native.beginCycle=[&](auto cycle){now+=25;handler.beginCycle(cycle);};
  native.framePresented=[&](const auto &presented){
   handler.framePresented(presented);if(!flow || injected)return;
   const auto request=XeenMonsterTestAccess::request(*flow);
   const bool atHold=request && request->kind==XeenPresentationKind::PortraitEffect;
   const bool atFlight=flow->encounter() && !flow->encounter()->appearance().projectiles.empty();
   if(projectile ? shots && atFlight : atHold) {
    const auto context=handler.inputContext(presented);queueable=context.acceptsQueuedInput;ready=context.readyForAction;retained=context.retainEventKeys;
    injected=true;if(mouse)click();else tap(SDLK_i);
   }
  };
  native.withPresentedInput=[&](const auto &action,auto input,const auto &origin){
   if(std::holds_alternative<ShootAction>(action))++shots;
   const bool info=std::holds_alternative<InfoAction>(action);
   if(info){++infos;const auto request=flow?XeenMonsterTestAccess::request(*flow):std::nullopt;
    during=projectile ? flow && flow->encounter() && !flow->encounter()->appearance().projectiles.empty() :
     request && request->kind==XeenPresentationKind::PortraitEffect;}
   auto result=handler.withPresentedInput(action,input,origin);
   if(info)opened=flow && XeenMonsterTestAccess::infoOpen(*flow);
   return result;
  };
  return SdlWindow().showInteractive(first,"Clouds queued input",native,escape,[&]()->std::optional<IndexedFrame>{
   if(++loops>4000){quit();return {};}
   if(!started && flow && flow->canSave() && handler.inputContext(flow->frame().presentation()).readyForAction){started=true;tap(projectile?SDLK_s:SDLK_SPACE);}
   if(infos)quit();return idle();
  },status);
 };
 const auto result=Application(program).playGameplay(services,source.camera,std::filesystem::path("m95b-in-memory-fixture"),true);
 check(result==0 && injected && queueable && !ready && (projectile || retained),"B1/B2 input was not queued during the hold or flight");
 check(infos==1 && !during && opened,"B1/B2 queued input did not open Info after the hold or flight");
 if(projectile)check(started && shots==1,"B2 original starting shot was not fired once");
}
void fullPacks(Inputs &in) {
 auto source=in.base();Fixture seed(in,source);XeenCombatRandom random(*seed.w.sessionState().journeyRandom());
 std::array<std::optional<XeenItem>,4> samples;
 for(unsigned n=0;n<10000 && std::any_of(samples.begin(),samples.end(),[](const auto &v){return !v;});++n) {
  XeenItemCandidate item(2);while(!item.complete()){XeenConsequenceDraw draw{random,64,{}};item.service(draw);}
  if(!samples[item.category()])samples[item.category()]=item.item();
 }
 check(std::all_of(samples.begin(),samples.end(),[](const auto &v){return bool(v);}),"I6 item fixture generation failed");
 for(auto owner:source.activeRosterIds)for(unsigned category=0;category<4;++category)
  for(auto &item:xeenRewardItems(source.characters[owner],static_cast<XeenRewardCategory>(category)))item=*samples[category];
 source.camera={12,1,11,XeenDirection::North};const auto visuals=XeenObjectVisualResolver::load(in.assets);Visit v(in,source,visuals);
 check(xeenPacksGloballyFull(v.p),"I6 packs were not full");const auto before=inventory(v);const auto gold=v.p.purse->gold,gems=v.p.purse->gems;
 check(v.interact()=="quiet" && v.rewardWarning && v.rewardReceipt && v.receiptLost>0 && inventory(v)==before &&
  v.p.purse->gold==gold+1200 && v.p.purse->gems==gems,"I6 full packs did not discard staged items and credit gold");
}
}
int main(int argc,char **argv)try {
 check(argc==3,"usage: clouds-invariants <installation> <case>");
 const auto install=xeenTestInstallationDetector().detect(argv[1]);check(bool(install),"Missing installation");Inputs in(*install);
 if(std::string(argv[2])=="i1")secondUse(in);else if(std::string(argv[2])=="i2")completion(in);else if(std::string(argv[2])=="i3")exactRestore(in);else if(std::string(argv[2])=="i5")departureReturn(in);else if(std::string(argv[2])=="i6")fullPacks(in);else if(std::string(argv[2])=="b1b2"){interruptedEvent(in);for(unsigned mode=0;mode<3;++mode)queuedInput(in,mode);}else throw std::runtime_error("Unknown invariant");
 std::cout<<"Clouds invariant "<<argv[2]<<" passed\n";return 0;
}catch(const std::exception &e){std::cerr<<e.what()<<'\n';return 1;}
