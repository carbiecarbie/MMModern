// Original initialization, ordinary owners, concrete SDL frames and actual F9.
// Seed, typed input, native injection and fault controls are test-only evidence.
#include "XeenDecisionNine.h"
#include "XeenProbeFired.h"
#include "app/Application.h"
#include "app/XeenGameplayServices.h"
#include "platform/XeenSaveFile.h"
#include "formats/xeen/XeenSaveFormat.h"
#include "games/xeen/XeenStateEquality.h"
#include "games/xeen/XeenRestoreGuard.h"
#include "XeenRestoreReplayProbe.h"
#include "XeenMonsterTestAccess.h"
#include "XeenM42Evidence.h"
#define SDL_MAIN_HANDLED
#include <SDL.h>
#include <deque>
#include <cstdlib>
#include <iostream>
#include <stdexcept>
namespace {
bool nativeUpload=false,nativeCopy=false,nativeFailed=false;
std::function<void(const SDL_Event &)> nativeInputReceived;
std::function<void(const SDL_KeyboardEvent &)> nativeInputRetired;
}
extern "C" int __real_SDL_PollEvent(SDL_Event *);
extern "C" int __wrap_SDL_PollEvent(SDL_Event *e){probe_fired::hit("SDL_PollEvent");const auto result=__real_SDL_PollEvent(e);if(result && e && nativeInputReceived)nativeInputReceived(*e);return result;}
extern "C" int __real_SDL_WaitEventTimeout(SDL_Event *,int);
extern "C" int __wrap_SDL_WaitEventTimeout(SDL_Event *e,int timeout){probe_fired::hit("SDL_WaitEventTimeout");const auto result=__real_SDL_WaitEventTimeout(e,timeout);if(result && e && nativeInputReceived)nativeInputReceived(*e);return result;}
extern "C" void __real_SDL_FilterEvents(SDL_EventFilter,void *);
extern "C" void __wrap_SDL_FilterEvents(SDL_EventFilter filter,void *context){probe_fired::hit("SDL_FilterEvents");
    if(!nativeInputRetired){__real_SDL_FilterEvents(filter,context);return;}
    struct Forward {SDL_EventFilter filter;void *context;} forward{filter,context};
    __real_SDL_FilterEvents([](void *p,SDL_Event *e)->int{
        const auto &f=*static_cast<Forward *>(p);const auto stamp=e->type==SDL_KEYDOWN?e->key.timestamp:0;
        const auto result=f.filter(f.context,e);
        if(!result && nativeInputReceived)nativeInputReceived(*e);
        if(e->type==SDL_KEYDOWN && e->key.timestamp!=stamp && nativeInputRetired)nativeInputRetired(e->key);
        return result;
    },&forward);
}
extern "C" int __real_SDL_UpdateTexture(SDL_Texture *,const SDL_Rect *,const void *,int);
extern "C" int __wrap_SDL_UpdateTexture(SDL_Texture *t,const SDL_Rect *r,const void *p,int pitch){probe_fired::hit("SDL_UpdateTexture");if(nativeUpload){nativeUpload=false;nativeFailed=true;return SDL_SetError("M39 injected upload failure");}return __real_SDL_UpdateTexture(t,r,p,pitch);}
extern "C" int __real_SDL_RenderCopy(SDL_Renderer *,SDL_Texture *,const SDL_Rect *,const SDL_Rect *);
extern "C" int __wrap_SDL_RenderCopy(SDL_Renderer *r,SDL_Texture *t,const SDL_Rect *s,const SDL_Rect *d){probe_fired::hit("SDL_RenderCopy");if(nativeCopy){nativeCopy=false;nativeFailed=true;return SDL_SetError("M39 injected copy failure");}return __real_SDL_RenderCopy(r,t,s,d);}
using namespace mmodern;
namespace fs=std::filesystem;
// Transparent executable-only probes: the opaque private response is merely
// forwarded. Tests cannot manufacture a production casting capability.
static unsigned castBegins=0,castResponses=0,castServices=0;
#define CAST_BEGIN "_ZN7mmodern10XeenCombat9beginCastERNS0_12CastResponseERKSt8functionIFNS_21XeenLearnedSpellNamesEvEE"
#define CAST_RESPOND "_ZN7mmodern10XeenCombat11respondCastERNS0_12CastResponseENS_19XeenCombatCastInputEjRKSt8functionIFNS_21XeenLearnedSpellNamesEvEE"
#define CAST_SERVICE "_ZN7mmodern10XeenCombat11serviceCastERKNS0_6TicketEy"
extern "C" XeenCombatResult realBegin(XeenCombat *,void *,const std::function<XeenLearnedSpellNames()> &) asm("__real_" CAST_BEGIN);
extern "C" XeenCombatResult wrapBegin(XeenCombat *,void *,const std::function<XeenLearnedSpellNames()> &) asm("__wrap_" CAST_BEGIN);
extern "C" XeenCombatResult wrapBegin(XeenCombat *c,void *r,const std::function<XeenLearnedSpellNames()> &p){probe_fired::hit("XeenCombat::beginCast");++castBegins;return realBegin(c,r,p);}
extern "C" XeenCombatResult realRespond(XeenCombat *,void *,XeenCombatCastInput,unsigned,const std::function<XeenLearnedSpellNames()> &) asm("__real_" CAST_RESPOND);
extern "C" XeenCombatResult wrapRespond(XeenCombat *,void *,XeenCombatCastInput,unsigned,const std::function<XeenLearnedSpellNames()> &) asm("__wrap_" CAST_RESPOND);
extern "C" XeenCombatResult wrapRespond(XeenCombat *c,void *r,XeenCombatCastInput a,unsigned i,const std::function<XeenLearnedSpellNames()> &p){probe_fired::hit("XeenCombat::respondCast");++castResponses;return realRespond(c,r,a,i,p);}
extern "C" XeenCombatResult realService(XeenCombat *,const XeenCombat::Ticket &,std::uint64_t) asm("__real_" CAST_SERVICE);
extern "C" XeenCombatResult wrapService(XeenCombat *,const XeenCombat::Ticket &,std::uint64_t) asm("__wrap_" CAST_SERVICE);
extern "C" XeenCombatResult wrapService(XeenCombat *c,const XeenCombat::Ticket &t,std::uint64_t n){probe_fired::hit("XeenCombat::serviceCast");++castServices;return realService(c,t,n);}

