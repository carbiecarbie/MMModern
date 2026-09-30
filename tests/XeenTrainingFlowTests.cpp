#include "XeenTrainingTestSupport.h"
#include <iostream>
using namespace training_test;
using save_test::rejects;
int main(int argc,char **argv) {
    try {
        check(argc==2,"usage: training-flow <original-installation>");const auto installation=XeenInstallationDetector().detect(argv[1]);
        check(bool(installation),"installation unavailable");Inputs inputs(*installation);const auto source=inputs.service();
        constexpr std::array<std::uint32_t,10> bases{1500,2000,2000,1500,2000,1000,1500,1500,1500,2000};
        for(unsigned cls=0;cls<10;++cls)for(unsigned level:{1u,3u,4u,9u}) {
            auto s=source;s.characters[18].characterClass=static_cast<XeenCharacterClass>(cls);s.characters[18].permanentLevel=level;
            s.characters[18].weapons={};s.characters[18].armor={};s.characters[18].accessories={};s.characters[18].miscellaneous={};
            const unsigned debit=bases[cls]*(1u<<(level==1?0:level-2));
            s.journey->supplements[18].inputs.experience=debit+1;s.journey->treasure->gold=10*level*level;
            Fixture fixture(inputs,s);const auto delta=xeenPrepareTraining(fixture.p,18,*fixture.p.encounterContext);
            check(delta.result.outcome==XeenTrainingOutcome::Trained && delta.result.levelAfter==level+1 &&
                delta.result.xpAfter==1 && delta.result.goldAfter==0 && delta.result.maxHpAfter==delta.result.hpAfter &&
                delta.result.maxSpAfter==delta.result.spAfter,"class/level progression debit/remainder/exact funds/derived refill differs");
        }
        // Independent reset matrix with nonzero selected, other active and
        // inactive fields, raw books/items, age and conditions preserved.
        auto matrix=source;matrix.journey->treasure->gold=90;matrix.characters[18].currentHp=100;
        for(unsigned id:{18u,0u,2u}) {
            auto &c=matrix.characters[id];auto &i=matrix.journey->supplements[id].inputs;
            c.temporaryLevel=2;c.intellect.temporary=3;c.personality.temporary=4;c.endurance.temporary=5;c.temporaryAge=6;
            i.might.temporary=7;i.speed.temporary=8;i.accuracy.temporary=9;i.temporaryAc=10;
            i.luck->temporary=11;i.resistances->coldTemporary=12;i.resistances->electricalTemporary=13;i.poisonResistance->temporary=14;
            c.learnedSpells->at(38)=255;
            c.armor[8]={255,0,255,0};
        }
        matrix.characters[0].conditions[8]=1;
        matrix.journey->supplements[18].inputs.experience=3000;
        {
            Fixture fixture(inputs,matrix);const auto inactive=fixture.p.roster.at(2);const auto inactiveInputs=*fixture.p.roster.combatInputs(2);
            fixture.enter();fixture.train(1);
            check(fixture.p.roster.at(18).permanentLevel==4 && fixture.p.roster.at(18).currentHp==64 && fixture.p.roster.at(18).currentSp==0 &&
                fixture.p.roster.combatInputs(18)->experience==0 && fixture.p.monsterTreasure->gold==0 && fixture.p.encounterContext->day==9,"exact funds/reset/refill/progression mismatch");
            for(auto id:kXeenCombatOwners) {
                const auto &c=fixture.p.roster.at(id);const auto &i=*fixture.p.roster.combatInputs(id);
                check(!c.temporaryLevel && !c.intellect.temporary && !c.personality.temporary && !c.endurance.temporary && !i.might.temporary &&
                    !i.speed.temporary && !i.accuracy.temporary && !i.temporaryAc && !i.luck->temporary && !i.resistances->coldTemporary &&
                    !i.resistances->electricalTemporary && !i.poisonResistance->temporary,"active reset matrix mismatch");
                check(c.temporaryAge==matrix.characters[id].temporaryAge && c.conditions==matrix.characters[id].conditions &&
                    c.learnedSpells==matrix.characters[id].learnedSpells,"reset changed age/conditions/book");
                check(xeen_state::sameItemCategory(c.weapons,matrix.characters[id].weapons) &&
                    xeen_state::sameItemCategory(c.armor,matrix.characters[id].armor) && xeen_state::sameItemCategory(c.accessories,matrix.characters[id].accessories) &&
                    xeen_state::sameItemCategory(c.miscellaneous,matrix.characters[id].miscellaneous),"Training changed an equipment byte");
                if(id!=18)check(c.currentHp==matrix.characters[id].currentHp && c.currentSp==matrix.characters[id].currentSp,"reset clamped/refilled nonselected member");
            }
            check(xeen_state::sameCharacter(inactive,fixture.p.roster.at(2)) && xeen_state::sameInputs(inactiveInputs,*fixture.p.roster.combatInputs(2)),"inactive owner reset");
            fixture.act(CancelInteractionAction{});check(fixture.flow->canSave() && fixture.p.encounterContext->day==10,"separate departure failed");
        }
        for(unsigned day:{8u,9u,97u,98u,99u}) {
            std::cout<<"Day fixture "<<day<<'\n';
            auto s=source;s.journey->context->day=day;Fixture fixture(inputs,s);
            if(day==99){fixture.act(InteractionAction{});check(fixture.flow->canSave() && fixture.p.encounterContext->day==99,"day99 acquired debt");continue;}
            fixture.enter();check(!fixture.flow->canSave() && !XeenSaveState::canCapture(fixture.p,fixture.c,fixture.w),"admitted visit save permitted");
            const auto before=fixture.p.monsterTreasure->gold;
            fixture.train(1);
            if(day==98) {
                check(fixture.p.roster.at(18).permanentLevel==3 && fixture.p.encounterContext->day==98 && fixture.p.monsterTreasure->gold==before,"day98 trained new member");
            } else {
                check(fixture.p.roster.at(18).permanentLevel==4 && fixture.p.encounterContext->day==day+1,"first member day missing");
                fixture.act(SelectMemberAction{0});fixture.train(1);
                check(fixture.p.roster.at(18).permanentLevel==5 && fixture.p.roster.combatInputs(18)->experience==0 &&
                    fixture.p.encounterContext->day==day+1,"same-member repeat/switch charged day or XP incorrectly");
                fixture.train(4);
                check(fixture.p.encounterContext->day==(day==97?98:day+2) && fixture.p.roster.at(1).permanentLevel==(day==97?3:4),"new-member replacement capacity incorrect");
            }
            fixture.act(CancelInteractionAction{});check(fixture.flow->canSave() && fixture.p.encounterContext->day==(day>=97?99:day+3),"mandatory departure charged wrong day");
            const auto after=fixture.snapshot();Fixture restart(inputs,after);check(XeenSaveFormat::encode(after)==XeenSaveFormat::encode(restart.snapshot()),"post-training restore changed durable state");
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
            fixture.act(AcknowledgeAction{});
            check(fixture.flow->canSave() && fixture.p.encounterContext->day==9 && fixture.p.roster.at(18).permanentLevel==3 &&
                fixture.p.monsterTreasure->gold==800,"finite authority made reserved departure unpayable");
        }
        for(unsigned counter=0;counter<4;++counter)for(bool sufficient:{false,true}) {
            Fixture fixture(inputs,source);fixture.act(InteractionAction{});
            const unsigned needed=counter<2?17:counter==2?9:4;
            XeenTrainingTestAccess::limit(*fixture.flow,counter,UINT64_MAX-needed+(sufficient?0:1));fixture.prepare();
            if(sufficient){check(!fixture.flow->canSave(),"sufficient admission suffix refused");fixture.act(SelectMemberAction{1});fixture.act(AcknowledgeAction{});
                check(fixture.flow->canSave() && fixture.p.encounterContext->day==9,"admission threshold lost departure suffix");}
            else check(fixture.flow->canSave() && fixture.p.encounterContext->day==8 && fixture.p.monsterTreasure->gold==800,"insufficient admission suffix acquired debt");
        }
        // RNG exhaustion in a replacement leaves the old complete departure;
        // exhaustion in admission has no debt, level, payment or private draws.
        for(unsigned day:{9u,10u}) {
            auto exhausted=source;exhausted.journey->context->day=day;exhausted.journey->random->count=UINT64_MAX-1;
            Fixture fixture(inputs,exhausted);fixture.act(InteractionAction{});fixture.prepare();
            if(day==10)check(fixture.flow->canSave() && fixture.p.encounterContext->day==10,"failed mandatory reservation acquired debt");
            else {fixture.train(1);fixture.act(AcknowledgeAction{});fixture.act(CancelInteractionAction{});
                check(fixture.flow->canSave() && fixture.p.encounterContext->day==10 && fixture.p.roster.at(18).permanentLevel==3 &&
                    fixture.p.monsterTreasure->gold==800 && fixture.w.sessionState().journeyRandom()->count==UINT64_MAX-1,"replacement exhaustion changed prefix/reservation/live RNG");}
        }
        for(auto boundary:{XeenTrainingBoundary::BeforeDeparture,XeenTrainingBoundary::Return,XeenTrainingBoundary::BeforeEventSettlement}) {
            Fixture fixture(inputs,source);fixture.enter();unsigned failures=0;
            fixture.flow->trainingBoundary=[&](auto here){if(here==boundary && failures++<12)throw std::bad_alloc();};
            fixture.act(CancelInteractionAction{});const auto input=fixture.flow->displayedInput();
            while(!fixture.flow->canSave()){fixture.act(AcknowledgeAction{});check(failures<=13,"retry lost mandatory settlement");
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
            if(fixture.flow->canSave())fixture.enter();
            if(fixture.p.roster.at(18).permanentLevel==3)fixture.train(1);
            if(fixture.p.roster.at(18).permanentLevel==3)fixture.train(1);
            fixture.act(CancelInteractionAction{});
            if(!fixture.flow->canSave())fixture.act(AcknowledgeAction{});
            check(fired && fixture.flow->canSave() && fixture.p.roster.at(18).permanentLevel==4 && fixture.p.monsterTreasure->gold==710 &&
                fixture.p.roster.combatInputs(18)->experience==6000 && fixture.p.encounterContext->day==11,"hook retry lost/duplicated committed prefix");
        }
        // Reentrant operations consume no second frame and no payment/day.
        for(auto boundary:{XeenTrainingBoundary::AfterAdmission,XeenTrainingBoundary::LevelPublished,XeenTrainingBoundary::DeparturePublished,
            XeenTrainingBoundary::AfterEventSettlement}) {
            Fixture fixture(inputs,source);bool armed=false,failed=false;
            fixture.flow->trainingBoundary=[&](auto here){if(here==boundary && !failed)armed=true;};
            fixture.flow->beforeEncounterFrameCopy=[&]{if(armed && !failed){failed=true;throw std::runtime_error("Synthetic healthy render retry");}};
            fixture.enter();fixture.train(1);fixture.act(CancelInteractionAction{});
            check(failed && fixture.flow->canSave() && fixture.p.roster.at(18).permanentLevel==4 && fixture.p.monsterTreasure->gold==710 &&
                fixture.p.encounterContext->day==10,"render reconstruction duplicated/lost Training publication");
        }
        {
            Fixture fixture(inputs,source);const auto art=fixture.flow->drawTrainingArt;unsigned calls=0;
            fixture.flow->drawTrainingArt=[&](auto &f){if(!calls++)throw std::runtime_error("Synthetic resource provider failure");art(f);};
            fixture.act(InteractionAction{});check(fixture.flow->canSave() && fixture.p.encounterContext->day==8,"unadmitted resource fault created debt");
            fixture.enter();fixture.act(SelectMemberAction{1});fixture.act(AcknowledgeAction{});fixture.act(CancelInteractionAction{});fixture.act(CancelInteractionAction{});
            check(fixture.flow->canSave() && fixture.p.encounterContext->day==9 && fixture.p.roster.at(0).permanentLevel==3 &&
                fixture.p.monsterTreasure->gold==800,"cancelled quote set trainee bit/payment/day");
        }
        {
            Fixture fixture(inputs,source);fixture.enter();bool called=false;
            fixture.flow->trainingBoundary=[&](auto boundary){if(boundary==XeenTrainingBoundary::BeforeLevel) {
                called=true;fixture.flow->handle(AcknowledgeAction{},fixture.flow->displayedInput());
                fixture.flow->handle(CancelInteractionAction{},fixture.flow->displayedInput());
                check(!fixture.flow->canSave(),"reentrant F9 exposed capture");
            }};
            fixture.train(1);check(called && fixture.p.monsterTreasure->gold==710 && fixture.p.encounterContext->day==9,"reentrant payment/day duplicated");
        }
        // Full mutation->reversion guard families, including inactive owners.
        for(unsigned family=0;family<20;++family)for(bool throwing:{false,true}) {
            Fixture fixture(inputs,source);fixture.enter();bool fired=false;
            fixture.flow->trainingBoundary=[&](auto boundary){if(boundary!=XeenTrainingBoundary::BeforeLevel || fired)return;fired=true;
                switch(family) {
                case 0:++fixture.p.roster.at(2).temporaryAge;--fixture.p.roster.at(2).temporaryAge;break;
                case 1:{auto &i=const_cast<XeenCombatInputs &>(*fixture.p.roster.combatInputs(2));++i.experience;--i.experience;break;}
                case 2:fixture.p.roster.at(2).learnedSpells->at(38)^=1;fixture.p.roster.at(2).learnedSpells->at(38)^=1;break;
                case 3:++fixture.p.monsterTreasure->pendingGold;--fixture.p.monsterTreasure->pendingGold;break;
                case 4:++fixture.p.serviceEconomy->bank.gold;--fixture.p.serviceEconomy->bank.gold;break;
                case 5:fixture.p.serviceEconomy->wares[1][3][3][8].material^=1;fixture.p.serviceEconomy->wares[1][3][3][8].material^=1;break;
                case 6:++fixture.c.x;--fixture.c.x;break;
                case 7:++fixture.p.encounterContext->minutes;--fixture.p.encounterContext->minutes;break;
                case 8:{auto &map=const_cast<XeenMap &>(fixture.w.map(28));map.geometry.cells[0].rawAttributes^=1;map.geometry.cells[0].rawAttributes^=1;break;}
                case 9:{auto &actor=const_cast<XeenActor &>(fixture.w.sessionState().regionalActors(28)[34]);++actor.hp;--actor.hp;break;}
                case 10:{auto &actor=const_cast<XeenActor &>(fixture.w.sessionState().actors()[0]);++actor.hp;--actor.hp;break;}
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
                fixture.p.monsterTreasure->gold==800 && fixture.p.encounterContext->day==8,"ABA failure published/reopened Quiet");
        }
        std::cout<<"Training synthetic reset/day/repeat/fault/reentrant/ABA fixtures passed\n";return 0;
    } catch(const std::exception &e){std::cerr<<e.what()<<'\n';return 1;}
}
