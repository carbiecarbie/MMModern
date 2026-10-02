#ifndef MMODERN_TESTS_REGIONAL_COMBAT_SUPPORT_H
#define MMODERN_TESTS_REGIONAL_COMBAT_SUPPORT_H
#include "XeenRegionalTestSupport.h"
#include "XeenCombatTestSupport.h"
#include "formats/xeen/XeenCharacterFormat.h"
namespace regional_combat_test {
using namespace combat_test;
inline XeenCombatRandom *tape=nullptr;
std::optional<std::uint32_t> realDraw(XeenCombatRandom *,std::uint32_t,std::uint32_t) asm("__real__ZN7mmodern16XeenCombatRandom4drawEjj");
std::optional<std::uint32_t> wrappedDraw(XeenCombatRandom *,std::uint32_t,std::uint32_t) asm("__wrap__ZN7mmodern16XeenCombatRandom4drawEjj");
std::optional<std::uint32_t> wrappedDraw(XeenCombatRandom *rng,std::uint32_t lo,std::uint32_t hi){
    auto value=realDraw(rng,lo,hi);
    return tape?realDraw(tape,lo,hi):value;
}
inline bool taped(const XeenCombatRandom &random){
    try{(void)random.continuation();return false;}catch(const std::invalid_argument &){return true;}
}
inline XeenSaveSnapshot combatSnapshot(const XeenCombatRandom &random,unsigned minutes=480){
    struct Pause { XeenCombatRandom *prior=tape; Pause(){tape=nullptr;} ~Pause(){tape=prior;} } pause;
    auto s=regional_test::snapshot();s.journey->context->minutes=minutes;
    const auto bytes=chr();auto p=XeenPartyLoader().loadFromResources(bytes,pty());
    s.characters=p.roster.characters();
    for(unsigned i=0;i<30;++i){
        s.characters[i].learnedSpells=XeenCharacterFormat::parseLearnedSpells(bytes,i);
        s.journey->supplements[i]={static_cast<std::uint8_t>(i),XeenCharacterFormat::parseCombatInputs(bytes,i,true,true,true)};
    }
    s.journey->random=taped(random)?XeenJourneyRandomState{1,1,0}:random.continuation();
    for(auto &a:s.journey->actors){a.hp=0;a.x=a.y=-128;a.activated=false;a.lifecycle=XeenActorLifecycle::Defeated;a.accounted=true;}
    auto &a=s.journey->actors[5];a.hp=20;a.x=8;a.y=11;a.activated=true;a.lifecycle=XeenActorLifecycle::Present;a.accounted=false;
    return s;
}
struct RegionalCombatFixture:regional_test::Fixture {
    XeenCombatRandom rng;
    bool usesTape;
    XeenCombatRandom *previous=tape;
    XeenCombat *combat=nullptr;
    explicit RegionalCombatFixture(XeenCombatRandom random=XeenCombatRandom(1),unsigned minutes=480):regional_test::Fixture(combatSnapshot(random,minutes)),rng(std::move(random)),usesTape(taped(rng)){}
    ~RegionalCombatFixture(){tape=previous;}
    void enter(bool delayed=false){
        if(delayed){actionBefore(XeenEncounterAction::Right);actionBefore(XeenEncounterAction::Forward);}
        actionBefore(XeenEncounterAction::Wait);
        check(flow->state().phase()==XeenEncounterPhase::Engaged,"Regional genuine engagement");
        previous=tape;tape=usesTape?&rng:nullptr;
        check(flow->attachJourney(flow->ticket(),[]{}),"Regional combat attachment");
        combat=flow->combat();check(combat!=nullptr,"Regional combat owner");
    }
    void actionBefore(XeenEncounterAction a){
        flow->journeyAction(flow->ticket(),a);present();
        for(unsigned i=0;i<20 && w.sessionState().journeyActivity()==XeenJourneyActivity::Approach;++i){flow->journeyPulse(flow->ticket());present();}
    }
    XeenTransferResult transfer(std::size_t from,std::size_t to,XeenInventoryCategory cat,std::size_t slot){
        auto r=flow->journeyTransfer(flow->ticket(),from,to,cat,slot);present();return r;
    }
    XeenEquipmentResult equipment(std::size_t owner,XeenInventoryCategory cat,std::size_t slot,XeenEquipmentOperation op){
        auto r=flow->journeyEquipment(flow->ticket(),owner,cat,slot,op);present();return r;
    }
    XeenCombatResult action(Command a){
        auto r=combat->command(combat->ticket(),a);
        for(unsigned i=0;i<100 && combat->pending()==Work::Action;++i)r=combat->service(combat->ticket());
        return r;
    }
    XeenCombatResult service(){return combat->service(combat->ticket());}
    void blockRound(){while(combat->phase()==Phase::PlayerReady)action(Command::Block);check(combat->pending()==Work::Enemy,"Mandatory enemy");}
};
}
#endif
