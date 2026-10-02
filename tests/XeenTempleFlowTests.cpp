#include "XeenTrainingTestSupport.h"
#include "games/xeen/XeenTempleHeal.h"
#include <iostream>
using namespace training_test;
using save_test::sameSnapshot;
namespace {
XeenSaveSnapshot injured(Inputs &in,unsigned day=8) {
    auto s=in.service(day);
    s.journey->contract=14;
    s.camera={28,15,28,XeenDirection::North};
    s.journey->treasure->gold=810;
    auto &seymour=s.characters[6];
    seymour.currentHp=-15;seymour.conditions[12]=1;seymour.conditions[13]=1;
    auto &rebecca=s.characters[1];
    rebecca.currentHp=0;rebecca.conditions[12]=1;
    return s;
}
struct TempleFixture:Fixture {
    TempleFixture(Inputs &in,const XeenSaveSnapshot &s):Fixture(in,s) {
        flow->drawTempleArt=[&in](auto &image){in.assets.drawTemple(image);};
    }
    void enter() {
        act(InteractionAction{});
        if(!flow->canSave())prepare();
        check(!flow->canSave(),"Temple entry did not retain service debt");
    }
    void heal(unsigned member) {
        act(SelectMemberAction{member});act(AcknowledgeAction{});act(AcknowledgeAction{});
        prepare();act(AcknowledgeAction{});
    }
    void leave(){act(CancelInteractionAction{});check(flow->canSave(),"Temple departure did not return Quiet");}
};
void departureRetries(Inputs &in) {
    for(bool paid:{false,true}) {
        TempleFixture reference(in,injured(in));reference.enter();
        if(paid)reference.heal(5);
        reference.leave();const auto expected=reference.snapshot();
        for(auto boundary:{XeenSmithBoundary::BeforeDeparture,XeenSmithBoundary::AfterDeparture,
                XeenSmithBoundary::Return,XeenSmithBoundary::BeforeEventSettlement,
                XeenSmithBoundary::AfterEventSettlement})for(unsigned room:{8u,9u,10u}) {
            TempleFixture retry(in,injured(in));retry.enter();if(paid)retry.heal(5);
            XeenTrainingTestAccess::limit(*retry.flow,2,UINT64_MAX-room);
            unsigned faults=0;
            retry.flow->smithBoundary=[&](auto here){if(here==boundary){++faults;
                throw std::runtime_error("persistent Temple departure fault");}};
            retry.act(CancelInteractionAction{});
            check(faults==1 && !retry.flow->canSave(),"Temple departure fault missed its boundary");
            const auto revision=XeenTrainingTestAccess::templeRevision(*retry.flow);
            const auto input=retry.flow->displayedInput();
            for(unsigned attempt=0;attempt<32;++attempt) {
                retry.act(AcknowledgeAction{});
                check(revision==XeenTrainingTestAccess::templeRevision(*retry.flow) &&
                    input==retry.flow->displayedInput() && !retry.flow->canSave(),
                    "unchanged Temple departure retry consumed finite UI/input authority");
            }
            check(faults==33,"Temple departure retries stopped reaching the reserved boundary");
            retry.flow->smithBoundary={};retry.act(AcknowledgeAction{});
            check(retry.flow->canSave(),"finite authority stranded mandatory Temple departure");
            sameSnapshot(expected,retry.snapshot());
        }
    }
}
void revisionLimits(Inputs &in) {
    for(bool alreadyPaid:{false,true})for(bool afterFault:{false,true})for(unsigned room:{1u,2u,3u}) {
        TempleFixture fixture(in,injured(in));fixture.enter();if(alreadyPaid)fixture.heal(5);
        fixture.act(SelectMemberAction{alreadyPaid?4u:5u});fixture.act(AcknowledgeAction{});
        const auto gold=fixture.p.monsterTreasure->gold;
        const auto owner=alreadyPaid?1u:6u;
        const auto before=fixture.p.roster.at(owner);
        XeenTrainingTestAccess::templeRevision(*fixture.flow,UINT64_MAX-room);
        unsigned faults=0;
        fixture.flow->smithBoundary=[&](auto here){if(afterFault && here==XeenSmithBoundary::AfterHeal && ++faults==1)
            throw std::runtime_error("post-publication Temple revision fault");};
        fixture.act(AcknowledgeAction{});
        if(room==1) {
            check(fixture.p.monsterTreasure->gold==gold &&
                xeen_state::sameCharacter(before,fixture.p.roster.at(owner)) &&
                !XeenTrainingTestAccess::templePending(*fixture.flow),
                "Temple confirmation paid without room for Upgrade and Result revisions");
        } else {
            check(XeenTrainingTestAccess::templeRevision(*fixture.flow)==UINT64_MAX-room+1,
                "Temple confirmation did not reserve the first UI revision");
            fixture.prepare();
            check(fixture.p.monsterTreasure->gold==gold-(alreadyPaid?60u:410u) &&
                XeenTrainingTestAccess::templeRevision(*fixture.flow)==UINT64_MAX-room+2,
                "Temple paid Result revision wrapped or debit differed");
            fixture.act(AcknowledgeAction{});
        }
        fixture.flow->smithBoundary={};
        if(XeenTrainingTestAccess::templeLobby(*fixture.flow))fixture.leave();
        else fixture.act(AcknowledgeAction{});
        check(fixture.flow->canSave() && fixture.p.encounterContext->day==(room==1 && !alreadyPaid?9:10),
            "Temple UI revision exhaustion lost mandatory departure");
    }
    // Independently exercise the final check while a detached Heal is pending.
    TempleFixture pending(in,injured(in));pending.enter();pending.act(SelectMemberAction{5});
    pending.act(AcknowledgeAction{});pending.act(AcknowledgeAction{});
    XeenTrainingTestAccess::templeRevision(*pending.flow,UINT64_MAX);
    pending.flow->beginCycle(++pending.cycle);
    check(!pending.flow->updatePresentation() && pending.p.monsterTreasure->gold==810 &&
        pending.p.roster.at(6).currentHp==-15 && XeenTrainingTestAccess::templePending(*pending.flow),
        "Temple publication ignored exhausted Result UI revision");
    pending.act(CancelInteractionAction{});pending.leave();
    check(pending.p.encounterContext->day==9 && pending.p.monsterTreasure->gold==810,
        "Temple publication capacity failure lost unpaid exit reservation");
}
void quotePreview(Inputs &in) {
    for(unsigned scenario=0;scenario<4;++scenario) {
        auto source=injured(in);auto &c=source.characters[6];
        c.conditions.fill(0);c.currentHp=1;c.endurance.permanent=13;
        c.endurance.temporary=20;c.temporaryLevel=1;c.conditions[4]=1;
        if(scenario==1)c.currentHp=100;
        if(scenario==2){c.currentHp=-15;c.conditions[12]=c.conditions[13]=1;}
        if(scenario==3){c.conditions[3]=2;c.conditions[8]=1;}
        TempleFixture fixture(in,source);
        const auto expected=xeenPrepareTempleHeal(fixture.p,6,*fixture.p.encounterContext,14);
        check(expected.result.outcome==XeenTempleHealOutcome::Healed,"Temple preview fixture is not payable");
        if(!scenario)check(expected.result.hpAfter==6 && expected.result.maxHpAfter==9,
            "Disease temporary-bonus preview fixture differs from independent HP 6/9");
        fixture.enter();fixture.act(SelectMemberAction{5});fixture.act(AcknowledgeAction{});
        const auto hp="HP "+std::to_string(expected.result.hpBefore)+" -> "+
            std::to_string(expected.result.hpAfter)+" (healthy max "+std::to_string(expected.result.maxHpAfter)+")";
        check(XeenTrainingTestAccess::templeText(*fixture.flow).find(hp)!=std::string::npos,
            "Temple quote HP preview differs from reset/assignment/condition-clear Heal result");
        fixture.act(AcknowledgeAction{});fixture.prepare();
        const auto resultText=XeenTrainingTestAccess::templeText(*fixture.flow);
        check(resultText.find(hp)!=std::string::npos &&
            resultText.find("SP "+std::to_string(expected.result.spBefore)+" -> "+
                std::to_string(expected.result.spAfter))!=std::string::npos &&
            resultText.find("Healed. Paid "+std::to_string(expected.result.price)+" gold.")!=std::string::npos &&
            resultText.find("Gold now "+std::to_string(expected.result.goldAfter))!=std::string::npos &&
            fixture.p.roster.at(6).currentHp==expected.result.hpAfter &&
            XeenCharacterRules::maxHp(fixture.p.roster.at(6),{610})==expected.result.maxHpAfter,
            "Temple displayed preview differs from published selected Heal");
        fixture.act(AcknowledgeAction{});fixture.leave();
    }
}
void refusedResult(Inputs &in,bool exhaustedRandom) {
    auto source=injured(in,exhaustedRandom?8:98);
    if(exhaustedRandom)source.journey->random->count=UINT64_MAX;
    TempleFixture fixture(in,source);
    const auto before=fixture.p.roster.at(6);
    const auto inputs=*fixture.p.roster.combatInputs(6);
    const auto gold=fixture.p.monsterTreasure->gold;
    const auto maximum=XeenCharacterRules::maxHp(before,{610});
    fixture.enter();fixture.act(SelectMemberAction{5});fixture.act(AcknowledgeAction{});
    check(XeenTrainingTestAccess::templeText(*fixture.flow).find("HP -15 -> 15 (healthy max 15)")!=std::string::npos,
        "Temple refusal fixture lost its projected quote");
    fixture.act(AcknowledgeAction{});
    if(exhaustedRandom)fixture.prepare();
    const auto text=XeenTrainingTestAccess::templeText(*fixture.flow);
    check(xeen_state::sameCharacter(before,fixture.p.roster.at(6)) &&
        xeen_state::sameInputs(inputs,*fixture.p.roster.combatInputs(6)) &&
        fixture.p.monsterTreasure->gold==gold &&
        fixture.w.sessionState().journeyRandom()==source.journey->random &&
        *fixture.p.encounterContext==*source.journey->context,
        "Temple refusal changed live owners before departure");
    check(text.find("Paid departure outside supported date or capacity.")!=std::string::npos &&
        text.find("Enter: return to Temple menu")!=std::string::npos,
        "Temple refusal did not reach the support-limit Result");
    check(text.find("HP -15 -> -15 (max "+std::to_string(maximum)+")")!=std::string::npos &&
        text.find("SP "+std::to_string(before.currentSp)+" -> "+std::to_string(before.currentSp)+
            "  Gold "+std::to_string(gold))!=std::string::npos &&
        text.find("Gold now "+std::to_string(gold)+"  Exit 1 day(s)")!=std::string::npos &&
        text.find("Healed")==std::string::npos && text.find("Paid 410")==std::string::npos &&
        text.find("Heal price")==std::string::npos,
        exhaustedRandom?"RNG-refused Result displays projected Heal values":"Day-98 refused Result displays projected Heal values");
    fixture.act(AcknowledgeAction{});fixture.leave();
    check(xeen_state::sameCharacter(before,fixture.p.roster.at(6)) &&
        fixture.p.monsterTreasure->gold==gold &&
        fixture.p.encounterContext->day==(exhaustedRandom?9:99),
        "Temple refusal lost the unchanged recipient/purse or mandatory one-day exit");
}
}
int main(int argc,char **argv) {
    try {
        check(argc==2 || argc==3,"usage: temple-flow <original-installation> [retries|revisions|preview|refused-date|refused-rng]");
        const auto installation=XeenInstallationDetector().detect(argv[1]);
        check(bool(installation),"original installation unavailable");Inputs in(*installation);
        if(argc==3) {
            const std::string regression=argv[2];
            if(regression=="retries")departureRetries(in);
            else if(regression=="revisions")revisionLimits(in);
            else if(regression=="preview")quotePreview(in);
            else if(regression=="refused-date")refusedResult(in,false);
            else if(regression=="refused-rng")refusedResult(in,true);
            else throw std::invalid_argument("Unknown Temple regression");
            return 0;
        }
        departureRetries(in);revisionLimits(in);quotePreview(in);
        refusedResult(in,false);refusedResult(in,true);
        auto source=injured(in);
        TempleFixture f(in,source);
        const auto quote=xeenQuoteTempleHeal(f.p.roster.at(6),f.p.monsterTreasure->gold,*f.p.encounterContext);
        check(quote.outcome==XeenTempleHealOutcome::Quoted && quote.price==410,"Dead/Unconscious quote differs");
        const auto selected=xeenPrepareTempleHeal(f.p,6,*f.p.encounterContext,14);
        check(selected.result.outcome==XeenTempleHealOutcome::Healed && selected.result.hpAfter==15 &&
            selected.result.spAfter==27 && selected.result.goldAfter==400 &&
            !selected.character.conditions[12] && !selected.character.conditions[13],
            "selected resurrection result differs");
        f.enter();f.heal(5);
        check(f.p.roster.at(6).currentHp==15 && f.p.roster.at(6).currentSp==27 &&
            !f.p.roster.at(6).conditions[13] && f.p.monsterTreasure->gold==400 &&
            f.p.encounterContext->day==8,"first Heal mutated wrong owners or charged time early");
        f.heal(4);
        check(f.p.roster.at(1).currentHp==21 && f.p.roster.at(1).currentSp==21 &&
            !f.p.roster.at(1).conditions[12] && f.p.monsterTreasure->gold==340,
            "second Heal mutated wrong owner or price");
        check(xeenQuoteTempleHeal(f.p.roster.at(6),f.p.monsterTreasure->gold,*f.p.encounterContext).outcome==
            XeenTempleHealOutcome::NoCharge,"healthy recipient gained an extra paid quote");
        f.leave();
        check(f.p.encounterContext->day==10 && f.p.monsterTreasure->gold==340,
            "paid Temple visit did not make one two-day departure");
        const auto saved=f.snapshot();TempleFixture restored(in,saved);sameSnapshot(saved,restored.snapshot());
        {
            auto noPay=injured(in);TempleFixture refused(in,noPay);
            refused.enter();refused.leave();
            check(refused.p.encounterContext->day==9 && refused.p.monsterTreasure->gold==810,
                "unpaid Temple visit did not make one one-day departure");
        }
		{
			TempleFixture reference(in,injured(in));reference.enter();reference.leave();
			const auto unpaid=reference.snapshot();
			for(bool overflow:{false,true}) {
				TempleFixture retry(in,injured(in));retry.enter();unsigned faults=0;
				retry.flow->smithBoundary=[&](auto here) {
					if(here==(overflow?XeenSmithBoundary::StockComplete:XeenSmithBoundary::BeforeHeal)) {
						++faults;
						if(overflow)throw std::overflow_error("synthetic boundary overflow");
						throw std::runtime_error("persistent synthetic Heal fault");
					}
				};
				retry.act(SelectMemberAction{5});retry.act(AcknowledgeAction{});
				retry.act(AcknowledgeAction{});
				check(XeenTrainingTestAccess::templeText(*retry.flow).find("Escape: cancel Heal")!=std::string::npos,
					"Temple Upgrade did not expose its cancellation control");
				for(unsigned slice=0;slice<1000 && faults<2;++slice) {
					retry.flow->beginCycle(++retry.cycle);
					const auto next=retry.flow->updatePresentation();
					if(next)retry.present(*next);
				}
				check(faults>=2 && XeenTrainingTestAccess::templePending(*retry.flow) &&
					XeenTrainingTestAccess::templeOneDayReserved(*retry.flow) &&
					retry.p.monsterTreasure->gold==810 && !retry.flow->canSave(),
					"persistent upgrade fault lost the unpaid reservation or published Heal");
				retry.act(CancelInteractionAction{});
				check(XeenTrainingTestAccess::templeLobby(*retry.flow) &&
					!XeenTrainingTestAccess::templePending(*retry.flow) &&
					XeenTrainingTestAccess::templeOneDayReserved(*retry.flow) &&
					!retry.flow->canSave(),
					"Escape did not cancel unpublished Heal and retain one-day exit");
				retry.leave();sameSnapshot(unpaid,retry.snapshot());
			}
		}
        {
            auto near=injured(in,98);TempleFixture refused(in,near);
            refused.enter();refused.act(SelectMemberAction{5});refused.act(AcknowledgeAction{});
            refused.act(AcknowledgeAction{});refused.act(AcknowledgeAction{});refused.leave();
            check(refused.p.encounterContext->day==99 && refused.p.monsterTreasure->gold==810 &&
                refused.p.roster.at(6).conditions[13],"day-98 paid Heal escaped date refusal");
        }
        {
            auto diseased=injured(in);auto &character=diseased.characters[6];
            character.conditions[12]=character.conditions[13]=0;
            character.currentHp=1;
            bool threshold=false;
            for(int endurance=1;endurance<=30 && !threshold;++endurance)
                for(unsigned severity=1;severity<=20 && !threshold;++severity) {
                    auto trial=character;
                    trial.endurance.permanent=endurance;trial.endurance.temporary=0;
                    trial.conditions[4]=static_cast<std::uint8_t>(severity);
                    auto healthy=trial;healthy.conditions[4]=0;
                    if(XeenCharacterRules::maxHp(trial,{610})<XeenCharacterRules::maxHp(healthy,{610})) {
                        character=trial;threshold=true;
                    }
                }
            check(threshold,"Disease Endurance threshold fixture unavailable");
            TempleFixture fixture(in,diseased);
            const auto before=fixture.snapshot();
            const auto candidate=xeenPrepareTempleHeal(fixture.p,6,*fixture.p.encounterContext,14);
            check(candidate.result.outcome==XeenTempleHealOutcome::Healed &&
                candidate.result.maxHpAtAssignment<candidate.result.maxHpAfter &&
                candidate.character.currentHp==candidate.result.maxHpAtAssignment &&
                candidate.character.currentSp==before.characters[6].currentSp,
                "Disease cure assigned HP after clearing the original condition or changed SP");
            for(unsigned owner=0;owner<30;++owner)if(owner!=6)
                check(xeen_state::sameCharacter(before.characters[owner],fixture.p.roster.at(owner)) &&
                    xeen_state::sameInputs(before.journey->supplements[owner].inputs,
                        *fixture.p.roster.combatInputs(owner)),
                    "detached Heal changed another owner");
            fixture.enter();fixture.heal(5);fixture.leave();
            check(fixture.p.roster.at(6).currentHp==candidate.result.maxHpAtAssignment &&
                XeenCharacterRules::maxHp(fixture.p.roster.at(6),{610})==candidate.result.maxHpAfter &&
                fixture.p.roster.at(6).currentSp==before.characters[6].currentSp,
                "published Disease Heal changed the required HP order or SP");
        }
        {
            auto healthy=injured(in);auto &character=healthy.characters[6];
            character.conditions[12]=character.conditions[13]=0;
            character.currentHp=XeenCharacterRules::maxHp(character,{610});
            character.currentSp=0;character.endurance.temporary=1;
            TempleFixture fixture(in,healthy);
            const auto result=xeenPrepareTempleHeal(fixture.p,6,*fixture.p.encounterContext,14);
            check(result.result.outcome==XeenTempleHealOutcome::NoCharge &&
                xeen_state::sameCharacter(result.character,fixture.p.roster.at(6)) &&
                fixture.p.roster.at(6).endurance.temporary==1,
                "zero-price low-SP Heal reset temporary state");
        }
        {
            auto recovered=injured(in);TempleFixture reference(in,recovered);
            reference.enter();reference.heal(5);reference.leave();
            const auto expected=reference.snapshot();
            for(auto boundary:{XeenSmithBoundary::BeforeHeal,XeenSmithBoundary::AfterHeal}) {
                TempleFixture retry(in,recovered);retry.enter();unsigned calls=0;
                retry.flow->smithBoundary=[&](auto here){
                    if(here==boundary && ++calls==1)throw std::runtime_error("synthetic Heal publication fault");
                };
                retry.heal(5);retry.leave();
                check(calls==(boundary==XeenSmithBoundary::BeforeHeal?2u:1u) && retry.flow->canSave(),
                    "Temple Heal publication fault did not retry to Quiet");
                sameSnapshot(expected,retry.snapshot());
            }
        }
        {
            auto tampered=injured(in);TempleFixture fixture(in,tampered);
            fixture.enter();unsigned calls=0;
            fixture.flow->smithBoundary=[&](auto here){if(here==XeenSmithBoundary::BeforeHeal) {
                ++calls;auto &hp=fixture.p.roster.at(1).currentHp;
                const auto original=std::int16_t(hp);hp=original+1;hp=original;
            }};
            fixture.act(SelectMemberAction{5});fixture.act(AcknowledgeAction{});
            fixture.act(AcknowledgeAction{});
            save_test::rejects([&]{fixture.prepare();});
            check(calls==1 && !fixture.flow->canSave() && fixture.p.monsterTreasure->gold==810 &&
                fixture.p.roster.at(6).currentHp==-15 && fixture.p.roster.at(1).currentHp==0,
                "Temple BeforeHeal mutation-reversion reached payment or Quiet");
        }
        {
            auto stale=injured(in);TempleFixture fixture(in,stale);
            fixture.enter();fixture.act(SelectMemberAction{5});fixture.act(AcknowledgeAction{});
            const auto old=fixture.flow->frame().presentation();
            const auto semantic=*fixture.flow->displayedInput();
            fixture.present(fixture.flow->refresh(true));
            const auto current=fixture.flow->frame().presentation();
            check(old!=current && semantic==fixture.flow->displayedInput(),
                "Temple cosmetic redraw did not preserve semantic input authority");
            fixture.flow->beginCycle(++fixture.cycle);
            fixture.present(fixture.flow->handle(AcknowledgeAction{},semantic,old));
            check(fixture.p.monsterTreasure->gold==810 && !fixture.flow->canSave(),
                "stale Temple confirmation paid or exposed Quiet");
            fixture.flow->beginCycle(++fixture.cycle);
            fixture.present(fixture.flow->handle(AcknowledgeAction{},semantic,current));
            fixture.prepare();
            check(fixture.p.monsterTreasure->gold==400,
                "current concrete Temple confirmation did not publish Heal");
            fixture.act(AcknowledgeAction{});fixture.leave();
            check(fixture.p.encounterContext->day==10,
                "Temple cosmetic handoff changed paid departure");
        }
        std::cout<<"Synthetic Temple recovery, one/two-day departure and exact restore passed\n";
        return 0;
    } catch(const std::exception &error){std::cerr<<error.what()<<'\n';return 1;}
}
