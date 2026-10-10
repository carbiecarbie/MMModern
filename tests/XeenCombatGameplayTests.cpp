#include "XeenRegionalGameplayTestSupport.h"
#include "app/XeenTitleFlow.h"
#include <iostream>
#include <fstream>
#define SDL_MAIN_HANDLED
#include <SDL.h>
using namespace regional_gameplay_test;
void panelCombat() {
 Harness h;auto services=h.services();unsigned providers=0;
 services.panel=[&](const auto &base,bool combat,bool restricted,bool saveable,auto current,const auto &name) {
  check(combat && saveable && !restricted,"combat panel admission");
  XeenTitleFlow::Services s{dos_test::text(),h.font,base,{}, {},[](auto &,const char *,unsigned,int,int){},
   [&]{++providers;return std::array<XeenSaveFile::Slot,10>{};},[](unsigned){return std::filesystem::path{};}};
  s.panel=true;s.combat=combat;s.currentSlot=current;s.currentName=name;return std::make_unique<XeenTitleFlow>(std::move(s));
 };
 services.show=[&](const auto &,const auto &handler,const auto &,const auto &idle,const auto &) {
  h.press(handler,WaitAction{});check(h.phase()==Phase::PlayerReady,"combat panel player boundary");
  const auto revision=h.result().revision,rng=h.randomPosition();const auto deadline=h.flow->encounter()->deadline();
  XeenRestoreGuard guard(*h.world,*h.party,*h.camera,*h.flags);
  IndexedFrame visible=h.flow->frame();
  const auto send=[&](const PlayerAction &action) {
   const auto next=handler.withPresentedInput(action,*handler.displayedInput(),visible.presentation());
   check(bool(next),"combat panel response");visible=*next;
   handler.framePresented(visible.presentation());handler.completeInputHandoff(visible.presentation());guard.check();
  };
  send(ControlPanelAction{});check(handler.inputContext(visible.presentation()).dialog->hits.size()==9,"combat panel failed to open");
  for(unsigned key:{'s','a','l'}) {
   send(DialogKeyAction{key});check(handler.inputContext(visible.presentation()).dialog->anyKey,"combat refusal not visible");
   send(DialogKeyAction{27});
  }
  h.now+=1000;idle();guard.check();
  check(providers==0 && h.result().revision==revision && h.randomPosition()==rng && h.flow->encounter()->deadline()==deadline,"combat panel consumed gameplay/providers");
  send(DialogKeyAction{27});check(!handler.inputContext(visible.presentation()).dialog,"combat panel Escape");return true;
 };
 check(h.run(services)==0,"combat panel production dispatcher");
}
void appearance() {
 Harness h;auto s=h.services();
 const auto compose=s.composeEncounter;bool delayedFrame=false;
 s.composeEncounter=[&](auto &w,const auto &p,const auto &c,auto ordinary,auto actor){
  auto result=compose(w,p,c,ordinary,actor);
  if(actor.kind==XeenMonsterSpriteKind::Attack&&!delayedFrame){h.now+=250;delayedFrame=true;}
  return result;
 };
 s.show=[&](const IndexedFrame &,const auto &handler,const auto &,const auto &idle,const auto &){

  h.press(handler,WaitAction{});
  auto frame=[&](unsigned expected){
   const auto value=h.observedAppearance;
   check(value.frame==(expected<8?expected:expected-8) && value.kind==
    (expected<8?XeenMonsterSpriteKind::Normal:XeenMonsterSpriteKind::Attack),"production MON/ATT sequence");

  };
  auto rebuild=[&]{
   const auto pixels=h.flow->frame().pixels;
   const auto deadline=h.flow->encounter()->cosmeticDeadline();
   const auto service=h.flow->encounter()->deadline();
   const auto revision=h.result().revision, rng=h.randomPosition();
   const auto hp=h.world->sessionState().actors()[5].hp;
   h.world->discardMapCache();
   h.flow->refresh(true);
   check(h.flow->frame().pixels==pixels && h.flow->encounter()->cosmeticDeadline()==deadline &&
    h.flow->encounter()->deadline()==service && h.result().revision==revision &&
    h.randomPosition()==rng && h.world->sessionState().actors()[5].hp==hp,"cache changed appearance or gameplay");
  };
  for(unsigned i=0;i<8;++i){frame(i);rebuild();h.tick(handler,idle);}
  for(unsigned i=0;i<6;++i)h.press(handler,BlockAction{});
  h.tick(handler,idle);frame(8);rebuild();
  check(h.flow->encounter()->cosmeticDeadline()==h.now+100,"attack cadence starts after fallible frame composition");
  const unsigned enemy[]{9,10,10,10,0};
  for(auto expected:enemy){h.tick(handler,idle);frame(expected);}
  // Seed1: Arturius misses, then Tyro hits. Pending intent is not a hit.
  h.press(handler,InteractionAction{});h.tick(handler,idle);
  check(h.result().attackOutcome==XeenCombatAttackOutcome::Miss,"seeded miss control");
  check(h.observedAppearance.kind==XeenMonsterSpriteKind::Normal,"miss created hit effect");
  h.press(handler,InteractionAction{});h.tick(handler,idle);frame(11);
  check(h.world->sessionState().actors()[5].hp==12,"partial live HP control");
  rebuild();
  const auto revision=h.result().revision,rng=h.randomPosition();
  // Same-time and backward observations do not advance or rearm.
  const auto deadline=h.flow->encounter()->cosmeticDeadline();
  idle();h.now-=1;idle();h.now+=1;frame(11);
  check(h.flow->encounter()->cosmeticDeadline()==deadline,"obsolete cosmetic clock");
  for(unsigned i=0;i<5;++i){h.tick(handler,idle);frame(i==4?0:11);}
  check(h.result().revision==revision && h.randomPosition()==rng,"cosmetics ran gameplay");
  h.now+=10000;h.tick(handler,idle);frame(1);
  check(h.flow->encounter()->cosmeticDeadline()==h.now+100,"no cosmetic backlog");
  return true;
 };
 check(h.run(s)==0,"production appearance");
}
void criticalPresentation() {
 Harness h;
 std::vector<XeenCombatRandom::Draw> tape;
 for(unsigned round=0;round<6;++round){
  const unsigned targets[]{0,0,1,2,3,5};
  if(round)tape.push_back({0,5,targets[round]});
  tape.push_back({1,20,20});tape.push_back({1,6,6});tape.push_back({1,6,6});tape.push_back({1,4,4});
  if(round!=1){tape.push_back({1,6,6});tape.push_back({1,6,6});}
 }
 h.random.emplace(tape);auto s=h.services();unsigned enemies=0;
 s.show=[&](const IndexedFrame &,const auto &handler,const auto &,const auto &idle,const auto &){
  h.press(handler,WaitAction{});
  for(unsigned step=0;step<100&&!h.flow->encounter()->terminal();++step){
   if(h.phase()==Phase::PlayerReady)h.press(handler,BlockAction{});
   else {
    h.tick(handler,idle);const auto &r=h.result();
    if(r.operation==XeenCombatOperation::EnemyAttack){
     ++enemies;check(r.critical&&r.injuryCount==(enemies==2?1U:2U),"ordered critical presentation observations");
     check(h.observedAppearance.kind==XeenMonsterSpriteKind::Attack&&h.observedAppearance.frame==0,"critical ATT initial frame");
     if(enemies==1)check(r.injuries[0].afterHp==-5&&r.injuries[1].afterHp==-17&&r.armorCount==2,"intermediate injury and armor facts");
    }
   }
  }
  check(enemies==6&&h.phase()==Phase::Defeat&&h.party->encounterContext->minutes==495,"critical production defeat");
  const auto &notice=h.flow->encounter()->notice();
  check(notice.find("Dead")!=std::string::npos&&notice.find("armor broken")!=std::string::npos,"complete condition/breakage feedback");
  return true;
 };
 check(h.run(s)==0,"critical presentation production route");
}
void delayed() {
 Harness h;auto s=h.services();
 s.show=[&](const IndexedFrame &,const auto &handler,const auto &,const auto &idle,const auto &){

  h.press(handler,NavigationAction::TurnRight);check(h.observedOrdinary==0,"facing reset ordinary animation");
  h.press(handler,NavigationAction::MoveForward);check(h.observedOrdinary==1,"forward advances ordinary phase");
  check(h.flow->encounter()->state().pending()==2,"action and supplied pulse separate");
  const auto pending=h.flow->encounter()->state().pending();idle();
  check(h.flow->encounter()->state().pending()==pending,"same-cycle idle cannot pulse again");
  h.tick(handler,idle);h.tick(handler,idle);h.press(handler,WaitAction{});
  check(h.phase()==Phase::PlayerReady&&h.party->encounterContext->minutes==500,"delayed production engagement at500");
  const auto ordinary=h.observedOrdinary;
  h.press(handler,BlockAction{});check(h.observedOrdinary==ordinary,"Block does not act as navigation");
  h.tick(handler,idle);check(h.observedOrdinary==ordinary+1,"ordinary idle animation continues in combat");
  check(h.randomPosition()==0,"idle without combat work creates no turn");
  return true;
 };
 check(h.run(s)==0,"delayed production route");
}
void failures() {
 for(unsigned fault=0;fault<6;++fault) {
  Harness h;auto s=h.services();bool injected=false;
  const auto compose=s.composeEncounter;
  s.composeEncounter=[&](auto &w,const auto &p,const auto &c,auto ordinary,auto actor){
   if(fault==0&&h.flow&&h.phase()==Phase::PreparingAction&&!injected){injected=true;throw std::runtime_error("composition failure");}
   return compose(w,p,c,ordinary,actor);
  };
  const auto configure=s.configureFlow;
  s.configureFlow=[&](auto &flow,const auto &camera){
   configure(flow,camera);
   flow.reportText=[&](const std::string &){
    if(fault==1&&h.phase()==Phase::PreparingAction&&!injected){injected=true;throw std::runtime_error("report failure");}
   };
   flow.beforeEncounterFrameCopy=[&]{
    if(fault==2&&h.phase()==Phase::PreparingAction&&!injected){injected=true;throw std::runtime_error("frame-copy failure");}
   };
   flow.reportEquipment=[&](const XeenEquipmentResult &r){
    if(fault==3&&!injected){check(r.status==XeenEquipmentStatus::Success,"fixed equipment adopted before callback");injected=true;throw std::runtime_error("equipment reporting failure");}
   };
  };
  s.show=[&](const IndexedFrame &,const auto &handler,const auto &,const auto &idle,const auto &){
   if(fault==3){
    h.press(handler,InspectInventoryAction{});h.press(handler,SelectInventorySlotAction{0});h.press(handler,DialogKeyAction{'r'});
    check(h.party->roster.at(0).weapons[0].frame==0,"published equipment survives reporting failure");
   }else{
    h.press(handler,WaitAction{});
    if(fault==4){h.now=std::numeric_limits<std::uint64_t>::max();injected=true;}
    try{h.press(handler,InteractionAction{});}catch(const std::exception &){check(fault==4,"Only overflow closes dispatch");}
    if(fault==5){
     injected=true;
     const auto old=h.flow->encounter()->ticket();
     h.tick(handler,idle);
     const auto generation=h.result().generation;
     h.flow->failEncounterHandoff(old);
     check(h.result().generation==generation,"stale handoff cannot fail newer combat");
     return true;
    }
   }
   check(injected,"Failure seam fired");
   check(fault<3 ? h.phase()==Phase::PreparingAction : fault==3 ? h.retired() : (!h.flow->encounterFrameCurrent() && h.phase()==Phase::PlayerReady),"Current Journey failure/recovery phase");
   check(h.randomPosition()==0,"failure did not roll pending player attack");
   if(fault!=3){check(!h.flow->canSave() && h.saves==0,"Failure or pending work is unsaveable");}
   return true;
  };
  const auto label="failure boundary route "+std::to_string(fault);
  const auto rc=h.run(s);check(rc==0,label.c_str());
 }
}
void publicationFailures() {
 for(unsigned fault=0;fault<4;++fault){
  Harness h;auto s=h.services(fault==3?19:1);bool injected=false;unsigned commands=0;
  const auto compose=s.composeEncounter;
  s.composeEncounter=[&](auto &w,const auto &p,const auto &c,auto ordinary,auto actor){
   if(h.flow&&!injected){
    const auto &r=h.result();
    const bool target=fault==0?(r.operation==XeenCombatOperation::PlayerAttack&&r.attackOutcome==XeenCombatAttackOutcome::HitPositiveDamage):
     fault==1?h.phase()==Phase::VictoryAwaitingEnd:fault==2?h.phase()==Phase::Victory:h.phase()==Phase::Defeat;
    if(target){injected=true;throw std::runtime_error("post-publication composition");}
   }
   return compose(w,p,c,ordinary,actor);
  };
  s.show=[&](const IndexedFrame &,const auto &handler,const auto &,const auto &idle,const auto &){
   if(fault==3){
    h.press(handler,InspectInventoryAction{});h.press(handler,DialogKeyAction{'a'});
    for(unsigned i=0;i<6;++i){h.press(handler,SelectMemberAction{i});for(unsigned j=0;j<9;++j)
     if(h.party->roster.at(kXeenCombatOwners[i]).armor[j].frame){h.press(handler,SelectInventorySlotAction{j});h.press(handler,DialogKeyAction{'r'});}}
    h.press(handler,CancelInteractionAction{});
   }
   h.press(handler,WaitAction{});
   for(unsigned n=0;n<500&&!injected;++n){
    if(h.phase()==Phase::PlayerReady){h.press(handler,fault==3||commands++<6?PlayerAction{BlockAction{}}:PlayerAction{InteractionAction{}});}
    else h.tick(handler,idle);
   }
   check(injected,"publication fault reached through real commands");
   check(h.phase()==(fault==2?Phase::Victory:fault==3?Phase::Defeat:fault==1?Phase::VictoryAwaitingEnd:Phase::PlayerReady),"Failure recovery preserves actual phase");
   const auto hp=h.world->sessionState().actors()[5].hp;
   check(fault==3||hp<20,"published damage survives failure");
   if(fault==1||fault==2){check(hp==0,"lethal removal retained");check(h.flow->encounter()->notice().find("XP82")!=std::string::npos,"award retained despite composition failure");}
   if(fault!=2){h.press(handler,SaveGameAction{});check(h.saves==0,"Combat save remains blocked");}
   check(h.world->sessionState().actors()[5].hp==hp,"No damage replay after recovery");
   return true;
  };
  check(h.run(s)==0,"post-publication failure route");
 }
}
void callbacks() {
 for(unsigned fault=0;fault<5;++fault){
  Harness h;auto s=h.services();bool armed=false,injected=false,observed=false;
  s.clock=[&]()->std::uint64_t{
   if(armed&&!injected&&(fault==0||fault==1)){
    injected=true;
    if(fault==1)const_cast<XeenCombat &>(h.fight()).invalidate();
    throw std::runtime_error("clock callback failure");
   }
   return h.now;
  };
  const auto compose=s.composeEncounter;
  s.composeEncounter=[&](auto &w,const auto &p,const auto &c,auto ordinary,auto actor){
   if(armed&&!injected&&(fault==2||fault==3)){
    injected=true;
    if(fault==3)const_cast<XeenCombat &>(h.fight()).invalidate();
    throw std::runtime_error("composition callback failure");
   }
   return compose(w,p,c,ordinary,actor);
  };
  const auto configure=s.configureFlow;
  s.configureFlow=[&](auto &f,const auto &c){
   configure(f,c);
   f.rebuildEncounterPresentation=[&]{if(fault==2)throw std::runtime_error("recovery failure");};
   f.reportInventory=[&](const XeenTransferResult &r){
    if(fault==4){injected=true;check(r.status==XeenTransferStatus::Success,"transfer result adopted");throw std::runtime_error("transfer report failure");}
   };
  };
  s.show=[&](const IndexedFrame &,const auto &handler,const auto &,const auto &,const auto &){
   try {
    if(fault==4){
     h.press(handler,InspectInventoryAction{});h.press(handler,SelectMemberAction{5});
     h.press(handler,DialogKeyAction{'c'});
     h.press(handler,SelectInventorySlotAction{1});
     h.press(handler,SelectMemberAction{0});h.press(handler,AcknowledgeAction{});
     check(h.party->roster.at(0).accessories[1].material==86,"transfer survives reporter failure");
    }else{
     h.press(handler,WaitAction{});armed=true;
     h.press(handler,InteractionAction{});
    }
   }catch(const std::exception &){
    check(fault<4,"Unexpected transfer callback exit");
   }
   observed=true;
   check(injected,"Callback seam fired");
   check(fault==4?h.retired():!h.flow->encounterFrameCurrent(),"Callback failure retains its legitimate stop");
   check(h.randomPosition()==0&&h.saves==0,"callback failure cannot roll or save");
   return fault==4;
  };
  const int result=h.run(s);
  check(observed&&result==(fault==4?0:4),"callback production exit boundary");
 }
}
void sdl() {
 Harness h;auto s=h.services();unsigned stage=0,accepted=0,idles=0;
 s.show=[&](const IndexedFrame &first,const auto &handler,const auto &escape,const auto &idle,const auto &status){
  auto wrapped=handler;
  std::vector<SDL_Event> pendingKeys;
  wrapped.beginCycle=[&](auto cycle){
   handler.beginCycle(cycle);
   // Commands planned by idle belong to its successfully acquired frame.
   for(auto &event:pendingKeys)check(SDL_PushEvent(&event)==1,"push production SDL key");
   pendingKeys.clear();
  };
  wrapped.withPresentedInput=[&](const PlayerAction &a,std::uint64_t t,const auto &origin){
   const auto before=h.result().generation;
   auto result=handler.withPresentedInput(a,t,origin);
   if(std::holds_alternative<BlockAction>(a)&&before!=h.result().generation)++accepted;
   return result;
  };
  auto key=[&](SDL_Keycode code,Uint32 type=SDL_KEYDOWN,Uint8 repeat=0){
   SDL_Event event{};event.type=type;event.key.keysym.sym=code;event.key.repeat=repeat;
   pendingKeys.push_back(event);
  };
  const auto scriptedIdle=[&]()->std::optional<IndexedFrame>{
   h.now+=20;++idles;
   auto frame=idle();
   if(idles>100)throw std::runtime_error("SDL combat test timed out");
   switch(stage++){
    case 0:key(SDLK_PERIOD);break;
    case 1:break;
    case 2:check(h.phase()==Phase::PlayerReady,"SDL handoff");break;
    case 3:key(SDLK_b);key(SDLK_b,SDL_KEYDOWN,1);key(SDLK_b,SDL_KEYUP);key(SDLK_b);break;
    case 4:check(accepted==1,"one owner per poll batch");key(SDLK_b);break;
    case 5:check(accepted==2,"two physical B edges queued; held B adds no action");key(SDLK_b,SDL_KEYUP);break;
    case 6:key(SDLK_b);break;
    case 7:check(accepted==3,"fresh B accepts next displayed owner");key(SDLK_b,SDL_KEYUP);key(SDLK_a);break;
    case 8:key(SDLK_a,SDL_KEYUP);break;
    case 9:break;
    case 10:key(SDLK_a);break;
    default:
     if(h.phase()==Phase::PlayerReady){
      check(h.randomPosition()>0,"automatic attack runs without another key");
      key(SDLK_ESCAPE);
     }
   }
   return frame;
  };
  return SdlWindow().showInteractive(first,"Regional combat input",wrapped,escape,scriptedIdle,status);
 };
 check(h.run(s)==0,"real SDL production route");
 check(stage>=11&&accepted==3,"SDL sequence completed");
}

