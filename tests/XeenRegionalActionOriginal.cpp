// Original resources with explicit artificial durable-state controls. These
// isolate regional action settlement; they are not production route evidence.
#include "XeenCombatGameplayTestSupport.h"
#include "XeenMonsterTestAccess.h"
#include "formats/xeen/XeenCharacterFormat.h"
#include "formats/xeen/XeenQuestFlagFormat.h"
#include "games/xeen/XeenCharacterRules.h"
#include "games/xeen/XeenStateEquality.h"
#include "games/xeen/XeenItemRewards.h"
#include <chrono>
#include <iostream>

using namespace combat_gameplay_test;
namespace fs=std::filesystem;
namespace {
struct QuietGameplay {
 std::ostringstream output;
 std::streambuf *previous=std::cout.rdbuf(output.rdbuf());
 ~QuietGameplay() {std::cout.rdbuf(previous);}
};
XeenGameplayServices services(Harness &h) {
 auto s=h.services();
 s.resources.regionalManifest=[&](const auto &m,const auto &o,const auto &e,const auto &mon) {
  xeenValidateRegionalManifest(m,o,e,mon,h.assets->readInitialResource("maze0023.dat"),
   h.assets->readInitialResource("maze0023.mob"),h.assets->readInitialResource("maze0023.evt"));
 };

 s.resources.loadInitialBankBalances=[&]{return XeenCharacterFormat::parseBankBalances(h.assets->readInitialResource("maze.pty"));};
 s.resources.loadInitialPurse=[&]{return XeenCharacterFormat::parseMonsterPurse(h.assets->readInitialResource("maze.pty"));};
 s.resources.loadInitialWorldFlags=[&]{return XeenQuestFlagFormat::parseWorldFlags(h.assets->readInitialResource("maze.pty"));};
 s.resources.loadRegionalText=[&](XeenMapIdentity id) {
  return XeenEventTextLoader([&](const std::string &name)->std::optional<Bytes> {
   if(!h.assets->hasArchiveResource(name))return {};return h.assets->readArchiveResource(name);
  }).load(id);
 };
 s.resources.loadLearnedSpellNames=[&]{return XeenLearnedSpellNames::parse(*h.assets->readLearnedSpellNamesFromDarkArchive());};
 s.texts=s.resources.loadRegionalText;
 s.composeEncounter=[](auto &,const auto &,const auto &,auto,auto) {
  XeenEventFlow::Composition c;c.frame.width=320;c.frame.height=200;c.frame.pixels.resize(64000);return c;
 };
 return s;
}
XeenSaveSnapshot baseline(const fs::path &game) {
 Harness h(game);auto s=services(h);XeenSaveSnapshot saved;
 s.show=[&](const auto &,const auto &handler,const auto &,const auto &,const auto &) {
  handler.framePresented(h.flow->frame().presentation());
  check(h.flow->canSave(),"Initial content-14 source must be Quiet");
  saved=XeenSaveState::capture(h.signature,*h.party,*h.camera,*h.flags,*h.world);return true;
 };
 check(Application(s.program).playGameplay(s,{}, {},false,XeenEncounterEntry::Journey,56)==0,"Original content-14 startup");
 // Eliminate incidental outdoor battles in these action-only controls. The
 // twelve mainland Orc identities remain canonical defeated/accounted actors.
 for(unsigned i=0;i<12;++i) {
  auto &a=saved.map(23).monsters->slots[i];a.x=a.y=-128;
  a.lifecycle=XeenActorLifecycle::Defeated;a.accounted=true;
 }
 saved.camera={23,10,12,XeenDirection::North};
 saved.journey->context->minutes=500;saved.journey->context->ctr24=0;
 saved.journey->context->newDay=false;
 const auto mob=s.objects(28);const auto mon=s.resources.loadMonsterStatistics();
 saved.map(28).monsters.emplace(XeenSaveMonsterTable{46,{}});
 for(unsigned i=0;i<46;++i) {
  const auto &original=mob.entities.monsters[i];
  saved.map(28).monsters->slots.push_back({original.x,original.y,XeenActorLifecycle::Present,XeenActorStatus::Physical,false,-1});
 }
 static_cast<void>(mon);
 auto &slime=saved.map(28).monsters->slots.at(35);
 slime.x=slime.y=-128;slime.lifecycle=XeenActorLifecycle::Defeated;slime.accounted=true;
 for(auto owner:saved.activeRosterIds) {
  saved.characters[owner].conditions.fill(0);
  saved.characters[owner].currentHp=10;saved.characters[owner].currentSp=20;
 }
 // Original cleric owner 1 learns only the two supported spells in this
 // controlled book, so UI row selection has an independent fixed order.
 auto &caster=saved.characters[1];caster.learnedSpells->fill(0);
 caster.learnedSpells->at(1)=1;caster.learnedSpells->at(14)=1;
 return saved;
}
enum class Action {Antidote,AntidoteCancel,FirstAid,FirstAidCancel,Awaken,Boundary};
enum class Arrival {EntryContact,EntryClear,EntryVisible,ExitContact,TreasureClear,TreasureBlocked};
void arrival(const fs::path &game,const fs::path &save,const XeenSaveSnapshot &initial,Arrival mode) {
 std::cerr<<"Arrival control "<<static_cast<unsigned>(mode)<<'\n';
 auto fixture=initial;
 const bool exit=mode==Arrival::ExitContact;
 const bool contact=mode==Arrival::EntryContact || exit;
 const bool treasure=mode==Arrival::TreasureClear || mode==Arrival::TreasureBlocked;
 fixture.camera=exit?XeenCamera{28,15,0,XeenDirection::South}:XeenCamera{23,10,13,XeenDirection::North};
 if(mode==Arrival::EntryContact || mode==Arrival::EntryVisible || mode==Arrival::TreasureBlocked) {
  auto &actor=fixture.map(28).monsters->slots.at(35);
  actor.x=15;actor.y=mode==Arrival::EntryContact?0:1;
  actor.lifecycle=XeenActorLifecycle::Present;actor.accounted=false;
 }
 if(exit || treasure) {
  // The original mainland Orc slot 11 at (9,15) is visible diagonally from
  // the entrance; retain that legal terrain as the source treasure blocker.
  auto &actor=fixture.map(23).monsters->slots[exit?0:11];
  actor.x=exit?10:9;actor.y=exit?12:15;
  actor.lifecycle=XeenActorLifecycle::Present;actor.accounted=false;
 }
 // Monster 1's gold and dropped weapon, staged in the running program after
 // the load (an entry clears pending currency and readiness); the other
 // arrivals carry a dormant item that every transition must retain.
 XeenTreasureStaging liveTreasure;liveTreasure.weapons[0]={0,30,0,0};
 if(treasure) {liveTreasure.gold=10;liveTreasure.ready=true;}
 XeenSaveFile::write(save,fixture);
 Harness h(game);auto s=services(h);
 const auto compose=s.composeEncounter;
 unsigned destinationCompositions=0;
 std::function<void()> duringComposition;
 s.composeEncounter=[&](auto &w,const auto &p,const auto &c,auto phase,auto appearance) {
  if(h.world && &w!=h.world) {
   ++destinationCompositions;
   check(!h.flow->canSave() && !XeenSaveState::canCapture(*h.party,*h.camera,*h.world),
    "Detached destination composition exposed a saveable source");
   check(c.mapId==XeenMapIdentity(exit?23:28),"Composition used wrong destination");
   if(duringComposition)duringComposition();
  }
  return compose(w,p,c,phase,appearance);
 };
 s.show=[&](const auto &,const auto &handler,const auto &,const auto &,const auto &) {
  const auto present=[&]{const auto frame=h.flow->frame().presentation();handler.framePresented(frame);handler.completeInputHandoff(frame);};
  const auto input=[&](PlayerAction action,bool show=true) {
   present();handler.beginCycle(++h.cycle);
   check(bool(handler.displayedInput()),"Arrival displayed input");
   handler.withDisplayedInput(action,*handler.displayedInput());if(show)present();
  };
  const auto denied=[&] {
   check(!h.flow->canSave(),"Arrival work exposed Quiet");
   const auto before=h.saves;const auto token=handler.displayedInput();
   check(bool(token),"Arrival F9 input token absent");
   handler.withDisplayedInput(SaveGameAction{},*token);
   check(h.saves==before,"F9 saved mandatory arrival work");
  };
  XeenMonsterTestAccess::stageTreasureLive(*h.flow,*h.world,liveTreasure);
  present();check(h.flow->canSave(),"Arrival source fixture is not Quiet");
  const auto rng=*h.world->sessionState().journeyRandom();
  const auto context=*h.party->encounterContext;
  auto expectedContext=context;expectedContext.minutes+=exit?10:1;expectedContext.ctr24=(context.ctr24+1)%24;
  const XeenCamera destination=exit?XeenCamera{23,10,12,XeenDirection::South}:XeenCamera{28,15,0,XeenDirection::North};
  auto destinationWorld=h.world->transitionCandidate();
  destinationWorld->loadEventDestination(h.camera->mapId,destination.mapId);
  XeenConsequenceCharacters characters;XeenConsequenceInputs inputs;
  const auto owners=h.party->party.activeOrder();
  for(unsigned n=0;n<6;++n) {characters[n]=h.party->roster.at(owners[n]);inputs[n]=*h.party->roster.combatInputs(owners[n]);}
  const std::vector<XeenActor> loaded=destinationWorld->sessionState().regionalActors(destination.mapId);
  std::optional<XeenRegionalOpportunityCandidate> opportunity;
  if(exit)opportunity.emplace(destinationWorld->map(destination.mapId),loaded,destination,characters,inputs,context.year,0x3f);
  else opportunity.emplace(*destinationWorld,loaded,destination,characters,inputs,context.year,0x3f);
  XeenCombatRandom opportunityRandom(rng);
  for(;;){XeenConsequenceDraw draw{opportunityRandom,64,{}};if(opportunity->service(draw))break;}
  check(opportunityRandom.continuation()==rng,"Arrival control unexpectedly has ranged randomness");
  const std::vector<XeenActor> source=h.world->sessionState().regionalActors(exit?28:23);
  const auto gold=h.party->purse->gold;
  duringComposition=denied;
  input(InteractionAction{});denied();
  input(YesAction{},false);
  check(destinationCompositions==1,"Original transition did not compose one detached destination");
  check(h.camera->mapId==XeenMapIdentity(exit?23:28),"Original Event did not publish destination");
  denied(); // The returned destination frame has not been presented yet.
  check(*h.party->encounterContext==expectedContext && *h.world->sessionState().journeyRandom()==rng,
   "Completed transition did not charge exactly one indoor/ten outdoor minutes with one ctr24 opportunity and unchanged RNG");
  check(xeen_state::sameCamera(*h.camera,destination),"Original transition changed destination coordinates/facing");
  sameActors(opportunity->actors,h.world->sessionState().regionalActors(destination.mapId));
  auto expectedCharacters=opportunity->characters;
  if(mode==Arrival::TreasureClear) {
   // The first visible transfer has not reached the Found-stage inventory sort.
   std::array<XeenCharacter *,6> recipients{};for(unsigned n=0;n<6;++n)recipients[n]=&expectedCharacters[n];
   auto item=liveTreasure.weapons[0];
   const auto recipient=xeenDeliverRewardItem(recipients.data(),recipients.size(),item,XeenRewardCategory::Weapons,{},false);
   check(recipient==0 && expectedCharacters[0].weapons[8].id==30,"Controlled treasure did not use owner0 slot8 before sorting");
  }
  for(unsigned n=0;n<6;++n)check(xeen_state::sameCharacter(h.party->roster.at(owners[n]),expectedCharacters[n]),
   "Completed transition changed characters outside its destination opportunity/item publication");
  if(!exit)sameActors(source,h.world->sessionState().monsters(23));
  if(contact) {
   check(h.flow->encounter()->combat() &&
    h.flow->encounter()->combat()->phase()!=XeenCombatPhase::Failed &&
    h.flow->encounter()->combat()->phase()!=XeenCombatPhase::SupportStopped,
    "Destination contact was not attached before presentation");
   present();denied();present();denied();
   check(h.flow->encounter()->combat() && h.world->treasureStaging().value()==liveTreasure,"Presentation erased mandatory contact or pending treasure");
  } else if(mode==Arrival::TreasureClear) {
   check(h.world->sessionState().journeyActivity()==XeenJourneyActivity::Reward,
    "Unobstructed city arrival did not begin source-qualified mainland delivery");
   check(h.world->treasureStaging().value()==liveTreasure &&
    h.party->purse->gold==gold,"Arrival prematurely credited pending gold");
   present();denied();present();denied();
   // One item has a transfer/blank stage, its found stage, then the final receipt.
   // They are distinct publications rather than pages of an atomic batch.
   for(unsigned stage=0;stage<2;++stage) {
    h.now+=110;
    input(AcknowledgeAction{});
    check(!h.flow->canSave() && h.party->purse->gold==gold && h.world->treasureStaging().value().gold==10,
     "Arrival item/warning acknowledgment prematurely credits currency");
   }
   h.now+=110;
   input(AcknowledgeAction{});
   check(h.flow->canSave() && h.world->treasureStaging().value()==XeenTreasureStaging{} && h.party->purse->gold==gold+10,
    "Receipt acknowledgment failed to settle source-qualified gold");
   const auto deliveredCharacters=[](const XeenTreasureStaging &staged,const XeenConsequenceCharacters &c) {
    XeenPartyState party;std::vector<std::uint8_t> ids;
    for(const auto &v:c){party.roster.at(v.rosterId)=v;ids.push_back(v.rosterId);}
    party.party=XeenParty::fromRosterIds(ids);xeenDeliverTreasure(staged,party);
    XeenConsequenceCharacters result;for(unsigned i=0;i<6;++i)result[i]=party.roster.at(c[i].rosterId);return result;
   };
   const auto delivered=deliveredCharacters(liveTreasure,opportunity->characters);
   for(unsigned n=0;n<6;++n)check(xeen_state::sameCharacter(h.party->roster.at(owners[n]),delivered[n]),
    "Final receipt did not retain exactly the sorted single-item transfer");
   input(AcknowledgeAction{});
   check(h.party->purse->gold==gold+10,"Repeated receipt acknowledgment duplicated gold");
  } else {
   present();check(h.flow->canSave(),"Non-contact arrival remained unavailable after presentation");
   check(h.world->treasureStaging().value()==liveTreasure,
    "Transition did not retain pending or dormant treasure exactly");
   if(mode==Arrival::EntryVisible || mode==Arrival::TreasureBlocked) {
    const auto &actor=h.world->sessionState().regionalActors(28)[35];
    check(actor.x==15 && actor.y==1 && actor.activated && actor.hp==2,
     "Destination opportunity changed the original visible actor control");
   }
  }
  if(!contact) {input(SaveGameAction{});check(h.saves==3,"Settled arrival F9 failed");}
  duringComposition={};return true;
 };
 check(Application(s.program).playGameplay(s,{},save,true)==0,"Original transition arrival control failed");
}
void run(const fs::path &game,const fs::path &save,const XeenSaveSnapshot &initial,
  bool indoor,Action action,unsigned minute=500) {
 auto fixture=initial;
 fixture.camera=indoor?XeenCamera{28,16,2,XeenDirection::North}:XeenCamera{23,10,12,XeenDirection::North};
 fixture.journey->context->minutes=minute;
 fixture.characters[0].miscellaneous={};fixture.characters[0].miscellaneous[0]={10,37,1,0};
 if(action==Action::Antidote || action==Action::AntidoteCancel)fixture.characters[18].conditions[3]=1;
 if(action==Action::Awaken)fixture.characters[18].conditions[8]=1;
 fixture.characters[18].currentHp=1;
 XeenSaveFile::write(save,fixture);
 Harness h(game);auto s=services(h);
 s.show=[&](const auto &,const auto &handler,const auto &,const auto &idle,const auto &) {
  const auto present=[&]{const auto frame=h.flow->frame().presentation();handler.framePresented(frame);handler.completeInputHandoff(frame);};
  const auto input=[&](PlayerAction action) {
   present();handler.beginCycle(++h.cycle);
   check(bool(handler.displayedInput()),"Regional action displayed input");
   handler.withDisplayedInput(action,*handler.displayedInput());present();
  };
  const auto settle=[&] {
   for(unsigned n=0;n<1000 && !h.flow->canSave();++n) {
    check(!h.flow->encounter()->combat(),"Action control unexpectedly entered combat");
    h.now+=100;handler.beginCycle(++h.cycle);idle();present();
   }
   check(h.flow->canSave(),"Regional action did not settle to saveable Quiet");
  };
  present();check(h.flow->canSave(),"Restored action fixture is not Quiet");
  const auto rng=*h.world->sessionState().journeyRandom();
  const std::vector<XeenActor> mainland=h.world->sessionState().monsters(23);
  const std::vector<XeenActor> city=h.world->sessionState().regionalActors(28);
  const auto deniedSave=[&] {
   check(!h.flow->canSave(),"Action exposed intermediate saveability");
   const auto n=h.saves;input(SaveGameAction{});check(n==h.saves,"F9 saved unfinished action");
  };
  if(action==Action::Antidote || action==Action::AntidoteCancel) {
   input(InspectInventoryAction{});input(DialogKeyAction{'m'});
   input(SelectInventorySlotAction{0});input(UseItemAction{});
   check(h.flow->inventorySelection().mode==XeenInventoryMode::UseTarget,"Antidote did not reach debited target selector");
   deniedSave();
   if(action==Action::Antidote)input(SelectMemberAction{1});else input(CancelInteractionAction{});
   check(h.world->scenePresentation().portraits[18].spellFrame==(action==Action::Antidote?0u:4u),"Antidote explicit recipient/cancel portrait effect");
   settle();
   check(h.party->encounterContext->minutes==minute,"Antidote charged time");
   check(h.party->roster.at(18).conditions[3]==(action==Action::Antidote?0:1),"Antidote cure/cancel consequence");
   check(h.party->roster.at(0).miscellaneous[0].id==0,"Antidote charge not consumed exactly once");
  } else {
   input(CastSpellAction{});input(SelectMemberAction{4});
   input(DialogKeyAction{'n'});input(DialogKeyAction{unsigned(action==Action::Awaken?'1':'2')});
   input(DialogKeyAction{'s'});input(DialogKeyAction{'c'});
   {
    check(h.flow->encounter()->castingCommitted(),"Supported regional casting did not debit");
    deniedSave();
    if(action!=Action::Awaken) {
     if(action==Action::FirstAidCancel)input(CancelInteractionAction{});else input(SelectMemberAction{1});
     check(h.world->scenePresentation().portraits[18].spellFrame==(action==Action::FirstAidCancel?4u:0u),"First Aid explicit recipient/cancel portrait effect");
    }
    settle();
    check(h.party->encounterContext->minutes==minute+(indoor?1:10),"Casting regional minute charge");
    check(h.party->encounterContext->ctr24==0,"Casting incremented movement opportunity counter");
    check(h.party->roster.at(1).currentSp==(action==Action::FirstAidCancel?20:19),"Casting debit/refund");
    if(action==Action::FirstAid || action==Action::Boundary)
     check(h.party->roster.at(18).currentHp==7,"First Aid independent six-HP effect");
    if(action==Action::FirstAidCancel)check(h.party->roster.at(18).currentHp==1,"Cancelled First Aid healed target");
    if(action==Action::Awaken)check(h.party->roster.at(18).conditions[8]==0,"Awaken retained Sleep");
   }
  }
  check(*h.world->sessionState().journeyRandom()==rng,"No-threat action changed RNG");
  sameActors(mainland,h.world->sessionState().monsters(23));sameActors(city,h.world->sessionState().regionalActors(28));
  check(XeenSaveState::canCapture(*h.party,*h.camera,*h.world),"Completed regional action cannot capture");
  input(SaveGameAction{});check(h.saves==3,"Completed regional action did not perform F9 capture/preflight/write");
  check(XeenSaveFile::read(save).journey->context==h.party->encounterContext,"F9 preserved wrong calendar");
  return true;
 };
 check(Application(s.program).playGameplay(s,{},save,true)==0,"Regional action Application failed");
}
}
int main(int argc,char **argv) {
 try {
  check(argc==2 || (argc==3 && std::string(argv[2])=="--arrivals"),
   "usage: mmodern_regional_action_original <installation> [--arrivals]");
  const fs::path game=argv[1];
  const auto save=fs::temp_directory_path()/("mmodern-m37-actions-"+
   std::to_string(std::chrono::steady_clock::now().time_since_epoch().count())+".mmsave");
  {
  QuietGameplay quiet;
  const auto initial=baseline(game);
  if(argc==2)for(bool indoor:{false,true}) {
   for(auto action:{Action::Antidote,Action::AntidoteCancel,Action::FirstAid,Action::FirstAidCancel,Action::Awaken})
    run(game,save,initial,indoor,action);
   run(game,save,initial,indoor,Action::Boundary,1255);
   run(game,save,initial,indoor,Action::Boundary,1259);
  }
  for(auto mode:{Arrival::EntryContact,Arrival::EntryClear,Arrival::EntryVisible,Arrival::ExitContact,
    Arrival::TreasureClear,Arrival::TreasureBlocked})arrival(game,save,initial,mode);
  }
  fs::remove(save);
  std::cout<<"M37 regional actions and original transition arrival/contact/treasure/F9 controls passed\n";
  return 0;
 } catch(const std::exception &e) {std::cerr<<e.what()<<'\n';return 1;}
}
