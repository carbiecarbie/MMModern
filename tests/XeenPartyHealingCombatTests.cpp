#include "XeenRegionalGameplayTestSupport.h"
#include "games/xeen/XeenLearnedSpellRules.h"
#include <iostream>
using namespace regional_gameplay_test;
namespace {
using R=XeenLearnedSpellRules;
void run(unsigned spell,unsigned target,unsigned targetSpeed,bool quick,bool cancel,bool tail=false) {
    Harness h;auto services=h.services();
    const auto configure=services.configureFlow;
    services.configureFlow=[&](auto &flow,const auto &camera) {
        configure(flow,camera);flow.drawDialogSprite=[](auto &,const char *,unsigned,int,int){};
    };
    auto saved=regional_combat_test::combatSnapshot(XeenCombatRandom(1));
    const auto owner=saved.activeRosterIds[0];auto &caster=saved.characters[owner];
    caster.characterClass=XeenCharacterClass::Cleric;caster.hasSpells=true;caster.currentSp=1000;caster.permanentLevel=7;
    caster.learnedSpells.emplace();caster.learnedSpells->fill(0);
    caster.currentSpell=*R::slotForSpell(XeenSpellCategory::Clerical,spell);caster.learnedSpells->at(caster.currentSpell)=1;caster.quickOption=1;
    saved.journey->treasure->gems=1000;
    for(unsigned i=0;i<6;++i) {
        const auto id=saved.activeRosterIds[i];auto &c=saved.characters[id];c.conditions.fill(0);c.currentHp=500;
        saved.journey->supplements[id].inputs.speed={int(60-i*5),0};
        for(auto *items:{&c.weapons,&c.armor,&c.accessories})for(auto &item:*items)item.frame=0;
    }
    auto &patient=saved.characters[saved.activeRosterIds[target]];
    saved.journey->supplements[patient.rosterId].inputs.speed={int(targetSpeed),0};
    if(spell==52){patient.permanentLevel=30;patient.currentHp=1;}
    saved.food=65535;
    if(spell==1 || spell==8 || tail)patient.conditions[spell==8?11:8]=1;
    const auto statistics=services.resources.loadMonsterStatistics;
    services.resources.loadMonsterStatistics=[=]{auto values=statistics();for(auto &m:values)m.raw[23]=tail?10:1;return values;};
    services.show=[&](const auto &,const auto &handler,const auto &,const auto &idle,const auto &) {
        const auto tick=[&] {h.tick(handler,idle);if(handler.completeInputHandoff)handler.completeInputHandoff(h.flow->frame().presentation());};
        const auto send=[&](PlayerAction action) {h.press(handler,action);if(handler.completeInputHandoff)handler.completeInputHandoff(h.flow->frame().presentation());};
        const auto ready=[&] {
            for(unsigned n=0;n<1000;++n) {
                if(!h.retired() && h.fight().phase()==Phase::PlayerReady)return;
                tick();
            }
            throw std::runtime_error("M75 combat did not reach a player turn");
        };
        send(WaitAction{});ready();check(h.fight().participant()==0,"M75 caster initiative fixture");
        if(tail) {
            for(unsigned i=0;i<5u;++i){check(h.fight().phase()==Phase::PlayerReady,"Missing party action");send(BlockAction{});}
            const auto minute=h.party->encounterContext->minutes;
            check(h.fight().phase()!=Phase::PlayerReady,"Skipped last party slot offered an extra action");
            ready();check(h.party->encounterContext->minutes==minute+1 && h.fight().participant()==0,"Skipped tail failed to rebuild the round without input");
            return true;
        }
        const auto minute=h.party->encounterContext->minutes;
        const auto cost=R::cost(spell,7);const auto rng=h.fight().random();
        send(quick?PlayerAction{QuickFightAction{}}:PlayerAction{CastSpellAction{}});
        if(h.fight().cast()->phase==XeenCombatCastPhase::Confirm)send(DialogKeyAction{'c'});
        check(h.fight().cast() && h.fight().cast()->committed && h.fight().cast()->result.spell==spell,"Wrong spell ID at paid combat admission");
        if(R::target(spell)==XeenSpellTarget::Member) {
            check(h.fight().cast()->phase==XeenCombatCastPhase::PartyTarget,"Member prompt missing");
            send(cancel?PlayerAction{CancelInteractionAction{}}:PlayerAction{SelectMemberAction{target}});
        }
        for(unsigned n=0;n<1000 && h.fight().cast();++n)tick();
        if(h.fight().cast())throw std::runtime_error("M75 combat effect did not settle: spell="+std::to_string(spell)+" quick="+std::to_string(quick)+" phase="+std::to_string(unsigned(h.fight().cast()->phase)));
        check(h.party->roster.at(owner).currentSp==1000-int(cancel?0:cost.sp) && h.party->monsterTreasure->gems==1000-(cancel?0:cost.gems),"Combat cancellation did not refund its own SP and gems");
        if(cancel){check(h.fight().random()==rng,"Cancelled Power Cure drew");return true;}
        if(spell==52) {
            auto expected=rng;std::optional<std::uint32_t> amount;while(!amount)amount=expected.draw(2,12);
            check(h.party->roster.at(patient.rosterId).currentHp==1+int(*amount*7) && h.fight().random()==expected,"Combat Power Cure draw/level source");
        }
        if(spell==6)check(h.party->food==5,"Combat Create Food prompt/wrap");
        if(spell!=1 && spell!=8)return true;
        const unsigned condition=spell==8?11:8;
        check(!h.party->roster.at(patient.rosterId).conditions[condition],"Combat cure did not re-enable target");
        if(targetSpeed<60)check(h.fight().participant()==int(target) && h.party->encounterContext->minutes==minute,"Member cured before its slot lost this round");
        else {
            for(unsigned n=0;n<1000 && h.party->encounterContext->minutes==minute;++n) {
                check(h.fight().participant()!=int(target) || h.fight().phase()!=Phase::PlayerReady,"Member cured after skip acted in the same round");
                if(h.fight().phase()==Phase::PlayerReady)send(BlockAction{});else tick();
            }
            ready();check(h.fight().participant()==int(target),"Skipped member failed to return next round");
        }
        return true;
    };
    const auto directory=child_test::freshDirectory(std::filesystem::current_path()/"m75-combat");h.runDirectories.push_back(directory);
    const auto path=directory/"fixture.mmsave";XeenSaveFile::write(path,saved);
    check(Application().playGameplay(services,{},path,true)==0,"M75 synthetic combat flow failed");
}
}
int main() {try {
    for(unsigned spell:{1u,8u})for(bool quick:{false,true})for(unsigned speed:{55u,70u})run(spell,1,speed,quick,false);
    run(1,5,2,false,false,true);run(8,5,2,false,false,true);
    for(unsigned spell:{7u,8u,9u,49u,52u,64u,67u,68u})for(bool quick:{false,true})run(spell,1,55,quick,true);
    for(bool quick:{false,true}){run(52,1,55,quick,false);run(6,1,55,quick,false);}
    std::cout<<"M75 turn order, HP admission, book/Quick Fight prompts and refunds passed\n";return 0;
}catch(const std::exception &e){std::cerr<<e.what()<<'\n';return 1;}}
