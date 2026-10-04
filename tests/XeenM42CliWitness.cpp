// Earned original-resource application witness. Every mutation comes from a
// successfully presented gameplay response; no live owner is rewritten.
#include "XeenProbeFired.h"
#include "app/Application.h"
#include "app/XeenGameplayServices.h"
#include "platform/XeenSaveFile.h"
#include "formats/xeen/XeenSaveFormat.h"
#include "games/xeen/XeenCharacterRules.h"
#include "games/xeen/XeenCombatRules.h"
#include "games/xeen/XeenRegionalRules.h"
#include "XeenRestoreReplayProbe.h"
#include "XeenM42Evidence.h"
#define SDL_MAIN_HANDLED
#include <SDL.h>
#include <deque>
#include <cstdlib>
#include <iostream>
using namespace mmodern;
namespace fs=std::filesystem;
using m42_test::check;
namespace {unsigned interestCalls=0,enemyConsumers=0,weaponConsumers=0,shootConsumers=0;bool observeConsumers=false,intactArmorConsumer=false,failUpload=false,failCopy=false,nativeFailed=false;}
extern "C" int __real_SDL_UpdateTexture(SDL_Texture *,const SDL_Rect *,const void *,int);
extern "C" int __wrap_SDL_UpdateTexture(SDL_Texture *t,const SDL_Rect *r,const void *p,int pitch) {probe_fired::hit("SDL_UpdateTexture");
    if(failUpload){failUpload=false;nativeFailed=true;return SDL_SetError("M42 injected native upload failure");}return __real_SDL_UpdateTexture(t,r,p,pitch);
}
extern "C" int __real_SDL_RenderCopy(SDL_Renderer *,SDL_Texture *,const SDL_Rect *,const SDL_Rect *);
extern "C" int __wrap_SDL_RenderCopy(SDL_Renderer *r,SDL_Texture *t,const SDL_Rect *s,const SDL_Rect *d) {probe_fired::hit("SDL_RenderCopy");
    if(failCopy){failCopy=false;nativeFailed=true;return SDL_SetError("M42 injected native copy failure");}return __real_SDL_RenderCopy(r,t,s,d);
}
#define ENEMY_SYMBOL "_ZN7mmodern24XeenEnemyAttackCandidateC1ERKSt5arrayINS_13XeenCharacterELy6EERKS1_INS_16XeenCombatInputsELy6EERKNS_17XeenMonsterRecordEjjRKS1_IbLy6EE"
#define PLAYER_SYMBOL "_ZN7mmodern27XeenPhysicalPlayerCandidateC1ERKNS_13XeenCharacterERKNS_16XeenCombatInputsERKNS_17XeenMonsterRecordEjjb"
// Executable-only probes of the actual combat constructors. They neither alter
// returned candidates nor provide gameplay callbacks or mutation authority.
struct RealConsumers {
    void enemy(const XeenConsequenceCharacters &,const XeenConsequenceInputs &,const XeenMonsterRecord &,unsigned,unsigned,const std::array<bool,6> &) asm("__real_" ENEMY_SYMBOL);
    void player(const XeenCharacter &,const XeenCombatInputs &,const XeenMonsterRecord &,unsigned,unsigned,bool) asm("__real_" PLAYER_SYMBOL);
};
struct ProbeConsumers {
    void enemy(const XeenConsequenceCharacters &,const XeenConsequenceInputs &,const XeenMonsterRecord &,unsigned,unsigned,const std::array<bool,6> &) asm("__wrap_" ENEMY_SYMBOL);
    void player(const XeenCharacter &,const XeenCombatInputs &,const XeenMonsterRecord &,unsigned,unsigned,bool) asm("__wrap_" PLAYER_SYMBOL);
};
void ProbeConsumers::enemy(const XeenConsequenceCharacters &c,const XeenConsequenceInputs &i,const XeenMonsterRecord &m,unsigned year,unsigned mask,const std::array<bool,6> &blocked) {probe_fired::hit("XeenEnemyAttackCandidate");
    if(observeConsumers) {
        ++enemyConsumers;const auto ac=XeenCharacterRules::combatArmorClass(c[1],i[1],{year});
        if(xeenSameItem(c[1].armor[4],{0,3,0,3}) && ac==11 && !blocked[1] && ac+10==21)intactArmorConsumer=true;
        std::cout<<"ENEMY_CONSUMER owner18 armor "<<unsigned(c[1].armor[4].material)<<':'<<unsigned(c[1].armor[4].id)<<':'<<unsigned(c[1].armor[4].state)<<':'<<unsigned(c[1].armor[4].frame)
            <<" AC "<<ac<<" blocked "<<blocked[1]<<" threshold "<<ac+(blocked[1]?c[1].currentLevel()/2+15:10)<<" mask "<<mask<<" HP "<<c[1].currentHp<<'\n';
    }
    reinterpret_cast<RealConsumers *>(this)->enemy(c,i,m,year,mask,blocked);
}
void ProbeConsumers::player(const XeenCharacter &c,const XeenCombatInputs &i,const XeenMonsterRecord &m,unsigned type,unsigned year,bool shoot) {probe_fired::hit("XeenPhysicalPlayerCandidate");
    if(observeConsumers && c.rosterId==18) {
        if(c.weapons[1].id==6 && c.weapons[1].frame==1)++weaponConsumers;
        if(shoot && xeenSameItem(c.weapons[1],{0,32,0,4}))++shootConsumers;
        std::cout<<"PLAYER_CONSUMER owner18 weapon "<<unsigned(c.weapons[1].material)<<':'<<unsigned(c.weapons[1].id)<<':'<<unsigned(c.weapons[1].state)<<':'<<unsigned(c.weapons[1].frame)<<" shoot "<<shoot<<'\n';
    }
    reinterpret_cast<RealConsumers *>(this)->player(c,i,m,type,year,shoot);
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
    const std::optional<fs::path> &target,bool resume,XeenEncounterEntry entry,std::optional<std::uint32_t> seed) {probe_fired::hit("Application::playGameplay");
    const std::string stage=std::getenv("MMODERN_M42_STAGE")?std::getenv("MMODERN_M42_STAGE"):"fresh";
    const std::string control=std::getenv("MMODERN_M42_CONTROL")?std::getenv("MMODERN_M42_CONTROL"):"";
    if(!resume && stage=="fresh")for(const char *probe:{"XEEN_REPLAY_COMMAND","XEEN_REPLAY_DRAW","XEEN_REPLAY_JOURNEY_CONSTRUCT","XEEN_REPLAY_REGIONAL_MOVE","XEEN_REPLAY_SERVICE","XEEN_REPLAY_TIME","XEEN_REPLAY_RETIRE","XEEN_REPLAY_MOVE","XEEN_REPLAY_EVENT_BEGIN","XEEN_REPLAY_FRESH_PUBLICATION_INITIALIZE","xeenPrepareBankInterest","XEEN_REPLAY_EQUIPMENT","XEEN_REPLAY_TRANSFER","XeenEnemyAttackCandidate","XeenPhysicalPlayerCandidate"})probe_fired::expect(probe);
    if(!resume && stage=="depleted")for(const char *probe:{"XEEN_REPLAY_COMMAND","XEEN_REPLAY_DRAW","XEEN_REPLAY_JOURNEY_CONSTRUCT","XEEN_REPLAY_REGIONAL_MOVE","XEEN_REPLAY_SERVICE","XEEN_REPLAY_TIME","XEEN_REPLAY_RETIRE","XEEN_REPLAY_MOVE","XEEN_REPLAY_EVENT_BEGIN","XEEN_REPLAY_FRESH_PUBLICATION_INITIALIZE","XeenEnemyAttackCandidate","XeenPhysicalPlayerCandidate"})probe_fired::expect(probe);
    auto services=original;XeenEventFlow *flow=nullptr;XeenWorld *world=nullptr;
    const XeenPartyState *party=nullptr;const XeenCamera *position=nullptr;const XeenGameFlags *flags=nullptr;
    unsigned providers=0,saves=0;std::uint64_t now=0,cycle=0;
    std::optional<unsigned> shotStart;bool shotTimeObserved=false;std::string shotDraws,expectedShotPrefix;
    services.clock=[&]{return now;};services.observeSaveStage=[&](auto){++saves;};
    const auto count=[&](auto fn){return [fn=std::move(fn),&providers](auto &&...args)->decltype(auto){++providers;return fn(std::forward<decltype(args)>(args)...);};};
#define M42_COUNT(field) if(services.field)services.field=count(services.field)
    M42_COUNT(resources.loadInitialParty);M42_COUNT(resources.loadInitialCharacters);M42_COUNT(resources.loadInitialContext);
    M42_COUNT(resources.loadEvents);M42_COUNT(resources.loadMonsterStatistics);M42_COUNT(resources.regionalManifest);M42_COUNT(resources.vertigoManifest);
    M42_COUNT(resources.loadInitialPurse);M42_COUNT(resources.loadInitialRegionalRecovery);M42_COUNT(resources.loadRegionalText);M42_COUNT(resources.loadLearnedSpellNames);
    M42_COUNT(resources.loadInitialBankBalances);M42_COUNT(maps);M42_COUNT(objects);M42_COUNT(texts);M42_COUNT(compose);M42_COUNT(npcDraw);
    M42_COUNT(validateEncounterSprite);M42_COUNT(validateCombatSprite);M42_COUNT(sampleJourneySeed);M42_COUNT(composeEncounter);
#undef M42_COUNT
    const auto composeEncounter=services.composeEncounter;
    services.composeEncounter=[&,composeEncounter](auto &w,const auto &p,const auto &c,auto phase,auto appearance) {
        auto result=composeEncounter(w,p,c,phase,appearance);
        if(shotStart && !shotTimeObserved && &w==world && &p==party && p.encounterContext->minutes>*shotStart) {
            check(p.encounterContext->minutes==*shotStart+10,"M42 ordinary Shoot publication did not charge exact10 minutes");
            shotTimeObserved=true;std::cout<<"SHOOT_TIME committed "<<*shotStart<<"->"<<p.encounterContext->minutes<<'\n';
        }
        return result;
    };
    services.observeGameplay=[&](auto &w,auto &,const auto &p,const auto &c,const auto &f){world=&w;party=&p;position=&c;flags=&f;};
    services.configureFlow=[&](auto &f,const auto &c) {
        original.configureFlow(f,c);flow=&f;f.drawSmithArt=count(f.drawSmithArt);
        f.smithBoundary=[&](XeenSmithBoundary boundary) {
            if(!nativeFailed && !control.empty() && ((boundary==XeenSmithBoundary::AfterAdmission && control.find("admission")!=std::string::npos) ||
                (boundary==XeenSmithBoundary::AfterPurchase && control.find("purchase")!=std::string::npos) ||
                (boundary==XeenSmithBoundary::AfterDeparture && control.find("departure")!=std::string::npos))) {
                failUpload=control.rfind("upload-",0)==0;failCopy=control.rfind("copy-",0)==0;
            }
            if(boundary==XeenSmithBoundary::AfterPurchase || boundary==XeenSmithBoundary::AfterDeparture)
                std::cout<<"PUBLICATION "<<int(boundary)<<' '<<party->encounterContext->day<<' '<<party->monsterTreasure->gold<<' '
                    <<world->sessionState().journeyRandom()->state<<':'<<world->sessionState().journeyRandom()->count<<'\n';
            if(boundary==XeenSmithBoundary::StockComplete || boundary==XeenSmithBoundary::BankPrepared)
                std::cout<<"PREPARATION departure "<<(boundary==XeenSmithBoundary::StockComplete?"stock-ready":"bank-ready")
                    <<" current-day "<<party->encounterContext->day<<" current-RNG "<<world->sessionState().journeyRandom()->state<<':'<<world->sessionState().journeyRandom()->count<<'\n';
            if(boundary==XeenSmithBoundary::Quote)
                std::cout<<"PREPARATION quote prepared current-day "<<party->encounterContext->day<<" purse "<<party->monsterTreasure->gold
                    <<" RNG "<<world->sessionState().journeyRandom()->state<<':'<<world->sessionState().journeyRandom()->count<<'\n';
        };
        f.reportInventory=[&](const auto &r){std::cout<<"TRANSFER "<<int(r.status)<<' '<<unsigned(r.sourceOwner)<<':'<<unsigned(r.destinationOwner)<<':'<<unsigned(r.destinationSlot)<<'\n';};
        f.reportEquipment=[&](const auto &r){std::cout<<"EQUIPMENT "<<int(r.status)<<'\n';};
    };
    replay_test::observeDraw=[&](auto lo,auto hi,auto value,auto cursor){
        const auto line="DRAW "+std::to_string(lo)+":"+std::to_string(hi)+":"+(value?std::to_string(*value):"rejected")+":"+std::to_string(cursor.state)+":"+std::to_string(cursor.count)+"\n";
        std::cout<<line;if(shotStart)shotDraws+=line;
    };
    services.show=[&](const IndexedFrame &first,const auto &handler,const auto &escape,const auto &idle,const auto &status) {
        check(flow && world && party && position && flags && target,"M42 production owners absent");
        check(world->sessionState().journey(),
            "M42/M43 fresh successor content not selected");
        std::deque<std::function<bool()>> steps;std::optional<IndexedFrame> next;bool shown=false,acted=false,cityFight=false;
        IndexedFrame::Presentation presented;unsigned iterations=0,nativeActions=0;
        const auto snapshot=[&]{return XeenSaveState::capture(original.resources.signature,*party,*position,*flags,*world);};
        const auto deny=[&] {
            check(!flow->canSave(),"M42 modal/service exposed Quiet");const auto p=providers,s=saves;
            handler.withDisplayedInput(SaveGameAction{},handler.displayedInput().value_or(0));
            check(p==providers && s==saves,"M42 denied F9 reached provider/capture/path/I/O");
        };
        const auto inputTrace=[&](const PlayerAction &a) {
            std::cout<<"INPUT "<<a.index();
            if(const auto m=std::get_if<SelectMemberAction>(&a))std::cout<<" member "<<m->partyIndex;
            if(const auto s=std::get_if<SelectInventorySlotAction>(&a))std::cout<<" physical-slot "<<s->slot;
            if(const auto n=std::get_if<NavigationAction>(&a))std::cout<<" navigation "<<int(*n);
            std::cout<<'\n';
        };
        const auto act=[&](PlayerAction a) {
            check(shown,"M42 action preceded presentation");const auto token=handler.displayedInput();check(bool(token),"M42 frame token absent");
            const bool wasService=world->sessionState().journeyActivity()==XeenJourneyActivity::Service;
            const auto origin=presented;inputTrace(a);next=handler.withPresentedInput(a,*token,origin);acted=true;shown=false;
            if(!flow->canSave()) {
                const auto p=providers,s=saves;handler.withPresentedInput(SaveGameAction{},*token,origin);
                if(wasService || world->sessionState().journeyActivity()==XeenJourneyActivity::Service)handler.withPresentedInput(a,*token,origin);
                check(p==providers && s==saves,"M42 stale/batched action or F9 reached work");
            }
        };
        const auto action=[&](PlayerAction a,bool native=false) {
            if(!native){steps.push_back([&,a]{act(a);return true;});return;}
            SDL_Keycode key=SDLK_UNKNOWN;
            if(std::holds_alternative<InteractionAction>(a))key=SDLK_SPACE;
            if(std::holds_alternative<AcknowledgeAction>(a))key=SDLK_RETURN;
            if(std::holds_alternative<CancelInteractionAction>(a))key=SDLK_ESCAPE;
            if(std::holds_alternative<YesAction>(a))key=SDLK_y;
            if(std::holds_alternative<NoAction>(a))key=SDLK_n;
            if(std::holds_alternative<BlockAction>(a))key=SDLK_b;
            if(std::holds_alternative<RevisitCompletedAction>(a))key=SDLK_r;
            if(const auto m=std::get_if<SelectMemberAction>(&a))key=SDLK_F1+int(m->partyIndex);
            if(const auto s=std::get_if<SelectInventorySlotAction>(&a))key=SDLK_1+int(s->slot);
            if(const auto n=std::get_if<NavigationAction>(&a))key=*n==NavigationAction::TurnRight?SDLK_RIGHT:SDLK_LEFT;
            check(key!=SDLK_UNKNOWN,"M42 native key mapping absent");
            auto phase=std::make_shared<unsigned>(0);auto token=std::make_shared<std::uint64_t>();
            steps.push_back([&,a,key,phase,token] {
                const auto send=[&](SDL_Keycode k,bool down,bool repeat=false) {SDL_Event e{};e.type=down?SDL_KEYDOWN:SDL_KEYUP;
                    e.key.keysym.sym=k;e.key.keysym.scancode=SDL_GetScancodeFromKey(k);e.key.timestamp=SDL_GetTicks()+1;e.key.repeat=repeat;
                    check(SDL_PushEvent(&e)==1,"M42 native key enqueue failed");if(down)acted=true;};
                const auto other=key==SDLK_RETURN?SDLK_ESCAPE:SDLK_RETURN;
                if(!*phase) {*token=*handler.displayedInput();inputTrace(a);send(key,true);send(key,true);send(key,true,true);send(other,true);++*phase;++nativeActions;return false;}
                if(*phase==1) {if(*handler.displayedInput()==*token)return false;
                    check(*handler.displayedInput()==*token+1,"M42 native batch crossed more than one phase");*token=*handler.displayedInput();
                    send(key,true);send(key,true,true);SDL_Event e{};e.type=SDL_WINDOWEVENT;e.window.event=SDL_WINDOWEVENT_EXPOSED;SDL_PushEvent(&e);++*phase;return false;}
                check(*handler.displayedInput()==*token,"M42 held/repeated key crossed fresh phase");
                send(key,false);send(other,false);return true;
            });
        };
        const auto inspect=[&](std::function<void()> fn){steps.push_back([fn]{fn();return true;});};
        const auto combat=[&]{return flow->encounter()->combat();};
        const auto settle=[&]{steps.push_back([&] {
            if(flow->canSave())return true;
            if(const auto c=combat()) {
                check(c->phase()!=XeenCombatPhase::Failed && c->phase()!=XeenCombatPhase::Defeat && c->phase()!=XeenCombatPhase::SupportStopped,"M42 combat stopped");
                if(c->phase()==XeenCombatPhase::PlayerReady) {
                    if(stage=="shoot")std::cout<<"OPERATION post-Shoot Combat Run owner"<<unsigned(kXeenCombatOwners[c->participant()])<<" minute"<<party->encounterContext->minutes<<'\n';
                    act(stage!="shoot" && (cityFight || c->participant()==1 || c->participant()==4)?PlayerAction{AttackAction{}}:PlayerAction{RunAction{}});
                }
            } else if(world->sessionState().journeyActivity()==XeenJourneyActivity::Event || world->sessionState().journeyActivity()==XeenJourneyActivity::Reward)act(AcknowledgeAction{});
            check(flow->encounter()->state().phase()!=XeenEncounterPhase::SupportStopped,"M42 route stopped");return false;
        });};
        const auto route=[&](const std::string &path) {for(char key:path){action(key=='U'?NavigationAction::MoveForward:key=='D'?NavigationAction::MoveBackward:key=='R'?NavigationAction::TurnRight:NavigationAction::TurnLeft);settle();}};
        const auto expect=[&](unsigned day,unsigned minute,std::uint32_t gold,std::uint32_t state,std::uint64_t count) {
            check(party->encounterContext->day==day && party->encounterContext->minutes==minute && party->monsterTreasure->gold==gold &&
                world->sessionState().journeyRandom()->state==state && world->sessionState().journeyRandom()->count==count,"M42 literal context/purse/RNG checkpoint differs");
            check(party->serviceEconomy->bank.gold==0 && party->serviceEconomy->bank.gems==0,"M42 production bank differs");
        };
        const auto checkpoint=[&](std::string label) {
            auto old=std::make_shared<unsigned>();inspect([&,old]{check(flow->canSave(),"M42 checkpoint is not Quiet");*old=saves;
                SDL_Event e{};e.type=SDL_KEYDOWN;e.key.keysym.sym=SDLK_F9;e.key.keysym.scancode=SDL_SCANCODE_F9;e.key.timestamp=SDL_GetTicks()+1;
                check(SDL_PushEvent(&e)==1,"M42 F9 enqueue failed");acted=true;});
            steps.push_back([&,old,label] {if(saves==*old)return false;check(saves==*old+3,"M42 actual save pipeline incomplete");
                SDL_Event e{};e.type=SDL_KEYUP;e.key.keysym.sym=SDLK_F9;e.key.keysym.scancode=SDL_SCANCODE_F9;SDL_PushEvent(&e);
                const auto actual=snapshot(),disk=XeenSaveFile::read(*target);m40_test::equalFields(actual,disk);
                check(XeenSaveFormat::encode(actual)==XeenSaveFormat::encode(disk),"M42 saved bytes differ");
                fs::copy_file(*target,target->parent_path()/(target->stem().string()+"-"+label+".mmsave"),fs::copy_options::overwrite_existing);
                std::cout<<"MARK "<<label<<'\n';return true;
            });
        };
        const auto waitService=[&]{steps.push_back([&]{deny();return world->sessionState().journeyActivity()==XeenJourneyActivity::Service;});};
        const auto ac=[&]{return XeenCharacterRules::combatArmorClass(party->roster.at(18),*party->roster.combatInputs(18),{610});};
        auto beforePurchase=std::make_shared<XeenSaveSnapshot>();
        inspect([&] {
            if(resume) {
                check(!replay_test::journeyInitializations && !replay_test::journeyConstructions && !replay_test::actions && !replay_test::pulses &&
                    !replay_test::commands && !replay_test::draws && !replay_test::retirements && !replay_test::timePreparations &&
                    !replay_test::eventExecutions && !replay_test::transfers && !replay_test::equipmentChanges && !interestCalls,"M42 restore replayed gameplay before input");
                const auto actual=snapshot(),disk=XeenSaveFile::read(*target);m40_test::equalFields(actual,disk);
                check(XeenSaveFormat::encode(actual)==XeenSaveFormat::encode(disk),"M42 pre-input restore differs");
                std::cout<<"RESTORE EXACT BEFORE INPUT\nMARK "<<stage<<'\n';
            } else expect(8,480,800,1652828136,901);
        });
        if(stage=="shoot") {
            inspect([&,beforePurchase]{*beforePurchase=snapshot();expect(11,803,670,2959920300u,2009);check(xeenSameItem(party->serviceEconomy->wares[0][0][0][5],{0,32,0,0}),"M42 actual D stock has no literal missile32 offer");});
            action(InteractionAction{});waitService();action(BlockAction{},true);action(SelectInventorySlotAction{5},true);action(AcknowledgeAction{},true);action(AcknowledgeAction{},true);
            inspect([&,beforePurchase]{deny();expect(11,803,620,2959920300u,2009);check(xeenSameItem(party->roster.at(0).weapons[1],{0,32,0,0}),"M42 actual missile purchase physical delivery differs");
                auto expected=*beforePurchase->journey->serviceEconomy;expected.wares[0][0][0]=m42_test::restockedWeaponsAfterMissile();check(expected==*party->serviceEconomy,"M42 missile purchase changed untouched stock/bank or depleted wrong quantity");
                for(unsigned owner=0;owner<30;++owner){auto c=beforePurchase->characters[owner];if(owner==0)c.weapons[1]={0,32,0,0};check(xeen_state::sameCharacter(c,party->roster.at(owner)) && xeen_state::sameInputs(beforePurchase->journey->supplements[owner].inputs,*party->roster.combatInputs(owner)),"M42 missile purchase changed other owner fields");}
                std::cout<<"PREPARATION purchase committed Weapons physical5 expected-price50\nOPERATION Buy Weapons physical5 price50 payment "<<beforePurchase->journey->treasure->gold<<"->"<<party->monsterTreasure->gold<<" delivery0:1 raw0:32:0:0 depletion8->7 stable\n";
            });
            action(AcknowledgeAction{},true);action(CancelInteractionAction{},true);action(CancelInteractionAction{},true);settle();
            inspect([&]{expect(12,803,620,2959920300u,2009);check(!interestCalls,"M42 missile visit repeated restock interest");});checkpoint("S");
            action(InspectInventoryAction{});action(SelectInventorySlotAction{1});action(SelectMemberAction{1});
            action(SelectMemberAction{1});action(SelectInventorySlotAction{1});action(EquipmentInventoryAction{});
            inspect([&]{check(xeenSameItem(party->roster.at(18).weapons[0],{0,2,0,1}) && xeenSameItem(party->roster.at(18).weapons[1],{0,32,0,4}),"M42 ordinary missile equip altered melee weapon or failed");});
            action(CancelInteractionAction{});settle();checkpoint("S1");
            route("RRUUUUUUURUUUU");action(InteractionAction{});action(YesAction{});settle();
            inspect([&]{const auto &map=world->map(23);for(unsigned y=3;y<=8;++y){const auto &cell=map.geometry.cells[y*16+5];const auto *l=xeenGetIf<XeenOutdoorLayers>(&cell.geometry);check(cell.rawWord==7 && cell.rawAttributes==0 && l && l->surface==7 && l->middle==0 && map.geometry.surfaceTypes[7]==7,"M42 original road/automatic-free Shoot approach differs");}
                const auto &a=world->sessionState().actors()[15];check(a.x==7 && a.y==3 && a.hp==90 && a.lifecycle==XeenActorLifecycle::Present && !a.activated,"M42 admitted original Giant Toad15 pre-approach differs");});
            // Reach the original road without entering its automatic sign cell.
            // Shoot retains intent while the existing pending approach finishes.
            route("UUURUUUULURULUUU");
            inspect([&]{observeConsumers=true;check(!combat() && flow->encounter()->state().phase()==XeenEncounterPhase::Exploring && flow->journeyInputCurrent(handler.displayedInput()) && handler.acceptsInputFrame(presented),"M42 Shoot input did not have current Exploring concrete-frame authority");check(position->mapId==XeenMapIdentity(23) && position->x==5 && position->y==5 && position->direction==XeenDirection::South,"M42 literal admitted Toad shooting position differs");const auto &a=world->sessionState().actors()[15];const auto pending=flow->encounter()->state().pending();
                std::cout<<"SHOOT_APPROACH actor15 "<<a.x<<':'<<a.y<<" HP"<<a.hp<<" activated"<<a.activated<<" pending"<<pending<<'\n';
                // The first charged step after activation moves7->6. Its owed
                // final opportunity moves6->5; acquisition can follow either.
                check(a.x==(pending?6:5) && a.y==3 && a.hp==90 && a.activated && a.lifecycle==XeenActorLifecycle::Present,"M42 independent Toad15 pending/settled approach relation differs");const auto &map=world->map(23);for(unsigned y=2;y<=4;++y){const auto &cell=map.geometry.cells[y*16+5];const auto *l=xeenGetIf<XeenOutdoorLayers>(&cell.geometry);check(cell.rawWord==7 && cell.rawAttributes==0 && l && l->surface==7 && l->middle==0,"M42 independent literal road ray geometry differs");}check(xeenPlayerRayRows(map,*position)==4,"M42 consumer ray disagrees with independent clear middle0 road geometry");shotStart=party->encounterContext->minutes;
                const auto cursor=*world->sessionState().journeyRandom();m42_test::StockOracle independent{cursor.state,cursor.count,{}};for(unsigned die=0;die<4;++die)independent.draw(1,2);independent.draw(1,20);expectedShotPrefix=independent.trace;
                std::cout<<"PREPARATION Shoot inherited-pending-approach "<<flow->encounter()->state().pending()<<" current-RNG "<<cursor.state<<':'<<cursor.count<<'\n';
                std::cout<<"SHOOT_RAY "<<position->x<<':'<<position->y<<':'<<unsigned(position->direction)<<" rows4 start-minute "<<*shotStart<<'\n';});action(ShootAction{});settle();
            inspect([&]{std::cout<<"SHOOT_RESULT minute "<<party->encounterContext->minutes<<" consumer-count "<<shootConsumers<<'\n';check(shootConsumers && shotTimeObserved && shotDraws.rfind(expectedShotPrefix,0)==0,"M42 purchased missile32 did not reach actual Shoot consumer with independently expected4d2/hit draws and exact10 minute publication");});checkpoint("SE");
        }
        if(stage!="shoot") {
        if(!resume) {
            route("RRRUUURUURRRU");for(unsigned member:{0u,1u,2u,3u,5u}) {action(InteractionAction{});action(SelectMemberAction{member});settle();}
            route("RRUUUU");route("URRRUU");route("UU");route("UURU");route("RRURRRUUUUUUUUURRRU");
            inspect([&]{expect(8,781,870,2762662790u,1060);check(party->roster.combatInputs(18)->experience==3264,"M42 earned XP prefix differs");});
            action(CastSpellAction{});action(SelectMemberAction{4});action(NavigationAction::MoveBackward);
            action(AcknowledgeAction{});action(AcknowledgeAction{});action(SelectMemberAction{4});settle();
            inspect([&]{check(party->roster.at(1).currentHp==15 && party->roster.at(1).currentSp==20,"M42 genuine First Aid differs");cityFight=true;});
            action(InteractionAction{});action(YesAction{});settle();route("UUUU");
            inspect([&]{expect(8,796,870,799325555,1101);});route("LUUUUUUU");
            inspect([&]{expect(8,803,870,799325555,1101);check(position->mapId==28 && position->x==8 && position->y==4 && position->direction==XeenDirection::West && ac()==10,"M42 pre-visit route/AC differs");});checkpoint("A");
        }
        if(!resume || stage=="A" || stage=="weapons") {
            inspect([&,beforePurchase]{*beforePurchase=snapshot();m42_test::sameCategory(party->serviceEconomy->wares[0][0][0],m42_test::weaponsBefore());m42_test::sameCategory(party->serviceEconomy->wares[0][0][1],m42_test::armorBefore());});
            action(InteractionAction{});waitService();action(SelectMemberAction{1},true);action(SelectMemberAction{0},true);action(BlockAction{},true);
            if(stage=="weapons") {
                const auto paidWeapon=[&,beforePurchase](unsigned ordinal,unsigned physical) {
                    deny();expect(8,803,870-60*ordinal,799325555,1101);
                    for(unsigned slot=1;slot<=ordinal;++slot)check(xeenSameItem(party->roster.at(0).weapons[slot],{0,6,0,0}),"M42 paid Weapon6 physical delivery differs");
                    std::cout<<"PREPARATION purchase committed Weapons physical"<<physical<<" expected-price60\nOPERATION Buy Weapons physical"<<physical
                        <<" price60 payment "<<beforePurchase->journey->treasure->gold-60*(ordinal-1)<<"->"<<party->monsterTreasure->gold
                        <<" delivery0:"<<ordinal<<" raw0:6:0:0 depletion"<<9-ordinal<<"->"<<8-ordinal<<" stable\n";
                };
                action(SelectInventorySlotAction{1},true);action(AcknowledgeAction{},true);action(AcknowledgeAction{},true);inspect([paidWeapon]{paidWeapon(1,1);});action(AcknowledgeAction{},true);
                // The second byte-identical weapon-6 quantity shifted from 4 to 3.
                action(SelectInventorySlotAction{3},true);action(AcknowledgeAction{},true);action(AcknowledgeAction{},true);inspect([paidWeapon]{paidWeapon(2,3);});action(AcknowledgeAction{},true);
                action(CancelInteractionAction{},true);action(CancelInteractionAction{},true);settle();
                inspect([&,beforePurchase]{expect(9,803,750,799325555,1101);check(xeenSameItem(party->roster.at(0).weapons[1],{0,6,0,0}) && xeenSameItem(party->roster.at(0).weapons[2],{0,6,0,0}),"M42 repeated weapon purchase delivery differs");
                    auto expected=*beforePurchase->journey->serviceEconomy;expected.wares[0][0][0]=m42_test::weaponsAfterTwo();check(expected==*party->serviceEconomy,"M42 duplicate weapon purchases changed untouched stock/bank");
                });checkpoint("W");
                action(InspectInventoryAction{});action(DialogKeyAction{'a'});action(DialogKeyAction{'w'});action(SelectInventorySlotAction{1});
                action(SelectMemberAction{1});
                action(SelectMemberAction{1});action(SelectInventorySlotAction{1});action(EquipmentInventoryAction{});
                inspect([&]{check(xeenSameItem(party->roster.at(18).weapons[0],{0,2,0,1}) && xeenSameItem(party->roster.at(18).weapons[1],{0,6,0,0}),"M42 conflicting melee equip was accepted");});
                action(AcknowledgeAction{});action(SelectInventorySlotAction{0});action(DialogKeyAction{'r'});action(SelectInventorySlotAction{1});action(EquipmentInventoryAction{});action(CancelInteractionAction{});settle();
                inspect([&]{check(xeenSameItem(party->roster.at(18).weapons[1],{0,6,0,1}),"M42 weapon legal equip differs");const auto d=xeenOrdinaryWeaponDice(6);check(d.count==4 && d.sides==2,"M42 weapon-6 independent 4d2 differs");});checkpoint("W1");
            } else {
                action(NavigationAction::TurnRight,true);action(SelectInventorySlotAction{3},true);action(AcknowledgeAction{},true);inspect(deny);action(CancelInteractionAction{},true);
                inspect([&,beforePurchase]{for(unsigned owner=0;owner<30;++owner)check(xeen_state::sameCharacter(beforePurchase->characters[owner],party->roster.at(owner)) && xeen_state::sameInputs(beforePurchase->journey->supplements[owner].inputs,*party->roster.combatInputs(owner)),"M42 cancelled quote changed owner");check(*party->serviceEconomy==*beforePurchase->journey->serviceEconomy,"M42 cancelled quote changed wares");expect(8,803,870,799325555,1101);std::cout<<"PREPARATION purchase discarded Armor physical3 expected-price200\nOPERATION Cancel Armor physical3 payment "<<beforePurchase->journey->treasure->gold<<"->"<<party->monsterTreasure->gold<<" delivery-none depletion-none\n";});
                action(SelectInventorySlotAction{4},true);action(SelectInventorySlotAction{3},true);action(AcknowledgeAction{},true);action(AcknowledgeAction{},true);inspect(deny);
                inspect([&,beforePurchase]{expect(8,803,670,799325555,1101);m42_test::sameCategory(party->serviceEconomy->wares[0][0][1],m42_test::armorAfter());
                    check(xeenSameItem(party->roster.at(0).armor[5],{0,3,0,0}),"M42 unequipped physical Armor delivery differs");
                    auto expected=*beforePurchase->journey->serviceEconomy;expected.wares[0][0][1]=m42_test::armorAfter();check(expected==*party->serviceEconomy,"M42 purchase changed untouched economy");
                    for(unsigned owner=0;owner<30;++owner){auto c=beforePurchase->characters[owner];if(owner==0)c.armor[5]={0,3,0,0};check(xeen_state::sameCharacter(c,party->roster.at(owner)) && xeen_state::sameInputs(beforePurchase->journey->supplements[owner].inputs,*party->roster.combatInputs(owner)),"M42 purchase changed other owner fields");}
                    std::cout<<"PREPARATION purchase committed Armor physical3 expected-price200\nOPERATION Buy Armor physical3 price200 payment "<<beforePurchase->journey->treasure->gold<<"->"<<party->monsterTreasure->gold<<" delivery0:5 raw0:3:0:0 depletion7->6 stable\n";
                });
                action(AcknowledgeAction{},true);action(SelectInventorySlotAction{0},true);action(AcknowledgeAction{},true);action(AcknowledgeAction{},true);
                inspect([&]{expect(8,803,670,799325555,1101);m42_test::sameCategory(party->serviceEconomy->wares[0][0][1],m42_test::armorAfter());});action(AcknowledgeAction{},true);
                action(CancelInteractionAction{},true);action(CancelInteractionAction{},true);settle();
                inspect([&]{expect(9,803,670,799325555,1101);check(!interestCalls,"M42 nontrigger departure applied interest");});checkpoint("B");
            }
        }
        if(stage!="depleted" && stage!="weapons" && (!resume || stage=="A" || stage=="B")) {
            action(InspectInventoryAction{});action(DialogKeyAction{'a'});action(SelectInventorySlotAction{5});action(SelectMemberAction{1});
            action(SelectMemberAction{1});action(SelectInventorySlotAction{4});action(EquipmentInventoryAction{});
            inspect([&]{check(xeenSameItem(party->roster.at(18).armor[0],{0,2,0,3}) && xeenSameItem(party->roster.at(18).armor[4],{0,3,0,0}) && ac()==10,"M42 conflicting body equip was accepted");});
            action(AcknowledgeAction{});action(SelectInventorySlotAction{0});action(DialogKeyAction{'r'});action(SelectInventorySlotAction{4});action(EquipmentInventoryAction{});
            inspect([&]{check(xeenSameItem(party->roster.at(18).armor[0],{0,2,0,0}) && xeenSameItem(party->roster.at(18).armor[4],{0,3,0,3}) && ac()==11 && party->roster.at(18).currentHp==67 && party->roster.at(18).currentSp==0,"M42 legal remove/equip +1AC changed HP/SP or failed");std::cout<<"UPGRADE Armor strength 4->5 AC 10->11; nonblocked threshold 20->21\n";});
            action(CancelInteractionAction{});settle();checkpoint("B1");
        }
        if(stage!="depleted") {
        if(stage!="weapons" && (!resume || stage=="A" || stage=="B" || stage=="B1")) {
            action(InteractionAction{});waitService();action(BlockAction{},true);action(NavigationAction::TurnRight,true);action(SelectInventorySlotAction{0},true);
            action(AcknowledgeAction{},true);action(CancelInteractionAction{},true);action(CancelInteractionAction{},true);
            // Existing repair mode coexists without admitting another departure.
            action(RevisitCompletedAction{},true);action(CancelInteractionAction{},true);action(CancelInteractionAction{},true);settle();
            inspect([&]{expect(10,803,670,799325555,1101);m42_test::sameCategory(party->serviceEconomy->wares[0][0][1],m42_test::armorAfter());});checkpoint("C");
        }
        if(stage!="weapons" && stage!="D") {
            action(InteractionAction{});waitService();action(CancelInteractionAction{},true);settle();
            inspect([&]{expect(11,803,670,2959920300u,2009);check(interestCalls==1,"M42 restock interest duplicated or skipped");m42_test::StockOracle oracle{799325555,1101,{}};const auto expected=oracle.generate();check(m40_test::stockBytes(expected)==m40_test::stockBytes(*party->serviceEconomy) && oracle.state==2959920300u && oracle.count==2009,"M42 full restock differs from independent generation oracle");std::cout<<"PREPARATION departure committed restock-all1152 day10->11 RNG799325555:1101->"<<world->sessionState().journeyRandom()->state<<':'<<world->sessionState().journeyRandom()->count<<" interest-calls"<<interestCalls<<'\n';});checkpoint("D");
        }
        // Original city exit/prelude/reset and real subsequent entrance combat.
        route("RRUUUUUUURUUUU");action(InteractionAction{});action(YesAction{});settle();route("RRU");action(InteractionAction{});action(YesAction{});settle();
        auto contact=std::make_shared<bool>(false),consumer=std::make_shared<bool>(false);
        inspect([&]{observeConsumers=true;});
        steps.push_back([&,contact,consumer] {
            if(flow->canSave()) {if(*contact)return true;check(position->y<4,"M42 reset Slime contact missing");act(NavigationAction::MoveForward);return false;}
            if(const auto c=combat()) {
                *contact=true;check(c->phase()!=XeenCombatPhase::Failed && c->phase()!=XeenCombatPhase::Defeat && c->phase()!=XeenCombatPhase::SupportStopped,"M42 continued combat failed");
                if(c->phase()==XeenCombatPhase::PlayerReady) {
                    if(!*consumer) {const auto value=ac();if(stage!="weapons")check(value==11,"M42 intact upgrade did not reach continued combat input");
                        std::cout<<"COMBAT_INPUT owner18 AC "<<value<<" level "<<party->roster.at(18).currentLevel()<<" nonblocked "<<value+10<<" blocked "<<value+party->roster.at(18).currentLevel()/2+15<<'\n';*consumer=true;}
                    act(stage=="weapons" && c->participant()!=1?PlayerAction{BlockAction{}}:PlayerAction{AttackAction{}});
                }
            } else if(world->sessionState().journeyActivity()==XeenJourneyActivity::Event || world->sessionState().journeyActivity()==XeenJourneyActivity::Reward)act(AcknowledgeAction{});
            return false;
        });
        inspect([&,contact,consumer]{check(*contact && *consumer && enemyConsumers,"M42 actual enemy constructor consumer evidence absent");if(stage=="weapons")check(weaponConsumers,"M42 actual weapon6 melee consumer not reached");else check(intactArmorConsumer,"M42 actual enemy constructor never consumed intact upgraded Armor3 AC11 nonblocked threshold21");check(flow->canSave(),"M42 continued combat not settled");});checkpoint(stage=="weapons"?"WE":"E");
        }
        }
        inspect([&]{std::cout<<(stage=="depleted"?"M43 DEPLETED BUY PREFIX PASSED; native modal actions ":
            "M42 PRODUCTION WITNESS PASSED; native modal actions ")<<nativeActions<<'\n';SDL_Event e{};e.type=SDL_QUIT;SDL_PushEvent(&e);});
        auto native=handler;native.closed={};native.beginCycle=[&](std::uint64_t){handler.beginCycle(++cycle);};unsigned evidenceFrame=0;
        native.framePresented=[&](const auto &frame) {
            check(!nativeFailed,"M42 failed native frame acquired authority");
            if(frame!=presented && !flow->canSave()){const auto p=providers,s=saves;handler.withPresentedInput(SaveGameAction{},handler.displayedInput().value_or(0),frame);check(p==providers && s==saves,"M42 unacquired handoff F9 reached provider/I/O");}
            handler.framePresented(frame);presented=frame;shown=true;
            if(stage!="depleted" && (world->sessionState().journeyActivity()==XeenJourneyActivity::Service || flow->inventoryOpen())) {
                std::vector<std::uint32_t> pixels;pixels.reserve(frame->pixels.size());for(auto i:frame->pixels){const auto n=i*3;pixels.push_back(0xff000000u|(frame->palette[n]<<16)|(frame->palette[n+1]<<8)|frame->palette[n+2]);}
                auto *surface=SDL_CreateRGBSurfaceFrom(pixels.data(),320,200,32,1280,0xff0000,0xff00,0xff,0xff000000);check(surface,"M42 evidence surface absent");
                const auto suffix=flow->inventoryOpen() && flow->inventorySelection().sourceOwner==18 && flow->inventorySelection().category==XeenInventoryCategory::Armor && ac()==11?
                    std::string("-upgrade-ac11.bmp"):(flow->inventoryOpen()?"-inventory-":"-smith-")+std::to_string(++evidenceFrame)+".bmp";
                const auto file=target->parent_path()/(target->stem().string()+suffix);const auto saved=SDL_SaveBMP(surface,file.string().c_str());SDL_FreeSurface(surface);check(saved==0,"M42 native frame write failed");
            }
        };
        const auto drive=[&]()->std::optional<IndexedFrame> {
            check(++iterations<40000,"M42 witness iteration bound");now+=100;next.reset();acted=false;
            if(shown && iterations%3!=0 && handler.acceptsInputFrame(presented) && flow->journeyInputCurrent(handler.displayedInput()))
                while(!steps.empty()){const bool done=steps.front()();if(done)steps.pop_front();if(acted || !done)break;}
            if(acted)return next;auto frame=idle();if(frame)shown=false;return frame;
        };
        const bool ok=original.show(first,native,escape,drive,status);
        if(!control.empty()) {
            const bool admitted=control.find("admission")!=std::string::npos,departed=control.find("departure")!=std::string::npos;
            check(nativeFailed && !ok && !flow->canSave(),"M42 native failure exposed Quiet or missed failure");
            expect(departed?9:8,803,admitted?870:670,799325555,1101);
            m42_test::sameCategory(party->serviceEconomy->wares[0][0][1],admitted?m42_test::armorBefore():m42_test::armorAfter());
            check(xeenSameItem(party->roster.at(0).armor[5],admitted?XeenSaveFile::read(*target).characters[0].armor[5]:XeenItem{0,3,0,0}),"M42 native failure lost delivery prefix");
            deny();std::cout<<"M42 NATIVE FAILURE PRESERVATION PASSED\n";
        }
        return ok;
    };
    return realPlay(app,services,camera,target,resume,entry,seed);
}
