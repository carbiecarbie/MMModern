// Opt-in original topology with synthetic sign text and fault providers.
// These representation controls are separate from the live-actor CLI routes.
#include "XeenCombatGameplayTestSupport.h"
#include "XeenRestoreReplayProbe.h"
#include "games/xeen/XeenRegionalRules.h"
#include "games/xeen/XeenCharacterRules.h"
#include "games/xeen/XeenStateEquality.h"
#include "formats/xeen/XeenQuestFlagFormat.h"
#include "formats/xeen/XeenCharacterFormat.h"
#include <iostream>
#include <string_view>
using namespace combat_gameplay_test;
namespace mmodern {
struct XeenInventoryTestAccess {
 static XeenEncounterFlow &encounter(XeenEventFlow &flow) { return *flow._encounter; }
 static std::uint64_t epoch(const XeenEventFlow &flow) { return flow._inventoryEpoch; }
 static std::uint64_t input(const XeenEventFlow &flow) { return flow._inputGeneration; }
 static std::uint64_t use(const XeenEventFlow &flow) { return flow._itemUseGeneration.value_or(0); }
};
}
namespace {
XeenGameplayServices regional(Harness &h) {
 auto s=h.services();
 s.resources.regionalManifest=[&](const auto &m,const auto &o,const auto &e,const auto &mon){
  xeenValidateRegionalManifest(m,o,e,mon,h.assets->readInitialResource("maze0023.dat"),h.assets->readInitialResource("maze0023.mob"),h.assets->readInitialResource("maze0023.evt"));
 };
 s.texts=[](auto id){XeenEventTextFile t{id,"synthetic-sign.txt",true,std::vector<std::string>(17)};t.strings[16]="Synthetic regional sign";return t;};
 s.composeEncounter=[](auto &,const auto &,const auto &,auto,auto){XeenEventFlow::Composition c;c.frame.width=320;c.frame.height=200;c.frame.pixels.resize(64000);return c;};
 return s;
}
auto bytes(Harness &h){return XeenSaveFormat::encode(XeenSaveState::capture(h.signature,*h.party,*h.camera,*h.flags,*h.world));}
}
int main(int argc,char **argv) {
 try {
  check(argc==3 || argc==4 || argc==5,"usage: regional_event_original <installation> <temporary-save> [stage] [run-seed]");
  const std::filesystem::path game=argv[1],path=argv[2];
  const std::string stage=argc>=4?argv[3]:"all";
  const bool overlay=stage.rfind("overlay-",0)==0;
  const bool treasureMyra=stage=="treasure-myra-dormant" || stage=="treasure-myra-ready";
  const bool treasureAfter=stage=="treasure-after-item-contact";
  const bool treasureWell=stage=="treasure-well-dormant" || stage=="treasure-well-ready" || treasureAfter;
  const bool treasureReady=stage=="treasure-myra-ready" || stage=="treasure-well-ready" || treasureAfter;
  const bool receiptFault=stage=="receipt-fault";
  const bool rewardVariant=overlay || receiptFault;
  const bool overlayWarning=stage=="overlay-warning";
  const unsigned overlayMask=overlay && !overlayWarning?static_cast<unsigned>(std::stoul(stage.substr(8))):overlayWarning?17u:0u;
  check(!overlay || (overlayMask>=1 && overlayMask<=31),"Overlay mask must select Myra reward producers");
  const unsigned runSeed=argc==5?static_cast<unsigned>(std::stoul(argv[4])):3;
  check(argc!=5 || ((stage=="run-quest" || stage=="run-full") && runSeed>=1 && runSeed<=256),"Run seed must be 1..256");
  const bool staged=stage!="all";
  check(!staged || stage=="request" || stage=="collected" || stage=="return" || stage=="exchange" || stage=="recovery" || stage=="continue" || stage=="full" ||
   stage=="branches" || stage=="selector-authority" || stage=="selector-aba" || stage=="phirna-grant-fault" || stage=="myra-take-fault" || stage=="item-owed-fault" || stage=="well-repeat" || stage=="well-equal" || stage=="well-frame-retry" || stage=="well-text-fault" ||
   stage=="run-quest" || stage=="run-restart" || stage=="run-full" || stage=="text-handoff" ||
   stage=="fresh-text-fault" || stage=="item-draw-fault" || rewardVariant || treasureMyra || treasureWell,"Unknown M35 stage");
  if(stage=="text-handoff"){
   Harness h(game);auto s=regional(h);
   s.resources.loadInitialPurse=[&]{return XeenCharacterFormat::parseMonsterPurse(h.assets->readInitialResource("maze.pty"));};
   s.resources.loadInitialRegionalRecovery=[&]{return XeenQuestFlagFormat::parseRegionalRecovery(h.assets->readInitialResource("maze.pty"));};
   s.resources.loadRegionalText=[&](XeenMapIdentity id){return XeenEventTextLoader([&](const std::string &name)->std::optional<std::vector<std::uint8_t>>{
    if(!h.assets->hasArchiveResource(name))return {};return h.assets->readArchiveResource(name);}).load(id);};
   const auto saved=XeenSaveFile::read(path);
   XeenPartyState party;XeenCamera camera;XeenGameFlags flags;
   XeenWorld world(s.maps,s.objects);
   XeenSaveState::restoreBeforeGameplay(saved,s.resources,party,camera,flags,world,
    [&](auto &w,const auto &p,const auto &c,const auto &){check(s.composeEncounter(w,p,c,0,XeenMonsterAppearance{0}).frame.isValid(),
     "Restored original-scene preflight");});
   XeenEncounterFlow coordinator(world,party,camera,flags,[]{return std::uint64_t{0};},XeenJourneyRestoreTag{});
   auto &guard=coordinator.journeySavePreimage();guard.check();
   const auto original=s.resources.loadRegionalText(23);
   guard.admitRegionalText(original); // Equal-byte cache reconstruction is valid.
   auto changed=original;changed.strings.at(18)+=" changed";
   bool rejected=false;try{guard.admitRegionalText(changed);}catch(const std::exception &){rejected=true;}
   check(rejected && !guard.current(),"Changed text at first restored runtime use invalidates Journey");
   bool reopened=false;try{guard.admitRegionalText(original);reopened=guard.current();}catch(const std::exception &){}
   check(!reopened,"Later original bytes cannot reopen failed restored text authority");
   std::cout<<"M35 regional text restore handoff/cache/latched failure PASS\n";return 0;
  }
  if(overlay){
   auto fixture=XeenSaveFile::read(path);
   for(unsigned i=0;i<5;++i)if(overlayMask&(1u<<i))fixture.disabledEvents.push_back({23,31u+i});
   std::sort(fixture.disabledEvents.begin(),fixture.disabledEvents.end());
   if(overlayWarning)for(auto owner:fixture.activeRosterIds){
    auto &c=fixture.characters[owner];
    for(auto &item:c.weapons)if(!item.id)item={0,6,0,0};
    for(auto &item:c.armor)if(!item.id)item={0,3,0,0};
    for(auto &item:c.accessories)if(!item.id)item={38,2,0,0};
    for(auto &item:c.miscellaneous)if(!item.id)item={10,37,1,0};
   }
   XeenSaveFile::write(path,fixture);
  }
  if(stage=="well-equal"){
   auto fixture=XeenSaveFile::read(path);
   check(fixture.journey.has_value() && fixture.journey->context.has_value(),
    "Equal-maximum well fixture requires a Journey context");
   fixture.characters[11].currentHp=XeenCharacterRules::maxHp(fixture.characters[11],
    {fixture.journey->context->year});
   XeenSaveFile::write(path,fixture);
  }
  if(treasureMyra || treasureWell){
   auto fixture=XeenSaveFile::read(path);
   check(fixture.journey && fixture.journey->treasure,"Treasure fixture requires contract-6 Journey");
   fixture.camera=treasureMyra?XeenCamera{23,9,11,XeenDirection::West}:
    XeenCamera{23,7,7,treasureReady?XeenDirection::North:XeenDirection::South};
   if(treasureMyra){fixture.questItems[17]=1;fixture.questFlags[2]=true;}
   auto &treasure=*fixture.journey->treasure;
   unsigned source=12;
   for(unsigned i=0;i<12;++i){
    bool stored=false;for(const auto &entry:treasure.weapons)stored=stored||(entry.item.id && entry.source==i);
    for(const auto &entry:treasure.armor)stored=stored||(entry.item.id && entry.source==i);
    if(fixture.journey->actors.at(i).accounted && !(treasure.pendingMask&(1u<<i)) && !stored){source=i;break;}
   }
   check(source<12 && !treasure.weapons[0].item.id,
    "Treasure fixture needs defeated/accounted Orc and empty store");
   treasure.weapons[0]={static_cast<std::uint8_t>(source),{0,30,0,0}};
   if(treasureReady){
    treasure.pendingMask|=1u<<source;treasure.pendingGold+=10;
    auto &blocker=fixture.journey->actors.at(0);
    check(!blocker.accounted && blocker.lifecycle==XeenActorLifecycle::Present,
     "Ready treasure fixture needs a surviving blocker");
    blocker.x=treasureMyra?8:7;blocker.y=treasureMyra?11:8;blocker.activated=true;
   }
   XeenSaveFile::write(path,fixture);
  }
  if(stage=="item-draw-fault"){
   auto fixture=XeenSaveFile::read(path);
   check(fixture.journey && fixture.journey->actors.size()==19,"Draw fault fixture requires regional Journey");
   fixture.camera={23,7,7,XeenDirection::North};
   auto &shooter=fixture.journey->actors.at(0);
   check(!shooter.accounted && shooter.lifecycle==XeenActorLifecycle::Present,
    "Draw fault requires surviving ranged actor");
   shooter.x=7;shooter.y=9;shooter.activated=true;
   XeenSaveFile::write(path,fixture);
   std::filesystem::copy_file(path,path.string()+".prepared",std::filesystem::copy_options::overwrite_existing);
  }
  XeenSaveSnapshot source;
  if(!staged){Harness h(game);auto s=regional(h);s.show=[&](const auto &,const auto &handler,const auto &,const auto &,const auto &){handler.framePresented(h.flow->frame().presentation());source=XeenSaveState::capture(h.signature,*h.party,*h.camera,*h.flags,*h.world);return true;};
   check(Application().playGameplay(s,{},path,false,XeenEncounterEntry::Journey,1,3)==0,"Regional source preparation");}
  {
   Harness h(game);auto s=regional(h);
   bool corruptText=false;
   bool grantFaultArmed=false,grantFaultRaised=false,owedFaultArmed=false,owedFaultRaised=false;
   const auto normalMaps=s.maps;
   s.maps=[&](auto id){
    if(grantFaultArmed && !grantFaultRaised && h.party && h.party->questItems.at(17)==1){
     grantFaultRaised=true;throw std::runtime_error("Artificial Phirna Remove map preparation failure");
    }
    if(owedFaultArmed && h.flow && h.flow->encounter()->itemUseReady()){
     owedFaultRaised=true;throw std::runtime_error("Artificial owed item opportunity map preparation failure");
    }
    return normalMaps(id);
   };
   s.resources.loadInitialPurse=[&]{return XeenCharacterFormat::parseMonsterPurse(h.assets->readInitialResource("maze.pty"));};
   s.npcDraw=[&](IndexedFrame &frame,std::uint8_t portrait,std::size_t index){h.assets->drawNpc(frame,portrait,index);};
   s.resources.loadInitialRegionalRecovery=[&]{return XeenQuestFlagFormat::parseRegionalRecovery(h.assets->readInitialResource("maze.pty"));};
   s.texts=[&](XeenMapIdentity id){auto value=XeenEventTextLoader([&](const std::string &name)->std::optional<std::vector<std::uint8_t>>{
    if(!h.assets->hasArchiveResource(name))return {};return h.assets->readArchiveResource(name);}).load(id);
    if(corruptText && id==XeenMapIdentity(23))value.strings.at(18)+=" changed";
    return value;};
   std::optional<XeenPresentationRequest> pending;
   unsigned rewardReceipts=0,rewardWarnings=0;
   bool receiptFaultRaised=false;
   s.configureFlow=[&](auto &f,const auto &){h.flow=&f;
    if(stage=="myra-take-fault")f.beforeRewardEnqueue=[&]{
     check(h.party->questItems.at(17)==0 && !h.party->questFlags.isSet(2),
      "Myra reward fault follows Root and Q2 publications");
     throw std::runtime_error("Artificial fault before first Myra reward enqueue");
    };
    f.reportManual=[&](const auto &result){
    if(const auto *suspended=std::get_if<XeenEventExecutionSuspended>(&result)){
     pending=suspended->request;
     rewardReceipts+=pending->kind==XeenPresentationKind::RewardReceipt;
     rewardWarnings+=pending->kind==XeenPresentationKind::RewardWarning;
     if(receiptFault && pending->kind==XeenPresentationKind::RewardReceipt && !receiptFaultRaised){
      receiptFaultRaised=true;throw std::runtime_error("Artificial post-delivery receipt failure");
     }
    }
    else pending.reset();
    if(const auto *error=std::get_if<XeenEventExecutionError>(&result))std::cerr<<"M35 event: "<<error->message<<'\n';};};
   s.show=[&](const auto &,const auto &handler,const auto &,const auto &idle,const auto &){
    handler.framePresented(h.flow->frame().presentation());
    check(h.flow->canSave() && h.party->regionalRecovery,"Contract-6 entry");
    if(stage=="collected" || stage=="return" || stage=="exchange" || rewardVariant || treasureMyra || treasureWell || stage=="item-draw-fault" || stage=="recovery" || stage=="continue" || stage=="branches" || stage=="selector-authority" || stage=="selector-aba" || stage=="phirna-grant-fault" || stage=="myra-take-fault" || stage=="item-owed-fault" || stage=="well-repeat" || stage=="well-equal" || stage=="well-frame-retry" ||
     stage=="well-text-fault" || stage=="run-restart"){
     const auto persisted=XeenSaveFile::read(path);
     const auto live=XeenSaveState::capture(h.signature,*h.party,*h.camera,*h.flags,*h.world);
     save_test::sameSnapshot(persisted,live);
     check(XeenSaveFormat::encode(persisted)==XeenSaveFormat::encode(live),"Fresh process initial exact bytes");
    }
    const auto input=[&](PlayerAction action){handler.framePresented(h.flow->frame().presentation());handler.beginCycle(++h.cycle);
     check(handler.displayedInput().has_value(),"M35 displayed input");handler.withDisplayedInput(action,*handler.displayedInput());
     handler.framePresented(h.flow->frame().presentation());};
    const auto pulse=[&]{h.now+=100;handler.beginCycle(++h.cycle);idle();handler.framePresented(h.flow->frame().presentation());};
    bool runCombat=stage=="run-quest" || stage=="run-full";
    unsigned combatInputs=0;
    const auto settle=[&](bool eventChoice=false){
     for(unsigned n=0;n<30000;++n){
      if(h.flow->encounter()->combat()){
       const auto *combat=h.flow->encounter()->combat();
       check(combat->phase()!=XeenCombatPhase::Failed && combat->phase()!=XeenCombatPhase::Defeat &&
        combat->phase()!=XeenCombatPhase::SupportStopped,"M35 combat terminal");
       if(combat->phase()==XeenCombatPhase::PlayerReady){
        check(++combatInputs<=600,"M35 600 player-input bound");
        const auto rows=combat->contacts();unsigned selected=0;
        for(unsigned i=0;i<rows.size();++i)if(rows[i] && (!rows[selected] || rows[i]->recordIndex<rows[selected]->recordIndex))selected=i;
        if(!(rows[selected]==combat->selectedTarget()))input(SelectCombatTargetAction{selected});
        else input(runCombat?PlayerAction{RunAction{}}:PlayerAction{AttackAction{}});
       }else pulse();continue;
      }
      const auto activity=h.world->sessionState().journeyActivity();
      if(activity==XeenJourneyActivity::Event || activity==XeenJourneyActivity::Reward){
       if(pending){
        switch(pending->response){
        case XeenPresentationResponseRequirement::YesNo: input(eventChoice?PlayerAction{YesAction{}}:PlayerAction{NoAction{}});break;
        case XeenPresentationResponseRequirement::CharacterSelection: input(SelectMemberAction{3});break;
        default: input(AcknowledgeAction{});break;
        }
       }else input(AcknowledgeAction{});
       continue;
      }
      if(h.flow->canSave())return;
      pulse();
     }
     throw std::runtime_error("M35 production work bound at ("+std::to_string(h.camera->x)+","+
      std::to_string(h.camera->y)+") activity="+std::to_string(unsigned(h.world->sessionState().journeyActivity()))+
      " pending="+std::to_string(h.flow->encounter()->state().pending())+
      " combat="+std::to_string(bool(h.flow->encounter()->combat()))+
      " phase="+std::to_string(h.flow->encounter()->combat()?unsigned(h.flow->encounter()->combat()->phase()):0)+
      " run="+std::to_string(h.flow->encounter()->combat()?unsigned(h.flow->encounter()->combat()->result().operation):0));
    };
    if(stage=="item-draw-fault"){
     std::size_t slot=9;for(std::size_t i=0;i<9;++i){const auto &item=h.party->roster.at(0).miscellaneous[i];
      if(item.material==10 && item.id==37 && item.state==1){slot=i;break;}}
     check(slot<9 && h.party->roster.at(18).conditions[3]==1,"Draw fault uses delivered antidote");
     const auto sourceBefore=h.party->roster.at(0).miscellaneous;
     input(InspectInventoryAction{});for(int i=0;i<3;++i)input(NavigationAction::TurnRight);
     input(SelectInventorySlotAction{slot});input(UseItemAction{});input(AcknowledgeAction{});
     input(SelectMemberAction{1});
     auto sourceExpected=sourceBefore;sourceExpected[slot]=XeenAntidoteUse::debit(sourceExpected[slot]);
     XeenAntidoteUse::settle(sourceExpected,slot,true);
     bool sameSource=true;for(unsigned i=0;i<9;++i)sameSource=sameSource&&
      xeenSameItem(sourceExpected[i],h.party->roster.at(0).miscellaneous[i]);
     check(h.flow->encounter()->itemUseReady() && h.party->roster.at(18).conditions[3]==0 && sameSource,
      "Draw fault starts after exact published debit/effect/compaction");
     const auto characters=h.party->roster.characters();
     const std::vector<XeenActor> actors=h.world->sessionState().actors();
     const auto random=h.world->sessionState().journeyRandom();
     const auto context=h.party->encounterContext;
     const auto treasure=h.party->monsterTreasure;
     bool drawFaultRaised=false;
     replay_test::observeDraw=[&](auto,auto,auto,auto){drawFaultRaised=true;throw std::bad_alloc();};
     try{pulse();}catch(const std::exception &){}
     replay_test::observeDraw={};
     check(drawFaultRaised && !h.flow->canSave(),"Owed ranged draw failure blocks continued gameplay/save");
     for(unsigned i=0;i<30;++i)check(xeen_state::sameCharacter(characters[i],h.party->roster.at(i)),
      "Ranged draw failure altered published character prefix");
     check(actors.size()==h.world->sessionState().actors().size(),"Ranged fault actor count");
     for(std::size_t i=0;i<actors.size();++i)check(xeen_state::sameActor(actors[i],h.world->sessionState().actors()[i]),
      "Ranged draw failure partially published actors");
     check(random==h.world->sessionState().journeyRandom() && context==h.party->encounterContext &&
      treasure==h.party->monsterTreasure && h.party->roster.at(18).conditions[3]==0,
      "Ranged draw failure replayed or refunded antidote prefix");
     std::cout<<"M35 owed ranged draw failure preserves published debit/effect atomically PASS\n";return true;
    }
    if(treasureMyra || treasureWell){
     const auto before=XeenSaveState::capture(h.signature,*h.party,*h.camera,*h.flags,*h.world);
     const auto retained=*h.party->monsterTreasure;
     check(treasureReady?retained.ready():retained.dormant(),"Treasure fixture readiness");
     if(treasureMyra){
      check(h.party->questItems.at(17)==1,"Treasure Myra fixture retains original Root");
      input(InteractionAction{});settle();
      auto expected=before;--expected.questItems[17];expected.questFlags[2]=false;
      XeenPartyState recipient;recipient.party=XeenParty::fromRosterIds(h.party->party.activeRosterIds());
      for(auto owner:h.party->party.activeRosterIds())recipient.roster.at(owner)=before.characters[owner];
      XeenPendingRewards rewards;for(unsigned i=0;i<5;++i)rewards.enqueue({10,37,1,0});
      xeenDeliverRewards(rewards,recipient,{});
      for(auto owner:h.party->party.activeRosterIds())expected.characters[owner].miscellaneous=recipient.roster.at(owner).miscellaneous;
      save_test::sameSnapshot(expected,XeenSaveState::capture(h.signature,*h.party,*h.camera,*h.flags,*h.world));
      check(*h.party->monsterTreasure==retained,"Myra rewards changed monster treasure readiness or provenance");
     } else {
      const auto hp=h.party->roster.at(11).currentHp;
      input(InteractionAction{});input(SelectMemberAction{3});
      check(h.party->roster.at(11).currentHp==hp+25 && *h.party->monsterTreasure==retained,
       "Well HP prefix changed monster treasure");
      settle();
      check(h.party->regionalRecovery->worldFlag16 && *h.party->monsterTreasure==retained,
       "Well flag publication changed monster treasure");
      std::size_t slot=9;for(std::size_t i=0;i<9;++i){const auto &item=h.party->roster.at(0).miscellaneous[i];
       if(item.material==10 && item.id==37 && item.state==1){slot=i;break;}}
      check(slot<9,"Treasure recovery fixture retains delivered antidote");
      input(InspectInventoryAction{});for(int i=0;i<3;++i)input(NavigationAction::TurnRight);
      input(SelectInventorySlotAction{slot});input(UseItemAction{});input(AcknowledgeAction{});
      input(SelectMemberAction{1});
      check(h.party->roster.at(18).conditions[3]==0 && *h.party->monsterTreasure==retained,
       "Antidote effect changed monster treasure before owed work");
      if(treasureReady){
       pulse();
       check(h.flow->encounter()->itemUseResult() &&
        h.flow->encounter()->result().movementOpportunities==1 && *h.party->monsterTreasure==retained,
        "Item opportunity changed ready blocked monster treasure before contact combat");
       check(h.flow->encounter()->combat() && h.flow->encounter()->combat()->participants()==0x3f,
        "Owed item contact opens a fresh full active-party participant mask");
       if(treasureAfter){
        unsigned combatFrameFault=0;
        h.flow->beforeEncounterFrameCopy=[&]{
         if(h.flow->encounter()->combat() && !combatFrameFault++)
          throw std::runtime_error("Artificial item-contact combat frame copy failure");
        };
        settle();
        h.flow->beforeEncounterFrameCopy={};
        check(combatFrameFault>=1 && h.flow->canSave() && h.party->roster.at(18).conditions[3]==0 &&
         h.flow->encounter()->itemUseResult() && h.flow->encounter()->result().movementOpportunities==1,
         "Post-contact combat frame retry/treasure did not preserve single antidote settlement");
        xeenValidateMonsterTreasure(*h.party->monsterTreasure,6);
        input(SaveGameAction{});check(h.saves>0,"Post-contact treasure F9 save");
        std::cout<<"M35 item contact followed by original combat/treasure/presentation PASS\n";return true;
       }
      }else{
       settle();
       check(*h.party->monsterTreasure==retained,"Antidote owed work changed dormant monster treasure");
      }
     }
     std::cout<<"M35 "<<(treasureReady?"ready":"dormant")<<" monster treasure preserved across "
      <<(treasureMyra?"Myra rewards":"well and antidote")<<" PASS\n";return true;
    }
    if(stage=="well-repeat"){
     check(h.camera->x==7 && h.camera->y==7 && h.party->regionalRecovery->worldFlag16,
      "Restored well repeat starting point");
     const auto maximum=XeenCharacterRules::maxHp(h.party->roster.at(11),{h.party->encounterContext->year});
     for(unsigned n=0;h.party->roster.at(11).currentHp<=maximum && n<10;++n){
      const auto before=h.party->roster.at(11).currentHp;
      const auto minutes=h.party->encounterContext->minutes;
      const auto draws=h.world->sessionState().journeyRandom()->count;
      input(InteractionAction{});check(pending && pending->response==XeenPresentationResponseRequirement::CharacterSelection,
       "Repeated well WhoWill");input(SelectMemberAction{3});settle();
      check(h.party->roster.at(11).currentHp==before+25 && h.party->encounterContext->minutes==minutes &&
       h.world->sessionState().journeyRandom()->count==draws,"Repeated live-maximum well gain");
     }
     check(h.party->roster.at(11).currentHp>maximum,"Well repetition crossed live maximum");
     const auto hp=h.party->roster.at(11).currentHp;
     const auto minutes=h.party->encounterContext->minutes;
     const auto draws=h.world->sessionState().journeyRandom()->count;
     input(InteractionAction{});input(SelectMemberAction{3});settle();
     check(h.party->roster.at(11).currentHp==hp && h.party->regionalRecovery->worldFlag16 &&
      h.party->encounterContext->minutes==minutes && h.world->sessionState().journeyRandom()->count==draws,
      "Above-maximum well refusal keeps complete local state");
     input(WaitAction{});settle();input(SaveGameAction{});check(h.saves>0,"Well refusal further gameplay save");
     std::cout<<"M35 genuine repeat/refusal well branch PASS\n";return true;
    }
    if(stage=="well-equal"){
     for(char key:std::string_view{"LUUURUULU"}){
      input(key=='U'?PlayerAction{NavigationAction::MoveForward}:key=='L'?PlayerAction{NavigationAction::TurnLeft}:PlayerAction{NavigationAction::TurnRight});settle();
     }
     const auto before=XeenSaveState::capture(h.signature,*h.party,*h.camera,*h.flags,*h.world);
     const auto maximum=XeenCharacterRules::maxHp(h.party->roster.at(11),{h.party->encounterContext->year});
     check(h.party->roster.at(11).currentHp==maximum,"Well equal-maximum entry");
     input(InteractionAction{});input(SelectMemberAction{3});settle();
     auto expected=before;expected.characters[11].currentHp=static_cast<std::int16_t>(maximum+25);
     expected.journey->regionalRecovery->worldFlag16=true;
     save_test::sameSnapshot(expected,XeenSaveState::capture(h.signature,*h.party,*h.camera,*h.flags,*h.world));
     input(SaveGameAction{});check(h.saves>0,"Equal-maximum well save");
     std::cout<<"M35 equal-maximum well full-state gain PASS\n";return true;
    }
    if(stage=="well-frame-retry"){
     for(char key:std::string_view{"LUUURUULU"}){
      input(key=='U'?PlayerAction{NavigationAction::MoveForward}:key=='L'?PlayerAction{NavigationAction::TurnLeft}:PlayerAction{NavigationAction::TurnRight});settle();
     }
     const auto before=XeenSaveState::capture(h.signature,*h.party,*h.camera,*h.flags,*h.world);
     const auto hp=h.party->roster.at(11).currentHp;
     input(InteractionAction{});
     unsigned failures=0;
     h.flow->beforeEncounterFrameCopy=[&]{
      if(h.party->roster.at(11).currentHp==hp+25 && !h.party->regionalRecovery->worldFlag16 && !failures++)
       throw std::runtime_error("Artificial well success frame copy failure");
     };
     input(SelectMemberAction{3});h.flow->beforeEncounterFrameCopy={};
     check(failures>=1 && h.party->roster.at(11).currentHp==hp+25 &&
      !h.party->regionalRecovery->worldFlag16 && !h.flow->canSave(),
      "Well success frame retry retains exactly one HP publication and blocks partial capture");
     settle();
     auto expected=before;expected.characters[11].currentHp=static_cast<std::int16_t>(hp+25);
     expected.journey->regionalRecovery->worldFlag16=true;
     save_test::sameSnapshot(expected,XeenSaveState::capture(h.signature,*h.party,*h.camera,*h.flags,*h.world));
     input(WaitAction{});settle();input(SaveGameAction{});check(h.saves>0,"Well frame retry further gameplay save");
     std::cout<<"M35 well HP/frame retry/flag full-state publication PASS\n";return true;
    }
    if(stage=="well-text-fault"){
     for(char key:std::string_view{"LUUURUULU"}){
      input(key=='U'?PlayerAction{NavigationAction::MoveForward}:key=='L'?PlayerAction{NavigationAction::TurnLeft}:PlayerAction{NavigationAction::TurnRight});settle();
     }
     check(h.camera->x==7 && h.camera->y==7 && !h.party->regionalRecovery->worldFlag16,"Well fault genuine arrival");
     const auto hp=h.party->roster.at(11).currentHp;
     const auto minutes=h.party->encounterContext->minutes;
     const auto draws=h.world->sessionState().journeyRandom()->count;
     input(InteractionAction{});input(SelectMemberAction{3});
     check(h.party->roster.at(11).currentHp==hp+25 && !h.party->regionalRecovery->worldFlag16,
      "Well fault after HP before flag");
     corruptText=true;h.eventSystem->discardTextCache();
     bool failed=false;try{h.flow->refresh(true);handler.framePresented(h.flow->frame().presentation());}
     catch(const std::exception &){failed=true;}
     check(failed && !h.flow->canSave() && h.party->roster.at(11).currentHp==hp+25 &&
      !h.party->regionalRecovery->worldFlag16 && h.party->encounterContext->minutes==minutes &&
      h.world->sessionState().journeyRandom()->count==draws,
      "Mutated text after well HP latches failure without rollback or flag replay");
     std::cout<<"M35 well partial publication/resource fault PASS\n";return true;
    }
    if(stage=="fresh-text-fault"){
     const auto before=XeenSaveState::capture(h.signature,*h.party,*h.camera,*h.flags,*h.world);
     corruptText=true;h.eventSystem->discardTextCache();
     bool failed=false;try{h.flow->refresh(true);handler.framePresented(h.flow->frame().presentation());}
     catch(const std::exception &){failed=true;}
     check(failed && !h.flow->canSave() && xeen_state::sameCamera(before.camera,*h.camera) &&
      before.questItems==h.party->questItems.counts() && before.questFlags==h.party->questFlags.values() &&
      before.journey->random==h.world->sessionState().journeyRandom() &&
      before.journey->context==h.party->encounterContext,
      "Fresh regional text replacement fails without quest/time/RNG mutation");
     for(unsigned i=0;i<30;++i)check(xeen_state::sameCharacter(before.characters[i],h.party->roster.at(i)),
      "Fresh text failure changed character state");
     corruptText=false;h.eventSystem->discardTextCache();
     bool reopened=false;try{h.flow->refresh(true);reopened=h.flow->canSave();}catch(const std::exception &){}
     check(!reopened,"Restored original text cannot reopen failed fresh Journey");
     std::cout<<"M35 fresh regional text/cache latched failure PASS\n";return true;
    }
    if(stage=="run-quest" || stage=="run-restart" || stage=="run-full"){
     if(stage!="run-restart"){
      input(NavigationAction::MoveForward);settle();input(ShootAction{});settle();
      input(NavigationAction::MoveForward);settle();
      check(h.camera->x==10 && h.camera->y==12 && h.camera->direction==XeenDirection::West &&
       h.world->sessionState().actors().at(9).hp==16 && h.party->monsterTreasure->gold==800,
       "Genuine contract-6 wounded Run/disengagement");
      for(const PlayerAction action:std::vector<PlayerAction>{NavigationAction::MoveForward,NavigationAction::TurnLeft,
        NavigationAction::MoveForward,NavigationAction::TurnRight}) {input(action);settle();}
      check(h.camera->x==9 && h.camera->y==11 && h.camera->direction==XeenDirection::West,
       "Genuine wounded return to Myra");
      input(InteractionAction{});settle();
      check(h.party->questFlags.isSet(2) && h.world->sessionState().actors().at(9).hp==16,
       "Post-disengagement explicit Myra request keeps wounded actor");
      runCombat=false;
      if(stage=="run-quest") {input(SaveGameAction{});check(h.saves>0,"Run/quest F9 save");return true;}
      input(SaveGameAction{});check(h.saves>0,"Run/full checkpoint save");
      std::filesystem::copy_file(path,path.string()+".run-quest",std::filesystem::copy_options::overwrite_existing);
     }else check(h.party->questFlags.isSet(2) && h.world->sessionState().actors().at(9).hp==16,
      "Restored wounded actor and quest request");
     input(WaitAction{});settle();input(SaveGameAction{});check(h.saves>0,"Run/quest further mutation save");
     std::cout<<"M35 genuine disengagement/quest/restart PASS\n";return true;
    }
    if(stage=="all" || stage=="request" || stage=="full") {input(InteractionAction{});settle();}
    handler.framePresented(h.flow->frame().presentation());
    check(h.flow->canSave() && h.party->questItems.at(17)==(stage=="return" || stage=="exchange" || rewardVariant || stage=="myra-take-fault") &&
     h.party->questFlags.isSet(2)==(stage=="all" || stage=="request" || stage=="collected" ||
      stage=="return" || stage=="exchange" || rewardVariant || stage=="phirna-grant-fault" || stage=="myra-take-fault" || stage=="full"),
     "Request/exchange quest state");
    const auto snapshot=XeenSaveState::capture(h.signature,*h.party,*h.camera,*h.flags,*h.world);
    check(snapshot.journey && snapshot.journey->schema==6 && snapshot.journey->regionalRecovery &&
     snapshot.journey->regionalRecovery->worldFlag16==(stage=="continue"),"Contract-6 capture");
    if(stage=="request") {input(SaveGameAction{});check(h.saves>0,"Request F9 save");return true;}
    const auto route=[&](std::string_view steps){for(char key:steps){check(h.flow->canSave(),"M35 route input boundary");
     input(key=='U'?PlayerAction{NavigationAction::MoveForward}:key=='L'?PlayerAction{NavigationAction::TurnLeft}:PlayerAction{NavigationAction::TurnRight});settle();}};
    const auto fullCheckpoint=[&](const char *name){if(stage!="full")return;
     input(SaveGameAction{});check(h.saves>0,"Full route F9 checkpoint");
     std::filesystem::copy_file(path,path.string()+"."+name,std::filesystem::copy_options::overwrite_existing);
    };
    fullCheckpoint("request");
    if(stage=="selector-aba"){
     const auto poison=h.party->roster.at(18).conditions[3];
     std::size_t slot=9;for(std::size_t i=0;i<9;++i){const auto &item=h.party->roster.at(0).miscellaneous[i];
      if(item.material==10 && item.id==37 && item.state==1){slot=i;break;}}
     check(slot<9,"ABA uses delivered antidote");
     input(InspectInventoryAction{});for(int i=0;i<3;++i)input(NavigationAction::TurnRight);
     input(SelectInventorySlotAction{slot});input(UseItemAction{});input(AcknowledgeAction{});
     check(h.flow->inventorySelection().mode==XeenInventoryMode::UseTarget,"ABA target selector presented");
     const auto oldTicket=h.flow->encounter()->ticket();
     const auto oldUse=XeenInventoryTestAccess::use(*h.flow),
      oldEpoch=XeenInventoryTestAccess::epoch(*h.flow),oldInput=XeenInventoryTestAccess::input(*h.flow);
     const auto oldFrame=h.flow->frame().presentation();
     const auto replacement=h.party->roster.at(0);
     const_cast<XeenCharacter &>(h.party->roster.at(0))=replacement;
     h.flow->invalidateInventory();
     check(!XeenInventoryTestAccess::encounter(*h.flow).finishItemUse(oldTicket,oldUse,oldEpoch,1,oldInput,oldFrame) &&
      h.party->roster.at(18).conditions[3]==poison && h.party->roster.at(0).miscellaneous[slot].state==0,
      "Byte-equal owner replacement cannot reuse obsolete selector or refund debit");
     check(!h.flow->canSave(),"Owner replacement cannot expose a Quiet capture gap");
     std::cout<<"M35 selector owner ABA refuses target and preserves debit PASS\n";return true;
    }
    if(stage=="item-owed-fault"){
     const auto before=XeenSaveState::capture(h.signature,*h.party,*h.camera,*h.flags,*h.world);
     std::size_t slot=9;for(std::size_t i=0;i<9;++i){const auto &item=h.party->roster.at(0).miscellaneous[i];
      if(item.material==10 && item.id==37 && item.state==1){slot=i;break;}}
     check(slot<9 && before.characters[18].conditions[3]==1,"Owed fault uses delivered antidote and real Poison");
     input(InspectInventoryAction{});for(int i=0;i<3;++i)input(NavigationAction::TurnRight);
     input(SelectInventorySlotAction{slot});input(UseItemAction{});input(AcknowledgeAction{});
     input(SelectMemberAction{1});
     check(h.flow->encounter()->itemUseReady() && h.party->roster.at(18).conditions[3]==0,
      "Antidote effect published before owed opportunity");
     h.world->discardMapCache();owedFaultArmed=true;
     bool failed=false;try{pulse();}catch(const std::exception &){failed=true;}
     check(owedFaultRaised && !h.flow->canSave(),"Owed opportunity provider failure cannot open Quiet/F9");
     auto expected=before;
     auto &misc=expected.characters[0].miscellaneous;
     misc[slot]=XeenAntidoteUse::debit(misc[slot]);XeenAntidoteUse::settle(misc,slot,true);
     expected.characters[18].conditions[3]=0;
     for(unsigned i=0;i<30;++i)check(xeen_state::sameCharacter(expected.characters[i],h.party->roster.at(i)),
      "Owed fault changed published character prefix");
     const auto &liveActors=h.world->sessionState().actors();
     check(before.journey->actors.size()==liveActors.size(),"Owed fault actor count");
     for(std::size_t i=0;i<liveActors.size();++i){
      const auto &old=before.journey->actors[i];const auto &now=liveActors[i];
      check(old.id==now.id && old.x==now.x && old.y==now.y && old.hp==now.hp &&
       old.activated==now.activated && old.lifecycle==now.lifecycle && old.status==now.status &&
       old.accounted==h.world->sessionState().accountedMonsters().count(now.id),
       "Owed preparation failure changed actor or accounting");
     }
     check(xeen_state::sameCamera(before.camera,*h.camera) &&
      before.questItems==h.party->questItems.counts() && before.questFlags==h.party->questFlags.values() &&
      before.journey->random==h.world->sessionState().journeyRandom() &&
      before.journey->context==h.party->encounterContext &&
      before.journey->treasure==h.party->monsterTreasure,
      "Owed preparation failure preserves actor/quest/treasure/time/RNG prefix");
     std::cout<<"M35 owed opportunity fault preserves debit/effect and blocks capture PASS\n";return true;
    }
    if(stage=="phirna-grant-fault"){
     route("LUUURUULURUULUUUUUUURRUURUUUL");
     check(h.camera->x==8 && h.camera->y==2,"Phirna fault genuine arrival");
     const auto before=XeenSaveState::capture(h.signature,*h.party,*h.camera,*h.flags,*h.world);
     input(InteractionAction{});input(YesAction{});
     h.world->discardMapCache();grantFaultArmed=true;
     input(AcknowledgeAction{});
     check(grantFaultRaised && h.flow->canSave(),"Phirna grant-before-Remove fault recovered");
     auto expected=before;++expected.questItems[17];
     save_test::sameSnapshot(expected,XeenSaveState::capture(h.signature,*h.party,*h.camera,*h.flags,*h.world));
     check(!h.world->sessionState().disabledObjects().count({23,13}),"Root grant did not synthesize Remove");
     input(SaveGameAction{});check(h.saves>0,"Grant-only prefix saved");
     input(InteractionAction{});settle();
     check(h.party->questItems.at(17)==1 && !h.world->sessionState().disabledObjects().count({23,13}),
      "Subsequent Phirna possession branch preserves independent Root and object");
     input(WaitAction{});settle();input(SaveGameAction{});check(h.saves>0,"Grant fault further gameplay save");
     std::cout<<"M35 Phirna grant-before-Remove full-state prefix PASS\n";return true;
    }
    if(stage=="selector-authority"){
     std::size_t slot=9;for(std::size_t i=0;i<9;++i){const auto &item=h.party->roster.at(0).miscellaneous[i];
      if(item.material==10 && item.id==37 && item.state==1){slot=i;break;}}
     check(slot<9,"Selector authority uses a delivered antidote");
     const auto poison=h.party->roster.at(18).conditions[3];
     const auto minutes=h.party->encounterContext->minutes;
     const auto random=*h.world->sessionState().journeyRandom();
     input(InspectInventoryAction{});for(int i=0;i<3;++i)input(NavigationAction::TurnRight);
     input(SelectInventorySlotAction{slot});input(UseItemAction{});
     const auto confirmLines=xeenInventoryLayout(h.font,XeenItemCatalog{},*h.party,
      h.flow->inventorySelection(),"",nullptr,false,false,true);
     bool warning=false,freeCancel=false;
     for(const auto &line:confirmLines){
      if(line.bounds.top==126)warning=line.text.find("Target Esc after Enter still spends 1 charge")!=std::string::npos;
      if(line.bounds.top==137)freeCancel=line.text.find("Esc now is free")!=std::string::npos;
     }
     check(warning && freeCancel &&
      drawXeenInventory(h.flow->frame(),h.font,XeenItemCatalog{},*h.party,
       h.flow->inventorySelection(),"",nullptr,false,false,true).isValid(),
      "Original-font confirmation renders full post-debit cancellation warning");
     const auto oldConfirmationInput=XeenInventoryTestAccess::input(*h.flow);
     const auto priorFrame=h.flow->frame().presentation();
     unsigned attempts=0;
     h.flow->beforeEncounterFrameCopy=[&]{
      if(h.flow->inventorySelection().mode!=XeenInventoryMode::UseTarget)return;
      ++attempts;
      XeenRestoreGuard unchanged(*h.world,*h.party,*h.camera,*h.flags);
      const auto &flow=*h.flow;
      check(!XeenInventoryTestAccess::encounter(*h.flow).finishItemUse(h.flow->encounter()->ticket(),
       XeenInventoryTestAccess::use(flow),XeenInventoryTestAccess::epoch(flow),1,
       XeenInventoryTestAccess::input(flow),priorFrame) && unchanged.current(),
       "Reentrant unpresented target cannot publish or alter authority");
      if(attempts==1)throw std::runtime_error("Artificial target frame copy failure");
     };
     input(AcknowledgeAction{});
     h.flow->beforeEncounterFrameCopy={};
     check(attempts==1 && h.flow->inventorySelection().mode==XeenInventoryMode::UseTarget &&
      h.party->roster.at(0).miscellaneous[slot].state==0 &&
      h.party->roster.at(18).conditions[3]==poison &&
      h.party->encounterContext->minutes==minutes && *h.world->sessionState().journeyRandom()==random,
      "Target retry preserves one debit and no premature effect/time/RNG");
     const auto savesBeforeStale=h.saves;
     for(const PlayerAction &old:std::vector<PlayerAction>{UseItemAction{},AcknowledgeAction{},
       SelectMemberAction{1},CancelInteractionAction{},InteractionAction{},SaveGameAction{},
       NavigationAction::MoveForward})handler.withDisplayedInput(old,oldConfirmationInput);
     check(h.flow->inventorySelection().mode==XeenInventoryMode::UseTarget && h.saves==savesBeforeStale &&
      h.party->roster.at(18).conditions[3]==poison &&
      h.party->roster.at(0).miscellaneous[slot].state==0 &&
      h.party->encounterContext->minutes==minutes && *h.world->sessionState().journeyRandom()==random,
      "Old U/Enter/F-key/Escape/Space/F9/forward input cannot cross confirmation to target");
     bool obsoletePresented=false;
     try{handler.framePresented(priorFrame);obsoletePresented=true;}catch(const std::exception &){}
     check(!obsoletePresented,"Obsolete concrete frame cannot mint target authority");
     const auto &flow=*h.flow;
     XeenRestoreGuard unchanged(*h.world,*h.party,*h.camera,*h.flags);
     check(!XeenInventoryTestAccess::encounter(*h.flow).finishItemUse(h.flow->encounter()->ticket(),
      XeenInventoryTestAccess::use(flow),XeenInventoryTestAccess::epoch(flow),1,
      XeenInventoryTestAccess::input(flow),priorFrame) && unchanged.current(),
      "Previous concrete selector cannot publish after retry");
     check(!XeenInventoryTestAccess::encounter(*h.flow).finishItemUse(h.flow->encounter()->ticket(),
      XeenInventoryTestAccess::use(flow)-1,XeenInventoryTestAccess::epoch(flow),1,
      XeenInventoryTestAccess::input(flow),h.flow->frame().presentation()) && unchanged.current(),
      "Stale use generation cannot publish target");
     input(SelectMemberAction{1});settle();
     check(h.party->roster.at(18).conditions[3]==0 && h.flow->encounter()->itemUseResult() &&
      h.flow->encounter()->result().movementOpportunities==1,"Presented target publishes once and services owed work");
     input(WaitAction{});settle();input(SaveGameAction{});check(h.saves>0,"Selector authority further gameplay save");
     std::cout<<"M35 reentrant selector/retry authority PASS\n";return true;
    }
    if(stage=="branches"){
     unsigned beforePotions=0;for(auto owner:kXeenCombatOwners)for(const auto &item:h.party->roster.at(owner).miscellaneous)
      beforePotions+=item.material==10 && item.id==37 && item.state==1;
     check(beforePotions==5 && !h.party->roster.at(0).conditions[3],"Genuine healthy-target branch entry");
     const auto freeBefore=XeenSaveFormat::encode(XeenSaveState::capture(h.signature,*h.party,*h.camera,*h.flags,*h.world));
     std::size_t freeSlot=9;for(std::size_t i=0;i<9;++i){const auto &item=h.party->roster.at(0).miscellaneous[i];
      if(item.material==10 && item.id==37 && item.state==1){freeSlot=i;break;}}
     check(freeSlot<9,"Free cancel has selected delivered antidote");
     input(InspectInventoryAction{});for(int i=0;i<3;++i)input(NavigationAction::TurnRight);
     input(SelectInventorySlotAction{freeSlot});input(UseItemAction{});
     check(h.flow->inventorySelection().mode==XeenInventoryMode::UseConfirm,"Pre-use confirmation");
     input(CancelInteractionAction{});input(CancelInteractionAction{});
     check(h.flow->canSave() && freeBefore==XeenSaveFormat::encode(XeenSaveState::capture(h.signature,*h.party,*h.camera,*h.flags,*h.world)),
      "Pre-use Escape is free and owes no actor opportunity");
     input(InteractionAction{});settle();
     check(h.party->questFlags.isSet(2) && !h.party->questItems.at(17),"Post-exchange no-Root request");
     const auto use=[&](bool cancel){
      std::size_t slot=9;for(std::size_t i=0;i<9;++i){const auto &item=h.party->roster.at(0).miscellaneous[i];
       if(item.material==10 && item.id==37 && item.state==1){slot=i;break;}}
      check(slot<9,"Genuine branch delivered antidote");
      const auto minutes=h.party->encounterContext->minutes,ctr=h.party->encounterContext->ctr24;
      input(InspectInventoryAction{});for(int i=0;i<3;++i)input(NavigationAction::TurnRight);
      input(SelectInventorySlotAction{slot});input(UseItemAction{});
      check(h.flow->inventorySelection().mode==XeenInventoryMode::UseConfirm,"Branch use confirmation");
      input(AcknowledgeAction{});
      check(h.flow->inventorySelection().mode==XeenInventoryMode::UseTarget,"Branch debit frame");
      const auto savesBeforeTarget=h.saves;
      check(!h.flow->canSave(),"Debited target selector blocks capture");
      input(SaveGameAction{});
      check(h.saves==savesBeforeTarget && h.flow->inventorySelection().mode==XeenInventoryMode::UseTarget,
       "F9 cannot save or dismiss debited target selector");
      input(cancel?PlayerAction{CancelInteractionAction{}}:PlayerAction{SelectMemberAction{0}});
      check(h.party->encounterContext->minutes==minutes && h.party->encounterContext->ctr24==ctr,
       "Selection and effect have zero direct time cost");
      settle();
      check(h.flow->encounter()->itemUseResult() &&
       bool(h.flow->encounter()->itemUseResult()->target)==!cancel &&
       h.flow->encounter()->result().movementOpportunities==1,
       "Branch one owed opportunity");
     };
     use(false);check(!h.party->roster.at(0).conditions[3],"Healthy target remains healthy");
     use(true);
     unsigned afterPotions=0;for(auto owner:kXeenCombatOwners)for(const auto &item:h.party->roster.at(owner).miscellaneous)
      afterPotions+=item.material==10 && item.id==37 && item.state==1;
     check(afterPotions==3,"Healthy/cancelled uses spent exactly two delivered potions");
     input(WaitAction{});settle();input(SaveGameAction{});check(h.saves>0,"Branch further gameplay save");
     std::cout<<"M35 genuine no-Root/healthy/cancel branches PASS\n";return true;
    }
    if(stage=="all" || stage=="collected" || stage=="full"){
    route("LUUURUULURUULUUUUUUURRUURUUUL");
    check(h.camera->x==8 && h.camera->y==2 && h.camera->direction==XeenDirection::North,"Connected Phirna arrival");
    input(InteractionAction{});settle(false);
    check(!h.party->questItems.at(17) && !h.world->sessionState().disabledObjects().count({23,13}),"Genuine Phirna No leaves Root and object");
    input(InteractionAction{});settle(true);
    check(h.party->questItems.at(17)==1 && h.world->sessionState().disabledObjects().count({23,13})==1,
     "Connected Phirna Root and Remove");
    const auto collected=XeenSaveFormat::encode(XeenSaveState::capture(h.signature,*h.party,*h.camera,*h.flags,*h.world));
    input(InteractionAction{});settle();
    check(collected==XeenSaveFormat::encode(XeenSaveState::capture(h.signature,*h.party,*h.camera,*h.flags,*h.world)),
     "Collected Phirna revisit cannot replay collection");
    if(stage=="collected") {input(SaveGameAction{});check(h.saves>0,"Collected F9 save");return true;}
    fullCheckpoint("collected");
    }
    if(stage=="all" || stage=="return" || stage=="full"){
    route("UULUUURUUU");
    if(stage=="return") {input(SaveGameAction{});check(h.saves>0,"Root return F9 save");return true;}
    fullCheckpoint("return");
    }
    if(stage=="all" || stage=="exchange" || stage=="full" || rewardVariant || stage=="myra-take-fault"){
    route("RUULURUULUUUL");
    check(h.camera->x==9 && h.camera->y==11 && h.camera->direction==XeenDirection::West,"Connected Myra return");
    check(h.party->encounterContext->minutes==848 && h.world->sessionState().journeyRandom()->count==281,
     "Quest events preserve outbound and return clock/RNG");
    std::optional<XeenSaveSnapshot> overlayBefore;
    std::optional<XeenSaveSnapshot> overlayExpected;
    if(rewardVariant){
     overlayBefore=XeenSaveState::capture(h.signature,*h.party,*h.camera,*h.flags,*h.world);
     overlayExpected=*overlayBefore;
     --overlayExpected->questItems[17];overlayExpected->questFlags[2]=false;
     XeenPartyState recipient;recipient.party=XeenParty::fromRosterIds(h.party->party.activeRosterIds());
     for(auto owner:h.party->party.activeRosterIds())recipient.roster.at(owner)=h.party->roster.at(owner);
     XeenPendingRewards rewards;
     for(unsigned i=0;i<5;++i)if(!(overlayMask&(1u<<i)))rewards.enqueue({10,37,1,0});
     if(rewards.hasWork())xeenDeliverRewards(rewards,recipient,{});
     for(auto owner:h.party->party.activeRosterIds())overlayExpected->characters[owner].miscellaneous=recipient.roster.at(owner).miscellaneous;
    }
    if(stage=="myra-take-fault"){
     const auto before=XeenSaveState::capture(h.signature,*h.party,*h.camera,*h.flags,*h.world);
     input(InteractionAction{});settle();
     auto expected=before;--expected.questItems[17];expected.questFlags[2]=false;
     save_test::sameSnapshot(expected,XeenSaveState::capture(h.signature,*h.party,*h.camera,*h.flags,*h.world));
     check(h.flow->canSave() && rewardReceipts==0,"Root consumption survives absent reward producer");
     h.flow->beforeRewardEnqueue={};
     input(InteractionAction{});settle();
     check(h.party->questFlags.isSet(2) && !h.party->questItems.at(17),
      "Subsequent Myra request after consumed Root");
     input(WaitAction{});settle();input(SaveGameAction{});check(h.saves>0,"Take fault further gameplay save");
     std::cout<<"M35 Myra consumed-Root-before-reward full-state prefix PASS\n";return true;
    }
    input(InteractionAction{});settle();
    check(!h.party->questItems.at(17) && !h.party->questFlags.isSet(2),"Connected Myra exchange");
    if(rewardVariant){
     const auto actual=XeenSaveState::capture(h.signature,*h.party,*h.camera,*h.flags,*h.world);
     save_test::sameSnapshot(*overlayExpected,actual);
     check(rewardReceipts==(overlayMask==31?0u:1u) && rewardWarnings==unsigned(overlayWarning) &&
      receiptFaultRaised==receiptFault,
      "Effective reward queue follows enabled producers and original warning/receipt");
     input(InteractionAction{});settle();
     check(h.party->questFlags.isSet(2) && !h.party->questItems.at(17),
      "Legitimate post-prefix Myra request remains available");
     input(WaitAction{});settle();input(SaveGameAction{});check(h.saves>0,"Overlay further gameplay save");
     std::cout<<(receiptFault?"M35 reward receipt fault prefix PASS":"M35 effective Myra reward overlay PASS")
      <<" mask="<<overlayMask<<" warning="<<overlayWarning<<'\n';return true;
    }
    unsigned potions=0;for(auto owner:kXeenCombatOwners)for(const auto &item:h.party->roster.at(owner).miscellaneous)
     potions+=item.material==10 && item.id==37 && item.state==1;
    check(potions==5,"Five actual Myra antidotes delivered");
    if(stage=="exchange") {input(SaveGameAction{});check(h.saves>0,"Exchange F9 save");return true;}
    fullCheckpoint("exchange");
    }
    if(stage=="recovery" || stage=="continue"){
     check(!h.party->questItems.at(17) && !h.party->questFlags.isSet(2),"Restored exchange state");
    }
    if(stage=="all" || stage=="recovery" || stage=="full"){
    route("LUUURUULU");
    check(h.camera->x==7 && h.camera->y==7 && h.camera->direction==XeenDirection::South,"Connected well arrival");
    check(h.party->encounterContext->minutes==910 && h.world->sessionState().journeyRandom()->count==315,
     "Connected recovery baseline clock/RNG");
    const auto hp=h.party->roster.at(11).currentHp;
    const auto beforeWellMinutes=h.party->encounterContext->minutes;
    const auto beforeWellDraws=h.world->sessionState().journeyRandom()->count;
    input(InteractionAction{});
    check(pending && pending->response==XeenPresentationResponseRequirement::CharacterSelection,
     "Original well WhoWill selection");
    input(SelectMemberAction{3});
    check(h.party->roster.at(11).currentHp==hp+25 && !h.party->regionalRecovery->worldFlag16 &&
     h.party->encounterContext->minutes==beforeWellMinutes && h.world->sessionState().journeyRandom()->count==beforeWellDraws,
     "Well HP publication precedes acknowledgment/flag without time or RNG");
    const auto savesBeforeWellAck=h.saves;
    check(!h.flow->canSave(),"Well acknowledgment blocks capture");
    input(SaveGameAction{});
    check(h.saves==savesBeforeWellAck && !h.party->regionalRecovery->worldFlag16,
     "F9 cannot save or acknowledge partial well publication");
    settle();
    check(h.party->roster.at(11).currentHp==hp+25 && h.party->regionalRecovery->worldFlag16,"Selected well recovery");
    check(h.party->roster.at(18).conditions[3]==1,"Genuine Poison target");
    std::size_t slot=9;for(std::size_t i=0;i<9;++i){const auto &item=h.party->roster.at(0).miscellaneous[i];
     if(item.material==10 && item.id==37 && item.state==1)slot=i;}
    check(slot<9,"Delivered source antidote");
    input(InspectInventoryAction{});for(int i=0;i<3;++i)input(NavigationAction::TurnRight);
    input(SelectInventorySlotAction{slot});input(UseItemAction{});
    check(h.flow->inventorySelection().mode==XeenInventoryMode::UseConfirm,"Antidote confirmation");
    input(AcknowledgeAction{});check(h.flow->inventorySelection().mode==XeenInventoryMode::UseTarget,"Antidote debit target phase");
    const auto savesBeforeCureTarget=h.saves;
    check(!h.flow->canSave(),"Cure target selector blocks capture");
    input(SaveGameAction{});
    check(h.saves==savesBeforeCureTarget && h.flow->inventorySelection().mode==XeenInventoryMode::UseTarget,
     "F9 cannot save or dismiss cure target selector");
    input(SelectMemberAction{1});settle();
    check(!h.party->roster.at(18).conditions[3] && h.flow->encounter()->itemUseResult() &&
     h.flow->encounter()->itemUseResult()->exhausted,"Actual delivered antidote cured Poison");
    if(stage=="recovery") {input(SaveGameAction{});check(h.saves>0,"Recovery F9 save");return true;}
    fullCheckpoint("recovery");
    }
    if(stage=="continue" || stage=="full"){
     route("LU");
     for(unsigned i=0;i<6;++i){input(WaitAction{});settle();}
     check(h.party->encounterContext->minutes>=960,"Post-antidote gameplay crossed minute 960");
     input(SaveGameAction{});check(h.saves>0,"Continued F9 save");
    }
    std::cout<<"M35 connected route request/Root/exchange/well/antidote PASS\n";return true;
   };
   const bool resume=stage=="collected" || stage=="return" || stage=="exchange" || rewardVariant || treasureMyra || treasureWell || stage=="item-draw-fault" || stage=="recovery" || stage=="continue" || stage=="branches" || stage=="selector-authority" || stage=="selector-aba" || stage=="phirna-grant-fault" || stage=="myra-take-fault" || stage=="item-owed-fault" ||
    stage=="well-repeat" || stage=="well-equal" || stage=="well-frame-retry" || stage=="well-text-fault" || stage=="run-restart";
   check(Application().playGameplay(s,{},path,resume,resume?XeenEncounterEntry::Ordinary:XeenEncounterEntry::Journey,
    resume?std::optional<std::uint32_t>{}:std::optional<std::uint32_t>{stage=="run-quest"||stage=="run-full"?runSeed:7u},
    resume?std::optional<std::uint16_t>{}:std::optional<std::uint16_t>{6})==0,"Connected regional stage");
  }
  if(staged) {std::cout<<"M35 STAGE PASS "<<stage<<'\n';return 0;}
  // Representation-only persistent activation permits each restored quiet view.
  // No actor is removed or relocated, and this is not a navigation witness.
  for(auto &a:source.journey->actors)a.activated=true;
  for(auto cell:{std::pair<int,int>{0,1},{4,5},{5,9},{5,13},{7,7},{8,2},{8,10},{9,11},{10,13},{12,12}})for(unsigned d=0;d<4;++d) {
   auto saved=source;saved.camera={23,cell.first,cell.second,static_cast<XeenDirection>(d)};XeenSaveFile::write(path,saved);
   Harness h(game);auto s=regional(h);unsigned noEvent=0,completed=0;
   s.configureFlow=[&](auto &f,const auto &){h.flow=&f;f.reportManual=[&](const auto &r){noEvent+=std::holds_alternative<XeenManualEventNoEvent>(r);completed+=std::holds_alternative<XeenManualEventCompleted>(r);};};
   bool shown=false;
   s.show=[&](const auto &,const auto &handler,const auto &,const auto &idle,const auto &){
    shown=true;handler.framePresented(h.flow->frame().presentation());const auto before=bytes(h);
    const bool sign=cell.first==5 && cell.second==9 && d==0;
    const auto expected=xeenRegionalEvent(h.eventLoader->load(23),*h.camera);
    handler.beginCycle(++h.cycle);const auto input=*handler.displayedInput();handler.withDisplayedInput(InteractionAction{},input);
    check(!h.flow->canSave(),"Manual interaction must present its result before capture");
    for(const PlayerAction &a:std::vector<PlayerAction>{NavigationAction::MoveForward,WaitAction{},SaveGameAction{},InspectInventoryAction{}})handler.withDisplayedInput(a,input);
    h.world->discardMapCache();h.eventSystem->discardScriptCache();h.eventSystem->discardTextCache();
    h.flow->refresh(true);handler.framePresented(h.flow->frame().presentation());
    for(unsigned n=0;!h.flow->canSave() && n<100;++n){h.now+=100;handler.beginCycle(++h.cycle);idle();handler.framePresented(h.flow->frame().presentation());}
    check(h.flow->canSave() && bytes(h)==before,"Manual address/facing, stale input and reconstruction preserve complete state");
    check(sign ? completed==1 : expected ? completed==0 && noEvent==0 : noEvent==1,"Original manual admission result");
    if(expected && !sign)check(h.flow->encounter()->notice().find("Unsupported event "+std::to_string(*expected))!=std::string::npos,"Unsupported original record notice");
    rejects([&]{h.flow->acceptAutomatic(XeenAutomaticEventCompleted{});});
    return true;
   };
   check(Application().playGameplay(s,{},path,true)==0 && shown,"Regional manual event case");
  }
  for(unsigned fault=0;fault<5;++fault) {
   auto saved=source;saved.camera={23,5,9,XeenDirection::North};if(fault==0 || fault==1)saved.disabledEvents={{23,56}};
   XeenSaveFile::write(path,saved);Harness h(game);auto s=regional(h);bool changed=false,shown=false;
   const auto load=s.resources.loadEvents;
   s.resources.loadEvents=[&](auto id){auto f=load(id);if(changed)f.records[56].parameters={17};return f;};
   s.show=[&](const auto &,const auto &handler,const auto &,const auto &,const auto &){
    shown=true;handler.framePresented(h.flow->frame().presentation());const auto before=bytes(h);
    if(fault<=1) {
     handler.beginCycle(++h.cycle);handler.withDisplayedInput(InteractionAction{},*handler.displayedInput());handler.framePresented(h.flow->frame().presentation());
     check(h.flow->canSave() && bytes(h)==before,"Effective None sign preserves independent overlay");
    }
    if(fault==0)return true;
    if(fault==1 || fault==2){changed=true;h.eventSystem->discardScriptCache();}
    if(fault==3)const_cast<XeenMap &>(h.world->map(23)).geometry.runX=11;
    if(fault==4)const_cast<XeenActor &>(h.world->sessionState().actors()[0]).statistics->raw[20]^=1;
    bool failed=false;try{h.flow->refresh(true);handler.framePresented(h.flow->frame().presentation());}catch(const std::exception &){failed=true;}
    check(failed && !h.flow->canSave(),"Changed warm/reloaded resources fail, including disabled sign");return true;
   };
   check(Application().playGameplay(s,{},path,true)==0 && shown,"Regional event resource failure case");
  }
  std::cout<<"Regional original event addresses: 40 facing cases, disabled sign and four resource-failure controls passed\n";return 0;
 }catch(const std::exception &e){std::cerr<<e.what()<<'\n';return 1;}
}
