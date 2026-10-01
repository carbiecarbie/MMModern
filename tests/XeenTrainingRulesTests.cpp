#include "games/xeen/XeenTraining.h"
#include "games/xeen/XeenServiceDay.h"
#include "games/xeen/XeenArmorRepair.h"
#include "XeenSaveTestSupport.h"
#include <iostream>
#include <climits>
using namespace mmodern;
using save_test::check;
using save_test::rejects;
int main() {
    try {
        XeenGameplayContext context;context.year=610;context.day=8;context.minutes=480;
        context.profile=XeenBehaviorProfile::WorldOfXeenClouds;context.difficulty=XeenDifficulty::Adventurer;
        XeenCharacter c;c.rosterId=18;c.race=XeenRace::Human;c.sex=XeenSex::Male;c.birthYear=592;
        c.intellect.permanent=c.personality.permanent=c.endurance.permanent=11;c.currentHp=1;
        XeenCombatInputs i;
        constexpr std::array<std::uint32_t,10> bases{1500,2000,2000,1500,2000,1000,1500,1500,1500,2000};
        for(unsigned cls=0;cls<10;++cls)for(unsigned level:{1u,3u,4u,9u}) {
            c.characterClass=static_cast<XeenCharacterClass>(cls);c.permanentLevel=level;
            const unsigned debit=bases[cls]*(1u<<(level==1?0:level-2));
            for(int delta:{-1,0,1}) {
                i.experience=debit+delta;
                const auto r=xeenQuoteTraining(c,i,10000,context);
                check(r.cost==10*level*level && r.missing==(delta<0?1u:0u) &&
                    r.outcome==(delta<0?XeenTrainingOutcome::MissingExperience:XeenTrainingOutcome::Quoted),"class XP threshold/cost mismatch");
            }
            i.experience=debit;
            check(xeenQuoteTraining(c,i,10*level*level,context).outcome==XeenTrainingOutcome::Quoted,"exact gold refused");
            check(xeenQuoteTraining(c,i,10*level*level-1,context).outcome==XeenTrainingOutcome::InsufficientGold,"insufficient gold admitted");
            c.temporaryLevel=7;
            check(xeenQuoteTraining(c,i,10000,context).missing==0 && xeenQuoteTraining(c,i,10000,context).cost==10*level*level,"temporary level changed Training arithmetic");
            c.temporaryLevel=0;
        }
        c.characterClass=XeenCharacterClass::Knight;c.permanentLevel=3;i.experience=UINT32_MAX;
        check(xeenQuoteTraining(c,i,10000,context).missing==3001,"unsigned XP wrap widened or saturated");
        for(unsigned cls=0;cls<10;++cls)for(unsigned level:{10u,255u}) {
            c.characterClass=static_cast<XeenCharacterClass>(cls);c.permanentLevel=level;i.experience=0;
            check(xeenQuoteTraining(c,i,0,context).outcome==XeenTrainingOutcome::Cap,"class cap evaluated XP shift");
        }
        c.characterClass=XeenCharacterClass::Knight;c.permanentLevel=3;
        c.temporaryLevel=INT_MAX;rejects([&]{xeenQuoteTraining(c,i,0,context);});c.temporaryLevel=0;
        for(auto condition:{XeenCondition::Asleep,XeenCondition::Paralyzed,XeenCondition::Unconscious,XeenCondition::Dead,XeenCondition::Stoned,XeenCondition::Eradicated}) {
            c.conditions[static_cast<unsigned>(condition)]=1;i.experience=2999;
            check(xeenQuoteTraining(c,i,0,context).outcome==XeenTrainingOutcome::MissingExperience,"XP refusal precedence changed");
            i.experience=3000;check(xeenQuoteTraining(c,i,0,context).outcome==XeenTrainingOutcome::CannotAct,"canAct refusal precedence changed");
            c.permanentLevel=10;check(xeenQuoteTraining(c,i,0,context).outcome==XeenTrainingOutcome::Cap,"cap precedence changed");
            c.permanentLevel=3;c.conditions[static_cast<unsigned>(condition)]=0;
        }
        c.permanentLevel=0;rejects([&]{xeenQuoteTraining(c,i,0,context);});
        c.permanentLevel=3;c.characterClass=static_cast<XeenCharacterClass>(10);rejects([&]{xeenQuoteTraining(c,i,0,context);});
        c.characterClass=XeenCharacterClass::Knight;
        for(unsigned owner=0;owner<30;++owner)for(unsigned offset:{312u,320u,322u}) {
            std::vector<std::uint8_t> bytes(10620);bytes[owner*354+offset]=1;
            rejects([&]{xeenValidateTrainingSource(bytes);});
        }
        xeenValidateTrainingSource(std::vector<std::uint8_t>(10620));
        check(xeenSupportedJourneyPair(9,12) && xeenSupportedJourneyPair(9,13) &&
            !xeenSupportedJourneyPair(8,12) && !xeenSupportedJourneyPair(8,13) && !xeenSupportedJourneyPair(10,13),"content pair differs");
        unsigned cells=0;
        for(int y=0;y<32;++y)for(int x=0;x<32;++x)cells+=xeenJourneyContent(12).vertigoCell(x,y);
        check(cells==28 && !xeenJourneyContent(11).vertigoCell(10,11) && !xeenJourneyContent(11).training(),"Training route/capability leaked");
        for(int y=0;y<32;++y)for(int x=0;x<32;++x)
            check(xeenJourneyContent(13).vertigoCell(x,y)==xeenJourneyContent(12).vertigoCell(x,y),"purchase route differs from Training route");
        check(xeenJourneyContent(13).equipmentPurchase() && xeenJourneyContent(13).training() &&
            xeenJourneyContent(13).serviceDays() && xeenJourneyContent(13).armorRepair() &&
            xeenJourneyContent(13).combatCasting() && !xeenJourneyContent(12).equipmentPurchase(),"purchase capability selection differs");
        for(unsigned content:{9u,10u,11u,12u,13u})for(unsigned day:{9u,10u,97u,98u,99u}) {
            context.day=day;const auto next=xeenPrepareSmithDeparture(context,content);
            check(bool(next)==(day<=((content==11 || content==12 || content==13)?98u:9u)),"legacy service date semantics changed");
        }
        context.day=97;XeenServiceEconomy economy;XeenJourneyRandomState rng{1,7,0};
        XeenCombatRandom generation(rng);XeenMerchantStockCandidate stock;
        while(!stock.complete()){XeenConsequenceDraw draw{generation,64,{}};stock.service(draw);}
        economy.wares=stock.wares();rng=generation.continuation();
        XeenServiceDayCandidate departure(context,economy,rng,12);
        check(departure.complete() && departure.context().day==98,"day97 departure missing");
        XeenServiceDayCandidate replacement(departure.context(),departure.economy(),departure.continuation(),12);
        check(replacement.complete() && replacement.context().day==99,"day98 replacement missing");
        rejects([&]{XeenServiceDayCandidate invalid(replacement.context(),replacement.economy(),replacement.continuation(),12);});
        std::cout<<"Training arithmetic, refusal, source zero-domain, route and date rules passed\n";return 0;
    } catch(const std::exception &e){std::cerr<<e.what()<<'\n';return 1;}
}