void inventory() {
 Harness h;auto s=h.services();
 s.show=[&](const auto &,const auto &handler,const auto &,const auto &,const auto &){
  h.press(handler,InspectInventoryAction{});h.press(handler,SelectMemberAction{5});h.press(handler,DialogKeyAction{'c'});
  h.press(handler,SelectInventorySlotAction{1});h.press(handler,SelectMemberAction{0});
  check(h.flow->transferResult().status==XeenTransferStatus::Success,"F1 transfers immediately");
  h.press(handler,SelectMemberAction{0});h.press(handler,SelectInventorySlotAction{1});h.press(handler,EquipmentInventoryAction{});
  check(h.party->roster.at(0).accessories[1].frame!=0,"equipment publication");
  h.press(handler,CancelInteractionAction{});check(!h.flow->inventoryOpen(),"Esc closes items");
  h.press(handler,WaitAction{});
  const auto before=h.party->roster.characters();const auto revision=h.result().revision;const auto random=h.randomPosition();
  h.press(handler,SelectMemberAction{0});check(h.flow->inventoryOpen(),"combat sheet opens");
  h.press(handler,DialogKeyAction{'e'});h.press(handler,AcknowledgeAction{});
  h.press(handler,DialogKeyAction{'i'});h.press(handler,SelectInventorySlotAction{0});
  h.press(handler,DialogKeyAction{'e'});h.press(handler,AcknowledgeAction{});
  h.press(handler,DialogKeyAction{'r'});h.press(handler,AcknowledgeAction{});
  h.press(handler,SelectMemberAction{1});h.press(handler,AcknowledgeAction{});
  h.press(handler,DialogKeyAction{'m'});h.press(handler,DialogKeyAction{'u'});h.press(handler,AcknowledgeAction{});
  for(unsigned i=0;i<30;++i) check(xeen_state::sameCharacter(before[i],h.party->roster.at(i)),"combat view mutated character");
  check(h.result().revision==revision && h.randomPosition()==random,"combat view advanced authority/RNG");
  h.press(handler,CancelInteractionAction{});h.press(handler,CancelInteractionAction{});
  const auto ready=*handler.displayedInput();h.press(handler,BlockAction{});
  const auto after=h.result().generation;handler.withDisplayedInput(BlockAction{},ready);
  check(h.result().generation==after,"buffered old owner cannot block again");return true;
 };
 check(h.run(s)==0,"Regional inventory and combat viewing");
}
void staleService() {
 Harness h;auto s=h.services();bool fired=false;
 s.show=[&](const auto &,const auto &handler,const auto &,const auto &idle,const auto &){
  h.press(handler,WaitAction{});h.press(handler,InteractionAction{});
  check(h.fight().pending()==Work::Action&&h.flow->encounter()->deadline(),"Pending combat service");
  const auto before=h.result();const auto deadline=h.flow->encounter()->deadline();const auto rng=h.randomPosition();
  h.fight().setProbe([&]{if(!fired){fired=true;const auto lease=h.combatBoundary->hold(XeenCombatBoundary::Work::Inventory);h.combatBoundary->release(XeenCombatBoundary::Work::Inventory,lease);}});
  bool threw=false;try{h.now+=100;handler.beginCycle(++h.cycle);idle();}catch(const std::runtime_error &){threw=true;}
  h.fight().setProbe({});
  check(fired&&threw,"Stale service probe fired and stopped chain");
  check(h.randomPosition()==rng&&h.result().generation==before.generation&&h.result().revision==before.revision,"Stale service adopts no RNG/result");
  check(h.fight().pending()==Work::Action&&h.phase()==Phase::PreparingAction&&h.world->sessionState().actors()[5].hp==20,"Stale service preserves pending work and HP");
  check(h.flow->encounter()->deadline()==deadline,"Stale service preserves deadline");return true;
 };
 check(h.run(s)==0,"Stale service production route");
}
void startup() {
 for(unsigned fault=0;fault<3;++fault){Harness h;auto s=h.services();unsigned windows=0;
  if(fault==0)s.composeEncounter=[](auto &,const auto &,const auto &,auto,auto){return XeenEventFlow::Composition{};};
  if(fault==1)s.configureFlow=[](auto &,const auto &){throw std::runtime_error("Configuration failure");};
  if(fault==2)s.composeEncounter=[](auto &,const auto &,const auto &,auto,auto)->XeenEventFlow::Composition{throw std::runtime_error("Missing/malformed sprite");};
  s.show=[&](const auto &,const auto &,const auto &,const auto &,const auto &){++windows;return true;};
  check(h.run(s)==3&&!windows,"Startup failure precedes window exposure");
 }
}

