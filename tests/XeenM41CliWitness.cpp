// Earned original-resource witness: only presented-frame gameplay input changes
// owners. Synthetic fault and arithmetic fixtures live in separate test targets.
#include "XeenDecisionNine.h"
#include "XeenProbeFired.h"
#include "app/Application.h"
#include "app/XeenGameplayServices.h"
#include "platform/XeenSaveFile.h"
#include "formats/xeen/XeenSaveFormat.h"
#include "games/xeen/XeenCharacterRules.h"
#include "games/xeen/XeenTraining.h"
#include "XeenRestoreReplayProbe.h"
#include "XeenM40Evidence.h"
#include "XeenM42Evidence.h"
#include "XeenM49CombatSupport.h"
#define SDL_MAIN_HANDLED
#include <SDL.h>
#include <deque>
#include <cstdlib>
#include <iostream>
using namespace mmodern;
namespace fs=std::filesystem;
using m40_test::check;
namespace {unsigned interestCalls=0;bool failUpload=false,failCopy=false,nativeFailed=false;}
extern "C" int __real_SDL_UpdateTexture(SDL_Texture *,const SDL_Rect *,const void *,int);
extern "C" int __wrap_SDL_UpdateTexture(SDL_Texture *texture,const SDL_Rect *rect,const void *pixels,int pitch) {probe_fired::hit("SDL_UpdateTexture");
    if(failUpload){failUpload=false;nativeFailed=true;return SDL_SetError("M41 injected upload failure");}
    return __real_SDL_UpdateTexture(texture,rect,pixels,pitch);
}
extern "C" int __real_SDL_RenderCopy(SDL_Renderer *,SDL_Texture *,const SDL_Rect *,const SDL_Rect *);
extern "C" int __wrap_SDL_RenderCopy(SDL_Renderer *renderer,SDL_Texture *texture,const SDL_Rect *source,const SDL_Rect *destination) {probe_fired::hit("SDL_RenderCopy");
    if(failCopy){failCopy=false;nativeFailed=true;return SDL_SetError("M41 injected render failure");}
    return __real_SDL_RenderCopy(renderer,texture,source,destination);
}
#define INTEREST_SYMBOL "_ZN7mmodern23xeenPrepareBankInterestERKNS_16XeenBankBalancesE"
XeenBankBalances realInterest(const XeenBankBalances &) asm("__real_" INTEREST_SYMBOL);
XeenBankBalances wrappedInterest(const XeenBankBalances &) asm("__wrap_" INTEREST_SYMBOL);
XeenBankBalances wrappedInterest(const XeenBankBalances &bank) {probe_fired::hit("xeenPrepareBankInterest");
    ++interestCalls;std::cout<<"INTEREST "<<bank.gold<<':'<<bank.gems<<'\n';return realInterest(bank);
}
#define PLAY_SYMBOL "_ZNK7mmodern11Application12playGameplayERKNS_20XeenGameplayServicesENS_10XeenCameraERKSt8optionalINSt10filesystem7__cxx114pathEEbNS_18XeenEncounterEntryES5_IjE"
extern "C" int realPlay(const Application *,const XeenGameplayServices &,XeenCamera,const std::optional<fs::path> &,bool,XeenEncounterEntry,std::optional<std::uint32_t>) asm("__real_" PLAY_SYMBOL);
extern "C" int wrappedPlay(const Application *,const XeenGameplayServices &,XeenCamera,const std::optional<fs::path> &,bool,XeenEncounterEntry,std::optional<std::uint32_t>) asm("__wrap_" PLAY_SYMBOL);
static const probe_fired::Expect playProbe{"Application::playGameplay","SDL_RenderCopy","SDL_UpdateTexture"};
extern "C" int wrappedPlay(const Application *app,const XeenGameplayServices &original,XeenCamera camera,
    const std::optional<fs::path> &target,bool resume,XeenEncounterEntry entry,std::optional<std::uint32_t> seed) {probe_fired::hit("Application::playGameplay");if(!resume)for(const char *probe:{"XEEN_REPLAY_COMMAND","XEEN_REPLAY_DRAW","XEEN_REPLAY_JOURNEY_CONSTRUCT","XEEN_REPLAY_REGIONAL_MOVE","XEEN_REPLAY_SERVICE","XEEN_REPLAY_TIME","XEEN_REPLAY_RETIRE","XEEN_REPLAY_INDOOR_MOVE","XEEN_REPLAY_EVENT_BEGIN","XEEN_REPLAY_FRESH_PUBLICATION_INITIALIZE","xeenPrepareBankInterest"})probe_fired::expect(probe);
    const char *stageEnv=std::getenv("MMODERN_M41_STAGE");const std::string stage=stageEnv?stageEnv:"fresh";
    const char *controlEnv=std::getenv("MMODERN_M41_CONTROL");const std::string control=controlEnv?controlEnv:"";
    auto services=original;XeenEventFlow *flow=nullptr;XeenWorld *world=nullptr;
    const XeenPartyState *party=nullptr;const XeenCamera *position=nullptr;const XeenGameFlags *flags=nullptr;
    unsigned providers=0,saves=0;std::uint64_t now=0,cycle=0;
    bool observeService=false;std::string serviceDraws;
    struct ServiceDrawTime {std::uint64_t now,sampleEnd;};
    std::vector<ServiceDrawTime> serviceDrawTimes;
    std::uint64_t sampleEnd=0;
    bool clockArmed=false,clockFault=false,clockVerified=false;unsigned clockCalls=0;
    const bool clockCase=control.rfind("clock-",0)==0 || control.rfind("presentation-",0)==0;
    const auto mutateEconomy=[&] {
        auto &p=const_cast<XeenPartyState &>(*party);const auto before=*p.serviceEconomy;
        if(control.find("bank-gold")!=std::string::npos){++p.serviceEconomy->bank.gold;--p.serviceEconomy->bank.gold;}
        else if(control.find("bank-gems")!=std::string::npos){++p.serviceEconomy->bank.gems;--p.serviceEconomy->bank.gems;}
        else if(control.find("stock")!=std::string::npos){p.serviceEconomy->wares[1][3][3][8].material^=1;p.serviceEconomy->wares[1][3][3][8].material^=1;}
        else {p.serviceEconomy.reset();p.serviceEconomy=before;}
        check(*p.serviceEconomy==before,"M41 clock probe changed final economy values");
    };
    services.clock=[&]{
        if(clockArmed && !clockFault && control.rfind("clock-",0)==0 && ++clockCalls==(control.find("cosmetic")!=std::string::npos?2u:1u)) {
            clockFault=true;mutateEconomy();if(control.find("throw")!=std::string::npos)throw std::runtime_error("M41 clock exceptional ABA");
        }
        return now;
    };services.observeSaveStage=[&](auto){++saves;};
    const auto count=[&](auto fn){return [fn=std::move(fn),&providers](auto &&...args)->decltype(auto){++providers;return fn(std::forward<decltype(args)>(args)...);};};
#define M41_COUNT(field) if(services.field)services.field=count(services.field)
    M41_COUNT(resources.loadInitialParty);M41_COUNT(resources.loadInitialCharacters);M41_COUNT(resources.loadInitialContext);
    M41_COUNT(resources.loadEvents);M41_COUNT(resources.loadMonsterStatistics);M41_COUNT(resources.regionalManifest);
    M41_COUNT(resources.loadInitialPurse);M41_COUNT(resources.loadInitialWorldFlags);M41_COUNT(resources.loadRegionalText);M41_COUNT(resources.loadLearnedSpellNames);
    M41_COUNT(resources.loadInitialBankBalances);M41_COUNT(maps);M41_COUNT(objects);M41_COUNT(texts);M41_COUNT(compose);M41_COUNT(npcDraw);
    M41_COUNT(validateEncounterSprite);M41_COUNT(validateCombatSprite);M41_COUNT(sampleJourneySeed);
#undef M41_COUNT
    services.observeGameplay=[&](auto &w,auto &,const auto &p,const auto &c,const auto &f){world=&w;party=&p;position=&c;flags=&f;};
    const auto compose=services.composeEncounter;
    services.composeEncounter=[&](auto &w,const auto &p,const auto &c,auto phase,auto actor){++providers;
        if(clockArmed && !clockFault && control.rfind("presentation-",0)==0){clockFault=true;mutateEconomy();}
        return compose(w,p,c,phase,actor);};
    services.configureFlow=[&](auto &f,const auto &c) {
        original.configureFlow(f,c);flow=&f;
        const auto sample=f.loadSampleDuration;
        f.loadSampleDuration=[&,sample](const char *name,XeenSceneArchive archive) {
            const auto duration=sample(name,archive);
            sampleEnd=PresentationClock::checkedAdd(PresentationClock::fromMilliseconds(now),duration);
            return duration;
        };
        f.drawTrainingArt=count(f.drawTrainingArt);
        f.trainingBoundary=[&](XeenTrainingBoundary boundary) {
            if(!nativeFailed && ((boundary==XeenTrainingBoundary::AfterAdmission && control.find("admission")!=std::string::npos) ||
                (boundary==XeenTrainingBoundary::LevelPublished && control.find("level")!=std::string::npos) ||
                (boundary==XeenTrainingBoundary::DeparturePublished && control.find("departure")!=std::string::npos))) {
                failUpload=control.rfind("upload-",0)==0;failCopy=control.rfind("copy-",0)==0;
            }
            if(boundary==XeenTrainingBoundary::LevelPublished || boundary==XeenTrainingBoundary::DeparturePublished)
                std::cout<<"DAY "<<(boundary==XeenTrainingBoundary::LevelPublished?"level":"departure")<<' '<<party->encounterContext->day<<' '
                    <<world->sessionState().journeyRandom()->state<<':'<<world->sessionState().journeyRandom()->count<<'\n';
        };
    };
    replay_test::observeDraw=[&](auto lo,auto hi,auto value,auto cursor){
        const auto line="DRAW "+std::to_string(lo)+":"+std::to_string(hi)+":"+
            (value?std::to_string(*value):"rejected")+":"+std::to_string(cursor.state)+":"+std::to_string(cursor.count)+"\n";
        std::cout<<line;
        if(observeService){serviceDraws+=line;serviceDrawTimes.push_back({PresentationClock::fromMilliseconds(now),sampleEnd});}
    };
    services.show=[&](const IndexedFrame &first,const auto &handler,const auto &escape,const auto &idle,const auto &status) {
        check(flow && world && party && position && flags && target,"M41 production owners absent");
        check(world->sessionState().journey(),"M41 content not selected");
        std::deque<std::function<bool()>> steps;std::optional<IndexedFrame> next;bool shown=false,acted=false;
        IndexedFrame::Presentation presented;
        unsigned iterations=0;bool cityFight=false;
        const auto snapshot=[&]{return XeenSaveState::capture(original.resources.signature,*party,*position,*flags,*world);};
        const auto act=[&](PlayerAction action) {
            check(shown,"M41 input preceded presentation");const auto token=handler.displayedInput();check(bool(token),"M41 missing frame token");
            const auto origin=presented;
            std::cout<<"INPUT "<<action.index()<<'\n';next=handler.withPresentedInput(action,*token,origin);acted=true;shown=false;
            const auto beforeProviders=providers,beforeSaves=saves;
            handler.withDisplayedInput(SaveGameAction{},*token);
            check(beforeProviders==providers && beforeSaves==saves,"M41 denied F9 reached providers/save");
            if(world->sessionState().journeyActivity()==XeenJourneyActivity::Service) {
                handler.withPresentedInput(action,*token,origin);
                check(beforeProviders==providers && beforeSaves==saves,"M41 stale/batched Training input reached providers/save");
            }
        };
        const auto action=[&](PlayerAction a,bool useNative=false){
            if(!useNative){steps.push_back([&,a]{act(a);return true;});return;}
            auto phase=std::make_shared<unsigned>(0);auto token=std::make_shared<std::uint64_t>();
            steps.push_back([&,a,phase,token] {
                const auto *member=std::get_if<SelectMemberAction>(&a);
                const auto key=member?SDLK_F1+int(member->partyIndex):SDLK_RETURN;
                const auto other=key==SDLK_RETURN?SDLK_ESCAPE:SDLK_RETURN;
                const auto send=[&](SDL_Keycode k,bool down,bool repeat=false){SDL_Event e{};e.type=down?SDL_KEYDOWN:SDL_KEYUP;
                    e.key.keysym.sym=k;e.key.keysym.scancode=SDL_GetScancodeFromKey(k);e.key.timestamp=SDL_GetTicks()+1;e.key.repeat=repeat;
                    check(SDL_PushEvent(&e)==1,"M41 native input enqueue failed");
                    if(down)acted=true; // Keep the acquired origin until SDL samples this batch.
                };
                if(!*phase){*token=*handler.displayedInput();std::cout<<"INPUT "<<a.index()<<'\n';
                    send(key,true);send(key,true);send(key,true,true);send(other,true);++*phase;return false;}
                if(*phase==1){if(*handler.displayedInput()==*token)return false;
                    check(*handler.displayedInput()==*token+1,"M41 native batch crossed multiple service phases");
                    *token=*handler.displayedInput();send(key,true);send(key,true,true);
                    for(auto kind:{SDL_WINDOWEVENT_SIZE_CHANGED,SDL_WINDOWEVENT_EXPOSED}){SDL_Event e{};e.type=SDL_WINDOWEVENT;
                        e.window.event=kind;e.window.data1=640;e.window.data2=400;SDL_PushEvent(&e);}++*phase;return false;}
                check(*handler.displayedInput()==*token,"M41 held key/expose crossed fresh service phase");
                if(*phase==2) {
                    send(key,false);send(other,false);SDL_Event stale{};stale.type=SDL_KEYDOWN;stale.key.keysym.sym=key;
                    stale.key.keysym.scancode=SDL_GetScancodeFromKey(key);stale.key.timestamp=0;
                    check(SDL_PeepEvents(&stale,1,SDL_ADDEVENT,0,0)==1,"M41 stale timestamp enqueue failed");++*phase;return false;
                }
                send(key,false);return true;
            });
        };
        const auto inspect=[&](std::function<void()> fn){steps.push_back([fn]{fn();return true;});};
        const auto combat=[&]{return flow->encounter()->combat();};
        const auto deny=[&]{
            check(!flow->canSave(),"M41 pending service exposed Quiet");const auto beforeProviders=providers,beforeSaves=saves;
            handler.withDisplayedInput(SaveGameAction{},handler.displayedInput().value_or(0));
            check(beforeProviders==providers && beforeSaves==saves,"M41 F9 reached provider or I/O");
        };
        const auto settle=[&]{steps.push_back([&] {
            if(flow->canSave())return true;
            if(flow->encounter()->castingCommitted()) {
                const auto context=handler.inputContext(presented);
                if(context.dialog && context.dialog->anyKey){act(AcknowledgeAction{});return false;}
            }
            if(const auto c=combat()) {
                check(c->phase()!=XeenCombatPhase::Failed && c->phase()!=XeenCombatPhase::Defeat && c->phase()!=XeenCombatPhase::SupportStopped,"M41 combat failed");
                if(const auto cast=c->cast()) {
                    if(cast->phase==XeenCombatCastPhase::PartyTarget) {
                        unsigned portrait=0;for(unsigned member=0;member<4;++member)portrait+=bool(c->participants()&(1u<<member));
                        act(SelectMemberAction{portrait});
                    }
                    else if(const auto input=m49_combat::finishSpell(*cast,*party,26u))act(*input);
                    return false;
                }
                // Ordinary First Aid prevents the corrected drop stream from
                // incapacitating one of the two earned Training candidates.
                // Mutation: self-targeting must use the displayed portrait after runners leave.
                if(!cityFight && c->phase()==XeenCombatPhase::PlayerReady && c->participant()==4 &&
                    party->roster.at(1).currentHp<18 && party->roster.at(1).currentSp>0) {
                    act(CastSpellAction{});return false;
                }
                if(c->phase()==XeenCombatPhase::PlayerReady)
                    act(cityFight || c->participant()==1 || c->participant()==4?PlayerAction{AttackAction{}}:PlayerAction{RunAction{}});
            } else if(world->sessionState().journeyActivity()==XeenJourneyActivity::Event || world->sessionState().journeyActivity()==XeenJourneyActivity::Reward)
                act(AcknowledgeAction{});
            check(flow->encounter()->state().phase()!=XeenEncounterPhase::SupportStopped,"M41 exploration stopped");return false;
        });};
        const auto route=[&](const std::string &path,bool drainLast=true) {
            for(unsigned n=0;n<path.size();++n) {
                action(path[n]=='U'?NavigationAction::MoveForward:path[n]=='D'?NavigationAction::MoveBackward:
                    path[n]=='R'?NavigationAction::TurnRight:NavigationAction::TurnLeft);
                if(drainLast || n+1<path.size())settle();
            }
        };
        const auto expect=[&](unsigned day,unsigned minute,std::uint32_t gold,std::uint32_t state,std::uint64_t count) {
            check(party->encounterContext->day==day && party->encounterContext->minutes==minute && party->monsterTreasure->gold==gold &&
                world->sessionState().journeyRandom()->state==state && world->sessionState().journeyRandom()->count==count,"M41 literal date/gold/RNG checkpoint mismatch");
        };
        auto serviceCursor=std::make_shared<XeenJourneyRandomState>();
        auto serviceEconomy=std::make_shared<XeenServiceEconomy>();
        const auto beginService=[&] {
            *serviceCursor=*world->sessionState().journeyRandom();*serviceEconomy=*party->serviceEconomy;
            serviceDraws.clear();serviceDrawTimes.clear();observeService=true;
        };
        // Independently replay speaking requests on either side of the complete
        // original merchant schedule. Check every raw request/value/cursor,
        // including rejection, and reject any unexplained request or suffix.
        const auto serviceOracle=[&](bool generate) {
            m42_test::StockOracle oracle{serviceCursor->state,serviceCursor->count,{}};
            std::size_t offset=0,row=0;auto economy=*serviceEconomy;
            const auto consume=[&](std::size_t previous,bool speaking) {
                const auto expected=oracle.trace.substr(previous);
                check(serviceDraws.compare(offset,expected.size(),expected)==0,"M41 service random request/value/cursor differs");
                for(char c:expected)if(c=='\n') {
                    check(row<serviceDrawTimes.size(),"M41 service random observation incomplete");
                    if(speaking)check(serviceDrawTimes[row].now<serviceDrawTimes[row].sampleEnd,"M41 speaking RNG outside sample interval");
                    ++row;
                }
                offset+=expected.size();
            };
            const auto speech=[&] {
                while(serviceDraws.compare(offset,10,"DRAW 8:12:")==0) {
                    const auto previous=oracle.trace.size();oracle.draw(8,12);consume(previous,true);
                }
            };
            speech();
            if(generate) {
                const auto previous=oracle.trace.size();economy.wares=oracle.generate().wares;consume(previous,false);
                economy.bank.gold=serviceEconomy->bank.gold+serviceEconomy->bank.gold/100u;
                economy.bank.gems=serviceEconomy->bank.gems+serviceEconomy->bank.gems/100u;
            }
            speech();
            check(offset==serviceDraws.size() && row==serviceDrawTimes.size(),"M41 unexpected service random suffix");
            check(*party->serviceEconomy==economy,"M41 independent complete stock/interest differs");
            return oracle;
        };
        const auto checkpoint=[&](std::string label) {
            auto oldSaves=std::make_shared<unsigned>();
            inspect([&,oldSaves,label]{check(flow->canSave(),"M41 checkpoint not Quiet");xeen_decision9::check(*world,*position,label,label=="A" || label=="B");*oldSaves=saves;
                SDL_Event e{};e.type=SDL_KEYDOWN;e.key.keysym.sym=SDLK_F9;e.key.keysym.scancode=SDL_SCANCODE_F9;e.key.timestamp=SDL_GetTicks()+1;
                check(SDL_PushEvent(&e)==1,"M41 F9 enqueue failed");acted=true;});
            steps.push_back([&,oldSaves,label] {
                if(saves==*oldSaves)return false;check(saves==*oldSaves+3,"M41 actual save pipeline incomplete");
                SDL_Event e{};e.type=SDL_KEYUP;e.key.keysym.sym=SDLK_F9;e.key.keysym.scancode=SDL_SCANCODE_F9;SDL_PushEvent(&e);
                const auto actual=snapshot(),disk=XeenSaveFile::read(*target);m40_test::equalFields(actual,disk);
                check(XeenSaveFormat::encode(actual)==XeenSaveFormat::encode(disk),"M41 saved bytes differ");
                fs::copy_file(*target,target->parent_path()/(target->stem().string()+"-"+label+".mmsave"),fs::copy_options::overwrite_existing);
                std::cout<<"MARK "<<label<<'\n';return true;
            });
        };
        const auto waitService=[&]{steps.push_back([&]{deny();return world->sessionState().journeyActivity()==XeenJourneyActivity::Service;});};
        const auto paid=[&](unsigned member,unsigned owner,unsigned day,unsigned gold,int hp,int sp) {
            auto beforeCharacters=std::make_shared<std::array<XeenCharacter,30>>();
            auto beforeInputs=std::make_shared<std::array<XeenCombatInputs,30>>();
            inspect([&,beforeCharacters,beforeInputs]{for(unsigned id=0;id<30;++id){(*beforeCharacters)[id]=party->roster.at(id);(*beforeInputs)[id]=*party->roster.combatInputs(id);}});
            action(SelectMemberAction{member},true);inspect(deny);action(DialogKeyAction{'t'});inspect(deny);
            steps.push_back([&,owner,day,gold,hp,sp,beforeCharacters,beforeInputs]{deny();if(party->roster.at(owner).permanentLevel!=4)return false;
                check(party->roster.combatInputs(owner)->experience==348 && party->monsterTreasure->gold==gold &&
                    party->roster.at(owner).currentHp==hp && party->roster.at(owner).currentSp==sp && party->encounterContext->day==day,
                    "M41 paid level/reset/refill/day mismatch");
                for(unsigned id=0;id<30;++id) {
                    auto c=(*beforeCharacters)[id];auto i=(*beforeInputs)[id];
                    if(std::find(kXeenCombatOwners.begin(),kXeenCombatOwners.end(),id)!=kXeenCombatOwners.end()) {
                        c.temporaryLevel=0;c.intellect.temporary=c.personality.temporary=c.endurance.temporary=0;
                        i.might.temporary=i.speed.temporary=i.accuracy.temporary=i.temporaryAc=0;i.luck->temporary=0;
                        i.resistances->coldTemporary=i.resistances->electricalTemporary=i.poisonResistance->temporary=0;
                    }
                    if(id==owner){++c.permanentLevel;i.experience-=3000;c.currentHp=hp;c.currentSp=sp;}
                    check(xeen_state::sameCharacter(c,party->roster.at(id)) && xeen_state::sameInputs(i,*party->roster.combatInputs(id)),
                        "M41 paid reset/permanent/condition/age/book/equipment/inactive full-owner matrix differs");
                }
                return true;});
            action(AcknowledgeAction{});
        };
        inspect([&] {
            if(resume) {
                check(!replay_test::journeyInitializations && !replay_test::journeyConstructions && !replay_test::actions && !replay_test::pulses &&
                    !replay_test::commands && !replay_test::draws && !replay_test::retirements && !replay_test::timePreparations &&
                    !replay_test::eventExecutions && !interestCalls,"M41 restore replayed gameplay before input");
                const auto actual=snapshot(),disk=XeenSaveFile::read(*target);m40_test::equalFields(actual,disk);
                check(XeenSaveFormat::encode(actual)==XeenSaveFormat::encode(disk),"M41 pre-input restore not exact");
                std::cout<<"RESTORE EXACT BEFORE INPUT\nMARK "<<stage<<'\n';
            } else expect(8,480,800,2000247898,875);
        });
        if(!resume) {
            route("RRRUUURUURRRU");for(unsigned member:{0u,1u,2u,3u,5u}) {action(InteractionAction{});action(SelectMemberAction{member});settle();}
            route("RRUUUU");route("URRRUU");route("UU");route("UURU");route("RRURRRUUUUUUUUURRRU");
            inspect([&]{expect(8,786,870,4263475254u,1096);check(party->roster.combatInputs(18)->experience==3332 && party->roster.combatInputs(1)->experience==3332,"M41 earned XP prefix mismatch");});
            action(CastSpellAction{});action(SelectMemberAction{4});action(DialogKeyAction{'n'});action(DialogKeyAction{'2'});
            action(DialogKeyAction{'s'});action(DialogKeyAction{'c'});action(SelectMemberAction{4});settle();
            inspect([&]{check(party->roster.at(1).currentHp==21 && party->roster.at(1).currentSp==13,"M41 genuine First Aid mismatch");cityFight=true;});
            action(InteractionAction{});action(YesAction{});settle();route("UUUU");
            inspect([&] {
                expect(8,801,870,4180726993u,1109);
                constexpr int hp[]{61,66,61,61,20,27},sp[]{6,0,6,0,13,27};
                constexpr unsigned xp[]{1016,3348,1016,1016,3348,1148};
                for(unsigned n=0;n<6;++n){const auto owner=kXeenCombatOwners[n];check(party->roster.at(owner).currentHp==hp[n] &&
                    party->roster.at(owner).currentSp==sp[n] && party->roster.combatInputs(owner)->experience==xp[n],"M41 Slime checkpoint mismatch");}
            // Decision 9: back up facing East from (15,7) to (10,7), so the view
            // West never wakes Slime #34 before the resumed checkpoints A and B.
            });route("UUURDDDDDLUUUU");inspect([&]{expect(8,813,870,4180726993u,1109);});checkpoint("A");
        }
        if(!resume || stage=="A") {
            inspect(beginService);
            action(InteractionAction{});waitService();paid(1,18,9,780,64,0);paid(4,1,10,690,28,28);
            action(SelectMemberAction{1});inspect([&]{deny();const auto oracle=serviceOracle(false);expect(10,813,690,oracle.state,oracle.count);});
            action(CancelInteractionAction{});settle();
            inspect([&]{observeService=false;const auto oracle=serviceOracle(true);expect(11,813,690,oracle.state,oracle.count);check(interestCalls==1,"M41 stock/interest duplicated");});checkpoint("B");
        }
        // B -> C: reset exit, useful First Aid, lethal Magic Arrow, empty visit.
        route("DDDDRUUUUURUUUUUUU");action(InteractionAction{});action(YesAction{});settle();
        route("RRU");action(InteractionAction{});action(YesAction{});settle();
        auto aid=std::make_shared<bool>(false);auto arrows=std::make_shared<unsigned>(0);auto spell=std::make_shared<unsigned>(0);
        auto enemy=std::make_shared<bool>(false);
        steps.push_back([&,aid,arrows,spell,enemy] {
            if(flow->canSave()){
                if(*aid && *arrows)return true;
                check(position->y<4,"M41 reset contact missing");act(NavigationAction::MoveForward);return false;
            }
            const auto c=combat();if(!c){if(world->sessionState().journeyActivity()==XeenJourneyActivity::Event || world->sessionState().journeyActivity()==XeenJourneyActivity::Reward)act(AcknowledgeAction{});return false;}
            if(clockCase && c->phase()==XeenCombatPhase::PlayerReady && !clockFault){clockArmed=true;return false;}
            check(c->phase()!=XeenCombatPhase::Failed && c->phase()!=XeenCombatPhase::Defeat && c->phase()!=XeenCombatPhase::SupportStopped,"M41 continued combat failed");
            if(const auto cast=c->cast()) {
                if(cast->phase==XeenCombatCastPhase::Result) {
                    if(cast->result.failed)act(AcknowledgeAction{});
                    *spell=0;*enemy=false;
                }else if(cast->phase==XeenCombatCastPhase::PartyTarget)act(SelectMemberAction{4});
                else if(const auto input=m49_combat::finishSpell(*cast,*party,*spell==1?26u:45u))act(*input);
            } else if(c->phase()==XeenCombatPhase::PlayerReady) {
                if(c->participant()==4 && !*aid && party->roster.at(1).currentHp<28){*aid=true;*spell=1;act(CastSpellAction{});}
                else if(c->participant()==5 && *aid){
                    if(!*enemy){*enemy=true;act(SelectCombatTargetAction{0});}
                    else {++*arrows;*spell=2;act(CastSpellAction{});}
                }
                else act(BlockAction{});
            }return false;
        });
        // Contact can occur before y4. Reach the same route start without
        // changing the specified combat policy or editing the camera.
        steps.push_back([&]{
            if(!flow->canSave())return false;
            if(position->y==4 && position->x==15)return true;
            act(NavigationAction::MoveForward);return false;
        });
        route("UUULUUUUURUUUU");inspect(beginService);action(InteractionAction{});waitService();
        action(SelectMemberAction{1});action(DialogKeyAction{'t'});
        action(SelectMemberAction{4});action(DialogKeyAction{'t'});
        action(CancelInteractionAction{});settle();inspect([&]{observeService=false;const auto oracle=serviceOracle(false);
            check(world->sessionState().journeyRandom()->state==oracle.state && world->sessionState().journeyRandom()->count==oracle.count,"M41 empty service RNG differs");
            check(party->encounterContext->day==12 && party->monsterTreasure->gold==690,
            "M41 later empty service added training/debit");});checkpoint("C");
        inspect([&]{std::cout<<"M41 PRODUCTION WITNESS PASSED\n";SDL_Event e{};e.type=SDL_QUIT;SDL_PushEvent(&e);});
        auto native=handler;native.closed={};
        // The simulated clock advances only when idle runs; a live deadline would make
        // the SDL loop wait in real time for it. Behavior is unchanged, only the wait.
        native.nextPresentationDeadline=[]{return std::optional<std::uint64_t>{0};};
        native.beginCycle=[&](std::uint64_t){handler.beginCycle(++cycle);};
        unsigned evidenceFrame=0;
        native.framePresented=[&](const auto &frame){
            check(!nativeFailed,"M41 failed native frame acquired authority");
            if(frame!=presented) {
                const auto beforeProviders=providers,beforeSaves=saves;
                handler.withPresentedInput(SaveGameAction{},handler.displayedInput().value_or(0),frame);
                check(beforeProviders==providers && beforeSaves==saves,"M41 unpresented handoff F9 invoked provider/I/O");
            }
            handler.framePresented(frame);presented=frame;shown=true;
            if(world->sessionState().journeyActivity()==XeenJourneyActivity::Service) {
                std::vector<std::uint32_t> pixels;pixels.reserve(frame->pixels.size());
                for(auto index:frame->pixels){const auto n=index*3;pixels.push_back(0xff000000u|(frame->palette[n]<<16)|(frame->palette[n+1]<<8)|frame->palette[n+2]);}
                auto *surface=SDL_CreateRGBSurfaceFrom(pixels.data(),320,200,32,1280,0xff0000,0xff00,0xff,0xff000000);
                check(surface,"M41 evidence surface absent");
                const auto file=target->parent_path()/(target->stem().string()+"-training-"+std::to_string(++evidenceFrame)+".bmp");
                const int saved=SDL_SaveBMP(surface,file.string().c_str());SDL_FreeSurface(surface);check(saved==0,"M41 native evidence frame write failed");
            }
        };
        const auto drive=[&]()->std::optional<IndexedFrame> {
            check(++iterations<30000,"M41 witness iteration bound");now+=100;next.reset();acted=false;
            if(shown && iterations%3!=0)while(!steps.empty()) {const bool done=steps.front()();if(done)steps.pop_front();if(acted || !done)break;}
            if(acted)return next;
            try {auto frame=idle();if(frame)shown=false;return frame;}
            catch(...) {
                if(!clockCase || !clockFault)throw;
                check(combat() && combat()->phase()==XeenCombatPhase::Failed && combat()->result().failure==XeenCombatFailure::Integrity &&
                    world->sessionState().journeyActivity()==XeenJourneyActivity::Failed && !flow->canSave(),"M41 post-Training combat ABA not permanent");
                const auto economy=*party->serviceEconomy;const auto rng=*world->sessionState().journeyRandom();
                const auto date=*party->encounterContext;const auto disk=m40_test::diskBytes(*target);
                const auto beforeProviders=providers,beforeSaves=saves;const auto beforeCommands=replay_test::commands;
                unsigned compositions=0;
                for(unsigned retry=0;retry<3;++retry) {
                    try{flow->refresh(true);}catch(...){}
                    try{const_cast<XeenCombat *>(combat())->preparePresentation(combat()->ticket(),[&]{++compositions;});}catch(...){}
                    for(PlayerAction a:{PlayerAction{AttackAction{}},PlayerAction{CastSpellAction{}},PlayerAction{SaveGameAction{}}})
                        try{handler.withDisplayedInput(a,handler.displayedInput().value_or(0));}catch(...){}
                    try{idle();}catch(...){}
                    check(!flow->canSave() && combat()->phase()==XeenCombatPhase::Failed,"M41 retry cleansed combat-clock failure");
                }
                check(!compositions && beforeProviders==providers && beforeSaves==saves && beforeCommands==replay_test::commands &&
                    economy==*party->serviceEconomy && rng==*world->sessionState().journeyRandom() && date==*party->encounterContext &&
                    disk==m40_test::diskBytes(*target),"M41 poisoned combat reached publication/provider/I/O");
                clockVerified=true;steps.clear();SDL_Event e{};e.type=SDL_QUIT;SDL_PushEvent(&e);return std::nullopt;
            }
        };
        const bool ok=original.show(first,native,escape,drive,status);
        if(clockCase){check(clockVerified,"M41 combat-clock ABA did not execute");std::cout<<"M41 POST-TRAINING CLOCK ABA MONOTONIC / NO QUIET / F9 DISK PRESERVED / RETRY REJECTED\n";return false;}
        if(!control.empty()) {
            check(nativeFailed && !ok && !flow->canSave(),"M41 native failure exposed Quiet");
            const bool admitted=control.find("admission")!=std::string::npos,departed=control.find("departure")!=std::string::npos;
            check(party->roster.at(18).permanentLevel==(admitted?3:4) && party->roster.combatInputs(18)->experience==(admitted?3348:348) &&
                party->monsterTreasure->gold==(admitted?870:departed?690:780) && party->encounterContext->day==(admitted?8:departed?11:9),
                "M41 native failure lost committed prefix");
            deny();std::cout<<"M41 NATIVE FAILURE PRESERVATION PASSED\n";
        }
        return ok;
    };
    return realPlay(app,services,camera,target,resume,entry,seed);
}
