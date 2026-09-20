// Test-only SDL input adapter around the real CLI and production construction.
#include "app/Application.h"
#include "app/XeenGameplayServices.h"
#include "app/XeenEventFlow.h"
#include "formats/xeen/XeenSaveFormat.h"
#include "platform/XeenSaveFile.h"
#include "XeenRestoreReplayProbe.h"
#include "games/xeen/XeenStateEquality.h"
#include "XeenSaveTestSupport.h"
#define SDL_MAIN_HANDLED
#include <SDL.h>
#include <cstdlib>
#include <fstream>
#include <iostream>
#include <deque>
#include <set>
#include "games/xeen/XeenCharacterRules.h"
#include "games/xeen/XeenRestoreGuard.h"
using namespace mmodern;
namespace fs=std::filesystem;
#define PLAY_SYMBOL "_ZNK7mmodern11Application12playGameplayERKNS_20XeenGameplayServicesENS_10XeenCameraERKSt8optionalINSt10filesystem7__cxx114pathEEbNS_18XeenEncounterEntryES5_IjES5_ItE"
extern "C" int realPlay(const Application *,const XeenGameplayServices &,XeenCamera,const std::optional<fs::path> &,bool,XeenEncounterEntry,std::optional<std::uint32_t>,std::optional<std::uint16_t>) asm("__real_" PLAY_SYMBOL);
extern "C" int wrappedPlay(const Application *,const XeenGameplayServices &,XeenCamera,const std::optional<fs::path> &,bool,XeenEncounterEntry,std::optional<std::uint32_t>,std::optional<std::uint16_t>) asm("__wrap_" PLAY_SYMBOL);

