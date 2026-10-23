#include "XeenCombatGameplayTestSupport.h"
#include "XeenRegionalTestSupport.h"
#include "games/xeen/XeenStateEquality.h"
#define SDL_MAIN_HANDLED
#include <SDL.h>
#include <iostream>
#include <limits>
#include "XeenCosmeticHandoffTestSupport.h"
namespace mmodern {
struct XeenCityEventTestAccess {
 static const XeenPartyState &visible(const XeenEventFlow &f) {if(f._transition){f._transition->guard->check();return f._transition->party;}return f._party;}
 // Fault trigger only: the interpreter owns intentional in-progress writes;
 // checked stagedEffects below verifies the retained boundary after return.
 static unsigned pendingQuest(const XeenEventFlow &f) {return f._transition?f._transition->party.questItems.at(17):f._party.questItems.at(17);}
 static void discardCandidateMaps(XeenEventFlow &f) {if(!f._transition)throw std::logic_error("retained Event candidate absent");f._transition->world->discardMapCache();}
 static const XeenWorld &visibleWorld(const XeenEventFlow &f) {if(f._transition){f._transition->guard->check();return *f._transition->world;}return f._world;}
};
}
using namespace combat_gameplay_test;
namespace fs=std::filesystem;
namespace {
using Handler=SdlWindow::FrameUpdateHandler;
XeenGameplayServices services(Harness &h,unsigned shift=0) {
 auto s=h.services();
 s.maps=regional_test::map;
 s.objects=[](auto id){auto o=regional_test::objects(id);o.entities.objects.clear();
  for(unsigned i=0;i<13;++i)o.entities.objects.push_back({1,1,0,0,7});o.entities.objects.push_back({8,2,0,0,26});return o;};
 s.resources=regional_test::resources();s.resources.signature=h.signature;
 s.resources.loadEvents=[shift](auto id){auto e=regional_test::events(id);if(id==XeenMapIdentity(23)){
  const unsigned sites[]{125,126,130,131,132,133,127,128,129,134,135};
  const unsigned op[]{0x20,0x29,0x09,0x0c,0x0e,0x12,0x12,0x12,0x12,0x12,0x12};
  const std::vector<Bytes> args{{0,3},{0},{0x2c,1,3},{0,0,0x15,0x63},{},{},{},{},{},{},{}};
  for(unsigned i=0;i<11;++i){auto &v=e.records[sites[i]];v.x=8;v.y=2;v.direction=4;v.line=i;v.opcode=op[i];v.parameters=args[i];v.lengthField=5+v.parameters.size();}

 }
 if(id==XeenMapIdentity(23))e.records.insert(e.records.begin(),shift,XeenEventRecord{0,5,201,201,4,0,0x12,{}});
 return e;};
 s.texts=[](auto id){auto t=regional_test::texts(id);t.strings[0]="Synthetic discovery";t.strings[3]="Synthetic bones";return t;};
 s.resources.loadRegionalText=s.texts;
 return s;
}
void send(Harness &h,const Handler &handler,const PlayerAction &a,bool present=true) {
 handler.beginCycle(++h.cycle);check(handler.displayedInput().has_value(),"displayed input token");
 handler.withDisplayedInput(a,*handler.displayedInput());
 if(present){check(handler.frameCurrent(),"current response frame");handler.framePresented(h.flow->frame().presentation());}
}
XeenSaveSnapshot snapshot(Harness &h){return XeenSaveState::capture(h.signature,*h.party,*h.camera,*h.flags,*h.world);}
void effects(Harness &h,unsigned count,unsigned objects,unsigned events) {
 check(h.party->questItems.at(17)==count&&h.world->sessionState().disabledObjectCount()==objects&&h.world->sessionState().disabledEventCount()==events,"exact independently published objective effects");
}
void stagedEffects(Harness &h,unsigned count,unsigned objects,unsigned events) {
 const auto &party=XeenCityEventTestAccess::visible(*h.flow);const auto &world=XeenCityEventTestAccess::visibleWorld(*h.flow);
 check(party.questItems.at(17)==count&&world.sessionState().disabledObjectCount()==objects&&world.sessionState().disabledEventCount()==events,"exact checked staged objective effects");
}
// Part 0: treasure 0x2429e and final Enter leave no Space Wait.
void acknowledgment(Harness &h,const Handler &handler){send(h,handler,InteractionAction{});check(h.flow->canCancelInteraction(),"WhoWill reached");send(h,handler,SelectMemberAction{4});check(h.flow->presentationGeneration()&&!h.flow->canCancelInteraction()&&!h.flow->canSave(),"acknowledgment reached without grant");}
// Action 44 stages the quest grant and Remove before the treasure blank.
// Every receipt renders that checked candidate while the live state stays
// unchanged until the final current-key completion is admitted and published once.
void questReceipt(Harness &h,const Handler &handler,const SdlWindow::IdleFrameHandler &idle,bool presentTerminal=true) {
 const auto quests=h.party->questItems.counts();
 const auto objects=h.world->sessionState().disabledObjectCount();
 const auto events=h.world->sessionState().disabledEventCount();
 const auto random=h.world->sessionState().journeyRandom();
 const auto context=h.party->encounterContext;
 auto generation=h.flow->presentationGeneration();
 check(generation&&!h.flow->canSave(),"quest blank retained before receipt stages");
 const auto unchanged=[&]{check(h.party->questItems.counts()==quests &&
  h.world->sessionState().disabledObjectCount()==objects &&
  h.world->sessionState().disabledEventCount()==events &&
  h.world->sessionState().journeyRandom()==random && h.party->encounterContext==context,
  "quest receipt redraw/acknowledgment never republishes gameplay effects");};
 for(const char *stage:{"found","final"}) {
  handler.framePresented(h.flow->frame().presentation());
  send(h,handler,AcknowledgeAction{});
  const auto next=h.flow->presentationGeneration();
  check(next&&next!=generation&&!h.flow->canSave(),stage);
  generation=next;unchanged();stagedEffects(h,std::uint8_t(quests[17]+1),objects+1,events+11);
 }
 handler.framePresented(h.flow->frame().presentation());
 send(h,handler,AcknowledgeAction{},presentTerminal);
 check(!h.flow->presentationGeneration(),"final quest acknowledgment consumes its continuation");
 if(presentTerminal)for(unsigned n=0;n<20&&!h.flow->canSave();++n)h.tick(handler,idle); // Retire the outer pass.
 check(h.party->questItems.at(17)==std::uint8_t(quests[17]+1) &&
  h.world->sessionState().disabledObjectCount()==objects+1 && h.world->sessionState().disabledEventCount()==events+11 &&
  h.world->sessionState().journeyRandom()==random && h.party->encounterContext->minutes==context->minutes &&
  h.party->encounterContext->ctr24==context->ctr24,
  ("final receipt publishes objective effects with Part 0 replacement key: time="+std::to_string(h.party->encounterContext->minutes)+" before="+std::to_string(context->minutes)+" ctr="+std::to_string(h.party->encounterContext->ctr24)+" before="+std::to_string(context->ctr24)+" rng="+std::to_string(h.world->sessionState().journeyRandom()->count-random->count)+" quest="+std::to_string(h.party->questItems.at(17))+"/"+std::to_string(quests[17])+" objects="+std::to_string(h.world->sessionState().disabledObjectCount())+"/"+std::to_string(objects)+" events="+std::to_string(h.world->sessionState().disabledEventCount())+"/"+std::to_string(events)).c_str());
}
void finishEvent(Harness &h,const Handler &handler,const SdlWindow::IdleFrameHandler &idle) {
 for(unsigned n=0;!h.flow->canSave();++n) {
  check(n<100,"retained Event recovery is bounded");h.tick(handler,idle);
  if(h.flow->canSave())break;
  if(h.flow->canCancelInteraction())send(h,handler,SelectMemberAction{4});
  else if(h.flow->presentationGeneration())send(h,handler,AcknowledgeAction{});
 }
}
XeenSaveSnapshot objective(const fs::path &path) {
 Harness h;auto s=services(h);std::optional<XeenSaveSnapshot> saved;
 s.show=[&](const auto &,const auto &handler,const auto &,const auto &idle,const auto &){handler.framePresented(h.flow->frame().presentation());h.tick(handler,idle);saved=snapshot(h);return true;};
 check(Application(s.program).playGameplay(s,{},path,false,XeenEncounterEntry::Journey,1)==0&&saved.has_value(),"production fresh fixture source");
 saved->camera={23,8,2,XeenDirection::North};
 for(auto &a:saved->map(23).monsters->slots){a.x=a.y=-128;a.lifecycle=XeenActorLifecycle::Defeated;a.accounted=true;}
 return *saved;
}
void run(const fs::path &path,const XeenSaveSnapshot &saved,const std::function<void(Harness &,XeenGameplayServices &)> &configure,int expected=0){
 XeenSaveFile::write(path,saved);Harness h;auto s=services(h);configure(h,s);s.resources.loadRegionalText=s.texts;bool shown=false;const auto show=s.show;s.show=[&](const auto &frame,const auto &handler,const auto &escape,const auto &idle,const auto &status){const bool result=show(frame,handler,escape,idle,status);shown=true;return result;};
 check(Application(s.program).playGameplay(s,{},path,true)==expected&&shown,"objective Application test completed after all show assertions");
}
void authority(const fs::path &path,const XeenSaveSnapshot &saved){
 run(path,saved,[&](Harness &h,XeenGameplayServices &s){s.show=[&](const auto &,const auto &handler,const auto &,const auto &idle,const auto &){
  handler.framePresented(h.flow->frame().presentation());h.tick(handler,idle);
  send(h,handler,InteractionAction{},false);auto who=*h.flow->presentationGeneration();
  check(!h.flow->respond(who,SelectedCharacter{4}),"direct response refuses unpresented WhoWill");handler.framePresented(h.flow->frame().presentation());
  check(!h.flow->respond(who,XeenPresentationResponse::Acknowledged)&&!h.flow->respond(who+1,SelectedCharacter{4}),"wrong phase and generation refuse selection");
  rejects([&]{h.flow->acceptManual(XeenManualEventCompleted{10,false,false});});
  send(h,handler,SelectMemberAction{99});check(h.flow->canCancelInteraction(),"invalid live index retains WhoWill");effects(h,0,0,0);
  const auto selectingInput=*handler.displayedInput();send(h,handler,SelectMemberAction{4},false);
  auto ack=*h.flow->presentationGeneration();check(!h.flow->respond(ack,XeenPresentationResponse::Acknowledged),"direct acknowledgment requires current presented modal");
  handler.framePresented(h.flow->frame().presentation());check(!h.flow->respond(who,SelectedCharacter{4})&&!h.flow->respond(ack,SelectedCharacter{4}),"stale and wrong phase responses refuse");
  handler.withDisplayedInput(AcknowledgeAction{},selectingInput);effects(h,0,0,0);
  const auto saves=h.saves;for(const PlayerAction &a:std::vector<PlayerAction>{SaveGameAction{},NavigationAction::MoveForward,InspectInventoryAction{},BlockAction{}})send(h,handler,a);
  check(h.saves==saves&&!h.flow->canSave(),"modal refusal precedes save providers");
  send(h,handler,AcknowledgeAction{},false);effects(h,0,0,0);stagedEffects(h,1,1,11);check(!h.flow->canSave()&&!h.flow->respond(ack,XeenPresentationResponse::Acknowledged),"staged effects require terminal frame and response is consumed");
  handler.framePresented(h.flow->frame().presentation());questReceipt(h,handler,idle,false);
  check(!h.flow->canSave(),"final receipt response still requires terminal frame acquisition");
  handler.framePresented(h.flow->frame().presentation());finishEvent(h,handler,idle);check(h.flow->canSave(),"terminal frame and final pass open save");
  auto expected=saved;++expected.questItems[17];save_test::setRemovedObjects(expected,{{23,13}});for(unsigned i=1;i<=11;++i)save_test::addDisabledEvent(expected,{23,124+i});
  check(XeenSaveFormat::encode(snapshot(h))==XeenSaveFormat::encode(expected),"authority tests preserve all nonobjective state");return true;};});
}
void modalSdlReturn(const fs::path &path,const XeenSaveSnapshot &saved) {
 run(path,saved,[&](Harness &h,XeenGameplayServices &s){s.show=[&](const auto &,const auto &handler,const auto &escape,const auto &idle,const auto &status){
  handler.framePresented(h.flow->frame().presentation());h.tick(handler,idle);acknowledgment(h,handler);
  unsigned loops=0,receiptTaps=0;bool initialQueued=false,turnQueued=false,quitQueued=false,quietAcquired=false;
  std::optional<XeenRewardPhase> rewardPhase;
  std::optional<std::uint64_t> acquiredGeneration;
  IndexedFrame::Presentation acquiredFrame;
  std::array<std::optional<std::uint64_t>,3> seen{},sent{};
  const auto index=[&]()->std::optional<unsigned>{
   if(rewardPhase==XeenRewardPhase::Blank)return 0;
   if(rewardPhase==XeenRewardPhase::Found)return 1;
   if(rewardPhase==XeenRewardPhase::Receipt)return 2;
   return {};
  };
  const auto report=h.flow->reportManual;
  h.flow->reportManual=[&](const auto &result){
   if(const auto *pending=std::get_if<XeenEventExecutionSuspended>(&result)) {
    if(pending->request.kind==XeenPresentationKind::RewardReceipt)rewardPhase=pending->state.reward.phase;
   } else rewardPhase.reset();
   if(report)report(result);
  };
  std::vector<SDL_Event> pendingKeys;
  const auto key=[&](SDL_Keycode code){SDL_Event e{};e.type=SDL_KEYDOWN;e.key.keysym.sym=code;pendingKeys.push_back(e);e.type=SDL_KEYUP;pendingKeys.push_back(e);};
  auto windowHandler=handler;
  windowHandler.framePresented=[&](const auto &frame){
   handler.framePresented(frame);acquiredFrame=frame;acquiredGeneration=h.flow->presentationGeneration();
   if(const auto stage=index()) {
    check(acquiredGeneration.has_value(),"acquired quest stage has a live generation");
    if(*stage)check(seen[*stage-1].has_value(),"quest blank/found/final frames acquired in order");
    if(seen[*stage])check(seen[*stage]==acquiredGeneration,"repaint retains quest stage generation");
    else seen[*stage]=acquiredGeneration;
    effects(h,0,0,0);stagedEffects(h,1,1,11);
    check(h.camera->direction==XeenDirection::North,"same SDL batch cannot cross acknowledgment to Quiet");
   } else if(initialQueued&&!acquiredGeneration&&h.flow->canSave())quietAcquired=true;
  };
  windowHandler.beginCycle=[&](std::uint64_t){
   h.now+=110;handler.beginCycle(++h.cycle);
   // Queue before idle when the acquired stage's draw floor becomes ready.
   // A late idle may expire a hold, so idle callback counts are not stage IDs.
   if(const auto stage=index())if(acquiredGeneration==h.flow->presentationGeneration() &&
      acquiredFrame==h.flow->frame().presentation() && handler.inputContext(acquiredFrame).readyForAction &&
      !sent[*stage]) {
    check(receiptTaps==*stage,"one distinct fresh acknowledgment per acquired quest stage");
    sent[*stage]=acquiredGeneration;++receiptTaps;key(SDLK_RETURN);
   }
   for(auto &event:pendingKeys)check(SDL_PushEvent(&event)==1,"modal SDL key/release");
   pendingKeys.clear();
  };
  const bool result=SdlWindow().showInteractive(h.flow->frame(),"Modal Event to Quiet input",windowHandler,escape,[&]()->std::optional<IndexedFrame>{
   check(++loops<60,"modal SDL return bounded");
   if(!initialQueued){key(SDLK_RETURN);key(SDLK_RIGHT);initialQueued=true;}
   else if(!turnQueued&&quietAcquired&&h.flow->canSave()&&!h.flow->presentationGeneration()) {
    check(receiptTaps==3&&seen[0]&&seen[1]&&seen[2],"genuine modal completion presented after three distinct quest acknowledgments");
    const auto context=handler.inputContext(h.flow->frame().presentation());
    if(context.readyForNavigation){key(SDLK_RIGHT);turnQueued=true;}
   } else if(turnQueued&&h.camera->direction==XeenDirection::East&&h.flow->canSave()&&!quitQueued) {
    check(quietAcquired,"fresh SDL movement follows acquired modal completion");
    SDL_Event e{};e.type=SDL_QUIT;check(SDL_PushEvent(&e)==1,"modal SDL quit");quitQueued=true;
   }
   return idle();
  },status);
  h.flow->reportManual=report;
  check(receiptTaps==3&&quietAcquired&&turnQueued&&quitQueued&&h.camera->direction==XeenDirection::East,
   "three acquired quest stages permit exactly the fresh post-Quiet turn");
  return result;
 };});
}
void modalCosmeticOmissions(const fs::path &path,const XeenSaveSnapshot &saved) {
 for(unsigned mode=0;mode<6;++mode)run(path,saved,[&](Harness &h,XeenGameplayServices &s){
  cosmetic_handoff_test::changingPixels(s);
  s.show=[&](const auto &,const auto &handler,const auto &escape,const auto &idle,const auto &status){
   handler.framePresented(h.flow->frame().presentation());h.tick(handler,idle);acknowledgment(h,handler);const auto response=h.flow->presentationGeneration();
   const auto unchanged=[&]{effects(h,0,0,0);check(h.flow->presentationGeneration()==response && h.world->sessionState().journeyActivity()==XeenJourneyActivity::Event,"Unuploaded cosmetic retains matching Event continuation and lease");};
   const auto accepted=[&]{effects(h,0,0,0);stagedEffects(h,1,1,11);check(h.flow->presentationGeneration()!=response,"Legitimate response enters quest blank after expected frame upload");questReceipt(h,handler,idle);};
   return cosmetic_handoff_test::exercise(h,handler,idle,escape,status,mode,SDLK_RETURN,AcknowledgeAction{},unchanged,accepted);
  };
 });
}
void failures(const fs::path &path,const XeenSaveSnapshot &saved){
 // The live grant follows the Event byte rule (0x22967..0x2296e): 255 wraps to 0.
 auto maximum=saved;maximum.questItems[17]=255;
 run(path,maximum,[&](Harness &h,XeenGameplayServices &s){s.show=[&](const auto &,const auto &handler,const auto &,const auto &idle,const auto &){handler.framePresented(h.flow->frame().presentation());h.tick(handler,idle);acknowledgment(h,handler);send(h,handler,AcknowledgeAction{});questReceipt(h,handler,idle);effects(h,0,1,11);
  auto expected=maximum;expected.questItems[17]=0;save_test::setRemovedObjects(expected,{{23,13}});for(unsigned i=1;i<=11;++i)save_test::addDisabledEvent(expected,{23,124+i});
  check(h.flow->canSave()&&XeenSaveFormat::encode(snapshot(h))==XeenSaveFormat::encode(expected),"live quest grant at 255 did not wrap to 0 exactly");return true;};});
 for(unsigned phase=0;phase<3;++phase)run(path,saved,[&](Harness &h,XeenGameplayServices &s){s.show=[&,phase](const auto &,const auto &handler,const auto &,const auto &idle,const auto &){
  handler.framePresented(h.flow->frame().presentation());h.tick(handler,idle);bool injected=false;
  h.flow->reportManual=[&](const auto &r){const bool suspended=std::holds_alternative<XeenEventExecutionSuspended>(r);if(!injected&&((phase==0&&suspended)||(phase==1&&suspended&&!h.flow->canCancelInteraction())||(phase==2&&std::holds_alternative<XeenManualEventCompleted>(r)))){injected=true;throw std::runtime_error("one report fault");}};
  send(h,handler,InteractionAction{});if(!injected)send(h,handler,SelectMemberAction{4});if(!injected)send(h,handler,AcknowledgeAction{});
  if(phase==2&&!injected)questReceipt(h,handler,idle);
  check(injected,"report fault reached the requested retained phase");
  if(phase<2){check(!h.flow->canSave(),"report failure retains Event until fresh recovery presentation");effects(h,0,0,0);}
  finishEvent(h,handler,idle);effects(h,1,1,11);
  auto expected=saved;++expected.questItems[17];save_test::setRemovedObjects(expected,{{23,13}});for(unsigned i=1;i<=11;++i)save_test::addDisabledEvent(expected,{23,124+i});
  check(XeenSaveFormat::encode(snapshot(h))==XeenSaveFormat::encode(expected),"report recovery publishes one exact objective and current-key completion");
  const auto bytes=XeenSaveFormat::encode(snapshot(h));send(h,handler,SaveGameAction{});check(XeenSaveFormat::encode(XeenSaveFile::read(path))==bytes,"fresh F9 persists surviving report-failure effects");h.flow->refresh(true);handler.framePresented(h.flow->frame().presentation());check(bytes==XeenSaveFormat::encode(snapshot(h)),"report recovery reconstruction never replays");h.flow->reportManual={};return true;};});
 run(path,saved,[&](Harness &h,XeenGameplayServices &s){auto state=std::make_shared<std::array<bool,2>>();const auto maps=s.maps;
  s.maps=[&,maps,state](auto id){if((*state)[0]&&!(*state)[1]&&XeenCityEventTestAccess::pendingQuest(*h.flow)==1){(*state)[1]=true;throw std::runtime_error("Remove map preparation fault");}return maps(id);};
  s.show=[&,state](const auto &,const auto &handler,const auto &,const auto &idle,const auto &){handler.framePresented(h.flow->frame().presentation());h.tick(handler,idle);acknowledgment(h,handler);h.world->discardMapCache();XeenCityEventTestAccess::discardCandidateMaps(*h.flow);(*state)[0]=true;send(h,handler,AcknowledgeAction{});
   finishEvent(h,handler,idle);check((*state)[1]&&h.flow->canSave(),"unobserved provider fault has a trusted recovery frame");effects(h,0,0,0);auto partial=snapshot(h);check(XeenSaveFormat::encode(partial)==XeenSaveFormat::encode(saved),"unobserved grant and failed Remove discard the detached action exactly");send(h,handler,SaveGameAction{});check(XeenSaveFormat::encode(XeenSaveFile::read(path))==XeenSaveFormat::encode(partial),"fresh F9 writes the exact discarded-action recovery state");
   acknowledgment(h,handler);send(h,handler,AcknowledgeAction{});questReceipt(h,handler,idle);effects(h,1,1,11);return true;};});
 for(bool persistent:{false,true})run(path,saved,[&](Harness &h,XeenGameplayServices &s){auto failures=std::make_shared<unsigned>(0);auto enabled=std::make_shared<bool>(true);const auto compose=s.composeEncounter;
  s.composeEncounter=[&,compose,persistent,failures,enabled](auto &w,const auto &p,const auto &c,auto ordinary,auto actor){if(*enabled&&w.sessionState().disabledObjectCount()&&(persistent||!*failures)){++*failures;throw std::runtime_error("postRemove composition fault");}return compose(w,p,c,ordinary,actor);};
  s.show=[&,persistent,failures,enabled](const auto &,const auto &handler,const auto &,const auto &idle,const auto &){handler.framePresented(h.flow->frame().presentation());h.tick(handler,idle);acknowledgment(h,handler);send(h,handler,AcknowledgeAction{});
   effects(h,0,0,0);stagedEffects(h,1,1,11);
   // Acquire all receipt phases; the final composition failure leaves the
   // completed candidate retained, without publishing its gameplay projection.
   for(unsigned n=0;n<3;++n)send(h,handler,AcknowledgeAction{});
   check(*failures>0&&!h.flow->canSave(),"postRemove composition fault retains a completed Event");effects(h,0,0,0);stagedEffects(h,1,1,11);
   if(persistent)for(unsigned n=0;n<2;++n){h.tick(handler,idle);check(!h.flow->canSave(),"persistent composition failure cannot expose Quiet");effects(h,0,0,0);}
   *enabled=false;finishEvent(h,handler,idle);effects(h,1,1,11);
   auto expected=saved;++expected.questItems[17];save_test::setRemovedObjects(expected,{{23,13}});for(unsigned i=1;i<=11;++i)save_test::addDisabledEvent(expected,{23,124+i});
   check(XeenSaveFormat::encode(snapshot(h))==XeenSaveFormat::encode(expected),"composition retry publishes one exact objective and current-key completion");return true;};});
}
}
void pages(const fs::path &path,const XeenSaveSnapshot &saved){
 auto injured=saved;injured.characters[injured.activeRosterIds[0]].currentHp=-1;injured.characters[injured.activeRosterIds[0]].conditions[12]=1;
 injured.characters[injured.activeRosterIds[1]].currentHp=-30;injured.characters[injured.activeRosterIds[1]].conditions[13]=1;
 run(path,injured,[&](Harness &h,XeenGameplayServices &s){
  auto loads=std::make_shared<std::array<unsigned,4>>();
  const auto maps=s.maps;s.maps=[maps,loads](auto id){++(*loads)[0];return maps(id);};
  const auto objects=s.objects;s.objects=[objects,loads](auto id){++(*loads)[1];return objects(id);};
  const auto events=s.resources.loadEvents;s.resources.loadEvents=[events,loads](auto id){++(*loads)[2];return events(id);};
  const auto compose=s.composeEncounter;s.composeEncounter=[compose](auto &w,const auto &p,const auto &c,auto ordinary,auto actor){w.map(c.mapId);w.objectFile(c.mapId);return compose(w,p,c,ordinary,actor);};
  s.texts=[loads](auto id){++(*loads)[3];std::string text;for(unsigned i=0;i<40;++i)text+="Synthetic discovery line.\n";auto t=regional_test::texts(id);t.strings[0]=text;t.strings[1]=t.strings[2]="";t.strings[3]="Synthetic bones";return t;};
  s.show=[&,loads](const auto &,const auto &handler,const auto &,const auto &idle,const auto &){handler.framePresented(h.flow->frame().presentation());h.tick(handler,idle);const auto before=snapshot(h);
   send(h,handler,InteractionAction{});send(h,handler,SelectMemberAction{0});check(h.flow->canCancelInteraction(),"Unconscious choice retains WhoWill");send(h,handler,AcknowledgeAction{});
   send(h,handler,SelectMemberAction{1});check(h.flow->canCancelInteraction(),"Dead choice retains WhoWill");send(h,handler,AcknowledgeAction{});effects(h,0,0,0);
   send(h,handler,SelectMemberAction{4});const auto count=h.flow->presenter().pageCount();check(count>1&&h.flow->presenter().pageIndex()==0,"synthetic discovery uses multiple real presenter pages");
   const auto generation=*h.flow->presentationGeneration();check(!h.flow->respond(generation,XeenPresentationResponse::Presented),"direct Presented cannot skip undisplayed discovery pages");
   for(unsigned page=0;page+1<count;++page){check(h.flow->presenter().pageIndex()==page,"pagination advances exactly one visible page");
    for(unsigned pass=0;pass<3;++pass){const auto priorLoads=*loads;h.eventSystem->discardScriptCache();h.eventSystem->discardTextCache();h.world->discardMapCache();h.flow->refresh(true);
     check(!h.flow->respond(generation,XeenPresentationResponse::Presented),"reconstruction must be presented before response");handler.framePresented(h.flow->frame().presentation());for(unsigned resource=0;resource<4;++resource)check((*loads)[resource]>priorLoads[resource],"combined modal rebuild reloads every discarded resource cache");
     check(h.flow->presentationGeneration()==generation&&h.flow->presenter().pageIndex()==page&&h.flow->presenter().pageCount()==count,"cache reconstruction retains semantic page and continuation generation");
     effects(h,0,0,0);check(h.party->encounterContext==before.journey->context&&h.world->sessionState().journeyRandom()==before.journey->random,"modal reconstruction advances no clock or RNG");}
    send(h,handler,AcknowledgeAction{});
   }
   check(h.flow->presentationGeneration()!=generation&&!h.flow->canCancelInteraction(),"discovery pages lead to separate Action44 acknowledgment");effects(h,0,0,0);
   const auto discovery=h.flow->frame();h.flow->refresh(true);handler.framePresented(h.flow->frame().presentation());
   bool ink=false;for(unsigned y=143;y<199;++y)for(unsigned x=0;x<320;++x){check(discovery.pixels[y*320+x]==h.flow->frame().pixels[y*320+x],"final discovery page remains unchanged through separate Action44 reconstruction");ink|=discovery.pixels[y*320+x]!=h.base.pixels[y*320+x];}check(ink,"retained final discovery is visibly drawn");
   send(h,handler,AcknowledgeAction{});effects(h,0,0,0);stagedEffects(h,1,1,11);questReceipt(h,handler,idle);check(h.flow->canSave(),"multipage collection becomes quiet after final quest receipt frame");
   const auto previous=*loads;h.eventSystem->discardScriptCache();h.eventSystem->discardTextCache();h.world->discardMapCache();send(h,handler,InteractionAction{});
   check((*loads)[0]>previous[0]&&(*loads)[1]>previous[1]&&(*loads)[2]>previous[2],"explicit effective-None repeat really reloads immutable map MOB and EVT");effects(h,1,1,11);
   check((*loads)[3]>0,"original text provider was used");return true;};
 });
}
void shiftedSourceAuthority(const fs::path &path,const XeenSaveSnapshot &saved) {
	for(unsigned shift:{1u,9u,37u})for(bool changed:{false,true}) {
		XeenSaveFile::write(path,saved);Harness h;auto s=services(h,shift);bool armed=false;
		const auto original=s.resources.loadEvents;
		s.resources.loadEvents=[&](auto id){auto file=original(id);if(armed && changed && id==XeenMapIdentity(23))file.records[131+shift].parameters={0,0,21,100};return file;};
		s.show=[&](const auto &,const auto &handler,const auto &,const auto &idle,const auto &){
			handler.framePresented(h.flow->frame().presentation());h.tick(handler,idle);acknowledgment(h,handler);
			armed=true;h.eventSystem->discardScriptCache();
			try{send(h,handler,AcknowledgeAction{});}catch(const std::exception &){}
			if(changed){effects(h,0,0,0);check(!h.flow->canSave(),"shifted source mutation loses publication authority");}
			else {
				questReceipt(h,handler,idle);effects(h,1,1,11);
				for(unsigned i=125;i<=135;++i)check(h.world->sessionState().isEventDisabled({23,i+shift}),"Remove uses shifted physical-cell source records");
				check(!h.world->sessionState().isEventDisabled({23,124+shift}),"Remove does not include neighboring source record");
			}
			return true;
		};
		check(Application(s.program).playGameplay(s,{},path,true)==0,"shifted source Application controls");
	}
}

