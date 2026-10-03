#include "games/xeen/XeenTempleHeal.h"
#include "games/xeen/XeenServiceDay.h"
#include "games/xeen/XeenArmorRepair.h"
#include "games/xeen/XeenJourneyContent.h"
#include "XeenSaveTestSupport.h"
#include <iostream>
using namespace mmodern;
using save_test::check;
using save_test::rejects;
namespace {
XeenServiceEconomy completeEconomy() {
    XeenCombatRandom random(XeenJourneyRandomState{1,7,0});XeenMerchantStockCandidate stock;
    while(!stock.complete()) {XeenConsequenceDraw draw{random,64,{}};stock.service(draw);}
    XeenServiceEconomy economy;economy.wares=stock.wares();return economy;
}
void finish(XeenServiceDayCandidate &day) {
    unsigned slices=0;while(!day.service(64))check(++slices<1000,"Temple stock generation did not finish");
    day.validateComplete();
}
}
int main() {
    try {
        XeenGameplayContext context;context.year=610;context.day=8;context.minutes=604;
        context.profile=XeenBehaviorProfile::WorldOfXeenClouds;
        context.difficulty=XeenDifficulty::Adventurer;
        XeenCharacter c;c.rosterId=6;c.race=XeenRace::Human;c.birthYear=592;
        c.intellect.permanent=c.personality.permanent=c.endurance.permanent=11;
		c.permanentLevel=0;c.temporaryLevel=0;c.currentHp=0;c.conditions[13]=1;
		const auto zeroHp=xeenQuoteTempleHeal(c,50,context);
		check(zeroHp.maxHpBefore==0 && zeroHp.price==50 &&
			zeroHp.outcome==XeenTempleHealOutcome::Quoted,
			"original zero-maximum-HP quote was rejected");
		c.conditions[13]=0;
		for(int y=0;y<32;++y)for(int x=0;x<32;++x)
			check(xeenJourneyContent().eventCameraCell(x,y)==xeenJourneyContent().vertigoCell(x,y),
				"Temple Event camera differs from its route");
        for(unsigned cls=0;cls<10;++cls)for(unsigned permanent:{1u,3u,9u,255u})
            for(unsigned temporary:{0u,1u,255u}) {
                c.characterClass=static_cast<XeenCharacterClass>(cls);
                c.permanentLevel=permanent;c.temporaryLevel=temporary;c.currentHp=-1;
                c.conditions[12]=1;c.conditions[13]=255;
                const auto r=xeenQuoteTempleHeal(c,UINT32_MAX,context);
                const auto level=permanent+temporary;
                check(r.outcome==XeenTempleHealOutcome::Quoted &&
                    r.price==10*level+10*level+100*level+50*255,
                    "Temple quote class/level/severity arithmetic differs");
            }
        c.characterClass=XeenCharacterClass::Knight;c.permanentLevel=3;c.temporaryLevel=0;
        c.conditions[12]=0;c.conditions[13]=0;c.currentHp=0;
        const auto low=xeenQuoteTempleHeal(c,30,context);
        check(low.price==30 && low.outcome==XeenTempleHealOutcome::Quoted,
            "exact carried funds refused");
        check(xeenQuoteTempleHeal(c,29,context).outcome==XeenTempleHealOutcome::InsufficientGold,
            "insufficient funds admitted");
        c.currentHp=static_cast<std::int16_t>(low.maxHpBefore);
        check(xeenQuoteTempleHeal(c,0,context).outcome==XeenTempleHealOutcome::NoCharge,
            "full HP produced paid Heal");
        c.currentHp=0;c.conditions[3]=255;c.conditions[4]=2;c.conditions[8]=1;c.conditions[12]=1;c.conditions[13]=1;
        const auto combination=xeenQuoteTempleHeal(c,UINT32_MAX,context);
        check(combination.price==30+4*30+300+50,
            "combined supported conditions or severity multiplication differs");
        const auto economy=completeEconomy();const XeenJourneyRandomState random{1,7,886};
        for(unsigned day:{8u,9u,10u,11u,19u,20u,97u,98u}) {
            context.day=day;
            XeenServiceDayCandidate unpaid(context,economy,random);finish(unpaid);
            check(unpaid.context().day==day+1 &&
                unpaid.triggered()==((day+1)%10==1),"unpaid Temple date/trigger differs");
            if(day==98){rejects([&]{unpaid.upgradeTemplePaid();});continue;}
            XeenServiceDayCandidate paid=unpaid.upgradeTemplePaid();finish(paid);
            XeenServiceDayCandidate direct(context,economy,random,XeenScriptServiceCharge::TemplePaid);
            finish(direct);
            check(paid.context().day==day+2 && paid.triggered() &&
                paid.context()==direct.context() && paid.economy()==direct.economy() &&
                paid.continuation()==direct.continuation(),
                "two-day reservation differed from one reference operation");
            if(unpaid.triggered())check(paid.continuation()==unpaid.continuation(),
                "already-generated reservation was redrawn");
        }
        context.day=99;rejects([&]{XeenServiceDayCandidate unsupported(context,economy,random);});
        std::cout<<"Temple quote and one/two-day complete-reservation matrix passed\n";return 0;
    } catch(const std::exception &error){std::cerr<<error.what()<<'\n';return 1;}
}
