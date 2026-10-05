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
    std::optional<std::uint32_t> seed) {probe_fired::hit("Application::playGameplay");if(!resume && std::string(std::getenv("MMODERN_M43_STAGE")?std::getenv("MMODERN_M43_STAGE"):"main")=="main")for(const char *probe:{"XeenCombatRandom::draw"})probe_fired::expect(probe);
    QuietCliOutput quietOutput;
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
        bool shown=false,acted=false,earnDeath=false;unsigned blocks=0,iterations=0;
        // Literal oracle: TempleLocation::doOptions at the pinned revision.
        // Prices and HP below are fixed operands, never a production Heal result.
        const auto expectedTemple=[&](unsigned owner,unsigned price,int hp,int sp,unsigned goldBefore,unsigned goldAfter) {
            check(party->monsterTreasure->gold==goldBefore && party->roster.at(owner).currentSp==sp,"Temple literal preimage differs");
            XeenTempleHealCandidate out;out.character=party->roster.at(owner);out.inputs=*party->roster.combatInputs(owner);
            out.character.currentHp=hp;
            out.character.intellect.temporary=out.character.personality.temporary=out.character.endurance.temporary=out.character.temporaryLevel=0;
            for(unsigned n=1;n<=15;++n)out.character.conditions[n]=0;
            out.inputs.might.temporary=out.inputs.speed.temporary=out.inputs.accuracy.temporary=out.inputs.temporaryAc=0;
            out.inputs.luck->temporary=out.inputs.resistances->coldTemporary=out.inputs.resistances->electricalTemporary=out.inputs.poisonResistance->temporary=0;
            out.result.owner=owner;out.result.price=price;out.result.goldBefore=goldBefore;out.result.goldAfter=goldAfter;
            return out;
        };
        auto resurrection=std::make_shared<XeenTempleHealCandidate>(),initialHeal=std::make_shared<XeenTempleHealCandidate>();
        auto resurrectionIndex=std::make_shared<unsigned>(0),healIndex=std::make_shared<unsigned>(0);
        auto arrivalMinutes=std::make_shared<unsigned>(0);
        unsigned openingAttacks=0;
        auto mainStockCursor=std::make_shared<XeenJourneyRandomState>();
        const auto inspect=[&](std::function<void()> fn){steps.push_back([fn]{fn();return true;});};
        const auto act=[&](PlayerAction action){
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
                    bool dead=false;for(auto owner:kXeenCombatOwners)dead=dead || bool(party->roster.at(owner).conditions[13]);
                    if(m49_combat::canAwaken(*party,combat->participant()))act(CastSpellAction{});
                    else if(earnDeath && !dead && openingAttacks++>=1) {++blocks;act(BlockAction{});}
                    else act(AttackAction{});
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
                const auto actual=XeenSaveState::capture(original.resources.signature,*party,*position,*flags,*world);
                check(XeenSaveFormat::encode(actual)==XeenSaveFormat::encode(XeenSaveFile::read(*target)),
                    "M43 restore changed encoded owners before first input");
                if(stage=="unpaid")
                    check(position->mapId==XeenMapIdentity(28) && position->x==15 && position->y==28 &&
                        party->encounterContext->day==8 && party->encounterContext->minutes==772 &&
                        party->monsterTreasure->gold==810 && party->roster.at(6).currentHp==-29 &&
                        party->roster.at(1).currentHp==14,
                        "M43 restored original refusal A baseline differs");
                else if(stage=="depleted" || stage=="depleted-unpaid" || stage=="depleted-training")
                    check(position->mapId==XeenMapIdentity(28) && position->x==8 && position->y==4 &&
                        position->direction==XeenDirection::West && party->encounterContext->day==9 &&
                        party->monsterTreasure->gold==670 && world->sessionState().journeyRandom()->state==4226505513u &&
                        world->sessionState().journeyRandom()->count==1073,
                        "M43 restored depleted Buy baseline differs");
                std::cerr<<"M43 RESTORE EXACT BEFORE INPUT\n";
            }else{
                check(seed==3626689381u && position->mapId==XeenMapIdentity(23),
                    "M43 witness needs fresh seed and city approach");
                check(party->monsterTreasure->gold==800 && party->encounterContext->day==8,"M43 fresh economy differs");
            }
        });
        if(!resume){
        route("RRRUUURUURRRU");for(unsigned member=0;member<6;++member) {
            action(InteractionAction{});action(SelectMemberAction{member});settle();
        }
        inspect([&]{earnDeath=true;});
        route("RUULUUU");
        inspect([&]{
            earnDeath=false;bool dead=false;for(auto owner:kXeenCombatOwners)if(party->roster.at(owner).conditions[13]) {
                dead=true;std::cerr<<"M49 ORDINARY COMBAT DEATH owner "<<unsigned(owner)<<" HP "<<party->roster.at(owner).currentHp<<'\n';
            }
            check(dead,"M49 reachable physical combat did not earn death");
        });
        route("DDDRDDLDRDDLDDDRLLULUU");action(InteractionAction{});action(YesAction{});settle();
        route(std::string(28,'U'));
        inspect([&]{
            std::cerr<<"M43 ARRIVAL OBSERVED "<<position->mapId<<' '<<position->x<<','<<position->y
                <<" TIME "<<party->encounterContext->day<<':'<<party->encounterContext->minutes
                <<" GOLD "<<party->monsterTreasure->gold<<'\n';
            check(position->mapId==XeenMapIdentity(28) && position->x==15 && position->y==28 &&
                party->encounterContext->day==8,
                "M43 Temple arrival differs");
            *arrivalMinutes=party->encounterContext->minutes;
            *mainStockCursor=*world->sessionState().journeyRandom();
            std::cerr<<"M43 TEMPLE ARRIVAL 15,28 MINUTE "<<*arrivalMinutes<<'\n';
        });
        checkpoint("A");
        action(InteractionAction{});
        steps.push_back([&]{return flow->serviceSaveBlocked() &&
            XeenTrainingTestAccess::templeLobby(*flow) &&
            world->sessionState().journeyActivity()==XeenJourneyActivity::Service;});
        inspect([&]{check(party->roster.at(6).currentHp==-29 && party->roster.at(6).conditions[12]==1 && party->roster.at(6).conditions[13]==1,"Ordinary death literal differs");
            *resurrection=expectedTemple(6,410,15,27,810,400);*resurrectionIndex=5;});
        steps.push_back([&]{act(SelectMemberAction{*resurrectionIndex});return true;});
        inspect([&]{check(!flow->canSave(),"M43 Temple quote exposed Quiet");});
        inspect([&]{observedStockDraws.clear();observeStockDraws=true;});
        action(DialogKeyAction{'h'});
        steps.push_back([&]{return party->monsterTreasure->gold==resurrection->result.goldAfter;});
        inspect([&]{observeStockDraws=false;
            const auto owner=resurrection->result.owner;
            check(resurrection->result.price>0 && !party->roster.at(owner).conditions[13] &&
                xeen_state::sameCharacter(party->roster.at(owner),resurrection->character) && party->encounterContext->day==8,
                "M49 paid resurrection differs from the original rule");
            std::cerr<<"M49 PAID RESURRECTION owner "<<unsigned(owner)<<" price "<<resurrection->result.price<<'\n';});
        inspect([&]{check(party->roster.at(1).currentHp==14 && party->roster.at(1).conditions[8]==1,"Ordinary wound literal differs");
            *initialHeal=expectedTemple(1,60,21,20,400,340);*healIndex=4;});
        steps.push_back([&]{act(SelectMemberAction{*healIndex});return true;});action(DialogKeyAction{'h'});
        steps.push_back([&]{return party->monsterTreasure->gold==initialHeal->result.goldAfter;});
        inspect([&]{check(initialHeal->result.price>0 && xeen_state::sameCharacter(party->roster.at(initialHeal->result.owner),initialHeal->character),
                "M49 paid Heal differs from the original rule");
            std::cerr<<"M49 PAID HEAL owner "<<unsigned(initialHeal->result.owner)<<" price "<<initialHeal->result.price<<'\n';});
        action(CancelInteractionAction{});
        steps.push_back([&]{return flow->canSave();});
        inspect([&]{
            m42_test::StockOracle oracle{mainStockCursor->state,mainStockCursor->count,{}};
            const auto expected=oracle.generate();
            check(party->encounterContext->day==10 && party->encounterContext->minutes==*arrivalMinutes &&
                party->monsterTreasure->gold==initialHeal->result.goldAfter && world->sessionState().journeyRandom()->state==oracle.state &&
                world->sessionState().journeyRandom()->count==oracle.count &&
                observedStockDraws==oracle.trace &&
                m40_test::stockBytes(expected)==m40_test::stockBytes(*party->serviceEconomy),
                "M43 one two-day paid departure or complete stock differs");
            std::cerr<<"M43 PAID DEPARTURE DAY10 RNG "<<oracle.state<<':'<<oracle.count<<" STOCK ALL1152\n";
        });
        checkpoint("B");
        }
        if(stage=="unpaid"){
            check(resume,"M43 original refusal branch requires original A checkpoint");
            auto economy=std::make_shared<XeenServiceEconomy>(*party->serviceEconomy);
            auto cursor=std::make_shared<XeenJourneyRandomState>(*world->sessionState().journeyRandom());
            auto seymour=std::make_shared<XeenCharacter>(party->roster.at(6));
            auto rebecca=std::make_shared<XeenCharacter>(party->roster.at(1));
            const auto unpaidMinutes=party->encounterContext->minutes;
            const auto unpaidGold=party->monsterTreasure->gold;
            action(InteractionAction{});
            steps.push_back([&]{return flow->serviceSaveBlocked() && XeenTrainingTestAccess::templeLobby(*flow);});
            action(SelectMemberAction{5});
            inspect([&]{check(!flow->canSave(),"M43 refusal quote exposed Quiet");});
            action(CancelInteractionAction{});action(CancelInteractionAction{});
            steps.push_back([&]{return flow->canSave();});
            inspect([&,economy,cursor,seymour,rebecca,unpaidMinutes,unpaidGold]{
                check(party->encounterContext->day==9 && party->encounterContext->minutes==unpaidMinutes &&
                    party->monsterTreasure->gold==unpaidGold && *party->serviceEconomy==*economy &&
                    *world->sessionState().journeyRandom()==*cursor &&
                    xeen_state::sameCharacter(party->roster.at(6),*seymour) &&
                    xeen_state::sameCharacter(party->roster.at(1),*rebecca),
                    "M43 original refusal charged Heal or changed stock/RNG/selected owners");
                std::cerr<<"M43 ORIGINAL A REFUSAL DAY9 STOCK/RNG RETAINED\n";
            });
            checkpoint("U");
        }else if(stage=="depleted-training"){
            check(resume,"M43 depleted Training branch requires an original production save");
            auto economy=std::make_shared<XeenServiceEconomy>();
            auto cursor=std::make_shared<XeenJourneyRandomState>();
            inspect([&,economy,cursor]{*economy=*party->serviceEconomy;*cursor=*world->sessionState().journeyRandom();
                m42_test::sameCategory(economy->wares[0][0][1],m42_test::armorAfter());
                std::cerr<<"M43 DEPLETED ORIGINAL BUY PREIMAGE DAY9 GOLD670\n";});
            route("RR"+std::string(7,'U')+"L"+std::string(3,'U')+"L"+std::string(5,'U')+"R"+std::string(4,'U'));
            inspect([&]{check(position->mapId==XeenMapIdentity(28) && position->x==10 && position->y==11 &&
                party->encounterContext->day==9 && party->monsterTreasure->gold==670,
                "M43 depleted Training arrival differs");observedStockDraws.clear();observeStockDraws=true;});
            action(InteractionAction{});
            steps.push_back([&]{return flow->serviceSaveBlocked() && XeenTrainingTestAccess::menu(*flow);});
            for(unsigned member:{1u,4u}) {
                const auto owner=member==1?18u:1u;
                action(SelectMemberAction{member});action(DialogKeyAction{'t'});
                steps.push_back([&,owner]{return party->roster.at(owner).permanentLevel==4;});
                inspect([&,member,economy,cursor]{
                    check(party->encounterContext->day==(member==1?10:11) &&
                        party->monsterTreasure->gold==(member==1?580:490),
                        "M43 depleted Training member-day or ninety-gold price differs");
                    if(member==1)check(*party->serviceEconomy==*economy &&
                        *world->sessionState().journeyRandom()==*cursor,
                        "M43 Training 9->10 changed depleted stock or live RNG");
                });
            }
            action(CancelInteractionAction{});steps.push_back([&]{return flow->canSave();});
            inspect([&,economy,cursor]{observeStockDraws=false;
                m42_test::StockOracle oracle{cursor->state,cursor->count,{}};const auto generated=oracle.generate();
                check(party->encounterContext->day==12 && party->monsterTreasure->gold==490 &&
                    m40_test::stockBytes(generated)==m40_test::stockBytes(*party->serviceEconomy) &&
                    observedStockDraws==oracle.trace &&
                    world->sessionState().journeyRandom()->state==oracle.state &&
                    world->sessionState().journeyRandom()->count==oracle.count &&
                    party->serviceEconomy->bank==economy->bank,
                    "M43 depleted Training did not publish exactly one complete reference generation");
                std::cerr<<"M43 DEPLETED TRAINING DAYS9/10/11/12 GOLD490 ALL1152 ONE GENERATION\n";
            });checkpoint("T");
        }else if(stage=="depleted" || stage=="depleted-unpaid"){
            check(resume,"M43 depleted branch requires an original production save");
            auto economy=std::make_shared<XeenServiceEconomy>();
            auto cursor=std::make_shared<XeenJourneyRandomState>();
            auto quote=std::make_shared<XeenTempleHealResult>();
            auto expectedHeal=std::make_shared<XeenTempleHealCandidate>();
            auto selected=std::make_shared<unsigned>();
            inspect([&,economy,cursor]{*economy=*party->serviceEconomy;*cursor=*world->sessionState().journeyRandom();
                m42_test::sameCategory(economy->wares[0][0][1],m42_test::armorAfter());
                std::cerr<<"M43 DEPLETED ORIGINAL BUY PREIMAGE DAY9 GOLD670\n";});
            route("RR"+std::string(7,'U')+"L"+std::string(24,'U'));
            inspect([&,selected,quote,expectedHeal]{
                check(position->mapId==XeenMapIdentity(28) && position->x==15 && position->y==28 &&
                    party->encounterContext->day==9 && party->encounterContext->minutes==834 &&
                    party->monsterTreasure->gold==670,"M43 depleted-stock Temple arrival differs");
                *selected=4;*expectedHeal=expectedTemple(1,30,21,20,670,640);*quote=expectedHeal->result;
            });
            action(InteractionAction{});
            steps.push_back([&]{return flow->serviceSaveBlocked() &&
                XeenTrainingTestAccess::templeLobby(*flow) &&
                world->sessionState().journeyActivity()==XeenJourneyActivity::Service;});
            if(stage=="depleted"){
                steps.push_back([&,selected]{act(SelectMemberAction{*selected});return true;});
                inspect([&]{observedStockDraws.clear();observeStockDraws=true;});
                action(DialogKeyAction{'h'});
                steps.push_back([&,expectedHeal]{return party->monsterTreasure->gold==expectedHeal->result.goldAfter;});
                inspect([&,quote,expectedHeal]{observeStockDraws=false;const auto owner=quote->owner;
                    std::cerr<<"M43 DEPLETED HEAL OBSERVED owner "<<unsigned(owner)<<" expectedHP "
                        <<expectedHeal->character.currentHp<<" actualHP "<<party->roster.at(owner).currentHp
                        <<" expectedSP "<<expectedHeal->character.currentSp<<" actualSP "<<party->roster.at(owner).currentSp
                        <<" gold "<<party->monsterTreasure->gold<<'\n';
                    check(xeen_state::sameCharacter(party->roster.at(owner),expectedHeal->character) &&
                        xeen_state::sameInputs(*party->roster.combatInputs(owner),expectedHeal->inputs),
                        "M43 depleted branch selected Heal differed from detached original rule");
                    std::cerr<<"M43 DEPLETED HEAL OWNER "<<unsigned(owner)<<" PRICE "<<quote->price<<'\n';});
            }
            action(CancelInteractionAction{});
            steps.push_back([&]{return flow->canSave();});
            inspect([&,economy,cursor,quote]{
                if(stage=="depleted"){
                    m42_test::StockOracle oracle{cursor->state,cursor->count,{}};
                    const auto generated=oracle.generate();
                    check(party->encounterContext->day==11 && party->monsterTreasure->gold==quote->goldBefore-quote->price &&
                        m40_test::stockBytes(generated)==m40_test::stockBytes(*party->serviceEconomy) &&
                        observedStockDraws==oracle.trace &&
                        world->sessionState().journeyRandom()->state==oracle.state &&
                        world->sessionState().journeyRandom()->count==oracle.count &&
                        party->serviceEconomy->bank==economy->bank,
                        "M43 paid Temple did not fully replace real depleted stock once");
                    std::cerr<<"M43 DEPLETED PAID REPLACEMENT ALL1152 DAY11\n";
                }else{
                    check(party->encounterContext->day==10 && party->monsterTreasure->gold==670 &&
                        *party->serviceEconomy==*economy && *world->sessionState().journeyRandom()==*cursor,
                        "M43 unpaid Temple departure changed depleted stock or RNG");
                    std::cerr<<"M43 DEPLETED UNPAID RETAINED DAY10\n";
                }
            });
            checkpoint("D");
        }else{
        auto resumedMinutes=std::make_shared<unsigned>(0),resumedGold=std::make_shared<unsigned>(0);
        inspect([&,resumedMinutes,resumedGold]{*resumedMinutes=party->encounterContext->minutes;*resumedGold=party->monsterTreasure->gold;});
        route("DDUU");
        inspect([&,resumedMinutes,resumedGold]{check(position->mapId==XeenMapIdentity(28) && position->x==15 && position->y==28 &&
            party->encounterContext->day==10 && party->encounterContext->minutes==*resumedMinutes+4 &&
            party->monsterTreasure->gold==*resumedGold,"M43 resumed north corridor continuation differs");});
        checkpoint("C");
        route("RR"+std::string(28,'U'));
        inspect([&]{check(position->mapId==XeenMapIdentity(28) && position->x==15 && position->y==0 &&
            position->direction==XeenDirection::South,"M43 southbound Temple seam or reset exit differs");});
        action(InteractionAction{});action(YesAction{});settle();
        inspect([&]{check(position->mapId==XeenMapIdentity(23) && position->x==10 && position->y==12 &&
            position->direction==XeenDirection::South,"M43 original city reset did not reach outdoor exit");});
        route("RRU");action(InteractionAction{});action(YesAction{});settle();
        inspect([&]{check(position->mapId==XeenMapIdentity(28) && position->x==15 && position->y==0 &&
            position->direction==XeenDirection::North,"M43 original city reentry differs");});
        auto contact=std::make_shared<bool>(false),arrow=std::make_shared<bool>(false);
        auto selected=std::make_shared<bool>(false);auto spBefore=std::make_shared<int>(0);
        inspect([&,spBefore]{*spBefore=party->roster.at(6).currentSp;});
        steps.push_back([&,contact,arrow,selected,spBefore]{
            if(flow->canSave()) {
                if(*contact) {
                    check(*arrow && party->roster.at(6).currentSp<*spBefore,
                        "recovered Seymour did not spend SP on a live reset Slime");
                    std::cerr<<"M43 RESET SLIME SEYMOUR MAGIC ARROW SP "<<*spBefore<<"->"
                        <<party->roster.at(6).currentSp<<'\n';
                    return true;
                }
                check(position->y<4,"M43 reset Slime contact missing");
                act(NavigationAction::MoveForward);return false;
            }
            const auto *combat=flow->encounter()->combat();
            if(!combat) {
                if(world->sessionState().journeyActivity()==XeenJourneyActivity::Event ||
                   world->sessionState().journeyActivity()==XeenJourneyActivity::Reward)act(AcknowledgeAction{});
                return false;
            }
            *contact=true;
            check(combat->phase()!=XeenCombatPhase::Failed && combat->phase()!=XeenCombatPhase::Defeat &&
                combat->phase()!=XeenCombatPhase::SupportStopped,"M43 reset Slime combat stopped");
            if(const auto cast=combat->cast()) {
                switch(cast->phase) {
                case XeenCombatCastPhase::Learned:
                    if(cast->slot==25)act(AcknowledgeAction{});
                    else act(NavigationAction::MoveBackward);
                    break;
                case XeenCombatCastPhase::Enemy:
                    if(!*selected){*selected=true;act(SelectCombatTargetAction{0});}
                    else act(AcknowledgeAction{});
                    break;
                case XeenCombatCastPhase::Confirm:act(AcknowledgeAction{});break;
                case XeenCombatCastPhase::Result:act(AcknowledgeAction{});*arrow=true;*selected=false;break;
                default:break;
                }
            } else if(combat->phase()==XeenCombatPhase::PlayerReady) {
                if(combat->participant()==5)act(CastSpellAction{});
                else act(BlockAction{});
            }
            return false;
        });
        steps.push_back([&]{
            if(!flow->canSave()) {
                if(world->sessionState().journeyActivity()==XeenJourneyActivity::Event ||
                   world->sessionState().journeyActivity()==XeenJourneyActivity::Reward)act(AcknowledgeAction{});
                return false;
            }
            if(position->mapId==XeenMapIdentity(28) && position->x==15 && position->y==28)return true;
            check(position->mapId==XeenMapIdentity(28) && position->x==15 && position->y<28 &&
                position->direction==XeenDirection::North,"M43 post-combat Temple return route differs");
            act(NavigationAction::MoveForward);return false;
        });
        action(InteractionAction{});
        steps.push_back([&]{return flow->serviceSaveBlocked() && XeenTrainingTestAccess::templeLobby(*flow);});
        auto returnHeal=std::make_shared<XeenTempleHealCandidate>();
        auto returnIndex=std::make_shared<unsigned>(0);
        inspect([&,returnHeal,returnIndex]{
            *returnHeal=expectedTemple(14,30,36,0,340,310);*returnIndex=2;
            std::cerr<<"M43 RETURN QUOTE "<<returnHeal->result.price<<" GOLD "
                <<returnHeal->result.goldBefore<<"->"<<returnHeal->result.goldAfter<<'\n';
        });
        steps.push_back([&,returnIndex]{act(SelectMemberAction{*returnIndex});return true;});
        action(DialogKeyAction{'h'});
        auto returnWait=std::make_shared<unsigned>(0);
        steps.push_back([&,returnHeal,returnWait]{check(++*returnWait<1000,
            "M43 return Heal did not publish after bounded idle work");
            return party->monsterTreasure->gold==returnHeal->result.goldAfter;});
        inspect([&,returnHeal]{const auto owner=returnHeal->result.owner;check(xeen_state::sameCharacter(party->roster.at(owner),returnHeal->character) &&
            xeen_state::sameInputs(*party->roster.combatInputs(owner),returnHeal->inputs),
            "M43 return visit selected Heal differed from detached rules");
            std::cerr<<"M43 RETURN HEAL PRICE "<<returnHeal->result.price<<" SP "
                <<party->roster.at(6).currentSp<<'\n';});
        action(CancelInteractionAction{});
        steps.push_back([&]{return flow->canSave();});
        inspect([&,returnHeal]{check(party->encounterContext->day==12 &&
            party->monsterTreasure->gold==returnHeal->result.goldAfter,
            "M43 returned paid visit did not make one two-day departure");
            std::cerr<<"M43 RETURN TEMPLE PAID DAY12\n";});
        checkpoint("E");
        }
        if(stage=="main") {
            auto before=std::make_shared<XeenSaveSnapshot>();
            inspect([&,before]{*before=XeenSaveState::capture(original.resources.signature,*party,*position,*flags,*world);});
            route("DDUU");
            inspect([&,before]{const auto after=XeenSaveState::capture(original.resources.signature,*party,*position,*flags,*world);
                check(xeen_state::sameCamera(after.camera,before->camera) && after.journey->context->day==12 &&
                    after.journey->context->minutes==before->journey->context->minutes+4 &&
                    after.journey->treasure->gold==before->journey->treasure->gold &&
                    after.journey->random==before->journey->random &&
                    after.journey->serviceEconomy==before->journey->serviceEconomy,
                    "M43 E continuation replayed Heal, departure or generation");
            });checkpoint("F");
        }
        inspect([&]{SDL_Event event{};event.type=SDL_QUIT;SDL_PushEvent(&event);});
        auto native=handler;native.closed={};native.beginCycle=[&](std::uint64_t){handler.beginCycle(++cycle);};
        native.framePresented=[&](const auto &frame){handler.framePresented(frame);presented=frame;shown=true; m49_trace::frame(*world,*party,*position,*flow);};
        const auto drive=[&]()->std::optional<IndexedFrame>{
            check(++iterations<30000,"M43 witness iteration bound");now+=100;next.reset();acted=false;
            if(shown)while(!steps.empty()){const bool done=steps.front()();if(done)steps.pop_front();if(acted || !done)break;}
            if(acted)return next;
            auto frame=idle();if(frame)shown=false;return frame;
        };
        const auto ok=original.show(first,native,escape,drive,status);
        check(ok && steps.empty(),"M43 witnessed actions did not complete");
        return ok;
    };
    return realPlay(app,services,camera,target,resume,entry,seed);
}
