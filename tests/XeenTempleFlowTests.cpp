#include "XeenTestInstallation.h"
#include "XeenTrainingTestSupport.h"
#include "games/xeen/XeenTempleHeal.h"
#include <iostream>
using namespace training_test;
using save_test::sameSnapshot;
namespace mmodern {
struct XeenTemplePriceStateTestAccess {
    static void publish(XeenTemplePriceState &state,XeenMapIdentity map) {
        state.checkPublication(state.revision());state.publish(state.prepare(map));
    }
    static void exhaust(XeenTemplePriceState &state) {state._revision=UINT64_MAX;}
    static const XeenTempleSurcharges &prices(const XeenEventFlow &flow) {
        const auto *ui=flow._smithUi?&*flow._smithUi:nullptr;
        return flow._encounter->templePrices(ui && flow._smithSettlement && ui->phase==XeenEventFlow::SmithUi::Phase::Departure?
            &ui->templePrices:nullptr,ui?ui->templePriceRevision:0);
    }
    static bool settlement(const XeenEventFlow &flow) {return flow._smithSettlement && !flow._encounter->_smith && bool(flow._smithUi);}
    static bool uiRetained(const XeenEventFlow &flow) {return bool(flow._smithUi);}
    static void withoutVisit(const XeenEventFlow &flow) {(void)flow._encounter->templePrices();}
    static void quoteWithoutVisit(XeenEventFlow &flow) {flow._encounter->quoteTempleHeal(0);}
    static void actionWithoutVisit(XeenEventFlow &flow,char key) {(void)flow._encounter->simpleService(0,key);}
    static IndexedFrame settlementPanel(XeenEventFlow &flow) {
        const auto feedback=std::exchange(flow._smithUi->feedback,{});
        auto panel=flow.drawSmith(IndexedFrame{320,200,save_test::Bytes(64000)});
        flow._smithUi->feedback=feedback;return panel;
    }
    static IndexedFrame expectedPanel(XeenEventFlow &flow,const XeenTempleSurcharges &row) {
        const auto &ui=*flow._smithUi;
        return drawXeenLocation(flow.dosText(),IndexedFrame{320,200,save_test::Bytes(64000)},ui.art,flow._inventoryFont,
            XeenLocationDialog::Temple,flow._party,ui.member,flow._camera.mapId,flow.drawDialogSprite,false,{},&row);
    }
    static std::uint8_t donations(const XeenEventFlow &flow) {return flow._encounter->_smith->donationCounter;}
    static std::string text(const XeenEventFlow &flow) {return flow.smithText();}
    static bool admitted(const XeenEventFlow &flow) {return bool(flow._encounter->_smith);}
    static void quote(XeenEventFlow &flow) {flow._encounter->_smith->frame.reset();flow._encounter->quoteTempleHeal(0);}
    static std::uint32_t quoted(const XeenEventFlow &flow) {return flow._encounter->_smith->healResult.price;}
};
}
namespace {
using PriceAccess=XeenTemplePriceStateTestAccess;
XeenSaveSnapshot templeSource(Inputs &in,unsigned map) {
    auto source=in.base();source.camera=map==30?XeenCamera{30,6,21,XeenDirection::North}:
        map==49?XeenCamera{49,15,3,XeenDirection::North}:XeenCamera{74,4,4,XeenDirection::North};
    source.journey->context->day=8;source.journey->purse->gold=100000;
    auto &c=source.characters[source.activeRosterIds[0]];c.currentHp=0;c.conditions[0]=1;
    const auto actors=XeenActorApproach::actorsFromResources(in.maps.loadObjects(in.assets,map),in.statistics);
    auto &table=source.map(map).monsters;table.emplace(XeenSaveMonsterTable{std::uint16_t(actors.size()),{}});
    for(const auto &actor:actors)table->slots.push_back({actor.x,actor.y,actor.lifecycle,actor.status,false,-1});
    return source;
}
// One process owner is borrowed by successive sessions; no session fallback.
struct PriceSession {
    Inputs &in;XeenWorld w;XeenPartyState p;XeenCamera c;XeenGameFlags flags;XeenEventSystem events;
    std::uint64_t cycle=0,now=0;std::unique_ptr<XeenEventFlow> flow;
    PriceSession(Inputs &in,XeenProgramState &prices,const XeenSaveSnapshot &source):in(in),
        w(in.mapLoader(),in.objectLoader()),events([&](auto id){return XeenEventScript(in.events.load(id));},[&](auto id){return in.texts.load(id);}) {
        XeenSaveState::restoreBeforeGameplay(XeenSaveFormat::decode(XeenSaveFormat::encode(source)),in.resources(),p,c,flags,w,
            [](auto &,const auto &,const auto &,const auto &){});
        flow=std::make_unique<XeenEventFlow>(prices,w,events,p,c,flags,in.font,[](auto){return XeenEventFlow::Composition{frame(),false};},
            XeenEventPresenter::NpcDraw{},[&]{return now;},XeenEventPresenter::RandomFrame{},nullptr,
            [](auto,auto){return XeenEventFlow::Composition{frame(),false};},nullptr,
            [](auto &,const auto &,const auto &,auto,auto){return XeenEventFlow::Composition{frame(),false};});
        flow->dialogText=&in.assets.uiText();
        flow->drawTempleArt=[&](auto &image){in.assets.drawTemple(image);};
        flow->drawDialogSprite=[&](auto &image,const char *resource,unsigned id,int x,int y){in.assets.drawDialogSprite(image,resource,id,x,y);};
        present(flow->frame());
    }
    void present(const IndexedFrame &f){if(f.presentation())flow->framePresented(f.presentation());}
    void act(PlayerAction a){flow->beginCycle(++cycle);present(flow->handle(a,flow->displayedInput(),flow->frame().presentation()));}
    void step(){flow->beginCycle(++cycle);if(auto f=flow->updatePresentation())present(*f);}
    void enter(){act(InteractionAction{});for(unsigned n=0;n<1000 && !PriceAccess::admitted(*flow);++n)step();check(PriceAccess::admitted(*flow),"Original Temple was not admitted");}
    void leave(){act(CancelInteractionAction{});for(unsigned n=0;n<1000 && !flow->canSave();++n)step();check(flow->canSave(),"Temple did not return Quiet");}
    XeenSaveSnapshot snapshot(){return XeenSaveState::capture(in.signature,p,c,flags,w);}
};
void settlementPrices(Inputs &in) {
    for(unsigned map:{30u,74u}) {
        XeenProgramState program{mmodern::XeenWallClock::fixed(0)};auto &prices=program.templePrices;
        if(map==74) {PriceSession prior(in,program,templeSource(in,30));prior.enter();prior.leave();}
        PriceSession s(in,program,templeSource(in,map));s.enter();
        const auto row=prices.row();const auto revision=prices.revision();
        check(row==xeenTempleSurcharges(30),"Settlement test lacks Rivercity prices");
        unsigned faults=0,redraws=0;
        const auto draw=s.flow->drawDialogSprite;
        s.flow->drawDialogSprite=[&](auto &image,const char *resource,unsigned id,int x,int y){
            if(PriceAccess::settlement(*s.flow)) {
                ++redraws;check(PriceAccess::prices(*s.flow)==row,"Settlement redraw lost visit prices");
            }
            draw(image,resource,id,x,y);
        };
        s.flow->smithBoundary=[&](auto boundary){if(boundary==XeenSmithBoundary::BeforeEventSettlement){++faults;throw std::runtime_error("Temple settlement retry");}};
        s.act(CancelInteractionAction{});
        for(unsigned n=0;n<1000 && !faults;++n)s.step();
        check(faults==1 && redraws>0 && PriceAccess::settlement(*s.flow),"Settlement failure did not redraw retained Temple");
        check(PriceAccess::settlementPanel(*s.flow).pixels==PriceAccess::expectedPanel(*s.flow,row).pixels,"Settlement screen uses different Temple prices");
        check(prices.row()==row && prices.revision()==revision,"Settlement failure republished process prices");
        save_test::rejects([&]{PriceAccess::withoutVisit(*s.flow);},"require");
        save_test::rejects([&]{PriceAccess::quoteWithoutVisit(*s.flow);},"unavailable");
        for(char key:{'D','u'})save_test::rejects([&]{PriceAccess::actionWithoutVisit(*s.flow,key);},"unavailable");
        s.act(AcknowledgeAction{});
        check(faults==2 && prices.row()==row && prices.revision()==revision,"Repeated settlement changed process row");
        s.flow->smithBoundary={};s.act(AcknowledgeAction{});
        for(unsigned n=0;n<1000 && !s.flow->canSave();++n)s.step();
        check(s.flow->canSave() && !PriceAccess::uiRetained(*s.flow),"Settlement did not discard Temple UI");
        check(prices.row()==row && prices.revision()==revision,"Completed settlement changed process row");
        save_test::rejects([&]{PriceAccess::prices(*s.flow);},"require");
    }
}
void processPrices(Inputs &in) {
    {
        XeenProgramState program{mmodern::XeenWallClock::fixed(0)};auto &prices=program.templePrices;const auto source=templeSource(in,74);PriceSession s(in,program,source);
        unsigned sounds=0;s.flow->loadSampleDuration=[&](const char *,XeenSceneArchive){++sounds;return std::uint64_t(0);};
        s.enter();check(prices.row()==XeenTempleSurcharges{} && prices.revision()==1,"Fresh map 74 did not publish zero row");
        const auto context=*s.p.encounterContext;const auto gold=s.p.purse->gold;
        const auto random=*s.w.sessionState().journeyRandom();const auto soundBefore=sounds;
        const auto text=PriceAccess::text(*s.flow);
        check(text==xeenTempleText(in.assets.uiText(),s.p,0,prices.row()),"Temple text does not use visit row");
        s.act(DialogKeyAction{'d'});
        check(!PriceAccess::donations(*s.flow) && *s.p.encounterContext==context && s.p.purse->gold==gold &&
            *s.w.sessionState().journeyRandom()==random && sounds==soundBefore,"Zero Donation changed counter, effects, gold, time, RNG or sound");
        const auto uncurse=xeenTempleUncurseCost(s.p.party.member(s.p.roster,0),prices.row());
        check(uncurse==20*s.p.party.member(s.p.roster,0).currentLevel(),"Zero row removed Uncurse base charge");
        s.act(DialogKeyAction{'u'});check(s.p.purse->gold==gold-uncurse && !s.p.party.member(s.p.roster,0).conditions[0],"Zero-row Uncurse charge differs");
        const auto heal=xeenQuoteTempleHeal(s.p.party.member(s.p.roster,0),s.p.purse->gold,*s.p.encounterContext,prices.row()).price;
        check(heal>0,"Zero row removed Heal base charge");
        // The one-step action must spend exactly the quote displayed by the panel.
        s.act(DialogKeyAction{'h'});for(unsigned n=0;n<1000 && s.p.purse->gold==gold-uncurse;++n)s.step();
        check(s.p.purse->gold==gold-uncurse-heal,"Map 74 Heal and displayed quote differ");s.leave();
    }
    for(unsigned map:{30u,49u}) {
        XeenProgramState program{mmodern::XeenWallClock::fixed(0)};auto &prices=program.templePrices;const auto row=xeenTempleSurcharges(map);
        const auto source=templeSource(in,74);const auto bytes=XeenSaveFormat::encode(source);
        {
            PriceSession s(in,program,templeSource(in,map));bool redrawn=false;
            const auto draw=s.flow->drawDialogSprite;
            s.flow->drawDialogSprite=[&](auto &image,const char *resource,unsigned id,int x,int y){
                if(!PriceAccess::admitted(*s.flow)) {
                    redrawn=true;check(prices.row()==XeenTempleSurcharges{} && prices.revision()==0,"Rendering published process prices");
                    check(PriceAccess::prices(*s.flow)==row && PriceAccess::text(*s.flow)==xeenTempleText(in.assets.uiText(),s.p,0,row),"Preparation redraw used old process prices");
                }
                draw(image,resource,id,x,y);
            };
            s.enter();check(redrawn && prices.row()==row,"Tabled Temple did not publish its prepared row");
            PriceAccess::quote(*s.flow);
            check(PriceAccess::quoted(*s.flow)==xeenQuoteTempleHeal(s.p.party.member(s.p.roster,0),s.p.purse->gold,*s.p.encounterContext,row).price,"Temple quote and text row disagree");
        }
        // New session owners and canonical restore do not own the process row.
        const auto revision=prices.revision();
        for(unsigned session=0;session<2;++session) {
            PriceSession s(in,program,source);
            check(prices.revision()==revision+session && XeenSaveFormat::encode(s.snapshot())==bytes,"Restore saved or reset process prices");
            s.enter();check(prices.row()==row && PriceAccess::prices(*s.flow)==row,"Session replacement lost Temple carry-over");
        }
        XeenProgramState fresh{mmodern::XeenWallClock::fixed(0)};PriceSession without(in,fresh,source);PriceSession with(in,program,source);
        check(XeenSaveFormat::encode(without.snapshot())==XeenSaveFormat::encode(with.snapshot()),"Prior visit entered save bytes");
    }
    for(unsigned fault=0;fault<5;++fault) {
        XeenProgramState program{mmodern::XeenWallClock::fixed(0)};auto &prices=program.templePrices;PriceSession s(in,program,templeSource(in,30));
        bool injected=false;const auto initial=prices.row();
        if(fault==0)s.flow->drawTempleArt=[](auto &){throw std::runtime_error("preflight refusal");};
        if(fault==3)PriceAccess::exhaust(prices);
        s.flow->smithBoundary=[&](auto boundary){
            if(boundary==XeenSmithBoundary::BankPrepared && (fault==1 || fault==2)) {
                injected=true;
                if(fault==1)throw std::runtime_error("failed admission");
                PriceAccess::publish(prices,49); // A different publication makes this preparation stale.
            }
            if(boundary==XeenSmithBoundary::AfterAdmission && fault==4){injected=true;throw std::runtime_error("post-admission failure");}
        };
        s.act(InteractionAction{});for(unsigned n=0;n<20 && !s.flow->canSave() && !PriceAccess::admitted(*s.flow);++n)s.step();
        if(fault==4) {
            check(injected && prices.row()==xeenTempleSurcharges(30) && prices.revision()==1 && PriceAccess::admitted(*s.flow),"AfterAdmission rolled back prices or visit");
            s.flow->smithBoundary={};s.act(AcknowledgeAction{});s.act(DialogKeyAction{'u'});
            check(s.p.purse->gold<100000,"Post-admission failure stranded usable visit");s.leave();
        } else {
            const auto expected=fault==2?xeenTempleSurcharges(49):initial;
            const auto revision=fault==2?1:fault==3?UINT64_MAX:0;
            check(!PriceAccess::admitted(*s.flow) && prices.row()==expected && prices.revision()==revision,"Refused, failed or stale admission published prices");
            if(fault==1 || fault==2)check(injected,"Admission fault was not reached");
        }
    }
    {
        XeenProgramState program{mmodern::XeenWallClock::fixed(0)};auto &prices=program.templePrices;auto source=templeSource(in,30);source.journey->context->year=65535;source.journey->context->day=99;
        PriceSession s(in,program,source);s.act(InteractionAction{});for(unsigned n=0;n<20;++n)s.step();
        check(prices.revision()==0 && prices.row()==XeenTempleSurcharges{},"Refused departure published Temple row");
    }
}
}

