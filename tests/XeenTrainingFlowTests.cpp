#include "XeenTestInstallation.h"
#include "XeenTrainingTestSupport.h"
#include "XeenPurchaseTestSupport.h"
#include "XeenM42Evidence.h"
#include "games/xeen/XeenItemTransfer.h"
#include <iostream>
using namespace training_test;
using save_test::rejects;
int main(int argc,char **argv) {
    try {
        check(argc==2,"usage: training-flow <original-installation>");const auto installation=xeenTestInstallationDetector().detect(argv[1]);
        check(bool(installation),"installation unavailable");Inputs inputs(*installation);const auto source=inputs.service();
        for(bool cartographer:{false,true}) {
            auto arrival=source;save_test::clearExplored(arrival);
            for(auto &c:arrival.characters)c.skills[4]=0;
            if(cartographer)arrival.characters[arrival.activeRosterIds.front()].skills[4]=1;
            Fixture f(inputs,arrival);f.enter();bool departure=false;
            f.flow->trainingBoundary=[&](auto point){if(point==XeenTrainingBoundary::DeparturePublished) {
                check(f.w.scenePresentation().lightingStepped,"Training departure leaves a step without requiring Cartographer");
                departure=true;
            }};
            f.act(CancelInteractionAction{});f.finishDeparture();
            check(departure && f.w.sampleCell(f.c.mapId,f.c.x,f.c.y)->cell->stepped==cartographer,
                "Training return frame marks only with an active Cartographer");
        }
        // Original resources with explicitly injected boundary dates: ordinary
        // Wait and the shared dawn notice, followed by quiet save/reload.
        for(unsigned minute:{299u,1439u}) {
            auto start=source;auto &time=*start.journey->context;
            time.day=99;time.year=610;time.minutes=minute;time.newDay=minute==299;
            start.food=27;start.journey->supplements[0].inputs.resistances->fireTemporary=7;
            Fixture f(inputs,start,true);const auto camera=f.c;f.act(WaitAction{});
            bool sawNotice=false;
            for(unsigned n=0;n<1000 && !f.flow->canSave();++n) {
                f.now+=125;f.flow->beginCycle(++f.cycle);
                if(const auto frame=f.flow->updatePresentation())f.present(*frame);
                const auto context=f.flow->inputContext(f.flow->frame().presentation());
                if(context.dialog && context.dialog->anyKey) {
                    check(!XeenSaveState::canCapture(f.p,f.c,f.w),"Dawn notice must block quiet capture");
                    sawNotice=true;f.act(NavigationAction::MoveForward);
                    check(save_test::sameCamera(camera,f.c),"Notice acknowledgment leaked movement");
                }
                check(!f.flow->encounter()->combat(),"Boundary fixture unexpectedly engaged actors");
            }
            check(f.flow->canSave() && f.p.food==27,"Boundary Wait failed quiet settlement or changed food");
            check(f.p.encounterContext->minutes==(minute==299?300:0) &&
                f.p.encounterContext->day==(minute==299?99:0) &&
                f.p.encounterContext->year==(minute==299?610:611),"Ordinary boundary charge/calendar");
            check(sawNotice==(minute==299) && f.p.encounterContext->newDay==(minute==1439),"Dawn notice/pending flag");
            check(f.p.roster.combatInputs(0)->resistances->fireTemporary==(minute==299?0:7),"Daily live resistance reset");
            const auto saved=f.snapshot();Fixture restored(inputs,saved);save_test::sameSnapshot(saved,restored.snapshot());
        }
        constexpr std::array<std::uint32_t,10> bases{1500,2000,2000,1500,2000,1000,1500,1500,1500,2000};
        for(unsigned cls=0;cls<10;++cls)for(unsigned level:{1u,3u,4u,9u,11u,12u,13u,19u}) {
            auto s=source;s.characters[18].characterClass=static_cast<XeenCharacterClass>(cls);s.characters[18].permanentLevel=level;
            s.characters[18].weapons={};s.characters[18].armor={};s.characters[18].accessories={};s.characters[18].miscellaneous={};
            const unsigned debit=level>=12?1024000u:bases[cls]*(1u<<(level==1?0:level-2));
            s.journey->supplements[18].inputs.experience=debit+1;s.journey->purse->gold=10*level*level;
            Fixture fixture(inputs,s);const auto delta=xeenPrepareTraining(fixture.p,18,*fixture.p.encounterContext,XeenMapIdentity(level<10?28:29));
            check(delta.result.outcome==XeenTrainingOutcome::Trained && delta.result.levelAfter==level+1 &&
                delta.result.xpAfter==1 && delta.result.goldAfter==0 && delta.result.maxHpAfter==delta.result.hpAfter &&
                delta.result.maxSpAfter==delta.result.spAfter,"class/level progression debit/remainder/exact funds/derived refill differs");
        }
        // Independent reset matrix with nonzero selected, other active and
        // inactive fields, raw books/items, age and conditions preserved.
        auto matrix=source;matrix.journey->purse->gold=90;matrix.characters[18].currentHp=100;
        for(unsigned id:{18u,0u,2u}) {
            auto &c=matrix.characters[id];auto &i=matrix.journey->supplements[id].inputs;
            c.temporaryLevel=2;c.intellect.temporary=3;c.personality.temporary=4;c.endurance.temporary=5;c.temporaryAge=6;
            i.might.temporary=7;i.speed.temporary=8;i.accuracy.temporary=9;i.temporaryAc=10;
            i.luck->temporary=11;i.resistances->coldTemporary=12;i.resistances->electricalTemporary=13;i.poisonResistance->temporary=14;
            c.learnedSpells->at(38)=255;
            c.armor[8]={255,0,255,0};
        }
        matrix.characters[0].conditions[8]=1;
        matrix.journey->context->effects.fill(1);matrix.journey->context->lightAndResistances.fill(3);
        for(unsigned id:{18u,0u,2u}) {
            auto &r=*matrix.journey->supplements[id].inputs.resistances;
            r.fireTemporary=15;r.energyTemporary=16;r.magicTemporary=17;
        }
        matrix.journey->supplements[18].inputs.experience=3000;
        {
            Fixture fixture(inputs,matrix);const auto inactive=fixture.p.roster.at(2);const auto inactiveInputs=*fixture.p.roster.combatInputs(2);
            fixture.enter();fixture.train(1);
            check(!fixture.p.encounterContext->effects[XeenPartyEffect::Levitate] && fixture.p.encounterContext->effects[XeenPartyEffect::AutomapOn]==1 &&
                fixture.p.encounterContext->lightAndResistances[1]==3,"Training reset preserves automap/torches");
            for(auto effect:kXeenPartyEffects)if(effect!=XeenPartyEffect::AutomapOn)check(!fixture.p.encounterContext->effects[effect],"Training party effect survived");
            for(unsigned n:{0u,2u,3u,4u,5u})check(!fixture.p.encounterContext->lightAndResistances[n],"Training party light/resistance survived");
            check(fixture.p.roster.at(18).permanentLevel==4 && fixture.p.roster.at(18).currentHp==64 && fixture.p.roster.at(18).currentSp==0 &&
                fixture.p.roster.combatInputs(18)->experience==0 && fixture.p.purse->gold==0 && fixture.p.encounterContext->day==9,"exact funds/reset/refill/progression mismatch");
            for(auto id:kXeenCombatOwners) {
                const auto &c=fixture.p.roster.at(id);const auto &i=*fixture.p.roster.combatInputs(id);
                check(!c.temporaryLevel && !c.intellect.temporary && !c.personality.temporary && !c.endurance.temporary && !i.might.temporary &&
                    !i.speed.temporary && !i.accuracy.temporary && !i.temporaryAc && !i.luck->temporary && !i.resistances->coldTemporary &&
                    !i.resistances->electricalTemporary && !i.resistances->fireTemporary && !i.resistances->energyTemporary &&
                    !i.resistances->magicTemporary && !i.poisonResistance->temporary,"active reset matrix mismatch");
                check(c.temporaryAge==matrix.characters[id].temporaryAge && c.conditions==matrix.characters[id].conditions &&
                    c.learnedSpells==matrix.characters[id].learnedSpells,"reset changed age/conditions/book");
                check(xeen_state::sameItemCategory(c.weapons,matrix.characters[id].weapons) &&
                    xeen_state::sameItemCategory(c.armor,matrix.characters[id].armor) && xeen_state::sameItemCategory(c.accessories,matrix.characters[id].accessories) &&
                    xeen_state::sameItemCategory(c.miscellaneous,matrix.characters[id].miscellaneous),"Training changed an equipment byte");
                if(id!=18)check(c.currentHp==matrix.characters[id].currentHp && c.currentSp==matrix.characters[id].currentSp,"reset clamped/refilled nonselected member");
            }
            check(xeen_state::sameCharacter(inactive,fixture.p.roster.at(2)) && xeen_state::sameInputs(inactiveInputs,*fixture.p.roster.combatInputs(2)),"inactive owner reset");
            fixture.act(CancelInteractionAction{});fixture.finishDeparture();check(fixture.flow->canSave() && fixture.p.encounterContext->day==10,"separate departure failed");
        }
        for(unsigned day:{8u,9u,97u,98u,99u}) {
            std::cout<<"Day fixture "<<day<<'\n';
            auto s=source;s.journey->context->day=day;Fixture fixture(inputs,s);
            fixture.enter();check(!fixture.flow->canSave() && !XeenSaveState::canCapture(fixture.p,fixture.c,fixture.w),"admitted visit save permitted");
            fixture.train(1);
            check(fixture.p.roster.at(18).permanentLevel==4 && fixture.p.encounterContext->day==(day+1)%100,"first member day missing");
            fixture.act(SelectMemberAction{0});fixture.train(1);
            check(fixture.p.roster.at(18).permanentLevel==5 && fixture.p.roster.combatInputs(18)->experience==0 &&
                fixture.p.encounterContext->day==(day+1)%100,"same-member repeat charged time or XP incorrectly");
            fixture.train(4);
            check(fixture.p.encounterContext->day==(day+2)%100 && fixture.p.roster.at(1).permanentLevel==4,"new-member day lost at rollover");
            fixture.act(CancelInteractionAction{});fixture.finishDeparture();
            check(fixture.flow->canSave() && fixture.p.encounterContext->day==(day+3)%100 &&
                fixture.p.encounterContext->year==610+(day+3>=100),"mandatory departure rollover");
            const auto after=fixture.snapshot();Fixture restart(inputs,after);check(XeenSaveFormat::encode(after)==XeenSaveFormat::encode(restart.snapshot()),"post-training restore changed durable state");
        }
        // Synthetic successor checkpoint contains an actual seed-7 supported
        // Armor deletion and physical delivered record. It must never be
        // classified under legacy generated-only semantics during Training.
        {
            auto s=source;s.journey->content=14;s.journey->context->day=9;
            auto &stock=s.journey->serviceEconomy->wares[0][0][1];
            stock=purchase_test::removeOffer(stock,purchase_test::plainOffer(stock,3));
            s.characters[0].armor[4]={0,3,0,0};s.journey->purse->gold=670;
            // Mutation: reject accidental regeneration/depletion loss before training.
            m42_test::StockOracle initial{7,0,{}};const auto generated=initial.generate();
            m42_test::sameCategory(stock,purchase_test::removeOffer(generated.wares[0][0][1],
                purchase_test::plainOffer(generated.wares[0][0][1],3)));
            Fixture fixture(inputs,s);save_test::sameSnapshot(s,fixture.snapshot());
            check(xeenPrepareTraining(fixture.p,18,*fixture.p.encounterContext,XeenMapIdentity(28)).result.outcome==XeenTrainingOutcome::Trained,
                "actual successor Training candidate rejected depleted economy");
            const auto before=*fixture.p.serviceEconomy;const auto cursor=*fixture.w.sessionState().journeyRandom();
            fixture.enter();fixture.train(1);
            check(fixture.p.encounterContext->day==10 && *fixture.p.serviceEconomy==before &&
                *fixture.w.sessionState().journeyRandom()==cursor && fixture.p.purse->gold==580 &&
                xeenSameItem(fixture.p.roster.at(0).armor[4],{0,3,0,0}),"nontrigger successor Training changed depletion/delivery/RNG");
            m42_test::StockOracle oracle{cursor.state,cursor.count,{}};
            auto expected=oracle.generate();expected.bank=before.bank;
            fixture.act(CancelInteractionAction{});fixture.finishDeparture();
            // Mutations: skipped/duplicate restock and an extra/premature RNG publication.
            check(*fixture.w.sessionState().journeyRandom()==XeenJourneyRandomState{1,oracle.state,oracle.count},"Training restock RNG differs");
            check(fixture.flow->canSave() && fixture.p.encounterContext->day==11 && *fixture.p.serviceEconomy==expected &&
                fixture.p.serviceEconomy->bank==before.bank && fixture.p.purse->gold==580 &&
                xeenSameItem(fixture.p.roster.at(0).armor[4],{0,3,0,0}),"successor Training trigger missed restock or changed bought inventory");
            xeenValidateServiceEconomy(*fixture.p.serviceEconomy);
            const auto settled=fixture.snapshot();check(settled.journey->schema==9 && settled.journey->content==14,"Training silently remapped successor content");
            Fixture restarted(inputs,settled);save_test::sameSnapshot(settled,restarted.snapshot());
            restarted.enter();restarted.train(4);restarted.act(CancelInteractionAction{});restarted.finishDeparture();
            check(restarted.p.encounterContext->day==13 && restarted.p.serviceEconomy->wares==settled.journey->serviceEconomy->wares &&
                *restarted.w.sessionState().journeyRandom()==*settled.journey->random,"postrestore successor Training changed nontrigger economy/RNG");
        }
        // Synthetic service addresses/XP/purse isolate the shared successor
        // candidates. Both new-member days and the restock publish through
        // actual Training, followed by actual Smith Buy on that exact stock.
        {
            auto s=source;s.journey->content=14;s.journey->context->day=9;
            auto &stock=s.journey->serviceEconomy->wares[0][0][1];
            stock=purchase_test::removeOffer(stock,purchase_test::plainOffer(stock,3));
            s.characters[0].armor[4]={0,3,0,0};s.journey->purse->gold=670;
            s.journey->serviceEconomy->bank={199,100};
            Fixture training(inputs,s);const auto depleted=*training.p.serviceEconomy;
            const auto cursor=*training.w.sessionState().journeyRandom();
            m42_test::StockOracle oracle{cursor.state,cursor.count,{}};
            auto restocked=oracle.generate();restocked.bank={200,101};
            training.enter();training.train(1);
            check(training.p.encounterContext->day==10 && *training.p.serviceEconomy==depleted &&
                *training.w.sessionState().journeyRandom()==cursor && training.p.purse->gold==580,
                "first successor member day changed depleted stock/RNG");
            training.train(4);
            check(training.p.encounterContext->day==11 && training.p.purse->gold==490 &&
                *training.p.serviceEconomy==restocked &&
                *training.w.sessionState().journeyRandom()==XeenJourneyRandomState{1,oracle.state,oracle.count},
                "second successor member day missed independently expected full restock/interest/RNG");
            training.act(CancelInteractionAction{});training.finishDeparture();
            auto settled=training.snapshot();
            check(settled.journey->context->day==12 && *settled.journey->serviceEconomy==restocked &&
                xeenSameItem(settled.characters[0].armor[4],{0,3,0,0}),
                "separate successor departure repeated restock or lost prior bought armor");
            // A synthetic checkpoint address selects the other admitted service;
            // every Training-published owner and resource value stays exact.
            settled.camera={28,8,4,XeenDirection::West};
            purchase_test::Fixture smith(inputs,settled);save_test::sameSnapshot(settled,smith.snapshot());
            unsigned offerSlot=9;
            for(unsigned slot=0;slot<8;++slot) {
                const auto &item=restocked.wares[0][0][0][slot];
                if(item.id>=1 && item.id<=33 && !item.material && !item.state && !item.frame){offerSlot=slot;break;}
            }
            check(offerSlot<8,"independent post-Training restock lacks a plain Weapon offer");
            const auto offer=restocked.wares[0][0][0][offerSlot];
            constexpr unsigned prices[]={50,15,100,80,40,60,1,10,150,30,60,8,50,100,15,30,15,200,80,250,150,400,100,40,120,300,100,200,300,25,100,50,15};
            const unsigned price=prices[offer.id-1];
            auto expected=settled;XeenItemCategory delivered{},removed{};unsigned recipient=0,retained=0;
            for(const auto &item:settled.characters[0].weapons)if(item.id)delivered[recipient++]=item;
            check(recipient<9,"synthetic post-Training recipient has no physical tail capacity");
            delivered[recipient]=offer;
            for(unsigned slot=0;slot<9;++slot)if(slot!=offerSlot && restocked.wares[0][0][0][slot].id)
                removed[retained++]=restocked.wares[0][0][0][slot];
            expected.characters[0].weapons=delivered;
            expected.journey->serviceEconomy->wares[0][0][0]=removed;
            expected.journey->purse->gold=490-price;expected.journey->context->day=13;
            // Cartography records the Smith return cell: physical tile 28,
            // (8,4) is bit 72, byte 9, mask 1. Keep every earlier cell exact.
            auto &explored=expected.map(28).explored;
            check(explored && !((*explored)[9]&1),
                "Training fixture must retain its tile without the Smith return cell");
            (*explored)[9]|=1;
            smith.enter();smith.quote(XeenInventoryCategory::Weapons,offerSlot);smith.act(YesAction{});
            check(smith.p.purse->gold==490-price &&
                xeenSameItem(smith.p.roster.at(0).weapons[recipient],offer),
                "Buy following actual successor Training/restock changed quote/payment/delivery");
            smith.leave();save_test::sameSnapshot(expected,smith.snapshot());
        }
        // A real synthetic Buy settles before capture. A genuinely fresh owner
        // restoration then repairs against the retained depleted economy.
        {
            auto s=purchase_test::service(inputs);
            // Independently recorded original Arturius body armor is ID3,
            // base200/divisor10 =20 gold (M27/M38 original-data contracts).
            check(xeenSameItem(s.characters[0].armor[0],{0,3,0,3}),"original Arturius repair source differs");
            s.characters[0].armor[0].state=128;
            purchase_test::Fixture original(inputs,s);original.enter();original.purchaseArmor();original.leave();
            const auto checkpoint=original.snapshot();
            purchase_test::Fixture restored(inputs,checkpoint);save_test::sameSnapshot(checkpoint,restored.snapshot());
            auto expected=checkpoint;expected.characters[0].armor[0].state=0;
            expected.journey->purse->gold=650;expected.journey->context->day=10;
            restored.enter();restored.act(DialogKeyAction{'b'});restored.act(DialogKeyAction{'a'});restored.act(DialogKeyAction{'f'});restored.act(SelectInventorySlotAction{0});
            check(XeenPurchaseTestAccess::text(*restored.flow).find("20")!=std::string::npos,
                "fresh-owner depleted Repair quote differs from literal base200/divisor10");
            restored.act(YesAction{});
            check(restored.p.purse->gold==650 && !restored.p.roster.at(0).armor[0].state &&
                *restored.p.serviceEconomy==*checkpoint.journey->serviceEconomy &&
                restored.w.sessionState().journeyRandom()==checkpoint.journey->random,
                "fresh-owner depleted Repair lost exact twenty-gold repair/stock/RNG semantics");
            restored.leave();save_test::sameSnapshot(expected,restored.snapshot());
        }
        // Every fallible publication/settlement hook, with committed prefixes.
        for(unsigned counter=0;counter<5;++counter) {
            Fixture fixture(inputs,source);fixture.enter();
            const unsigned suffix=counter<2?16:counter==2?8:counter==3?3:0;
            XeenTrainingTestAccess::limit(*fixture.flow,counter,UINT64_MAX-suffix);
            if(counter<2)check(XeenTrainingTestAccess::room(*fixture.flow,16,4) && !XeenTrainingTestAccess::room(*fixture.flow,17,4),"authority suffix threshold differs");
            fixture.act(SelectMemberAction{1});
            const auto input=fixture.flow->displayedInput();
            for(unsigned n=0;n<100;++n){fixture.act(SelectMemberAction{0});fixture.act(NavigationAction::TurnLeft);}
            check(input==fixture.flow->displayedInput(),"wrong key drained mandatory suffix");
            fixture.act(CancelInteractionAction{});fixture.finishDeparture();
            check(fixture.flow->canSave() && fixture.p.encounterContext->day==9 && fixture.p.roster.at(18).permanentLevel==3 &&
                fixture.p.purse->gold==800,"finite authority made reserved departure unpayable");
        }
        for(unsigned counter=0;counter<4;++counter)for(bool sufficient:{false,true}) {
            Fixture fixture(inputs,source);fixture.act(InteractionAction{});
            const unsigned needed=counter<2?17:counter==2?9:4;
            XeenTrainingTestAccess::limit(*fixture.flow,counter,UINT64_MAX-needed+(sufficient?0:1));fixture.prepare();
            if(sufficient){check(!fixture.flow->canSave(),"sufficient admission suffix refused");fixture.act(SelectMemberAction{1});fixture.act(CancelInteractionAction{});fixture.finishDeparture();
                check(fixture.flow->canSave() && fixture.p.encounterContext->day==9,"admission threshold lost departure suffix");}
            else {fixture.finishDeparture();check(fixture.flow->canSave() && fixture.p.encounterContext->day==8 && fixture.p.purse->gold==800,"insufficient admission suffix acquired debt");}
        }
        // RNG exhaustion in a replacement leaves the old complete departure;
        // exhaustion in admission has no debt, level, payment or private draws.
        for(unsigned day:{9u,10u}) {
            auto exhausted=source;exhausted.journey->context->day=day;exhausted.journey->random->count=UINT64_MAX-1;
            Fixture fixture(inputs,exhausted);fixture.act(InteractionAction{});if(!fixture.flow->canSave())fixture.prepare();
            if(day==10)check(fixture.flow->canSave() && fixture.p.encounterContext->day==10,"failed mandatory reservation acquired debt");
            else {fixture.train(1);fixture.act(CancelInteractionAction{});fixture.finishDeparture();
                check(fixture.flow->canSave() && fixture.p.encounterContext->day==10 && fixture.p.roster.at(18).permanentLevel==3 &&
                    fixture.p.purse->gold==800 && fixture.w.sessionState().journeyRandom()->count==UINT64_MAX-1,"replacement exhaustion changed prefix/reservation/live RNG");}
        }
        for(auto boundary:{XeenTrainingBoundary::BeforeDeparture,XeenTrainingBoundary::Return,XeenTrainingBoundary::BeforeEventSettlement}) {
            Fixture fixture(inputs,source);fixture.enter();unsigned failures=0;
            fixture.flow->trainingBoundary=[&](auto here){if(here==boundary && failures++<12)throw std::bad_alloc();};
            fixture.act(CancelInteractionAction{});const auto input=fixture.flow->displayedInput();
            while(!fixture.flow->canSave()){fixture.act(CancelInteractionAction{});check(failures<=13,"retry lost mandatory settlement");
                if(!fixture.flow->serviceSaveBlocked()){fixture.finishDeparture();break;}
                if(!fixture.flow->canSave())check(input==fixture.flow->displayedInput(),"identical settlement retry spent semantic input revision");}
            check(fixture.p.encounterContext->day==9,"retry duplicated departure");
        }
        for(auto boundary:{XeenTrainingBoundary::BeforeReservation,XeenTrainingBoundary::StockComplete,XeenTrainingBoundary::BankPrepared,
            XeenTrainingBoundary::BeforeAdmission,XeenTrainingBoundary::AfterAdmission,XeenTrainingBoundary::Quote,
            XeenTrainingBoundary::BeforeLevel,XeenTrainingBoundary::LevelPublished,XeenTrainingBoundary::BeforeDeparture,
            XeenTrainingBoundary::DeparturePublished,XeenTrainingBoundary::Return,
            XeenTrainingBoundary::BeforeEventSettlement,XeenTrainingBoundary::AfterEventSettlement}) {
            std::cout<<"Fault boundary "<<static_cast<unsigned>(boundary)<<'\n';
            auto s=source;s.journey->context->day=9;Fixture fixture(inputs,s);bool fired=false;
            fixture.flow->trainingBoundary=[&](auto here){if(here==boundary && !fired){fired=true;throw std::bad_alloc();}};
            fixture.act(InteractionAction{});
            if(fixture.w.sessionState().journeyActivity()==XeenJourneyActivity::Event)fixture.prepare();
            if(!fixture.flow->serviceSaveBlocked())fixture.finishDeparture();
            if(fixture.flow->canSave())fixture.enter();
            if(fixture.p.roster.at(18).permanentLevel==3)fixture.train(1);
            if(fixture.p.roster.at(18).permanentLevel==3)fixture.train(1);
            fixture.act(CancelInteractionAction{});
            if(!fixture.flow->canSave())fixture.act(CancelInteractionAction{});
            fixture.finishDeparture();
            check(fired && fixture.flow->canSave() && fixture.p.roster.at(18).permanentLevel==4 && fixture.p.purse->gold==710 &&
                fixture.p.roster.combatInputs(18)->experience==6000 && fixture.p.encounterContext->day==11,"hook retry lost/duplicated committed prefix");
        }
        // Reentrant operations consume no second frame and no payment/day.
        for(auto boundary:{XeenTrainingBoundary::AfterAdmission,XeenTrainingBoundary::LevelPublished,XeenTrainingBoundary::DeparturePublished,
            XeenTrainingBoundary::AfterEventSettlement}) {
            Fixture fixture(inputs,source);bool armed=false,failed=false;
            fixture.flow->trainingBoundary=[&](auto here){if(here==boundary && !failed)armed=true;};
            fixture.flow->beforeEncounterFrameCopy=[&]{if(armed && !failed){failed=true;throw std::runtime_error("Synthetic healthy render retry");}};
            fixture.enter();fixture.train(1);fixture.act(CancelInteractionAction{});fixture.finishDeparture();
            check(failed && fixture.flow->canSave() && fixture.p.roster.at(18).permanentLevel==4 && fixture.p.purse->gold==710 &&
                fixture.p.encounterContext->day==10,"render reconstruction duplicated/lost Training publication");
        }
        {
            Fixture fixture(inputs,source);const auto art=fixture.flow->drawTrainingArt;unsigned calls=0;
            fixture.flow->drawTrainingArt=[&](auto &f){if(!calls++)throw std::runtime_error("Synthetic resource provider failure");art(f);};
            fixture.act(InteractionAction{false});fixture.finishDeparture();check(fixture.flow->canSave() && fixture.p.encounterContext->day==8,"unadmitted resource fault created debt");
            fixture.enter();fixture.act(SelectMemberAction{1});fixture.act(CancelInteractionAction{});fixture.finishDeparture();
            check(fixture.flow->canSave() && fixture.p.encounterContext->day==9 && fixture.p.roster.at(0).permanentLevel==3 &&
                fixture.p.purse->gold==800,"cancelled quote set trainee bit/payment/day");
        }
        {
            Fixture fixture(inputs,source);fixture.enter();bool called=false;
            fixture.flow->trainingBoundary=[&](auto boundary){if(boundary==XeenTrainingBoundary::BeforeLevel) {
                called=true;fixture.flow->handle(AcknowledgeAction{},fixture.flow->displayedInput());
                fixture.flow->handle(CancelInteractionAction{},fixture.flow->displayedInput());
                check(!fixture.flow->canSave(),"reentrant F9 exposed capture");
            }};
            fixture.train(1);check(called && fixture.p.purse->gold==710 && fixture.p.encounterContext->day==9,"reentrant payment/day duplicated");
        }
        // Full mutation->reversion guard families, including inactive owners.
        for(unsigned family=0;family<20;++family)for(bool throwing:{false,true}) {
            Fixture fixture(inputs,source);fixture.enter();bool fired=false;
            fixture.flow->trainingBoundary=[&](auto boundary){if(boundary!=XeenTrainingBoundary::BeforeLevel || fired)return;fired=true;
                switch(family) {
                case 0:++fixture.p.roster.at(2).temporaryAge;--fixture.p.roster.at(2).temporaryAge;break;
                case 1:{auto &i=const_cast<XeenCombatInputs &>(*fixture.p.roster.combatInputs(2));++i.experience;--i.experience;break;}
                case 2:fixture.p.roster.at(2).learnedSpells->at(38)^=1;fixture.p.roster.at(2).learnedSpells->at(38)^=1;break;
                case 3:{auto &g=const_cast<XeenTreasureStaging &>(fixture.w.treasureStaging().value()).gold;++g;--g;break;}
                case 4:++fixture.p.serviceEconomy->bank.gold;--fixture.p.serviceEconomy->bank.gold;break;
                case 5:fixture.p.serviceEconomy->wares[1][3][3][8].material^=1;fixture.p.serviceEconomy->wares[1][3][3][8].material^=1;break;
                case 6:++fixture.c.x;--fixture.c.x;break;
                case 7:++fixture.p.encounterContext->minutes;--fixture.p.encounterContext->minutes;break;
                case 8:{auto &map=const_cast<XeenMap &>(fixture.w.map(28));map.geometry.cells[0].rawAttributes^=1;map.geometry.cells[0].rawAttributes^=1;break;}
                case 9:{auto &actor=const_cast<XeenActor &>(fixture.w.sessionState().regionalActors(28)[34]);++actor.hp;--actor.hp;break;}
                case 10:{auto &actor=const_cast<XeenActor &>(fixture.w.sessionState().monsters(23)[0]);++actor.hp;--actor.hp;break;}
                case 11:{auto &rng=const_cast<XeenMutableOptional<XeenJourneyRandomState> &>(fixture.w.sessionState().journeyRandom());++rng->count;--rng->count;break;}
                case 12:fixture.p.roster.at(2).armor[8].frame^=1;fixture.p.roster.at(2).armor[8].frame^=1;break;
                case 13:{const auto before=fixture.p.serviceEconomy;fixture.p.serviceEconomy.reset();fixture.p.serviceEconomy=before;break;}
                case 14:{auto &i=const_cast<XeenMutableOptional<XeenCombatInputs> &>(fixture.p.roster.combatInputs(2));const auto before=i;i.reset();i=before;++i->experience;--i->experience;break;}
                case 15:{fixture.w.discardMapCache();auto &map=const_cast<XeenMap &>(fixture.w.map(28));map.geometry.cells[0].rawAttributes^=1;map.geometry.cells[0].rawAttributes^=1;break;}
                case 16:{fixture.w.discardMapCache();auto &obj=const_cast<XeenObjectFile &>(fixture.w.objectFile(28));obj.entities.objects[0].x^=1;obj.entities.objects[0].x^=1;break;}
                case 17:fixture.p.roster.at(2).temporaryAge=fixture.p.roster.at(2).temporaryAge;break;
                case 18:{const bool before=fixture.f.isSet(0);fixture.f.set(0);fixture.f.clear(0);if(before)fixture.f.set(0);break;}
                case 19:fixture.p.party=fixture.p.party;break;
                }
                if(throwing)throw std::runtime_error("ABA exceptional exit");
            };
            rejects([&]{fixture.train(1);});check(fired && !fixture.flow->canSave() && fixture.p.roster.at(18).permanentLevel==3 &&
                fixture.p.purse->gold==800 && fixture.p.encounterContext->day==8,"ABA failure published/reopened Quiet");
        }
        std::cout<<"Training synthetic reset/day/repeat/fault/reentrant/ABA fixtures passed\n";return 0;
    } catch(const std::exception &e){std::cerr<<e.what()<<'\n';return 1;}
}
