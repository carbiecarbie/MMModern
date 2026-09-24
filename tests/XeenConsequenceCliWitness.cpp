// Test-only SDL input adapter around the real CLI and production construction.
#include "app/Application.h"
#include "app/XeenGameplayServices.h"
#include "app/XeenEventFlow.h"
#include "formats/xeen/XeenSaveFormat.h"
#include "platform/XeenSaveFile.h"
#include "XeenRestoreReplayProbe.h"
#include "games/xeen/XeenStateEquality.h"
#include "XeenSaveTestSupport.h"
#include "XeenM35CliWitness.h"
#define SDL_MAIN_HANDLED
#include <SDL.h>
#include <cstdlib>
#include <fstream>
#include <iostream>
#include <deque>
#include <set>
#include <sstream>
#include "games/xeen/XeenMovement.h"
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
  if(std::getenv("MMODERN_M35_STAGE")) {
   const auto m35Contract=resume?contract:std::optional<std::uint16_t>{6};
   return runM35CliWitness(original,target,resume,seed,m35Contract,[&](const XeenGameplayServices &services) {
    return realPlay(app,services,camera,target,resume,entry,seed,m35Contract);
   });
  }
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
   const auto characters=party->roster.characters();const std::vector<XeenActor> actors=world->sessionState().actors();const auto rng=world->sessionState().journeyRandom();
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
    const auto characters=party->roster.characters();const std::vector<XeenActor> actors=world->sessionState().actors();const auto rng=world->sessionState().journeyRandom();
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
  const bool m34=std::getenv("MMODERN_M34_POLICY")!=nullptr;
  const bool stopAfterExit=std::getenv("MMODERN_M34_STOP_AFTER_EXIT")!=nullptr;
  const auto checkpointFile=std::getenv("MMODERN_M34_CHECKPOINT_FILE");
  const auto checkpointRoute=std::getenv("MMODERN_M34_CHECKPOINT_ROUTE")?std::strtoul(std::getenv("MMODERN_M34_CHECKPOINT_ROUTE"),nullptr,10):0;
  const std::string policyText=m34?std::getenv("MMODERN_M34_POLICY"):"attack";
  std::vector<std::string> policies;std::istringstream policyStream(policyText);std::string policy;
  while(std::getline(policyStream,policy,',')){check(policy=="attack"||policy=="run"||policy=="mixed"||policy=="first-run-attack"||policy=="first-run-block"||policy=="item-run"||policy=="item-first-run-block"||policy=="pending-first-run-block","Unknown combat policy");policies.push_back(policy);}
  check(!policies.empty(),"Combat policy is required");
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
   std::size_t cursor=0;unsigned steps=0;bool checkpointObserved=false;
   unsigned poison=0,sleep=0,disease=0;std::uint32_t contacts=0;unsigned maxContacts=0;bool generatedShoot=false;bool wasCombat=false;unsigned episodes=0;unsigned playerInputs=0,resourceAttacks=0,runSuccesses=0,runFailures=0,episodeSuccesses=0;bool retainedItem=false,returning=false;unsigned returnActor=9;unsigned returnMoves=0;std::string emitted;std::array<std::optional<std::uint32_t>,6> escapedXp{};
   unsigned wake=0,reapply=0,allParty=0,poisonDerived=0,sleepSkipped=0,enemyProjectile=0,playerProjectile=0,blockedSaves=0;
   std::array<XeenCharacter,6> preceding;for(unsigned i=0;i<6;++i)preceding[i]=party->roster.at(kXeenCombatOwners[i]);
   std::uint64_t observedCombatRevision=0;std::set<unsigned> saveExclusions;
   const auto initialTreasure=*party->monsterTreasure;const std::set<XeenMonsterIdentity> initialAccounting=world->sessionState().accountedMonsters();
   auto previousTreasure=initialTreasure;auto receiptCharacters=preceding;std::uint64_t forfeitedTotal=0;std::set<std::uint64_t> finishObserved;
   const auto observeFinish=[&](const XeenCombatResult &r){if(m34 && r.operation==XeenCombatOperation::FinishDisengagement && finishObserved.insert(r.revision).second){if(r.exitCause==XeenCombatExitCause::DirectRun){check(r.forfeitedGold==previousTreasure.pendingGold&&r.forfeitedMask==previousTreasure.pendingMask,"DirectRun forfeiture differs from published pending sources");check(previousTreasure.weapons==party->monsterTreasure->weapons&&previousTreasure.armor==party->monsterTreasure->armor,"DirectRun changed stored item provenance or bytes");}forfeitedTotal+=r.forfeitedGold;std::cout<<"FORFEIT gold="<<r.forfeitedGold<<" sources="<<r.forfeitedMask<<" revision="<<r.revision<<'\n';}};
   const auto sameItem=[](const XeenItem &a,const XeenItem &b){return a.material==b.material&&a.id==b.id&&a.state==b.state&&a.frame==b.frame;};
   const auto describeResult=[&](const XeenCombatResult &r){
    std::cout<<"CONSEQUENCE status="<<unsigned(r.status)<<" target="<<(r.targetMonster?int(r.targetMonster->recordIndex):-1)<<" actorHP="<<r.actorHpBefore<<','<<r.actorHpAfter<<" targeted="<<unsigned(r.targetedMembers)<<" cause="<<unsigned(r.exitCause)<<" casualties="<<unsigned(r.casualties)<<" forfeited="<<r.forfeitedGold<<','<<r.forfeitedMask<<'\n';
    for(unsigned i=0;i<r.injuryCount;++i){const auto &v=r.injuries[i];std::cout<<"INJURY owner="<<unsigned(v.owner)<<" hp="<<v.beforeHp<<','<<v.afterHp<<" damage="<<v.amount<<" ac="<<v.beforeAc<<','<<v.afterAc<<" conditions=";for(auto c:v.conditions)std::cout<<unsigned(c)<<',';std::cout<<'\n';}
    for(unsigned i=0;i<r.xpCount;++i){const auto &v=r.xp[i];std::cout<<"XP owner="<<unsigned(v.owner)<<" before="<<v.before<<" after="<<v.after<<'\n';}
    for(unsigned i=0;i<r.armorCount;++i){const auto &v=r.armor[i];std::cout<<"ARMOR owner="<<unsigned(v.owner)<<" slot="<<unsigned(v.slot)<<" before="<<unsigned(v.before.material)<<','<<unsigned(v.before.id)<<','<<unsigned(v.before.state)<<','<<unsigned(v.before.frame)<<" after="<<unsigned(v.after.material)<<','<<unsigned(v.after.id)<<','<<unsigned(v.after.state)<<','<<unsigned(v.after.frame)<<'\n';}
    if(r.monsterDrop)std::cout<<"DROP source="<<unsigned(r.generatedItem.source)<<" outcome="<<unsigned(*r.monsterDrop)<<" armor="<<r.generatedArmor<<" bytes="<<unsigned(r.generatedItem.item.material)<<','<<unsigned(r.generatedItem.item.id)<<','<<unsigned(r.generatedItem.item.state)<<','<<unsigned(r.generatedItem.item.frame)<<'\n';
   };
   const bool summary=std::getenv("MMODERN_M33_SUMMARY")!=nullptr;
   auto verify=[&]{if(verifyPublication){verifyPublication();verifyPublication={};}};
   auto input=[&](const PlayerAction &action){if(m34)check(++playerInputs<=600,"M34 600 player-input search bound");const auto old=*handler.displayedInput();handler.withDisplayedInput(action,old);present();verify();
    if(old!=*handler.displayedInput()){XeenRestoreGuard retained(*world,*party,*position,*flags);const auto before=saveCalls;handler.withDisplayedInput(action,old);check(retained.current() && saveCalls==before,"Stale displayed input changed owners or entered save provider");present();}
   };
   while(++steps<10000) {
    observeFinish(flow->encounter()->combatResult());observeFinish(flow->encounter()->combatObservation());
    if(m34){const auto &current=*party->monsterTreasure;
     for(unsigned category=0;category<2;++category)for(const auto &old:category?previousTreasure.armor:previousTreasure.weapons){if(!old.item.id)continue;bool retained=false;for(const auto &item:category?current.armor:current.weapons)retained|=item==old;if(retained)continue;
      check(world->sessionState().journeyActivity()==XeenJourneyActivity::Reward,"Stored item vanished outside production receipt");
      bool delivered=false,recipientPossible=false;
      for(unsigned i=0;i<6;++i){const auto &before=category?receiptCharacters[i].armor:receiptCharacters[i].weapons;const auto &after=category?party->roster.at(kXeenCombatOwners[i]).armor:party->roster.at(kXeenCombatOwners[i]).weapons;unsigned nBefore=0,nAfter=0;for(const auto &item:before)nBefore+=sameItem(item,old.item);for(const auto &item:after)nAfter+=sameItem(item,old.item);
       recipientPossible|=receiptCharacters[i].canAct()&&!before.back().id;
       if(nAfter>nBefore){check(receiptCharacters[i].canAct()&&!before.back().id,"Receipt used ineligible or full recipient");delivered=true;std::cout<<"ITEM DELIVERY source="<<unsigned(old.source)<<" category="<<category<<" bytes="<<unsigned(old.item.material)<<','<<unsigned(old.item.id)<<','<<unsigned(old.item.state)<<','<<unsigned(old.item.frame)<<" owner="<<unsigned(kXeenCombatOwners[i])<<'\n';}}
      check(delivered||!recipientPossible,"Stored item lost despite eligible capacity");if(!delivered)std::cout<<"ITEM LOSS source="<<unsigned(old.source)<<" category="<<category<<" bytes="<<unsigned(old.item.material)<<','<<unsigned(old.item.id)<<','<<unsigned(old.item.state)<<','<<unsigned(old.item.frame)<<'\n';
     }
     previousTreasure=current;for(unsigned i=0;i<6;++i)receiptCharacters[i]=party->roster.at(kXeenCombatOwners[i]);
    }
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
     if(!wasCombat){if(returning){for(const auto &id:flow->encounter()->combat()->contacts())if(id&&(id->recordIndex==returnActor||(route[cursor]=='O'&&world->sessionState().actors().at(id->recordIndex).original.resourceId==6))){returnActor=id->recordIndex;++cursor;returning=false;returnMoves=0;break;}}++episodes;escapedXp={};episodeSuccesses=0;retainedItem=false;observedCombatRevision=0;std::cout<<"CONTACT episode="<<episodes<<" camera="<<position->x<<','<<position->y<<" facing="<<unsigned(position->direction)<<'\n';for(const auto &id:flow->encounter()->combat()->contacts())if(id){const auto &a=world->sessionState().actors().at(id->recordIndex);std::cout<<"CONTACT ACTOR id="<<id->recordIndex<<" hp="<<a.hp<<" position="<<a.x<<','<<a.y<<'\n';}}wasCombat=true;
     unsigned count=0;for(const auto &id:flow->encounter()->combat()->contacts())if(id){contacts|=1u<<id->recordIndex;++count;}maxContacts=std::max(maxContacts,count);
     const auto &observation=flow->encounter()->combatObservation();
     if(observation.revision!=observedCombatRevision && observation.revision==flow->encounter()->combat()->result().revision){observedCombatRevision=observation.revision;
      if(m34){
       std::cout<<"PUBLICATION episode="<<episodes<<" operation="<<unsigned(observation.operation)<<" revision="<<observation.revision<<" participants="<<unsigned(flow->encounter()->combat()->participants())<<" cause="<<unsigned(flow->encounter()->combat()->exitCause())<<" rng="<<world->sessionState().journeyRandom()->count<<'\n';
       describeResult(observation);if(observation.ranged)for(unsigned i=0;i<observation.ranged->count;++i){const auto &shot=observation.ranged->shots[i];std::cout<<"RANGED source="<<shot.source.recordIndex<<" position="<<shot.x<<','<<shot.y<<" direction="<<unsigned(shot.direction)<<" distance="<<shot.distance<<'\n';describeResult(shot.attack);}
       for(const auto &actor:world->sessionState().actors())std::cout<<"ACTOR STATE id="<<actor.id.recordIndex<<" position="<<actor.x<<','<<actor.y<<" hp="<<actor.hp<<" activated="<<actor.activated<<" lifecycle="<<unsigned(actor.lifecycle)<<" accounted="<<world->sessionState().accountedMonsters().count(actor.id)<<'\n';
       if(observation.operation==XeenCombatOperation::PlayerRun && observation.status==XeenCombatStatus::Advanced){if(observation.runSuccess){++runSuccesses;++episodeSuccesses;for(unsigned i=0;i<6;++i)if(kXeenCombatOwners[i]==*observation.actingOwner)escapedXp[i]=party->roster.combatInputs(kXeenCombatOwners[i])->experience;}else ++runFailures;std::cout<<"RUN owner="<<unsigned(*observation.actingOwner)<<" roll="<<observation.runRoll<<" success="<<observation.runSuccess<<" before="<<unsigned(observation.participantsBefore)<<" after="<<unsigned(observation.participantsAfter)<<'\n';}
       if(observation.status==XeenCombatStatus::Advanced){if(observation.operation==XeenCombatOperation::EnemyAttack)++resourceAttacks;if(observation.ranged)resourceAttacks+=observation.ranged->count;check(resourceAttacks<=128,"M34 128 resource-attack search bound");}
       if(observation.generatedItem.item.id){const auto &queue=observation.generatedArmor?party->monsterTreasure->armor:party->monsterTreasure->weapons;for(const auto &item:queue)if(item==observation.generatedItem)retainedItem=true;}
       if(observation.operation==XeenCombatOperation::FinishDisengagement)std::cout<<"DISENGAGEMENT cause="<<unsigned(observation.exitCause)<<" casualties="<<unsigned(observation.casualties)<<" forfeitedGold="<<observation.forfeitedGold<<'\n';
      }
      if(m34)for(unsigned i=0;i<6;++i)if(escapedXp[i])check(*escapedXp[i]==party->roster.combatInputs(kXeenCombatOwners[i])->experience,"Escaped member received later encounter XP");
      if(observation.actingMonster && (observation.actingMonster->recordIndex==14 || observation.actingMonster->recordIndex==15)) {if(observation.targetedMembers==63)++allParty;}}
     const auto phase=flow->encounter()->combat()->phase();
     if(phase==XeenCombatPhase::PlayerReady) {
      if(flow->encounter()->appearance().projectile){now+=100;if(idle())present();verify();continue;}
      check(party->roster.at(kXeenCombatOwners[flow->encounter()->combat()->participant()]).canAct(),"Sleeping/disabled member received ready turn");
      for(auto id:kXeenCombatOwners)if(party->roster.at(id).conditions[8])++sleepSkipped;
      const auto rows=flow->encounter()->combat()->contacts();
      unsigned selected=0;for(unsigned i=0;i<rows.size();++i)if(rows[i] && (!rows[selected] || rows[i]->recordIndex<rows[selected]->recordIndex))selected=i;
      if(!(rows[selected]==flow->encounter()->combat()->selectedTarget())) {input(SelectCombatTargetAction{selected});continue;}
      const auto &policy=policies[std::min<std::size_t>(episodes-1,policies.size()-1)];
      const bool run=policy=="run" || policy=="mixed" || ((policy=="first-run-attack"||(policy=="first-run-block"&&count>=2))&&!episodeSuccesses) || (policy=="item-run"&&retainedItem) || (policy=="item-first-run-block"&&retainedItem&&!episodeSuccesses) || (policy=="pending-first-run-block"&&party->monsterTreasure->pending()&&!episodeSuccesses);
      const bool groupPreparation=(policy=="item-run"||policy=="item-first-run-block")&&!retainedItem&&count==1&&world->sessionState().actors().at(rows[selected]->recordIndex).original.resourceId==6;
      const bool block=((policy=="first-run-block"||policy=="item-first-run-block"||policy=="pending-first-run-block")&&episodeSuccesses)||groupPreparation;
      std::cout<<"COMBAT INPUT episode="<<episodes<<" owner="<<unsigned(kXeenCombatOwners[flow->encounter()->combat()->participant()])<<" action="<<(run?"Run":block?"Block":"Attack")<<" target="<<rows[selected]->recordIndex<<'\n';
      if(run)input(RunAction{});else if(block)input(BlockAction{});else input(AttackAction{});continue;}
     if(phase==XeenCombatPhase::Defeat || phase==XeenCombatPhase::Failed || phase==XeenCombatPhase::SupportStopped)std::cout<<"TERMINAL phase="<<unsigned(phase)<<" failure="<<unsigned(flow->encounter()->combat()->result().failure)<<" route="<<cursor<<" poison="<<poison<<" sleep="<<sleep<<" disease="<<disease<<" contacts="<<contacts<<" episodes="<<episodes<<'\n';
     // Successful End is serviced by the production idle boundary.
     check(phase!=XeenCombatPhase::Defeat && phase!=XeenCombatPhase::Failed && phase!=XeenCombatPhase::SupportStopped,"Combat terminal witness");
    } else if(world->sessionState().journeyActivity()==XeenJourneyActivity::Reward || world->sessionState().journeyActivity()==XeenJourneyActivity::Event) {input(AcknowledgeAction{});continue;}
    if(flow->encounter()->journeyMutable()) {
     if(wasCombat){if(m34){const auto &r=flow->encounter()->combatResult();if(r.operation==XeenCombatOperation::FinishDisengagement)std::cout<<"DISENGAGEMENT cause="<<unsigned(r.exitCause)<<" casualties="<<unsigned(r.casualties)<<" forfeitedGold="<<r.forfeitedGold<<'\n';}std::cout<<"EPISODE QUIET route="<<cursor<<" episodes="<<episodes<<"\n";if(m34){std::cout<<"RETIRED camera="<<position->x<<','<<position->y<<" facing="<<unsigned(position->direction)<<" successes="<<runSuccesses<<" failures="<<runFailures<<'\n'<<flow->encounter()->journeyInspection();
      for(unsigned i=0;i<6;++i)if(escapedXp[i])check(*escapedXp[i]==party->roster.combatInputs(kXeenCombatOwners[i])->experience,"Retirement replayed XP to escaped owner");
      if(stopAfterExit&&episodeSuccesses)cursor=route.size();
     }}
     wasCombat=false;if(!summary)std::cout<<"CHECKPOINT "<<cursor<<'\n'<<flow->encounter()->journeyInspection();
     if(checkpointFile&&!checkpointObserved&&cursor==checkpointRoute){const auto expected=XeenSaveFile::read(checkpointFile);const auto actual=XeenSaveState::capture(original.resources.signature,*party,*position,*flags,*world);save_test::sameSnapshot(expected,actual);check(XeenSaveFormat::encode(expected)==XeenSaveFormat::encode(actual),"Uninterrupted checkpoint exact state before suffix");checkpointObserved=true;std::cout<<"PROCESS CHECKPOINT\n";}
     if(m34 && cursor<route.size() && (route[cursor]=='H'||route[cursor]=='O')) {
      if(!returning){returnActor=9;if(route[cursor]=='O'){bool found=false;for(const auto &candidate:world->sessionState().actors())if(candidate.original.resourceId==6 && candidate.lifecycle==XeenActorLifecycle::Present && candidate.hp>0){returnActor=candidate.id.recordIndex;found=true;break;}check(found,"No living original Orc remains for reactivation");}}
      returning=true;
      const auto &actor=world->sessionState().actors().at(returnActor);check(actor.lifecycle==XeenActorLifecycle::Present && actor.hp>0,"Return target must remain alive");
      const auto &map=world->map(23);const auto component=XeenMovement::component(map,9,11,{});
      std::array<int,256> distance;distance.fill(-1);std::deque<unsigned> queue;const auto goal=actor.y*16+actor.x;distance[goal]=0;queue.push_back(goal);
      const int dx[]={0,1,0,-1},dy[]={1,0,-1,0};
      while(!queue.empty()){const auto cell=queue.front();queue.pop_front();for(unsigned d=0;d<4;++d){const int x=int(cell%16)+dx[d],y=int(cell/16)+dy[d];if(x<0||x>=16||y<0||y>=16)continue;const auto next=y*16+x;if(component[next]&&distance[next]<0&&XeenMovement::localOutdoor(map,x,y,cell%16,cell/16,{})==XeenMovementResult::Moved){distance[next]=distance[cell]+1;queue.push_back(next);}}}
      const auto current=position->y*16+position->x;check(distance[current]>=0,"Live survivor reachable on admitted mainland");
      unsigned direction=unsigned(position->direction);if(distance[current]){bool found=false;for(unsigned d=0;d<4;++d){const auto x=position->x+dx[d],y=position->y+dy[d];if(x<0||x>=16||y<0||y>=16)continue;if(distance[y*16+x]==distance[current]-1&&XeenMovement::localOutdoor(map,position->x,position->y,x,y,{})==XeenMovementResult::Moved){direction=d;found=true;break;}}check(found,"Return path has a production step");}
      const unsigned turn=(direction+4-unsigned(position->direction))%4;const char key=turn?(turn==3?'L':'R'):'U';
      if(key=='U')check(++returnMoves<=64,"M34 64 charged return-move bound");emitted+=key;
      std::cout<<"RETURN INPUT "<<key<<" actor="<<returnActor<<" hp="<<actor.hp<<" target="<<actor.x<<','<<actor.y<<" camera="<<position->x<<','<<position->y<<'\n';
      input(key=='U'?NavigationAction::MoveForward:key=='L'?NavigationAction::TurnLeft:NavigationAction::TurnRight);continue;
     }
     if(cursor==route.size()) {
      check(!checkpointFile||checkpointObserved,"Uninterrupted checkpoint marker was reached");
      if(route=="LUUURUUUURU")check(automatic==1 && position->x==5 && position->y==9,"Production contract-4 automatic sign");
      if(resume && route.empty())check(automatic==0,"Restart must not replay automatic event");
      std::cout<<"AUTOMATIC EVENTS "<<automatic<<'\n';
      if(target) {const auto before=saveCalls;input(SaveGameAction{});check(saveCalls>before&&fs::exists(*target),"F9 entered production provider and wrote save");}
      check(fault.empty() || faultFired,"Requested artificial presentation fault reached");
      const auto snapshot=XeenSaveState::capture(original.resources.signature,*party,*position,*flags,*world);
      if(target){const auto written=XeenSaveFile::read(*target);save_test::sameSnapshot(snapshot,written);check(XeenSaveFormat::encode(snapshot)==XeenSaveFormat::encode(written),"F9 wrote current exact snapshot bytes");}
      save_test::sameSnapshot(snapshot,XeenSaveFormat::decode(XeenSaveFormat::encode(snapshot)));
      check(XeenSaveFormat::encode(XeenSaveFormat::decode(XeenSaveFormat::encode(snapshot)))==XeenSaveFormat::encode(snapshot),"Exact codec");
      if(const auto expected=std::getenv("MMODERN_M33_EXPECT")) {const auto control=XeenSaveFile::read(expected);save_test::sameSnapshot(control,snapshot);check(XeenSaveFormat::encode(control)==XeenSaveFormat::encode(snapshot),"Uninterrupted/resume exact bytes");std::cout<<"CONTROL FULL SEMANTIC STATE AND BYTES PASS\n";}
      if(const auto oracle=std::getenv("MMODERN_M33_ORACLE")) {
       const std::string name=oracle;const auto &j=*snapshot.journey;
       const bool wound=name=="wound",contact=name=="contact";check(wound||contact||name=="ranged","Named fixed oracle");
       check(j.schema==(m34?5:4) && j.contract==(m34?5:4) && snapshot.camera.x==(contact?7:8) && snapshot.camera.y==11 && snapshot.camera.direction==XeenDirection::West,"Fixed camera and version");
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
      if(m34){std::uint32_t newSources=0;for(const auto &a:world->sessionState().actors())if(a.original.resourceId==6&&world->sessionState().accountedMonsters().count(a.id)&&!initialAccounting.count(a.id))newSources|=1u<<a.id.recordIndex;unsigned kills=0;for(unsigned i=0;i<19;++i)kills+=bool(newSources&(1u<<i));
       const std::uint64_t produced=std::uint64_t(initialTreasure.gold)+initialTreasure.pendingGold+10u*kills;
       const auto &finalTreasure=*party->monsterTreasure;check(produced==std::uint64_t(finalTreasure.gold)+finalTreasure.pendingGold+forfeitedTotal,"New Orc gold and forfeiture conservation");
       std::cout<<"GOLD CONSERVATION initial="<<initialTreasure.gold<<" initialPending="<<initialTreasure.pendingGold<<" newSources="<<newSources<<" produced="<<10u*kills<<" forfeited="<<forfeitedTotal<<" final="<<finalTreasure.gold<<" pending="<<finalTreasure.pendingGold<<'\n';
       const auto &a=world->sessionState().actors().at(9);const auto &t=*party->monsterTreasure;unsigned items=0;for(const auto &v:t.weapons)items+=bool(v.item.id);for(const auto &v:t.armor)items+=bool(v.item.id);unsigned dead=0;for(unsigned i=0;i<6;++i)if(party->roster.at(kXeenCombatOwners[i]).conditions[unsigned(XeenCondition::Dead)])dead|=1u<<i;
       std::cout<<"M34 STATE camera="<<position->x<<','<<position->y<<" facing="<<unsigned(position->direction)<<" actor9="<<a.x<<','<<a.y<<','<<a.hp<<','<<unsigned(a.lifecycle)<<" gold="<<t.gold<<" pendingGold="<<t.pendingGold<<" pendingMask="<<t.pendingMask<<" storedItems="<<items<<" dead="<<dead<<'\n';}
      std::cout<<"STATE minute="<<party->encounterContext->minutes<<" pendingGold="<<party->monsterTreasure->pendingGold<<'\n';
      std::cout<<"LIVING POISON "<<livingPoison<<'\n';
      std::cout<<"CONDITIONS wake="<<wake<<" reapply="<<reapply<<" allParty="<<allParty<<" poisonDerived="<<poisonDerived<<" sleepSkipped="<<sleepSkipped<<" enemyProjectile="<<enemyProjectile<<" playerProjectile="<<playerProjectile<<" blockedSaves="<<blockedSaves<<'\n';
      std::cout<<"OBSERVATIONS poison="<<poison<<" sleep="<<sleep<<" disease="<<disease<<" contacts="<<contacts<<" grouped="<<maxContacts<<" generatedShoot="<<generatedShoot<<" episodes="<<episodes<<'\n';
      if(m34)std::cout<<"M34 OBSERVATIONS successes="<<runSuccesses<<" failures="<<runFailures<<" inputs="<<playerInputs<<" attacks="<<resourceAttacks<<" return="<<emitted<<'\n';
      std::cout<<(m34?"M34":"M33")<<" GAMEPLAY PASS route="<<route<<" count="<<world->sessionState().journeyRandom()->count<<" gold="<<party->monsterTreasure->gold<<'\n';return true;
     }
     const auto key=route[cursor++];
     if(m34)std::cout<<"NAV INPUT "<<key<<" camera="<<position->x<<','<<position->y<<" facing="<<unsigned(position->direction)<<'\n';
     if(key=='Q'){input(NavigationAction::MoveForward);input(ShootAction{});}
     else if(key=='U')input(NavigationAction::MoveForward);
     else if(key=='D')input(NavigationAction::MoveBackward);
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
  if(!resume)contract=m34?5:4; // Explicit retained legacy contracts, independent of the M35 CLI default.
  check(resume || contract==(m34?5:4),"Fresh production CLI must admit selected witness contract");
  return realPlay(app,services,camera,target,resume,entry,seed,contract);
 } catch(const std::exception &e) {std::cerr<<"M33 witness: "<<e.what()<<'\n';return 8;}
}