namespace {
XeenSaveSnapshot injured(Inputs &in,unsigned day=8) {
    auto s=in.service(day);
    s.journey->content=14;
    s.camera={28,15,28,XeenDirection::North};
    s.journey->purse->gold=810;
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
        act(SelectMemberAction{member});act(DialogKeyAction{'h'});
        prepare();
    }
    void leave(){act(CancelInteractionAction{});finishDeparture();check(flow->canSave(),"Temple departure did not return Quiet");}
};
void departureRetries(Inputs &in) {
    for(bool paid:{false,true}) {
        TempleFixture reference(in,injured(in));reference.enter();
        if(paid)reference.heal(5);
        reference.leave();const auto expected=reference.snapshot();
        auto boundaries=std::vector<XeenSmithBoundary>{XeenSmithBoundary::BeforeDeparture,XeenSmithBoundary::AfterDeparture,
                XeenSmithBoundary::Return,XeenSmithBoundary::BeforeEventSettlement,
                XeenSmithBoundary::AfterEventSettlement};
        if(paid)boundaries.push_back(XeenSmithBoundary::StockComplete);
        for(auto boundary:boundaries)for(unsigned room:{8u,9u,10u}) {
            TempleFixture retry(in,injured(in));retry.enter();if(paid)retry.heal(5);
            XeenTrainingTestAccess::limit(*retry.flow,2,UINT64_MAX-room);
            unsigned faults=0;
            retry.flow->smithBoundary=[&](auto here){if(here==boundary){++faults;
                throw std::runtime_error("persistent Temple departure fault");}};
            retry.act(CancelInteractionAction{});
            // Paid departure now generates stock in bounded retained slices.
            // Reach the injected boundary without issuing another service action.
            for(unsigned n=0;!faults && n<1000;++n) {
                retry.flow->beginCycle(++retry.cycle);
                if(auto next=retry.flow->updatePresentation())retry.present(*next);
            }
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
            retry.flow->smithBoundary={};retry.act(AcknowledgeAction{});retry.finishDeparture();
            check(retry.flow->canSave(),"finite authority stranded mandatory Temple departure");
            sameSnapshot(expected,retry.snapshot());
        }
    }
}
void revisionLimits(Inputs &in) {
    for(bool alreadyPaid:{false,true})for(bool afterFault:{false,true})for(unsigned room:{1u,2u,3u}) {
        TempleFixture fixture(in,injured(in));fixture.enter();if(alreadyPaid)fixture.heal(5);
        fixture.act(SelectMemberAction{alreadyPaid?4u:5u});
        const auto gold=fixture.p.purse->gold;
        const auto owner=alreadyPaid?1u:6u;
        const auto before=fixture.p.roster.at(owner);
        XeenTrainingTestAccess::templeRevision(*fixture.flow,UINT64_MAX-room);
        unsigned faults=0;
        fixture.flow->smithBoundary=[&](auto here){if(afterFault && here==XeenSmithBoundary::AfterHeal && ++faults==1)
            throw std::runtime_error("post-publication Temple revision fault");};
        fixture.act(DialogKeyAction{'h'});
        if(room==1) {
            check(fixture.p.purse->gold==gold &&
                xeen_state::sameCharacter(before,fixture.p.roster.at(owner)) &&
                !XeenTrainingTestAccess::templePending(*fixture.flow),
                "Temple confirmation paid without room for Upgrade and Result revisions");
        } else {
            check(XeenTrainingTestAccess::templeRevision(*fixture.flow)==UINT64_MAX-room+1,
                "Temple confirmation did not reserve the first UI revision");
            fixture.prepare();
            check(fixture.p.purse->gold==gold-(alreadyPaid?60u:380u) &&
                XeenTrainingTestAccess::templeRevision(*fixture.flow)==UINT64_MAX-room+2,
                "Temple paid Result revision wrapped or debit differed");
            fixture.act(AcknowledgeAction{});
        }
        fixture.flow->smithBoundary={};
        if(XeenTrainingTestAccess::templeLobby(*fixture.flow))fixture.leave();
        else fixture.act(AcknowledgeAction{});
        fixture.finishDeparture();
        const unsigned paidActions=unsigned(alreadyPaid)+unsigned(room!=1);
        check(fixture.flow->canSave() && fixture.p.encounterContext->day==9+paidActions,
            "Temple UI revision exhaustion lost mandatory departure");
    }
    // Independently exercise the final check while a detached Heal is pending.
    TempleFixture pending(in,injured(in));pending.enter();pending.act(SelectMemberAction{5});
    pending.act(DialogKeyAction{'h'});
    XeenTrainingTestAccess::templeRevision(*pending.flow,UINT64_MAX);
    pending.flow->beginCycle(++pending.cycle);
    check(!pending.flow->updatePresentation() && pending.p.purse->gold==810 &&
        pending.p.roster.at(6).currentHp==-15 && XeenTrainingTestAccess::templePending(*pending.flow),
        "Temple publication ignored exhausted Result UI revision");
    pending.act(CancelInteractionAction{});pending.leave();
    check(pending.p.encounterContext->day==9 && pending.p.purse->gold==810,
        "Temple publication capacity failure lost unpaid exit reservation");
}
void quotePreview(Inputs &in) {
    {
        auto source=injured(in);
        auto &i=source.journey->supplements[6].inputs;
        i.resistances->fireTemporary=i.resistances->coldTemporary=i.resistances->electricalTemporary=5;
        i.resistances->energyTemporary=i.resistances->magicTemporary=i.poisonResistance->temporary=5;
        TempleFixture fixture(in,source);
        const auto heal=xeenPrepareTempleHeal(fixture.p,6,*fixture.p.encounterContext,XeenMapIdentity(28));
        const auto &r=*heal.inputs.resistances;
        check(heal.result.outcome==XeenTempleHealOutcome::Healed && !r.fireTemporary && !r.coldTemporary &&
            !r.electricalTemporary && !r.energyTemporary && !r.magicTemporary && !heal.inputs.poisonResistance->temporary,
            "Paid Temple Heal must clear all six temporary resistances");
    }
    for(unsigned scenario=0;scenario<4;++scenario) {
        auto source=injured(in);auto &c=source.characters[6];
        c.conditions.fill(0);c.currentHp=1;c.endurance.permanent=13;
        c.endurance.temporary=20;c.temporaryLevel=1;c.conditions[4]=1;
        if(scenario==1)c.currentHp=100;
        if(scenario==2){c.currentHp=-15;c.conditions[12]=c.conditions[13]=1;}
        if(scenario==3){c.conditions[3]=2;c.conditions[8]=1;}
        TempleFixture fixture(in,source);
        // Quote uses level4 before temporary reset. HP assignment uses level3:
        // Endurance13/Disease1 gives 6; Dead suppresses that modifier, giving9.
        // Scenario 2 has HP -15, which the DOS unsigned comparison treats as
        // above maximum: Disease 40 + Unconscious 40 + Dead 450, no HP term.
        constexpr unsigned prices[]{80,40,530,160};
        constexpr int hp[]{6,6,9,6};
        fixture.enter();fixture.act(SelectMemberAction{5});
        check(XeenTrainingTestAccess::templeText(*fixture.flow).find(std::to_string(prices[scenario]))!=std::string::npos,
            "Temple original panel omits Heal price");
        fixture.act(DialogKeyAction{'h'});fixture.prepare();
        check(fixture.p.roster.at(6).currentHp==hp[scenario] &&
            fixture.p.roster.at(6).currentSp==27 &&
            fixture.p.purse->gold==810-prices[scenario] &&
            XeenTrainingTestAccess::templeLobby(*fixture.flow),
            "one-step Temple Heal differs from independent candidate or retains result phase");
        fixture.leave();
    }
}
void refusedResult(Inputs &in,bool exhaustedRandom) {
    auto source=injured(in,exhaustedRandom?8:98);
    if(exhaustedRandom)source.journey->random->count=UINT64_MAX;
    TempleFixture fixture(in,source);
    const auto before=fixture.p.roster.at(6);
    const auto inputs=*fixture.p.roster.combatInputs(6);
    const auto gold=fixture.p.purse->gold;
    const auto maximum=XeenCharacterRules::maxHp(before,{610});
    const auto expected=xeenPrepareTempleHeal(fixture.p,6,*fixture.p.encounterContext,XeenMapIdentity(28));
    fixture.enter();fixture.act(SelectMemberAction{5});fixture.act(DialogKeyAction{'h'});
    if(XeenTrainingTestAccess::templePending(*fixture.flow))fixture.prepare();
    if(!exhaustedRandom) {
        fixture.leave();check(fixture.p.encounterContext->day==0 && fixture.p.encounterContext->year==611 &&
            fixture.p.purse->gold==expected.result.goldAfter &&
            xeen_state::sameCharacter(expected.character,fixture.p.roster.at(6)),"Paid Temple Heal year rollover");
        const auto saved=fixture.snapshot();TempleFixture restored(in,saved);sameSnapshot(saved,restored.snapshot());return;
    }
    const auto text=XeenTrainingTestAccess::templeText(*fixture.flow);
    check(xeen_state::sameCharacter(before,fixture.p.roster.at(6)) &&
        xeen_state::sameInputs(inputs,*fixture.p.roster.combatInputs(6)) &&
        fixture.p.purse->gold==gold &&
        fixture.w.sessionState().journeyRandom()==source.journey->random &&
        *fixture.p.encounterContext==*source.journey->context,
        "Temple refusal changed live owners before departure");
    check(text.find("not supported yet")!=std::string::npos,
        "Temple date/RNG refusal lacks a visible notice");
    fixture.act(AcknowledgeAction{});fixture.leave();
    check(xeen_state::sameCharacter(before,fixture.p.roster.at(6)) &&
        fixture.p.purse->gold==gold &&
        fixture.p.encounterContext->day==(exhaustedRandom?9:99),
        "Temple refusal lost the unchanged recipient/purse or mandatory one-day exit");
}
void templeOptions(Inputs &in) {
    auto source=in.service(8);source.camera={28,15,28,XeenDirection::North};
    source.journey->purse->gold=1000;
    TempleFixture donation(in,source);donation.enter();
    const auto cursor=*donation.w.sessionState().journeyRandom();
    const auto acknowledge=[&](TempleFixture &v) {
        const auto dialog=v.flow->inputContext(v.flow->frame().presentation()).dialog;
        if(dialog && dialog->anyKey)v.act(AcknowledgeAction{});
    };
    for(unsigned n=0;n<7;++n){donation.act(DialogKeyAction{'d'});acknowledge(donation);}
    check(donation.p.purse->gold==930 && donation.p.encounterContext->effects[XeenPartyEffect::PowerShield]==0,
        "Donation blessed before day-modulo counter matched");
    donation.act(DialogKeyAction{'d'});acknowledge(donation);
    check(donation.p.purse->gold==920 && donation.p.encounterContext->effects[XeenPartyEffect::PowerShield]==8 &&
        donation.p.encounterContext->lightAndResistances[0]==1 && donation.p.encounterContext->effects[XeenPartyEffect::Clairvoyance]==1 && !donation.p.encounterContext->effects[XeenPartyEffect::Levitate] &&
        *donation.w.sessionState().journeyRandom()==cursor,"Donation price, blessing, light or draw differs");
    donation.act(SelectMemberAction{1});donation.act(DialogKeyAction{'d'});acknowledge(donation);
    check(donation.p.purse->gold==920,"blessed visit charged again after member selection");
    donation.leave();check(donation.p.encounterContext->day==9,"Donation added a paid farewell day");
    const auto saved=donation.snapshot();TempleFixture restored(in,saved);
    check(XeenSaveFormat::encode(saved)==XeenSaveFormat::encode(restored.snapshot()),
        "Donation v14 restore replayed effect, time, debit or RNG");
    restored.enter();restored.act(DialogKeyAction{'d'});acknowledge(restored);
    check(restored.p.purse->gold==910,"new visit inherited the free Donation latch");
    restored.leave();

    TempleFixture selected(in,source);selected.enter();
    for(unsigned n=0;n<3;++n){selected.act(DialogKeyAction{'d'});acknowledge(selected);}
    selected.act(SelectMemberAction{0});
    for(unsigned n=0;n<7;++n){selected.act(DialogKeyAction{'d'});acknowledge(selected);}
    check(selected.p.encounterContext->effects[XeenPartyEffect::PowerShield]==0,"same-member F-key did not reset Donation counter");
    selected.act(DialogKeyAction{'d'});acknowledge(selected);
    check(selected.p.encounterContext->effects[XeenPartyEffect::PowerShield]==8 && selected.p.purse->gold==890,
        "Donation counter did not restart after same-member selection");
    selected.leave();

    const auto owner=source.activeRosterIds[0];auto &c=source.characters[owner];
    c.permanentLevel=3;c.temporaryLevel=2;c.conditions[0]=1;
    const auto second=source.activeRosterIds[1];source.characters[second].conditions[0]=1;
    source.characters[second].permanentLevel=3;source.characters[second].temporaryLevel=0;
    check(c.weapons[0].id!=0,"original selected member has no weapon for Uncurse witness");
    c.weapons[0].state=std::uint8_t(c.weapons[0].state)|0xc0;
    auto expected=c;expected.conditions[0]=0;expected.weapons[0].state=std::uint8_t(expected.weapons[0].state)&0xbf;
    TempleFixture uncurse(in,source);uncurse.enter();
    const auto before=*uncurse.w.sessionState().journeyRandom();
    uncurse.act(DialogKeyAction{'u'});acknowledge(uncurse);
    check(uncurse.p.purse->gold==900 && xeen_state::sameCharacter(expected,uncurse.p.roster.at(owner)) &&
        *uncurse.w.sessionState().journeyRandom()==before,"Uncurse changed item bytes, charged incorrectly or drew");
    uncurse.act(SelectMemberAction{1});uncurse.act(DialogKeyAction{'u'});acknowledge(uncurse);
    check(uncurse.p.purse->gold==840 && !uncurse.p.roster.at(second).conditions[0],
        "second paid Uncurse failed");
    uncurse.leave();check(uncurse.p.encounterContext->day==11,"two paid Uncurses did not cost three farewell days");
    const auto final=uncurse.snapshot();TempleFixture loaded(in,final);
    check(XeenSaveFormat::encode(final)==XeenSaveFormat::encode(loaded.snapshot()),
        "Uncurse v14 restore replayed removal, time, debit or RNG");
}
}
int main(int argc,char **argv) {
    try {
        check(argc==2 || argc==3,"usage: temple-flow <original-installation> [retries|revisions|preview|refused-date|refused-rng]");
        const auto installation=xeenTestInstallationDetector().detect(argv[1]);
        check(bool(installation),"original installation unavailable");Inputs in(*installation);
        if(argc==2){processPrices(in);settlementPrices(in);}
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
        departureRetries(in);revisionLimits(in);quotePreview(in);templeOptions(in);
        refusedResult(in,false);refusedResult(in,true);
        auto source=injured(in);
        TempleFixture f(in,source);
        const auto quote=xeenQuoteTempleHeal(f.p.roster.at(6),f.p.purse->gold,*f.p.encounterContext,XeenMapIdentity(28));
        // Level 3, HP -15: no HP term (DOS unsigned comparison), Unconscious 30,
        // Dead 300+50; Vertigo surcharges are zero.
        check(quote.outcome==XeenTempleHealOutcome::Quoted && quote.price==380,"Dead/Unconscious quote differs");
        const auto selected=xeenPrepareTempleHeal(f.p,6,*f.p.encounterContext,XeenMapIdentity(28));
        check(selected.result.outcome==XeenTempleHealOutcome::Healed && selected.result.hpAfter==15 &&
            selected.result.spAfter==27 && selected.result.goldAfter==430 &&
            !selected.character.conditions[12] && !selected.character.conditions[13],
            "selected resurrection result differs");
        f.enter();f.heal(5);
        check(f.p.roster.at(6).currentHp==15 && f.p.roster.at(6).currentSp==27 &&
            !f.p.roster.at(6).conditions[13] && f.p.purse->gold==430 &&
            f.p.encounterContext->day==8,"first Heal mutated wrong owners or charged time early");
        f.heal(4);
        check(f.p.roster.at(1).currentHp==21 && f.p.roster.at(1).currentSp==21 &&
            !f.p.roster.at(1).conditions[12] && f.p.purse->gold==370,
            "second Heal mutated wrong owner or price");
        check(xeenQuoteTempleHeal(f.p.roster.at(6),f.p.purse->gold,*f.p.encounterContext,XeenMapIdentity(28)).outcome==
            XeenTempleHealOutcome::NoCharge,"healthy recipient gained an extra paid quote");
        f.leave();
        check(f.p.encounterContext->day==11 && f.p.purse->gold==370,
            "two paid Temple actions did not make one three-day departure");
        const auto saved=f.snapshot();TempleFixture restored(in,saved);sameSnapshot(saved,restored.snapshot());
        {
            auto noPay=injured(in);TempleFixture refused(in,noPay);
            refused.enter();refused.leave();
            check(refused.p.encounterContext->day==9 && refused.p.purse->gold==810,
                "unpaid Temple visit did not make one one-day departure");
        }
		{
			TempleFixture reference(in,injured(in));reference.enter();reference.leave();
			const auto unpaid=reference.snapshot();
			for(bool overflow:{false,true}) {
				TempleFixture retry(in,injured(in));retry.enter();unsigned faults=0;
				retry.flow->smithBoundary=[&](auto here) {
					if(here==XeenSmithBoundary::BeforeHeal) {
						++faults;
						if(overflow)throw std::overflow_error("synthetic boundary overflow");
						throw std::runtime_error("persistent synthetic Heal fault");
					}
				};
				retry.act(SelectMemberAction{5});retry.act(DialogKeyAction{'h'});
				check(retry.flow->inputContext(retry.flow->frame().presentation()).dialog->key(InputKey::Escape).has_value(),
					"Temple Upgrade did not expose its cancellation control");
				for(unsigned slice=0;slice<1000 && faults<2;++slice) {
					retry.flow->beginCycle(++retry.cycle);
					const auto next=retry.flow->updatePresentation();
					if(next)retry.present(*next);
				}
				check(faults>=2 && XeenTrainingTestAccess::templePending(*retry.flow) &&
					XeenTrainingTestAccess::templeOneDayReserved(*retry.flow) &&
					retry.p.purse->gold==810 && !retry.flow->canSave(),
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
            refused.enter();refused.act(SelectMemberAction{5});refused.act(DialogKeyAction{'h'});
            refused.prepare();refused.leave();
            check(refused.p.encounterContext->day==0 && refused.p.encounterContext->year==611 && refused.p.purse->gold<810 &&
                !refused.p.roster.at(6).conditions[13],"Paid Heal failed at year rollover");
        }
        {
            auto diseased=injured(in);auto &character=diseased.characters[6];
            character.conditions[12]=character.conditions[13]=0;
            character.currentHp=1;
            character.endurance.permanent=13;character.endurance.temporary=0;
            character.conditions[4]=1;
            TempleFixture fixture(in,diseased);
            const auto before=fixture.snapshot();
            for(unsigned owner=0;owner<30;++owner)if(owner!=6)
                check(xeen_state::sameCharacter(before.characters[owner],fixture.p.roster.at(owner)) &&
                    xeen_state::sameInputs(before.journey->supplements[owner].inputs,
                        *fixture.p.roster.combatInputs(owner)),
                    "detached Heal changed another owner");
            fixture.enter();fixture.heal(5);fixture.leave();
            check(fixture.p.roster.at(6).currentHp==6 &&
                XeenCharacterRules::maxHp(fixture.p.roster.at(6),{610})==9 &&
                fixture.p.roster.at(6).currentSp==27 && fixture.p.purse->gold==750,
                "published Disease Heal changed the required HP order or SP");
        }
        {
            auto healthy=injured(in);auto &character=healthy.characters[6];
            character.conditions[12]=character.conditions[13]=0;
            character.currentHp=XeenCharacterRules::maxHp(character,{610});
            character.currentSp=0;character.endurance.temporary=1;
            TempleFixture fixture(in,healthy);
            const auto result=xeenPrepareTempleHeal(fixture.p,6,*fixture.p.encounterContext,XeenMapIdentity(28));
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
            fixture.act(SelectMemberAction{5});fixture.act(DialogKeyAction{'h'});
            save_test::rejects([&]{fixture.prepare();});
            check(calls==1 && !fixture.flow->canSave() && fixture.p.purse->gold==810 &&
                fixture.p.roster.at(6).currentHp==-15 && fixture.p.roster.at(1).currentHp==0,
                "Temple BeforeHeal mutation-reversion reached payment or Quiet");
        }
        {
            auto stale=injured(in);TempleFixture fixture(in,stale);
            fixture.enter();fixture.act(SelectMemberAction{5});
            const auto old=fixture.flow->frame().presentation();
            const auto semantic=*fixture.flow->displayedInput();
            fixture.present(fixture.flow->refresh(true));
            const auto current=fixture.flow->frame().presentation();
            check(old!=current && semantic==fixture.flow->displayedInput(),
                "Temple cosmetic redraw did not preserve semantic input authority");
            fixture.flow->beginCycle(++fixture.cycle);
            fixture.present(fixture.flow->handle(DialogKeyAction{'h'},semantic,old));
            check(fixture.p.purse->gold==810 && !fixture.flow->canSave(),
                "stale Temple confirmation paid or exposed Quiet");
            fixture.flow->beginCycle(++fixture.cycle);
            fixture.present(fixture.flow->handle(DialogKeyAction{'h'},semantic,current));
            fixture.prepare();
            check(fixture.p.purse->gold==430,
                "current concrete Temple confirmation did not publish Heal");
            fixture.act(AcknowledgeAction{});fixture.leave();
            check(fixture.p.encounterContext->day==10,
                "Temple cosmetic handoff changed paid departure");
        }
        std::cout<<"Temple recovery, cumulative paid departure, services and exact restore passed\n";
        return 0;
    } catch(const std::exception &error){std::cerr<<error.what()<<'\n';return 1;}
}
