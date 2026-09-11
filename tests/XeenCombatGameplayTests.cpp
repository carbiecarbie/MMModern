#include "XeenCombatTestSupport.h"
#include "XeenSaveGameplayTestSupport.h"
#include "XeenChildProcessTestSupport.h"
#include "formats/xeen/XeenAssetSource.h"
#include "games/xeen/XeenInstallationDetector.h"
#include "games/xeen/XeenMapLoader.h"
#include "games/xeen/XeenEventLoader.h"
#include "games/xeen/XeenEventTextLoader.h"
#include "games/xeen/CloudsMapComposer.h"
#define SDL_MAIN_HANDLED
#include <SDL.h>
#include <iostream>
#include <fstream>

using namespace combat_test;
namespace {
struct Harness {
 XeenFontFormat font{gameplay_test::fontBytes()};
 std::unique_ptr<XeenAssetSource> assets;
 std::unique_ptr<XeenEventLoader> eventLoader;
 XeenMapLoader mapLoader;
 XeenEventFlow *flow=nullptr;
 XeenCombat *combat=nullptr;
 XeenCombatBoundary *combatBoundary=nullptr;
 const XeenPartyState *party=nullptr;
 XeenWorld *world=nullptr;
 std::uint64_t now=0,cycle=0;
 unsigned saves=0,compositions=0;
 std::uint64_t observedOrdinary=0;
 bool badTerrain=false;
 explicit Harness(const std::optional<std::filesystem::path> &game={}) {
  if (!game) return;
  const auto installation=XeenInstallationDetector().detect(*game);
  check(installation&&installation->hasDarkside(),"World of Xeen required");
  assets=std::make_unique<XeenAssetSource>(*installation,320,200);
  font=XeenFontFormat(assets->readArchiveResource("fnt"));
  eventLoader=std::make_unique<XeenEventLoader>([&](const std::string &name)->std::optional<Bytes>{
   if(!assets->hasInitialResource(name))return {};return assets->readInitialResource(name);});
 }
 XeenGameplayServices services(unsigned seed=1) {
  XeenGameplayServices s{
   {{{1,2},{}},[&]{return assets?XeenPartyLoader().loadInitialCloudsParty(*assets):XeenPartyLoader().loadFromResources(chr(),pty());},
    [&](XeenMapIdentity id){return assets?eventLoader->load(id):events();}},
   []{return XeenGameFlags{};},
   [&](XeenMapIdentity id){
    if(assets)return mapLoader.loadGeometryMap(*assets,id);
    auto value=map();if(badTerrain)value.geometry.flags=1;return value;
   },
   [&](XeenMapIdentity id){return assets?mapLoader.loadObjects(*assets,id):objects();},
   [](XeenMapIdentity){return XeenEventTextFile{};},font,
   [](XeenWorld &,const XeenPartyState &,const XeenCamera &,std::uint64_t)->XeenEventFlow::Composition{
    throw std::runtime_error("Combat routed through ordinary composition");}, {},
   [&](XeenEventFlow &f,const XeenCamera &){flow=&f;}, {},
   [&](XeenWorld &w,XeenEventSystem &,const XeenPartyState &p,XeenCamera &,const XeenGameFlags &){world=&w;party=&p;}
  };
  s.clock=[&]{return now;};
  s.prepareCombat=[&,seed](XeenWorld &w,XeenPartyState &p,XeenCamera &c,XeenCombatBoundary &b){
   check(w.sessionState().encounterEntry()==XeenEncounterEntry::Diagnostic27,"typed reservation precedes providers");
   const auto bytes=assets?assets->readInitialResource("maze.chr"):chr();
   const auto context=XeenGameplayContextFormat::parse(assets?assets->readInitialResource("maze.pty"):pty());
   const auto stats=assets?XeenMonsterFormat::parse(*assets->readCloudsMonsterStatisticsFromDarkArchive()):statistics();
   auto value=std::make_unique<XeenCombat>(w,p,c,b,bytes,context,stats,assets?eventLoader->load(20):events(),XeenCombatRandom(seed));
   combat=value.get();combatBoundary=&b;
   return value;
  };
  s.validateEncounterSprite=[&](std::uint8_t image){if(assets)assets->validateNormalMonster(image);};
  s.composeEncounter=[&](XeenWorld &w,const XeenPartyState &p,const XeenCamera &c,std::uint64_t ordinary,std::uint8_t actor){
   ++compositions;
   observedOrdinary=ordinary;
   check(p.roster.combatMarked(),"composition borrows marked roster");
   XeenEventFlow::Composition out;
   if(assets)out.frame=CloudsMapComposer().compose(*assets,w,p,c,{610},nullptr,ordinary,&out.containsOrdinaryAnimation,actor);
   else {out.frame.width=320;out.frame.height=200;out.frame.pixels.resize(64000);out.containsOrdinaryAnimation=true;}
   return out;
  };
  s.observeSaveStage=[&](auto){++saves;};
  return s;
 }
 const XeenCombat &fight() const {return *combat;}
 XeenCombat &fight() {return *combat;}
 void press(const SdlWindow::FrameUpdateHandler &handler,const PlayerAction &action) {
  handler.beginCycle(++cycle);
  check(handler.displayedInput().has_value(),"displayed ticket exists");
  handler.withDisplayedInput(action,*handler.displayedInput());
  check(handler.frameCurrent(),"current returned frame");
 }
 void tick(const SdlWindow::FrameUpdateHandler &handler,const SdlWindow::IdleFrameHandler &idle) {
  now+=100;handler.beginCycle(++cycle);idle();check(handler.frameCurrent(),"current idle frame");
 }
};

void preparation() {
 Harness h;auto s=h.services();
 const auto save=std::filesystem::temp_directory_path()/("combat-save-"+std::to_string(GetCurrentProcessId())+".mmsave");
 {std::ofstream out(save,std::ios::binary);out<<"unchanged existing save";}
 s.show=[&](const IndexedFrame &,const auto &handler,const auto &escape,const auto &idle,const auto &status){
  check(!escape()&&h.fight().phase()==Phase::Preparation,"preparation and top-level escape");
  check(h.world->sessionState().actors().empty()&&!h.flow->encounter()->deadline(),"no preparation actors/deadlines");
  const auto ticket=*handler.displayedInput();
  handler(AcknowledgeAction{});h.flow->handle(BeginEncounterAction{});
  check(h.fight().phase()==Phase::Preparation,"unticketed service and Flow bypass refused");
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
  check(h.flow->transferResult().status==XeenTransferStatus::Success&&h.fight().phase()==Phase::Preparation,"Enter transfers only");
  h.press(handler,SelectMemberAction{0});h.press(handler,SelectInventorySlotAction{1});h.press(handler,EquipmentInventoryAction{});
  check(h.party->roster.at(0).accessories[1].frame!=0,"coordinator equipment publication");
  h.press(handler,InspectInventoryAction{});
  check(!h.flow->inventoryOpen(),"I closes Browse");
  h.press(handler,SaveGameAction{});check(!h.saves&&status().find("unsaveable")!=std::string::npos,"zero save stages");
  check(status().find(save.filename().string())==std::string::npos,"save refusal precedes target formatting");
  h.press(handler,AcknowledgeAction{});check(h.fight().phase()==Phase::Approach,"single-use Begin");
  h.press(handler,AcknowledgeAction{});h.press(handler,InspectInventoryAction{});check(!h.flow->inventoryOpen(),"no return to preparation");
  h.press(handler,WaitAction{});check(h.fight().phase()==Phase::PlayerReady,"automatic handoff");
  check(h.party->encounterContext->minutes==490&&!h.flow->encounter()->deadline(),"handoff retires approach work");
  const auto ready=*handler.displayedInput();
  h.press(handler,BlockAction{});
  const auto after=h.fight().result().generation;
  handler.withDisplayedInput(BlockAction{},ready);
  check(h.fight().result().generation==after,"buffered old owner cannot block again");
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
  check(h.fight().phase()==Phase::PlayerReady&&h.party->encounterContext->minutes==500,"delayed production engagement at500");
  const auto ordinary=h.observedOrdinary;
  h.press(handler,BlockAction{});check(h.observedOrdinary==ordinary,"Block does not act as navigation");
  h.tick(handler,idle);check(h.observedOrdinary==ordinary+1,"ordinary idle animation continues in combat");
  check(h.fight().random().position()==0,"idle without combat work creates no turn");
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
    check(h.fight().phase()==Phase::PlayerReady,"service stale fixture entered combat");
    h.press(handler,InteractionAction{});
    check(h.fight().pending()==Work::Action&&h.flow->encounter()->deadline(),"service stale fixture pending work");
   }
   const auto revision=mode==2?h.fight().result().revision:h.flow->encounter()->state().revision();
   const auto pending=mode==2?static_cast<unsigned>(h.fight().pending()):h.flow->encounter()->state().pending();
   const auto deadline=h.flow->encounter()->deadline();
   const auto resultGeneration=h.fight().result().generation;
   const auto randomPosition=h.fight().random().position();
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
   check(h.fight().random().position()==randomPosition,"stale production operation adopted RNG");
   if(mode!=3) check(h.fight().result().generation==resultGeneration,
    "stale production operation adopted result");
   check(h.flow->encounter()->deadline()==deadline,"stale production operation altered deadline");
   if(mode==2) {
    check(h.fight().result().revision==revision&&static_cast<unsigned>(h.fight().pending())==pending&&
     h.fight().phase()==Phase::PreparingAction&&h.world->sessionState().actors()[5].hp==20,
     "stale service consumed automatic combat work");
   } else if(mode==3) {
    check(h.flow->encounter()->state().revision()==revision+1&&h.flow->encounter()->state().pending()==0&&
     h.fight().phase()==Phase::Engaged&&h.fight().result().revision==revision+1&&
     h.fight().result().operation==XeenCombatOperation::ApproachAction&&
     h.fight().result().generation==resultGeneration+1,
     "stale handoff changed the accepted engagement or entered combat");
   } else {
    check(h.flow->encounter()->state().revision()==revision&&h.flow->encounter()->state().pending()==pending&&
     h.fight().phase()==Phase::Approach&&!h.world->sessionState().encounterTerminal(),
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
  check(h.fight().phase()==Phase::SupportStopped&&h.flow->encounter()->state().revision()==revision+1&&
   h.flow->encounter()->state().pending()==0&&!h.flow->encounter()->deadline(),
   "current automatic support stop did not retire only its own work");
  return true;
 };
 check(Application().playGameplay(s,XeenActorApproach::kEntry,{},false,XeenEncounterEntry::Diagnostic27)==0,
  "automatic support-stop production route");
}
void outcome(bool loss,const std::optional<std::filesystem::path> &game={}) {
 Harness h(game);auto s=h.services(loss?19:1);unsigned commands=0;
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
  for(unsigned n=0;n<500&&h.fight().phase()!=Phase::Victory&&h.fight().phase()!=Phase::Defeat;++n){
   check(!h.flow->encounter()->terminal(),"unexpected combat failure");
   h.press(handler,SaveGameAction{});check(h.saves==0,"zero save side effects in every phase");
   if(h.fight().phase()==Phase::PlayerReady){
    h.press(handler,loss||commands<6?PlayerAction{BlockAction{}}:PlayerAction{InteractionAction{}});++commands;
   }else{
    const auto before=h.fight().result().generation;
    h.press(handler,InteractionAction{});
    check(before==h.fight().result().generation,"pending input does not replace automatic work");
    h.tick(handler,idle);
    const auto after=h.fight().result().generation;idle();
    check(after==h.fight().result().generation,"same time/cycle cannot repeat automatic work");
   }
  }
  check(h.fight().phase()==(loss?Phase::Defeat:Phase::Victory),"real production terminal outcome");
  if(game)check(h.party->encounterContext->minutes==(loss?500:493),"original seeded time");
  const auto terminal=h.fight().result().generation;
  for(const PlayerAction action:std::initializer_list<PlayerAction>{InteractionAction{},BlockAction{},AcknowledgeAction{},InspectInventoryAction{},WaitAction{},SaveGameAction{}})
   h.press(handler,action);
  h.tick(handler,idle);
  check(h.fight().result().generation==terminal&&!h.saves,"terminal stays terminal and unsaveable");
  check(h.flow->encounter()->notice().find(loss?"DEFEAT":"VICTORY")!=std::string::npos,"in-frame terminal text");
  if(!loss)check(h.flow->encounter()->notice().find("XP +82")!=std::string::npos,"fixed award retained through End");
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
   if(fault==0&&h.flow&&h.fight().phase()==Phase::PreparingAction&&!injected){injected=true;throw std::runtime_error("composition failure");}
   return compose(w,p,c,ordinary,actor);
  };
  const auto configure=s.configureFlow;
  s.configureFlow=[&](auto &flow,const auto &camera){
   configure(flow,camera);
   flow.reportText=[&](const std::string &){
    if(fault==1&&h.fight().phase()==Phase::PreparingAction&&!injected){injected=true;throw std::runtime_error("report failure");}
   };
   flow.beforeEncounterFrameCopy=[&]{
    if(fault==2&&h.fight().phase()==Phase::PreparingAction&&!injected){injected=true;throw std::runtime_error("frame-copy failure");}
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
     const auto generation=h.fight().result().generation;
     h.flow->failEncounterHandoff(old);
     check(h.fight().result().generation==generation,"stale handoff cannot fail newer combat");
     return true;
    }
   }
   check(injected&&h.fight().phase()==Phase::Failed,"current failure stops combat");
   check(h.fight().random().position()==0,"failure did not roll pending player attack");
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
    const auto &r=h.fight().result();
    const bool target=fault==0?(r.operation==XeenCombatOperation::PlayerAttack&&r.attackOutcome==XeenCombatAttackOutcome::HitPositiveDamage):
     fault==1?h.fight().phase()==Phase::VictoryAwaitingEnd:fault==2?h.fight().phase()==Phase::Victory:h.fight().phase()==Phase::Defeat;
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
    if(h.fight().phase()==Phase::PlayerReady){h.press(handler,fault==3||commands++<6?PlayerAction{BlockAction{}}:PlayerAction{InteractionAction{}});}
    else h.tick(handler,idle);
   }
   check(injected,"publication fault reached through real commands");
   check(h.fight().phase()==(fault==2?Phase::Victory:fault==3?Phase::Defeat:Phase::Failed),"failure preserves actual terminal boundary");
   const auto hp=h.world->sessionState().actors()[5].hp;
   check(fault==3||hp<20,"published damage survives failure");
   if(fault==1||fault==2){check(hp==0,"lethal removal retained");check(h.flow->encounter()->notice().find("XP +82")!=std::string::npos,"award retained despite composition failure");}
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
   const auto before=h.fight().result().generation;
   auto result=handler.withDisplayedInput(a,t);
   if(std::holds_alternative<BlockAction>(a)&&before!=h.fight().result().generation)++accepted;
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
    case 1:check(h.fight().phase()==Phase::Approach,"SDL Begin");key(SDLK_PERIOD);break;
    case 2:check(h.fight().phase()==Phase::PlayerReady,"SDL handoff");break;
    case 3:key(SDLK_b);key(SDLK_b,SDL_KEYDOWN,1);key(SDLK_b,SDL_KEYUP);key(SDLK_b);break;
    case 4:check(accepted==1,"one owner per poll batch");key(SDLK_b);break;
    case 5:check(accepted==1,"held B requires release");key(SDLK_b,SDL_KEYUP);break;
    case 6:key(SDLK_b);break;
    case 7:check(accepted==2,"fresh B accepts next displayed owner");key(SDLK_b,SDL_KEYUP);key(SDLK_SPACE);break;
    case 8:key(SDLK_SPACE,SDL_KEYUP);break;
    case 9:break;
    case 10:key(SDLK_SPACE);break;
    default:
     if(h.fight().phase()==Phase::PlayerReady){
      check(h.fight().random().position()>0,"automatic attack runs without another key");
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
   check(injected&&h.fight().phase()==Phase::Failed,"callback failure retains its legitimate stop");
   check(h.fight().random().position()==0&&h.saves==0,"callback failure cannot roll or save");
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
  {L"--encounter-27",L"--encounter-27",L"missing"},{L"--encounter-27",L"missing",L"extra"},
  {L"--encounter-27",L"--encounter-26",L"missing"},
  {L"--encounter-27",L"--render-map",L"missing"},
  {L"--encounter-27",L"--load-game",L"missing"},
  {L"--encounter-27",L"missing",L"--save-file",L"untouched"},
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
 else if(argc==2){outcome(false,std::filesystem::path(argv[1]));outcome(true,std::filesystem::path(argv[1]));}
 else {preparation();delayed();staleCoordination();automaticSupportStop();outcome(false);outcome(true);failures();publicationFailures();callbacks();}
 std::cout<<"Combat production tests passed\n";return 0;
}catch(const std::exception &e){std::cerr<<e.what()<<'\n';return 1;}}
