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
        // All item bytes except the curse bit survive, including empty slots.
        c.conditions[0]=9;c.weapons[0]={37,1,0xc3,4};c.armor[8]={8,0,0x40,7};
        check(xeenTempleUncurseCost(c,{XeenSide::Clouds,30})==160,"Uncurse surcharge/level price differs");
        xeenTempleClearCurse(c);
        check(!c.conditions[0] && c.weapons[0].material==37 && c.weapons[0].id==1 &&
            c.weapons[0].state==0x83 && c.weapons[0].frame==4 && c.armor[8].state==0 &&
            c.armor[8].material==8 && c.armor[8].frame==7,"Uncurse changed other item bytes");
        check(!xeenTempleUncurseCost(c),"Uncursed character still priced");
        auto donation=context;donation.day=8;donation.effects[5]=99;donation.lightAndResistances[0]=100;
        std::uint8_t counter=0;
        for(unsigned count=1;count<=8;++count)check(xeenTempleDonationBlessing(donation,counter)==(count==8),"Donation threshold differs");
        check(counter==8 && donation.effects[3]==1 && donation.effects[5]==8 && donation.effects[6]==8 &&
            donation.effects[7]==8 && donation.effects[8]==8 && donation.lightAndResistances[0]==1,"Donation did not overwrite spell effects");
        donation.day=0;counter=0;
        for(unsigned count=1;count<=10;++count)check(xeenTempleDonationBlessing(donation,counter)==(count==10),"Donation day zero differs");
        check(donation.effects[5]==10 && counter==0,"Donation zero counter blessing amount differs");
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
        context.day=8;
        XeenServiceDayCandidate visit(context,economy,random,XeenScriptServiceCharge::OneDay,false,true);
        visit=visit.addTemplePaidDay();visit=visit.addTemplePaidDay();
        check(visit.minutes()==4320 && visit.context().day==11 && visit.triggered(),"Repeated Temple payments did not accumulate");
        auto effects=context;effects.effects[3]=1;effects.lightAndResistances[0]=1;
        visit=visit.rebindContext(effects);
        check(visit.context().effects[3]==1 && visit.context().lightAndResistances[0]==1 &&
            visit.beforeRandom()==random && visit.minutes()==4320,"Donation rebind lost reserved time/RNG");
        // Forty-six payments: 47*1440 = 67680 -> 2144 minutes in the DOS word.
        XeenServiceDayCandidate wrapped(context,economy,random,XeenScriptServiceCharge::OneDay,false,true);
        for(unsigned payment=0;payment<46;++payment)wrapped=wrapped.addTemplePaidDay();
        check(wrapped.minutes()==2144 && wrapped.context().day==9 && wrapped.context().minutes==context.minutes+704,
            "Temple farewell word did not wrap");
        // DOS 0x263be: minute word sum wraps before unsigned normalization.
        auto high=context;high.day=0;high.minutes=1300;
        XeenServiceDayCandidate minuteWrap(high,economy,random,static_cast<XeenScriptServiceCharge>(65000),false,true);
        check(minuteWrap.context().day==0 && minuteWrap.context().minutes==764 && !minuteWrap.triggered() &&
            minuteWrap.continuation()==random,"Service minute sum did not wrap 1300+65000 to 764");
        // 604+32767=33371 -> day23/min251; one more minute flips only the
        // signed regeneration test, not the unsigned calendar normalization.
        high.minutes=604;
        XeenServiceDayCandidate signedLow(high,economy,random,static_cast<XeenScriptServiceCharge>(32767),false,true);
        XeenServiceDayCandidate signedHigh(high,economy,random,static_cast<XeenScriptServiceCharge>(32768),false,true);
        check(signedLow.context().day==23 && signedLow.context().minutes==251 && signedLow.triggered() &&
            signedHigh.context().day==23 && signedHigh.context().minutes==252 && !signedHigh.triggered() &&
            signedHigh.continuation()==random,"Service signed charge boundary 32767/32768 differs");
        high.day=8;
        XeenServiceDayCandidate decade(high,economy,random,static_cast<XeenScriptServiceCharge>(32768),false,true);
        check(decade.context().day==31 && decade.triggered(),"Day-one decade regeneration was lost for a high-word charge");
        auto limit=context;limit.year=65535;limit.day=98;
        XeenServiceDayCandidate lastDay(limit,economy,random,XeenScriptServiceCharge::OneDay,false,true);
        rejects([&]{static_cast<void>(lastDay.addTemplePaidDay());},"calendar year overflow");
        check(lastDay.minutes()==1440 && lastDay.context().day==99 && lastDay.beforeRandom()==random,
            "refused paid-day successor changed the retained unpaid departure");
        context.day=99;XeenServiceDayCandidate rollover(context,economy,random);check(rollover.context().day==0 && rollover.context().year==611,"Temple rollover");
        std::cout<<"Temple quote and one/two-day complete-reservation matrix passed\n";return 0;
    } catch(const std::exception &error){std::cerr<<error.what()<<'\n';return 1;}
}
