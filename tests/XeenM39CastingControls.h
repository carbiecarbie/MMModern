// Explicit headless presentation and artificial faults. This is separate from
// the original native A/B/C production witnesses; it never mints CastResponse.
#ifndef MMODERN_M39_CAST_CONTROLS_H
#define MMODERN_M39_CAST_CONTROLS_H
#include "games/xeen/XeenRestoreGuard.h"
#include "games/xeen/XeenOutdoorScene.h"
#include "games/xeen/XeenRegionalRules.h"
template<class Handler,class Idle,class Fault>
bool m39CastControls(const std::string &control,const IndexedFrame &first,const Handler &handler,const Idle &idle,
    XeenEventFlow &flow,XeenWorld &world,const XeenPartyState &party,const XeenCamera &camera,
    const XeenGameFlags &flags,const fs::path &target,std::uint64_t &now,std::uint64_t &cycle,
    unsigned &providers,unsigned &saves,std::function<void()> &resourceFault,bool &renderFault,Fault mutate) {
    const auto present=[&](const IndexedFrame &frame){check(flow.acceptsFrame(frame.presentation()),"M39 test presentation must be concrete and current");handler.framePresented(frame.presentation());handler.completeInputHandoff(frame.presentation());};
    const auto respond=[&](PlayerAction action){handler.beginCycle(++cycle);auto token=handler.displayedInput();check(bool(token),"M39 control requires presented input");auto next=handler.withDisplayedInput(action,*token);
        const auto before=providers,save=saves;
        if(flow.encounter()->combat()) {
        handler.withDisplayedInput(CastSpellAction{},*token);handler.withDisplayedInput(SaveGameAction{},*token);
        check(before==providers && save==saves,"M39 consumed test input reached providers");
        }
        if(next)present(*next);};
    const auto tick=[&]{now+=110;handler.beginCycle(++cycle);auto frame=idle();if(frame)present(*frame);};
    const auto fight=[&]()->XeenCombat &{check(flow.encounter() && flow.encounter()->combat(),"M39 artificial control lost combat");return const_cast<XeenCombat &>(*flow.encounter()->combat());};
    const auto view=[&]()->XeenCombatCastView{const auto observed=fight().cast();check(observed.has_value(),"M39 artificial control lost cast");return *observed;};
    const auto select=[&](unsigned row){respond(DialogKeyAction{'n'});respond(DialogKeyAction{'0'+row});respond(DialogKeyAction{'s'});};
    const auto castSelected=[&]{respond(DialogKeyAction{'c'});};
    const auto ready=[&](unsigned slot){for(unsigned i=0;i<500;++i){if(flow.encounter()->combat()) {
        check(fight().phase()!=XeenCombatPhase::Failed && fight().phase()!=XeenCombatPhase::Defeat,"M39 control prefix failed");
        if(fight().phase()==XeenCombatPhase::PlayerReady){if(fight().participant()==int(slot))return;respond(BlockAction{});continue;}
        }else if(world.sessionState().journeyActivity()==XeenJourneyActivity::Event || world.sessionState().journeyActivity()==XeenJourneyActivity::Reward)respond(AcknowledgeAction{});
        tick();}throw std::runtime_error("M39 control prefix bound");};
    const auto quiet=[&]{for(unsigned i=0;i<500 && !flow.canSave();++i)tick();check(flow.canSave(),"M39 artificial prefix quiet bound");};
    const auto finishPortraits=[&]{
        // Result publication precedes the original acquired portrait holds.
        // Drive their concrete handoffs without replaying the paid effect.
        const XeenRestoreGuard settled(world,party,camera,flags);
        for(unsigned i=0;i<500 && (world.scenePresentation().feedbackActive() || !handler.inputContext(flow.frame().presentation()).readyForAction);++i)tick();
        check(!world.scenePresentation().feedbackActive() && handler.inputContext(flow.frame().presentation()).readyForAction && settled.current(),
            "M39 result portrait hold changed settled state or never released input");
    };
    present(first);respond(SaveGameAction{});check(saves==3,"M39 control prior save pipeline");
    const auto disk=XeenSaveFormat::encode(XeenSaveFile::read(target));
    respond(NavigationAction::MoveForward);quiet();if(control.find("-group")==std::string::npos){respond(ShootAction{});quiet();}respond(NavigationAction::MoveForward);ready(control=="partial"?0:4);
    if(control=="partial") {
        respond(RunAction{});ready(4);check(!(fight().participants()&1u),"M39 partial control needs legitimate earlier escape");
        respond(CastSpellAction{});select(2);castSelected();
        respond(SelectMemberAction{5});check(view().phase==XeenCombatCastPhase::PartyTarget && party.roster.at(1).currentSp==20,"M39 First Aid accepted unused compact slot/refunded");
        respond(CancelInteractionAction{});check(view().result.refunded && party.roster.at(1).currentSp==21,"M39 partial explicit refund");
        respond(AcknowledgeAction{});ready(5);respond(CastSpellAction{});select(1);castSelected();tick();
        check(view().phase==XeenCombatCastPhase::Result && view().result.count==6 && party.roster.at(6).currentSp==26 && !(fight().participants()&1u),"M39 Awaken lost full active scope/restored escape");
        std::cout<<"M39 ARTIFICIAL CONTROL PASSED partial\n";return true;
    }
    const auto cursor=*world.sessionState().journeyRandom();const auto priorHp=int(party.roster.at(6).currentHp);
    if(control.rfind("m60-",0)==0) {
        const unsigned spell=unsigned(std::stoul(control.substr(4)));
        const bool member=spell==10 || spell==59;
        const bool group=control.find("-group")!=std::string::npos;
        const bool quick=control.find("-quick")!=std::string::npos;
        const unsigned casterIndex=(member || spell==27 || spell==50)?4:5;
        if(casterIndex==5) {respond(BlockAction{});ready(casterIndex);}
        const auto owner=party.party.activeRosterIds().at(casterIndex);
        const auto beforeSp=party.roster.at(owner).currentSp;
        const auto beforeGems=party.monsterTreasure->gems;
        const auto beforeRandom=*world.sessionState().journeyRandom();
        const auto monster=*fight().selectedTarget();
        const auto beforeActor=world.sessionState().actors().at(monster.recordIndex);
        const auto beforeMember=party.roster.at(0);
        const auto &caster=party.roster.at(owner);
        const auto price=XeenLearnedSpellRules::cost(std::uint8_t(spell),unsigned(std::max(caster.permanentLevel+caster.temporaryLevel,0)));
        auto expectedRandom=fight().random();int expectedDamage=0;
        const auto groupTargets=xeenSpellTargets(world,camera,world.sessionState().actors(),fight().selectedTarget(),group);
        std::array<int,3> groupHp{};unsigned groupCount=0;
        auto expectedTreasure=*party.monsterTreasure;
        const auto accounted=world.sessionState().accountedMonsters();std::set<XeenMonsterIdentity> expectedAccounted(accounted.begin(),accounted.end());
        std::array<std::uint32_t,6> beforeXp{};std::uint32_t xpPerMember=0;
        for(unsigned i=0;i<6;++i)beforeXp[i]=party.roster.combatInputs(party.party.activeRosterIds().at(i))->experience;
        if(!member) {
            for(unsigned i=0;i<groupTargets.size() && groupTargets[i];++i) {
                const auto &actor=world.sessionState().actors().at(groupTargets[i]->recordIndex);
                XeenSpellDamageCandidate candidate(caster.permanentLevel,caster.temporaryLevel,
                    actor.statistics->magicResistance(),actor.original.resourceId,
                    *XeenLearnedSpellRules::supported(std::uint8_t(spell)),actor.statistics->physicalResistance());
                XeenConsequenceDraw draws{expectedRandom};check(candidate.service(draws),"M60 expected bounded damage");
                if(!i)expectedDamage=candidate.damage;
                groupHp[i]=std::max(actor.hp-candidate.damage,0);++groupCount;
                if(!groupHp[i]) {
                    expectedAccounted.insert(actor.id);xpPerMember+=actor.statistics->experience()/6;
                    if(actor.original.resourceId==6) {
                        XeenMonsterDropCandidate drop(expectedTreasure,actor.id.recordIndex);
                        check(drop.service(draws),"M60 expected bounded drop");expectedTreasure=drop.treasure;
                    }
                }
            }
        }
        if(quick)respond(QuickFightAction{});else {respond(CastSpellAction{});castSelected();}
        check(view().committed && party.roster.at(owner).currentSp==beforeSp-int(price.sp) &&
            party.monsterTreasure->gems==beforeGems-price.gems,"M60 exact debit");
        check(!flow.canSave(),"M60 paid cast allowed Save");
        if(member)respond(SelectMemberAction{0});
        else for(unsigned i=0;i<100 && fight().cast() && view().phase!=XeenCombatCastPhase::Result;++i) {
            check(!flow.canSave() && fight().phase()!=XeenCombatPhase::Failed,"M60 projectile save/failure");tick();
        }
        check(quick ? !fight().cast() : view().phase==XeenCombatCastPhase::Result && !view().result.failed,"M60 effect result");
        if(member) {
            check(world.sessionState().journeyRandom()==beforeRandom,"M60 recovery consumed random");
            const auto &after=party.roster.at(0);
            if(spell==10)check(after.currentHp==std::min<int>(beforeMember.currentHp+15,XeenCharacterRules::maxHp(beforeMember,{party.encounterContext->year})),"M60 Cure Wounds healing");
            else check(after.conditions[2]==0 && after.currentHp==beforeMember.currentHp,"M60 Revitalize Weak repair");
        } else {
            const auto &after=world.sessionState().actors().at(monster.recordIndex);
            check(after.hp==std::max(beforeActor.hp-expectedDamage,0),"M60 exact single/group contact damage");
            check(world.sessionState().journeyRandom()==expectedRandom.continuation(),"M60 combat damage draw tape");
            if(group) {
                check(groupCount==3,"M60 grouped fixture needs three targets");
                for(unsigned i=0;i<groupCount;++i)check(world.sessionState().actors().at(groupTargets[i]->recordIndex).hp==groupHp[i],"M60 group target damage");
                check(world.sessionState().accountedMonsters()==expectedAccounted,"M60 grouped death publication");
                check(party.monsterTreasure->pendingGold==expectedTreasure.pendingGold,"M60 grouped loot publication");
                check(fight().participants()==0x3f,"M60 group XP fixture participants");
                for(unsigned i=0;i<6;++i)check(party.roster.combatInputs(party.party.activeRosterIds().at(i))->experience==beforeXp[i]+xpPerMember*(party.roster.at(party.party.activeRosterIds().at(i)).permanentLevel<15?2:1),"M60 grouped XP exactly once");
            }
        }
        const auto saveProviders=providers,saveCount=saves;handler.withDisplayedInput(SaveGameAction{},handler.displayedInput().value_or(0));
        check(!flow.canSave() && providers==saveProviders && saves==saveCount,"M60 result allowed Save/providers");
        std::cout<<"M39 ARTIFICIAL CONTROL PASSED "<<control<<'\n';return true;
    }
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
        respond(CastSpellAction{});respond(DialogKeyAction{'n'});
        if(control=="observation-selection") {
            auto observed=fight().cast();auto &&result=fight().result();poison(observed,result);
            // A corrupted/stale learned-list copy cannot switch the reserved caster or row.
            respond(DialogKeyAction{'2'});
            check(view().owner==1 && view().participant==4 && view().selection==1,"M39 detached selection redirected caster/spell");
        }else {
            respond(DialogKeyAction{unsigned(arrow?'3':control=="observation-awaken"?'1':'2')});
        }
        respond(DialogKeyAction{'s'});
        auto observed=fight().cast();auto &&result=fight().result();unsigned callbacks=0;
        if(control=="observation-payer" || control=="observation-target") {
            resourceFault=[&]{++callbacks;poison(observed,result);};
        }
        castSelected();resourceFault={};
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
            tick();check(view().phase==XeenCombatCastPhase::Projectile && world.sessionState().journeyRandom()==cursor,
                "Travel sampled Arrow damage before target arrival");
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
                    check(view().phase==XeenCombatCastPhase::Projectile && world.sessionState().actors()[9].hp==7 &&
                        world.sessionState().journeyRandom()==cursor && party.roster.at(6).currentSp==25,
                        "M39 corrupted yielded observation published incomplete unit");
                    tick();
                }
                check(callbacks>0 && view().phase==XeenCombatCastPhase::Impact && fight().result().actorHpBefore==7 &&
                    fight().result().actorHpAfter==0 && fight().result().damage==8 && world.sessionState().actors()[9].hp==7 &&
                    !world.sessionState().accountedMonsters().count({23,9}) && world.sessionState().journeyRandom()==cursor &&
                    world.sessionState().actors()[15].hp==other.hp && world.sessionState().actors()[15].id==other.id,
                    "M49 Arrow preparation published HP/reward/RNG before arrival");
                check(requests.at(0)==std::pair<unsigned,unsigned>{1,56} &&
                    requests.size()==(control=="observation-yield"?65u:1u),
                    "Arrow damage target changed or drop sampled before post-HP draw");
                for(unsigned i=0;i<6;++i)check(party.roster.combatInputs(kXeenCombatOwners[i])->experience==(i==1 || i==4?2000u:1000u),
                    "M49 Arrow credited XP during preparation");
                fight().setProbe({});
                auto projectile=fight().cast();auto &&projectileResult=fight().result();poison(projectile,projectileResult);
                check(flow.encounter()->appearance().projectile.has_value(),"M39 corrupted public phase hid retained projectile");
                const auto visuals=flow.encounter()->appearance().projectiles;
                check(visuals.size()==1 && visuals[0].lane==0 && visuals[0].row==1 && visuals[0].pow==11 &&
                    visuals[0].target==std::optional<XeenMonsterIdentity>{{23,9}},"Magic Arrow shared single lane/selected target");
                const auto laneAt=[&](unsigned row) {
                    const auto appearance=flow.encounter()->appearance();
                    check(appearance.projectiles.size()==1 && appearance.projectiles[0].row==row && flow.frame().isValid(),"Magic Arrow lane did not advance with its acquired impact stage");
                    const auto commands=XeenOutdoorScene().build(world,camera,nullptr,nullptr,{},appearance);
                    const auto lane=std::find_if(commands.begin(),commands.end(),[](const auto &c){return c.projectile()!=nullptr;});
                    check(lane!=commands.end() && lane->projectile()->row==row && lane->projectile()->pow==11 && lane->drawOptions().scaleIndex==4*row,
                        "Magic Arrow rendered row/depth/resource differs from retained cosmetic lane");
                };
                laneAt(1);
                check(view().phase==XeenCombatCastPhase::Impact && world.sessionState().actors()[9].hp==7 &&
                    flow.encounter()->appearance().frame==3 && world.sessionState().journeyRandom()==cursor &&
                    !world.sessionState().accountedMonsters().count({23,9}),"M49 hit frame missing before HP publication");
                deny();tick();laneAt(2);
                check(view().phase==XeenCombatCastPhase::PostImpact && world.sessionState().actors()[9].hp==0 &&
                    world.sessionState().actors()[9].lifecycle==XeenActorLifecycle::Present &&
                    flow.encounter()->appearance().impactSnapshot && !world.sessionState().accountedMonsters().count({23,9}) &&
                    party.monsterTreasure->pendingGold==0,"M49 post-HP frame removed/rewarded lethal actor");
                for(unsigned i=0;i<6;++i)check(party.roster.combatInputs(kXeenCombatOwners[i])->experience==(i==1 || i==4?2000u:1000u),"M49 post-HP frame credited XP");
                deny();tick();laneAt(3);const auto settled=*world.sessionState().journeyRandom();
                check(requests.back()==std::pair<unsigned,unsigned>{1,100},"Arrow drop omitted original post-HP call point");
                check(view().phase==XeenCombatCastPhase::Result && world.sessionState().accountedMonsters().count({23,9}) &&
                    world.sessionState().actors()[9].lifecycle==XeenActorLifecycle::Defeated && party.monsterTreasure->pendingGold==10,"M49 lethal accounting/removal did not settle once");
                for(unsigned i=0;i<6;++i)check(party.roster.combatInputs(kXeenCombatOwners[i])->experience==(i==1 || i==4?2066u:1066u),"M49 final Arrow XP recipients changed");
                auto receipt=fight().cast();auto &&receiptResult=fight().result();poison(receipt,receiptResult);
                tick();check(flow.encounter()->appearance().projectiles.empty(),"Magic Arrow endAttack did not clear the acquired lethal removal lane");
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
            finishPortraits();respond(AcknowledgeAction{});
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
        respond(CastSpellAction{});select(2);
        fight().setProbe([]{throw std::runtime_error("M39 pre-cost injected failure");});
        try{castSelected();}catch(const std::exception &){}
        check(fight().phase()==XeenCombatPhase::Casting && view().phase==XeenCombatCastPhase::Confirm && party.roster.at(1).currentSp==21,"M39 ordinary pre-cost failure lost unchanged confirmation");
        fight().setProbe({});respond(AcknowledgeAction{});castSelected();respond(CancelInteractionAction{});
        check(view().result.refunded && party.roster.at(1).currentSp==21,"M39 newly presented confirmation retry/refund failed");
    }else if(control.rfind("arrow-",0)==0 || control=="projectile-retry") {
        respond(BlockAction{});ready(5);respond(CastSpellAction{});select(3);castSelected();
        check(view().phase==XeenCombatCastPhase::Preparing && party.roster.at(6).currentSp==25 && world.sessionState().actors()[9].hp==7 && world.sessionState().journeyRandom()==cursor,"M39 Arrow debit/effect separation");
        if(control.rfind("arrow-fail-",0)==0 || control=="arrow-xp-overflow" || control=="arrow-drop-overflow") {
            tick();check(view().phase==XeenCombatCastPhase::Projectile,"Arrow failure fixture missing travel draw");
            const auto limit=(control=="arrow-xp-overflow" || control=="arrow-drop-overflow")?0:unsigned(std::stoul(control.substr(11)));auto calls=std::make_shared<unsigned>(0);
            fight().setProbe([calls,limit]{if(++*calls==limit)throw std::runtime_error("M39 Arrow draw/drop pre-store failure");});
            try{for(unsigned i=0;i<6 && fight().phase()!=XeenCombatPhase::Failed;++i)tick();}catch(const std::exception &){}
            check(fight().phase()==XeenCombatPhase::Failed && party.roster.at(6).currentSp==25 && world.sessionState().actors()[9].hp==7 &&
                world.sessionState().journeyRandom()==cursor && !world.sessionState().accountedMonsters().count({23,9}),"M39 Arrow incomplete unit published/refunded");
        }else {
            if(control=="projectile-retry")renderFault=true;
            tick();check(view().phase==XeenCombatCastPhase::Projectile && party.roster.at(6).currentSp==25 && world.sessionState().actors()[9].hp==7 && !world.sessionState().accountedMonsters().count({23,9}) && world.sessionState().journeyRandom()==cursor,"M49 retry published before arrival");
            if(control=="arrow-quit-travel") {deny();std::cout<<"M39 ARTIFICIAL CONTROL PASSED "<<control<<'\n';return true;}
            if(control=="arrow-yield") {
                auto calls=std::make_shared<unsigned>(0);replay_test::filterDraw=[calls](auto v)->std::optional<std::uint32_t>{return ++*calls<=64 ? std::nullopt : v;};
                tick();check(view().phase==XeenCombatCastPhase::Projectile && *world.sessionState().journeyRandom()==cursor &&
                    party.roster.at(6).currentSp==25 && world.sessionState().actors()[9].hp==7,"M39 raw rejection yield published live prefix");
            }
            deny();tick();
            check(view().phase==XeenCombatCastPhase::Impact && world.sessionState().actors()[9].hp==7,"M49 impact before HP boundary");
            deny();if(control=="projectile-retry")renderFault=true;
            tick();check(view().phase==XeenCombatCastPhase::PostImpact && world.sessionState().actors()[9].hp==0 && !world.sessionState().accountedMonsters().count({23,9}),"M49 HP/removal boundary");
            deny();if(control=="projectile-retry")renderFault=true;
            tick();const auto settled=*world.sessionState().journeyRandom();check(view().phase==XeenCombatCastPhase::Result && world.sessionState().accountedMonsters().count({23,9}),"M49 final lethal result missing");
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
        respond(CastSpellAction{});select(2);
        if(control=="target-retry")renderFault=true;
        castSelected();deny();check(view().phase==XeenCombatCastPhase::PartyTarget && party.roster.at(1).currentSp==20,"M39 target presentation repeated debit");
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
            deny();
            finishPortraits();
            respond(AcknowledgeAction{});check(fight().participant()==5 && !fight().cast(),"M39 settlement consumed wrong action");
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
