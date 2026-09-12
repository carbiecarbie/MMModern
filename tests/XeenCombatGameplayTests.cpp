#include "XeenCombatGameplayTestSupport.h"
#include "XeenCombatTestSupport.h"
#include "XeenSaveGameplayTestSupport.h"
#include "XeenChildProcessTestSupport.h"
#include "formats/xeen/XeenAssetSource.h"
#include "games/xeen/XeenInstallationDetector.h"
#include "games/xeen/XeenMapLoader.h"
#include "games/xeen/XeenEventLoader.h"
#include "games/xeen/XeenEventTextLoader.h"
#include "games/xeen/CloudsMapComposer.h"
#include "games/xeen/CloudsUiComposer.h"
#define SDL_MAIN_HANDLED
#include <SDL.h>
#include <iostream>
#include <fstream>

using namespace combat_test;
namespace {
std::optional<std::filesystem::path> evidence;
void ppm(const std::string &name, const IndexedFrame &f) {
 if(!evidence)return;
 std::filesystem::create_directories(*evidence);
 std::ofstream out(*evidence/(name+".ppm"),std::ios::binary);
 out<<"P6\n"<<f.width<<' '<<f.height<<"\n255\n";
 for(auto p:f.pixels)out.write(reinterpret_cast<const char*>(f.palette.data()+3*p),3);
 check(bool(out),"image output");
}
using combat_gameplay_test::Harness;

void appearance(const std::optional<std::filesystem::path> &game={}) {
 Harness h(game);auto s=h.services();
 const auto compose=s.composeEncounter;bool delayedFrame=false;
 s.composeEncounter=[&](auto &w,const auto &p,const auto &c,auto ordinary,auto actor){
  auto result=compose(w,p,c,ordinary,actor);
  if(actor.kind==XeenMonsterSpriteKind::Attack&&!delayedFrame){h.now+=250;delayedFrame=true;}
  return result;
 };
 s.show=[&](const IndexedFrame &,const auto &handler,const auto &,const auto &idle,const auto &){
  ppm("preparation",h.flow->frame());
  h.press(handler,AcknowledgeAction{});ppm("approach",h.flow->frame());
  h.press(handler,WaitAction{});
  auto frame=[&](unsigned expected){
   const auto value=h.observedAppearance;
   check(value.frame==(expected<8?expected:expected-8) && value.kind==
    (expected<8?XeenMonsterSpriteKind::Normal:XeenMonsterSpriteKind::Attack),"production MON/ATT sequence");
   ppm("logical-"+std::to_string(expected),h.flow->frame());
   if(h.assets){
    const auto resolver=XeenObjectVisualResolver::load(*h.assets);
    auto commands=XeenOutdoorScene().build(*h.world,XeenActorApproach::kEntry,&resolver,nullptr,h.observedOrdinary,value);
    unsigned actors=0;
    int previous=-1;
    for(const auto &command:commands){
     check(command.originalOrder>=previous,"shared scene order");previous=command.originalOrder;
     if(command.actor()){
      ++actors;check(command.originalOrder==(expected<8?118:121)&&command.x==-5&&command.y==2&&
       command.actor()->frame==value.frame&&command.actor()->kind==value.kind,"production command appearance/order");
     }
    }
    check(actors==1,"one original selected actor");
    commands.erase(std::remove_if(commands.begin(),commands.end(),[](const auto &c){return c.actor()!=nullptr;}),commands.end());
    CloudsUiComposer().loadBackground(*h.assets);
    CloudsMapComposer composer;composer.drawOutdoorCommands(*h.assets,commands);composer.drawInterfaceLayers(*h.assets,*h.party,{610});
    const auto omitted=h.assets->snapshot();unsigned visible=0;
    for(int y=0;y<200;++y)for(int x=0;x<320;++x){
     const auto index=y*320+x;
     if(h.base.pixels[index]!=omitted.pixels[index]){
      check(x>=8&&x<223&&y>=8&&y<140,"original actor raster escaped scene/bottom clip");
      if(y<135&&h.flow->frame().pixels[index]!=omitted.pixels[index])++visible;
     }
    }
    check(visible>500,"original actor unreadable after final panels");
    std::cout<<"logical="<<expected<<" final visible actor pixels="<<visible<<'\n';
   }
  };
  auto rebuild=[&]{
   const auto pixels=h.flow->frame().pixels;
   const auto deadline=h.flow->encounter()->cosmeticDeadline();
   const auto service=h.flow->encounter()->deadline();
   const auto revision=h.result().revision, rng=h.randomPosition();
   const auto hp=h.world->sessionState().actors()[5].hp;
   h.world->discardMapCache();if(h.assets)h.assets->discardSpriteCache();
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
 check(Application().playGameplay(s,XeenActorApproach::kEntry,{},false,XeenEncounterEntry::Diagnostic27)==0,"production appearance");
}

void attackAdmission() {
 for(unsigned failure=0;failure<3;++failure){
  Harness h;auto s=h.services();bool shown=false,checked=false;
  s.show=[&](const auto &,const auto &,const auto &,const auto &,const auto &){shown=true;return true;};
  if(!failure)s.validateCombatSprite={};
  else s.validateCombatSprite=[&](std::uint8_t image){
   checked=true;check(image==8&&h.phase()==Phase::Preparation,"typed attack admission before gameplay");
   if(failure==1)throw std::runtime_error("invalid attack sprite");
   h.fight().invalidate();
  };
  check(Application().playGameplay(s,XeenActorApproach::kEntry,{},false,XeenEncounterEntry::Diagnostic27)==3&&
   !shown&&!h.saves&&(checked==(failure!=0)),"attack admission failure/stale startup boundary");
 }
}

void criticalPresentation(const std::optional<std::filesystem::path> &game={}) {
 Harness h(game);
 std::vector<XeenCombatRandom::Draw> tape;
 for(unsigned round=0;round<6;++round){
  const unsigned targets[]{0,0,1,2,3,5};
  if(round)tape.push_back({0,5,targets[round]});
  tape.push_back({1,20,20});tape.push_back({1,6,6});tape.push_back({1,6,6});tape.push_back({1,4,4});
  if(round!=1){tape.push_back({1,6,6});tape.push_back({1,6,6});}
 }
 h.random.emplace(tape);auto s=h.services();unsigned enemies=0;
 s.show=[&](const IndexedFrame &,const auto &handler,const auto &,const auto &idle,const auto &){
  h.press(handler,AcknowledgeAction{});h.press(handler,WaitAction{});
  for(unsigned step=0;step<100&&!h.flow->encounter()->terminal();++step){
   if(h.phase()==Phase::PlayerReady)h.press(handler,BlockAction{});
   else {
    h.tick(handler,idle);const auto &r=h.result();
    if(r.operation==XeenCombatOperation::EnemyAttack){
     ++enemies;check(r.critical&&r.injuryCount==(enemies==2?1U:2U),"ordered critical presentation observations");
     check(h.observedAppearance.kind==XeenMonsterSpriteKind::Attack&&h.observedAppearance.frame==0,"critical ATT initial frame");
     ppm("critical-round-"+std::to_string(enemies),h.flow->frame());
     if(enemies==1)check(r.injuries[0].afterHp==-5&&r.injuries[1].afterHp==-17&&r.armorCount==2,"intermediate injury and armor facts");
    }
   }
  }
  check(enemies==6&&h.phase()==Phase::Defeat&&h.party->encounterContext->minutes==495,"critical production defeat");
  const auto &notice=h.flow->encounter()->notice();
  check(notice.find("Uncon. + Dead")!=std::string::npos&&notice.find("Broken armor:")!=std::string::npos,"complete condition/breakage feedback");
  return true;
 };
 check(Application().playGameplay(s,XeenActorApproach::kEntry,{},false,XeenEncounterEntry::Diagnostic27)==0,"critical presentation production route");
}

void preparation() {
 Harness h;auto s=h.services();
 const auto save=std::filesystem::temp_directory_path()/("combat-save-"+std::to_string(GetCurrentProcessId())+".mmsave");
 {std::ofstream out(save,std::ios::binary);out<<"unchanged existing save";}
 s.show=[&](const IndexedFrame &,const auto &handler,const auto &escape,const auto &idle,const auto &status){
  check(!escape()&&h.phase()==Phase::Preparation,"preparation and top-level escape");
  check(h.world->sessionState().actors().empty()&&!h.flow->encounter()->deadline(),"no preparation actors/deadlines");
  const auto ticket=*handler.displayedInput();
  handler(AcknowledgeAction{});h.flow->handle(BeginEncounterAction{});
  check(h.phase()==Phase::Preparation,"unticketed service and Flow bypass refused");
  h.tick(handler,idle);check(h.world->sessionState().actors().empty(),"idle does not begin actors");
  h.press(handler,InspectInventoryAction{});
  handler.withDisplayedInput(AcknowledgeAction{},ticket);
  check(h.flow->inventoryOpen(),"stale begin refused");
  h.press(handler,SelectMemberAction{5});
  h.press(handler,NavigationAction::TurnRight);h.press(handler,NavigationAction::TurnRight);
  h.press(handler,SelectInventorySlotAction{1});
  h.press(handler,TransferInventoryAction{});
  check(h.flow->inventorySelection().mode==XeenInventoryMode::ChooseDestination,"real transfer selection");
  h.press(handler,InspectInventoryAction{});
  check(h.flow->inventorySelection().mode==XeenInventoryMode::Browse,"I cancels to Browse");
  h.press(handler,TransferInventoryAction{});h.press(handler,SelectMemberAction{0});
  check(h.flow->inventoryConfirmation().has_value(),"confirmation armed");
  h.press(handler,NoAction{});check(!h.flow->inventoryConfirmation(),"N consumes confirmation");
  h.press(handler,SelectInventorySlotAction{1});h.press(handler,TransferInventoryAction{});h.press(handler,SelectMemberAction{0});
  h.press(handler,InspectInventoryAction{});
  check(h.flow->inventoryOpen()&&!h.flow->inventoryConfirmation(),"I confirm cancellation stays open");
  h.press(handler,SelectInventorySlotAction{1});h.press(handler,TransferInventoryAction{});h.press(handler,SelectMemberAction{0});
  h.press(handler,AcknowledgeAction{});
  check(h.flow->transferResult().status==XeenTransferStatus::Success&&h.phase()==Phase::Preparation,"Enter transfers only");
  h.press(handler,SelectMemberAction{0});h.press(handler,SelectInventorySlotAction{1});h.press(handler,EquipmentInventoryAction{});
  check(h.party->roster.at(0).accessories[1].frame!=0,"coordinator equipment publication");
  h.press(handler,InspectInventoryAction{});
  check(!h.flow->inventoryOpen(),"I closes Browse");
  h.press(handler,SaveGameAction{});check(!h.saves&&status().find("unsaveable")!=std::string::npos,"zero save stages");
  check(status().find(save.filename().string())==std::string::npos,"save refusal precedes target formatting");
  h.press(handler,AcknowledgeAction{});check(h.phase()==Phase::Approach,"single-use Begin");
  h.press(handler,AcknowledgeAction{});h.press(handler,InspectInventoryAction{});check(!h.flow->inventoryOpen(),"no return to preparation");
  h.press(handler,WaitAction{});check(h.phase()==Phase::PlayerReady,"automatic handoff");
  check(h.party->encounterContext->minutes==490&&!h.flow->encounter()->deadline(),"handoff retires approach work");
  const auto ready=*handler.displayedInput();
  h.press(handler,BlockAction{});
  const auto after=h.result().generation;
  handler.withDisplayedInput(BlockAction{},ready);
  check(h.result().generation==after,"buffered old owner cannot block again");
  return true;
 };
 check(Application().playGameplay(s,XeenActorApproach::kEntry,save,false,XeenEncounterEntry::Diagnostic27)==0,"preparation production route");
 std::ifstream input(save,std::ios::binary);check(std::string(std::istreambuf_iterator<char>(input),{})=="unchanged existing save","F9 preserves existing save bytes");
}
void delayed() {
 Harness h;auto s=h.services();
 s.show=[&](const IndexedFrame &,const auto &handler,const auto &,const auto &idle,const auto &){
  h.press(handler,AcknowledgeAction{});
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
 check(Application().playGameplay(s,XeenActorApproach::kEntry,{},false,XeenEncounterEntry::Diagnostic27)==0,"delayed production route");
}
void staleCoordination() {
 for(unsigned mode=0;mode<4;++mode) {
  Harness h;auto s=h.services();bool observed=false;
  s.show=[&](const IndexedFrame &,const auto &handler,const auto &,const auto &idle,const auto &){
   h.press(handler,AcknowledgeAction{});
   if(mode!=2) {
    h.press(handler,NavigationAction::TurnRight);
    h.press(handler,NavigationAction::MoveForward);
    check(h.flow->encounter()->state().pending()==2&&h.flow->encounter()->deadline(),"approach stale fixture pending work");
   } else if(mode==2) {
    h.press(handler,WaitAction{});
    check(h.phase()==Phase::PlayerReady,"service stale fixture entered combat");
    h.press(handler,InteractionAction{});
    check(h.fight().pending()==Work::Action&&h.flow->encounter()->deadline(),"service stale fixture pending work");
   }
   const auto revision=mode==2?h.result().revision:h.flow->encounter()->state().revision();
   const auto pending=mode==2?static_cast<unsigned>(h.fight().pending()):h.flow->encounter()->state().pending();
   const auto deadline=h.flow->encounter()->deadline();
   const auto resultGeneration=h.result().generation;
   const auto randomPosition=h.randomPosition();
   unsigned probes=0;
   h.fight().setProbe([&]{
    ++probes;
    if((mode<3&&probes==1)||(mode==3&&probes==2)) {
     const auto lease=h.combatBoundary->hold(XeenCombatBoundary::Work::Inventory);
     h.combatBoundary->release(XeenCombatBoundary::Work::Inventory,lease);
    }
   });
   bool threw=false;
   try {
    if(mode==0||mode==3) {
     handler.beginCycle(++h.cycle);
     handler.withDisplayedInput(mode==0?PlayerAction{NavigationAction::TurnLeft}:PlayerAction{WaitAction{}},
      *handler.displayedInput());
    } else {
     h.now+=100;handler.beginCycle(++h.cycle);idle();
    }
   } catch(const std::runtime_error &) { threw=true; }
   h.fight().setProbe({});
   check(threw,"stale production operation stops its call chain");
   check(h.randomPosition()==randomPosition,"stale production operation adopted RNG");
   if(mode!=3) check(h.result().generation==resultGeneration,
    "stale production operation adopted result");
   check(h.flow->encounter()->deadline()==deadline,"stale production operation altered deadline");
   if(mode==2) {
    check(h.result().revision==revision&&static_cast<unsigned>(h.fight().pending())==pending&&
     h.phase()==Phase::PreparingAction&&h.world->sessionState().actors()[5].hp==20,
     "stale service consumed automatic combat work");
   } else if(mode==3) {
    check(h.flow->encounter()->state().revision()==revision+1&&h.flow->encounter()->state().pending()==0&&
     h.phase()==Phase::Engaged&&h.result().revision==revision+1&&
     h.result().operation==XeenCombatOperation::ApproachAction&&
     h.result().generation==resultGeneration+1,
     "stale handoff changed the accepted engagement or entered combat");
   } else {
    check(h.flow->encounter()->state().revision()==revision&&h.flow->encounter()->state().pending()==pending&&
     h.phase()==Phase::Approach&&!h.world->sessionState().encounterTerminal(),
     "stale approach operation advanced or retired pending work");
   }
   observed=true;return true;
  };
  check(Application().playGameplay(s,XeenActorApproach::kEntry,{},false,XeenEncounterEntry::Diagnostic27)==0&&observed,
   "stale production coordination route");
 }
}
void automaticSupportStop() {
 Harness h;auto s=h.services();
 s.show=[&](const IndexedFrame &,const auto &handler,const auto &,const auto &idle,const auto &){
  h.press(handler,AcknowledgeAction{});h.press(handler,NavigationAction::TurnRight);h.press(handler,NavigationAction::MoveForward);
  const auto revision=h.flow->encounter()->state().revision();
  check(h.flow->encounter()->deadline()&&h.flow->encounter()->state().pending()==2,"support-stop fixture pending work");
  h.badTerrain=true;h.world->discardMapCache();h.tick(handler,idle);
  check(h.phase()==Phase::SupportStopped&&h.flow->encounter()->state().revision()==revision+1&&
   h.flow->encounter()->state().pending()==0&&!h.flow->encounter()->deadline(),
   "current automatic support stop did not retire only its own work");
  return true;
 };
 check(Application().playGameplay(s,XeenActorApproach::kEntry,{},false,XeenEncounterEntry::Diagnostic27)==0,
  "automatic support-stop production route");
}
void outcome(bool loss,const std::optional<std::filesystem::path> &game={}) {
 Harness h(game);auto s=h.services(loss?19:1);unsigned commands=0;
 bool criticalImage=false,zeroImage=false;
 s.show=[&](const IndexedFrame &,const auto &handler,const auto &,const auto &idle,const auto &){
  if(loss){
   h.press(handler,InspectInventoryAction{});h.press(handler,NavigationAction::TurnRight);
   for(unsigned i=0;i<6;++i){
    h.press(handler,SelectMemberAction{i});
    for(unsigned j=0;j<9;++j)if(h.party->roster.at(kXeenCombatOwners[i]).armor[j].id){
     h.press(handler,SelectInventorySlotAction{j});h.press(handler,EquipmentInventoryAction{});
    }
   }
   h.press(handler,InspectInventoryAction{});
  }
  h.press(handler,AcknowledgeAction{});h.press(handler,WaitAction{});
  for(unsigned n=0;n<500&&h.phase()!=Phase::Victory&&h.phase()!=Phase::Defeat;++n){
   check(!h.flow->encounter()->terminal(),"unexpected combat failure");
   h.press(handler,SaveGameAction{});check(h.saves==0,"zero save side effects in every phase");
   if(h.phase()==Phase::PlayerReady){
    h.press(handler,loss||commands<6?PlayerAction{BlockAction{}}:PlayerAction{InteractionAction{}});++commands;
   }else{
    const auto before=h.result().generation;
    h.press(handler,InteractionAction{});
    check(before==h.result().generation,"pending input does not replace automatic work");
    h.tick(handler,idle);
    const auto &r=h.result();
    if(r.critical&&!criticalImage){ppm(loss?"loss-critical":"victory-critical",h.flow->frame());criticalImage=true;}
    if(r.attackOutcome==XeenCombatAttackOutcome::HitZeroDamage&&!zeroImage){ppm("zero-damage",h.flow->frame());zeroImage=true;}
    const auto after=h.result().generation;idle();
    check(after==h.result().generation,"same time/cycle cannot repeat automatic work");
   }
  }
  check(h.phase()==(loss?Phase::Defeat:Phase::Victory),"real production terminal outcome");
  ppm(loss?"defeat":"victory",h.flow->frame());
  if(game){
   check(commands==(loss?35U:15U),"seeded command count");
   const int won[]{12,16,12,10,-4,5},lost[]{-7,-1,-1,-5,-3,-7};
   for(unsigned i=0;i<6;++i)check(h.party->roster.at(kXeenCombatOwners[i]).currentHp==(loss?lost[i]:won[i]),"seeded final HP");
  }
  const auto pixels=h.flow->frame().pixels;
  const auto rng=h.randomPosition();
  h.world->discardMapCache();if(h.assets)h.assets->discardSpriteCache();h.flow->refresh(true);handler.framePresented();
  check(h.flow->frame().pixels==pixels && h.randomPosition()==rng,"terminal cache reconstruction");
  if(game)check(h.party->encounterContext->minutes==(loss?500:493),"original seeded time");
  const auto terminal=h.result().generation;
  for(const PlayerAction action:std::initializer_list<PlayerAction>{InteractionAction{},BlockAction{},AcknowledgeAction{},InspectInventoryAction{},WaitAction{},SaveGameAction{}})
   h.press(handler,action);
  h.tick(handler,idle);
  check(h.result().generation==terminal&&!h.saves,"terminal stays terminal and unsaveable");
  check(h.flow->encounter()->notice().find(loss?"DEFEAT":"Victory completed")!=std::string::npos,"in-frame terminal text");
  if(!loss)check(h.flow->encounter()->notice().find(h.flow->completed()?"XP 82":"XP +82")!=std::string::npos,"fixed award retained through End");
  std::cout<<(loss?"Defeat":"Victory")<<" commands="<<commands<<" time="<<h.party->encounterContext->minutes<<'\n';
  return true;
 };
 check(Application().playGameplay(s,XeenActorApproach::kEntry,{},false,XeenEncounterEntry::Diagnostic27)==0,"production fight");
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
    h.press(handler,InspectInventoryAction{});h.press(handler,SelectInventorySlotAction{0});h.press(handler,EquipmentInventoryAction{});
    check(h.party->roster.at(0).weapons[0].frame==0,"published equipment survives reporting failure");
   }else{
    h.press(handler,AcknowledgeAction{});h.press(handler,WaitAction{});
    if(fault==4){h.now=std::numeric_limits<std::uint64_t>::max();injected=true;}
    h.press(handler,InteractionAction{});
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
   check(injected&&h.phase()==Phase::Failed,"current failure stops combat");
   check(h.randomPosition()==0,"failure did not roll pending player attack");
   h.press(handler,SaveGameAction{});check(h.saves==0,"failure is unsaveable");
   return true;
  };
  const auto label="failure boundary route "+std::to_string(fault);
  check(Application().playGameplay(s,XeenActorApproach::kEntry,{},false,XeenEncounterEntry::Diagnostic27)==0,label.c_str());
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
    h.press(handler,InspectInventoryAction{});h.press(handler,NavigationAction::TurnRight);
    for(unsigned i=0;i<6;++i){h.press(handler,SelectMemberAction{i});for(unsigned j=0;j<9;++j)
     if(h.party->roster.at(kXeenCombatOwners[i]).armor[j].id){h.press(handler,SelectInventorySlotAction{j});h.press(handler,EquipmentInventoryAction{});}}
    h.press(handler,InspectInventoryAction{});
   }
   h.press(handler,AcknowledgeAction{});h.press(handler,WaitAction{});
   for(unsigned n=0;n<500&&!injected;++n){
    if(h.phase()==Phase::PlayerReady){h.press(handler,fault==3||commands++<6?PlayerAction{BlockAction{}}:PlayerAction{InteractionAction{}});}
    else h.tick(handler,idle);
   }
   check(injected,"publication fault reached through real commands");
   check(h.phase()==(fault==2?Phase::Victory:fault==3?Phase::Defeat:Phase::Failed),"failure preserves actual terminal boundary");
   const auto hp=h.world->sessionState().actors()[5].hp;
   check(fault==3||hp<20,"published damage survives failure");
   if(fault==1||fault==2){check(hp==0,"lethal removal retained");check(h.flow->encounter()->notice().find(h.flow->completed()?"XP 82":"XP +82")!=std::string::npos,"award retained despite composition failure");}
   h.press(handler,SaveGameAction{});h.tick(handler,idle);check(h.saves==0&&h.world->sessionState().actors()[5].hp==hp,"no save or replay after failure");
   return true;
  };
  check(Application().playGameplay(s,XeenActorApproach::kEntry,{},false,XeenEncounterEntry::Diagnostic27)==0,"post-publication failure route");
 }
}
void sdl() {
 Harness h;auto s=h.services();unsigned stage=0,accepted=0,idles=0;
 s.show=[&](const IndexedFrame &first,const auto &handler,const auto &escape,const auto &idle,const auto &status){
  auto wrapped=handler;
  wrapped.withDisplayedInput=[&](const PlayerAction &a,std::uint64_t t){
   const auto before=h.result().generation;
   auto result=handler.withDisplayedInput(a,t);
   if(std::holds_alternative<BlockAction>(a)&&before!=h.result().generation)++accepted;
   return result;
  };
  auto key=[](SDL_Keycode code,Uint32 type=SDL_KEYDOWN,Uint8 repeat=0){
   SDL_Event event{};event.type=type;event.key.keysym.sym=code;event.key.repeat=repeat;
   check(SDL_PushEvent(&event)==1,"push production SDL key");
  };
  const auto scriptedIdle=[&]()->std::optional<IndexedFrame>{
   h.now+=20;++idles;
   auto frame=idle();
   if(idles>100)throw std::runtime_error("SDL combat test timed out");
   switch(stage++){
    case 0:key(SDLK_RETURN);break;
    case 1:check(h.phase()==Phase::Approach,"SDL Begin");key(SDLK_PERIOD);break;
    case 2:check(h.phase()==Phase::PlayerReady,"SDL handoff");break;
    case 3:key(SDLK_b);key(SDLK_b,SDL_KEYDOWN,1);key(SDLK_b,SDL_KEYUP);key(SDLK_b);break;
    case 4:check(accepted==1,"one owner per poll batch");key(SDLK_b);break;
    case 5:check(accepted==1,"held B requires release");key(SDLK_b,SDL_KEYUP);break;
    case 6:key(SDLK_b);break;
    case 7:check(accepted==2,"fresh B accepts next displayed owner");key(SDLK_b,SDL_KEYUP);key(SDLK_SPACE);break;
    case 8:key(SDLK_SPACE,SDL_KEYUP);break;
    case 9:break;
    case 10:key(SDLK_SPACE);break;
    default:
     if(h.phase()==Phase::PlayerReady){
      check(h.randomPosition()>0,"automatic attack runs without another key");
      key(SDLK_ESCAPE);
     }
   }
   return frame;
  };
  return SdlWindow().showInteractive(first,"M27 production input",wrapped,escape,scriptedIdle,status);
 };
 check(Application().playGameplay(s,XeenActorApproach::kEntry,{},false,XeenEncounterEntry::Diagnostic27)==0,"real SDL production route");
 check(stage>=11&&accepted==2,"SDL sequence completed");
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
     h.press(handler,NavigationAction::TurnRight);h.press(handler,NavigationAction::TurnRight);
     h.press(handler,SelectInventorySlotAction{1});h.press(handler,TransferInventoryAction{});
     h.press(handler,SelectMemberAction{0});h.press(handler,AcknowledgeAction{});
     check(h.party->roster.at(0).accessories[1].material==86,"transfer survives reporter failure");
    }else{
     h.press(handler,AcknowledgeAction{});h.press(handler,WaitAction{});armed=true;
     h.press(handler,InteractionAction{});
    }
   }catch(const std::exception &){
    check(fault==1||fault==2||fault==3,"unexpected callback exit");
   }
   observed=true;
   check(injected&&h.phase()==Phase::Failed,"callback failure retains its legitimate stop");
   check(h.randomPosition()==0&&h.saves==0,"callback failure cannot roll or save");
   return fault==0||fault==4;
  };
  const int result=Application().playGameplay(s,XeenActorApproach::kEntry,{},false,XeenEncounterEntry::Diagnostic27);
  check(observed&&result==((fault==0||fault==4)?0:4),"callback production exit boundary");
 }
}
void cli(const std::filesystem::path &exe) {
 const auto directory=std::filesystem::temp_directory_path()/("mmodern-combat-cli-"+std::to_string(GetCurrentProcessId()));
 std::filesystem::create_directories(directory);
 const std::vector<std::vector<std::wstring>> invalid{
  {L"--combat-seed",L"1",L"missing"},{L"--encounter-27"},
  {L"--load-game",L"missing",L"save",L"--combat-seed",L"1"},
  {L"--load-game",L"missing",L"save",L"--encounter-27"},
  {L"--encounter-27",L"--save-file",L"save",L"missing"},
  {L"--encounter-27",L"--encounter-27",L"missing"},{L"--encounter-27",L"missing",L"extra"},
  {L"--encounter-27",L"--encounter-26",L"missing"},
  {L"--encounter-27",L"--render-map",L"missing"},
  {L"--encounter-27",L"--load-game",L"missing"},
  {L"--encounter-27",L"missing",L"--save-file",L""},
  {L"--encounter-27",L"missing",L"--save-file",L"untouched",L"--save-file",L"again"},
  {L"--encounter-27",L"missing",L"20",L"13",L"1",L"north"},
  {L"--encounter-27",L"--combat-seed",L"1",L"--combat-seed",L"2",L"missing"}
 };
 unsigned index=0;
 auto reject=[&](const auto &args){
  const auto result=child_test::launch(exe,args,directory/(std::to_string(index++)+".log"));
  check(result.exit==1&&result.output.find("Usage: --encounter-27")!=std::string::npos,"strict CLI rejection before resources");
 };
 for(const auto &args:invalid)reject(args);
 for(const std::wstring seed:{L"0",L"4294967296",L"-1",L"+1",L" 1",L"1x",L"",L"999999999999999999999999"})
  reject(std::vector<std::wstring>{L"--encounter-27",L"--combat-seed",seed,L"missing"});
}
}
int main(int argc,char **argv){try{
 if(argc==2&&std::string(argv[1])=="sdl")sdl();
 else if(argc==3&&std::string(argv[1])=="cli")cli(std::filesystem::path(argv[2]));
 else if(argc==2||argc==3){if(argc==3)evidence=std::filesystem::path(argv[2]);
  appearance(std::filesystem::path(argv[1]));outcome(false,std::filesystem::path(argv[1]));outcome(true,std::filesystem::path(argv[1]));criticalPresentation(std::filesystem::path(argv[1]));}
 else {attackAdmission();preparation();delayed();staleCoordination();automaticSupportStop();appearance();outcome(false);outcome(true);criticalPresentation();failures();publicationFailures();callbacks();}
 std::cout<<"Combat production tests passed\n";return 0;
}catch(const std::exception &e){std::cerr<<e.what()<<'\n';return 1;}}
