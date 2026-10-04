// Dedicated content-11 original-resource witness. Normal owners are changed
// exclusively through fresh concrete-frame input and production mandatory work.
#include "XeenProbeFired.h"
#include "app/Application.h"
#include "app/XeenGameplayServices.h"
#include "platform/XeenSaveFile.h"
#include "formats/xeen/XeenSaveFormat.h"
#include "games/xeen/XeenCharacterRules.h"
#include "XeenRestoreReplayProbe.h"
#include "XeenM40Evidence.h"
#define SDL_MAIN_HANDLED
#include <SDL.h>
#include <cstdlib>
#include <deque>
#include <iostream>
#include <stdexcept>
namespace mmodern { struct XeenTrainingTestAccess {static bool admitted(const XeenEventFlow &f){return f._smithUi && f._smithUi->phase!=XeenEventFlow::SmithUi::Phase::Preparation;} static std::string text(const XeenEventFlow &f){return f._smithUi?f.smithText():"none";}}; }
using namespace mmodern;
namespace fs=std::filesystem;
namespace {
unsigned interestOperations=0;
bool failNativeUpload=false,failNativeCopy=false,nativeFailed=false;
std::string env(const char *key,const char *fallback="") {const auto value=std::getenv(key);return value?value:fallback;}
using m40_test::check;
}
extern "C" int __real_SDL_UpdateTexture(SDL_Texture *,const SDL_Rect *,const void *,int);
extern "C" int __wrap_SDL_UpdateTexture(SDL_Texture *texture,const SDL_Rect *rect,const void *pixels,int pitch) {probe_fired::hit("SDL_UpdateTexture");
    if(failNativeUpload){failNativeUpload=false;nativeFailed=true;return SDL_SetError("M40 injected native upload failure");}
    return __real_SDL_UpdateTexture(texture,rect,pixels,pitch);
}
extern "C" int __real_SDL_RenderCopy(SDL_Renderer *,SDL_Texture *,const SDL_Rect *,const SDL_Rect *);
extern "C" int __wrap_SDL_RenderCopy(SDL_Renderer *renderer,SDL_Texture *texture,const SDL_Rect *source,const SDL_Rect *destination) {probe_fired::hit("SDL_RenderCopy");
    if(failNativeCopy){failNativeCopy=false;nativeFailed=true;return SDL_SetError("M40 injected native render failure");}
    return __real_SDL_RenderCopy(renderer,texture,source,destination);
}
#define M40_INTEREST_SYMBOL "_ZN7mmodern23xeenPrepareBankInterestERKNS_16XeenBankBalancesE"
XeenBankBalances realInterest(const XeenBankBalances &) asm("__real_" M40_INTEREST_SYMBOL);
XeenBankBalances wrappedInterest(const XeenBankBalances &) asm("__wrap_" M40_INTEREST_SYMBOL);
XeenBankBalances wrappedInterest(const XeenBankBalances &bank) {probe_fired::hit("xeenPrepareBankInterest");++interestOperations;return realInterest(bank);}
#define PLAY_SYMBOL "_ZNK7mmodern11Application12playGameplayERKNS_20XeenGameplayServicesENS_10XeenCameraERKSt8optionalINSt10filesystem7__cxx114pathEEbNS_18XeenEncounterEntryES5_IjE"
extern "C" int realPlay(const Application *,const XeenGameplayServices &,XeenCamera,const std::optional<fs::path> &,bool,XeenEncounterEntry,std::optional<std::uint32_t>) asm("__real_" PLAY_SYMBOL);
extern "C" int wrappedPlay(const Application *,const XeenGameplayServices &,XeenCamera,const std::optional<fs::path> &,bool,XeenEncounterEntry,std::optional<std::uint32_t>) asm("__wrap_" PLAY_SYMBOL);
static const probe_fired::Expect playProbe{"Application::playGameplay","SDL_RenderCopy","SDL_UpdateTexture"};
extern "C" int wrappedPlay(const Application *app,const XeenGameplayServices &original,XeenCamera camera,
 const std::optional<fs::path> &target,bool resume,XeenEncounterEntry entry,
 std::optional<std::uint32_t> seed) {probe_fired::hit("Application::playGameplay");if(!resume)for(const char *probe:{"XEEN_REPLAY_COMMAND","XEEN_REPLAY_DRAW","XEEN_REPLAY_JOURNEY_CONSTRUCT","XEEN_REPLAY_REGIONAL_MOVE","XEEN_REPLAY_SERVICE","XEEN_REPLAY_TIME","XEEN_REPLAY_RETIRE","XEEN_REPLAY_MOVE","XEEN_REPLAY_EVENT_BEGIN","XEEN_REPLAY_FRESH_PUBLICATION_INITIALIZE","xeenPrepareBankInterest"})probe_fired::expect(probe);
    const auto stage=env("MMODERN_M40_STAGE","fresh"),branch=env("MMODERN_M40_BRANCH","production"),control=env("MMODERN_M40_CONTROL");
    if(resume) {
        replay_test::journeyInitializations=replay_test::journeyConstructions=0;
        replay_test::actions=replay_test::pulses=replay_test::retirements=0;
        replay_test::commands=replay_test::draws=0;
        replay_test::constructions=replay_test::services=replay_test::preparations=0;
        replay_test::timePreparations=replay_test::eventExecutions=0;
        replay_test::transfers=replay_test::equipmentChanges=0;
    }
    interestOperations=0;
    auto services=original;
    XeenEventFlow *flow=nullptr;XeenWorld *world=nullptr;const XeenPartyState *party=nullptr;
    const XeenCamera *position=nullptr;const XeenGameFlags *flags=nullptr;
    std::uint64_t now=0,cycle=0;unsigned saveCalls=0,providerCalls=0,bankInputCalls=0;
    bool faultFired=false,retriedPreparation=false;std::function<void()> recursiveProbe;
    bool clockArmed=false,clockVerified=false,recoveryFired=false;
    unsigned armedClockCalls=0;
    const auto mutateEconomy=[&] {
        auto &p=const_cast<XeenPartyState &>(*party);const auto before=*p.serviceEconomy;
        if(control.find("bank-gold")!=std::string::npos){++p.serviceEconomy->bank.gold;--p.serviceEconomy->bank.gold;}
        else if(control.find("bank-gems")!=std::string::npos){++p.serviceEconomy->bank.gems;--p.serviceEconomy->bank.gems;}
        else if(control.find("stock")!=std::string::npos){p.serviceEconomy->wares[1][3][3][8].material^=1;p.serviceEconomy->wares[1][3][3][8].material^=1;}
        else if(control.find("presence")!=std::string::npos){p.serviceEconomy.reset();p.serviceEconomy=before;}
        else throw std::runtime_error("Unknown combat callback ABA control");
        check(*p.serviceEconomy==before,"Combat callback ABA did not restore identical economy values");
    };
    unsigned committedInterestOperations=0;
    const auto count=[&](auto fn){return [fn=std::move(fn),&providerCalls](auto &&...args)->decltype(auto){++providerCalls;return fn(std::forward<decltype(args)>(args)...);};};
#define M40_COUNT(field) if(services.field)services.field=count(services.field)
    M40_COUNT(resources.loadInitialParty);M40_COUNT(resources.loadInitialCharacters);M40_COUNT(resources.loadInitialContext);
    M40_COUNT(resources.loadEvents);M40_COUNT(resources.loadMonsterStatistics);M40_COUNT(resources.regionalManifest);M40_COUNT(resources.vertigoManifest);
    M40_COUNT(resources.loadInitialPurse);M40_COUNT(resources.loadInitialRegionalRecovery);M40_COUNT(resources.loadRegionalText);M40_COUNT(resources.loadLearnedSpellNames);
    M40_COUNT(maps);M40_COUNT(objects);M40_COUNT(texts);M40_COUNT(compose);M40_COUNT(npcDraw);
    M40_COUNT(validateEncounterSprite);M40_COUNT(validateCombatSprite);M40_COUNT(sampleJourneySeed);
#undef M40_COUNT
    const auto bankInput=original.resources.loadInitialBankBalances;
    services.resources.loadInitialBankBalances=[&]{++providerCalls;++bankInputCalls;check(bool(bankInput),"M40 bank resource provider absent");return bankInput();};
    const auto compose=original.composeEncounter;
    services.composeEncounter=[&](auto &w,const auto &p,const auto &c,auto phase,auto actor){
        ++providerCalls;
        if(clockArmed && !faultFired && control.rfind("presentation-",0)==0) {
            faultFired=true;mutateEconomy();std::cout<<"M40 COMBAT PRESENTATION EQUAL-VALUE ABA EXECUTED\n";
        }
        if(clockArmed && faultFired && !recoveryFired && control.find("recovery")!=std::string::npos) {
            recoveryFired=true;throw std::runtime_error("M40 healthy cosmetic presentation retry");
        }
        return compose(w,p,c,phase,actor);
    };
    services.clock=[&]{
        if(clockArmed && !faultFired && control.rfind("clock-",0)==0) {
            check(flow->encounter()->combat()->phase()==XeenCombatPhase::PlayerReady,"Clock ABA left real PlayerReady");
            const unsigned selected=control.find("cosmetic")!=std::string::npos?2:1;
            if(++armedClockCalls==selected) {
                faultFired=true;
                if(control.find("healthy")==std::string::npos && control.find("recovery")==std::string::npos)mutateEconomy();
                std::cout<<"M40 COMBAT CLOCK CALLBACK EXECUTED "<<selected<<' '<<control<<'\n';
                if(control.find("throw")!=std::string::npos)throw std::runtime_error("M40 clock throws after equal-value ABA");
            }
        }
        return now;
    };services.observeSaveStage=[&](auto){++saveCalls;};
    services.observeGameplay=[&](auto &w,auto &,const auto &p,const auto &c,const auto &f){world=&w;party=&p;position=&c;flags=&f;};
    services.configureFlow=[&](auto &f,const auto &c) {
        original.configureFlow(f,c);flow=&f;
        f.smithBoundary=[&](XeenSmithBoundary boundary) {
            if(boundary==XeenSmithBoundary::DeparturePublished && party && party->encounterContext->day%10==1)
                ++committedInterestOperations;
            if(control=="recursive"){if(recursiveProbe)recursiveProbe();return;}
            if(faultFired || control.empty() || !party)return;
            const bool suffix=boundary==XeenSmithBoundary::DeparturePublished || boundary==XeenSmithBoundary::AfterDeparture ||
                boundary==XeenSmithBoundary::Return || boundary==XeenSmithBoundary::BeforeEventSettlement || boundary==XeenSmithBoundary::AfterEventSettlement;
            if(party->encounterContext->day!=(suffix?11:10))return;
            const auto selected=control=="fail-before-reservation"?XeenSmithBoundary::BeforeReservation:
                control=="fail-after-reservation"?XeenSmithBoundary::AfterReservation:
                control=="fail-stock-complete"?XeenSmithBoundary::StockComplete:
                control=="fail-bank-prepared"?XeenSmithBoundary::BankPrepared:
                control=="fail-departure-published"?XeenSmithBoundary::DeparturePublished:
                control=="fail-before-event-settlement"?XeenSmithBoundary::BeforeEventSettlement:
                control=="fail-after-event-settlement"?XeenSmithBoundary::AfterEventSettlement:
                control=="fail-before-admission"?XeenSmithBoundary::BeforeAdmission:
                control=="fail-after-admission" || control=="upload-admission" || control=="copy-admission"?XeenSmithBoundary::AfterAdmission:
                control=="fail-after-repair" || control=="upload-repair" || control=="copy-repair"?XeenSmithBoundary::AfterRepair:
                control=="fail-after-departure" || control=="upload-departure" || control=="copy-departure"?XeenSmithBoundary::AfterDeparture:
                control=="fail-return"?XeenSmithBoundary::Return:XeenSmithBoundary::BeforeDeparture;
            if(boundary!=selected)return;
            faultFired=true;std::cout<<"M40 SYNTHETIC FAULT CONTROL "<<control<<'\n';
            if(control.rfind("upload-",0)==0){failNativeUpload=true;return;}
            if(control.rfind("copy-",0)==0){failNativeCopy=true;return;}
            if(control.rfind("fail-",0)==0)throw std::runtime_error("M40 injected preparation/settlement failure");
            auto &p=const_cast<XeenPartyState &>(*party);
            if(control=="aba-bank-gold"){++p.serviceEconomy->bank.gold;--p.serviceEconomy->bank.gold;}
            else if(control=="aba-bank-gems"){++p.serviceEconomy->bank.gems;--p.serviceEconomy->bank.gems;}
            else if(control=="aba-stock"){p.serviceEconomy->wares[1][3][3][8].material^=1;p.serviceEconomy->wares[1][3][3][8].material^=1;}
            else if(control=="aba-presence"){const auto saved=p.serviceEconomy;p.serviceEconomy.reset();p.serviceEconomy=saved;}
            else throw std::runtime_error("M40 unknown synthetic fault control");
        };
    };
    replay_test::observeDraw=[&](auto lo,auto hi,auto value,auto cursor) {
        std::cout<<"DRAW "<<lo<<':'<<hi<<':'<<(value?std::to_string(*value):"rejected")<<':'<<cursor.state<<':'<<cursor.count<<'\n';
    };
    services.show=[&](const IndexedFrame &first,const auto &handler,const auto &escape,const auto &idle,const auto &status) {
        check(flow && world && party && position && flags && target,"M40 production owners absent");
        check(world->sessionState().journey() && party->serviceEconomy,"M40 witness content/economy differs");
        std::deque<std::function<bool()>> steps;std::optional<IndexedFrame> next;IndexedFrame::Presentation presented;
        bool shown=false,acted=false,breakArmor=false;unsigned blocks=0,iterations=0;
        const auto snapshot=[&]{return XeenSaveState::capture(original.resources.signature,*party,*position,*flags,*world);};
        const auto exact=[&](const XeenSaveSnapshot &expected) {
            const auto actual=snapshot();m40_test::equalFields(expected,actual);
            check(XeenSaveFormat::encode(expected)==XeenSaveFormat::encode(actual),"M40 unrelated durable state changed");
        };
        recursiveProbe=[&] {
            const auto providers=providerCalls,saves=saveCalls;
            const auto gold=std::uint32_t(party->monsterTreasure->gold);const auto date=*party->encounterContext;
            const auto stock=m40_test::stockBytes(*party->serviceEconomy);const auto rng=*world->sessionState().journeyRandom();
            handler.withDisplayedInput(SaveGameAction{},flow->displayedInput().value_or(0));
            handler.withDisplayedInput(AcknowledgeAction{},flow->displayedInput().value_or(0));
            handler.withDisplayedInput(CancelInteractionAction{},flow->displayedInput().value_or(0));
            check(providers==providerCalls && saves==saveCalls && gold==party->monsterTreasure->gold && date==*party->encounterContext &&
                stock==m40_test::stockBytes(*party->serviceEconomy) && rng==*world->sessionState().journeyRandom(),"M40 recursive response/save reached provider/publication");
        };
        const auto act=[&](PlayerAction a) {
            check(shown,"M40 input preceded successful native presentation");
            const auto token=handler.displayedInput();check(bool(token),"M40 input authority absent");
            next=handler.withPresentedInput(a,*token,presented);acted=true;shown=false;
            const auto providers=providerCalls,saves=saveCalls;
            handler.withDisplayedInput(SaveGameAction{},*token);handler.withDisplayedInput(a,*token);
            check(providers==providerCalls && saves==saveCalls,"M40 consumed/unpresented input reached providers/save");
            check(!flow->acceptsFrame(std::make_shared<const IndexedFrame>(flow->frame())),"M40 equal foreign frame acquired authority");
        };
        const auto action=[&](PlayerAction a) {steps.push_back([&,a]{act(a);return true;});};
        const auto inspect=[&](std::function<void()> fn){steps.push_back([fn]{fn();return true;});};
        const auto combat=[&]()->const XeenCombat *{return flow->encounter()->combat();};
        const auto settle=[&]{steps.push_back([&] {
            if(flow->canSave())return true;
            if(const auto c=combat()) {
                check(c->phase()!=XeenCombatPhase::Failed && c->phase()!=XeenCombatPhase::Defeat && c->phase()!=XeenCombatPhase::SupportStopped,"M40 combat support stop");
                if(const auto casting=c->cast()){if(casting->phase==XeenCombatCastPhase::Result)act(AcknowledgeAction{});}
                else if(c->phase()==XeenCombatPhase::PlayerReady) {
                    if(breakArmor && !(party->roster.at(6).armor[0].state&128)){++blocks;act(BlockAction{});}else act(AttackAction{});
                }
            } else if(world->sessionState().journeyActivity()==XeenJourneyActivity::Event || world->sessionState().journeyActivity()==XeenJourneyActivity::Reward)act(AcknowledgeAction{});
            check(flow->encounter()->state().phase()!=XeenEncounterPhase::SupportStopped,"M40 exploration/service support stop");
            return false;
        });};
        const auto route=[&](const std::string &path,bool drainLast=true) {
            for(unsigned i=0;i<path.size();++i) {
                switch(path[i]) {
                    case 'U':action(NavigationAction::MoveForward);break;case 'D':action(NavigationAction::MoveBackward);break;
                    case 'L':action(NavigationAction::TurnLeft);break;case 'R':action(NavigationAction::TurnRight);break;case 'F':action(ShootAction{});break;
                }
                if(drainLast || i+1<path.size())settle();
            }
        };
        const auto deny=[&]{inspect([&] {
            check(!flow->canSave(),"M40 service/casting exposed Quiet");const auto providers=providerCalls,saves=saveCalls;
            const auto prior=fs::exists(*target)?XeenSaveFormat::encode(XeenSaveFile::read(*target)):std::vector<std::uint8_t>{};
            handler.withDisplayedInput(SaveGameAction{},*handler.displayedInput());
            check(providers==providerCalls && saves==saveCalls,"M40 modal F9 reached provider/path/I/O");
            if(!prior.empty())check(prior==XeenSaveFormat::encode(XeenSaveFile::read(*target)),"M40 denied F9 changed disk");
        });};
        const auto checkpoint=[&](const std::string &label) {
            auto before=std::make_shared<unsigned>();
            inspect([&,before]{check(flow->canSave(),"M40 checkpoint not Quiet");*before=saveCalls;SDL_Event e{};e.type=SDL_KEYDOWN;e.key.keysym.sym=SDLK_F9;e.key.keysym.scancode=SDL_SCANCODE_F9;e.key.timestamp=SDL_GetTicks()+1;check(SDL_PushEvent(&e)==1,"M40 native F9 enqueue");acted=true;});
            steps.push_back([&,before,label] {
                if(saveCalls==*before)return false;
                check(saveCalls==*before+3,"M40 native F9 did not complete normal pipeline");
                SDL_Event e{};e.type=SDL_KEYUP;e.key.keysym.sym=SDLK_F9;e.key.keysym.scancode=SDL_SCANCODE_F9;SDL_PushEvent(&e);
                fs::copy_file(*target,target->parent_path()/(target->stem().string()+"-"+label+".mmsave"),fs::copy_options::overwrite_existing);
                std::cout<<"M40 CHECKPOINT "<<label<<" day="<<party->encounterContext->day<<" minute="<<party->encounterContext->minutes<<" ctr="<<party->encounterContext->ctr24
                    <<" gold="<<party->monsterTreasure->gold<<" rng="<<world->sessionState().journeyRandom()->state<<':'<<world->sessionState().journeyRandom()->count<<'\n';
                return true;
            });
        };
        const auto expectDate=[&](unsigned day,unsigned minute,unsigned ctr,std::uint32_t state,std::uint64_t count,std::uint32_t gold) {
            check(party->encounterContext->year==610 && party->encounterContext->day==day && party->encounterContext->minutes==minute && party->encounterContext->ctr24==ctr &&
                world->sessionState().journeyRandom()->state==state && world->sessionState().journeyRandom()->count==count && party->monsterTreasure->gold==gold,"M40 literal checkpoint date/purse/RNG differs");
        };
        const auto dismissServiceNotice=[&] {steps.push_back([&] {
            const auto context=handler.inputContext(presented);
            if(context.dialog && context.dialog->anyKey)act(AcknowledgeAction{});
            return true;
        });};
        const auto waitService=[&] {
            steps.push_back([&] {
                if(XeenTrainingTestAccess::admitted(*flow))return true;
                if(flow->canSave() && faultFired && !retriedPreparation &&
                    (control=="fail-before-reservation" || control=="fail-after-reservation" || control=="fail-stock-complete" || control=="fail-bank-prepared")) {
                    retriedPreparation=true;act(InteractionAction{});return false;
                }
                check(!flow->canSave(),"M40 detached stock preparation exposed Quiet");
                const auto providers=providerCalls,saves=saveCalls;
                handler.withDisplayedInput(SaveGameAction{},handler.displayedInput().value_or(0));
                check(providers==providerCalls && saves==saveCalls,"M40 preparation F9 reached capture/provider/path/I/O");
                return false;
            });
            dismissServiceNotice();
        };
        const auto emptyVisit=[&] {
            auto before=std::make_shared<XeenSaveSnapshot>();inspect([&,before]{*before=snapshot();});
            action(InteractionAction{});waitService();deny();action(CancelInteractionAction{});settle();
            inspect([&,before]{++before->journey->context->day;exact(*before);});
        };
        const auto repair=[&](unsigned slot,std::uint32_t goldAfter) {
            auto before=std::make_shared<XeenSaveSnapshot>();auto ac=std::make_shared<int>();
            inspect([&,before,ac]{*before=snapshot();*ac=XeenCharacterRules::combatArmorClass(party->roster.at(6),*party->roster.combatInputs(6),{610});});
            action(InteractionAction{});
            if(control=="fail-before-admission" && slot==0)action(InteractionAction{});
            waitService();
            deny();action(SelectMemberAction{5});action(DialogKeyAction{'b'});action(DialogKeyAction{'a'});action(DialogKeyAction{'f'});deny();
            action(SelectInventorySlotAction{slot});deny();action(YesAction{});
            if(control=="fail-before-repair" && slot==0)action(YesAction{});
            dismissServiceNotice();
            inspect([&,slot,goldAfter,ac] {
                check(party->monsterTreasure->gold==goldAfter && party->roster.at(6).armor[slot].state==0 &&
                    XeenCharacterRules::combatArmorClass(party->roster.at(6),*party->roster.combatInputs(6),{610})==*ac+(slot?1:2),"M40 real repair payment/item/AC differs");
                if(control=="fail-after-repair" && slot==0)check(party->encounterContext->day==10,"M40 later failure refunded/settled repair");
            });
            deny();action(CancelInteractionAction{});action(CancelInteractionAction{});
            if(slot==0 && (control=="fail-before-departure" || control=="fail-after-departure" || control=="fail-return" ||
                control=="fail-departure-published" || control=="fail-before-event-settlement" || control=="fail-after-event-settlement"))action(AcknowledgeAction{});
            settle();
            inspect([&,before,slot,goldAfter] {
                before->characters[6].armor[slot].state=0;before->journey->treasure->gold=goldAfter;++before->journey->context->day;
                if(slot==0){before->journey->serviceEconomy=snapshot().journey->serviceEconomy;before->journey->random=XeenJourneyRandomState{1,3686439625u,2109};}
                exact(*before);
            });
        };
        inspect([&] {
            if(resume) {
                check(!replay_test::journeyInitializations && !replay_test::journeyConstructions && !replay_test::actions && !replay_test::pulses && !replay_test::commands &&
                    !replay_test::draws && !replay_test::retirements && !replay_test::constructions && !replay_test::services && !replay_test::preparations &&
                    !replay_test::timePreparations && !replay_test::eventExecutions && !replay_test::transfers && !replay_test::equipmentChanges && !interestOperations && !bankInputCalls,"M40 pre-input restore replayed initialization/time/service/spell/RNG");
                exact(XeenSaveFile::read(*target));std::cout<<"M40 RESTORE EXACT BEFORE INPUT\n";
            }else {
                check(seed==3626689381u,"M40 witness requires normal fresh chosen seed");
                expectDate(8,480,0,7,886,800);
                check(party->monsterTreasure->gems==10 && party->serviceEconomy->bank.gold==0 && party->serviceEconomy->bank.gems==0 && !interestOperations && bankInputCalls==1 &&
                    m40_test::sha256(m40_test::stockBytes(*party->serviceEconomy))=="39cbe3234d1701fc7859afbb31a5e48f7d41407c75b4fa2364f3ee87c9143b18","M40 fresh economy literal oracle differs");
                std::cout<<"M40 FRESH GENERATED ECONOMY EXACT\n";
            }
        });
        if(!resume) {
            route("UFUDD");inspect([&]{check(party->monsterTreasure->gold==810 && world->sessionState().actors().at(9).lifecycle==XeenActorLifecycle::Defeated,"M40 earned Orc gold absent");});
            route("LLULUU");action(InteractionAction{});action(YesAction{});settle();
            inspect([&]{breakArmor=true;});route("URULUUULUUU");
            inspect([&]{breakArmor=false;check(blocks==39 && party->roster.at(6).currentHp==-11 && party->roster.at(6).armor[0].state==128 &&
                party->roster.at(6).armor[1].state==128 && party->encounterContext->minutes==577,"M40 natural armor-break input route differs");});
            for(unsigned i=0;i<2;++i){action(CastSpellAction{});action(SelectMemberAction{4});action(NavigationAction::MoveBackward);action(AcknowledgeAction{});action(AcknowledgeAction{});action(SelectMemberAction{5});settle();}
            route("UUUUU");inspect([&]{expectDate(8,584,2,2732157854u,1203,810);check(party->roster.at(6).currentHp==1 && party->roster.at(1).currentSp==19,"M40 genuine First Aid preparation differs");});checkpoint("A");
        }
        if(branch=="empty") {
            const auto generatingEmpty=[&](bool firstGeneration) {
                auto before=std::make_shared<XeenSaveSnapshot>();auto operations=std::make_shared<unsigned>();auto publications=std::make_shared<unsigned>();
                inspect([&,before,operations,publications]{*before=snapshot();*operations=interestOperations;*publications=committedInterestOperations;});
                action(InteractionAction{});waitService();deny();
                inspect([&,before] {
                    // Capture remains correctly forbidden while Service owns
                    // the graph. Compare borrowed durable fields directly.
                    const auto &saved=*before->journey;
                    check(*party->encounterContext==*saved.context && *world->sessionState().journeyRandom()==*saved.random &&
                        *party->serviceEconomy==*saved.serviceEconomy && *party->monsterTreasure==*saved.treasure &&
                        *party->regionalRecovery==*saved.regionalRecovery && party->party.activeRosterIds()==before->activeRosterIds &&
                        position->mapId==before->camera.mapId && position->x==before->camera.x && position->y==before->camera.y &&
                        position->direction==before->camera.direction,"M40 detached preparation changed durable party/date/economy/RNG/camera");
                    for(unsigned owner=0;owner<30;++owner)check(xeen_state::sameCharacter(party->roster.at(owner),before->characters[owner]) &&
                        xeen_state::sameInputs(*party->roster.combatInputs(owner),saved.supplements[owner].inputs),"M40 preparation changed a roster owner/book/raw item/supplement");
                    const auto actors=[&](const auto &live,const auto &expected) {
                        check(live.size()==expected.size(),"M40 preparation changed region actor count");
                        for(unsigned i=0;i<live.size();++i){const auto &a=live.at(i),&b=expected[i];
                            check(a.id==b.id && a.x==b.x && a.y==b.y && a.hp==b.hp && a.activated==b.activated && a.lifecycle==b.lifecycle &&
                                a.status==b.status && bool(world->sessionState().accountedMonsters().count(a.id))==b.accounted,"M40 preparation changed actor durable fields");}
                    };
                    actors(world->sessionState().actors(),saved.actors);actors(world->sessionState().regionalActors(28),*saved.vertigoActors);
                    check(party->questItems.counts()==before->questItems && party->questFlags.values()==before->questFlags && flags->values()==before->gameFlags,"M40 preparation changed quests/flags");
                    const auto events=world->sessionState().disabledEvents();const auto objects=world->sessionState().disabledObjects();
                    check(std::vector<XeenEventIdentity>(events.begin(),events.end())==before->disabledEvents &&
                        std::vector<XeenObjectIdentity>(objects.begin(),objects.end())==before->disabledObjects,"M40 preparation changed overlays");
                });
                action(CancelInteractionAction{});settle();
                inspect([&,before,operations,publications,firstGeneration] {
                    const auto after=snapshot();check(interestOperations==*operations+1 && committedInterestOperations==*publications+1,"M40 empty generating service omitted/repeated interest");
                    check(after.journey->random->count>before->journey->random->count &&
                        m40_test::stockBytes(*after.journey->serviceEconomy)!=m40_test::stockBytes(*before->journey->serviceEconomy),"M40 later trigger did not regenerate stock/RNG");
                    ++before->journey->context->day;before->journey->random=after.journey->random;before->journey->serviceEconomy=after.journey->serviceEconomy;
                    exact(*before);
                    if(firstGeneration) {expectDate(11,584,2,3686439625u,2109,810);check(m40_test::sha256(m40_test::stockBytes(*party->serviceEconomy))==
                        "b2d744b92079134a10dc16c73ef4ec90e738a5b7d46b8e90229c5f240b9d6cc9","M40 empty10->11 stock literal oracle differs");}
                    check(party->roster.at(6).armor[0].state==128 && party->roster.at(6).armor[1].state==128 && party->monsterTreasure->gold==810,
                        "M40 zero-transaction service repaired/debited carried purse");
                });
            };
            if(stage!="empty11" && stage!="empty20" && stage!="empty21") {emptyVisit();emptyVisit();generatingEmpty(true);checkpoint("empty11");}
            if(stage!="empty20" && stage!="empty21") {for(unsigned i=0;i<9;++i)emptyVisit();checkpoint("empty20");}
            if(stage!="empty21") {generatingEmpty(false);checkpoint("empty21");}
            route("LR");checkpoint("continued");
        } else {
        if(!resume || stage=="A") {emptyVisit();emptyVisit();repair(0,808);
            inspect([&]{expectDate(11,584,2,3686439625u,2109,808);check(interestOperations==(control=="fail-bank-prepared"?2u:1u) && committedInterestOperations==1 &&
                m40_test::sha256(m40_test::stockBytes(*party->serviceEconomy))=="b2d744b92079134a10dc16c73ef4ec90e738a5b7d46b8e90229c5f240b9d6cc9" &&
                party->serviceEconomy->bank.gold==0 && party->serviceEconomy->bank.gems==0 && party->roster.at(6).armor[1].state==128,"M40 10->11 restock/interest literal oracle differs");
                });checkpoint("B");}
        if(!resume || stage=="A" || stage=="B") {route("DDDDDDDUUUUUUU");inspect([&]{expectDate(11,598,16,3686439625u,2109,808);});checkpoint("C");}
        if(!resume || stage=="A" || stage=="B" || stage=="C") {repair(1,807);inspect([&]{expectDate(12,598,16,3686439625u,2109,807);});checkpoint("D");}
        if(stage!="E") {
            for(unsigned i=0;i<4;++i){action(CastSpellAction{});action(SelectMemberAction{4});action(NavigationAction::MoveBackward);action(AcknowledgeAction{});action(AcknowledgeAction{});action(SelectMemberAction{i<3?5u:4u});settle();}
            inspect([&]{expectDate(12,602,16,3686439625u,2109,807);check(party->roster.at(1).currentSp==15 && party->roster.at(6).currentSp==27,"M40 post-service exploration First Aid differs");});
            route("DDDDDDDLUUUU");action(InteractionAction{});action(YesAction{});settle();route("LLU");action(InteractionAction{});action(YesAction{});settle();route("URULU",false);
            if(branch=="clock") steps.push_back([&] {
                if(clockArmed && faultFired)return true;
                if(const auto c=combat();c && c->phase()==XeenCombatPhase::PlayerReady) {
                    check(resume && stage=="D" && party->encounterContext->day==12,"Clock control lacks genuine post-service/full-exit/restore domain");
                    clockArmed=true;std::cout<<"M40 POST-SERVICE RESTORED PLAYERREADY CLOCK ARMED\n";return false;
                }
                if(!combat() && (world->sessionState().journeyActivity()==XeenJourneyActivity::Event || world->sessionState().journeyActivity()==XeenJourneyActivity::Reward))act(AcknowledgeAction{});
                return false;
            });
            auto rebeccaDone=std::make_shared<bool>(false);auto seymourCasts=std::make_shared<unsigned>(0);
            auto castPhase=std::make_shared<unsigned>(0);auto enemySelected=std::make_shared<bool>(false);
            steps.push_back([&,rebeccaDone,seymourCasts,castPhase,enemySelected] {
                if(flow->canSave()) {check(*rebeccaDone && *seymourCasts==2,"M40 reset Slime settled before three combat spells");return true;}
                const auto c=combat();
                if(!c) {
                    check(flow->encounter()->state().phase()!=XeenEncounterPhase::SupportStopped,"M40 reset Slime approach stopped");
                    if(world->sessionState().journeyActivity()==XeenJourneyActivity::Event || world->sessionState().journeyActivity()==XeenJourneyActivity::Reward)act(AcknowledgeAction{});
                    return false;
                }
                check(c->phase()!=XeenCombatPhase::Failed && c->phase()!=XeenCombatPhase::Defeat && c->phase()!=XeenCombatPhase::SupportStopped,"M40 reset Slime unavailable");
                if(const auto cast=c->cast()) {
                    switch(cast->phase) {
                        case XeenCombatCastPhase::Learned:
                            if(cast->slot==(*castPhase==1?14u:*castPhase==2?0u:25u))act(AcknowledgeAction{});else act(NavigationAction::MoveBackward);break;
                        case XeenCombatCastPhase::Enemy:
                            if(!*enemySelected){*enemySelected=true;act(SelectCombatTargetAction{0});}else act(AcknowledgeAction{});break;
                        case XeenCombatCastPhase::Confirm:act(AcknowledgeAction{});break;
                        case XeenCombatCastPhase::PartyTarget:act(SelectMemberAction{5});break;
                        case XeenCombatCastPhase::Result:
                            if(*castPhase==3)check(c->result().actorHpBefore==2 && c->result().actorHpAfter==0,"M40 Slime Arrow was not genuine 2->0 lethal");
                            act(AcknowledgeAction{});*castPhase=0;break;
                        default:break;
                    }
                }else if(c->phase()==XeenCombatPhase::PlayerReady) {
                    if(c->participant()==4 && !*rebeccaDone){*rebeccaDone=true;*castPhase=1;act(CastSpellAction{});}
                    else if(c->participant()==5 && *seymourCasts<2){*castPhase=++*seymourCasts==1?2:3;act(CastSpellAction{});}
                    else act(BlockAction{});
                }
                return false;
            });
            inspect([&] {
                expectDate(12,628,12,2018868320u,2180,807);
                check(position->mapId==XeenMapIdentity(28) && position->x==16 && position->y==2 && position->direction==XeenDirection::North &&
                    party->roster.at(1).currentSp==14 && party->roster.at(6).currentSp==24 && world->sessionState().regionalActors(28).size()==52 &&
                    world->sessionState().regionalActors(28).at(36).lifecycle==XeenActorLifecycle::Defeated && world->sessionState().disabledEvents().count({28,764}),"M40 E actor/reset/SP/camera differs");
                const int hp[]{10,11,4,29,3,13};const unsigned owners[]{0,18,14,11,1,6};
                for(unsigned i=0;i<6;++i){check(party->roster.at(owners[i]).currentHp==hp[i],"M40 E survival HP differs");for(const auto &condition:party->roster.at(owners[i]).conditions)check(condition==0,"M40 E condition differs");}
                check(m40_test::sha256(m40_test::stockBytes(*party->serviceEconomy))=="b2d744b92079134a10dc16c73ef4ec90e738a5b7d46b8e90229c5f240b9d6cc9" && party->serviceEconomy->bank.gold==0 && party->serviceEconomy->bank.gems==0,"M40 E retained economy differs");
            });checkpoint("E");
        }
        route("LR");checkpoint("continued");
        }
        inspect([&]{check(control.empty() || faultFired || control=="recursive","M40 requested synthetic control did not execute");
            std::cout<<(!control.empty()?"M40 SYNTHETIC FAULT CONTINUATION PASSED":branch=="empty"?"M40 EMPTY DEPARTURE WITNESS PASSED":"M40 PRODUCTION WITNESS PASSED")<<'\n';SDL_Event e{};e.type=SDL_QUIT;SDL_PushEvent(&e);});
        auto native=handler;native.closed={};native.beginCycle=[&](std::uint64_t){handler.beginCycle(++cycle);};
        native.framePresented=[&](const auto &frame){check(!nativeFailed,"M40 failed native frame gained input authority");handler.framePresented(frame);presented=frame;shown=true;};
        const auto verifyClockFailure=[&] {
            check(faultFired && combat() && combat()->phase()==XeenCombatPhase::Failed &&
                combat()->result().failure==XeenCombatFailure::Integrity && world->sessionState().journeyActivity()==XeenJourneyActivity::Failed &&
                !flow->canSave(),"Combat callback ABA did not permanently poison gameplay/save authority");
            const auto economy=*party->serviceEconomy;const auto random=*world->sessionState().journeyRandom();
            const auto date=*party->encounterContext;const auto prior=m40_test::diskBytes(*target);
            const auto providers=providerCalls,saves=saveCalls;const auto commands=replay_test::commands;
            unsigned compositions=0;
            for(unsigned retry=0;retry<3;++retry) {
                try{flow->refresh(true);}catch(...){}
                try{const_cast<XeenCombat *>(combat())->preparePresentation(combat()->ticket(),[&]{++compositions;});}catch(...){}
                for(PlayerAction a:{PlayerAction{AttackAction{}},PlayerAction{BlockAction{}},PlayerAction{CastSpellAction{}},PlayerAction{SaveGameAction{}}})
                    try{handler.withDisplayedInput(a,handler.displayedInput().value_or(0));}catch(...){}
                try{idle();}catch(...){}
                check(!flow->canSave() && combat()->phase()==XeenCombatPhase::Failed && combat()->result().failure==XeenCombatFailure::Integrity &&
                    world->sessionState().journeyActivity()==XeenJourneyActivity::Failed,"Retry/guard replacement cleansed combat integrity failure");
            }
            check(!compositions && providers==providerCalls && saves==saveCalls && commands==replay_test::commands &&
                economy==*party->serviceEconomy && random==*world->sessionState().journeyRandom() && date==*party->encounterContext &&
                prior==m40_test::diskBytes(*target),"Poisoned combat continued gameplay, reached providers/save, or changed previous disk");
            clockVerified=true;std::cout<<"M40 COMBAT CLOCK ABA MONOTONIC / NO QUIET / F9 DISK PRESERVED / RETRY REJECTED\n";
            steps.clear();SDL_Event e{};e.type=SDL_QUIT;SDL_PushEvent(&e);
        };
        const auto drive=[&]()->std::optional<IndexedFrame> {
            check(++iterations<30000,"M40 witness iteration bound");now+=100;next.reset();acted=false;
            if(shown)while(!steps.empty()){const bool done=steps.front()();if(done)steps.pop_front();if(acted || !done)break;}
            if(acted)return next;
            try {auto frame=idle();if(frame)shown=false;return frame;}
            catch(...) {
                if(branch!="clock" || !faultFired || control.find("healthy")!=std::string::npos || control.find("recovery")!=std::string::npos)throw;
                verifyClockFailure();return std::nullopt;
            }
        };
        const auto ok=original.show(first,native,escape,drive,status);
        if(branch=="clock") {
            if(control.find("healthy")!=std::string::npos || control.find("recovery")!=std::string::npos) {
                check(ok && faultFired && (control.find("recovery")==std::string::npos || recoveryFired),"Healthy clock/presentation recovery did not continue");
                std::cout<<"M40 COMBAT CLOCK HEALTHY CONTINUATION PASSED\n";
            } else {check(clockVerified,"Clock ABA did not execute permanent-failure assertions");return false;}
        }
        if(control.rfind("upload-",0)==0 || control.rfind("copy-",0)==0) {
            check(faultFired && nativeFailed && !ok && !flow->canSave(),"M40 native failure exposed Quiet");
            check(party->monsterTreasure->gold==(control.find("admission")!=std::string::npos?810u:808u) &&
                party->encounterContext->day==(control.find("departure")!=std::string::npos?11:10),"M40 native failure refunded/replayed repair/departure prefix");
            if(control.find("departure")!=std::string::npos)check(world->sessionState().journeyRandom()->count==2109 &&
                m40_test::sha256(m40_test::stockBytes(*party->serviceEconomy))=="b2d744b92079134a10dc16c73ef4ec90e738a5b7d46b8e90229c5f240b9d6cc9","M40 native failure repeated/dropped stock/RNG prefix");
            const auto providers=providerCalls,saves=saveCalls;handler.withDisplayedInput(SaveGameAction{},handler.displayedInput().value_or(0));
            check(providers==providerCalls && saves==saveCalls,"M40 failed native-state F9 reached provider/path/I/O");
            std::cout<<"M40 NATIVE FAILURE PRESERVATION PASSED\n";
        }
        return ok;
    };
    return realPlay(app,services,camera,target,resume,entry,seed);
}
