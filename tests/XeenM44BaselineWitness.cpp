// Original-resource M43 witness: all party, combat, route and Temple mutations
// arise from the production application after concrete presented-frame input.
#include "XeenDecisionNine.h"
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
        auto repairGold=std::make_shared<unsigned>(0),repairPrice=std::make_shared<unsigned>(0);
        auto trainingMember=std::make_shared<unsigned>(0),trainingGold=std::make_shared<unsigned>(0);
        auto templeExpected=std::make_shared<XeenTempleHealCandidate>();
        auto buyGold=std::make_shared<unsigned>(0),plainArmorBefore=std::make_shared<unsigned>(0);
        auto buySlot=std::make_shared<unsigned>(0);
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
                case 'F':action(ShootAction{});break;
                case 'Q':action(NavigationAction::StrafeLeft);break;
                case 'E':action(NavigationAction::StrafeRight);break;default:throw std::logic_error("Bad M43 route step");}
            settle();
            if(c=='U')inspect([&]{if(position->mapId==XeenMapIdentity(28) && position->y>=21)
                std::cerr<<"M43 STEP "<<position->x<<','<<position->y<<'\n';});
        }};
        const auto checkpoint=[&](const char *label){
            inspect([&,label]{xeen_decision9::check(*world,*position,label,scenario=="roundtrip" && (std::string(label)=="A" || std::string(label)=="B"));});
            action(SaveGameAction{});inspect([&,label]{
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
                    if(combat && combat->phase()==XeenCombatPhase::PlayerReady) {
                        bool leaveCasualty=false;
                        for(auto owner:kXeenCombatOwners) {
                            const auto &c=party->roster.at(owner);
                            const auto q=xeenQuoteTraining(c,*party->roster.combatInputs(owner),party->monsterTreasure->gold,*party->encounterContext);
                            leaveCasualty=leaveCasualty || (q.levelBefore==3 && !q.missing &&
                                (c.conditions[8] || c.conditions[12] || c.conditions[13]));
                        }
                        // Ordinary Run kills incapacitated members left behind.
                        // Retain Services' paid resurrection obligation rather
                        // than replacing it with a living learner's cheap Heal.
                        act(leaveCasualty ? PlayerAction{RunAction{}} :
                            combat->participant()==1 || combat->participant()==4 ? PlayerAction{AttackAction{}} : PlayerAction{RunAction{}});
                    }
                    else if(world->sessionState().journeyActivity()==XeenJourneyActivity::Event ||
                            world->sessionState().journeyActivity()==XeenJourneyActivity::Reward)act(AcknowledgeAction{});
                    return false;
                }
                bool eligible=false;for(unsigned member=0;member<6;++member) {
                    const auto owner=kXeenCombatOwners[member];const auto quote=xeenQuoteTraining(party->roster.at(owner),
                        *party->roster.combatInputs(owner),party->monsterTreasure->gold,*party->encounterContext);
                    if(!quote.missing && quote.levelBefore==3 && party->roster.at(owner).conditions[13]) {*trainingMember=member;eligible=true;break;}
                }
                int x=10,y=13;
                if(!eligible) {
                    const XeenActor *selected=nullptr;int distance=1000;
                    const auto walkable=XeenMovement::component(world->map(23),position->x,position->y,xeenJourneyContent().traversal);
                    for(unsigned pass=0;pass<2 && !selected;++pass)
                        for(const auto &actor:world->sessionState().monsters(23))if(actor.lifecycle==XeenActorLifecycle::Present &&
                            actor.x>=0 && actor.y>=0 && actor.x<16 && actor.y<16 && walkable[actor.y*16+actor.x] &&
                            (pass || actor.original.resourceId==6)) {
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
                *plainArmorBefore+=armor.material==0 && armor.id==3 && armor.state==0 && armor.frame==0;
                bool found=false;const auto &offers=party->serviceEconomy->wares[0][0][1];
                for(unsigned slot=0;slot<8;++slot)if(offers[slot].material==0 && offers[slot].id==3 && !offers[slot].state && !offers[slot].frame) {*buySlot=slot;found=true;break;}
                check(found,"M44 original stock has no plain Armor3 offer");});
            steps.push_back([&]{act(SelectInventorySlotAction{*buySlot});return true;});action(YesAction{});
            action(CancelInteractionAction{});action(CancelInteractionAction{});settle();
            inspect([&]{unsigned count=0;for(const auto &armor:party->roster.at(0).armor)
                count+=armor.material==0 && armor.id==3 && armor.state==0 && armor.frame==0;
                check(party->monsterTreasure->gold==*buyGold-200 && count==*plainArmorBefore+1,"M44 paid Buy delivery");});
            action(InteractionAction{});steps.push_back([&]{return XeenPurchaseTestAccess::lobby(*flow);});
            steps.push_back([&]{act(SelectMemberAction{*repairedOwner});return true;});action(DialogKeyAction{'b'});action(DialogKeyAction{'a'});action(DialogKeyAction{'f'});
            steps.push_back([&]{act(SelectInventorySlotAction{*repairedSlot});return true;});
            inspect([&]{check(XeenPurchaseTestAccess::quote(*flow),"M44 Repair quote absent");*repairGold=party->monsterTreasure->gold;
                const auto &item=party->roster.at(kXeenCombatOwners[*repairedOwner]).armor[*repairedSlot];
                // Independent base armor prices, pinned constants.cpp. This
                // protects paid Fix against free repair and wrong-item prices.
                constexpr unsigned base[]{0,20,100,200,400,600,1000,2000,100,60,40,250,200,100};
                check(item.material==0 && item.id<std::size(base),"M44 earned repair requires ordinary armor");
                *repairPrice=std::max(1u,base[item.id]/10);
            });action(YesAction{});
            action(CancelInteractionAction{});action(CancelInteractionAction{});settle();
            inspect([&]{check(party->monsterTreasure->gold==*repairGold-*repairPrice &&
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
                case 2:{
                    // Independently apply the DOS Temple price and doOptions. Never
                    // turn the operation's own quote into its expected result;
                    // free/wrong-price recovery and lost resets must fail.
                    const auto owner=kXeenCombatOwners[*trainingMember];
                    auto &expected=*templeExpected;expected.character=party->roster.at(owner);
                    check(expected.character.conditions[13],"M44 Services lost its ordinary paid resurrection preimage");
                    expected.inputs=*party->roster.combatInputs(owner);
                    auto &c=expected.character;auto &i=expected.inputs;
                    const unsigned level=std::max(0,int(c.permanentLevel)+c.temporaryLevel);
                    // DOS XEEN.DAT price routine 0x1c9ce: unsigned HP comparison,
                    // 10*L per condition 1..12, 100*L+50*n for Dead and Stoned.
                    // Vertigo surcharges are zero; Eradicated is unreachable here.
                    const auto maxHp=XeenCharacterRules::maxHp(c,{party->encounterContext->year});
                    unsigned price=std::uint16_t(c.currentHp)<std::uint16_t(maxHp)?10*level:0;
                    for(unsigned condition=1;condition<=12;++condition)if(c.conditions[condition])price+=10*level;
                    for(unsigned condition:{13u,14u})if(c.conditions[condition])price+=100*level+50*c.conditions[condition];
                    check(!c.conditions[15],"M44 recovery reached Eradicated");
                    expected.result.price=price;expected.result.goldBefore=party->monsterTreasure->gold;
                    check(price>0 && price<=expected.result.goldBefore,"Learner recovery preimage differs");
                    expected.result.goldAfter=expected.result.goldBefore-price;
                    c.temporaryLevel=c.intellect.temporary=c.personality.temporary=c.endurance.temporary=0;
                    i.might.temporary=i.speed.temporary=i.accuracy.temporary=i.temporaryAc=0;
                    i.luck->temporary=i.resistances->coldTemporary=i.resistances->electricalTemporary=i.poisonResistance->temporary=0;
                    c.currentHp=XeenCharacterRules::maxHp(c,{party->encounterContext->year});
                    for(unsigned condition=1;condition<=15;++condition)c.conditions[condition]=0;
                    act(DialogKeyAction{'h'});++*recoveryStage;
                    return false;}
                case 3:if(party->monsterTreasure->gold!=templeExpected->result.goldAfter)return false;
                    check(xeen_state::sameCharacter(party->roster.at(kXeenCombatOwners[*trainingMember]),templeExpected->character) &&
                        xeen_state::sameInputs(*party->roster.combatInputs(kXeenCombatOwners[*trainingMember]),templeExpected->inputs),
                        "Temple paid recovery differs");
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
        // M69 fourth scenario "vertigo-f3-roundtrip" (fresh run, or resumed from
        // checkpoint A or B; a plain reload resumes nothing and saves the final).
        const std::string roundtripStage=std::getenv("MMODERN_M44_STAGE")?std::getenv("MMODERN_M44_STAGE"):"";
        if(scenario=="roundtrip") {
            const auto here=[&](int map,int x,int y,const char *what){inspect([&,map,x,y,what]{
                check(position->mapId==XeenMapIdentity(map) && position->x==x && position->y==y,what);});};
            const auto report=[&](const char *label){inspect([&,label]{
                std::cerr<<"M69 "<<label<<" at "<<position->mapId.number<<':'<<position->x<<','<<position->y<<" d"<<unsigned(position->direction)
                    <<" day "<<party->encounterContext->day<<':'<<party->encounterContext->minutes<<" woken:";
                for(const auto &entry:world->sessionState().mapStates())if(entry.second.monsters)
                    for(const auto &a:*entry.second.monsters)if(a.lifecycle==XeenActorLifecycle::Present && (a.activated || a.hp<a.statistics->baseHp()))
                        std::cerr<<' '<<entry.first.number<<'#'<<a.id.recordIndex<<'@'<<a.x<<','<<a.y<<" hp"<<a.hp<<(a.activated?"*":"");
                std::cerr<<'\n';});};
            // Decision 9 (shared XeenDecisionNine.h): checkpoints A and B, which the
            // roundtrip resumes from, hold every live camera-map monster asleep at
            // MON HP; checkpoint() enforces it before each save.
            // The Orc wounded before the Vertigo round trip, by record: decision 8
            // heals it when F3 is loaded again, so its HP cannot identify it.
            auto woundedOrc=std::make_shared<std::optional<std::size_t>>();
            const auto contactFight=[&](bool wound){steps.push_back([&,wound,woundedOrc]{
                // Attack the lowest record in contact; when wounding, Run once it is hurt.
                const auto *combat=flow->encounter()->combat();
                if(!combat) {
                    if(flow->canSave())return true;
                    if(world->sessionState().journeyActivity()==XeenJourneyActivity::Event ||
                       world->sessionState().journeyActivity()==XeenJourneyActivity::Reward)act(AcknowledgeAction{});
                    check(flow->encounter()->state().phase()!=XeenEncounterPhase::SupportStopped,"M69 journey stopped");
                    return false;
                }
                check(combat->phase()!=XeenCombatPhase::Failed && combat->phase()!=XeenCombatPhase::Defeat &&
                    combat->phase()!=XeenCombatPhase::SupportStopped,"M69 combat stopped");
                if(combat->phase()!=XeenCombatPhase::PlayerReady)return false;
                const auto rows=combat->contacts();unsigned selected=0;
                for(unsigned i=0;i<rows.size();++i)if(rows[i] && (!rows[selected] || rows[i]->recordIndex<rows[selected]->recordIndex))selected=i;
                if(!(rows[selected]==combat->selectedTarget())){act(SelectCombatTargetAction{selected});return false;}
                const auto &target=world->sessionState().regionalActors(rows[selected]->mapId).at(rows[selected]->recordIndex);
                const bool run=wound && target.hp<target.statistics->baseHp();
                if(run)*woundedOrc=std::size_t(target.id.recordIndex);
                act(run ? PlayerAction{RunAction{}} : PlayerAction{AttackAction{}});
                return false;
            });};
            const auto seekOrc=[&](bool wounded){steps.push_back([&,wounded,woundedOrc]{
                // Walk toward the nearest live Orc (the wounded one when asked) until contact.
                if(flow->encounter()->combat())return true;
                if(!flow->canSave()) {
                    if(world->sessionState().journeyActivity()==XeenJourneyActivity::Event ||
                       world->sessionState().journeyActivity()==XeenJourneyActivity::Reward)act(AcknowledgeAction{});
                    return false;
                }
                const XeenActor *selected=nullptr;int distance=1000;
                const auto walkable=XeenMovement::component(world->map(23),9,11,xeenJourneyContent().traversal);
                for(const auto &actor:world->sessionState().monsters(23))
                    if(actor.lifecycle==XeenActorLifecycle::Present && actor.original.resourceId==6 &&
                        walkable[actor.y*16+actor.x] && (!wounded || (*woundedOrc && actor.id.recordIndex==**woundedOrc))) {
                        const int next=std::abs(actor.x-position->x)+std::abs(actor.y-position->y);
                        if(next<distance){selected=&actor;distance=next;}
                    }
                check(selected,"M69 mainland Orc absent");
                if(const auto input=m49_combat::toward(*world,*position,selected->x,selected->y,position->direction)){act(*input);return false;}
                return false;
            });};
            const auto toEntrance=[&]{steps.push_back([&]{
                // Reach the entrance facing North without ever facing West, so
                // the view never wakes the stranded Orc at (7,14).
                if(!flow->canSave()) {
                    if(world->sessionState().journeyActivity()==XeenJourneyActivity::Event ||
                       world->sessionState().journeyActivity()==XeenJourneyActivity::Reward)act(AcknowledgeAction{});
                    return false;
                }
                if(position->direction!=XeenDirection::North) {
                    act(position->direction==XeenDirection::West ? NavigationAction::TurnRight : NavigationAction::TurnLeft);return false;
                }
                if(position->x==10 && position->y==13)return true;
                check(position->x==10 && position->y==12,"M69 entrance approach must start at (10,12)");
                act(NavigationAction::MoveForward);return false;
            });};
            const auto awaitOrc=[&]{steps.push_back([&]{
                // The prepared start view wakes Orc #9; wait facing West until it arrives.
                if(flow->encounter()->combat())return true;
                if(!flow->canSave()) {
                    if(world->sessionState().journeyActivity()==XeenJourneyActivity::Event ||
                       world->sessionState().journeyActivity()==XeenJourneyActivity::Reward)act(AcknowledgeAction{});
                    return false;
                }
                act(WaitAction{});return false;
            });};
            const auto enterVertigo=[&]{action(InteractionAction{});action(YesAction{});settle();
                here(28,15,0,"M69 Vertigo arrival");};
            const auto exitVertigo=[&]{action(InteractionAction{});action(YesAction{});settle();
                here(23,10,12,"M69 mainland return");};
            const auto segmentA=[&]{
                // The prepared start view wakes an Orc: kill it (and every monster
                // the fight wakes) before the first map change.
                report("start");awaitOrc();contactFight(false);report("mainland kill");
                here(23,9,11,"M69 mainland fight cell");
                // The entrance view (10,13 North) wakes Orc #11 at (9,15); no
                // route avoids it and the party cannot reach or shoot it there.
                route("LLULUU");here(23,10,13,"M69 entrance");report("entrance");
                enterVertigo();report("arrival");
                route("URUL");report("before Slime");route("U");report("after Slime");
                route("LLURUUUUUUU");here(28,9,1,"M69 grate approach");report("grate");
                steps.push_back([&]{
                    // Bash until the original grate opens; each blow is ordinary play.
                    if(flow->encounter()->combat())return true; // A monster came through.
                    if(!flow->canSave()) {
                        if(world->sessionState().journeyActivity()==XeenJourneyActivity::Event ||
                           world->sessionState().journeyActivity()==XeenJourneyActivity::Reward)act(AcknowledgeAction{});
                        check(flow->encounter()->state().phase()!=XeenEncounterPhase::SupportStopped,"M69 bash stopped");
                        return false;
                    }
                    const auto cell=world->sampleCell(28,9,1);
                    if(xeenGet<XeenIndoorWalls>(cell->cell->geometry).walls[3]!=9)return true;
                    act(BashAction{});return false;
                });
                report("bashed");contactFight(false);report("after grate fight");
                here(28,9,1,"M69 grate cell after the fight");route("U");contactFight(false);report("past grate");
                route("URU");here(28,7,2,"M69 crate");report("crate");
                action(InteractionAction{});action(SelectMemberAction{3});settle();report("searched");
            };
            const auto segmentB=[&]{
                route("LLULUUUUUUUURU");here(28,15,0,"M69 exit cell");report("exit");
                exitVertigo();report("returned");
            };
            const auto segmentFinal=[&]{
                seekOrc(false);contactFight(true);settle();report("wounded and ran");
                toEntrance();enterVertigo();report("second entry");route("LL");exitVertigo();report("second return");
                seekOrc(true);contactFight(false);report("wounded Orc killed");
            };
            if(!resume) {segmentA();checkpoint("A");segmentB();checkpoint("B");segmentFinal();}
            else if(roundtripStage=="A") {segmentB();checkpoint("B");segmentFinal();}
            else if(roundtripStage=="B") segmentFinal();
            else check(roundtripStage.empty(),"M69 unknown resume checkpoint");
        }
        checkpoint("final");
        inspect([&]{SDL_Event event{};event.type=SDL_QUIT;SDL_PushEvent(&event);});
        auto native=handler;native.closed={};native.beginCycle=[&](std::uint64_t){handler.beginCycle(++cycle);};
        // The simulated clock advances only when idle runs (see the M43 witness);
        // the roundtrip scenario skips the real-time wait. Older scenarios keep theirs.
        if(scenario=="roundtrip")native.nextPresentationDeadline=[]{return std::optional<std::uint64_t>{0};};
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
