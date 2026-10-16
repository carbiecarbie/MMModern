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
		const auto zeroHp=xeenQuoteTempleHeal(c,50,context,XeenMapIdentity(28));
		check(zeroHp.maxHpBefore==0 && zeroHp.price==50 &&
			zeroHp.outcome==XeenTempleHealOutcome::Quoted,
			"original zero-maximum-HP quote was rejected");
		c.conditions[13]=0;
		for(int y=0;y<32;++y)for(int x=0;x<32;++x)
			check(xeenIndoorCoordinate(x,y),"Indoor coordinate capacity excludes a map cell");
		check(!xeenIndoorCoordinate(-1,0)&&!xeenIndoorCoordinate(32,0),"Indoor coordinate capacity admits an invalid cell");
        // DOS XEEN.DAT price routine 0x1c9ce..0x1cd3e. L is the permanent plus
        // temporary level byte; Vertigo surcharges are zero.
        const XeenMapIdentity vertigo(28);
        for(unsigned cls=0;cls<10;++cls)for(unsigned permanent:{1u,3u,9u,255u})
            for(unsigned temporary:{0u,1u,255u}) {
                c.characterClass=static_cast<XeenCharacterClass>(cls);
                c.permanentLevel=permanent;c.temporaryLevel=temporary;c.currentHp=-1;
                c.conditions[12]=1;c.conditions[13]=255;
                const auto r=xeenQuoteTempleHeal(c,UINT32_MAX,context,vertigo);
                const auto level=permanent+temporary;
                // HP -1 is 0xffff unsigned (0x1cadb), so no HP term.
                check(r.outcome==XeenTempleHealOutcome::Quoted &&
                    r.price==10*level+100*level+50*255,
                    "Temple quote class/level/severity arithmetic differs");
            }
        c.characterClass=XeenCharacterClass::Knight;c.permanentLevel=3;c.temporaryLevel=0;
        c.conditions[12]=0;c.conditions[13]=0;c.currentHp=0;
        const auto low=xeenQuoteTempleHeal(c,30,context,vertigo);
        check(low.price==30 && low.outcome==XeenTempleHealOutcome::Quoted,
            "exact carried funds refused");
        check(xeenQuoteTempleHeal(c,29,context,vertigo).outcome==XeenTempleHealOutcome::InsufficientGold,
            "insufficient funds admitted");
        c.currentHp=static_cast<std::int16_t>(low.maxHpBefore);
        check(xeenQuoteTempleHeal(c,0,context,vertigo).outcome==XeenTempleHealOutcome::NoCharge,
            "full HP produced paid Heal");
        // Each of conditions 1..12 alone at full HP: 10*L = 30 (0x1cb08..0x1cb40).
        for(unsigned condition=1;condition<=12;++condition) {
            c.conditions.fill(0);c.conditions[condition]=condition==3?255:1;
            check(xeenQuoteTempleHeal(c,UINT32_MAX,context,vertigo).price==30,
                "Temple condition price differs");
        }
        c.conditions.fill(0);c.conditions[0]=1;
        check(xeenQuoteTempleHeal(c,UINT32_MAX,context,vertigo).outcome==XeenTempleHealOutcome::NoCharge,
            "Cursed entered the Heal price");
        // Dead 0x1cb4e and Stoned 0x1cb93: 100*L+50*n, n the counter.
        c.conditions.fill(0);c.conditions[13]=2;
        check(xeenQuoteTempleHeal(c,UINT32_MAX,context,vertigo).price==400,"Dead price differs");
        c.conditions.fill(0);c.conditions[14]=1;
        check(xeenQuoteTempleHeal(c,UINT32_MAX,context,vertigo).price==350,"Stoned price differs");
        c.conditions[13]=1;
        check(xeenQuoteTempleHeal(c,UINT32_MAX,context,vertigo).price==700,"Dead and Stoned are not both charged");
        // Eradicated 0x1cbd8: 1000*L plus 500*n in signed 16 bits.
        c.conditions.fill(0);c.conditions[15]=1;
        check(xeenQuoteTempleHeal(c,UINT32_MAX,context,vertigo).price==3500,"Eradicated price differs");
        c.conditions[15]=65;
        check(xeenQuoteTempleHeal(c,UINT32_MAX,context,vertigo).price==3000+32500,"Eradicated 16-bit edge differs");
        c.conditions[15]=66; // 500*66 = 33000 wraps to -32536.
        rejects([&]{xeenQuoteTempleHeal(c,UINT32_MAX,context,vertigo);},
            "Temple Heal price is outside carried-gold range");
        c.permanentLevel=40;c.currentHp=-1; // HP -1: no HP term at any level.
        check(xeenQuoteTempleHeal(c,UINT32_MAX,context,vertigo).price==40000-32536,
            "Eradicated 16-bit wrap differs");
        c.permanentLevel=3;
        // Negative and zero HP at Vertigo: below-maximum zero pays 10*L, negative pays nothing.
        c.conditions.fill(0);c.currentHp=-15;
        check(xeenQuoteTempleHeal(c,UINT32_MAX,context,vertigo).outcome==XeenTempleHealOutcome::NoCharge,
            "negative HP paid the HP term");
        c.currentHp=0;
        check(xeenQuoteTempleHeal(c,UINT32_MAX,context,vertigo).price==30,"zero HP price differs");
        // Combined at HP 0: HP 30 + Heart Broken/Weak/Disease/Unconscious 4*30 + Dead 350.
        c.conditions[3]=255;c.conditions[4]=2;c.conditions[8]=1;c.conditions[12]=1;c.conditions[13]=1;
        const auto combination=xeenQuoteTempleHeal(c,UINT32_MAX,context,vertigo);
        check(combination.price==30+4*30+350,
            "combined conditions or severity multiplication differs");
        // Surcharges (0x1ca13..0x1cab3): HP term adds heal, Dead/Stoned add
        // resurrect, Eradicated adds eradicate. Clouds Nightshadow (30),
        // Rivercity (49), Asp (73); Darkside 29/31/37 and other maps.
        const struct {XeenMapIdentity map;std::int16_t heal,resurrect,eradicate,uncurse,donation;} table[]{
            {{XeenSide::Clouds,28},0,0,0,0,10},{{XeenSide::Clouds,30},10,50,500,100,25},
            {{XeenSide::Clouds,49},100,500,5000,300,100},{{XeenSide::Clouds,73},20,100,1000,200,50},
            {{XeenSide::Darkside,29},0,0,0,0,10},{{XeenSide::Darkside,31},10,50,500,100,25},
            {{XeenSide::Darkside,37},20,100,1000,200,50},{{XeenSide::Darkside,28},100,500,5000,300,100}};
        for(const auto &row:table) {
            const auto s=xeenTempleSurcharges(row.map);
            check(s.heal==row.heal && s.resurrect==row.resurrect && s.eradicate==row.eradicate &&
                s.uncurse==row.uncurse && s.donation==row.donation,"Temple surcharge table differs");
        }
        // Asp, level 3, HP 0, Dead 1 and Eradicated 1:
        // (30+20) + (300+50+100) + (3000+500+1000) = 5000.
        c.conditions.fill(0);c.conditions[13]=1;c.conditions[15]=1;
        check(xeenQuoteTempleHeal(c,UINT32_MAX,context,XeenMapIdentity(XeenSide::Clouds,73)).price==5000,
            "surcharged Temple price differs");
        rejects([&]{xeenQuoteTempleHeal(c,UINT32_MAX,context,XeenMapIdentity(XeenSide::Clouds,29));},
            "Temple surcharges are not known");
        c.conditions.fill(0);
        const auto economy=completeEconomy();const XeenJourneyRandomState random{1,7,886};
        for(unsigned day:{8u,9u,10u,11u,19u,20u,97u,98u}) {
            context.day=day;
            XeenServiceDayCandidate unpaid(context,economy,random);finish(unpaid);
            check(unpaid.context().day==day+1 &&
                unpaid.triggered()==((day+1)%10==1),"unpaid Temple date/trigger differs");
            // The paid reservation may cross the year boundary.
            XeenServiceDayCandidate paid=unpaid.upgradeTemplePaid();finish(paid);
            XeenServiceDayCandidate direct(context,economy,random,XeenScriptServiceCharge::TemplePaid);
            finish(direct);
            check(paid.context().day==(day+2)%100 && paid.context().year==610+(day>=98) && paid.triggered() &&
                paid.context()==direct.context() && paid.economy()==direct.economy() &&
                paid.continuation()==direct.continuation(),
                "two-day reservation differed from one reference operation");
            if(unpaid.triggered())check(paid.continuation()==unpaid.continuation(),
                "already-generated reservation was redrawn");
        }
        context.day=99;XeenServiceDayCandidate rollover(context,economy,random);check(rollover.context().day==0 && rollover.context().year==611,"Temple rollover");
        std::cout<<"Temple quote and one/two-day complete-reservation matrix passed\n";return 0;
    } catch(const std::exception &error){std::cerr<<error.what()<<'\n';return 1;}
}
