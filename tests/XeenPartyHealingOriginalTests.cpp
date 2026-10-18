#include "XeenTrainingTestSupport.h"
#include "XeenTestInstallation.h"
#include "games/xeen/XeenItemServiceRules.h"
#include <iostream>
#include <sstream>
using namespace training_test;
namespace {
using R=XeenLearnedSpellRules;
std::string scenario;
void tick(Fixture &f) {f.now+=125;f.flow->beginCycle(++f.cycle);if(auto next=f.flow->updatePresentation())f.present(*next);}
void quiet(Fixture &f) {
    for(unsigned n=0;n<1500 && !f.flow->canSave();++n) {
        const auto input=f.flow->inputContext(f.flow->frame().presentation());
        if(f.flow->presentationGeneration() || f.w.sessionState().journeyActivity()==XeenJourneyActivity::Reward || (input.dialog && input.dialog->anyKey))f.act(AcknowledgeAction{});
        tick(f);
    }
    if(!f.flow->canSave())throw std::runtime_error("M75 cast/item did not settle: "+scenario+" activity="+std::to_string(unsigned(f.w.sessionState().journeyActivity()))+" casting="+std::to_string(f.flow->encounter()->castingCommitted())+" countdown="+std::to_string(f.flow->encounter()->state().pending())+" feedback="+std::to_string(f.w.scenePresentation().feedbackActive()));
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
void matches(const XeenPartyState &party,const XeenSpellPreparation &effect) {
    for(const auto &value:effect.effects) {
        const auto &c=party.roster.at(value.owner);
        check(c.currentHp==value.hp && c.conditions==value.conditions,"Shared healing HP/conditions publication");
        check((!value.permanentEndurance || c.endurance.permanent==*value.permanentEndurance) &&
            (!value.temporaryAge || c.temporaryAge==*value.temporaryAge),"Shared healing Endurance/age publication");
    }
}
void books(Inputs &in) {
    for(unsigned spell:{6u,7u,8u,9u,16u,49u,52u,56u,58u,64u,67u,68u})for(bool cancel:{false,true}) {
        const bool member=R::target(spell)==XeenSpellTarget::Member;
        if(!member && cancel)continue;
        scenario="book spell="+std::to_string(spell)+" cancel="+std::to_string(cancel);
        auto s=source(in,spell);auto &target=s.characters[s.activeRosterIds[1]];
        target.currentHp=1;
        if(!cancel) {
            if(spell==9 || spell==68)target.conditions[3]=7;
            if(spell==7 || spell==67)target.conditions[4]=7;
            if(spell==8)target.conditions[11]=1;
        }
        if(spell==64 && !cancel){target.conditions[14]=1;target.conditions[12]=1;target.currentHp=0;}
        if(spell==56 || spell==58){target.conditions[spell==56?13:15]=1;target.currentHp=0;target.temporaryAge=248;target.endurance.permanent=18;}
        if(spell==16) {
            s.characters[s.activeRosterIds[0]].temporaryAge=248;
            target.conditions.fill(1);target.currentHp=-3;
            s.characters[s.activeRosterIds[2]].conditions[13]=1;
        }
        s.food=65535;Fixture f(in,s);
        XeenCombatRandom expected(*f.w.sessionState().journeyRandom());
        XeenSpellPreparation effect;
        if(!cancel) {
            if(spell==52){XeenConsequenceDraw draw{expected};effect=*R::preparePowerCure(f.p,1,610,7,draw);}
            else effect=R::prepareEffect(*R::supported(spell),f.p,member?std::optional<std::size_t>{1}:std::nullopt,610,s.activeRosterIds[0]);
        }
        const auto cost=R::cost(spell,7);
        f.act(CastSpellAction{});f.act(SelectMemberAction{0});f.act(DialogKeyAction{'c'});
        check(f.flow->encounter()->castingCommitted() && f.p.roster.at(s.activeRosterIds[0]).currentSp==1000-int(cost.sp) && f.p.monsterTreasure->gems==1000-cost.gems,"Exploration paid own spell cost");
        if(member)f.act(cancel?PlayerAction{CancelInteractionAction{}}:PlayerAction{SelectMemberAction{1}});
        quiet(f);
        check(f.p.roster.at(s.activeRosterIds[0]).currentSp==1000-int(cancel?0:cost.sp) && f.p.monsterTreasure->gems==1000-(cancel?0:cost.gems),"Exploration cancellation SP/gems refund");
        if(f.p.encounterContext->minutes!=601 || f.w.sessionState().journeyRandom()!=expected.continuation())
            throw std::runtime_error("Exploration healing spell="+std::to_string(spell)+" cancel="+std::to_string(cancel)+" minute="+std::to_string(f.p.encounterContext->minutes)+" RNG="+std::to_string(f.w.sessionState().journeyRandom()->count)+" expected="+std::to_string(expected.position()));
        matches(f.p,effect);
        if(cancel && (spell==56 || spell==58)) {
            const auto &after=f.p.roster.at(target.rosterId);
            check(after.currentHp==target.currentHp && after.conditions==target.conditions && after.endurance.permanent==18 && after.temporaryAge==248,"Cancelled revival changed its target");
        }
        if(spell==6)check(f.p.food==5,"Create Food did not wrap without a prompt");
        roundtrip(in,f);
    }
}
void items(Inputs &in) {
    for(unsigned power:{26u,27u,37u,41u,44u,51u,56u,57u,66u,73u})for(bool cancel:{false,true})for(unsigned charges:{1u,2u})for(unsigned userIndex:{0u,2u}) {
        if(power<57 && (charges==1 || userIndex==2))continue;
        const auto spell=*xeenItemPowerSpellId(power);auto s=source(in,spell);
        const bool member=R::target(spell)==XeenSpellTarget::Member;
        if(!member && cancel)continue;
        scenario="item power="+std::to_string(power)+" cancel="+std::to_string(cancel)+" charges="+std::to_string(charges)+" user="+std::to_string(userIndex);
        const auto user=s.activeRosterIds[userIndex];auto &c=s.characters[user];
        c.characterClass=XeenCharacterClass::Knight;c.hasSpells=false;c.learnedSpells.emplace();c.learnedSpells->fill(0);c.currentSp=3;
        c.permanentLevel=7;c.temporaryAge=248;
        c.miscellaneous={};c.miscellaneous[0]={1,static_cast<std::uint8_t>(power),static_cast<std::uint8_t>(charges),0};
        auto &target=s.characters[s.activeRosterIds[1]];target.currentHp=1;
        if(!cancel) {
            if(spell==9)target.conditions[3]=7;
            if(spell==7)target.conditions[4]=7;
            if(spell==8)target.conditions[11]=1;
        }
        if(power==56 && !cancel){target.currentHp=0;target.conditions[14]=1;target.conditions[12]=1;}
        if(power==57 || power==66){target.currentHp=0;target.conditions[power==57?13:15]=1;target.temporaryAge=248;target.endurance.permanent=18;}
        if(power==73){target.currentHp=-3;target.conditions.fill(1);s.characters[s.activeRosterIds[3]].conditions[13]=1;}
        s.food=65535;Fixture f(in,s);XeenCombatRandom expected(*f.w.sessionState().journeyRandom());
        XeenSpellPreparation effect;
        if(!cancel) {
            if(power==26){XeenConsequenceDraw draw{expected};effect=*R::preparePowerCure(f.p,1,610,7,draw);}
            else effect=R::prepareEffect(*R::supported(spell),f.p,member?std::optional<std::size_t>{1}:std::nullopt,610,user);
        }
        f.act(SelectMemberAction{userIndex});for(unsigned key:{unsigned('i'),unsigned('m'),unsigned('1'),unsigned('u')})f.act(DialogKeyAction{key});
        check(f.p.roster.at(user).miscellaneous[0].state==charges-1,"Item charge was not spent before target/cancel");
        if(member)f.act(cancel?PlayerAction{CancelInteractionAction{}}:PlayerAction{SelectMemberAction{1}});
        quiet(f);const auto refund=R::cost(spell,7);
        check(f.p.roster.at(user).currentSp==3+int(cancel?refund.sp:0) && f.p.monsterTreasure->gems==1000+(cancel?refund.gems:0),"Item own-ID cancellation refund/level source");
        check(f.p.roster.at(user).miscellaneous[0].state==charges-1 && bool(f.p.roster.at(user).miscellaneous[0].id)==(charges>1) && f.p.encounterContext->minutes==600 && f.w.sessionState().journeyRandom()==expected.continuation(),"Item repeated charge, last-charge removal, charged time or wrong RNG");
        matches(f.p,effect);
        if(cancel && (power==57 || power==66)) {
            const auto &after=f.p.roster.at(target.rosterId);
            check(after.currentHp==target.currentHp && after.conditions==target.conditions && after.endurance.permanent==18 && after.temporaryAge==248,"Cancelled revival item changed its target");
        }
        if(power==51)check(f.p.food==5,"Item Create Food wrap");
        roundtrip(in,f);
    }
}
void failedRevival(Inputs &in) {
    for(unsigned spell:{56u,58u}) {
        scenario="failed revival spell="+std::to_string(spell);
        auto s=source(in,spell);const auto owner=s.activeRosterIds[1];
        const auto &before=s.characters[owner];const auto cost=R::cost(spell,7);
        Fixture f(in,s);
        f.act(CastSpellAction{});f.act(SelectMemberAction{0});f.act(DialogKeyAction{'c'});f.act(SelectMemberAction{1});
        quiet(f);
        const auto &after=f.p.roster.at(owner);
        check(f.p.roster.at(s.activeRosterIds[0]).currentSp==1000-int(cost.sp) && f.p.monsterTreasure->gems==1000-cost.gems,"Failed revival refunded its cost");
        check(after.currentHp==before.currentHp && after.conditions==before.conditions && after.endurance.permanent==before.endurance.permanent && after.temporaryAge==before.temporaryAge,"Failed revival changed a living target");
        roundtrip(in,f);
    }
}
void combat(Inputs &in) {
    for(unsigned spell:{8u,56u,58u})for(bool before:{false,true}) {
        const unsigned condition=spell==8?11:spell==56?13:15;
        auto s=source(in,spell,true);s.characters[s.activeRosterIds[1]].conditions[condition]=255;
        s.journey->supplements[s.activeRosterIds[1]].inputs.temporaryAc=250;
        if(spell!=8)s.characters[s.activeRosterIds[1]].currentHp=0;
        for(unsigned i=0;i<6;++i)s.journey->supplements[s.activeRosterIds[i]].inputs.speed={int(180-i*10),0};
        s.journey->supplements[s.activeRosterIds[1]].inputs.speed={before?170:200,0};
        auto &enemy=s.map(23).monsters->slots[5];enemy.x=8;enemy.y=11;enemy.lifecycle=XeenActorLifecycle::Present;enemy.accounted=false;
        Fixture f(in,s);f.act(WaitAction{});
        for(unsigned n=0;n<1000 && (!f.flow->encounter()->combat() || f.flow->encounter()->combat()->phase()!=XeenCombatPhase::PlayerReady);++n)tick(f);
        const auto fight=[&]()->const XeenCombat &{return *f.flow->encounter()->combat();};
        check(fight().participant()==0,"Original mainland caster turn");const auto minute=f.p.encounterContext->minutes;
        f.act(CastSpellAction{});f.act(DialogKeyAction{'c'});f.act(SelectMemberAction{1});
        for(unsigned n=0;n<1000 && fight().cast();++n)tick(f);
        check(!f.p.roster.at(s.activeRosterIds[1]).conditions[condition],"Original combat cure/revival");
        if(before) {
            check(fight().participant()==1 && f.p.encounterContext->minutes==minute,"Original cured member missed its upcoming slot");
            for(unsigned n=0;n<1000 && f.w.scenePresentation().feedbackActive();++n)tick(f);
            f.act(BlockAction{});
            for(unsigned n=0;n<1000 && fight().phase()!=XeenCombatPhase::PlayerReady;++n)tick(f);
            check(fight().participant()!=1,"Original raised member command admission failed");
        }
        else {
            for(unsigned n=0;n<1500 && f.p.encounterContext->minutes==minute;++n) {
                check(fight().participant()!=1 || fight().phase()!=XeenCombatPhase::PlayerReady,"Original cured member acted after its skipped slot");
                if(fight().phase()==XeenCombatPhase::PlayerReady && !f.w.scenePresentation().feedbackActive())f.act(BlockAction{});else tick(f);
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
    try{books(in);items(in);failedRevival(in);combat(in);}catch(...){std::cout.rdbuf(old);throw;}
    std::cout.rdbuf(old);std::cout<<"M75 original Vertigo cures, mainland turn timing, charged items and exact restore passed\n";return 0;
}catch(const std::exception &e){std::cerr<<e.what()<<'\n';return 1;}}