void windowFailures() {
 for(unsigned mode=0;mode<5;++mode){Harness h;auto s=h.services();bool observed=false;
  s.show=[&](const auto &,const auto &handler,const auto &escape,const auto &idle,const auto &status){
   h.press(handler,NavigationAction::TurnRight);h.press(handler,NavigationAction::MoveForward);
   const std::vector<XeenActor> actors=h.world->sessionState().actors();const auto context=h.party->encounterContext;
   const auto pending=h.flow->encounter()->state().pending();check(pending==2,"Window exit starts with pending approach");
   unsigned failures=0,closes=0,cycles=0;auto wrapped=handler;
   wrapped.withPresentedInput=[&](const auto &a,auto input,const auto &origin){auto frame=handler.withPresentedInput(a,input,origin);if(mode==0&&frame)frame->width=319;return frame;};
   wrapped.failed=[&]{++failures;handler.failed();};wrapped.closed=[&]{++closes;handler.closed();};
   wrapped.beginCycle=[&](auto){check(++cycles==1,"Failed or closed window kept running");handler.beginCycle(++h.cycle);
    SDL_Event e{};if(mode==3)e.type=SDL_QUIT;else{e.type=SDL_KEYDOWN;e.key.keysym.sym=mode==4?SDLK_ESCAPE:SDLK_LEFT;}
    check(SDL_PushEvent(&e)==1,"Window failure event");};
   auto first=h.flow->frame();if(mode==2)first.pixels.clear();
   const bool ok=SdlWindow().showInteractive(first,"Regional window boundary",wrapped,escape,idle,[&]()->std::string{if(mode==1)throw std::runtime_error("Status failure");return status();});
   check(ok==(mode>=3)&&closes==1&&failures==(ok?0u:1u),"Exactly one failure/close notification");
   if(ok){sameActors(actors,h.world->sessionState().actors());check(context==h.party->encounterContext&&h.flow->encounter()->state().pending()==pending,"Quit/Escape supplied no final gameplay pulse");}
   check(!h.flow->canSave()&&!h.saves,"Closed window cannot save");
   const std::vector<XeenActor> closedActors=h.world->sessionState().actors();const auto closedContext=h.party->encounterContext;
   const auto closedRandom=h.world->sessionState().journeyRandom();h.now=10000;
   handler(WaitAction{});idle();handler(SaveGameAction{});
   sameActors(closedActors,h.world->sessionState().actors());
   check(h.party->encounterContext==closedContext&&h.world->sessionState().journeyRandom()==closedRandom&&!h.saves,"Inactive callbacks cannot advance or save");
   observed=true;return ok;
  };
  check(h.run(s)==(mode>=3?0:4)&&observed,"Window failure production route");
 }
}