namespace {
void check(bool v,const char *m) { if(!v) throw std::runtime_error(m); }
}
extern "C" int wrappedPlay(const Application *app,const XeenGameplayServices &original,XeenCamera camera,
 const std::optional<fs::path> &target,bool resume,XeenEncounterEntry entry,std::optional<std::uint32_t> seed,std::optional<std::uint16_t> contract) {
 try {
  std::function<void(std::uint64_t)> drawFault;
  std::vector<XeenCombatRandom::Draw> trace;
  replay_test::observeDraw=[&](auto lo,auto hi,auto value,auto state){if(value)trace.push_back({lo,hi,*value});std::cout<<"DRAW ["<<lo<<','<<hi<<"] "<<(value?std::to_string(*value):"rejected")<<" count="<<state.count<<'\n';if(drawFault)drawFault(state.count);};
  struct ClearTrace {~ClearTrace(){replay_test::observeDraw={};}} clearTrace;
  auto services=original;XeenEventFlow *flow=nullptr;const XeenPartyState *party=nullptr;XeenWorld *world=nullptr;const XeenCamera *position=nullptr;const XeenGameFlags *flags=nullptr;
  unsigned automatic=0;
  unsigned saveCalls=0;services.observeSaveStage=[&](auto){++saveCalls;};
  std::uint64_t now=0;services.clock=[&]{return now;};
  const std::string fault=std::getenv("MMODERN_M33_FAULT")?std::getenv("MMODERN_M33_FAULT"):"";
  bool faultFired=false;std::function<void()> verifyPublication;
  const auto failDraw=std::getenv("MMODERN_M33_FAIL_DRAW")?std::strtoull(std::getenv("MMODERN_M33_FAIL_DRAW"),nullptr,10):0;
  bool drawFaultFired=false;std::function<void()> verifyFailedUnit;
  drawFault=[&](auto count){if(!failDraw || count!=failDraw || drawFaultFired)return;
   const auto characters=party->roster.characters();const auto actors=world->sessionState().actors();const auto rng=world->sessionState().journeyRandom();
   const auto treasure=party->monsterTreasure;const auto context=party->encounterContext;const auto camera=*position;
   std::array<std::optional<XeenCombatInputs>,30> inputs;for(unsigned i=0;i<30;++i)inputs[i]=party->roster.combatInputs(i);
   verifyFailedUnit=[&,characters,actors,rng,treasure,context,camera,inputs]{
    for(unsigned i=0;i<30;++i){check(xeen_state::sameCharacter(characters[i],party->roster.at(i)),"Failed unit changed character");check(xeen_state::sameInputs(*inputs[i],*party->roster.combatInputs(i)),"Failed unit changed XP/input");}
    for(unsigned i=0;i<actors.size();++i)check(xeen_state::sameActor(actors[i],world->sessionState().actors()[i]),"Failed unit partially published actors");
    check(rng==world->sessionState().journeyRandom() && treasure==party->monsterTreasure && context==party->encounterContext && xeen_state::sameCamera(camera,*position),"Failed unit changed RNG/time/treasure");
    check(!flow->canSave(),"Failed unit reopened save");
   };
   drawFaultFired=true;throw std::bad_alloc();
  };
  const auto configure=services.configureFlow;
  services.configureFlow=[&](auto &f,const auto &c){if(configure)configure(f,c);flow=&f;
   const auto report=f.reportAutomatic;f.reportAutomatic=[&,report](const auto &r){if(std::holds_alternative<XeenAutomaticEventCompleted>(r))++automatic;if(report)report(r);};
   f.beforeEncounterFrameCopy=[&]{
    if(fault.empty() || faultFired || !world || !party)return;
    const auto activity=world->sessionState().journeyActivity();const auto draws=world->sessionState().journeyRandom()->count;
    const bool eligible=(fault=="shoot" && draws==5) || (fault=="ranged" && draws==8) ||
     (fault=="lethal" && flow->encounter()->combat() && world->sessionState().actors()[9].lifecycle==XeenActorLifecycle::Defeated) ||
     (fault=="delivery" && activity==XeenJourneyActivity::Reward) || (fault=="credit" && party->monsterTreasure->gold==810);
    if(!eligible)return;
    const auto characters=party->roster.characters();const auto actors=world->sessionState().actors();const auto rng=world->sessionState().journeyRandom();
    const auto treasure=party->monsterTreasure;const auto context=party->encounterContext;const auto camera=*position;
    std::array<std::optional<XeenCombatInputs>,30> inputs;for(unsigned i=0;i<30;++i)inputs[i]=party->roster.combatInputs(i);
    verifyPublication=[&,characters,actors,rng,treasure,context,camera,inputs]{
     for(unsigned i=0;i<30;++i){check(xeen_state::sameCharacter(characters[i],party->roster.at(i)),"Post-publication retry changed character");check(xeen_state::sameInputs(*inputs[i],*party->roster.combatInputs(i)),"Post-publication retry changed supplement");}
     for(unsigned i=0;i<actors.size();++i)check(xeen_state::sameActor(actors[i],world->sessionState().actors()[i]),"Post-publication retry changed actor");
     check(rng==world->sessionState().journeyRandom() && treasure==party->monsterTreasure && context==party->encounterContext && xeen_state::sameCamera(camera,*position),"Retry rerolled/recredited predecessor");
     std::cout<<"ARTIFICIAL PRESENTATION FAULT RECOVERY PASS "<<fault<<'\n';
    };
    faultFired=true;throw std::bad_alloc();
   };
  };
  services.observeGameplay=[&](auto &w,auto &,const auto &p,auto &c,const auto &f){world=&w;party=&p;position=&c;flags=&f;};
  const std::string route=std::getenv("MMODERN_M33_ROUTE")?std::getenv("MMODERN_M33_ROUTE"):"UFU";
  services.show=[&](const auto &,const auto &handler,const auto &,const auto &idle,const auto &) {
   try {
   auto present=[&] {check(handler.frameCurrent(),"Current production frame");handler.framePresented(flow->frame().presentation());};
   present();
   if(resume) {
    check(bool(target),"Resume target");const auto saved=XeenSaveFile::read(*target);
    const auto restored=XeenSaveState::capture(original.resources.signature,*party,*position,*flags,*world);
    save_test::sameSnapshot(saved,restored);check(XeenSaveFormat::encode(saved)==XeenSaveFormat::encode(restored),"Fresh-process initial exact bytes");
    std::cout<<"FRESH PROCESS FULL STATE AND BYTES PASS\n";
   }
   std::array<std::array<unsigned,35>,2> initialItems{};
   for(auto owner:kXeenCombatOwners)for(unsigned category=0;category<2;++category)for(const auto &v:category?party->roster.at(owner).armor:party->roster.at(owner).weapons)if(v.id<35)++initialItems[category][v.id];
   std::size_t cursor=0;unsigned steps=0;
   unsigned poison=0,sleep=0,disease=0;std::uint32_t contacts=0;unsigned maxContacts=0;bool generatedShoot=false;bool wasCombat=false;unsigned episodes=0;
   unsigned wake=0,reapply=0,allParty=0,poisonDerived=0,sleepSkipped=0,enemyProjectile=0,playerProjectile=0,blockedSaves=0;
   std::array<XeenCharacter,6> preceding;for(unsigned i=0;i<6;++i)preceding[i]=party->roster.at(kXeenCombatOwners[i]);
   std::uint64_t observedCombatRevision=0;std::set<unsigned> saveExclusions;
   const bool summary=std::getenv("MMODERN_M33_SUMMARY")!=nullptr;
   auto verify=[&]{if(verifyPublication){verifyPublication();verifyPublication={};}};
   auto input=[&](const PlayerAction &action){const auto old=*handler.displayedInput();handler.withDisplayedInput(action,old);present();verify();
    if(old!=*handler.displayedInput()){XeenRestoreGuard retained(*world,*party,*position,*flags);const auto before=saveCalls;handler.withDisplayedInput(action,old);check(retained.current() && saveCalls==before,"Stale displayed input changed owners or entered save provider");present();}
   };
   while(++steps<10000) {
    for(unsigned i=0;i<6;++i){const auto &c=party->roster.at(kXeenCombatOwners[i]);const auto &old=preceding[i];
     if(old.conditions[8] && c.currentHp<old.currentHp){++wake;if(c.conditions[8])++reapply;}
     if(c.conditions[3] && !old.conditions[3]){const auto &in=*party->roster.combatInputs(c.rosterId);if(XeenCharacterRules::effectivePhysical(c,in,XeenCharacterRules::PhysicalAttribute::Might,{party->encounterContext->year})!=XeenCharacterRules::effectivePhysical(old,in,XeenCharacterRules::PhysicalAttribute::Might,{party->encounterContext->year}))++poisonDerived;}
     preceding[i]=c;
    }
    const auto appearance=flow->encounter()->appearance();if(appearance.projectile){if(appearance.projectile->enemy)++enemyProjectile;else ++playerProjectile;}
    if(!flow->canSave()){
     const unsigned state=unsigned(world->sessionState().journeyActivity())*32+(flow->encounter()->combat()?unsigned(flow->encounter()->combat()->phase()):flow->encounter()->state().pending());
     if(saveExclusions.insert(state).second){const auto before=saveCalls;input(SaveGameAction{});check(saveCalls==before,"Blocked F9 entered save provider");++blockedSaves;}
    }

    if(flow->encounter()->notice().find("Shoot owner 0")!=std::string::npos)generatedShoot=true;
    for(auto owner:kXeenCombatOwners) { const auto &c=party->roster.at(owner);poison|=c.conditions[3];sleep|=c.conditions[8];disease|=c.conditions[4]; }
    if(flow->encounter()->combat()) {
     if(!wasCombat)++episodes;wasCombat=true;
     unsigned count=0;for(const auto &id:flow->encounter()->combat()->contacts())if(id){contacts|=1u<<id->recordIndex;++count;}maxContacts=std::max(maxContacts,count);
     const auto &observation=flow->encounter()->combatObservation();
     if(observation.revision!=observedCombatRevision){observedCombatRevision=observation.revision;if(observation.actingMonster && (observation.actingMonster->recordIndex==14 || observation.actingMonster->recordIndex==15)) {if(observation.targetedMembers==63)++allParty;}}
     const auto phase=flow->encounter()->combat()->phase();
     if(phase==XeenCombatPhase::PlayerReady) {
      check(party->roster.at(kXeenCombatOwners[flow->encounter()->combat()->participant()]).canAct(),"Sleeping/disabled member received ready turn");
      for(auto id:kXeenCombatOwners)if(party->roster.at(id).conditions[8])++sleepSkipped;
      const auto rows=flow->encounter()->combat()->contacts();
      unsigned selected=0;for(unsigned i=0;i<rows.size();++i)if(rows[i] && (!rows[selected] || rows[i]->recordIndex<rows[selected]->recordIndex))selected=i;
      if(!(rows[selected]==flow->encounter()->combat()->selectedTarget())) {input(SelectCombatTargetAction{selected});continue;}
      input(AttackAction{});continue;}
     if(phase==XeenCombatPhase::Defeat || phase==XeenCombatPhase::Failed || phase==XeenCombatPhase::SupportStopped)std::cout<<"TERMINAL phase="<<unsigned(phase)<<" failure="<<unsigned(flow->encounter()->combat()->result().failure)<<" route="<<cursor<<" poison="<<poison<<" sleep="<<sleep<<" disease="<<disease<<" contacts="<<contacts<<" episodes="<<episodes<<'\n';
     // Successful End is serviced by the production idle boundary.
     check(phase!=XeenCombatPhase::Defeat && phase!=XeenCombatPhase::Failed && phase!=XeenCombatPhase::SupportStopped,"Combat terminal witness");
    } else if(world->sessionState().journeyActivity()==XeenJourneyActivity::Reward || world->sessionState().journeyActivity()==XeenJourneyActivity::Event) {input(AcknowledgeAction{});continue;}
    if(flow->encounter()->journeyMutable()) {
     if(wasCombat)std::cout<<"EPISODE QUIET route="<<cursor<<" episodes="<<episodes<<"\n";
     wasCombat=false;if(!summary)std::cout<<"CHECKPOINT "<<cursor<<'\n'<<flow->encounter()->journeyInspection();
     if(cursor==route.size()) {
      if(route=="LUUURUUUURU")check(automatic==1 && position->x==5 && position->y==9,"Production contract-4 automatic sign");
      if(resume && route.empty())check(automatic==0,"Restart must not replay automatic event");
      std::cout<<"AUTOMATIC EVENTS "<<automatic<<'\n';
      if(target) {input(SaveGameAction{});check(fs::exists(*target),"F9 wrote save");}
      check(fault.empty() || faultFired,"Requested artificial presentation fault reached");
      const auto snapshot=XeenSaveState::capture(original.resources.signature,*party,*position,*flags,*world);
      save_test::sameSnapshot(snapshot,XeenSaveFormat::decode(XeenSaveFormat::encode(snapshot)));
      check(XeenSaveFormat::encode(XeenSaveFormat::decode(XeenSaveFormat::encode(snapshot)))==XeenSaveFormat::encode(snapshot),"Exact codec");
      if(const auto expected=std::getenv("MMODERN_M33_EXPECT")) {const auto control=XeenSaveFile::read(expected);save_test::sameSnapshot(control,snapshot);check(XeenSaveFormat::encode(control)==XeenSaveFormat::encode(snapshot),"Uninterrupted/resume exact bytes");std::cout<<"CONTROL FULL SEMANTIC STATE AND BYTES PASS\n";}
      if(const auto oracle=std::getenv("MMODERN_M33_ORACLE")) {
       const std::string name=oracle;const auto &j=*snapshot.journey;
       const bool wound=name=="wound",contact=name=="contact";check(wound||contact||name=="ranged","Named fixed oracle");
       check(j.schema==4 && j.contract==4 && snapshot.camera.x==(contact?7:8) && snapshot.camera.y==11 && snapshot.camera.direction==XeenDirection::West,"Fixed camera and version");
       check(j.context->minutes==(wound?500:contact?511:510) && j.context->ctr24==(contact?2:1),"Fixed clock oracle");
       check(j.random->state==(wound?0x4a767d04u:contact?0x692b3851u:0xb8d3b48au) && j.random->count==(wound?8:contact?22:14),"Fixed RNG oracle");
       const std::array<int,6> hp{36,contact?43:48,36,40,21,15};
       const std::array<unsigned,6> xp{1000,2000,1000,1000,2000,1000};
       for(unsigned i=0;i<6;++i){const auto id=kXeenCombatOwners[i];check(snapshot.characters[id].currentHp==hp[i] && j.supplements[id].inputs.experience==xp[i]+(wound?0:66),"Fixed HP and XP oracle");}
       check(j.actors[9].hp==(wound?16:0) && j.actors[9].accounted==!wound && j.treasure->gold==(wound?800:810) && j.treasure->gems==10 && !j.treasure->pending(),"Fixed wound/accounting/purse oracle");
       const std::vector<XeenCombatRandom::Draw> prefix{{1,2,2},{1,2,2},{1,2,2},{1,20,14},{1,56,4},{0,5,1},{1,20,4},{1,5,2}};
       const std::vector<XeenCombatRandom::Draw> contactTail{{0,5,1},{1,20,16},{1,5,4},{1,10,5},{1,2,2},{1,2,1},{1,2,2},{1,2,2},{1,20,8},{1,100,8},{0,100,48},{0,100,50},{1,7,2},{1,100,46}};
       const std::vector<XeenCombatRandom::Draw> rangedTail{{1,2,2},{1,2,2},{1,2,2},{1,20,15},{1,56,20},{1,100,59}};
       std::vector<XeenCombatRandom::Draw> expected;if(!resume)expected=prefix;
       if(!wound){const auto &tail=contact?contactTail:rangedTail;expected.insert(expected.end(),tail.begin(),tail.end());}
       check(trace.size()==expected.size(),"Fixed accepted draw count");for(unsigned i=0;i<trace.size();++i)check(trace[i].lo==expected[i].lo && trace[i].hi==expected[i].hi && trace[i].value==expected[i].value,"Fixed literal draw interval/value/order");
       std::cout<<"FIXED ORACLE PASS "<<name<<'\n';
      }
      for(unsigned category=0;category<2;++category){auto counts=initialItems[category];std::array<unsigned,35> after{};for(auto owner:kXeenCombatOwners)for(const auto &v:category?party->roster.at(owner).armor:party->roster.at(owner).weapons)if(v.id<35)++after[v.id];for(unsigned id=1;id<35;++id)if(after[id]>counts[id])std::cout<<"LOOT "<<(category?"armor":"weapon")<<" id="<<id<<'\n';}
      unsigned livingPoison=0;for(auto id:kXeenCombatOwners)if(xeenCombatTargetable(party->roster.at(id)) && party->roster.at(id).conditions[3])++livingPoison;
      std::cout<<"STATE minute="<<party->encounterContext->minutes<<" pendingGold="<<party->monsterTreasure->pendingGold<<'\n';
      std::cout<<"LIVING POISON "<<livingPoison<<'\n';
      std::cout<<"CONDITIONS wake="<<wake<<" reapply="<<reapply<<" allParty="<<allParty<<" poisonDerived="<<poisonDerived<<" sleepSkipped="<<sleepSkipped<<" enemyProjectile="<<enemyProjectile<<" playerProjectile="<<playerProjectile<<" blockedSaves="<<blockedSaves<<'\n';
      std::cout<<"OBSERVATIONS poison="<<poison<<" sleep="<<sleep<<" disease="<<disease<<" contacts="<<contacts<<" grouped="<<maxContacts<<" generatedShoot="<<generatedShoot<<" episodes="<<episodes<<'\n';
      std::cout<<"M33 GAMEPLAY PASS route="<<route<<" count="<<world->sessionState().journeyRandom()->count<<" gold="<<party->monsterTreasure->gold<<'\n';return true;
     }
     const auto key=route[cursor++];
     if(key=='Q'){input(NavigationAction::MoveForward);input(ShootAction{});}
     else if(key=='U')input(NavigationAction::MoveForward);
     else if(key=='L')input(NavigationAction::TurnLeft);
     else if(key=='R')input(NavigationAction::TurnRight);
     else if(key=='W')input(WaitAction{});
     else if(key=='F')input(ShootAction{});
     else if(key=='G') {
      const auto before=XeenSaveState::capture(original.resources.signature,*party,*position,*flags,*world);
      std::size_t slot=9;unsigned generated=0;
      for(std::size_t i=0;i<9;++i){const auto &v=party->roster.at(0).weapons[i];if(v.id && !v.frame && !v.material && !v.state){slot=i;generated=v.id;}}
      check(slot<9,"Actual generated weapon in delivered pack");
      input(InspectInventoryAction{});input(SelectInventorySlotAction{slot});input(TransferInventoryAction{});input(SelectMemberAction{1});input(AcknowledgeAction{});
      input(SelectMemberAction{1});slot=9;for(std::size_t i=0;i<9;++i)if(party->roster.at(18).weapons[i].id==generated && !party->roster.at(18).weapons[i].frame)slot=i;
      check(slot<9,"Generated transfer to Tyro");input(SelectInventorySlotAction{slot});input(TransferInventoryAction{});input(SelectMemberAction{0});input(AcknowledgeAction{});
      input(SelectMemberAction{0});slot=9;for(std::size_t i=0;i<9;++i)if(party->roster.at(0).weapons[i].id==generated && !party->roster.at(0).weapons[i].frame)slot=i;
      check(slot<9,"Generated transfer back to Arturius");
      // The retained first weapon witness is missile ID32, so no melee/shield conflict.
      check(generated==32,"Retained seed64 weapon identity");input(SelectInventorySlotAction{slot});input(EquipmentInventoryAction{});
      check(party->roster.at(0).weapons[slot].frame==4,"Generated missile legally equipped");input(CancelInteractionAction{});
      auto expected=before;expected.characters[0].weapons=party->roster.at(0).weapons;expected.characters[18].weapons=party->roster.at(18).weapons;
      save_test::sameSnapshot(expected,XeenSaveState::capture(original.resources.signature,*party,*position,*flags,*world));
      std::cout<<"PRODUCTION GENERATED WEAPON "<<generated<<" TRANSFER/EQUIP PASS\n";
     }
     else if(key=='A' || key=='E') {
      const auto before=XeenSaveState::capture(original.resources.signature,*party,*position,*flags,*world);
      input(InspectInventoryAction{});check(flow->inventoryOpen(),"Production inventory opens");
      input(NavigationAction::TurnRight);check(flow->inventorySelection().category==XeenInventoryCategory::Armor,"Armor category");
      std::size_t received=9;
      if(key=='A') {
       for(std::size_t i=0;i<9;++i){const auto &v=party->roster.at(0).armor[i];if(v.id==2 && !v.material && !v.state && !v.frame)received=i;}
       check(received<9,"Actual delivered armor in Arturius pack");
       input(SelectInventorySlotAction{received});input(TransferInventoryAction{});input(SelectMemberAction{1});input(AcknowledgeAction{});
       check(flow->inventorySelection().mode==XeenInventoryMode::Browse,"Transfer acknowledged");
       input(SelectMemberAction{1});received=9;std::size_t original=9;
       for(std::size_t i=0;i<9;++i){const auto &v=party->roster.at(18).armor[i];if(v.id==2 && v.frame==3)original=i;if(v.id==2 && !v.frame)received=i;}
       check(original<9 && received<9 && original!=received,"Distinct original and transferred slots");
       input(SelectInventorySlotAction{original});input(EquipmentInventoryAction{});check(!party->roster.at(18).armor[original].frame,"Original armor removed");
      } else {
       input(SelectMemberAction{1});for(std::size_t i=0;i<9;++i)if(party->roster.at(18).armor[i].id==2 && party->roster.at(18).armor[i].frame==3)received=i;
       check(received<9,"Retained received armor");input(SelectInventorySlotAction{received});input(EquipmentInventoryAction{});
      }
      input(SelectInventorySlotAction{received});input(EquipmentInventoryAction{});check(party->roster.at(18).armor[received].frame==3,"Received armor legally equipped");
      input(CancelInteractionAction{});check(!flow->inventoryOpen(),"Inventory closes");
      auto expected=before;expected.characters[0].armor=party->roster.at(0).armor;expected.characters[18].armor=party->roster.at(18).armor;
      save_test::sameSnapshot(expected,XeenSaveState::capture(original.resources.signature,*party,*position,*flags,*world));
      std::cout<<"PRODUCTION ARMOR "<<key<<" slot="<<received<<" PASS\n";
     }
     else throw std::invalid_argument("Unknown transcript action");
     continue;
    }
    check(flow->encounter()->state().phase()!=XeenEncounterPhase::SupportStopped,"Exploration support stop");
    now+=100;if(auto frame=idle())present();verify();
   }
   throw std::runtime_error("Gameplay witness service bound");
   }catch(...){if(drawFaultFired){verifyFailedUnit();const auto before=saveCalls;handler.withDisplayedInput(SaveGameAction{},*handler.displayedInput());check(saveCalls==before,"Failed F9 entered providers");std::cout<<"ARTIFICIAL RAW-DRAW FAILURE ATOMICITY PASS "<<failDraw<<'\n';return false;}throw;}
  };
  check(resume || contract==4,"Fresh production CLI must admit contract 4");
  return realPlay(app,services,camera,target,resume,entry,seed,contract);
 } catch(const std::exception &e) {std::cerr<<"M33 witness: "<<e.what()<<'\n';return 8;}
}
