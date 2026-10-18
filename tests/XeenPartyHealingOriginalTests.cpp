#include "XeenTrainingTestSupport.h"
#include "XeenTestInstallation.h"
#include "games/xeen/XeenItemServiceRules.h"
#include <iostream>
#include <sstream>
using namespace training_test;
namespace {
using R=XeenLearnedSpellRules;
void tick(Fixture &f) {f.now+=125;f.flow->beginCycle(++f.cycle);if(auto next=f.flow->updatePresentation())f.present(*next);}
void quiet(Fixture &f) {
    for(unsigned n=0;n<1500 && !f.flow->canSave();++n) {
        const auto input=f.flow->inputContext(f.flow->frame().presentation());
        if(f.flow->presentationGeneration() || f.w.sessionState().journeyActivity()==XeenJourneyActivity::Reward || (input.dialog && input.dialog->anyKey))f.act(AcknowledgeAction{});
        else tick(f);
    }
    check(f.flow->canSave(),"M75 cast/item did not settle");
}
XeenSaveSnapshot source(Inputs &in,unsigned spell,bool combat=false) {
    auto s=in.service();s.camera=combat?XeenCamera{23,9,11,XeenDirection::West}:XeenCamera{28,18,4,XeenDirection::West};
    for(auto *actors:{&s.map(23).monsters->slots,&s.map(28).monsters->slots})for(auto &a:*actors) {
        a.x=a.y=-128;a.lifecycle=XeenActorLifecycle::Defeated;a.accounted=true;
    }
    for(auto id:s.activeRosterIds) {
        auto &c=s.characters[id];c.skills[4]=0;c.conditions.fill(0);c.currentHp=combat?1000:10;
        for(auto *items:{&c.weapons,&c.armor,&c.accessories})for(auto &item:*items)item.frame=0;
    }
    auto &caster=s.characters[s.activeRosterIds[0]];caster.characterClass=XeenCharacterClass::Cleric;
    caster.hasSpells=true;caster.permanentLevel=7;caster.temporaryLevel=0;caster.currentSp=1000;
    caster.learnedSpells.emplace();caster.learnedSpells->fill(0);
    caster.currentSpell=*R::slotForSpell(XeenSpellCategory::Clerical,spell);caster.learnedSpells->at(caster.currentSpell)=1;
    s.characters[s.activeRosterIds[1]].permanentLevel=30;
    s.journey->treasure->gems=1000;s.journey->context->ctr24=23;s.journey->context->minutes=600;
    save_test::clearExplored(s);return s;
}
void roundtrip(Inputs &in,Fixture &f) {
    const auto bytes=XeenSaveFormat::encode(f.snapshot());Fixture restored(in,XeenSaveFormat::decode(bytes));
    check(bytes==XeenSaveFormat::encode(restored.snapshot()),"M75 save/load replayed effects/time/RNG/refunds");
}
void books(Inputs &in) {
    for(unsigned spell:{6u,7u,8u,9u,49u,52u,64u,67u,68u})for(bool cancel:{false,true}) {
        if(spell==6 && cancel)continue;
        auto s=source(in,spell);auto &target=s.characters[s.activeRosterIds[1]];
        target.currentHp=1;
        if(!cancel) {
            if(spell==9 || spell==68)target.conditions[3]=7;
            if(spell==7 || spell==67)target.conditions[4]=7;
            if(spell==8)target.conditions[11]=1;
        }
        if(spell==64 && !cancel){target.conditions[14]=1;target.conditions[12]=1;target.currentHp=0;}
        s.food=65535;Fixture f(in,s);
        XeenCombatRandom expected(*f.w.sessionState().journeyRandom());
        XeenSpellPreparation effect;
        if(!cancel) {
            if(spell==52){XeenConsequenceDraw draw{expected};effect=*R::preparePowerCure(f.p,1,610,7,draw);}
            else effect=R::prepareEffect(*R::supported(spell),f.p,spell==6?std::nullopt:std::optional<std::size_t>{1},610);
        }
        const auto cost=R::cost(spell,7);
        f.act(CastSpellAction{});f.act(SelectMemberAction{0});f.act(DialogKeyAction{'c'});
        check(f.flow->encounter()->castingCommitted() && f.p.roster.at(s.activeRosterIds[0]).currentSp==1000-int(cost.sp) && f.p.monsterTreasure->gems==1000-cost.gems,"Exploration paid own spell cost");
        if(spell!=6)f.act(cancel?PlayerAction{CancelInteractionAction{}}:PlayerAction{SelectMemberAction{1}});
        quiet(f);
        check(f.p.roster.at(s.activeRosterIds[0]).currentSp==1000-int(cancel?0:cost.sp) && f.p.monsterTreasure->gems==1000-(cancel?0:cost.gems),"Exploration cancellation SP/gems refund");
        if(f.p.encounterContext->minutes!=601 || f.w.sessionState().journeyRandom()!=expected.continuation())
            throw std::runtime_error("Exploration healing spell="+std::to_string(spell)+" cancel="+std::to_string(cancel)+" minute="+std::to_string(f.p.encounterContext->minutes)+" RNG="+std::to_string(f.w.sessionState().journeyRandom()->count)+" expected="+std::to_string(expected.position()));
        for(const auto &value:effect.effects)check(f.p.roster.at(value.owner).currentHp==value.hp && f.p.roster.at(value.owner).conditions==value.conditions,"Exploration cure result");
        if(spell==6)check(f.p.food==5,"Create Food did not wrap without a prompt");
        roundtrip(in,f);
    }
}
void items(Inputs &in) {
    for(unsigned power:{26u,27u,37u,41u,44u,51u,56u})for(bool cancel:{false,true}) {
        if(power==51 && cancel)continue;
        const auto spell=*xeenItemPowerSpellId(power);auto s=source(in,spell);
        const auto user=s.activeRosterIds[0];auto &c=s.characters[user];
        c.characterClass=XeenCharacterClass::Knight;c.hasSpells=false;c.learnedSpells->fill(0);c.currentSp=3;
        c.miscellaneous={};c.miscellaneous[0]={1,static_cast<std::uint8_t>(power),2,0};
        auto &target=s.characters[s.activeRosterIds[1]];target.currentHp=1;
        if(!cancel) {
            if(spell==9)target.conditions[3]=7;
            if(spell==7)target.conditions[4]=7;
            if(spell==8)target.conditions[11]=1;
        }
        if(power==56 && !cancel){target.currentHp=0;target.conditions[14]=1;target.conditions[12]=1;}
        s.food=65535;Fixture f(in,s);XeenCombatRandom expected(*f.w.sessionState().journeyRandom());
        XeenSpellPreparation effect;
        if(!cancel) {
            if(power==26){XeenConsequenceDraw draw{expected};effect=*R::preparePowerCure(f.p,1,610,7,draw);}
            else effect=R::prepareEffect(*R::supported(spell),f.p,power==51?std::nullopt:std::optional<std::size_t>{1},610);
        }
        f.act(SelectMemberAction{0});for(unsigned key:{unsigned('i'),unsigned('m'),unsigned('1'),unsigned('u')})f.act(DialogKeyAction{key});
        check(f.p.roster.at(user).miscellaneous[0].state==1,"Item charge was not spent before target/cancel");
        if(power!=51)f.act(cancel?PlayerAction{CancelInteractionAction{}}:PlayerAction{SelectMemberAction{1}});
        quiet(f);const auto refund=R::cost(spell,7);
        check(f.p.roster.at(user).currentSp==3+int(cancel?refund.sp:0) && f.p.monsterTreasure->gems==1000+(cancel?refund.gems:0),"Item own-ID cancellation refund/level source");
        check(f.p.roster.at(user).miscellaneous[0].state==1 && f.p.encounterContext->minutes==600 && f.w.sessionState().journeyRandom()==expected.continuation(),"Item repeated charge, charged time or wrong Power Cure RNG");
        for(const auto &value:effect.effects)check(f.p.roster.at(value.owner).currentHp==value.hp && f.p.roster.at(value.owner).conditions==value.conditions,"Item shared healing result");
        if(power==51)check(f.p.food==5,"Item Create Food wrap");
        roundtrip(in,f);
    }
    for(unsigned power:{57u,66u,73u})check(!xeenItemUseSupported({1,static_cast<std::uint8_t>(power),3,0}),"Part B item power admitted");
}
void combat(Inputs &in) {
    for(bool before:{false,true}) {
        auto s=source(in,8,true);s.characters[s.activeRosterIds[1]].conditions[11]=255;
        for(unsigned i=0;i<6;++i)s.journey->supplements[s.activeRosterIds[i]].inputs.speed={int(180-i*10),0};
        s.journey->supplements[s.activeRosterIds[1]].inputs.speed={before?170:200,0};
        auto &enemy=s.map(23).monsters->slots[5];enemy.x=8;enemy.y=11;enemy.lifecycle=XeenActorLifecycle::Present;enemy.accounted=false;
        Fixture f(in,s);f.act(WaitAction{});
        for(unsigned n=0;n<1000 && (!f.flow->encounter()->combat() || f.flow->encounter()->combat()->phase()!=XeenCombatPhase::PlayerReady);++n)tick(f);
        const auto fight=[&]()->const XeenCombat &{return *f.flow->encounter()->combat();};
        check(fight().participant()==0,"Original mainland caster turn");const auto minute=f.p.encounterContext->minutes;
        f.act(CastSpellAction{});f.act(DialogKeyAction{'c'});f.act(SelectMemberAction{1});
        for(unsigned n=0;n<1000 && fight().cast();++n)tick(f);
        check(!f.p.roster.at(s.activeRosterIds[1]).conditions[11],"Original combat Cure Paralysis");
        if(before)check(fight().participant()==1 && f.p.encounterContext->minutes==minute,"Original cured member missed its upcoming slot");
        else {
            for(unsigned n=0;n<1500 && f.p.encounterContext->minutes==minute;++n) {
                check(fight().participant()!=1 || fight().phase()!=XeenCombatPhase::PlayerReady,"Original cured member acted after its skipped slot");
                if(fight().phase()==XeenCombatPhase::PlayerReady)f.act(BlockAction{});else tick(f);
            }
            for(unsigned n=0;n<1000 && fight().phase()!=XeenCombatPhase::PlayerReady;++n)tick(f);
            check(fight().participant()==1,"Original cured member did not act next round");
        }
    }
}
}
int main(int argc,char **argv) {try {
    check(argc==2,"usage: party-healing-original <installation>");auto installation=xeenTestInstallationDetector().detect(argv[1]);check(bool(installation),"Missing installation");Inputs in(*installation);
    std::ostringstream captured;auto *old=std::cout.rdbuf(captured.rdbuf());
    try{books(in);items(in);combat(in);}catch(...){std::cout.rdbuf(old);throw;}
    std::cout.rdbuf(old);std::cout<<"M75 original Vertigo cures, mainland turn timing, charged items and exact restore passed\n";return 0;
}catch(const std::exception &e){std::cerr<<e.what()<<'\n';return 1;}}
