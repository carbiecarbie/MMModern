// Explicit headless presentation and artificial faults. This is separate from
// the original native A/B/C production witnesses; it never mints CastResponse.
#ifndef MMODERN_M39_CAST_CONTROLS_H
#define MMODERN_M39_CAST_CONTROLS_H
#include "games/xeen/XeenRestoreGuard.h"
template<class Handler,class Idle,class Fault>
bool m39CastControls(const std::string &control,const IndexedFrame &first,const Handler &handler,const Idle &idle,
    XeenEventFlow &flow,XeenWorld &world,const XeenPartyState &party,const XeenCamera &camera,
    const XeenGameFlags &flags,const fs::path &target,std::uint64_t &now,std::uint64_t &cycle,
    unsigned &providers,unsigned &saves,std::function<void()> &resourceFault,bool &renderFault,Fault mutate) {
    const auto present=[&](const IndexedFrame &frame){check(flow.acceptsFrame(frame.presentation()),"M39 test presentation must be concrete and current");handler.framePresented(frame.presentation());};
    const auto respond=[&](PlayerAction action){handler.beginCycle(++cycle);auto token=handler.displayedInput();check(bool(token),"M39 control requires presented input");auto next=handler.withDisplayedInput(action,*token);
        const auto before=providers,save=saves;
        if(flow.encounter()->combat()) {
        handler.withDisplayedInput(CastSpellAction{},*token);handler.withDisplayedInput(SaveGameAction{},*token);
        check(before==providers && save==saves,"M39 consumed test input reached providers");
        }
        if(next)present(*next);};
    const auto tick=[&]{now+=100;handler.beginCycle(++cycle);auto frame=idle();if(frame)present(*frame);};
    const auto fight=[&]()->XeenCombat &{check(flow.encounter() && flow.encounter()->combat(),"M39 artificial control lost combat");return const_cast<XeenCombat &>(*flow.encounter()->combat());};
    const auto view=[&]()->XeenCombatCastView{const auto observed=fight().cast();check(observed.has_value(),"M39 artificial control lost cast");return *observed;};
    const auto ready=[&](unsigned slot){for(unsigned i=0;i<500;++i){if(flow.encounter()->combat()) {
        check(fight().phase()!=XeenCombatPhase::Failed && fight().phase()!=XeenCombatPhase::Defeat,"M39 control prefix failed");
        if(fight().phase()==XeenCombatPhase::PlayerReady){if(fight().participant()==int(slot))return;respond(BlockAction{});continue;}
        }else if(world.sessionState().journeyActivity()==XeenJourneyActivity::Event || world.sessionState().journeyActivity()==XeenJourneyActivity::Reward)respond(AcknowledgeAction{});
        tick();}throw std::runtime_error("M39 control prefix bound");};
    const auto quiet=[&]{for(unsigned i=0;i<500 && !flow.canSave();++i)tick();check(flow.canSave(),"M39 artificial prefix quiet bound");};
    present(first);respond(SaveGameAction{});check(saves==3,"M39 control prior save pipeline");
    const auto disk=XeenSaveFormat::encode(XeenSaveFile::read(target));
    respond(NavigationAction::MoveForward);quiet();respond(ShootAction{});quiet();respond(NavigationAction::MoveForward);ready(control=="partial"?0:4);
    if(control=="partial") {
        respond(RunAction{});ready(4);check(!(fight().participants()&1u),"M39 partial control needs legitimate earlier escape");
        respond(CastSpellAction{});respond(NavigationAction::MoveBackward);respond(AcknowledgeAction{});respond(AcknowledgeAction{});
        respond(SelectMemberAction{5});check(view().phase==XeenCombatCastPhase::PartyTarget && party.roster.at(1).currentSp==20,"M39 First Aid accepted unused compact slot/refunded");
        respond(CancelInteractionAction{});check(view().result.refunded && party.roster.at(1).currentSp==21,"M39 partial explicit refund");
        respond(AcknowledgeAction{});ready(5);respond(CastSpellAction{});respond(AcknowledgeAction{});respond(AcknowledgeAction{});tick();
        check(view().phase==XeenCombatCastPhase::Result && view().result.count==6 && party.roster.at(6).currentSp==26 && !(fight().participants()&1u),"M39 Awaken lost full active scope/restored escape");
        std::cout<<"M39 ARTIFICIAL CONTROL PASSED partial\n";return true;
    }
    const auto cursor=*world.sessionState().journeyRandom();const auto priorHp=int(party.roster.at(6).currentHp);
    const auto verifyDisk=[&]{check(disk==XeenSaveFormat::encode(XeenSaveFile::read(target)),"M39 fault changed earlier disk save");};
    const auto deny=[&]{const auto p=providers,s=saves;handler.withDisplayedInput(SaveGameAction{},handler.displayedInput().value_or(0));
        check(!flow.canSave() && p==providers && s==saves,"M39 fault/modal save reached work");verifyDisk();};
    if(control.rfind("observation-",0)==0) {
        // Keep the exposed observations across callbacks. With the former pointer /
        // reference APIs these deliberately mutated the retained combat objects.
        const auto poison=[](auto &observed,auto &result) {
            auto &v=const_cast<XeenCombatCastView &>(*observed);
            v.owner=6;v.participant=3;v.slot=25;v.enemy=XeenMonsterIdentity{23,15};
            v.phase=XeenCombatCastPhase::Result;v.committed=false;
            v.successorPhase=XeenCombatPhase::Failed;v.successorWork=XeenCombatWork::End;v.successorParticipant=0;
            v.result.spell=45;v.result.spBefore=123;v.result.spAfter=-123;
            v.result.refunded=true;v.result.count=6;v.result.effects[0].owner=29;v.result.effects[0].afterHp=99;
            auto &r=const_cast<XeenCombatResult &>(result);
            r.actorHpBefore=17;r.actorHpAfter=99;r.damage=99;r.participant=0;r.actingOwner=29;
            r.monster={23,15};r.targetMonster=XeenMonsterIdentity{23,15};r.participantsAfter=0;r.xpCount=6;
            r.xp[0]={29,0,0xffffffffu};r.generatedArmor=true;r.generation=0;
        };
        const bool arrow=control=="observation-target" || control=="observation-hp" ||
            control=="observation-yield" || control=="observation-successor" || control=="observation-guard";
        if(arrow) {respond(BlockAction{});ready(5);}
        respond(CastSpellAction{});
        if(control=="observation-selection") {
            auto observed=fight().cast();auto &&result=fight().result();poison(observed,result);
            // A corrupted/stale learned-list copy cannot switch the reserved caster or row.
            respond(NavigationAction::MoveBackward);
            check(view().owner==1 && view().participant==4 && view().slot==14,"M39 detached selection redirected caster/spell");
        }else if(control!="observation-awaken") {
            respond(NavigationAction::MoveBackward);if(arrow)respond(NavigationAction::MoveBackward);
        }
        respond(AcknowledgeAction{});if(arrow)respond(AcknowledgeAction{});
        auto observed=fight().cast();auto &&result=fight().result();unsigned callbacks=0;
        if(control=="observation-payer" || control=="observation-target") {
            resourceFault=[&]{++callbacks;poison(observed,result);};
        }
        respond(AcknowledgeAction{});resourceFault={};
        if(control=="observation-payer" || control=="observation-target")check(callbacks>0,"M39 observation provider attack was not exercised");
        check(party.roster.at(1).currentSp==(arrow?21:20) && party.roster.at(6).currentSp==(arrow?25:27),
            "M39 exposed caster/result redirected debit");
        // Fresh reads must be independent of both previously returned copies.
        check(view().owner==(arrow?6u:1u) && view().participant==(arrow?5u:4u),"M39 public cast copy contaminated retained state");
        auto paid=fight().cast();auto &&paidResult=fight().result();
        if(arrow) {
            const auto other=world.sessionState().actors()[15];
            check(view().enemy==std::optional<XeenMonsterIdentity>{{23,9}},"M39 detached target redirected private reservation");
            auto &&random=fight().random();const_cast<XeenCombatRandom &>(random).draw(1,56);
            check(world.sessionState().journeyRandom()==cursor && fight().random().continuation()==cursor,"M39 public RNG copy advanced authoritative cursor");
            std::vector<std::pair<unsigned,unsigned>> requests;const auto trace=replay_test::observeDraw;
            replay_test::observeDraw=[&](auto lo,auto hi,auto value,auto state){requests.emplace_back(lo,hi);if(trace)trace(lo,hi,value,state);};
            if(control=="observation-yield") {
                auto attempts=std::make_shared<unsigned>(0);
                replay_test::filterDraw=[attempts](auto value)->std::optional<std::uint32_t>{return ++*attempts<=64?std::nullopt:value;};
            }
            fight().setProbe([&]{++callbacks;poison(paid,paidResult);
                auto &&exposedRandom=fight().random();const_cast<XeenCombatRandom &>(exposedRandom)=XeenCombatRandom(123);
                if(control=="observation-guard") {
                    auto &actor=const_cast<XeenActor &>(world.sessionState().actors()[9]);++actor.hp;--actor.hp;
                }
            });
            try{tick();}catch(const std::exception &){if(control!="observation-guard")throw;}
            if(control=="observation-guard") {
                check(fight().phase()==XeenCombatPhase::Failed && party.roster.at(6).currentSp==25 &&
                    world.sessionState().actors()[9].hp==7 && world.sessionState().journeyRandom()==cursor,
                    "M39 post-callback authoritative mismatch published effect/refund");
            }else {
                if(control=="observation-yield") {
                    check(view().phase==XeenCombatCastPhase::Preparing && world.sessionState().actors()[9].hp==7 &&
                        world.sessionState().journeyRandom()==cursor && party.roster.at(6).currentSp==25,
                        "M39 corrupted yielded observation published incomplete unit");
                    tick();
                }
                check(callbacks>0 && view().phase==XeenCombatCastPhase::Projectile && fight().result().actorHpBefore==7 &&
                    fight().result().actorHpAfter==0 && fight().result().damage==8 && world.sessionState().actors()[9].hp==0 &&
                    world.sessionState().accountedMonsters().count({23,9}) &&
                    world.sessionState().actors()[15].hp==other.hp && world.sessionState().actors()[15].id==other.id,
                    "M39 observation corrupted Arrow identity/HP/eight-damage/lethal publication");
                check(requests.at(0)==std::pair<unsigned,unsigned>{1,56} &&
                    requests.at(control=="observation-yield"?65:1)==std::pair<unsigned,unsigned>{1,100},
                    "M39 Arrow statistics identity split from damage/drop target");
                for(unsigned i=0;i<6;++i)check(party.roster.combatInputs(kXeenCombatOwners[i])->experience==(i==1 || i==4?2066u:1066u),
                    "M39 detached XP observation redirected lethal credit");
                fight().setProbe({});const auto settled=*world.sessionState().journeyRandom();
                auto projectile=fight().cast();auto &&projectileResult=fight().result();poison(projectile,projectileResult);
                check(flow.encounter()->appearance().projectile.has_value(),"M39 corrupted public phase hid retained projectile");
                const auto visuals=flow.encounter()->appearance().projectiles;
                check(visuals.size()==1 && visuals[0].lane==0 && visuals[0].row==0 && visuals[0].pow==11 &&
                    visuals[0].target==std::optional<XeenMonsterIdentity>{{23,9}},"Magic Arrow shared single lane/selected target");
                tick();check(view().phase==XeenCombatCastPhase::Result && world.sessionState().journeyRandom()==settled,
                    "M39 detached projectile/result repeated RNG/effect");
                check(flow.encounter()->appearance().projectiles.empty(),"Magic Arrow lane did not terminate at its existing phase boundary");
                auto receipt=fight().cast();auto &&receiptResult=fight().result();poison(receipt,receiptResult);
                respond(AcknowledgeAction{});
                check(!fight().cast() && fight().phase()==XeenCombatPhase::VictoryAwaitingEnd && fight().pending()==XeenCombatWork::End &&
                    world.sessionState().journeyRandom()==settled && party.roster.at(6).currentSp==25,
                    "M39 detached successor redirected handoff/acted state");
            }
            replay_test::observeDraw=trace;replay_test::filterDraw={};fight().setProbe({});
        }else {
            poison(paid,paidResult);
            fight().setProbe([&]{++callbacks;poison(paid,paidResult);});
            if(control=="observation-refund")respond(CancelInteractionAction{});
            else if(control=="observation-awaken")tick();
            else respond(SelectMemberAction{5});
            fight().setProbe({});
            check(callbacks>0 && view().phase==XeenCombatCastPhase::Result && party.roster.at(6).currentSp==27 &&
                party.roster.at(1).currentSp==(control=="observation-refund"?21:20) &&
                party.roster.at(6).currentHp==(control=="observation-refund" || control=="observation-awaken"?priorHp:15),
                "M39 detached observation redirected recovery/refund/payer");
            check(view().result.spell==(control=="observation-awaken"?1u:26u) &&
                view().result.count==(control=="observation-awaken"?6u:control=="observation-refund"?0u:1u),
                "M39 detached result changed authoritative effect membership");
            auto receipt=fight().cast();auto &&receiptResult=fight().result();poison(receipt,receiptResult);
            respond(AcknowledgeAction{});
            check(!fight().cast() && fight().phase()==XeenCombatPhase::PlayerReady && fight().participant()==5 && fight().participants()==0x3f &&
                world.sessionState().journeyRandom()==cursor,"M39 detached result redirected acted participant/successor");
        }
        deny();std::cout<<"M39 ARTIFICIAL CONTROL PASSED "<<control<<'\n';return true;
    }
    if(control=="reservation-retry") {
        auto calls=std::make_shared<unsigned>(0);resourceFault=[calls] {if(!(*calls)++)throw std::runtime_error("M39 ordinary reservation provider failure");};
        respond(CastSpellAction{});resourceFault={};check(!fight().cast() && fight().phase()==XeenCombatPhase::PlayerReady && party.roster.at(1).currentSp==21,"M39 ordinary reservation failure did not preserve retry");
        respond(CastSpellAction{});respond(CancelInteractionAction{});
        std::cout<<"M39 ARTIFICIAL CONTROL PASSED reservation-retry\n";return true;
    }
    if(control=="recursive") {
        resourceFault=[&]{const auto p=providers,s=saves;const auto sp=party.roster.at(1).currentSp;
            for(PlayerAction a:{PlayerAction{CastSpellAction{}},PlayerAction{AcknowledgeAction{}},PlayerAction{SelectMemberAction{5}},PlayerAction{CancelInteractionAction{}},PlayerAction{AttackAction{}},PlayerAction{RunAction{}},PlayerAction{SaveGameAction{}}})
                handler.withDisplayedInput(a,flow.displayedInput().value_or(0));
            check(p==providers && s==saves && sp==party.roster.at(1).currentSp,"M39 provider recursion reached work");};
    }else if(control.rfind("aba-",0)==0 || control=="names")resourceFault=[&]{mutate(control);};
    if(control.rfind("aba-",0)==0 || control=="names") {
        try{respond(CastSpellAction{});}catch(const std::exception &){}
        resourceFault={};check(fight().phase()==XeenCombatPhase::Failed && party.roster.at(1).currentSp==21 && world.sessionState().journeyRandom()==cursor,"M39 reservation integrity prefix");
    }else if(control=="before-debit") {
        respond(CastSpellAction{});respond(NavigationAction::MoveBackward);respond(AcknowledgeAction{});
        fight().setProbe([]{throw std::runtime_error("M39 pre-cost injected failure");});
        try{respond(AcknowledgeAction{});}catch(const std::exception &){}
        check(fight().phase()==XeenCombatPhase::Casting && view().phase==XeenCombatCastPhase::Confirm && party.roster.at(1).currentSp==21,"M39 ordinary pre-cost failure lost unchanged confirmation");
        fight().setProbe({});respond(AcknowledgeAction{});respond(CancelInteractionAction{});
        check(view().result.refunded && party.roster.at(1).currentSp==21,"M39 newly presented confirmation retry/refund failed");
    }else if(control.rfind("arrow-",0)==0 || control=="projectile-retry") {
        respond(BlockAction{});ready(5);respond(CastSpellAction{});respond(NavigationAction::MoveBackward);respond(NavigationAction::MoveBackward);
        respond(AcknowledgeAction{});respond(AcknowledgeAction{});respond(AcknowledgeAction{});
        check(view().phase==XeenCombatCastPhase::Preparing && party.roster.at(6).currentSp==25 && world.sessionState().actors()[9].hp==7 && world.sessionState().journeyRandom()==cursor,"M39 Arrow debit/effect separation");
        if(control.rfind("arrow-fail-",0)==0 || control=="arrow-xp-overflow" || control=="arrow-drop-overflow") {
            const auto limit=(control=="arrow-xp-overflow" || control=="arrow-drop-overflow")?0:unsigned(std::stoul(control.substr(11)));auto calls=std::make_shared<unsigned>(0);
            fight().setProbe([calls,limit]{if(++*calls==limit)throw std::runtime_error("M39 Arrow draw/drop pre-store failure");});
            try{tick();}catch(const std::exception &){}
            check(fight().phase()==XeenCombatPhase::Failed && party.roster.at(6).currentSp==25 && world.sessionState().actors()[9].hp==7 &&
                world.sessionState().journeyRandom()==cursor && !world.sessionState().accountedMonsters().count({23,9}),"M39 Arrow incomplete unit published/refunded");
        }else {
            if(control=="arrow-yield") {
                auto calls=std::make_shared<unsigned>(0);replay_test::filterDraw=[calls](auto v)->std::optional<std::uint32_t>{return ++*calls<=64 ? std::nullopt : v;};
                tick();check(view().phase==XeenCombatCastPhase::Preparing && *world.sessionState().journeyRandom()==cursor &&
                    party.roster.at(6).currentSp==25 && world.sessionState().actors()[9].hp==7,"M39 raw rejection yield published live prefix");
            }
            if(control=="projectile-retry")renderFault=true;
            tick();check(view().phase==XeenCombatCastPhase::Projectile && party.roster.at(6).currentSp==25 && world.sessionState().actors()[9].hp==0 && world.sessionState().accountedMonsters().count({23,9}),"M39 retry replayed complete lethal unit");
            const auto settled=*world.sessionState().journeyRandom();deny();tick();check(view().phase==XeenCombatCastPhase::Result && *world.sessionState().journeyRandom()==settled,"M39 projectile/result replayed RNG");
            renderFault=true;respond(AcknowledgeAction{});
            if(control=="arrow-successor-fail") {
                fight().setProbe([]{throw std::runtime_error("M39 independent owed End failure");});
                try{tick();}catch(const std::exception &){}
                check(fight().phase()==XeenCombatPhase::Failed && party.roster.at(6).currentSp==25 &&
                    world.sessionState().actors()[9].hp==0 && world.sessionState().accountedMonsters().count({23,9}) && party.monsterTreasure->pendingGold==10,
                    "M39 independent End failure rolled back cast lethal unit");
            }
            check(party.roster.at(6).currentSp==25 && *world.sessionState().journeyRandom()==settled,"M39 successor/report retry replayed effect");
        }
    }else {
        respond(CastSpellAction{});respond(NavigationAction::MoveBackward);respond(AcknowledgeAction{});
        if(control=="target-retry")renderFault=true;
        respond(AcknowledgeAction{});deny();check(view().phase==XeenCombatCastPhase::PartyTarget && party.roster.at(1).currentSp==20,"M39 target presentation repeated debit");
        if(control=="target-retry") {
            const auto old=flow.frame().presentation();const auto token=*handler.displayedInput();auto redraw=flow.refresh(true);
            check(!flow.acceptsFrame(old) && !flow.acceptsFrame(std::make_shared<const IndexedFrame>(redraw)),"M39 redraw accepted foreign/stale concrete frame");
            handler.withDisplayedInput(CancelInteractionAction{},token);check(party.roster.at(1).currentSp==20,"M39 unpresented redraw refunded");present(redraw);
        }
        if(control=="target-fail" || control=="refund-fail")fight().setProbe([]{throw std::runtime_error("M39 post-cost effect/refund failure");});
        if(control=="result-retry")renderFault=true;
        try{respond(control=="refund-fail" || control=="target-retry" ? PlayerAction{CancelInteractionAction{}} : PlayerAction{SelectMemberAction{5}});}catch(const std::exception &){}
        if(control=="target-fail" || control=="refund-fail") {
            check(fight().phase()==XeenCombatPhase::Failed && party.roster.at(1).currentSp==20 && party.roster.at(6).currentHp==priorHp && *world.sessionState().journeyRandom()==cursor,"M39 post-cost failure fabricated refund/effect");
        }else {
            check(view().phase==XeenCombatCastPhase::Result && party.roster.at(1).currentSp==(control=="target-retry"?21:20) && party.roster.at(6).currentHp==(control=="target-retry"?priorHp:15),"M39 presentation retry changed settled prefix");
            const auto old=fight().ticket();check(fight().command(old,XeenCombatCommand::Attack).status==XeenCombatStatus::Refused && fight().service(old).status==XeenCombatStatus::Refused,"M39 public ticket bypassed modal");
            const auto forged=std::make_shared<const IndexedFrame>(flow.frame());check(!flow.acceptsFrame(forged),"M39 same-pixel foreign frame admitted");
            deny();respond(AcknowledgeAction{});check(fight().participant()==5 && !fight().cast(),"M39 settlement consumed wrong action");
            check(fight().command(old,XeenCombatCommand::Run).status==XeenCombatStatus::Stale,"M39 old modal ticket released successor");
        }
    }
    resourceFault={};replay_test::filterDraw={};fight().setProbe({});deny();
    if(fight().phase()==XeenCombatPhase::Failed) {
        const auto t=fight().ticket();const auto revision=fight().result().revision;
        handler.withDisplayedInput(CancelInteractionAction{},flow.displayedInput().value_or(0));handler.withDisplayedInput(AcknowledgeAction{},flow.displayedInput().value_or(0));
        fight().service(t);fight().command(t,XeenCombatCommand::Attack);
        check(fight().phase()==XeenCombatPhase::Failed && fight().result().revision==revision,"M39 matching retry cleared integrity failure");
    }
    verifyDisk();std::cout<<"M39 ARTIFICIAL CONTROL PASSED "<<control<<'\n';return true;
}
#endif
