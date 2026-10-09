#include "XeenTestInstallation.h"
#include "XeenPurchaseTestSupport.h"
#include <iostream>
using namespace purchase_test;
namespace {
std::string context;
void limits(Inputs &in) {
    for(int margin:{-1,0,1}) {
        context="Event ticket margin "+std::to_string(margin);
        Fixture f(in,service(in));const Owners before(f);XeenPurchaseTestAccess::limit(*f.flow,7,UINT64_MAX-1-margin);f.act(InteractionAction{});
        if(margin<0){check(f.flow->canSave(),"exhausted Event ticket admitted Smith");before.unchanged(f);}
        else{f.prepare();f.leave();check(f.p.encounterContext->day==9,"exact Event ticket lost departure");}
    }
    for(unsigned phase=0;phase<2;++phase)for(unsigned counter=0;counter<4;++counter)for(int margin:{-1,0,1}) {
        context="Admission phase "+std::to_string(phase)+" counter "+std::to_string(counter)+" margin "+std::to_string(margin);
        Fixture f(in,service(in));f.act(InteractionAction{});
        check(XeenPurchaseTestAccess::preparing(*f.flow),"Smith admission did not retain exclusive preparation");denied(f);
        const unsigned need=counter<2?(phase?17:20):counter==2?(phase?9:10):5;
        XeenPurchaseTestAccess::limit(*f.flow,counter,UINT64_MAX-need-margin);
        if(!phase)f.present(XeenPurchaseTestAccess::restartPreparation(*f.flow));
        if(!f.flow->canSave())f.prepare();
        if(margin<0)check(f.flow->canSave() && f.p.encounterContext->day==8 && f.p.monsterTreasure->gold==870,"insufficient admission acquired debt");
        else {check(XeenPurchaseTestAccess::lobby(*f.flow),"exact/one-more admission refused");f.leave();check(f.p.encounterContext->day==9,"admission bound lost mandatory departure");}
    }
    for(unsigned counter=0;counter<7;++counter)for(int margin:{-1,0,1}) {
        context="Optional counter "+std::to_string(counter)+" margin "+std::to_string(margin);
        Fixture f(in,service(in));f.enter();
        const bool confirmation=counter==6;
        if(counter>=5){f.quote(XeenInventoryCategory::Armor,3);if(counter==5){f.act(CancelInteractionAction{});}}
        const unsigned need=counter<2?(confirmation?19:18):counter==2?10:counter==3?4:1;
        XeenPurchaseTestAccess::limit(*f.flow,counter,UINT64_MAX-need-margin);
        const auto before=f.p.monsterTreasure->gold;
        if(counter==5)f.act(SelectInventorySlotAction{3});
        else if(confirmation)f.act(YesAction{});
        else f.act(SelectMemberAction{1});
        if(margin<0)check(XeenPurchaseTestAccess::departure(*f.flow) && f.p.monsterTreasure->gold==before,"optional exhaustion consumed departure authority");
        else if(counter==5)check(XeenPurchaseTestAccess::quote(*f.flow),"exact/one-more operation ID refused quote");
        else if(confirmation)armorPaid(f);
        else check(!XeenPurchaseTestAccess::departure(*f.flow),"exact/one-more optional selection refused");
        const auto input=f.flow->displayedInput();const auto revision=XeenPurchaseTestAccess::revision(*f.flow);
        for(unsigned n=0;n<30;++n)f.act(NavigationAction::MoveForward);
        check(input==f.flow->displayedInput() && revision==XeenPurchaseTestAccess::revision(*f.flow),"wrong key consumed reserved suffix");
        f.leave();check(f.p.encounterContext->day==9 && f.p.monsterTreasure->gold==(confirmation && margin>=0?670u:870u),"authority boundary lost/repeated payment/departure");
    }
    // Confirmed publication reserves its own 3/2 increment bound independently
    // of operation/reservation exhaustion and every other counter.
    for(unsigned counter=0;counter<4;++counter)for(int margin:{-1,0,1}) {
        context="Confirmation counter "+std::to_string(counter)+" margin "+std::to_string(margin);
        Fixture f(in,service(in));f.enter();f.quote(XeenInventoryCategory::Armor,3);
        const unsigned need=counter<2?19:counter==2?10:4;
        XeenPurchaseTestAccess::limit(*f.flow,counter,UINT64_MAX-need-margin);f.act(YesAction{});
        if(margin<0)check(XeenPurchaseTestAccess::departure(*f.flow) && f.p.monsterTreasure->gold==870,"confirmation headroom shortage published");else armorPaid(f);
        f.leave();check(f.p.encounterContext->day==9,"confirmation threshold lost settlement");
    }
}
void connectedSynthetic(Inputs &in) {
    for(unsigned day:{8u,9u,10u,97u,98u}) {
        context="Connected synthetic day "+std::to_string(day);
        auto source=service(in,day);if(day==10)source.journey->random=XeenJourneyRandomState{1,2732157854u,1203};
        Fixture f(in,source);const Owners before(f);f.enter();denied(f);
        f.quote(XeenInventoryCategory::Armor,3);denied(f);f.act(CancelInteractionAction{});before.unchanged(f);
        f.act(SelectInventorySlotAction{3});f.act(YesAction{});armorPaid(f);denied(f);
        const auto reservation=XeenPurchaseTestAccess::reservation(*f.flow);check(reservation==2,"purchase did not replace complete reservation once");
        const auto facts=XeenPurchaseTestAccess::facts(*f.flow);
        check(facts.quotedOperation==XeenPurchaseTestAccess::operation(*f.flow) && facts.quotedReservation==1 &&
            facts.publishedReservation==2,"fixed purchase result omitted exact operation/reservation identities");
        const Owners paid(f);f.act(YesAction{});check(!XeenPurchaseTestAccess::selected(*f.flow),"success followed shifted row implicitly");
        f.act(YesAction{});paid.unchanged(f);check(XeenPurchaseTestAccess::browse(*f.flow),"unselected Buy Enter purchased shifted row");
        f.act(SelectInventorySlotAction{0});f.act(YesAction{});f.act(YesAction{});
        paid.unchanged(f);check(XeenPurchaseTestAccess::result(*f.flow),"actual armor6 insufficient refusal absent");
        f.leave();armorDelivery(f);check(f.p.encounterContext->day==day+1 && f.p.encounterContext->minutes==source.journey->context->minutes &&
            XeenSaveState::canCapture(f.p,f.c,f.w),"one-day settlement/time/save boundary differs");
        if(day==10) {
            check(*f.w.sessionState().journeyRandom()==XeenJourneyRandomState{1,3686439625u,2109},"synthetic trigger expected raw cursor differs");
            check(*f.p.serviceEconomy!=before.economy,"trigger failed full-stock replacement");
        } else check(*f.w.sessionState().journeyRandom()==before.random && f.p.serviceEconomy->wares[0][0][1][3].id==5,"nontrigger lost depletion/RNG");
        const auto saved=f.snapshot();Fixture restored(in,saved);save_test::sameSnapshot(saved,restored.snapshot());
    }
    {
        auto source=service(in,99);Fixture f(in,source);const Owners before(f);f.enter();check(!f.flow->canSave(),"Smith retains service debt across rollover");before.unchanged(f);f.leave();
        check(f.flow->canSave() && f.p.encounterContext->day==0 && f.p.encounterContext->year==611,"Smith rollover departure");
    }
    // Buy and inherited repair use one lobby/lease/departure. Distinct units
    // retain prior payments/items even through later cancelled operations.
    {
        context="Same-visit Buy/Repair coexistence";
        auto source=service(in);
        check(source.characters[0].armor[0].material==0 && source.characters[0].armor[0].id==3,
            "original Arturius first armor differs from the literal repair oracle");
        source.characters[0].armor[0].state=128;
        Fixture f(in,source);f.enter();f.purchaseArmor();armorPaid(f);f.act(YesAction{});f.act(CancelInteractionAction{});
        f.act(DialogKeyAction{'b'});f.act(DialogKeyAction{'a'});f.act(DialogKeyAction{'f'});f.act(SelectInventorySlotAction{0});f.act(YesAction{});f.act(YesAction{});
        // Plain ID 3 has literal base cost 200; inherited repair is 200/10=20.
        check(f.p.monsterTreasure->gold==650 && !f.p.roster.at(0).armor[0].state,"Buy/Repair coexistence lost old repair arithmetic");
        f.leave();check(f.p.encounterContext->day==9 && f.p.monsterTreasure->gold==650 && f.p.roster.at(0).armor[4].id==3,"coexistence paid two departures/lost purchase");
    }
    {
        const auto source=service(in,8,120);unsigned count=0;for(const auto &item:source.characters[0].weapons)if(item.id)++count;
        Fixture f(in,source);f.enter();f.quote(XeenInventoryCategory::Weapons,1);f.act(YesAction{});
        check(f.p.monsterTreasure->gold==60,"first duplicate payment differs");f.act(YesAction{});
        f.act(SelectInventorySlotAction{3});f.act(YesAction{});f.act(YesAction{});
        check(f.p.monsterTreasure->gold==0 && f.p.roster.at(0).weapons[count].id==6 && f.p.roster.at(0).weapons[count+1].id==6,
            "two physical duplicate offers were not delivered separately");f.leave();
    }
}
void faults(Inputs &in) {
    for(unsigned day:{8u,10u}) {
        context="RNG exhaustion day "+std::to_string(day);
        auto source=service(in,day);source.journey->random->count=UINT64_MAX-1;
        Fixture f(in,source);const Owners before(f);f.act(InteractionAction{});if(!f.flow->canSave())f.prepare();
        if(day==10){check(f.flow->canSave(),"RNG exhaustion during complete admission armed debt");before.unchanged(f);}
        else {f.quote(XeenInventoryCategory::Armor,3);f.act(YesAction{});armorPaid(f);f.leave();
            check(*f.w.sessionState().journeyRandom()==before.random,"nontrigger purchase/departure drew from exhausted-near cursor");}
    }
    for(auto boundary:{XeenSmithBoundary::BeforeReservation,XeenSmithBoundary::AfterReservation,XeenSmithBoundary::BeforeAdmission,
        XeenSmithBoundary::StockComplete,XeenSmithBoundary::BankPrepared,XeenSmithBoundary::AfterAdmission,XeenSmithBoundary::Quote,
        XeenSmithBoundary::BeforeRebind,XeenSmithBoundary::AfterRebind,XeenSmithBoundary::BeforePurchase,XeenSmithBoundary::AfterPurchase,
        XeenSmithBoundary::BeforeDeparture,XeenSmithBoundary::DeparturePublished,XeenSmithBoundary::AfterDeparture,XeenSmithBoundary::Return,
        XeenSmithBoundary::BeforeEventSettlement,XeenSmithBoundary::AfterEventSettlement}) {
        context="Fault boundary "+std::to_string(unsigned(boundary));
        Fixture f(in,service(in,10));bool fired=false;
        f.flow->smithBoundary=[&](auto here){denied(f);if(here==boundary && !fired){fired=true;throw std::bad_alloc();}};
        f.act(InteractionAction{});if(!f.flow->canSave())f.prepare();if(f.flow->canSave())f.enter();
        check(XeenPurchaseTestAccess::lobby(*f.flow),"admission fault lost service continuation");
        f.quote(XeenInventoryCategory::Armor,3);f.act(YesAction{});
        if(XeenPurchaseTestAccess::quote(*f.flow))f.act(YesAction{});
        armorPaid(f);f.leave();check(fired && f.p.encounterContext->day==11 && f.p.monsterTreasure->gold==670,"boundary fault lost/duplicated committed prefix/departure");
    }
    for(auto boundary:{XeenSmithBoundary::BeforeRebind,XeenSmithBoundary::AfterRebind,XeenSmithBoundary::BeforePurchase}) {
        Fixture f(in,service(in));f.enter();f.quote(XeenInventoryCategory::Armor,3);const Owners before(f);unsigned faults=0;
        f.flow->smithBoundary=[&](auto here){if(here==boundary && ++faults<=3)throw std::bad_alloc();};
        const auto input=f.flow->displayedInput();const auto operation=XeenPurchaseTestAccess::operation(*f.flow);
        for(unsigned n=0;n<3;++n){f.act(YesAction{});before.unchanged(f);check(input==f.flow->displayedInput() && operation==XeenPurchaseTestAccess::operation(*f.flow) &&
            XeenPurchaseTestAccess::reservation(*f.flow)==1 && XeenPurchaseTestAccess::reservationCurrent(*f.flow),"failed optional preparation lost/rebound old obligation");}
        f.act(YesAction{});armorPaid(f);f.leave();
    }
    for(auto boundary:{XeenSmithBoundary::BeforeDeparture,XeenSmithBoundary::Return,XeenSmithBoundary::BeforeEventSettlement}) {
        Fixture f(in,service(in));f.enter();f.purchaseArmor();unsigned failures=0;
        f.flow->smithBoundary=[&](auto here){if(here==boundary && failures++<12)throw std::bad_alloc();};
        f.act(YesAction{});f.act(CancelInteractionAction{});f.act(CancelInteractionAction{});
        const auto input=f.flow->displayedInput();while(!f.flow->canSave()){f.act(CancelInteractionAction{});check(failures<=13,"mandatory retry bound exceeded");
            if(!f.flow->canSave())check(input==f.flow->displayedInput(),"same settlement retry consumed semantic authority");}
        check(f.p.encounterContext->day==9 && f.p.monsterTreasure->gold==670,"postcommit/departure retries repeated payment/day");
    }
    {
        Fixture f(in,service(in));f.enter();f.quote(XeenInventoryCategory::Armor,3);const auto old=f.flow->frame().presentation();const auto input=f.flow->displayedInput();bool called=false;
        f.flow->smithBoundary=[&](auto here){if(here==XeenSmithBoundary::BeforePurchase){called=true;denied(f);
            f.flow->handle(AcknowledgeAction{},input,old);f.flow->handle(CancelInteractionAction{},input,old);}};
        f.act(YesAction{});check(called,"reentrant purchase callback unrun");armorPaid(f);f.leave();
    }
    // Tampering the private completed ending state is caught by the immutable
    // binding even when its new value is still an otherwise canonical economy.
    for(unsigned day:{8u,10u}) {
        Fixture f(in,service(in,day));f.enter();f.quote(XeenInventoryCategory::Armor,3);
        auto &reserved=XeenPurchaseTestAccess::day(*f.flow);
        ++const_cast<XeenServiceEconomy &>(reserved.economy()).bank.gold;
        save_test::rejects([&]{f.act(YesAction{});});check(!f.flow->canSave() && f.p.monsterTreasure->gold==870,"tampered departure published or became Quiet");
    }
}
void feedbackRetries(Inputs &in) {
    const XeenItemCategory depleted{{{0,6,0,0},{0,4,0,0},{0,6,0,0},
        {0,5,0,0},{40,8,0,0},{48,6,0,0},{},{},{}}};
    for(unsigned kind=0;kind<2;++kind) {
        context=kind?"Successful report followed by settlement fault":"Paid Buy frame-copy reconstruction";
        Fixture f(in,service(in));f.enter();const Owners before(f);
        auto expected=before;expected.treasure.gold=670;
        expected.characters[0].armor[4]={0,3,0,0};expected.economy.wares[0][0][1]=depleted;
        bool armed=false,copyFailed=false;unsigned copyCalls=0,rebuilds=0,reports=0,settlements=0;
        if(!kind) {
            f.flow->smithBoundary=[&](auto here){if(here==XeenSmithBoundary::AfterPurchase)armed=true;};
            f.flow->beforeEncounterFrameCopy=[&]{if(armed){++copyCalls;if(!copyFailed){copyFailed=true;
                throw std::runtime_error("Synthetic paid Buy frame-copy retry");}}};
            f.flow->rebuildEncounterPresentation=[&]{++rebuilds;};
        }
        f.purchaseArmor();expected.unchanged(f);denied(f);
        const auto facts=XeenPurchaseTestAccess::facts(*f.flow);
        check(facts.outcome==XeenEquipmentPurchaseOutcome::Purchased && facts.category==XeenInventoryCategory::Armor &&
            facts.side==0 && facts.shop==0 && facts.member==0 && facts.owner==0 && facts.offerSlot==3 && facts.recipientSlot==4 &&
            xeenSameItem(facts.offer,{0,3,0,0}) && facts.price==200 && facts.goldBefore==870 && facts.goldAfter==670 && facts.shortfall==0 &&
            facts.quotedOperation==1 && facts.quotedReservation==1 && facts.publishedReservation==2 &&
            xeen_state::sameItemCategory(facts.recipientBefore,before.characters[0].armor) &&
            xeen_state::sameItemCategory(facts.recipientAfter,expected.characters[0].armor) &&
            xeen_state::sameItemCategory(facts.stockBefore,before.economy.wares[0][0][1]) &&
            xeen_state::sameItemCategory(facts.stockAfter,depleted),"feedback retry lost independent fixed purchase facts");
        check(XeenPurchaseTestAccess::operation(*f.flow)==1 && XeenPurchaseTestAccess::reservation(*f.flow)==2 &&
            XeenPurchaseTestAccess::day(*f.flow).beforeEconomy()==expected.economy &&
            XeenPurchaseTestAccess::day(*f.flow).economy()==expected.economy &&
            XeenPurchaseTestAccess::day(*f.flow).continuation()==before.random &&
            XeenPurchaseTestAccess::reservationCurrent(*f.flow),"feedback retry repeated operation/rebind or changed prepared departure");
        if(!kind) {
            // The first-attempt probe is intentionally skipped on recovery;
            // its distinct reconstruction callback proves the second attempt.
            check(copyFailed && copyCalls==1 && rebuilds==1 && XeenPurchaseTestAccess::result(*f.flow),
                "paid Buy did not reconstruct one healthy failed frame copy");
            const auto input=f.flow->displayedInput();f.present(f.flow->refresh(true));expected.unchanged(f);
            const auto repeated=XeenPurchaseTestAccess::facts(*f.flow);
            check(input==f.flow->displayedInput() && repeated.quotedOperation==1 && repeated.quotedReservation==1 &&
                repeated.publishedReservation==2 && repeated.goldBefore==870 && repeated.goldAfter==670 &&
                xeen_state::sameItemCategory(repeated.recipientAfter,expected.characters[0].armor) &&
                xeen_state::sameItemCategory(repeated.stockAfter,depleted),"result redraw renewed/recomputed paid facts");
            f.leave();expected.context.day=9;expected.unchanged(f);
        } else {
            // A completed report precedes this later fault. Retrying the retained
            // terminal suffix must skip that successful callback entirely.
            f.flow->reportManual=[&](const auto &result){++reports;
                check(std::holds_alternative<XeenManualEventCompleted>(result),"Smith terminal report changed result kind");};
            f.flow->smithBoundary=[&](auto here){if(here==XeenSmithBoundary::AfterEventSettlement && ++settlements==1)
                throw std::runtime_error("Synthetic fault after successful Smith report");};
            f.act(YesAction{});f.act(CancelInteractionAction{});f.act(CancelInteractionAction{});
            expected.context.day=9;expected.unchanged(f);denied(f);
            check(reports==1 && settlements==1,"successful report/later settlement fault unexercised");
            f.act(CancelInteractionAction{});expected.unchanged(f);
            check(f.flow->canSave() && reports==1 && settlements==2,
                "later terminal retry repeated successful report/payment/delivery/depletion/day");
        }
    }
}
void integrity(Inputs &in) {
    for(unsigned family=0;family<22;++family)for(bool throwing:{false,true}) {
        context="Integrity family "+std::to_string(family)+" throwing "+std::to_string(throwing);
        Fixture f(in,service(in));f.enter();f.purchaseArmor();f.act(YesAction{});
        f.act(SelectInventorySlotAction{1});bool fired=false;
        f.flow->smithBoundary=[&](auto here){if(here!=XeenSmithBoundary::BeforePurchase || fired)return;fired=true;
            switch(family) {
            case 0:++f.p.roster.at(2).temporaryAge;--f.p.roster.at(2).temporaryAge;break;
            case 1:{auto &i=const_cast<XeenCombatInputs &>(*f.p.roster.combatInputs(2));++i.experience;--i.experience;break;}
            case 2:f.p.roster.at(2).learnedSpells->at(38)^=1;f.p.roster.at(2).learnedSpells->at(38)^=1;break;
            case 3:++f.p.monsterTreasure->pendingGold;--f.p.monsterTreasure->pendingGold;break;
            case 4:++f.p.serviceEconomy->bank.gold;--f.p.serviceEconomy->bank.gold;break;
            case 5:f.p.serviceEconomy->wares[1][3][3][8].material^=1;f.p.serviceEconomy->wares[1][3][3][8].material^=1;break;
            case 6:++f.c.x;--f.c.x;break;
            case 7:++f.p.encounterContext->minutes;--f.p.encounterContext->minutes;break;
            case 8:{auto &m=const_cast<XeenMap &>(f.w.map(28));m.geometry.cells[0].rawAttributes^=1;m.geometry.cells[0].rawAttributes^=1;break;}
            case 9:{auto &a=const_cast<XeenActor &>(f.w.sessionState().regionalActors(28)[34]);++a.hp;--a.hp;break;}
            case 10:{auto &a=const_cast<XeenActor &>(f.w.sessionState().actors()[0]);++a.hp;--a.hp;break;}
            case 11:{auto &rng=const_cast<XeenMutableOptional<XeenJourneyRandomState> &>(f.w.sessionState().journeyRandom());++rng->count;--rng->count;break;}
            case 12:f.p.roster.at(2).armor[8].frame^=1;f.p.roster.at(2).armor[8].frame^=1;break;
            case 13:{const auto v=f.p.serviceEconomy;f.p.serviceEconomy.reset();f.p.serviceEconomy=v;break;}
            case 14:{auto &i=const_cast<XeenMutableOptional<XeenCombatInputs> &>(f.p.roster.combatInputs(2));const auto v=i;i.reset();i=v;++i->experience;--i->experience;break;}
            case 15:{f.w.discardMapCache();auto &m=const_cast<XeenMap &>(f.w.map(28));m.geometry.cells[0].rawAttributes^=1;m.geometry.cells[0].rawAttributes^=1;break;}
            case 16:{f.w.discardMapCache();auto &o=const_cast<XeenObjectFile &>(f.w.objectFile(28));o.entities.objects[0].x^=1;o.entities.objects[0].x^=1;break;}
            case 17:f.p.roster.at(2).temporaryAge=f.p.roster.at(2).temporaryAge;break;
            case 18:{const bool v=f.f.isSet(0);f.f.set(0);f.f.clear(0);if(v)f.f.set(0);break;}
            case 19:f.p.party=f.p.party;break;
            case 20:++f.p.monsterTreasure->gold;--f.p.monsterTreasure->gold;break;
            case 21:f.p.roster.at(0).armor[4].id^=1;f.p.roster.at(0).armor[4].id^=1;break;
            }
            if(throwing)throw std::runtime_error("Purchase ABA exceptional exit");
        };
        save_test::rejects([&]{f.act(YesAction{});});check(fired && !f.flow->canSave() && f.p.monsterTreasure->gold==670 &&
            f.p.roster.at(0).armor[4].id==3 && f.p.encounterContext->day==8,"ABA failure lost prior purchase or published/adopted another");
    }
}
}
int main(int argc,char **argv) {
    try {
        check(argc==2,"usage: purchase-flow <installation>");const auto installation=xeenTestInstallationDetector().detect(argv[1]);check(bool(installation),"installation unavailable");
        Inputs in(*installation);limits(in);connectedSynthetic(in);faults(in);feedbackRetries(in);integrity(in);
        std::cout<<"M42 synthetic Flow, independent counter bounds, failures, rebind and mutation-history checks passed\n";return 0;
    }catch(const std::exception &e){std::cerr<<context<<": "<<e.what()<<'\n';return 1;}
}
