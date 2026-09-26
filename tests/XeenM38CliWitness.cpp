// Actual CLI/Application with deterministic typed input. Every response follows a
// successful native SDL presentation; F9 is injected as an SDL keyboard event.
#include "app/Application.h"
#include "XeenRestoreReplayProbe.h"
#include "app/XeenGameplayServices.h"
#include "platform/XeenSaveFile.h"
#include "formats/xeen/XeenSaveFormat.h"
#include "games/xeen/XeenCharacterRules.h"
#include "games/xeen/XeenInstallationDetector.h"
#include "formats/xeen/XeenAssetSource.h"
#include "games/xeen/XeenIndoorScene.h"
#define SDL_MAIN_HANDLED
#include <SDL.h>
#include <cstdlib>
#include <deque>
#include <iostream>
#include <stdexcept>
namespace { bool failNativeUpload=false,failNativeCopy=false,nativeFailed=false; }
extern "C" int __real_SDL_UpdateTexture(SDL_Texture *,const SDL_Rect *,const void *,int);
extern "C" int __wrap_SDL_UpdateTexture(SDL_Texture *texture,const SDL_Rect *rect,const void *pixels,int pitch) {
 if(failNativeUpload){failNativeUpload=false;nativeFailed=true;return SDL_SetError("Injected M38 native upload failure");}
 return __real_SDL_UpdateTexture(texture,rect,pixels,pitch);
}
extern "C" int __real_SDL_RenderCopy(SDL_Renderer *,SDL_Texture *,const SDL_Rect *,const SDL_Rect *);
extern "C" int __wrap_SDL_RenderCopy(SDL_Renderer *renderer,SDL_Texture *texture,const SDL_Rect *source,const SDL_Rect *destination) {
 if(failNativeCopy){failNativeCopy=false;nativeFailed=true;return SDL_SetError("Injected M38 native render failure");}
 return __real_SDL_RenderCopy(renderer,texture,source,destination);
}
using namespace mmodern;
namespace fs=std::filesystem;
#define PLAY_SYMBOL "_ZNK7mmodern11Application12playGameplayERKNS_20XeenGameplayServicesENS_10XeenCameraERKSt8optionalINSt10filesystem7__cxx114pathEEbNS_18XeenEncounterEntryES5_IjES5_ItE"
extern "C" int realPlay(const Application *,const XeenGameplayServices &,XeenCamera,const std::optional<fs::path> &,bool,XeenEncounterEntry,std::optional<std::uint32_t>,std::optional<std::uint16_t>) asm("__real_" PLAY_SYMBOL);
extern "C" int wrappedPlay(const Application *,const XeenGameplayServices &,XeenCamera,const std::optional<fs::path> &,bool,XeenEncounterEntry,std::optional<std::uint32_t>,std::optional<std::uint16_t>) asm("__wrap_" PLAY_SYMBOL);
namespace {
void check(bool v,const char *m){if(!v)throw std::runtime_error(m);}
template<class F> auto countProvider(F fn,unsigned &calls) {
 return [fn=std::move(fn),&calls](auto &&...args)->decltype(auto) {
  ++calls;return fn(std::forward<decltype(args)>(args)...);
 };
}
}
extern "C" int wrappedPlay(const Application *app,const XeenGameplayServices &original,XeenCamera camera,
 const std::optional<fs::path> &target,bool resume,XeenEncounterEntry entry,
 std::optional<std::uint32_t> seed,std::optional<std::uint16_t> contract) {
 if(resume) {
  replay_test::journeyInitializations=replay_test::journeyConstructions=0;
  replay_test::actions=replay_test::pulses=replay_test::retirements=0;
  replay_test::commands=replay_test::draws=0;
  replay_test::constructions=replay_test::services=replay_test::preparations=0;
  replay_test::timePreparations=replay_test::eventExecutions=0;
  replay_test::transfers=replay_test::equipmentChanges=0;
 }
 // ScummVM archive registration is process-wide; keep the read-only view probe source alive through CLI teardown.
 std::unique_ptr<XeenAssetSource> viewAssets;
 auto services=original;
 XeenEventFlow *flow=nullptr;XeenWorld *world=nullptr;const XeenPartyState *party=nullptr;
 const XeenCamera *position=nullptr;const XeenGameFlags *flags=nullptr;
 std::uint64_t now=0,cycle=0;unsigned saveCalls=0;
 const std::string control=std::getenv("MMODERN_M38_CONTROL")?std::getenv("MMODERN_M38_CONTROL"):"";
 bool faultFired=false,renderFault=false;unsigned providerCalls=0;std::function<void()> recursiveProbe;
 #define COUNT_PROVIDER(field) if(services.field)services.field=countProvider(services.field,providerCalls)
 COUNT_PROVIDER(resources.loadInitialParty);COUNT_PROVIDER(resources.loadEvents);
 COUNT_PROVIDER(resources.loadInitialCharacters);COUNT_PROVIDER(resources.loadInitialContext);
 COUNT_PROVIDER(resources.loadMonsterStatistics);COUNT_PROVIDER(resources.regionalManifest);
 COUNT_PROVIDER(resources.vertigoManifest);COUNT_PROVIDER(resources.loadInitialPurse);
 COUNT_PROVIDER(resources.loadInitialRegionalRecovery);COUNT_PROVIDER(resources.loadRegionalText);
 COUNT_PROVIDER(resources.loadLearnedSpellNames);COUNT_PROVIDER(maps);COUNT_PROVIDER(objects);
 COUNT_PROVIDER(texts);COUNT_PROVIDER(compose);COUNT_PROVIDER(npcDraw);COUNT_PROVIDER(initializeEncounter);
 COUNT_PROVIDER(validateEncounterSprite);COUNT_PROVIDER(validateCombatSprite);COUNT_PROVIDER(prepareCombat);
 COUNT_PROVIDER(sampleJourneySeed);
#undef COUNT_PROVIDER
 const auto compose=original.composeEncounter;
 services.composeEncounter=[&](auto &w,const auto &p,const auto &c,auto phase,auto actor){++providerCalls;if(renderFault){renderFault=false;throw std::runtime_error("Injected composition failure");}if(control=="recursive" && recursiveProbe)recursiveProbe();return compose(w,p,c,phase,actor);};
 services.clock=[&]{return now;};
 services.observeSaveStage=[&](auto){++saveCalls;};
 services.observeGameplay=[&](auto &w,auto &,const auto &p,const auto &c,const auto &f){world=&w;party=&p;position=&c;flags=&f;};
  services.configureFlow=[&](auto &f,const auto &c){
  original.configureFlow(f,c);flow=&f;
  if(f.drawSmithArt) {
   const auto art=f.drawSmithArt;
   f.drawSmithArt=[&,art](IndexedFrame &frame) {
    ++providerCalls;art(frame);
    if(control=="immutable-art" || control=="aba-art-provider") {
     faultFired=true;
     if(control=="immutable-art")throw std::invalid_argument("Injected immutable smith art mismatch");
     auto &owner=const_cast<XeenPartyState &>(*party).roster.at(29);
     ++owner.currentHp;--owner.currentHp;
    }
   };
  }
  f.smithBoundary=[&](XeenSmithBoundary boundary){
   // Explicit artificial fault controls, separate from the production witness.
   if(control=="recursive"){if(recursiveProbe)recursiveProbe();return;}
   if(faultFired || control.empty())return;
   const auto selected=control=="fail-quote" || control=="aba-quote"?XeenSmithBoundary::Quote:
    control=="aba-departure"?XeenSmithBoundary::BeforeDeparture:
    control=="fail-before-admission"?XeenSmithBoundary::BeforeAdmission:
    control=="fail-after-admission" || control=="render-admission" || control=="upload-admission" || control=="copy-admission"?XeenSmithBoundary::AfterAdmission:
    control=="fail-after-repair" || control=="aba-after-repair" || control=="render-repair" || control=="upload-repair" || control=="copy-repair"?XeenSmithBoundary::AfterRepair:
    control=="fail-before-departure"?XeenSmithBoundary::BeforeDeparture:
    control=="fail-after-departure" || control=="upload-departure" || control=="copy-departure"?XeenSmithBoundary::AfterDeparture:
    control=="fail-return" || control=="render-return"?XeenSmithBoundary::Return:XeenSmithBoundary::BeforeRepair;
   if(boundary!=selected)return;
   faultFired=true;std::cout<<"M38 ARTIFICIAL CONTROL "<<control<<'\n';
   if(control.rfind("upload-",0)==0){failNativeUpload=true;return;}
   if(control.rfind("copy-",0)==0){failNativeCopy=true;return;}
   if(control.rfind("render-",0)==0){renderFault=true;return;}
   if(control.rfind("fail-",0)==0)throw std::runtime_error("Injected ordinary preparation failure");
   auto &p=const_cast<XeenPartyState &>(*party);
   if(control=="aba-inactive") {++p.roster.at(29).currentHp;--p.roster.at(29).currentHp;}
   else if(control=="aba-mainland") {auto &a=const_cast<XeenActor &>(world->sessionState().actors().at(0));++a.hp;--a.hp;}
   else if(control=="aba-cache") {world->discardMapCache();auto &map=const_cast<XeenMap &>(world->map(28));map.geometry.cells[0].rawAttributes^=1;map.geometry.cells[0].rawAttributes^=1;}
   else if(control=="aba-object-cache") {world->discardMapCache();auto &mob=const_cast<XeenObjectFile &>(world->objectFile(28));++mob.entities.objects[53].x;--mob.entities.objects[53].x;}
   else if(control=="aba-membership") {const auto saved=p.party;p.party=XeenParty::fromRosterIds({0,1,2,3,4,5});p.party=saved;}
   else if(control=="aba-purse") {++p.monsterTreasure->gold;--p.monsterTreasure->gold;}
   else if(control=="aba-context") {++p.encounterContext->minutes;--p.encounterContext->minutes;}
   else if(control=="aba-city") {auto &a=const_cast<XeenActor &>(world->sessionState().regionalActors(28).at(0));++a.hp;--a.hp;}
   else if(control=="aba-item" || control=="aba-after-repair" || control=="aba-quote" || control=="aba-departure") {p.roster.at(6).armor[0].frame^=1;p.roster.at(6).armor[0].frame^=1;}
   else throw std::runtime_error("Unknown M38 artificial control");
  };
 };
 services.show=[&](const IndexedFrame &first,const auto &handler,const auto &escape,const auto &idle,const auto &status){
  check(target && flow && world && party && position && flags,"M38 production owners absent");
  check(world->sessionState().journeyContract()==9,"M38 expected content 9");
  recursiveProbe=[&] {
   const auto calls=providerCalls,saves=saveCalls;
   const auto gold=std::uint32_t(party->monsterTreasure->gold);
   const auto date=*party->encounterContext;
   handler.withDisplayedInput(SaveGameAction{},flow->displayedInput().value_or(0));
   handler.withDisplayedInput(AcknowledgeAction{},flow->displayedInput().value_or(0));
   handler.withDisplayedInput(CancelInteractionAction{},flow->displayedInput().value_or(0));
   check(calls==providerCalls && saves==saveCalls && gold==party->monsterTreasure->gold && date==*party->encounterContext,
    "M38 recursive provider response/save reached work or publication");
  };
  std::deque<std::function<bool()>> steps;
  std::optional<IndexedFrame> next;
  bool shown=false,acted=false,breakArmor=false;unsigned blocks=0,iterations=0,nativeSaveResponses=0;
  const auto act=[&](PlayerAction action){
   check(shown,"M38 input preceded successful presentation");
   const auto token=handler.displayedInput();check(bool(token),"M38 displayed token absent");
   const bool wasService=world->sessionState().journeyActivity()==XeenJourneyActivity::Service;
   next=handler.withDisplayedInput(action,*token);acted=true;shown=false;
   if(wasService || world->sessionState().journeyActivity()==XeenJourneyActivity::Service) {
    const auto beforeCalls=providerCalls;const auto beforeSave=saveCalls;
    handler.withDisplayedInput(SaveGameAction{},*token); // Pending unpresented handoff must refuse before provider work.
    const auto gold=std::uint32_t(party->monsterTreasure->gold);
    const auto context=*party->encounterContext;
    flow->handle(action,*token); // Consumed or unpresented direct seam.
    flow->handle(AcknowledgeAction{},flow->displayedInput());
    if(const auto pending=flow->presentationGeneration())check(!flow->respond(*pending,XeenPresentationResponse::Acknowledged),"M38 direct Event seam escaped service");
    check(beforeCalls==providerCalls && beforeSave==saveCalls && gold==party->monsterTreasure->gold && context==*party->encounterContext,
     "M38 stale/unpresented response invoked a provider or publication");
    auto forged=std::make_shared<const IndexedFrame>(flow->frame());
    check(!flow->acceptsFrame(forged),"M38 equal foreign frame was authority");
   }
  };
  const bool nativeControls=std::getenv("MMODERN_M38_STAGE") && (std::string(std::getenv("MMODERN_M38_STAGE"))=="native" || std::string(std::getenv("MMODERN_M38_STAGE"))=="synthetic-native");
  const auto action=[&](PlayerAction a){
   if(!nativeControls){steps.push_back([&,a]{act(a);return true;});return;}
   SDL_Keycode key=SDLK_UNKNOWN;
   if(std::holds_alternative<InteractionAction>(a))key=SDLK_SPACE;
   if(std::holds_alternative<AcknowledgeAction>(a))key=SDLK_RETURN;
   if(std::holds_alternative<CancelInteractionAction>(a))key=SDLK_ESCAPE;
   if(std::holds_alternative<YesAction>(a))key=SDLK_y;
   if(std::holds_alternative<NoAction>(a))key=SDLK_n;
   if(const auto *m=std::get_if<SelectMemberAction>(&a))key=SDLK_F1+static_cast<int>(m->partyIndex);
   if(const auto *s=std::get_if<SelectInventorySlotAction>(&a))key=SDLK_1+static_cast<int>(s->slot);
   check(key!=SDLK_UNKNOWN,"M38 native action mapping absent");
   auto token=std::make_shared<std::uint64_t>();auto phase=std::make_shared<unsigned>(0);
   steps.push_back([&,key,token,phase]{
    const auto send=[&](SDL_Keycode k,bool down,bool repeat=false){SDL_Event e{};e.type=down?SDL_KEYDOWN:SDL_KEYUP;
     e.key.keysym.sym=k;e.key.keysym.scancode=SDL_GetScancodeFromKey(k);e.key.timestamp=SDL_GetTicks()+1;e.key.repeat=repeat;
     check(SDL_PushEvent(&e)==1,"M38 native batch enqueue");};
    if(!*phase){*token=*handler.displayedInput();send(key,true);send(key,true);send(key,true,true);
     send(key==SDLK_RETURN?SDLK_ESCAPE:SDLK_RETURN,true);++*phase;return false;}
    if(*phase==1) {
     if(*handler.displayedInput()==*token)return false;
     check(*handler.displayedInput()==*token+1,"M38 native batch crossed multiple boundaries");
     *token=*handler.displayedInput();send(key,true);send(key,true,true);
     for(auto event:{SDL_WINDOWEVENT_SIZE_CHANGED,SDL_WINDOWEVENT_EXPOSED}) {
      SDL_Event redraw{};redraw.type=SDL_WINDOWEVENT;redraw.window.event=event;
      redraw.window.data1=640;redraw.window.data2=400;check(SDL_PushEvent(&redraw)==1,"M38 native redraw enqueue");
     }
     ++*phase;return false;
    }
    check(*handler.displayedInput()==*token,"M38 held key crossed a later frame boundary");
    send(key,false);send(key==SDLK_RETURN?SDLK_ESCAPE:SDLK_RETURN,false);
    return true;
   });
  };
  const auto inspect=[&](std::function<void()> f){steps.push_back([f]{f();return true;});};
  const auto settle=[&]{steps.push_back([&]{
   if(flow->canSave())return true;
   if(const auto *combat=flow->encounter()->combat()) {
    check(combat->phase()!=XeenCombatPhase::Failed && combat->phase()!=XeenCombatPhase::Defeat &&
     combat->phase()!=XeenCombatPhase::SupportStopped,"M38 combat support stop");
    if(combat->phase()==XeenCombatPhase::PlayerReady) {
     if(breakArmor && !(party->roster.at(6).armor[0].state&128)){++blocks;act(BlockAction{});}
     else act(AttackAction{});
    }
   } else if(world->sessionState().journeyActivity()==XeenJourneyActivity::Event || world->sessionState().journeyActivity()==XeenJourneyActivity::Reward) act(AcknowledgeAction{});
   check(flow->encounter()->state().phase()!=XeenEncounterPhase::SupportStopped,"M38 exploration support stop");
   return false;
  });};
  const auto nav=[&](NavigationAction a){action(a);settle();};
  const auto route=[&](const char *s){for(;*s;++s)switch(*s){
   case 'U':nav(NavigationAction::MoveForward);break;
   case 'D':nav(NavigationAction::MoveBackward);break;
   case 'L':nav(NavigationAction::TurnLeft);break;
   case 'R':nav(NavigationAction::TurnRight);break;
   case 'F':action(ShootAction{});settle();break;
  }};
  const auto pushKey=[&](SDL_Keycode key,bool down=true){SDL_Event e{};e.type=down?SDL_KEYDOWN:SDL_KEYUP;e.key.keysym.sym=key;
   e.key.keysym.scancode=SDL_GetScancodeFromKey(key);e.key.timestamp=SDL_GetTicks()+1;
   check(SDL_PushEvent(&e)==1,"M38 native key enqueue failed");};
  const auto checkpoint=[&](char label){
   auto count=std::make_shared<unsigned>();
   inspect([&,count]{check(flow->canSave(),"M38 checkpoint is not Quiet");*count=saveCalls;pushKey(SDLK_F9);});
   steps.push_back([&,count,label]{
    if(saveCalls==*count)return false;
    check(saveCalls==*count+3,"M38 F9 did not complete the normal save pipeline");
    pushKey(SDLK_F9,false);
    const auto copy=target->parent_path()/(target->stem().string()+"-"+label+".mmsave");
    fs::copy_file(*target,copy,fs::copy_options::overwrite_existing);
    std::cout<<"M38 CHECKPOINT "<<label<<" day="<<party->encounterContext->day<<" minute="<<party->encounterContext->minutes
     <<" gold="<<party->monsterTreasure->gold<<" rng="<<world->sessionState().journeyRandom()->count<<'\n';
    return true;
   });
  };
  const auto deniedSave=[&]{
   auto count=std::make_shared<unsigned>();auto responses=std::make_shared<unsigned>();auto prior=std::make_shared<std::vector<std::uint8_t>>();
   inspect([&,count,responses,prior]{check(!flow->canSave(),"M38 service exposed Quiet");*count=saveCalls;*responses=nativeSaveResponses;
    *prior=XeenSaveFormat::encode(XeenSaveFile::read(*target));pushKey(SDLK_F9);});
   steps.push_back([&,count,responses,prior]{
    if(nativeSaveResponses==*responses)return false;
    check(nativeSaveResponses==*responses+1 && status().find("Cannot save")!=std::string::npos,"M38 native F9 refusal response missing");
    check(saveCalls==*count && *prior==XeenSaveFormat::encode(XeenSaveFile::read(*target)),"M38 refused F9 performed work or changed disk");
    pushKey(SDLK_F9,false);return true;
   });
  };
  const std::string stage=std::getenv("MMODERN_M38_STAGE")?std::getenv("MMODERN_M38_STAGE"):"fresh";
  inspect([&]{if(resume) {
   check(!replay_test::journeyInitializations && !replay_test::journeyConstructions &&
    !replay_test::actions && !replay_test::pulses && !replay_test::commands &&
    !replay_test::draws && !replay_test::retirements && !replay_test::constructions &&
    !replay_test::services && !replay_test::preparations &&
    !replay_test::timePreparations && !replay_test::eventExecutions &&
    !replay_test::transfers && !replay_test::equipmentChanges,
    "M38 restore replayed initialization, Event, combat, time, inventory or RNG");
   const auto saved=XeenSaveFile::read(*target);
   const auto live=XeenSaveState::capture(original.resources.signature,*party,*position,*flags,*world);
   check(XeenSaveFormat::encode(saved)==XeenSaveFormat::encode(live),"M38 restore changed durable state before first input");
   std::cout<<"M38 RESTORE EXACT BEFORE INPUT\n";
  }});
  if(!resume) {
   route("UFUDD");
   inspect([&]{check(party->monsterTreasure->gold==810,"M38 production earned gold mismatch");});
   route("LLULUU");action(InteractionAction{});action(YesAction{});settle();
   inspect([&]{breakArmor=true;});route("URULUUULUUU");
   inspect([&]{breakArmor=false;
    check(blocks==39 && position->mapId==XeenMapIdentity(28) && position->x==13 && position->y==4,
     "M38 original broken-armor route mismatch");
    check(party->roster.at(6).currentHp==-11 && party->roster.at(6).armor[0].state==128 &&
     party->roster.at(6).armor[1].state==128 && party->encounterContext->minutes==577 &&
     world->sessionState().journeyRandom()->count==317,"M38 natural injury witness mismatch");
   });
   for(unsigned i=0;i<2;++i) {
    action(CastSpellAction{});action(SelectMemberAction{4});action(NavigationAction::MoveBackward);
    action(AcknowledgeAction{});action(AcknowledgeAction{});action(SelectMemberAction{5});settle();
   }
   inspect([&]{check(party->roster.at(6).currentHp==1 && party->roster.at(1).currentSp==19 &&
    party->encounterContext->minutes==579,"M38 First Aid witness mismatch");});
   route("UUUUU");inspect([&]{check(position->x==8 && position->y==4 && party->encounterContext->minutes==584,"M38 five-cell corridor endpoint/time");});checkpoint('A');
  }
  const auto visit=[&](unsigned slot,std::uint32_t goldAfter){
   auto before=std::make_shared<XeenSaveSnapshot>();auto ac=std::make_shared<int>();
   inspect([&,before,ac]{*before=XeenSaveState::capture(original.resources.signature,*party,*position,*flags,*world);
    *ac=XeenCharacterRules::combatArmorClass(party->roster.at(6),*party->roster.combatInputs(6),{party->encounterContext->year});});
      action(InteractionAction{});
   if(control=="fail-before-admission" && slot==0) {
    inspect([&]{if(party->encounterContext->day==8)check(flow->canSave(),"Pre-admission failure retained obligation");});
    action(InteractionAction{});
   }
   deniedSave();action(SelectMemberAction{5});action(AcknowledgeAction{});deniedSave();
   action(SelectInventorySlotAction{slot});action(AcknowledgeAction{});
   if(control=="fail-quote" && slot==0)action(AcknowledgeAction{});
   action(SelectMemberAction{0});action(SelectInventorySlotAction{8}); // Quote remains bound to the selected physical item.
   action(NoAction{}); // Free quote cancellation; visit remains owed.
      action(AcknowledgeAction{});deniedSave();action(AcknowledgeAction{});
   if(control=="fail-before-repair" && slot==0) action(AcknowledgeAction{});
   inspect([&,slot,goldAfter,ac]{check(XeenCharacterRules::combatArmorClass(party->roster.at(6),*party->roster.combatInputs(6),{party->encounterContext->year})==*ac+(slot==0?2:1),"M38 repaired AC contribution");check(party->monsterTreasure->gold==goldAfter && party->roster.at(6).armor[slot].state==0,
    "M38 payment/item publication mismatch");});
   deniedSave();action(AcknowledgeAction{});action(AcknowledgeAction{}); // Intact refusal.
   inspect([&,goldAfter]{check(party->monsterTreasure->gold==goldAfter,"M38 intact repeat charged gold");});
      action(AcknowledgeAction{});action(CancelInteractionAction{});action(CancelInteractionAction{});
   if(slot==0 && (control=="fail-before-departure" || control=="fail-after-departure" || control=="fail-return")) {deniedSave();action(AcknowledgeAction{});}
   settle();
   inspect([&,before,slot,goldAfter]{
    auto expected=*before;expected.characters[6].armor[slot].state=0;
    expected.journey->treasure->gold=goldAfter;++expected.journey->context->day;
    const auto actual=XeenSaveState::capture(original.resources.signature,*party,*position,*flags,*world);
    check(XeenSaveFormat::encode(actual)==XeenSaveFormat::encode(expected),"M38 visit changed unrelated durable state");
   });
  };
  if(stage=="multi") {
   auto expected=std::make_shared<XeenSaveSnapshot>();
   inspect([&,expected]{*expected=XeenSaveState::capture(original.resources.signature,*party,*position,*flags,*world);});
   action(InteractionAction{});action(SelectMemberAction{5});action(AcknowledgeAction{});
   for(unsigned slot=0;slot<2;++slot) {
    action(SelectInventorySlotAction{slot});action(AcknowledgeAction{});action(AcknowledgeAction{});deniedSave();
    inspect([&,slot]{check(party->encounterContext->day==8 && party->monsterTreasure->gold==(slot?807u:808u),
     "M38 multiple repairs settled early or rolled back earlier payment");});
    action(AcknowledgeAction{});
   }
   action(CancelInteractionAction{});action(CancelInteractionAction{});settle();
   inspect([&,expected]{expected->characters[6].armor[0].state=0;expected->characters[6].armor[1].state=0;
    expected->journey->treasure->gold=807;expected->journey->context->day=9;
    check(XeenSaveFormat::encode(*expected)==XeenSaveFormat::encode(XeenSaveState::capture(original.resources.signature,*party,*position,*flags,*world)),
     "M38 multiple repairs did not preserve unrelated durable state");});checkpoint('M');
  } else if(stage=="views") {
   inspect([&] {
    const auto before=XeenSaveFormat::encode(XeenSaveState::capture(original.resources.signature,*party,*position,*flags,*world));
    const char *root=std::getenv("MMODERN_M38_INSTALLATION");check(root,"M38 original view installation absent");
    const auto installation=XeenInstallationDetector().detect(root);check(bool(installation),"M38 original view installation invalid");
    viewAssets=std::make_unique<XeenAssetSource>(*installation,320,200);const auto resolver=XeenObjectVisualResolver::load(*viewAssets);
    unsigned count=0;
    for(int y=0;y<=4;++y)for(int x=8;x<=16;++x) {
     if(!((x==15 && y<=4)||(x==16 && y>=1)||(y==4 && x>=8 && x<=14)))continue;
     for(unsigned facing=0;facing<4;++facing)for(unsigned phase=0;phase<8;++phase) {
      const XeenCamera sample{28,x,y,static_cast<XeenDirection>(facing)};
      std::vector<XeenObjectVisual> diagnostics;
      XeenIndoorScene().build(*world,sample,&resolver,&diagnostics,phase);
      check(diagnostics.empty(),"M38 admitted corridor has unsupported original objects");
      const auto a=original.composeEncounter(*world,*party,sample,phase,{}).frame;
      world->discardMapCache();
      const auto b=original.composeEncounter(*world,*party,sample,phase,{}).frame;
      check(a.isValid() && a.pixels==b.pixels && a.palette==b.palette,"M38 corridor appearance changed after cache reconstruction");
      ++count;
     }
    }
    check(count==512 && before==XeenSaveFormat::encode(XeenSaveState::capture(original.resources.signature,*party,*position,*flags,*world)),
     "M38 all-cell/facing/phase composition mutated durable state");
    std::cout<<"M38 512 ORIGINAL CORRIDOR VIEWS AND RECONSTRUCTIONS PASSED\n";
   });checkpoint('V');
  } else if(stage=="synthetic" || stage=="synthetic-native") {
   // Explicit artificial funds/inventory fixture, never the production witness.
   auto expected=std::make_shared<XeenSaveSnapshot>();
   inspect([&,expected]{*expected=XeenSaveState::capture(original.resources.signature,*party,*position,*flags,*world);});
   action(InteractionAction{});deniedSave();action(SelectMemberAction{5});action(AcknowledgeAction{});deniedSave();
   action(SelectInventorySlotAction{8});action(AcknowledgeAction{});deniedSave();action(AcknowledgeAction{});
   inspect([&,expected]{
    if(expected->journey->treasure->gold) {
     --expected->journey->treasure->gold;expected->characters[6].armor[8].state=0x45;
    }
    check(party->monsterTreasure->gold==expected->journey->treasure->gold &&
     party->roster.at(6).armor[8].state==expected->characters[6].armor[8].state,
     "M38 artificial exact/insufficient/full-u32 payment");
   });
   action(AcknowledgeAction{});action(CancelInteractionAction{});action(CancelInteractionAction{});settle();
   inspect([&,expected]{++expected->journey->context->day;
    check(XeenSaveFormat::encode(*expected)==XeenSaveFormat::encode(
     XeenSaveState::capture(original.resources.signature,*party,*position,*flags,*world)),
     "M38 artificial inventory aliases/holes/inactive owner/dormant treasure changed");
   });
   checkpoint('S');
  } else {
  const bool empty=stage.rfind("empty",0)==0;
  const bool detour=stage.rfind("detour",0)==0;
  if(empty) {
   if(stage=="empty") {
   action(InteractionAction{});deniedSave();action(CancelInteractionAction{});settle();
   inspect([&]{check(party->encounterContext->day==9 && party->encounterContext->minutes==584 &&
    party->monsterTreasure->gold==810 && party->roster.at(6).armor[0].state==128 &&
    party->roster.at(6).armor[1].state==128,"M38 transaction-free departure mismatch");});
   checkpoint('E');
   }
   if(stage!="empty-F") {visit(0,808);checkpoint('F');}
  } else if(detour) {
   if(!resume || stage=="detour-A") {visit(0,808);checkpoint('B');}
   if(stage!="detour-E" && stage!="detour-F" && stage!="detour-G") {
    route("DDDDDDDLUUUU");
    inspect([&]{check(position->x==15 && position->y==0 && position->direction==XeenDirection::South,"M38 city exit route");});
    action(InteractionAction{});action(YesAction{});settle();
    inspect([&]{check(position->mapId==XeenMapIdentity(23) && world->sessionState().regionalActors(28).size()==52,
     "M38 original city reset/return absent");});checkpoint('E');
   }
   if(stage!="detour-F" && stage!="detour-G") {
    route("LLU");action(InteractionAction{});action(YesAction{});settle();
    route("URULUUULUUUUUUUU");
    inspect([&]{check(position->x==8 && position->y==4 &&
     world->sessionState().regionalActors(28).at(36).lifecycle==XeenActorLifecycle::Defeated,
     "M38 reset/revisit route absent");});checkpoint('F');
   }
   if(stage!="detour-G") {visit(1,807);checkpoint('G');}
   inspect([&]{check(party->encounterContext->day==10,"M38 detour second departure absent");});
  } else {
   if(!resume || (stage=="A" || stage=="native")) {visit(0,808);checkpoint('B');}
   if(!resume || stage=="A" || stage=="B" || stage=="native") {visit(1,807);checkpoint('C');}
   inspect([&]{check(party->encounterContext->day==10 && party->encounterContext->minutes==584 &&
    party->monsterTreasure->gold==807 && world->sessionState().journeyRandom()->count==317,
    "M38 two-visit date/purse/RNG witness mismatch");});
  }
  auto refusalBefore=std::make_shared<XeenSaveSnapshot>();
  inspect([&,refusalBefore]{*refusalBefore=XeenSaveState::capture(original.resources.signature,*party,*position,*flags,*world);});
  action(InteractionAction{});settle();
  inspect([&,refusalBefore]{check(XeenSaveFormat::encode(*refusalBefore)==XeenSaveFormat::encode(
   XeenSaveState::capture(original.resources.signature,*party,*position,*flags,*world)),"M38 day10 refusal changed durable state");});
  checkpoint('D');
  }
  inspect([&]{std::cout<<"M38 PRODUCTION WITNESS PASSED\n";SDL_Event e{};e.type=SDL_QUIT;SDL_PushEvent(&e);});
  auto native=handler;native.closed={};
  native.withDisplayedInput=[&](const PlayerAction &a,std::uint64_t token){if(std::holds_alternative<SaveGameAction>(a))++nativeSaveResponses;return handler.withDisplayedInput(a,token);};
  native.beginCycle=[&](std::uint64_t){handler.beginCycle(++cycle);};
  unsigned evidenceFrame=0;
  native.framePresented=[&](const auto &f){
   check(!nativeFailed,"M38 failed native frame was authorized");
   handler.framePresented(f);shown=true;
   const char *directory=std::getenv("MMODERN_M38_FRAMES");
   if(directory && f && world->sessionState().journeyActivity()==XeenJourneyActivity::Service) {
    std::vector<std::uint32_t> pixels;pixels.reserve(f->pixels.size());
    for(auto index:f->pixels){const auto p=index*3;pixels.push_back(0xff000000u|(f->palette[p]<<16)|(f->palette[p+1]<<8)|f->palette[p+2]);}
    auto *surface=SDL_CreateRGBSurfaceFrom(pixels.data(),f->width,f->height,32,f->width*4,0xff0000,0xff00,0xff,0xff000000);
    check(surface,"M38 evidence surface");
    const auto path=fs::path(directory)/("smith-"+std::to_string(++evidenceFrame)+".bmp");
    const auto result=SDL_SaveBMP(surface,path.string().c_str());SDL_FreeSurface(surface);
    check(result==0,"M38 presented-frame evidence write");
   }
  };
  const auto drive=[&]()->std::optional<IndexedFrame>{
   check(++iterations<30000,"M38 native witness iteration bound");now+=100;next.reset();acted=false;
   if(shown) while(!steps.empty()) {
    const bool done=steps.front()();if(done)steps.pop_front();
    if(acted || !done)break;
   }
   if(acted)return next;
   auto updated=idle();if(updated)shown=false;return updated;
  };
    const bool ok=original.show(first,native,escape,drive,status);
  if(control.rfind("upload-",0)==0 || control.rfind("copy-",0)==0) {
   check(faultFired && nativeFailed && !ok && !flow->canSave(),"M38 native failure exposed Quiet");
   check(party->monsterTreasure->gold==(control.find("admission")!=std::string::npos?810u:808u) &&
    party->encounterContext->day==(control.find("departure")!=std::string::npos?9:8),
    "M38 native failure rolled back or repeated publication");
   const auto calls=providerCalls,saves=saveCalls;
   handler.withDisplayedInput(SaveGameAction{},flow->displayedInput().value_or(0));
   check(calls==providerCalls && saves==saveCalls,"M38 failed-state F9 reached providers");
   std::cout<<"M38 NATIVE FAILURE PRESERVATION PASSED\n";
  }
  if(control.rfind("aba-",0)==0 || control=="immutable-art") {
   check(faultFired && !ok && !flow->canSave(),"M38 ABA did not fail monotonically");
   check(party->monsterTreasure->gold==(control=="aba-after-repair" || control=="aba-departure"?808u:810u),"M38 ABA crossed payment boundary");
   check(party->encounterContext->day==8,"M38 ABA incorrectly settled departure");
   std::cout<<"M38 ABA UNSAVEABLE PRESERVATION PASSED\n";
  }
  return ok;
 };
 return realPlay(app,services,camera,target,resume,entry,seed,contract);
}