void liveSourceGuards() {
 const auto disk=[](const auto &path){std::ifstream in(path,std::ios::binary);return Bytes(std::istreambuf_iterator<char>(in),{});};
 for(unsigned mode=0;mode<5;++mode){Harness h;auto s=h.services();bool armed=false,injected=false,observed=false;
  const auto compose=s.composeEncounter;
  s.composeEncounter=[&](auto &w,const auto &p,const auto &c,auto phase,auto actor){
   if(armed&&!injected){injected=true;
    if(mode<2){const auto &m=h.world->map(21);if(mode==1)const_cast<XeenMap &>(m).geometry.cells[0].rawWord^=1;}
    if(mode==2){auto *owner=const_cast<XeenPartyState *>(h.party);owner->~XeenPartyState();new(owner) XeenPartyState;}
    if(mode==3){h.world->~XeenWorld();new(h.world) XeenWorld(regional_test::map);}
    if(mode==4){auto *owner=const_cast<XeenGameFlags *>(h.flags);owner->~XeenGameFlags();new(owner) XeenGameFlags;}
   }return compose(w,p,c,phase,actor);
  };
  s.show=[&](const auto &,const auto &handler,const auto &,const auto &idle,const auto &){
   h.press(handler,NavigationAction::TurnRight);for(unsigned n=0;n<10&&!h.flow->canSave();++n)h.tick(handler,idle);
   check(h.flow->canSave(),"Source quiet before save");const auto previous=disk(h.savePath);armed=true;bool threw=false;
   try{h.press(handler,SaveGameAction{});}catch(const std::exception &){threw=true;}armed=false;
   check(injected,"Retained source cache/lifetime seam fired");
   if(mode==0){check(!threw&&h.flow->canSave(),"Trusted newly populated source cache admitted");check(disk(h.savePath)!=previous,"Valid new source cache save completed");}
   else check(threw&&disk(h.savePath)==previous,"Source corruption/replacement prevents write");
   observed=true;return mode==0;
  };
  const auto result=h.run(s);check(observed&&result==(mode==0?0:mode==1?4:3),"Source cache/lifetime exit boundary");
 }
}
int main(int argc,char **argv){try{
 if(argc==2&&std::string(argv[1])=="sdl"){sdl();windowFailures();}
 else if(argc==3&&std::string(argv[1])=="cli"){
  const auto root=child_test::freshDirectory(std::filesystem::current_path()/"combat-cli");
  unsigned sequence=0;
  for(const std::wstring mode:{L"--encounter-26",L"--encounter-27",L"--journey-skeleton",L"--journey-expedition"}){
   const auto result=child_test::launch(std::filesystem::absolute(argv[2]),{mode,L"missing"},root/(std::to_string(sequence++)+".log"));
   check(result.exit==1&&result.output.find("Usage:")!=std::string::npos,"Removed mode prints usage before resources");
  }
 }else{panelCombat();appearance();criticalPresentation();delayed();failures();publicationFailures();callbacks();inventory();staleService();startup();liveSourceGuards();}
 std::cout<<"Regional combat production tests passed\n";return 0;
}catch(const std::exception &e){std::cerr<<e.what()<<'\n';return 1;}}