#define PLAY_SYMBOL "_ZNK7mmodern11Application12playGameplayERKNS_20XeenGameplayServicesENS_10XeenCameraERKSt8optionalINSt10filesystem7__cxx114pathEEbNS_18XeenEncounterEntryES5_IjE"
extern "C" int realPlay(const Application *,const XeenGameplayServices &,XeenCamera,const std::optional<fs::path> &,bool,XeenEncounterEntry,std::optional<std::uint32_t>) asm("__real_" PLAY_SYMBOL);
extern "C" int wrappedPlay(const Application *,const XeenGameplayServices &,XeenCamera,const std::optional<fs::path> &,bool,XeenEncounterEntry,std::optional<std::uint32_t>) asm("__wrap_" PLAY_SYMBOL);
namespace {
void check(bool v,const char *m){if(!v)throw std::runtime_error(m);}
std::string env(const char *key,const char *fallback=""){const auto p=std::getenv(key);return p?p:fallback;}
void checkRestoredFields(const XeenSaveSnapshot &disk,const XeenSaveSnapshot &live) {
    check(disk.journey && live.journey,"M39 pre-input Journey missing");
    const auto &a=*disk.journey,&b=*live.journey;
    for(unsigned i=0;i<30;++i)check(xeen_state::sameCharacter(disk.characters[i],live.characters[i]) &&
        a.supplements[i].owner==b.supplements[i].owner && xeen_state::sameInputs(a.supplements[i].inputs,b.supplements[i].inputs),
        "M39 pre-input owner/book/item/supplement mismatch");
    check(disk.maps==live.maps,"M39 pre-input per-map state mismatch");
    check(disk.resources==live.resources && disk.activeRosterIds==live.activeRosterIds &&
        disk.camera.mapId==live.camera.mapId && disk.camera.x==live.camera.x && disk.camera.y==live.camera.y &&
        disk.camera.direction==live.camera.direction && disk.questItems==live.questItems && disk.questFlags==live.questFlags &&
        disk.gameFlags==live.gameFlags &&
        a.schema==b.schema && a.content==b.content &&
        a.context==b.context && a.random==b.random && a.treasure==b.treasure && a.worldFlags==b.worldFlags,
        "M39 pre-input domain/camera/context/flags/treasure/RNG mismatch");
}
}
#include "XeenM39CastingControls.h"
#include "XeenM39InputControls.h"
static const probe_fired::Expect playProbe{"Application::playGameplay"};
extern "C" int wrappedPlay(const Application *app,const XeenGameplayServices &original,XeenCamera camera,
    const std::optional<fs::path> &target,bool resume,XeenEncounterEntry entry,
    std::optional<std::uint32_t> seed) {probe_fired::hit("Application::playGameplay");if(!resume && !(std::getenv("MMODERN_M39_CONTROL") && *std::getenv("MMODERN_M39_CONTROL")))for(const char *probe:{"XEEN_REPLAY_COMMAND","XEEN_REPLAY_DRAW","XEEN_REPLAY_JOURNEY_CONSTRUCT","XEEN_REPLAY_REGIONAL_MOVE","XEEN_REPLAY_SERVICE","XEEN_REPLAY_TIME","SDL_RenderCopy","SDL_UpdateTexture","SDL_PollEvent","SDL_WaitEventTimeout","SDL_FilterEvents","XeenCombat::beginCast","XeenCombat::respondCast","XeenCombat::serviceCast"})probe_fired::expect(probe);
    const auto branch=env("MMODERN_M39_BRANCH","A"),stage=env("MMODERN_M39_STAGE","fresh"),control=env("MMODERN_M39_CONTROL");
    const bool seededControl=branch!="B" && branch!="C";
    m42_test::StockOracle fresh{3626689381u,0,{}};const auto expectedFresh=fresh.generate();
    const auto rngOffset=seededControl?0:fresh.count;
    if(!resume && !seededControl)seed=3626689381u;
    if(resume) {
        castBegins=castResponses=castServices=0;
        replay_test::journeyInitializations=replay_test::journeyConstructions=0;
        replay_test::actions=replay_test::pulses=replay_test::retirements=0;
        replay_test::commands=replay_test::draws=0;replay_test::services=replay_test::timePreparations=0;
        replay_test::eventExecutions=replay_test::equipmentChanges=replay_test::transfers=0;
    }
    auto services=original;
    XeenEventFlow *flow=nullptr;XeenWorld *world=nullptr;const XeenPartyState *party=nullptr;
    const XeenCamera *position=nullptr;const XeenGameFlags *flags=nullptr;
    std::uint64_t now=0,cycle=0;unsigned saves=0,providers=0;
    std::function<void()> resourceFault;bool renderFault=false;
    const auto count=[&](auto fn){return [fn=std::move(fn),&providers](auto &&...args)->decltype(auto){++providers;return fn(std::forward<decltype(args)>(args)...);};};
#define M39_COUNT(field) if(services.field)services.field=count(services.field)
    M39_COUNT(resources.loadInitialParty);M39_COUNT(resources.loadEvents);M39_COUNT(resources.loadInitialCharacters);M39_COUNT(resources.loadInitialContext);
    M39_COUNT(resources.loadMonsterStatistics);M39_COUNT(resources.regionalManifest);M39_COUNT(resources.loadInitialPurse);
    M39_COUNT(resources.loadInitialWorldFlags);M39_COUNT(resources.loadRegionalText);M39_COUNT(maps);M39_COUNT(objects);M39_COUNT(texts);M39_COUNT(compose);
    M39_COUNT(npcDraw);M39_COUNT(validateEncounterSprite);M39_COUNT(validateCombatSprite);M39_COUNT(sampleJourneySeed);
#undef M39_COUNT
    services.clock=[&]{return now;};services.observeSaveStage=[&](auto){++saves;};
    services.observeGameplay=[&](auto &w,auto &,const auto &p,const auto &c,const auto &f){world=&w;party=&p;position=&c;flags=&f;
        // Decision 8: the grouped M60 controls wound and wake their three Orcs
        // live after the game load, never through the save.
        if(resume && control.rfind("m60-",0)==0 && control.find("-group")!=std::string::npos)
            for(unsigned index:{7u,8u,9u})XeenMonsterTestAccess::setLive(*flow,w,23,index,1,true);};
    services.configureFlow=[&](auto &f,const auto &c){original.configureFlow(f,c);flow=&f;};
    const auto compose=original.composeEncounter;
    services.composeEncounter=[&](auto &w,const auto &p,const auto &c,auto phase,auto appearance){++providers;if(resourceFault)resourceFault();if(renderFault){renderFault=false;throw std::runtime_error("M39 injected presentation-only failure");}return compose(w,p,c,phase,appearance);};
    const auto names=original.resources.loadLearnedSpellNames;
    services.resources.loadLearnedSpellNames=[&]{++providers;auto value=names();if(resourceFault)resourceFault();if(control=="names" && resourceFault)value.names[1]="Changed";return value;};
    replay_test::observeDraw=[&](auto lo,auto hi,auto value,auto cursor){
        std::cout<<"DRAW "<<lo<<':'<<hi<<':'<<(value?std::to_string(*value):"rejected")<<':'<<cursor.state<<':'<<cursor.count<<'\n';
    };
    services.show=[&](const IndexedFrame &first,const auto &handler,const auto &escape,const auto &idle,const auto &status) {
        check(flow && world && party && position && flags && target,"M39 owners absent");
        if(branch=="input")return m39InputControls(control,first,handler,idle,original.show,escape,status,*flow,*world,*party,*position,now,cycle,resourceFault);
        if(branch=="fault") {
            const auto mutate=[&](const std::string &which) {
                auto &p=const_cast<XeenPartyState &>(*party);auto &c=p.roster.at(which=="aba-hp"?1:29);
                if(which=="names")return;
                if(which=="aba-hp"){++c.currentHp;--c.currentHp;}
                else if(which=="aba-inactive"){++c.currentHp;--c.currentHp;}
                else if(which=="aba-sp"){++c.currentSp;--c.currentSp;}
                else if(which=="aba-book"){c.learnedSpells->at(0)^=1;c.learnedSpells->at(0)^=1;}
                else if(which=="aba-presence"){auto original=c.learnedSpells;c.learnedSpells.reset();c.learnedSpells=original;}
                else if(which=="aba-class"){auto original=c.characterClass;c.characterClass=XeenCharacterClass::Knight;c.characterClass=original;}
                else if(which=="aba-item"){c.miscellaneous[8].material^=1;c.miscellaneous[8].material^=1;}
                else if(which=="aba-supplement"){auto &v=const_cast<XeenCombatInputs &>(*p.roster.combatInputs(29));++v.experience;--v.experience;}
                else if(which=="aba-membership"){auto original=p.party;p.party=XeenParty::fromRosterIds({0,1,2,3,4,5});p.party=original;}
                else if(which=="aba-purse"){++p.monsterTreasure->gold;--p.monsterTreasure->gold;}
                else if(which=="aba-context"){++p.encounterContext->minutes;--p.encounterContext->minutes;}
                else if(which=="aba-flags"){auto &f=const_cast<XeenGameFlags &>(*flags);auto original=f;f.set(255);f=original;}
                else if(which=="aba-rng"){auto &r=const_cast<XeenMutableOptional<XeenJourneyRandomState> &>(world->sessionState().journeyRandom());++r->count;--r->count;}
                else if(which=="aba-mainland" || which=="aba-statistics") {auto &a=const_cast<XeenActor &>(world->sessionState().monsters(23).at(9));if(which=="aba-mainland"){++a.hp;--a.hp;}else{a.statistics->raw[39]^=1;a.statistics->raw[39]^=1;}}
                else if(which=="aba-city"){auto &a=const_cast<XeenActor &>(world->sessionState().regionalActors(28).at(0));++a.hp;--a.hp;}
                else if(which=="aba-map"){world->discardMapCache();auto &m=const_cast<XeenMap &>(world->map(28));m.geometry.cells[0].rawAttributes^=1;m.geometry.cells[0].rawAttributes^=1;}
                else if(which=="aba-event"){world->discardMapCache();auto &m=const_cast<XeenMap &>(world->map(28));m.instructions.push_back({});m.instructions.pop_back();}
                else if(which=="aba-mob"){world->discardMapCache();auto &m=const_cast<XeenObjectFile &>(world->objectFile(28));++m.entities.objects[53].x;--m.entities.objects[53].x;}
                else throw std::runtime_error("unknown M39 ABA control");
            };
            return m39CastControls(control,first,handler,idle,*flow,*world,*party,*position,*flags,*target,now,cycle,providers,saves,resourceFault,renderFault,mutate);
        }
        check(world->sessionState().journey(),"M39 content selection mismatch");
        std::deque<std::function<bool()>> steps;std::optional<IndexedFrame> next;
        bool shown=false,acted=false;unsigned iterations=0,arrowProjectiles=0;
        const auto act=[&](PlayerAction a) {
            check(shown,"M39 response before concrete presentation");
            const auto token=handler.displayedInput();check(bool(token),"M39 input authority absent");
            next=handler.withDisplayedInput(a,*token);acted=true;shown=false;
            // A consumed/unpresented response cannot reach resource or save work.
            const auto beforeProviders=providers,beforeSaves=saves;
            handler.withDisplayedInput(CastSpellAction{},*token);
            handler.withDisplayedInput(SaveGameAction{},*token);
            check(beforeProviders==providers && beforeSaves==saves,"M39 stale input invoked work");
        };
        const auto action=[&](PlayerAction a){steps.push_back([&,a]{act(a);return true;});};
        const auto selectSpell=[&](unsigned row){action(DialogKeyAction{'n'});action(DialogKeyAction{'0'+row});action(DialogKeyAction{'s'});};
        const auto castSelected=[&]{action(DialogKeyAction{'c'});};
        const auto inspect=[&](std::function<void()> f){steps.push_back([f]{f();return true;});};
        const auto combat=[&]()->const XeenCombat *{return flow->encounter()->combat();};
        const auto cast=[&]()->std::optional<XeenCombatCastView>{const auto c=combat();return c?c->cast():std::nullopt;};
        const auto result=[&]{steps.push_back([&]{
            check(combat() && combat()->phase()!=XeenCombatPhase::Failed,"M39 cast technical failure");
            return cast() && cast()->phase==XeenCombatCastPhase::Result;
        });};
        const auto settle=[&]{steps.push_back([&]{
            if(flow->canSave())return true;
            if(const auto c=combat()) {
                check(c->phase()!=XeenCombatPhase::Failed && c->phase()!=XeenCombatPhase::Defeat && c->phase()!=XeenCombatPhase::SupportStopped,"M39 combat stop");
                if(!c->cast() && c->phase()==XeenCombatPhase::PlayerReady)act(branch=="B" || branch=="C" ? PlayerAction{RunAction{}} : PlayerAction{AttackAction{}});
            }else if(world->sessionState().journeyActivity()==XeenJourneyActivity::Event || world->sessionState().journeyActivity()==XeenJourneyActivity::Reward)act(AcknowledgeAction{});
            check(flow->encounter()->state().phase()!=XeenEncounterPhase::SupportStopped,"M39 exploration stop");
            return false;
        });};
        const auto route=[&](const std::string &r,bool drainLast=true){for(unsigned i=0;i<r.size();++i){
            switch(r[i]) {
                case 'U':action(NavigationAction::MoveForward);break;
                case 'D':action(NavigationAction::MoveBackward);break;
                case 'L':action(NavigationAction::TurnLeft);break;
                case 'R':action(NavigationAction::TurnRight);break;
                case 'F':action(ShootAction{});break;
            }
            if(drainLast || i+1<r.size())settle();
        }};
        const auto untilActor=[&](unsigned slot,bool attack){steps.push_back([&,slot,attack]{
            if(!combat()) {
                if(world->sessionState().journeyActivity()==XeenJourneyActivity::Event || world->sessionState().journeyActivity()==XeenJourneyActivity::Reward)act(AcknowledgeAction{});
                return false;
            }
            check(combat()->phase()!=XeenCombatPhase::Failed && combat()->phase()!=XeenCombatPhase::Defeat,"M39 expected actor unavailable");
            if(combat()->phase()!=XeenCombatPhase::PlayerReady)return false;
            if(combat()->participant()==int(slot))return true;
            act(attack ? PlayerAction{AttackAction{}} : PlayerAction{BlockAction{}});return false;
        });};
        const auto deny=[&]{inspect([&]{
            check(!flow->canSave(),"M39 modal manufactured Quiet");
            const auto before=saves,resource=providers;
            auto prior=fs::exists(*target)?XeenSaveFormat::encode(XeenSaveFile::read(*target)):std::vector<std::uint8_t>{};
            handler.withDisplayedInput(SaveGameAction{},*handler.displayedInput());
            check(before==saves && resource==providers,"M39 early F9 reached providers");
            if(!prior.empty())check(prior==XeenSaveFormat::encode(XeenSaveFile::read(*target)),"M39 denied save changed disk");
        });};
        const auto firstAid=[&](unsigned targetSlot) {
            action(CastSpellAction{});deny();selectSpell(2);deny();
            castSelected();deny();action(SelectMemberAction{targetSlot});result();deny();
        };
        const auto arrow=[&](unsigned record) {
            steps.push_back([&,record]{const auto rows=combat()->contacts();for(unsigned i=0;i<3;++i)if(rows[i] && rows[i]->recordIndex==record){act(SelectCombatTargetAction{i});return true;}throw std::runtime_error("M39 Arrow contact absent");});
            action(CastSpellAction{});deny();selectSpell(3);deny();castSelected();deny();result();deny();
        };
        const auto checkpoint=[&](const std::string &label) {
            auto before=std::make_shared<unsigned>();
            inspect([&,before,label]{check(flow->canSave(),"M39 checkpoint not presented Quiet");xeen_decision9::check(*world,*position,label,label=="A" || label=="city"); /* B/C: decision 8 cases. */*before=saves;SDL_Event e{};e.type=SDL_KEYDOWN;e.key.keysym.sym=SDLK_F9;e.key.keysym.scancode=SDL_SCANCODE_F9;e.key.timestamp=SDL_GetTicks()+1;check(SDL_PushEvent(&e)==1,"M39 F9 enqueue");acted=true;});
            steps.push_back([&,before,label]{
                if(saves==*before)return false;
                check(saves==*before+3,"M39 actual F9 pipeline incomplete");
                SDL_Event e{};e.type=SDL_KEYUP;e.key.keysym.sym=SDLK_F9;e.key.keysym.scancode=SDL_SCANCODE_F9;SDL_PushEvent(&e);
                fs::copy_file(*target,target->parent_path()/(target->stem().string()+"-"+label+".mmsave"),fs::copy_options::overwrite_existing);
                std::cout<<"M39 CHECKPOINT "<<label<<" map="<<position->mapId<<" x="<<position->x<<" y="<<position->y
                    <<" minute="<<party->encounterContext->minutes<<" ctr="<<party->encounterContext->ctr24
                    <<" SP="<<party->roster.at(1).currentSp<<','<<party->roster.at(6).currentSp<<" gold="<<party->monsterTreasure->gold
                    <<" rng="<<world->sessionState().journeyRandom()->state<<':'<<world->sessionState().journeyRandom()->count<<'\n';
                return true;
            });
        };
        inspect([&]{if(resume) {
            check(!replay_test::journeyInitializations && !replay_test::journeyConstructions && !replay_test::actions && !replay_test::pulses &&
                !replay_test::retirements && !replay_test::commands && !replay_test::draws && !replay_test::services && !replay_test::timePreparations &&
                !replay_test::eventExecutions && !replay_test::equipmentChanges && !replay_test::transfers &&
                !castBegins && !castResponses && !castServices,"M39 restore replay including cast cost/effect");
            const auto disk=XeenSaveFile::read(*target);const auto live=XeenSaveState::capture(original.resources.signature,*party,*position,*flags,*world);
            check(XeenSaveFormat::encode(disk)==XeenSaveFormat::encode(live),"M39 pre-input recapture mismatch");
            checkRestoredFields(disk,live);
            if(stage=="continued") {
                // Decision 8: B and C save the Toad wounded (33 or 25) and woken;
                // the game load rebuilds it at MON HP and asleep.
                const auto &toad=world->sessionState().monsters(23)[15];
                check(toad.hp==std::int32_t(toad.statistics->baseHp()) && !toad.activated,"M39 restored Toad was not rebuilt at MON HP, asleep");
                std::cout<<"M39 DECISION8 TOAD HP="<<toad.hp<<" woken="<<toad.activated<<'\n';
            }
            std::cout<<"M39 RESTORE EXACT BEFORE INPUT\n";
        }});
        if(!resume) {
            if(!seededControl)inspect([&]{check(m40_test::stockBytes(*party->serviceEconomy)==m40_test::stockBytes(expectedFresh) && world->sessionState().journeyRandom()->state==fresh.state && world->sessionState().journeyRandom()->count==fresh.count,"M71 M39 fresh stock/RNG oracle: catches wrong item or extra draw before combat");});
            if(branch=="native-fault")checkpoint("prior");
            if(branch=="A" || branch=="controls" || branch=="native" || branch=="native-fault") {
                route("UFU",false);untilActor(4,false);
                inspect([&]{check(position->x==7 && position->y==11 && party->roster.at(6).currentHp==11 && party->roster.at(1).currentSp==21,"M39 A prefix");});
                if(branch=="native-fault") {
                    if(control.find("projectile")!=std::string::npos) {
                        // Arm native failure at the first settled projectile compose.
                        steps.clear();checkpoint("prior");route("UFU",false);untilActor(5,false);
                        action(CastSpellAction{});selectSpell(3);castSelected();
                        inspect([&]{if(control.rfind("upload-",0)==0)nativeUpload=true;else nativeCopy=true;});
                    }else {
                        action(CastSpellAction{});selectSpell(2);
                        if(control.find("target")!=std::string::npos)inspect([&]{if(control.rfind("upload-",0)==0)nativeUpload=true;else nativeCopy=true;});
                        castSelected();
                        if(control.find("result")!=std::string::npos)inspect([&]{if(control.rfind("upload-",0)==0)nativeUpload=true;else nativeCopy=true;});
                        action(SelectMemberAction{5});
                    }
                }else if(branch=="native") {
                    // Actual native Cast/New/row/Select/Cast/F4. Held/repeated,
                    // old timestamps and multiple responses cannot cross a frame.
                    auto remembered=std::make_shared<unsigned>();
                    inspect([&,remembered]{*remembered=party->roster.at(1).currentSpell;});
                    // Poll native intake and acquired readiness; a retained effect may span many draws.
                    const auto keys=[&](std::function<void()> enqueue,std::function<void()> verify,std::function<bool()> completed=[] {return true;}) {
                        auto laps=std::make_shared<unsigned>(0);auto enqueued=std::make_shared<bool>(false);
                        steps.push_back([&,laps,enqueued,enqueue,verify,completed]{
                            check(++*laps<=100,"M39 native acquired readiness bound");
                            if(!handler.inputContext(flow->frame().presentation()).readyForAction)return false;
                            if(!*enqueued){*enqueued=true;*laps=0;enqueue();acted=true;return false;}
                            if(SDL_HasEvents(SDL_KEYDOWN,SDL_KEYUP) || !completed())return false;
                            verify();return true;
                        });
                    };
                    const auto key=[](SDL_Keycode code,Uint32 type=SDL_KEYDOWN,Uint8 repeat=0,bool stale=false){
                        SDL_Event e{};e.type=type;e.key.keysym.sym=code;e.key.keysym.scancode=SDL_GetScancodeFromKey(code);e.key.repeat=repeat;e.key.timestamp=stale?0:SDL_GetTicks()+1;
                        check(SDL_PeepEvents(&e,1,SDL_ADDEVENT,0,0)==1,"M39 native keyboard enqueue");};
                    keys([key]{key(SDLK_c);key(SDLK_c,SDL_KEYDOWN,1);key(SDLK_n);key(SDLK_n,SDL_KEYUP);},[&]{check(cast() && cast()->phase==XeenCombatCastPhase::Confirm && party->roster.at(1).currentSp==21,"M39 native batch crossed Cast");},[&]{return bool(cast());});
                    keys([key]{key(SDLK_c,SDL_KEYUP);key(SDLK_n);key(SDLK_n,SDL_KEYUP);},[&]{check(cast()->phase==XeenCombatCastPhase::Learned,"M39 native New");});
                    keys([key]{key(SDLK_2);key(SDLK_2,SDL_KEYUP);},[&,remembered]{check(cast()->selection==1 && party->roster.at(1).currentSpell==*remembered,"M39 row selected before Select");});
                    keys([key]{key(SDLK_RETURN);key(SDLK_RETURN);key(SDLK_RETURN,SDL_KEYDOWN,1);},[&]{check(cast()->phase==XeenCombatCastPhase::Confirm && party->roster.at(1).currentSpell==14 && party->roster.at(1).currentSp==21,"M39 held/batched Select debited");});
                    keys([key]{
                        for(auto kind:{SDL_WINDOWEVENT_SIZE_CHANGED,SDL_WINDOWEVENT_EXPOSED}){SDL_Event e{};e.type=SDL_WINDOWEVENT;e.window.event=kind;e.window.data1=640;e.window.data2=400;check(SDL_PushEvent(&e)==1,"M39 native resize enqueue");}
                        key(SDLK_RETURN);key(SDLK_RETURN,SDL_KEYUP);key(SDLK_c,SDL_KEYDOWN,0,true);key(SDLK_c,SDL_KEYUP);},[&]{check(cast()->phase==XeenCombatCastPhase::Confirm && party->roster.at(1).currentSp==21,"M39 stale timestamp admitted cost");});
                    keys([key]{key(SDLK_c);key(SDLK_c,SDL_KEYUP);},[&]{check(cast()->phase==XeenCombatCastPhase::PartyTarget && party->roster.at(1).currentSp==20,"M39 native debit");});
                    keys([key]{key(SDLK_F4);key(SDLK_F4,SDL_KEYUP);},[&]{check(!cast() && combat()->participant()==5 && party->roster.at(kXeenCombatOwners[3]).currentHp==40 && party->roster.at(1).currentSp==20,"M39 native healthy First Aid automatic handoff");},[&]{return !cast();});
                    keys([key]{key(SDLK_c);key(SDLK_c,SDL_KEYUP);},[&]{check(cast()->phase==XeenCombatCastPhase::Confirm,"M39 native next actor Cast");});
                    keys([key]{key(SDLK_n);key(SDLK_n,SDL_KEYUP);},[&]{check(cast()->phase==XeenCombatCastPhase::Learned,"M39 native Awaken New");});
                    keys([key]{key(SDLK_1);key(SDLK_1,SDL_KEYUP);},[&]{check(cast()->selection==0,"M39 native Awaken row");});
                    keys([key]{key(SDLK_s);key(SDLK_s,SDL_KEYUP);},[&]{check(cast()->phase==XeenCombatCastPhase::Confirm && party->roster.at(6).currentSpell==0,"M39 native Awaken Select");});
                    keys([key]{key(SDLK_c);key(SDLK_c,SDL_KEYUP);},[&]{check(party->roster.at(6).currentSp==26 &&
                        (!cast() || cast()->phase==XeenCombatCastPhase::Preparing || cast()->phase==XeenCombatCastPhase::Result),
                        "M39 native paid Awaken debit");});
                    auto handoffLaps=std::make_shared<unsigned>(0);
                    steps.push_back([&,handoffLaps]{
                        check(++*handoffLaps<=500,"M39 native Awaken automatic handoff bound");
                        check(party->roster.at(6).currentSp==26,"M39 native Awaken handoff repeated debit");
                        return !cast();
                    });
                    settle();checkpoint("native");
                }else if(branch=="controls") {
                    // Independent original A prefix. No live fixture edits.
                    auto snapshot=std::make_shared<std::unique_ptr<XeenRestoreGuard>>();
                    const auto unchanged=[&,snapshot]{check((*snapshot)->current(),"M39 free cancellation changed durable state");check(combat()->participant()==4,"M39 free cancellation consumed turn");};
                    inspect([&,snapshot]{*snapshot=std::make_unique<XeenRestoreGuard>(*world,*party,*position,*flags);});
                    action(CastSpellAction{});deny();action(CancelInteractionAction{});inspect(unchanged);
                    action(CastSpellAction{});selectSpell(2);deny();
                    inspect([&,snapshot]{
                        check(party->roster.at(1).currentSpell==14,"M55 accepted First Aid slot is remembered");
                        *snapshot=std::make_unique<XeenRestoreGuard>(*world,*party,*position,*flags);
                    });
                    action(DialogKeyAction{'n'});action(CancelInteractionAction{});action(CancelInteractionAction{});
                    inspect([&,snapshot]{
                        // Accepting First Aid then cancelling New and Cast retains only its saved slot.
                        check((*snapshot)->current() && party->roster.at(1).currentSpell==14 && combat()->participant()==4,"M39 outer cancellation changes only remembered First Aid slot");
                    });
                    // Post-cost explicit Escape restores original SP, but settles this action.
                    action(CastSpellAction{});castSelected();
                    inspect([&]{check(party->roster.at(1).currentSp==20 && cast()->phase==XeenCombatCastPhase::PartyTarget,"M39 refund prompt/debit");});deny();
                    action(CancelInteractionAction{});result();
                    inspect([&]{check(cast()->result.refunded && party->roster.at(1).currentSp==21 && party->roster.at(6).currentHp==11,"M39 exact recorded refund");});
                    action(AcknowledgeAction{});untilActor(5,false);
                    inspect([&]{check(combat()->participant()==5,"M39 refund did not consume Rebecca turn");});
                    auto seymour=std::make_shared<std::unique_ptr<XeenRestoreGuard>>();
                    inspect([&,seymour]{*seymour=std::make_unique<XeenRestoreGuard>(*world,*party,*position,*flags);});
                    action(CastSpellAction{});selectSpell(2);
                    inspect([&,seymour]{*seymour=std::make_unique<XeenRestoreGuard>(*world,*party,*position,*flags);});
                    castSelected();
                    inspect([&,seymour]{check(cast()->phase==XeenCombatCastPhase::Confirm && !cast()->refusal.empty() && (*seymour)->current() && combat()->participant()==5,"M39 unsupported Light charged or consumed turn");});
                    action(AcknowledgeAction{});selectSpell(3);deny();
                    inspect([&,seymour]{
                        check(party->roster.at(6).currentSpell==25,"M55 accepted Arrow slot is remembered");
                        *seymour=std::make_unique<XeenRestoreGuard>(*world,*party,*position,*flags);
                    });
                    action(DialogKeyAction{'n'});action(CancelInteractionAction{});action(CancelInteractionAction{});
                    inspect([&,seymour]{
                        check((*seymour)->current() && party->roster.at(6).currentSpell==25 && combat()->participant()==5,"M39 Light/Arrow cancellation retains only remembered Arrow slot");
                    });
                    // Healthy Awaken still spends one SP and consumes Seymour's action.
                    action(CastSpellAction{});selectSpell(1);castSelected();result();
                    inspect([&]{check(cast()->result.noop && party->roster.at(6).currentSp==26,"M39 paid Awaken no-op");});
                    action(AcknowledgeAction{});settle();checkpoint("controls");
                    std::cout<<"M39 ORIGINAL CANCEL / REFUND / LIGHT / PAID NOOP CONTROLS\n";
                }else {
                    firstAid(5);
                    inspect([&]{check(party->roster.at(6).currentHp==15 && party->roster.at(1).currentSp==20,"M39 A First Aid publication");});
                    action(AcknowledgeAction{});untilActor(5,false);arrow(9);
                    inspect([&]{check(party->roster.at(6).currentSp==25 && world->sessionState().monsters(23)[9].hp==0 &&
                        world->sessionState().accountedMonsters().count({23,9}) && party->monsterTreasure->pendingGold==10,"M39 A lethal publication");
                        for(unsigned i=0;i<6;++i)check(party->roster.combatInputs(kXeenCombatOwners[i])->experience==(i==1 || i==4?2066u:1066u),"M39 Orc exact XP split");
                        check(party->encounterContext->minutes==510 && party->encounterContext->ctr24==2,"M39 cast charged time/ctr");
                        std::cout<<"M39 A ORC LETHAL: HP7->0 SP27->25 XP66 EACH GOLD10\n";});
                    action(AcknowledgeAction{});settle();checkpoint("A");
                }
            }else {
                route("LUUURUULURUULUUU",false);untilActor(5,true);
                inspect([&]{std::cout<<"M49 MONSTER PREFIX HP="<<world->sessionState().monsters(23)[15].hp<<" RNG="<<world->sessionState().journeyRandom()->count<<" minute="<<party->encounterContext->minutes<<'\n';
                    check(party->encounterContext->minutes==590 && world->sessionState().journeyRandom()->count==rngOffset+19 &&
                    world->sessionState().monsters(23)[15].hp==43,"M49 corrected single-target monster first-cycle prefix");});
                if(branch=="C") {
                    arrow(15);inspect([&]{check(world->sessionState().monsters(23)[15].hp==35 && party->roster.at(6).currentSp==25,"M39 C wound");std::cout<<"M49 C WOUND HP43->35 SP27->25\n";});
                    action(AcknowledgeAction{});settle();checkpoint("C");
                }else {
                    // M71: perform the healthy paid First Aid before the ordinary Sleep attack
                    // removes Rebecca from the turn order. Block until a real sleeper exists;
                    // the exact prefix below detects altered targeting, damage, or extra draws.
                    action(BlockAction{});untilActor(4,false);firstAid(4);
                    inspect([&]{check(party->roster.at(1).currentHp==21 && party->roster.at(1).currentSp==20 && cast()->result.noop,"M71 paid healthy First Aid: catches healing a healthy target or omitted SP debit");});action(AcknowledgeAction{});
                    steps.push_back([&]{const auto c=combat();check(c && c->phase()!=XeenCombatPhase::Defeat,"M71 sleep route lost combat");if(c->phase()!=XeenCombatPhase::PlayerReady)return false;bool sleeping=false;for(auto owner:kXeenCombatOwners)sleeping=sleeping || party->roster.at(owner).conditions[8];if(c->participant()==5 && sleeping)return true;act(BlockAction{});return false;});
                    inspect([&]{std::cout<<"M49 B PREFIX HP="<<world->sessionState().monsters(23)[15].hp<<" RNG="<<world->sessionState().journeyRandom()->count<<" minute="<<party->encounterContext->minutes<<" Rebecca="<<party->roster.at(1).currentHp<<" sleep="<<unsigned(party->roster.at(1).conditions[8])<<","<<unsigned(party->roster.at(14).conditions[8])<<","<<unsigned(party->roster.at(11).conditions[8])<<'\n';
                        check(party->encounterContext->minutes==593 && world->sessionState().journeyRandom()->count==rngOffset+43 &&
                        world->sessionState().monsters(23)[15].hp==43 && party->roster.at(1).conditions[8]==1 && party->roster.at(1).currentHp==0 &&
                        !party->roster.at(14).conditions[8] && !party->roster.at(11).conditions[8],"M49 B single-target Sleep prefix");});
                    action(CastSpellAction{});selectSpell(1);castSelected();result();
                    inspect([&]{check(party->roster.at(6).currentSp==26 && !party->roster.at(1).conditions[8] && party->roster.at(1).currentHp==0 &&
                        !party->roster.at(14).conditions[8] && !party->roster.at(11).conditions[8] &&
                        party->encounterContext->minutes==593 && world->sessionState().journeyRandom()->count==rngOffset+43,"M39 B Awaken");std::cout<<"M39 B AWAKEN Sleep1->0 unchanged RNG/time\n";});
                    action(AcknowledgeAction{});settle();checkpoint("B");
                }
            }
        }
        if(branch=="A" && stage!="city" && stage!="final" && stage!="reset") {
            route("DDLLULUU");action(InteractionAction{});action(YesAction{});settle();
            // First Slime: leave the ordinary contact for Seymour's learned Arrow.
            auto used=std::make_shared<bool>(false);
            for(const char symbol:std::string("URULUUULUUU")) {
                action(symbol=='U'?PlayerAction{NavigationAction::MoveForward}:symbol=='L'?PlayerAction{NavigationAction::TurnLeft}:PlayerAction{NavigationAction::TurnRight});
                steps.push_back([&,used]{
                    if(flow->canSave())return true;
                    if(const auto c=combat()) {
                        check(c->phase()!=XeenCombatPhase::Defeat && c->phase()!=XeenCombatPhase::Failed,"M39 city continuation stop");
                        if(!*used && c->phase()==XeenCombatPhase::PlayerReady) {
                            if(c->participant()!=5){act(BlockAction{});return false;}
                            act(CastSpellAction{});return false;
                        }
                        if(*used && !c->cast() && c->phase()==XeenCombatPhase::PlayerReady)act(AttackAction{});
                        if(c->cast()) {
                            const auto observed=c->cast();const auto &v=*observed;
                            if(v.phase==XeenCombatCastPhase::Learned)act(DialogKeyAction{unsigned(v.selection==2?'s':'3')});
                            else if(v.phase==XeenCombatCastPhase::Confirm)act(DialogKeyAction{unsigned(v.slot==25?'c':'n')});
                            else if(v.phase==XeenCombatCastPhase::Result) {
                                check(c->result().actorHpBefore==2 && c->result().actorHpAfter==0 && party->roster.at(6).currentSp==23,"M39 city Slime Arrow lethal");
                                *used=true;
                            }
                        }
                    }else if(world->sessionState().journeyActivity()==XeenJourneyActivity::Event || world->sessionState().journeyActivity()==XeenJourneyActivity::Reward)act(AcknowledgeAction{});
                    return false;
                });
            }
            inspect([&,used]{check(*used && position->mapId==XeenMapIdentity(28) && party->roster.at(6).currentSp==23,"M39 retained city casting witness missing");});checkpoint("city");
        }
        if(branch=="A" && stage=="reset") {
            route("DDLUUUU");
            inspect([&]{check(position->mapId==XeenMapIdentity(28) && position->x==15 && position->y==0 && position->direction==XeenDirection::South,"M39 original city exit route");});
            action(InteractionAction{});deny();action(NoAction{});settle();
            action(InteractionAction{});deny();action(YesAction{});settle();
            inspect([&]{check(position->mapId==XeenMapIdentity(23) && world->sessionState().regionalActors(28).size()==46 &&
                world->sessionState().accountedMonsters().count({23,9}) && world->sessionState().monsters(23)[9].hp==0 &&
                party->roster.at(1).currentSp==20 && party->roster.at(6).currentSp==23,"M39 city reset changed mainland/SP");});
            checkpoint("reset");route("LLU");action(InteractionAction{});action(YesAction{});settle();route("URULU");
            inspect([&]{check(position->mapId==XeenMapIdentity(28) && position->x==16 && position->y==2 &&
                world->sessionState().regionalActors(28).at(36).lifecycle==XeenActorLifecycle::Defeated &&
                world->sessionState().accountedMonsters().count({23,9}) && party->roster.at(6).currentSp==23,"M39 reset/revisit life and retained SP");});
            checkpoint("revisit");
        }
        if(branch=="A" && stage!="reset") {
            route("LR");
            auto before=std::make_shared<XeenGameplayContext>();auto cursor=std::make_shared<XeenJourneyRandomState>();
            inspect([&,before,cursor]{*before=*party->encounterContext;*cursor=*world->sessionState().journeyRandom();});
            action(CastSpellAction{});action(SelectMemberAction{5});selectSpell(3);
            inspect([&]{check(!flow->encounter()->castingCommitted() && party->roster.at(6).currentSp==23,"M60 exploration Arrow cancelled before commitment");});
            action(CancelInteractionAction{});settle();
            inspect([&,before,cursor]{check(*party->encounterContext==*before && *world->sessionState().journeyRandom()==*cursor,"M60 exploration Arrow selection cancellation charged time/RNG");});
            action(CastSpellAction{});action(SelectMemberAction{4});selectSpell(2);
            castSelected();action(SelectMemberAction{5});settle();
            inspect([&]{check(party->roster.at(1).currentSp==19 && party->roster.at(6).currentSp==23,"M39 exploration continuation SP");});
            action(CastSpellAction{});action(SelectMemberAction{4});selectSpell(1);castSelected();settle();
            inspect([&]{check(party->roster.at(1).currentSp==18 && party->roster.at(6).currentSp==23,"M39 inherited exploration Awaken");});
            checkpoint("final");
        }else if(branch=="B" || branch=="C") {route("LR");
            inspect([&]{const auto &toad=world->sessionState().monsters(23)[15];
                std::cout<<"M39 TOAD END HP="<<toad.hp<<" woken="<<toad.activated<<'\n';});
            checkpoint("continued");}
        inspect([&]{std::cout<<"M39 PRODUCTION WITNESS PASSED projectiles="<<arrowProjectiles<<'\n';SDL_Event e{};e.type=SDL_QUIT;SDL_PushEvent(&e);});
        auto native=handler;native.closed={};
        // The simulated clock advances only when idle runs; a live deadline would make
        // the SDL loop wait in real time for it. Behavior is unchanged, only the wait.
        native.nextPresentationDeadline=[]{return std::optional<std::uint64_t>{0};};
        native.beginCycle=[&](std::uint64_t){now+=100;handler.beginCycle(++cycle);};
        native.framePresented=[&](const auto &frame){handler.framePresented(frame);shown=true;if(cast() && cast()->phase==XeenCombatCastPhase::Projectile)++arrowProjectiles;};
        const auto drive=[&]()->std::optional<IndexedFrame>{
            check(++iterations<12000,"M39 witness iteration bound");next.reset();acted=false;
            if(shown)while(!steps.empty()){const bool done=steps.front()();if(done)steps.pop_front();if(acted || !done)break;}
            if(acted)return next;auto frame=idle();if(frame)shown=false;return frame;
        };
        const auto ok=original.show(first,native,escape,drive,status);
        if(branch=="native-fault") {
            check(!ok && nativeFailed && saves==3 && !flow->canSave(),"M39 native failed frame manufactured authority/save");
            const auto prior=XeenSaveFile::read(*target);
            check(prior.characters[1].currentSp==21 && prior.characters[6].currentSp==27 && prior.journey->random->count==rngOffset,"M39 native failure overwrote previous disk save");
            if(control.find("projectile")!=std::string::npos)check(party->roster.at(6).currentSp==25 && world->sessionState().monsters(23)[9].hp==7 && !world->sessionState().accountedMonsters().count({23,9}) && world->sessionState().journeyRandom()->count==rngOffset+12,"M49 failed projectile published HP/reward/RNG or refunded reservation");
            else check(party->roster.at(1).currentSp==20 && party->roster.at(6).currentHp==(control.find("result")!=std::string::npos?15:11),"M39 native failure changed committed cost/effect prefix");
            std::cout<<"M39 NATIVE FAILURE PREFIX PASSED "<<control<<'\n';return true;
        }
        return ok;
    };
    if(!resume && seededControl) {
        // The exact injury/casting failure controls use a current-format fixture.
        // Generate legitimate stock first, then select the detached combat cursor
        // before restore publishes any gameplay owner.
        auto setup=original;
        XeenWorld *initialWorld=nullptr;
        const XeenPartyState *initialParty=nullptr;
        const XeenCamera *initialCamera=nullptr;
        const XeenGameFlags *initialFlags=nullptr;
        setup.observeGameplay=[&](auto &w,auto &,const auto &p,const auto &c,const auto &f) {
            initialWorld=&w;initialParty=&p;initialCamera=&c;initialFlags=&f;
        };
        setup.show=[&](const auto &first,const auto &handler,const auto &,const auto &,const auto &) {
            handler.framePresented(first.presentation());
            auto fixture=XeenSaveState::capture(original.resources.signature,*initialParty,*initialCamera,*initialFlags,*initialWorld);
            fixture.journey->random=XeenJourneyRandomState{1,1,0};
            XeenSaveFile::write(*target,fixture);
            return true;
        };
        const auto prepared=realPlay(app,setup,camera,target,false,entry,7);
        check(prepared==0,"M39 current synthetic cursor fixture preparation failed");
        std::cout<<"M39 CURRENT-FORMAT SYNTHETIC CURSOR CONTROL\n";
        return realPlay(app,services,camera,target,true,XeenEncounterEntry::Ordinary,std::nullopt);
    }
    return realPlay(app,services,camera,target,resume,entry,seed);
}