void integrity(const fs::path &path,const XeenSaveSnapshot &saved){
 for(unsigned seam=0;seam<6;++seam)for(bool replace:{false,true})run(path,saved,[&](Harness &h,XeenGameplayServices &s){

  struct State{bool armed=false,injected=false;unsigned reentrant=0;};auto state=std::make_shared<State>();
  const auto attack=[&,state,replace]{if(!state->armed||state->injected)return;state->injected=true;
   check(!h.flow->canSave(),"callback cannot capture a live Event operation");
   const auto generation=h.flow->presentationGeneration();
   if(generation)check(!h.flow->respond(*generation,XeenPresentationResponse::Acknowledged),"reentrant response refused during callback");
   h.flow->handle(InteractionAction{},h.flow->displayedInput());++state->reentrant;
   auto *flags=const_cast<XeenGameFlags *>(h.flags);if(replace){const auto values=flags->values();flags->~XeenGameFlags();new(flags)XeenGameFlags(values);}else flags->set(7);
  };
  if(seam==0){const auto provider=s.resources.loadEvents;s.resources.loadEvents=[provider,attack](auto id){attack();return provider(id);};}
  if(seam==1){const auto provider=s.texts;s.texts=[provider,attack](auto id){attack();return provider(id);};}
  if(seam==2){const auto provider=s.maps;s.maps=[provider,attack](auto id){attack();return provider(id);};}
  if(seam==3){const auto provider=s.objects;s.objects=[provider,attack](auto id){attack();return provider(id);};}
  if(seam==5){const auto compose=s.composeEncounter;s.composeEncounter=[compose,attack](auto &w,const auto &p,const auto &c,auto ordinary,auto actor){attack();return compose(w,p,c,ordinary,actor);};}
  s.show=[&,state,attack,seam](const auto &,const auto &handler,const auto &,const auto &idle,const auto &){handler.framePresented(h.flow->frame().presentation());h.tick(handler,idle);
   if(seam==0)h.eventSystem->discardScriptCache();if(seam==1)h.eventSystem->discardTextCache();if(seam==2||seam==3)h.world->discardMapCache();
   if(seam==4)h.flow->reportManual=[attack](const auto &){attack();};
   state->armed=true;try{send(h,handler,InteractionAction{});}catch(const std::exception &){}
   check(state->injected&&state->reentrant==1&&!h.flow->canSave(),"provider/report/composer owner mutation cannot recover quiet authority");effects(h,0,0,0);
   const auto saves=h.saves;try{send(h,handler,SaveGameAction{});}catch(const std::exception &){}check(saves==h.saves,"integrity violation refuses saving before I/O");return true;
  };
 },replace?3:0);
}
int main(){try{const auto directory=fs::temp_directory_path()/("mmodern-m31-events-"+std::to_string(GetCurrentProcessId())+"-"+std::to_string(GetTickCount64()));fs::create_directory(directory);const auto path=directory/"objective.mmsave";const auto saved=objective(path);authority(path,saved);modalSdlReturn(path,saved);modalCosmeticOmissions(path,saved);failures(path,saved);pages(path,saved);shiftedSourceAuthority(path,saved);integrity(path,saved);std::cout<<"Journey objective authority and failure controls passed\n";child_test::removePassedRun(directory);return 0;}catch(const std::exception &e){std::cerr<<e.what()<<'\n';return 1;}}
