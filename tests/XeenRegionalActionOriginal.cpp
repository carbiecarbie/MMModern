// Original resources with explicit artificial durable-state controls. These
// isolate regional action settlement; they are not production route evidence.
#include "XeenCombatGameplayTestSupport.h"
#include "formats/xeen/XeenCharacterFormat.h"
#include "formats/xeen/XeenQuestFlagFormat.h"
#include "games/xeen/XeenCharacterRules.h"
#include "games/xeen/XeenStateEquality.h"
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
 s.resources.vertigoManifest=[&](auto &w,const auto &evt,const auto &mon) {
  xeenValidateVertigoManifest(w,evt,mon,[&](const std::string &name) {
   return name.rfind("aaze",0)==0?h.assets->readArchiveResource(name):h.assets->readInitialResource(name);
  });
 };
 s.resources.loadInitialPurse=[&]{return XeenCharacterFormat::parseMonsterPurse(h.assets->readInitialResource("maze.pty"));};
 s.resources.loadInitialRegionalRecovery=[&]{return XeenQuestFlagFormat::parseRegionalRecovery(h.assets->readInitialResource("maze.pty"));};
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
  check(h.flow->canSave(),"Initial content-8 source must be Quiet");
  saved=XeenSaveState::capture(h.signature,*h.party,*h.camera,*h.flags,*h.world);return true;
 };
 check(Application().playGameplay(s,{}, {},false,XeenEncounterEntry::Journey,56,8)==0,"Original content-8 startup");
 // Eliminate incidental outdoor battles in these action-only controls. The
 // twelve mainland Orc identities remain canonical defeated/accounted actors.
 for(unsigned i=0;i<12;++i) {
  auto &a=saved.journey->actors[i];a.x=a.y=-128;a.hp=0;a.activated=false;
  a.lifecycle=XeenActorLifecycle::Defeated;a.accounted=true;
 }
 saved.camera={23,10,12,XeenDirection::North};
 saved.journey->context->minutes=500;saved.journey->context->ctr24=0;
 saved.journey->context->newDay=false;
 saved.journey->treasure->pendingGold=0;saved.journey->treasure->pendingMask=0;
 const auto mob=s.objects(28);const auto mon=s.resources.loadMonsterStatistics();
 saved.journey->vertigoActors.emplace();
 for(unsigned i=0;i<46;++i) {
  const auto &original=mob.entities.monsters[i];
  saved.journey->vertigoActors->push_back({{28,i},original.x,original.y,
   mon.at(original.resourceId).baseHp(),false,XeenActorLifecycle::Present,XeenActorStatus::Physical,false});
 }
 auto &slime=saved.journey->vertigoActors->at(35);
 slime.x=slime.y=-128;slime.hp=0;slime.activated=false;slime.lifecycle=XeenActorLifecycle::Defeated;slime.accounted=true;
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
  auto &actor=fixture.journey->vertigoActors->at(35);
  actor.x=15;actor.y=mode==Arrival::EntryContact?0:1;actor.hp=2;
  actor.lifecycle=XeenActorLifecycle::Present;actor.accounted=false;actor.activated=true;
 }
 if(exit || treasure) {
  // The original mainland Orc slot 11 at (9,15) is visible diagonally from
  // the entrance; retain that legal terrain as the source treasure blocker.
  auto &actor=fixture.journey->actors[exit?0:11];
  actor.x=exit?10:9;actor.y=exit?12:15;actor.hp=10;
  actor.lifecycle=XeenActorLifecycle::Present;actor.accounted=false;actor.activated=true;
 }
 if(treasure) {
  auto &reward=*fixture.journey->treasure;
  reward.pendingMask=1u<<1;reward.pendingGold=10;
  reward.weapons[0]={1,{0,30,0,0}};
 }
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
  const auto present=[&]{handler.framePresented(h.flow->frame().presentation());};
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
  present();check(h.flow->canSave(),"Arrival source fixture is not Quiet");
  const auto rng=*h.world->sessionState().journeyRandom();
  const auto context=*h.party->encounterContext;
  const std::vector<XeenActor> source=h.world->sessionState().regionalActors(exit?28:23);
  const auto gold=h.party->monsterTreasure->gold;
  duringComposition=denied;
  input(InteractionAction{});denied();
  input(YesAction{},false);
  check(destinationCompositions==1,"Original transition did not compose one detached destination");
  check(h.camera->mapId==XeenMapIdentity(exit?23:28),"Original Event did not publish destination");
  denied(); // The returned destination frame has not been presented yet.
  check(*h.party->encounterContext==context && *h.world->sessionState().journeyRandom()==rng,
   "Transition/arrival changed time or RNG");
  if(!exit)sameActors(source,h.world->sessionState().actors());
  if(contact) {
   check(h.flow->encounter()->combat() &&
    h.flow->encounter()->combat()->phase()!=XeenCombatPhase::Failed &&
    h.flow->encounter()->combat()->phase()!=XeenCombatPhase::SupportStopped,
    "Destination contact was not attached before presentation");
   present();denied();present();denied();
   check(h.flow->encounter()->combat(),"Presentation erased mandatory contact");
  } else if(mode==Arrival::TreasureClear) {
   check(h.world->sessionState().journeyActivity()==XeenJourneyActivity::Reward,
    "Unobstructed city arrival did not begin source-qualified mainland delivery");
   check(h.party->monsterTreasure->pendingMask==2 && h.party->monsterTreasure->pendingGold==10 &&
    h.party->monsterTreasure->gold==gold,"Arrival prematurely credited pending gold");
   present();denied();present();denied();
   input(AcknowledgeAction{});
   check(h.flow->canSave() && h.party->monsterTreasure->pendingMask==0 &&
    h.party->monsterTreasure->pendingGold==0 && h.party->monsterTreasure->gold==gold+10,
    "Receipt acknowledgment failed to settle source-qualified gold");
   input(AcknowledgeAction{});
   check(h.party->monsterTreasure->gold==gold+10,"Repeated receipt acknowledgment duplicated gold");
  } else {
   present();check(h.flow->canSave(),"Non-contact arrival remained unavailable after presentation");
   if(mode==Arrival::TreasureBlocked)
    check(*h.party->monsterTreasure==*fixture.journey->treasure,
     "Selected live city obstruction did not preserve mainland pending provenance");
   if(mode==Arrival::EntryVisible || mode==Arrival::TreasureBlocked) {
    const auto &actor=h.world->sessionState().regionalActors(28)[35];
    check(actor.x==15 && actor.y==1 && actor.activated && actor.hp==2,
     "Arrival classification performed synthetic actor movement");
   }
  }
  if(!contact) {input(SaveGameAction{});check(h.saves==3,"Settled arrival F9 failed");}
  duringComposition={};return true;
 };
 check(Application().playGameplay(s,{},save,true)==0,"Original transition arrival control failed");
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
  const auto present=[&]{handler.framePresented(h.flow->frame().presentation());};
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
  const std::vector<XeenActor> mainland=h.world->sessionState().actors();
  const std::vector<XeenActor> city=h.world->sessionState().regionalActors(28);
  const auto deniedSave=[&] {
   check(!h.flow->canSave(),"Action exposed intermediate saveability");
   const auto n=h.saves;input(SaveGameAction{});check(n==h.saves,"F9 saved unfinished action");
  };
  if(action==Action::Antidote || action==Action::AntidoteCancel) {
   input(InspectInventoryAction{});for(unsigned i=0;i<3;++i)input(NavigationAction::TurnRight);
   input(SelectInventorySlotAction{0});input(UseItemAction{});input(AcknowledgeAction{});
   check(h.flow->inventorySelection().mode==XeenInventoryMode::UseTarget,"Antidote did not reach debited target selector");
   deniedSave();
   if(action==Action::Antidote)input(SelectMemberAction{1});else input(CancelInteractionAction{});
   settle();
   check(h.party->encounterContext->minutes==minute,"Antidote charged time");
   check(h.party->roster.at(18).conditions[3]==(action==Action::Antidote?0:1),"Antidote cure/cancel consequence");
   check(h.party->roster.at(0).miscellaneous[0].id==0,"Antidote charge not consumed exactly once");
  } else {
   input(CastSpellAction{});input(SelectMemberAction{4});
   if(action!=Action::Awaken)input(NavigationAction::MoveBackward);
   input(AcknowledgeAction{});input(AcknowledgeAction{});
   const bool refused=action==Action::Boundary && (minute==1259 || !indoor);
   if(refused) {
    check(!h.flow->encounter()->castingCommitted() && h.party->roster.at(1).currentSp==20,
     "Unsupported regional time boundary debited SP");
    check(h.party->encounterContext->minutes==minute,"Refused cast changed time");
    for(unsigned n=0;n<4 && !h.flow->canSave();++n)input(CancelInteractionAction{});
    settle();
   } else {
    check(h.flow->encounter()->castingCommitted(),"Supported regional casting did not debit");
    deniedSave();
    if(action!=Action::Awaken) {
     if(action==Action::FirstAidCancel)input(CancelInteractionAction{});else input(SelectMemberAction{1});
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
  sameActors(mainland,h.world->sessionState().actors());sameActors(city,h.world->sessionState().regionalActors(28));
  check(XeenSaveState::canCapture(*h.party,*h.camera,*h.world),"Completed regional action cannot capture");
  input(SaveGameAction{});check(h.saves==3,"Completed regional action did not perform F9 capture/preflight/write");
  check(XeenSaveFile::read(save).journey->context==h.party->encounterContext,"F9 preserved wrong calendar");
  return true;
 };
 check(Application().playGameplay(s,{},save,true)==0,"Regional action Application failed");
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
