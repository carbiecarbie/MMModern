#ifndef MMODERN_PURCHASE_TEST_SUPPORT_H
#define MMODERN_PURCHASE_TEST_SUPPORT_H
#include "XeenTrainingTestSupport.h"
#include "games/xeen/XeenEquipmentPurchase.h"
namespace mmodern {
// Synthetic authority/fault controls only; never used in production witnesses.
struct XeenPurchaseTestAccess {
    static std::string text(const XeenEventFlow &f){return f.smithText();}
    static unsigned phase(const XeenEventFlow &f){return f._smithUi?unsigned(f._smithUi->phase):99;}
    static bool quote(const XeenEventFlow &f){return f._smithUi && f._smithUi->phase==XeenEventFlow::SmithUi::Phase::Quote;}
    static bool result(const XeenEventFlow &f){return f._smithUi && f._smithUi->phase==XeenEventFlow::SmithUi::Phase::Result;}
    static bool browse(const XeenEventFlow &f){return f._smithUi && f._smithUi->phase==XeenEventFlow::SmithUi::Phase::Browse;}
    static bool lobby(const XeenEventFlow &f){return f._smithUi && f._smithUi->phase==XeenEventFlow::SmithUi::Phase::Lobby;}
    static bool departure(const XeenEventFlow &f){return f._smithUi && f._smithUi->phase==XeenEventFlow::SmithUi::Phase::Departure;}
    static bool selected(const XeenEventFlow &f){return f._smithUi && f._smithUi->selected;}
    static bool preparing(const XeenEventFlow &f){return bool(f._encounter->_smithPreparation);}
    static std::uint64_t operation(const XeenEventFlow &f){return f._encounter->_smith->operation;}
    static std::uint64_t reservation(const XeenEventFlow &f){return f._encounter->_smith->reservation;}
    static std::uint64_t revision(const XeenEventFlow &f){return f._smithUi->revision;}
    static XeenEquipmentPurchaseResult facts(const XeenEventFlow &f){return f._encounter->_smith->purchase->result;}
    static std::uint64_t counter(const XeenEventFlow &f,unsigned n) {
        const auto &e=*f._encounter;
        switch(n){case 0:return e._generation;case 1:return e._world.sessionState()._journeyGeneration;
        case 2:return f._inputGeneration;case 3:return e._boundary.epoch;case 4:return f._smithUi->revision;
        case 5:return e._smith->operation;case 6:return e._smith->reservation;case 7:return f._generation;}
        throw std::logic_error("Unknown synthetic Smith counter");
    }
    static void limit(XeenEventFlow &f,unsigned n,std::uint64_t value) {
        auto &e=*f._encounter;
        switch(n){case 0:e._generation=value;break;
        case 1:const_cast<XeenSessionWorldState &>(e._world.sessionState())._journeyGeneration=value;break;
        case 2:f._inputGeneration=value;if(e._smith)e._smith->input=value;break;
        case 3:e._boundary.epoch=value;break;
        case 4:f._smithUi->revision=value;f._smithRenderedRevision=value;break;
        case 5:e._smith->operation=value;if(e._smith->quoted){
            e._smith->quoteOperation=value;
            if(e._smith->purchase)e._smith->purchase->result.quotedOperation=value;
        }break;
        case 6:e._smith->reservation=value;if(e._smith->quoted){
            e._smith->quoteReservation=value;
            if(e._smith->purchase){
                e._smith->purchase->result.quotedReservation=value;
                e._smith->purchase->result.publishedReservation=value;
            }
        }break;
        case 7:f._generation=value;if(f._pending)f._pending->generation=value;break;}
        e._journeyPreimage->adoptJourneyCoordination();f._encounterFrame=e.ticket();
    }
    static void consume(XeenEventFlow &f){if(!f._encounter->consumeSmithFrame(f._inputGeneration,f._frame.presentation()))throw std::logic_error("Synthetic Smith authority absent");}
    static void confirm(XeenEventFlow &f){f._encounter->confirmSmithBuy();}
    static void directQuote(XeenEventFlow &f,std::size_t member,XeenInventoryCategory c,unsigned slot){f._encounter->quoteSmithBuy(member,c,slot);}
    static void depart(XeenEventFlow &f){f._encounter->departSmith();}
    static bool room(XeenEventFlow &f,unsigned steps,unsigned boundary){return f._encounter->smithCapacity(steps,boundary);}
    static XeenServiceDayCandidate &day(XeenEventFlow &f){return *f._encounter->_smith->departure;}
    static bool reservationCurrent(XeenEventFlow &f){try{f._encounter->checkSmithReservation();return true;}catch(...){return false;}}
    static IndexedFrame restartPreparation(XeenEventFlow &f){
        checkNoDebt(f);f._encounter->_smithPreparation.reset();f._smithUi.reset();f._smithRenderedRevision.reset();
        // Repeat only optional detached admission inside its actual exclusive
        // Event wrapper, including the no-debt refusal-to-Quiet settlement.
        const bool dispatching=f._dispatching;f._dispatching=true;
        try{auto frame=f.journeyEventWork([&]{f.prepareSmith();});f._dispatching=dispatching;return frame;}
        catch(...){f._dispatching=dispatching;throw;}
    }
    static void checkNoDebt(const XeenEventFlow &f){if(f._encounter->_smith)throw std::logic_error("Synthetic admission reset encountered debt");}
};
}
namespace purchase_test {
using namespace mmodern;
using save_test::check;
using Inputs=training_test::Inputs;
inline XeenSaveSnapshot service(Inputs &in,unsigned day=8,std::uint32_t gold=870) {
    auto s=in.service(day);s.journey->contract=14;s.camera={28,8,4,XeenDirection::West};s.journey->treasure->gold=gold;return s;
}
struct Fixture:training_test::Fixture {
    Fixture(Inputs &in,const XeenSaveSnapshot &s,bool animated=false):training_test::Fixture(in,s,animated) {
        flow->drawSmithArt=[&in](auto &frame){in.assets.drawSmith(frame);};
    }
    void enter(){act(InteractionAction{});if(!flow->canSave())prepare();check(XeenPurchaseTestAccess::lobby(*flow),"synthetic Smith lobby absent");}
    void buy(){act(BlockAction{});check(XeenPurchaseTestAccess::browse(*flow),"Buy browser absent");}
    void choose(XeenInventoryCategory category,unsigned slot,std::size_t member=0) {
        act(SelectMemberAction{member});
        for(unsigned n=0;n<unsigned(category);++n)act(NavigationAction::TurnRight);
        act(SelectInventorySlotAction{slot});
    }
    void quote(XeenInventoryCategory category,unsigned slot,std::size_t member=0) {
        buy();choose(category,slot,member);act(AcknowledgeAction{});check(XeenPurchaseTestAccess::quote(*flow),"Buy quote absent");
    }
    void purchaseArmor(){quote(XeenInventoryCategory::Armor,3);act(AcknowledgeAction{});check(XeenPurchaseTestAccess::result(*flow),"Buy result absent");}
    void leave(){
        for(unsigned n=0;n<40 && !flow->canSave();++n) {
            if(XeenPurchaseTestAccess::quote(*flow))act(CancelInteractionAction{});
            else if(XeenPurchaseTestAccess::result(*flow))act(AcknowledgeAction{});
            else act(CancelInteractionAction{});
        }
        check(flow->canSave(),"reserved Smith departure did not settle");
    }
};
struct Owners {
    std::array<XeenCharacter,30> characters;
    std::array<XeenCombatInputs,30> inputs;
    XeenMonsterTreasure treasure;XeenGameplayContext context;XeenServiceEconomy economy;XeenJourneyRandomState random;
    XeenCamera camera;
    explicit Owners(const Fixture &f):treasure(*f.p.monsterTreasure),context(*f.p.encounterContext),
        economy(*f.p.serviceEconomy),random(*f.w.sessionState().journeyRandom()),camera(f.c) {
        for(unsigned n=0;n<30;++n){characters[n]=f.p.roster.at(n);inputs[n]=*f.p.roster.combatInputs(n);}
    }
    void unchanged(const Fixture &f) const {
        for(unsigned n=0;n<30;++n)check(xeen_state::sameCharacter(characters[n],f.p.roster.at(n)) &&
            xeen_state::sameInputs(inputs[n],*f.p.roster.combatInputs(n)),"modal action changed character/supplement without publication");
        check(treasure==*f.p.monsterTreasure && context==*f.p.encounterContext && economy==*f.p.serviceEconomy &&
            random==*f.w.sessionState().journeyRandom() && save_test::sameCamera(camera,f.c),"modal action changed purse/stock/time/RNG/camera without publication");
    }
};
inline void armorDelivery(const Fixture &f) {
    check(f.p.monsterTreasure->gold==670 && f.p.roster.at(0).armor[4].id==3 && !f.p.roster.at(0).armor[4].frame,"Armor payment/delivery missing/duplicated");
}
inline void armorPaid(const Fixture &f) {
    armorDelivery(f);check(
        f.p.serviceEconomy->wares[0][0][1][3].id==5 && !f.p.serviceEconomy->wares[0][0][1][6].id,"Armor purchase publication missing/duplicated");
}
inline void denied(Fixture &f) {
    check(!f.flow->canSave() && f.flow->serviceSaveBlocked(),"active service exposes save authority");
    save_test::rejects([&]{f.flow->beginSave();});
    check(!XeenSaveState::canCapture(f.p,f.c,f.w),"active service permits direct capture");
}
}
#endif
