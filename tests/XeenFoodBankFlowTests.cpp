#include "XeenTestInstallation.h"
#include "formats/xeen/XeenVocDuration.h"
#include "XeenTrainingTestSupport.h"
#include "games/xeen/XeenRestoreGuard.h"
#include <array>
#include <iostream>
#include <queue>

using namespace training_test;
void foodBankSaveChecks(const mmodern::GameInstallation &);
namespace {
std::string stage;
struct Visit : Fixture {
    Visit(Inputs &inputs, const XeenSaveSnapshot &source, const XeenEventFile *city = nullptr)
        : Fixture(inputs, source, false, city) {
        flow->drawTavernArt = [&inputs](auto &image) { inputs.assets.drawTavern(image); };
        flow->drawBankArt = [&inputs](auto &image) { inputs.assets.drawBank(image); };
        flow->tavernFoodOffer = [&inputs](unsigned index) { return inputs.assets.tavernFoodOffer(index); };
        flow->tavernString = [&inputs](std::size_t index) { return inputs.assets.tavernString(index); };
        flow->loadRestDream = [&inputs] { return inputs.assets.restDreamImage(); };
    }
    auto dialog() { return flow->inputContext(flow->frame().presentation()).dialog; }
    void pulse() {
        now += 100; flow->beginCycle(++cycle);
        if (const auto next = flow->updatePresentation()) present(*next);
    }
    void denied() {
        check(!flow->canSave() && !XeenSaveState::canCapture(p,c,w), "Modal Food/Bank state permits saving");
        save_test::rejects([&] { flow->beginSave(); });
        XeenRestoreGuard owners(w,p,c,f);
        const auto frame = flow->frame().presentation();
        const auto input = flow->displayedInput();
        flow->handle(SaveGameAction{},input,frame);
        // Application uses canSave to gate its control panel. The low-level
        // Flow treats any key as acknowledgment on a refusal, so avoid
        // accidentally consuming feedback while probing that same gate.
        const auto controls=dialog();
        if(!controls || !controls->anyKey)flow->handle(ControlPanelAction{},input,frame);
        owners.check();
        check(!flow->canSave(), "Save/control-panel input escaped service ownership");
    }
    void key(unsigned value) { act(DialogKeyAction{value}); }
    void choose(unsigned value, bool mouse = false) {
        const auto input = dialog(); check(bool(input), "Service input dialog is absent");
        if (!mouse) {
            const auto action = input->key(value); check(bool(action), "Service key is absent"); act(*action);
        } else {
            for (const auto &hit : input->hits) if (hit.key == value) {
                const auto action = input->click((hit.left+hit.right)/2,(hit.top+hit.bottom)/2);
                check(bool(action), "Service mouse hit is absent"); act(*action); return;
            }
            throw std::runtime_error("Service mouse command is absent: " + std::to_string(value));
        }
    }
    void dismiss() { if (const auto input = dialog(); input && input->anyKey) act(AcknowledgeAction{}); }
    void open() {
        act(InteractionAction{}); denied();
        for (unsigned n=0;n<1000;++n) {
            const auto input=dialog();
            if(input && (input->key('f') || input->key('d'))) break;
            pulse();
        }
        check(w.sessionState().journeyActivity()==XeenJourneyActivity::Service && dialog() &&
            (dialog()->key('f') || dialog()->key('d')), "Food/Bank lobby was not admitted");
        denied();
    }
    void leave() {
        for(unsigned n=0;n<100 && !flow->canSave();++n) {
            denied(); dismiss();
            if(!flow->canSave()) act(CancelInteractionAction{});
            if(!flow->canSave()) pulse();
        }
        if(!flow->canSave())throw std::runtime_error("Food/Bank departure did not settle: "+flow->encounter()->notice());
    }
    void food(bool yes, bool mouse = false) {
        dismiss(); choose('f',mouse); denied();
        check(dialog() && dialog()->key('y') && dialog()->key('n'), "Food Yes/No is absent");
        choose(yes?'y':'n',mouse); denied();
    }
    void currency(bool withdrawal, bool gems, bool mouse=false) {
        dismiss(); choose(withdrawal?'w':'d',mouse); denied();
        choose(gems?'e':'o',mouse); denied();
    }
    void amount(const std::string &text, bool textInput=true) {
        check(dialog() && dialog()->textEntry, "Bank amount editor is absent");
        if(textInput) act(TextInputAction{text}); else for(unsigned char digit:text) key(digit);
        denied(); key(InputKey::Enter); denied(); dismiss();
        check(dialog() && dialog()->key('o') && dialog()->key('e') && !dialog()->textEntry,
            "Bank attempt did not return to currency submenu");
    }
};

XeenCamera entrance(const XeenEventFile &city, unsigned action) {
    const int x=action==3?24:26,y=action==3?5:17;
    for(const auto &record:city.records) if(record.x==x && record.y==y && record.line==0)
        return {28,x,y,record.direction==4?XeenDirection::North:XeenDirection(record.direction)};
    throw std::runtime_error("Original Food/Bank entry is absent");
}
XeenSaveSnapshot source(Inputs &in,unsigned action,unsigned day=8) {
    auto result=in.service(day);result.camera=entrance(in.city,action);return result;
}
void checkBalances(const Visit &v,std::uint32_t gold,std::uint32_t gems,
        std::uint32_t bankGold,std::uint32_t bankGems) {
    check(v.p.monsterTreasure->gold==gold && v.p.monsterTreasure->gems==gems &&
        v.p.serviceEconomy->bank.gold==bankGold && v.p.serviceEconomy->bank.gems==bankGems,
        "Bank transfer balances differ");
}
void tavern(Inputs &in) {
    stage="Original Tavern full/refill/cancel and mouse parity";
    for(bool mouse:{false,true}) {
        auto s=source(in,3);s.food=90;s.journey->treasure->gold=30;
        s.gameFlags[57]=s.gameFlags[58]=s.gameFlags[232]=s.gameFlags[255]=true;
        Visit v(in,s);unsigned handoffs=0;
        v.flow->smithBoundary=[&](auto boundary) {
            v.denied();
            if(boundary==XeenSmithBoundary::BeforeReservation) {
                ++handoffs;check(v.f.isSet(231) && !v.f.isSet(57) && !v.f.isSet(58) &&
                    !v.f.isSet(232) && !v.f.isSet(255), "Tavern prelude was not published before service admission");
            }
        };
        v.open();check(handoffs==1,"Tavern prelude/handoff repeated");
        for(unsigned command:{'s'}) {
            XeenRestoreGuard owners(v.w,v.p,v.c,v.f);v.choose(command,mouse);
            check(v.dialog() && v.dialog()->anyKey,"Deferred Tavern command has no notice");v.dismiss();owners.check();
        }
        v.choose(InputKey::F1+1,mouse);v.food(true,mouse);
        check(v.p.food==90 && v.p.monsterTreasure->gold==30,"Full packs charged or reduced food");
        v.dismiss();v.food(false,mouse);check(v.p.food==90 && v.p.monsterTreasure->gold==30,"No charged food");
        v.dismiss();v.choose('f',mouse);v.act(CancelInteractionAction{});
        check(v.p.food==90 && v.p.monsterTreasure->gold==30,"Cancelled offer changed food/gold");
        v.leave();check(v.p.encounterContext->day==9 && v.p.encounterContext->minutes==s.journey->context->minutes,
            "Unpaid Tavern visit did not cost exactly one day");
    }
    for(unsigned food:{0u,89u,90u,91u}) for(unsigned gold:{9u,10u}) {
        auto s=source(in,3);s.food=food;s.journey->treasure->gold=gold;
        s.characters[s.activeRosterIds[0]].conditions[13]=1;s.characters[s.activeRosterIds[0]].currentHp=0;
        Visit v(in,s);v.open();v.food(true);
        const bool paid=food<90 && gold>=10;
        check(v.p.food==(paid?90:food) && v.p.monsterTreasure->gold==(paid?gold-10:gold),
            "Food full/insufficient/selected-dead-member rule differs");v.leave();
    }
    auto s=source(in,3);s.food=1;
    Visit key(in,s),mouse(in,s);key.open();mouse.open();
    check(key.flow->frame().pixels==mouse.flow->frame().pixels,"Original Tavern lobby pixels differ");
    key.food(true);mouse.food(true,true);key.leave();mouse.leave();save_test::sameSnapshot(key.snapshot(),mouse.snapshot());
}
void tavernOptions(Inputs &in) {
    stage="Original Tavern Drink, Tip, Rumors and exact saved cursor";
    for(bool mouse:{false,true}) {
        auto s=source(in,3);s.journey->treasure->gold=100;
        s.journey->random=XeenJourneyRandomState{1,1,0};
        const auto owner=s.activeRosterIds[0];s.characters[owner].conditions[7]=0;
        Visit v(in,s);v.open();
        const auto initialTip=v.w.tavernTipIndex(28);
        const auto before=*v.w.sessionState().journeyRandom();
        XeenCombatRandom expected(before);std::optional<std::uint32_t> roll;
        do {roll=expected.draw(1,100);} while(!roll);
        v.choose('d',mouse);
        check(v.p.monsterTreasure->gold==99 && *v.w.sessionState().journeyRandom()==expected.continuation() &&
            v.p.roster.at(owner).conditions[7]==(*roll<26?1u:0u),
            "Drink payment, single original draw or Drunk increment differs");
        check(*roll>=26,"Tavern fixture seed must leave its member sober");
        v.dismiss();const auto beforeTip=*v.w.sessionState().journeyRandom();
        v.choose('t',mouse);
        check(v.p.monsterTreasure->gold==98 && v.w.tavernTipIndex(28)==std::uint8_t(std::uint8_t(initialTip+1)%15) &&
            *v.w.sessionState().journeyRandom()==beforeTip,"Tip did not retain DOS modulo cursor without a draw");
        v.dismiss();v.choose('t',mouse);v.dismiss();
        check(v.p.monsterTreasure->gold==98 && *v.w.sessionState().journeyRandom()==beforeTip,
            "Tip without another Drink charged or drew");
        v.choose('r',mouse);const auto rumor=v.flow->frame().pixels;v.dismiss();
        v.choose('r',mouse);check(rumor==v.flow->frame().pixels,"same-day Rumors changed text");v.dismiss();
        check(v.p.monsterTreasure->gold==98 && *v.w.sessionState().journeyRandom()==beforeTip,"Rumors paid or drew");
        v.leave();const auto saved=v.snapshot();const auto bytes=XeenSaveFormat::encode(saved);
        Visit restored(in,XeenSaveFormat::decode(bytes));
        check(bytes==XeenSaveFormat::encode(restored.snapshot()) && restored.w.tavernTipIndex(28)==v.w.tavernTipIndex(28),
            "Tavern v14 restore replayed a payment, tip, time or draw");
    }
    auto s=source(in,3);s.journey->treasure->gold=10;
    const auto owner=s.activeRosterIds[0];s.characters[owner].conditions[7]=1;
    Visit drunk(in,s);drunk.open();const auto tip=drunk.w.tavernTipIndex(28);
    drunk.choose('d');drunk.dismiss();const auto random=*drunk.w.sessionState().journeyRandom();
    drunk.choose('t');drunk.dismiss();drunk.choose('t');drunk.dismiss();
    check(drunk.p.monsterTreasure->gold==9 && drunk.w.tavernTipIndex(28)==tip &&
        *drunk.w.sessionState().journeyRandom()==random,"Drunk Tip charged, advanced or drew");
    drunk.leave();
    auto unable=source(in,3);unable.journey->treasure->gold=10;
    unable.characters[unable.activeRosterIds[0]].conditions[13]=1;
    unable.characters[unable.activeRosterIds[0]].currentHp=0;
    Visit disabled(in,unable);disabled.open();
    const auto untouched=*disabled.w.sessionState().journeyRandom();
    for(const auto action:{'d','t'}) {
        disabled.choose(action);
        check(disabled.dialog() && disabled.dialog()->anyKey && disabled.p.monsterTreasure->gold==10 &&
            *disabled.w.sessionState().journeyRandom()==untouched,"disabled Tavern member needs a notice without payment or draw");
        disabled.dismiss();
    }
    disabled.leave();
}
void bank(Inputs &in) {
    stage="Original Bank transactions and amount input";
    for(bool mouse:{false,true}) {
        auto s=source(in,0);s.journey->treasure->gold=500;s.journey->treasure->gems=50;
        s.journey->serviceEconomy->bank={100,10};Visit v(in,s);v.open();
        v.currency(false,false,mouse);v.amount("125",!mouse);checkBalances(v,375,50,225,10);
        v.choose('e',mouse);v.amount("50");checkBalances(v,375,0,225,60);
        v.choose('e',mouse);check(!v.dialog()->textEntry && v.dialog()->anyKey,"Empty source entered amount input");
        v.dismiss();v.choose('o',mouse);v.amount("0");checkBalances(v,375,0,225,60);
        v.choose('o',mouse);v.amount("");checkBalances(v,375,0,225,60);
        v.choose('o',mouse);v.amount("376");checkBalances(v,375,0,225,60);
        for(const char *overflow:{"4294967296","4294967306","9999999999"}) {
            v.choose('o',mouse);v.amount(overflow);checkBalances(v,375,0,225,60);
        }
        v.choose('o',mouse);v.act(TextInputAction{"123"});v.key(8);v.key(127);v.key('4');v.key(InputKey::Enter);v.dismiss();
        checkBalances(v,361,0,239,60);
        v.choose('o',mouse);v.act(TextInputAction{"900"});v.act(CancelInteractionAction{});checkBalances(v,361,0,239,60);
        check(v.dialog()->key('o') && !v.dialog()->textEntry,"Amount Escape did not back out one level");
        v.act(CancelInteractionAction{});check(bool(v.dialog()->key('w')),"Currency Escape did not return to lobby");
        v.currency(true,false,mouse);v.amount("239");checkBalances(v,600,0,0,60);
        v.choose('e',mouse);v.amount("60");checkBalances(v,600,60,0,0);
        v.leave();check(v.p.encounterContext->day==9 && v.p.encounterContext->minutes==s.journey->context->minutes,
            "Bank transactions added time or departure was not one day");
        Visit restored(in,v.snapshot());save_test::sameSnapshot(v.snapshot(),restored.snapshot());
    }
    auto s=source(in,0);s.journey->treasure->gold=100;s.journey->serviceEconomy->bank.gold=UINT32_MAX;
    Visit v(in,s);v.open();v.currency(false,false);v.amount("1");
    check(v.p.serviceEconomy->bank.gold==0 && v.p.monsterTreasure->gold==99,"Bank destination arithmetic did not wrap uint32");v.leave();
}
void departure(Inputs &in) {
    stage="Bank final balances, stock/RNG order and settlement retry";
    for(unsigned day:{8u,10u,99u}) for(bool withdrawal:{false,true}) {
        stage="Bank departure day="+std::to_string(day)+" withdrawal="+std::to_string(withdrawal);
        auto s=source(in,0,day);s.journey->treasure->gold=500;s.journey->serviceEconomy->bank={1000,200};
        Visit v(in,s);v.open();v.currency(withdrawal,false);v.amount("250");
        auto finalEconomy=*s.journey->serviceEconomy;finalEconomy.bank.gold=withdrawal?750:1250;
        XeenServiceDayCandidate expected(*s.journey->context,finalEconomy,*s.journey->random);
        while(!expected.service(64)){} expected.validateComplete();
        v.leave();check(*v.p.encounterContext==expected.context() && *v.p.serviceEconomy==expected.economy() &&
            *v.w.sessionState().journeyRandom()==expected.continuation(),
            "Departure interest did not use final balances exactly once, or regenerated stock/RNG");
    }
    for(auto boundary:{XeenSmithBoundary::BeforeDeparture,XeenSmithBoundary::DeparturePublished,
            XeenSmithBoundary::AfterDeparture,XeenSmithBoundary::Return,
            XeenSmithBoundary::BeforeEventSettlement,XeenSmithBoundary::AfterEventSettlement}) {
        stage="Bank departure boundary="+std::to_string(unsigned(boundary));
        auto s=source(in,0,10);s.journey->treasure->gold=500;s.journey->serviceEconomy->bank.gold=1000;
        Visit v(in,s);v.open();v.currency(false,false);v.amount("200");
        unsigned faults=0,completed=0;v.flow->reportManual=[&](const auto &result) {
            if(std::holds_alternative<XeenManualEventCompleted>(result))++completed;
        };
        v.flow->smithBoundary=[&](auto here) {v.denied();if(here==boundary && faults++==0)throw std::bad_alloc();};
        v.leave();check(faults && completed==1 && v.p.monsterTreasure->gold==300 &&
            v.p.serviceEconomy->bank.gold==1212 && v.p.encounterContext->day==11,
            "Settlement retry lost or repeated transfer/interest/day/Event completion");
    }
}
void transactionFaults(Inputs &in) {
    stage="Food/Bank transaction publication retries";
    for(unsigned action:{0u,3u}) for(auto boundary:{XeenSmithBoundary::BeforePurchase,XeenSmithBoundary::AfterPurchase}) {
        auto s=source(in,action);s.food=1;s.journey->treasure->gold=100;
        Visit v(in,s);v.open();unsigned faults=0;
        v.flow->smithBoundary=[&](auto here){v.denied();if(here==boundary && faults++==0)throw std::bad_alloc();};
        if(action==3)v.food(true);else{v.currency(false,false);v.amount("10");}
        check(faults,"Transaction publication seam was not reached");
        const bool published=boundary==XeenSmithBoundary::AfterPurchase;
        check(v.p.monsterTreasure->gold==(published?90:100) &&
            (action==3?v.p.food==(published?90:1):v.p.serviceEconomy->bank.gold==(published?10:0)),
            "Transaction fault changed publication atomicity");
        v.dismiss();v.flow->smithBoundary={};
        if(!published) {
            if(action==3)v.food(true);else{v.choose('o');v.amount("10");}
        }
        v.leave();check(v.p.monsterTreasure->gold==90 &&
            (action==3?v.p.food==90:v.p.serviceEconomy->bank.gold==10),
            "Transaction retry lost or repeated paid publication");
    }
}
XeenEventRecord record(unsigned x,unsigned y,unsigned line,unsigned opcode,std::vector<std::uint8_t> parameters={}) {
    XeenEventRecord result;result.x=x;result.y=y;result.direction=4;result.line=line;
    result.opcode=opcode;result.parameters=std::move(parameters);result.lengthField=5+result.parameters.size();return result;
}
void genericPrelude(Inputs &in) {
    stage="Generic relocated Event prelude and terminal services";
    for(bool calledTerminal:{false,true}) for(bool unsupported:{false,true}) {
        auto s=source(in,0);s.camera={28,20,20,XeenDirection::North};
        XeenWorld geometry(in.mapLoader());const auto cell=geometry.sampleCell(28,20,20);bool facing=false;
        for(unsigned direction=0;direction<4;++direction) {
            const auto wall=wallAt(*cell->cell,XeenDirection(direction));
            if(wall!=1 && wall!=6 && wall!=9 && wall!=13){s.camera.direction=XeenDirection(direction);facing=true;break;}
        }
        check(facing,"Relocated service has no ordinary Event-facing wall");
        XeenEventFile city{28,"maze0028.evt",true,{}};
        city.records={record(20,20,0,0x19,{70,71,0}),
            record(20,20,1,calledTerminal?0x13:0x11,calledTerminal?std::vector<std::uint8_t>{}:std::vector<std::uint8_t>{0}),
            record(20,20,2,0x13),record(70,71,0,0x0c,{0,0,20,220}),
            record(70,71,1,unsupported?0x13:calledTerminal?0x11:0x1a,
                !unsupported && calledTerminal?std::vector<std::uint8_t>{0}:std::vector<std::uint8_t>{})};
        Visit v(in,s,&city);const auto before=XeenSaveFormat::encode(v.snapshot());
        if(unsupported) {
            v.act(InteractionAction{});for(unsigned n=0;n<20 && !v.flow->canSave();++n){v.dismiss();v.pulse();}
            check(v.flow->canSave() && !v.f.isSet(220) && before==XeenSaveFormat::encode(v.snapshot()),
                "Unsupported dependent prelude published a prefix or admitted service");
        } else {
            unsigned publications=0,completed=0;
            v.flow->smithBoundary=[&](auto boundary) {
                v.denied();if(boundary==XeenSmithBoundary::BeforeReservation){++publications;check(v.f.isSet(220),"Relocated prelude lost publication");}
            };
            v.flow->reportManual=[&](const auto &result){if(std::holds_alternative<XeenManualEventCompleted>(result))++completed;};
            v.open();v.leave();check(publications==1 && completed==1 && v.f.isSet(220) && v.p.encounterContext->day==9,
                "Generic terminal service replayed prelude, resumed suffix or lost settlement");
        }
    }
}
void directionPrelude(Inputs &in) {
    stage="Direction-changing prelude preserves original Event lookup and terminal settlement";
    for(unsigned form=0;form<3;++form) for(bool unsupported:{false,true}) {
        auto s=source(in,0);s.camera={28,20,20,XeenDirection::North};
        XeenWorld geometry(in.mapLoader());const auto cell=geometry.sampleCell(28,20,20);bool facing=false;
        for(unsigned direction=0;direction<4;++direction) {
            const auto wall=wallAt(*cell->cell,XeenDirection(direction));
            if(wall!=1 && wall!=6 && wall!=9 && wall!=13){s.camera.direction=XeenDirection(direction);facing=true;break;}
        }
        check(facing,"Direction-prelude fixture has no ordinary Event-facing wall");
        const auto originalDirection=s.camera.direction;
        const auto finalDirection=XeenDirection((unsigned(originalDirection)+1)%4);
        XeenEventFile city{28,"maze0028.evt",true,{}};
        if(form==0) {
            city.records={record(20,20,0,0x1b,{84,std::uint8_t(finalDirection)}),
                record(20,20,1,0x0c,{0,0,20,220}),
                record(20,20,2,unsupported?0x13:0x11,unsupported?std::vector<std::uint8_t>{}:std::vector<std::uint8_t>{0}),
                record(20,20,3,0x13)};
        } else {
            const bool calledTerminal=form==1;
            city.records={record(20,20,0,0x19,{70,71,0}),
                record(20,20,1,calledTerminal?0x13:0x11,calledTerminal?std::vector<std::uint8_t>{}:std::vector<std::uint8_t>{0}),
                record(20,20,2,0x13),record(70,71,0,0x1b,{84,std::uint8_t(finalDirection)}),
                record(70,71,1,0x0c,{0,0,20,220}),
                record(70,71,2,unsupported?0x13:calledTerminal?0x11:0x1a,
                    !unsupported && calledTerminal?std::vector<std::uint8_t>{0}:std::vector<std::uint8_t>{})};
        }
        // Every instruction matches only the entry direction. Using the
        // changed camera direction for lookup loses the flag/service suffix.
        for(auto &instruction:city.records)instruction.direction=unsigned(originalDirection);
        Visit v(in,s,&city);const auto before=XeenSaveFormat::encode(v.snapshot());
        if(unsupported) {
            v.act(InteractionAction{});for(unsigned n=0;n<20 && !v.flow->canSave();++n){v.dismiss();v.pulse();}
            check(v.flow->canSave() && !v.f.isSet(220) && v.c.direction==originalDirection &&
                before==XeenSaveFormat::encode(v.snapshot()),"Refused direction prelude published camera or flag prefix");
        } else {
            unsigned publications=0,completed=0;
            v.flow->smithBoundary=[&](auto boundary) {
                v.denied();
                if(boundary==XeenSmithBoundary::BeforeReservation) {
                    ++publications;check(v.f.isSet(220) && v.c.direction==finalDirection && v.c.x==20 && v.c.y==20,
                        "Direction prelude did not publish camera and flag together before Bank admission");
                }
            };
            v.flow->reportManual=[&](const auto &result){if(std::holds_alternative<XeenManualEventCompleted>(result))++completed;};
            v.open();const auto suspended=XeenTrainingTestAccess::serviceState(*v.flow);
            check(suspended.lookupDirection==originalDirection && suspended.workingCamera.direction==finalDirection,
                "Direction prelude changed the retained Event lookup direction");
            v.leave();check(publications==1 && completed==1 && v.f.isSet(220) && v.c.direction==finalDirection &&
                v.p.encounterContext->day==9,"Direction-changing terminal service replayed prelude or resumed caller suffix");
        }
    }
}

XeenSaveSnapshot fresh(Inputs &in) {
    auto party=XeenPartyLoader().loadFromResources(in.chr,in.pty);
    auto camera=XeenCharacterFormat::parsePartyLocation(in.pty);XeenGameFlags flags;
    XeenWorld world(in.mapLoader(),in.objectLoader());
    XeenJourneySetup setup{in.chr,XeenGameplayContextFormat::parse(in.pty),in.statistics,in.city,1,in.regional()};
    setup.prepared=false;setup.purse=XeenCharacterFormat::parseMonsterPurse(in.pty);
    setup.bank=XeenCharacterFormat::parseBankBalances(in.pty);setup.worldFlags=XeenQuestFlagFormat::parseWorldFlags(in.pty);
    setup.regionalText=in.texts.load(23);setup.learnedNames=in.names;setup.learnedNamesProvider=[&in]{return in.names;};
    setup.cityEventsProvider=[&in]{return in.city;};
    setup.mainlandEventsProvider=[&in]{return in.mainland;};
    XeenEncounterFlow flow(world,party,camera,flags,[]{return 0;},setup);
    check(flow.prepareJourneyFrame(flow.ticket(),[]{}) && flow.presentJourney(flow.ticket()),"Original New Game frame");
    check(party.food==90 && party.monsterTreasure->gold==800 && camera.mapId==XeenMapIdentity(28),"Original New Game state");
    return XeenSaveState::capture(in.signature,party,camera,flags,world);
}
void settle(Visit &v) {
    for(unsigned n=0;n<6000;++n) {
        if(v.flow->canSave())return;
        if(const auto *combat=v.flow->encounter()->combat()) {
            check(combat->phase()!=XeenCombatPhase::Defeat && combat->phase()!=XeenCombatPhase::SupportStopped &&
                combat->phase()!=XeenCombatPhase::Failed,"Original route could not continue combat");
            if(combat->phase()==XeenCombatPhase::PlayerReady)v.act(AttackAction{});else v.pulse();
        } else {
            const auto dialog=v.dialog();
            if(dialog && dialog->anyKey)v.act(AcknowledgeAction{});
            else if(dialog && dialog->key('y') && dialog->key('n'))v.act(YesAction{});
            else if(v.w.sessionState().journeyActivity()==XeenJourneyActivity::Event ||
                    v.w.sessionState().journeyActivity()==XeenJourneyActivity::Reward)v.act(AcknowledgeAction{});
            else v.pulse();
        }
    }
    throw std::runtime_error("Original route settlement exceeded bound: "+v.flow->encounter()->notice());
}
void face(Visit &v,XeenDirection direction) {
    for(unsigned n=0;n<4 && v.c.direction!=direction;++n){v.act(NavigationAction::TurnRight);settle(v);}
    check(v.c.direction==direction,"Original route failed to turn");
}
void walk(Visit &v,int x,int y) {
    std::array<int,1024> parent;parent.fill(-1);std::array<XeenDirection,1024> directions{};
    const int start=v.c.y*32+v.c.x,end=y*32+x;parent[start]=start;std::queue<int> pending;pending.push(start);
    while(!pending.empty() && parent[end]<0) {
        const int cell=pending.front();pending.pop();
        for(unsigned d=0;d<4;++d) {
            XeenCamera next{28,cell%32,cell/32,XeenDirection(d)};
            if(XeenMovement().apply(v.w,next,NavigationAction::MoveForward)!=XeenMovementResult::Moved)continue;
            const int index=next.y*32+next.x;if(parent[index]>=0)continue;
            parent[index]=cell;directions[index]=XeenDirection(d);pending.push(index);
        }
    }
    check(parent[end]>=0,"Original service route is unreachable");std::vector<XeenDirection> route;
    for(int cell=end;cell!=start;cell=parent[cell])route.push_back(directions[cell]);
    for(auto it=route.rbegin();it!=route.rend();++it){face(v,*it);v.act(NavigationAction::MoveForward);settle(v);}
    check(v.c.x==x && v.c.y==y,"Original route failed to reach service");
}
void originalJourney(Inputs &in) {
    stage="Original New Game -> Tavern -> Rest -> refill -> Bank -> save/load";
    Visit v(in,fresh(in));const auto tavern=entrance(in.city,3),bank=entrance(in.city,0);
    walk(v,tavern.x,tavern.y);face(v,tavern.direction);const auto gold=v.p.monsterTreasure->gold;
    v.open();v.food(true);check(v.p.food==90 && v.p.monsterTreasure->gold==gold,"New Game full packs charged");v.leave();
    // Economic fixtures intentionally have no shop sound provider. This original
    // route supplies the actual VOC headers for the complete Rest sequence.
    v.flow->loadSampleDuration=[&](const char *name,XeenSceneArchive archive){return xeenVocDurationNanoseconds(in.assets.readArchiveResource(name,archive));};
    v.act(RestAction{});settle(v);v.flow->loadSampleDuration={};check(v.p.food<90,"Original Rest did not consume food");
    const auto refillGold=v.p.monsterTreasure->gold;v.open();v.food(true);
    check(v.p.food==90 && v.p.monsterTreasure->gold==refillGold-10,"Original post-Rest refill failed");v.leave();
    walk(v,bank.x,bank.y);face(v,bank.direction);v.open();
    v.currency(false,false);v.amount("10");v.choose('e');v.amount("1");
    v.act(CancelInteractionAction{});v.currency(true,false);v.amount("1");v.choose('e');v.amount("1");v.leave();
    const auto saved=v.snapshot();const auto bytes=XeenSaveFormat::encode(saved);
    Visit restored(in,XeenSaveFormat::decode(bytes));save_test::sameSnapshot(saved,restored.snapshot());
    check(bytes==XeenSaveFormat::encode(restored.snapshot()) && v.f.isSet(231),
        "Food/balances/flags/time/RNG did not restore byte-exactly");
    check(XeenSaveFormat::kJourneyVersion==16 && XeenSaveFormat::kJourneySchema==9 && XeenSaveFormat::kJourneyContent==14,
        "Food/Bank did not retain the current save format");
}
}
int main(int argc,char **argv) {
    try {
        check(argc==2,"usage: food-bank-flow <original-installation>");
        const auto installation=xeenTestInstallationDetector().detect(argv[1]);check(bool(installation),"Original installation unavailable");
        Inputs inputs(*installation);tavern(inputs);tavernOptions(inputs);bank(inputs);departure(inputs);transactionFaults(inputs);genericPrelude(inputs);directionPrelude(inputs);originalJourney(inputs);stage="Application Food/Bank save exclusion";foodBankSaveChecks(*installation);
        std::cout<<"Food/Bank original UI, prelude, transactions, settlement, saving and New Game route passed\n";return 0;
    } catch(const std::exception &error) {std::cerr<<stage<<": "<<error.what()<<'\n';return 1;}
}
