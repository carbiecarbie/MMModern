// Original-resource M43 witness: all party, combat, route and Temple mutations
// arise from the production application after concrete presented-frame input.
#include "XeenProbeFired.h"
#include "XeenM49Trace.h"
#include "XeenM49CombatSupport.h"
#include "app/Application.h"
#include "app/XeenGameplayServices.h"
#include "platform/XeenSaveFile.h"
#include "formats/xeen/XeenSaveFormat.h"
#include "games/xeen/XeenRegionalRules.h"
#include "games/xeen/XeenTempleHeal.h"
#include "XeenM40Evidence.h"
#include "XeenM42Evidence.h"
#include "XeenTrainingTestSupport.h"
#include "XeenPurchaseTestSupport.h"
#define SDL_MAIN_HANDLED
#include <SDL.h>
#include <deque>
#include <cstdlib>
#include <fstream>
#include <iostream>
#include <stdexcept>
using namespace mmodern;
namespace fs=std::filesystem;
using m40_test::check;
namespace {
bool observeStockDraws=false;
std::string observedStockDraws;
std::uint32_t crc32(const std::vector<std::uint8_t> &bytes){
    std::uint32_t crc=0xffffffffu;
    for(auto byte:bytes){crc^=byte;for(unsigned bit=0;bit<8;++bit)crc=(crc>>1)^((crc&1)?0xedb88320u:0u);}
    return crc^0xffffffffu;
}
struct QuietCliOutput {
    std::ofstream sink{"NUL"};
    std::streambuf *original=std::cout.rdbuf(sink.rdbuf());
    ~QuietCliOutput(){std::cout.rdbuf(original);}
};
}
struct RealRandom {
    std::optional<std::uint32_t> draw(std::uint32_t,std::uint32_t) asm("__real__ZN7mmodern16XeenCombatRandom4drawEjj");
};
struct ProbeRandom {
    std::optional<std::uint32_t> draw(std::uint32_t,std::uint32_t) asm("__wrap__ZN7mmodern16XeenCombatRandom4drawEjj");
};
std::optional<std::uint32_t> ProbeRandom::draw(std::uint32_t lo,std::uint32_t hi) {probe_fired::hit("XeenCombatRandom::draw");
    auto result=reinterpret_cast<RealRandom *>(this)->draw(lo,hi);
    m49_trace::draw(lo,hi,result,reinterpret_cast<XeenCombatRandom *>(this)->continuation());
    if(observeStockDraws) {
        const auto cursor=reinterpret_cast<XeenCombatRandom *>(this)->continuation();
        observedStockDraws+="DRAW "+std::to_string(lo)+":"+std::to_string(hi)+":"+
            (result?std::to_string(*result):"rejected")+":"+std::to_string(cursor.state)+":"+
            std::to_string(cursor.count)+"\n";
    }
    return result;
}
#define PLAY_SYMBOL "_ZNK7mmodern11Application12playGameplayERKNS_20XeenGameplayServicesENS_10XeenCameraERKSt8optionalINSt10filesystem7__cxx114pathEEbNS_18XeenEncounterEntryES5_IjE"
extern "C" int realPlay(const Application *,const XeenGameplayServices &,XeenCamera,const std::optional<fs::path> &,bool,XeenEncounterEntry,std::optional<std::uint32_t>) asm("__real_" PLAY_SYMBOL);
extern "C" int wrappedPlay(const Application *,const XeenGameplayServices &,XeenCamera,const std::optional<fs::path> &,bool,XeenEncounterEntry,std::optional<std::uint32_t>) asm("__wrap_" PLAY_SYMBOL);
static const probe_fired::Expect playProbe{"Application::playGameplay"};
extern "C" int wrappedPlay(const Application *app,const XeenGameplayServices &original,XeenCamera camera,
    const std::optional<fs::path> &target,bool resume,XeenEncounterEntry entry,
    std::optional<std::uint32_t> seed) {probe_fired::hit("Application::playGameplay");
    QuietCliOutput quietOutput;
    const std::string scenario=std::getenv("MMODERN_M44_SCENARIO")?std::getenv("MMODERN_M44_SCENARIO"):"mainland";
    const std::string stage=std::getenv("MMODERN_M43_STAGE")?std::getenv("MMODERN_M43_STAGE"):"main";
    auto services=original;XeenEventFlow *flow=nullptr;XeenWorld *world=nullptr;
    const XeenPartyState *party=nullptr;const XeenCamera *position=nullptr;const XeenGameFlags *flags=nullptr;
    std::uint64_t now=0,cycle=0;
    services.clock=[&]{return now;};
    services.observeGameplay=[&](auto &w,auto &,const auto &p,const auto &c,const auto &f){world=&w;party=&p;position=&c;flags=&f;};
    services.configureFlow=[&](auto &f,const auto &c){original.configureFlow(f,c);flow=&f;
        f.reportText=[](const std::string &){};};
    services.show=[&](const IndexedFrame &first,const auto &handler,const auto &escape,const auto &idle,const auto &status){
        check(flow && world && party && position && flags && target,"M43 production owners absent");
        check(world->sessionState().journey(),"M43 fresh content is not 14");
        std::deque<std::function<bool()>> steps;std::optional<IndexedFrame> next;
        IndexedFrame::Presentation presented;
        bool shown=false,acted=false,earnArmor=false;unsigned blocks=0,iterations=0;
        auto repairedOwner=std::make_shared<unsigned>(0),repairedSlot=std::make_shared<unsigned>(0);
        auto repairGold=std::make_shared<unsigned>(0);
        auto trainingMember=std::make_shared<unsigned>(0),trainingGold=std::make_shared<unsigned>(0);
        auto buyGold=std::make_shared<unsigned>(0),plainArmorBefore=std::make_shared<unsigned>(0);
        unsigned openingAttacks=0;
        auto mainStockCursor=std::make_shared<XeenJourneyRandomState>();
        const auto inspect=[&](std::function<void()> fn){steps.push_back([fn]{fn();return true;});};
        const auto act=[&](PlayerAction action){
            std::cerr << "ACTION " << action.index() << " at " << position->x << "," << position->y << " service " << flow->serviceSaveBlocked() << " gold " << party->monsterTreasure->gold << std::endl;
            check(shown && handler.displayedInput(),"M43 input preceded native presentation");
            next=handler.withPresentedInput(action,*handler.displayedInput(),presented);acted=true;shown=false;
            if(position->mapId==XeenMapIdentity(28) && position->y>=27)
                std::cerr<<"M43 ACTION RESULT "<<position->x<<','<<position->y<<" FRAME "<<bool(next)
                    <<" OUTCOME "<<int(flow->encounter()->actionResult().outcome)
                    <<" REASON "<<int(flow->encounter()->actionResult().reason)
                    <<" GOLD "<<party->monsterTreasure->gold<<std::endl;
        };
        const auto action=[&](PlayerAction value){steps.push_back([&,value]{act(value);return true;});};
        const auto settle=[&]{steps.push_back([&]{
            if(flow->canSave())return true;
            const auto *combat=flow->encounter()->combat();
            if(combat){
                check(combat->phase()!=XeenCombatPhase::Failed && combat->phase()!=XeenCombatPhase::Defeat &&
                    combat->phase()!=XeenCombatPhase::SupportStopped,"M43 combat stopped");
                if(const auto cast=combat->cast()){
                    if(const auto input=m49_combat::finishAwaken(*cast,*party))act(*input);
                }else if(combat->phase()==XeenCombatPhase::PlayerReady){
                    if(scenario=="services") {
                        bool broken=false;for(auto owner:kXeenCombatOwners)for(const auto &armor:party->roster.at(owner).armor)broken=broken || (armor.id && (armor.state&128));
                        if(m49_combat::canAwaken(*party,combat->participant()))act(CastSpellAction{});
                        else if(earnArmor && !broken && openingAttacks++>=1)act(BlockAction{});
                        else if(!earnArmor && position->mapId==XeenMapIdentity(23))
                            act(combat->participant()==1 || combat->participant()==4 ? PlayerAction{AttackAction{}} : PlayerAction{RunAction{}});
                        else act(AttackAction{});
                    }
                    else {
                        const auto rows=combat->contacts();unsigned selected=0;
                        for(unsigned i=0;i<rows.size();++i)if(rows[i] && (!rows[selected] || rows[i]->recordIndex<rows[selected]->recordIndex))selected=i;
                        if(!(rows[selected]==combat->selectedTarget()))act(SelectCombatTargetAction{selected});else act(AttackAction{});
                    }
                }
            }else if(world->sessionState().journeyActivity()==XeenJourneyActivity::Event ||
                     world->sessionState().journeyActivity()==XeenJourneyActivity::Reward)act(AcknowledgeAction{});
            check(flow->encounter()->state().phase()!=XeenEncounterPhase::SupportStopped,"M43 journey stopped");
            return false;
        });};
        const auto route=[&](const std::string &path){for(char c:path){
            switch(c){case 'U':action(NavigationAction::MoveForward);break;
                case 'D':action(NavigationAction::MoveBackward);break;
                case 'L':action(NavigationAction::TurnLeft);break;
                case 'R':action(NavigationAction::TurnRight);break;
                case 'F':action(ShootAction{});break;default:throw std::logic_error("Bad M43 route step");}
            settle();
            if(c=='U')inspect([&]{if(position->mapId==XeenMapIdentity(28) && position->y>=21)
                std::cerr<<"M43 STEP "<<position->x<<','<<position->y<<'\n';});
        }};
        const auto checkpoint=[&](const char *label){action(SaveGameAction{});inspect([&,label]{
            check(flow->canSave(),"M43 checkpoint is not Quiet");
            const auto path=fs::path(target->wstring()+L"-"+fs::path(label).wstring()+L".mmsave");
            check(fs::exists(*target),"M43 native F9 did not save");
            fs::copy_file(*target,path,fs::copy_options::overwrite_existing);
            std::cerr<<"M43 CHECKPOINT "<<label<<' '<<position->x<<','<<position->y<<' '<<party->encounterContext->day<<':'
                <<party->encounterContext->minutes<<" gold "<<party->monsterTreasure->gold<<'\n';
        });};

        inspect([&]{
            if(resume){
                const auto live=XeenSaveState::capture(original.resources.signature,*party,*position,*flags,*world);
                check(XeenSaveFormat::encode(live)==XeenSaveFormat::encode(XeenSaveFile::read(*target)),"M44 exact restart before input");
                std::cerr<<"M44 RESTORE EXACT BEFORE INPUT\n";
            }
        });
        if(!resume && scenario=="mainland") {
            action(InteractionAction{});settle();
            inspect([&]{check(party->questFlags.isSet(2),"M44 Myra request");});
            route("LUUURUULURUULUUUUUUURRUURUUUL");
            action(InteractionAction{});action(YesAction{});settle();
            inspect([&]{check(party->questItems.at(17)==1 && world->sessionState().disabledObjects().count({23,13}),"M44 Phirna collection");});
            route("UULUUURUUURUULURUULUUUL");
            action(InteractionAction{});settle();
            inspect([&]{check(!party->questItems.at(17) && !party->questFlags.isSet(2) && !world->sessionState().accountedMonsters().empty(),"M44 mainland combat and Myra exchange");});
        }
        if(!resume && scenario=="services") {
            route("RRRUUURUURRRU");for(unsigned member=0;member<6;++member) {
                action(InteractionAction{});action(SelectMemberAction{member});settle();
            }
            inspect([&]{earnArmor=true;});
            route("RUULUUU");
            inspect([&]{earnArmor=false;bool found=false;
                for(unsigned member=0;member<6 && !found;++member)for(unsigned slot=0;slot<9 && !found;++slot)
                    if(party->roster.at(kXeenCombatOwners[member]).armor[slot].id &&
                        (party->roster.at(kXeenCombatOwners[member]).armor[slot].state&128)) {
                        *repairedOwner=member;*repairedSlot=slot;found=true;
                    }
                check(found,"M49 ordinary mainland combat did not earn broken armor");
                std::cerr<<"M49 COMBAT BROKEN ARMOR owner "<<unsigned(kXeenCombatOwners[*repairedOwner])<<" slot "<<*repairedSlot<<'\n';
            });
            route("DDDRDDL");
            route("RRUUUU");route("URRRUU");route("UU");route("UURU");route("RRURRRUUUUUUUUURRRU");
            steps.push_back([&]{
                if(!flow->canSave()) {
                    const auto *combat=flow->encounter()->combat();
                    if(combat && combat->phase()==XeenCombatPhase::PlayerReady)
                        act(combat->participant()==1 || combat->participant()==4 ? PlayerAction{AttackAction{}} : PlayerAction{RunAction{}});
                    else if(world->sessionState().journeyActivity()==XeenJourneyActivity::Event ||
                            world->sessionState().journeyActivity()==XeenJourneyActivity::Reward)act(AcknowledgeAction{});
                    return false;
                }
                bool eligible=false;for(unsigned member=0;member<6;++member) {
                    const auto owner=kXeenCombatOwners[member];const auto quote=xeenQuoteTraining(party->roster.at(owner),
                        *party->roster.combatInputs(owner),party->monsterTreasure->gold,*party->encounterContext);
                    if(!quote.missing && quote.levelBefore==3) {*trainingMember=member;eligible=true;break;}
                }
                int x=10,y=13;
                if(!eligible) {
                    const XeenActor *selected=nullptr;int distance=1000;
                    for(const auto &actor:world->sessionState().actors())if(actor.lifecycle==XeenActorLifecycle::Present && actor.original.resourceId==6) {
                        const int next=std::abs(actor.x-position->x)+std::abs(actor.y-position->y);
                        if(next<distance){selected=&actor;distance=next;}
                    }
                    check(selected,"M49 reachable ordinary combat did not provide paid Training XP");x=selected->x;y=selected->y;
                }
                if(const auto input=m49_combat::toward(*world,*position,x,y,XeenDirection::North)){act(*input);return false;}
                check(eligible,"Standing on a living Orc did not attach ordinary combat");return true;
            });
            action(InteractionAction{});action(YesAction{});settle();route("UUUULUUUUUUU");
            inspect([&]{check(position->mapId==28 && position->x==8 && position->y==4,"M44 Smith arrival");});
            action(InteractionAction{});steps.push_back([&]{return XeenPurchaseTestAccess::lobby(*flow);});action(DialogKeyAction{'b'});action(DialogKeyAction{'a'});
            inspect([&]{*buyGold=party->monsterTreasure->gold;for(const auto &armor:party->roster.at(0).armor)
                *plainArmorBefore+=armor.material==0 && armor.id==3 && armor.state==0 && armor.frame==0;});
            action(SelectInventorySlotAction{3});action(YesAction{});
            action(CancelInteractionAction{});action(CancelInteractionAction{});settle();
            inspect([&]{unsigned count=0;for(const auto &armor:party->roster.at(0).armor)
                count+=armor.material==0 && armor.id==3 && armor.state==0 && armor.frame==0;
                check(party->monsterTreasure->gold==*buyGold-200 && count==*plainArmorBefore+1,"M44 paid Buy delivery");});
            action(InteractionAction{});steps.push_back([&]{return XeenPurchaseTestAccess::lobby(*flow);});
            steps.push_back([&]{act(SelectMemberAction{*repairedOwner});return true;});action(DialogKeyAction{'b'});action(DialogKeyAction{'a'});action(DialogKeyAction{'f'});
            steps.push_back([&]{act(SelectInventorySlotAction{*repairedSlot});return true;});
            inspect([&]{check(XeenPurchaseTestAccess::quote(*flow),"M44 Repair quote absent");*repairGold=party->monsterTreasure->gold;});action(YesAction{});
            action(CancelInteractionAction{});action(CancelInteractionAction{});settle();
            inspect([&]{check(party->monsterTreasure->gold<*repairGold &&
                party->roster.at(kXeenCombatOwners[*repairedOwner]).armor[*repairedSlot].state==0,"M44 paid Repair");});
            // A casualty can retain earned XP but cannot Train. Recover the
            // selected learner through the original paid Temple, never by a
            // test mutation or a preset save.
            route("RRUUUUUUUL"+std::string(24,'U'));
            auto recoveryStage=std::make_shared<unsigned>(0);
            steps.push_back([&,recoveryStage]{
                switch(*recoveryStage) {
                case 0:act(InteractionAction{});++*recoveryStage;return false;
                case 1:if(!XeenTrainingTestAccess::templeLobby(*flow))return false;
                    act(SelectMemberAction{*trainingMember});++*recoveryStage;return false;
                case 2:check(*trainingMember==1 && party->monsterTreasure->gold==648 && party->roster.at(18).currentHp==-3 &&
                        party->roster.at(18).conditions[12]==1 && party->roster.at(18).conditions[13]==1,"Learner recovery literal preimage differs");
                    act(DialogKeyAction{'h'});++*recoveryStage;
                    return false;
                case 3:if(party->monsterTreasure->gold!=238)return false;
                    check(party->roster.at(18).currentHp==48 && party->roster.at(18).currentSp==0 &&
                        party->roster.at(18).conditions==std::array<std::uint8_t,16>{},"Temple literal 410-gold recovery differs");
                    act(CancelInteractionAction{});++*recoveryStage;return false;
                default:return flow->canSave();
                }
            });
            route("RR"+std::string(21,'U')+"R"+std::string(5,'U')+"R"+std::string(4,'U'));
            action(InteractionAction{});
            steps.push_back([&]{return XeenTrainingTestAccess::menu(*flow);});
            steps.push_back([&]{act(SelectMemberAction{*trainingMember});return true;});
            inspect([&]{*trainingGold=party->monsterTreasure->gold;});action(DialogKeyAction{'t'});
            steps.push_back([&]{return party->roster.at(kXeenCombatOwners[*trainingMember]).permanentLevel==4;});
            action(CancelInteractionAction{});settle();
            inspect([&]{check(party->roster.at(kXeenCombatOwners[*trainingMember]).permanentLevel==4 &&
                party->monsterTreasure->gold==*trainingGold-90,"M44 paid Training");});
        }
        checkpoint("final");
        inspect([&]{SDL_Event event{};event.type=SDL_QUIT;SDL_PushEvent(&event);});
        auto native=handler;native.closed={};native.beginCycle=[&](std::uint64_t){handler.beginCycle(++cycle);};
        native.framePresented=[&](const auto &frame){handler.framePresented(frame);presented=frame;shown=true; m49_trace::frame(*world,*party,*position,*flow);};
        const auto drive=[&]()->std::optional<IndexedFrame>{
            check(++iterations<4000,"M43 witness iteration bound");now+=100;next.reset();acted=false;
            if(iterations%100==0)std::cerr << "WAIT " << iterations << " menu " << XeenTrainingTestAccess::menu(*flow) << " quote " << XeenTrainingTestAccess::quote(*flow) << " clericHP "<<party->roster.at(1).currentHp<<" SP "<<party->roster.at(1).currentSp<<" armorHP "<<party->roster.at(6).currentHp<< " level " << party->roster.at(18).permanentLevel << " gold " << party->monsterTreasure->gold << std::endl;
            if(shown && iterations%3!=0 && handler.acceptsInputFrame(presented) && flow->journeyInputCurrent(handler.displayedInput()))while(!steps.empty()){const bool done=steps.front()();if(done)steps.pop_front();if(acted || !done)break;}
            if(acted)return next;
            auto frame=idle();if(frame)shown=false;return frame;
        };
        const auto ok=original.show(first,native,escape,drive,status);
        check(ok && steps.empty(),"M43 witnessed actions did not complete");
        return ok;
    };
    return realPlay(app,services,camera,target,resume,entry,seed);
}
