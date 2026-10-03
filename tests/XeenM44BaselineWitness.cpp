// Original-resource M43 witness: all party, combat, route and Temple mutations
// arise from the production application after concrete presented-frame input.
#include "XeenProbeFired.h"
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
        bool shown=false,acted=false,cityFight=false;unsigned blocks=0,iterations=0;
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
                    if(cast->phase==XeenCombatCastPhase::Result)act(AcknowledgeAction{});
                }else if(combat->phase()==XeenCombatPhase::PlayerReady){
                    if(scenario=="services") {if(cityFight && !(party->roster.at(6).armor[0].state&128))act(BlockAction{});else act(cityFight || combat->participant()==1 || combat->participant()==4 ? PlayerAction{AttackAction{}} : PlayerAction{RunAction{}});}
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
            route("RRRUUURUURRRU");for(unsigned member:{0u,1u,2u,3u,5u}) {action(InteractionAction{});action(SelectMemberAction{member});settle();}
            route("RRUUUU");route("URRRUU");route("UU");route("UURU");route("RRURRRUUUUUUUUURRRU");
            action(CastSpellAction{});action(SelectMemberAction{4});action(NavigationAction::MoveBackward);
            action(AcknowledgeAction{});action(AcknowledgeAction{});action(SelectMemberAction{4});settle();
            inspect([&]{cityFight=true;});
            action(InteractionAction{});action(YesAction{});settle();route("UUUULUUUUUUU");
            inspect([&]{check(position->mapId==28 && position->x==8 && position->y==4 && party->monsterTreasure->gold==870,"M44 Smith arrival");});
            inspect([&]{check((party->roster.at(6).armor[0].state&128),"M44 earned broken armor");});
            action(InteractionAction{});steps.push_back([&]{return XeenPurchaseTestAccess::lobby(*flow);});action(BlockAction{});action(NavigationAction::TurnRight);
            action(SelectInventorySlotAction{3});action(AcknowledgeAction{});action(AcknowledgeAction{});action(AcknowledgeAction{});
            action(CancelInteractionAction{});action(CancelInteractionAction{});settle();
            inspect([&]{check(party->monsterTreasure->gold==670 && party->roster.at(0).armor[5].id==3,"M44 Buy delivery");});
            action(InteractionAction{});steps.push_back([&]{return XeenPurchaseTestAccess::lobby(*flow);});action(SelectMemberAction{5});action(RevisitCompletedAction{});
            action(SelectInventorySlotAction{0});action(AcknowledgeAction{});inspect([&]{check(XeenPurchaseTestAccess::quote(*flow),"M44 Repair quote absent");});action(AcknowledgeAction{});action(AcknowledgeAction{});
            action(CancelInteractionAction{});action(CancelInteractionAction{});settle();
            inspect([&]{check(party->monsterTreasure->gold==668 && party->roster.at(6).armor[0].state==0,"M44 paid Repair");});
            route("RRUUUUUUULUUULUUUUURUUUU");
            action(InteractionAction{});
            steps.push_back([&]{return XeenTrainingTestAccess::menu(*flow);});
            for(unsigned member:{1u}) {
                action(SelectMemberAction{member});action(AcknowledgeAction{});
                inspect([&]{check(XeenTrainingTestAccess::quote(*flow),"M44 Training quote absent");});action(AcknowledgeAction{});
                steps.push_back([&,member]{return party->roster.at(kXeenCombatOwners[member]).permanentLevel==4;});
                action(AcknowledgeAction{});
            }
            action(CancelInteractionAction{});settle();
            inspect([&]{check(party->roster.at(18).permanentLevel==4 && party->monsterTreasure->gold==578,"M44 paid Training");});
        }
        checkpoint("final");
        inspect([&]{SDL_Event event{};event.type=SDL_QUIT;SDL_PushEvent(&event);});
        auto native=handler;native.closed={};native.beginCycle=[&](std::uint64_t){handler.beginCycle(++cycle);};
        native.framePresented=[&](const auto &frame){handler.framePresented(frame);presented=frame;shown=true;};
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
