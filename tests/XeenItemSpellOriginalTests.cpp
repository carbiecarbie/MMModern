#include "XeenTrainingTestSupport.h"
#include "XeenTestInstallation.h"
#include "games/xeen/XeenItemServiceRules.h"
#include <iostream>
#include <sstream>
using namespace training_test;
namespace {
using R=XeenLearnedSpellRules;using S=XeenLearnedSpell;using C=XeenInventoryCategory;
std::string scenario;
void tick(Fixture &f) {f.now+=125;f.flow->beginCycle(++f.cycle);if(auto next=f.flow->updatePresentation())f.present(*next);}
void quiet(Fixture &f) {
    for(unsigned n=0;n<1500 && !f.flow->canSave();++n) {
        const auto input=f.flow->inputContext(f.flow->frame().presentation());
        if(input.dialog && input.dialog->anyKey)f.act(AcknowledgeAction{});
        tick(f);
    }
    if(!f.flow->canSave())throw std::runtime_error("Item spell did not settle: "+scenario);
}
void key(Fixture &f,unsigned value,bool mouse=false) {
    const auto input=f.flow->inputContext(f.flow->frame().presentation());
    check(input.dialog!=nullptr,"Item spell dialog input missing");
    std::optional<PlayerAction> action;
    if(mouse) {for(const auto &hit:input.dialog->hits)if(hit.key==value) {action=input.dialog->click((hit.left+hit.right)/2,(hit.top+hit.bottom)/2);break;}}
    if(!mouse)action=input.dialog->key(value);
    if(!action)throw std::runtime_error("Missing item-spell key/hit "+std::to_string(value)+": "+scenario);
    f.act(*action);
}
XeenSaveSnapshot source(Inputs &in,unsigned spell,bool combat=false) {
    auto s=in.service();s.camera=combat?XeenCamera{23,9,11,XeenDirection::West}:XeenCamera{28,18,4,XeenDirection::West};
    for(auto *actors:{&s.map(23).monsters->slots,&s.map(28).monsters->slots})for(auto &a:*actors) {a.x=a.y=-128;a.lifecycle=XeenActorLifecycle::Defeated;a.accounted=true;}
    for(auto id:s.activeRosterIds) {
        auto &c=s.characters[id];c.conditions.fill(0);c.currentHp=1000;
        s.journey->supplements[id].inputs.speed={int(180-id),0};
        c.weapons={};c.armor={};c.accessories={};c.miscellaneous={};
    }
    auto &caster=s.characters[s.activeRosterIds[0]];
    caster.characterClass=XeenCharacterClass::Sorcerer;caster.hasSpells=true;
    caster.permanentLevel=10;caster.temporaryLevel=2;caster.currentSp=1000;caster.quickOption=1;
    caster.learnedSpells.emplace();caster.learnedSpells->fill(0);
    caster.currentSpell=*R::slotForSpell(XeenSpellCategory::Wizardry,spell);caster.learnedSpells->at(caster.currentSpell)=1;
    s.journey->supplements[caster.rosterId].inputs.speed={250,0};
    auto &recipient=s.characters[s.activeRosterIds[1]];recipient.permanentLevel=1;recipient.temporaryLevel=0;
    recipient.weapons[0]={0,1,0,1};recipient.weapons[1]={0,2,0,0};
    recipient.armor[0]={0,1,0,0};recipient.miscellaneous[0]={1,4,2,0};
    s.journey->purse->gems=1000;s.journey->context->ctr24=23;s.journey->context->minutes=600;
    if(combat) {auto &enemy=s.map(23).monsters->slots[5];enemy.x=8;enemy.y=11;enemy.lifecycle=XeenActorLifecycle::Present;enemy.accounted=false;}
    save_test::clearExplored(s);return s;
}
void roundtrip(Inputs &in,Fixture &f) {
    const auto bytes=XeenSaveFormat::encode(f.snapshot());Fixture restored(in,XeenSaveFormat::decode(bytes));
    check(bytes==XeenSaveFormat::encode(restored.snapshot()),"Item spell restore changed item/gold/cost/time/RNG");
}
void begin(Fixture &f,bool item,unsigned spell,bool mouse) {
    if(item) {f.act(SelectMemberAction{0});key(f,'i',mouse);key(f,'m',mouse);key(f,'1',mouse);key(f,'u',mouse);}
    else {f.act(CastSpellAction{});f.act(SelectMemberAction{0});key(f,'c',mouse);}
    const auto cost=R::cost(spell,12);
    const auto owner=f.p.party.activeRosterIds()[0];
    check(f.p.roster.at(owner).currentSp==(item?3:1000-int(cost.sp)) && f.p.purse->gems==1000-(item?0:cost.gems),"Cost is spent before the member prompt");
}
void exploration(Inputs &in) {
    for(unsigned spell:{19u,57u,39u})for(bool item:{false,true})for(unsigned path=0;path<6;++path) {
        if(item && spell==39)continue;
        const bool mouse=path%2;scenario="exploration spell="+std::to_string(spell)+" item="+std::to_string(item)+" path="+std::to_string(path);
        auto s=source(in,spell);const auto owner=s.activeRosterIds[0],recipient=s.activeRosterIds[1];
        if(item) {auto &c=s.characters[owner];c.characterClass=XeenCharacterClass::Knight;c.hasSpells=false;c.learnedSpells->fill(0);c.currentSp=3;c.miscellaneous[0]={1,std::uint8_t(spell==19?63:53),std::uint8_t(path==0?1:2),0};}
        auto &target=s.characters[recipient];
        if(path==3) {if(spell==19)target.weapons[0].state=0x40;else if(spell==57)target.miscellaneous[0].id=53;else target.weapons[0]={0,34,0,0};}
        if(path==4 && spell==57)target.miscellaneous[0].state=0x82;
        if(path==5) {if(spell==57)target.miscellaneous[0].state=0x42;else target.weapons[0].state=spell==19?0x80:0x40;}
        const C category=spell==57?C::Miscellaneous:spell==19 && path==4?C::Armor:C::Weapons;
        const XeenItem selected=xeenInventoryItems(target,category)->at(0);
        const auto gold=s.journey->purse->gold;
        Fixture f(in,s);const auto before=f.w.sessionState().journeyRandom();auto expected=XeenCombatRandom(*before);
        begin(f,item,spell,mouse);
        if(path==0)key(f,InputKey::Escape,mouse);
        else {
            key(f,InputKey::F1+1,mouse);
            const auto input=f.flow->inputContext(f.flow->frame().presentation());
            check(input.dialog && !input.dialog->key('q') && !input.dialog->key(InputKey::F1),"Spell Items mode offers neither Quest nor transfers");
            if(category==C::Armor)key(f,'a',mouse);
            const auto action=spell==19?'e':spell==57?'r':'g';
            if(path==1)key(f,InputKey::Escape,mouse);
            else {
                if(path==2) {key(f,action,mouse);key(f,InputKey::Escape,mouse);key(f,'1',mouse);key(f,action,mouse);}
                else {key(f,action,mouse);key(f,'1',mouse);}
                // The action is immediate: no Yes/No, including equipped Item to Gold.
                const auto after=f.flow->inputContext(f.flow->frame().presentation());
                check(!after.dialog || after.dialog->anyKey || !after.dialog->key('y'),"Item spell must not ask for confirmation");
                XeenItemSpellPreparation work(static_cast<S>(spell),12,category,selected);XeenConsequenceDraw draws{expected,64,{}};
                check(work.service(draws),"Expected item preparation");
                check(xeenSameItem(xeenInventoryItems(f.p.roster.at(recipient),category)->at(0),
                    work.gold?target.weapons[1]:work.item),"Published item change/refusal/compaction");
                check(f.p.purse->gold==gold+work.gold,"Gold credited exactly once");
                if(work.failed) {check(after.dialog && after.dialog->anyKey,"Original refusal awaits acknowledgment");tick(f);check(f.p.encounterContext->minutes==600,"Refusal waits before time/response");}
                if(spell==57 && path==4)check(f.p.roster.at(recipient).miscellaneous[0].state==0xbf,"Broken Recharge fills to 63 in real flow");
            }
        }
        quiet(f);const auto cost=R::cost(spell,12);const bool refunded=path==0;
        check(f.p.roster.at(owner).currentSp==(item?3+int(refunded?cost.sp:0):1000-int(refunded?0:cost.sp)),"Member Escape refunds; later Escape/refusal keeps SP");
        check(f.p.purse->gems==1000+(item?(refunded?cost.gems:0):0)-(item?0:(refunded?0:cost.gems)),"Own-ID gem refund including item cancellation");
        if(item)check(path==0?!f.p.roster.at(owner).miscellaneous[0].material:f.p.roster.at(owner).miscellaneous[0].state==1,"Refund does not restore the spent charge");
        check(f.w.sessionState().journeyRandom()==expected.continuation(),"Item spell DOS draw order without extra response draws");
        check(f.p.encounterContext->minutes==(item?600:601),"Existing book/item time suffix");roundtrip(in,f);
    }
}
void combat(Inputs &in) {
    for(unsigned spell:{19u,57u,39u})for(bool quick:{false,true})for(unsigned path=0;path<4;++path) {
        if(spell==19 && path)continue;
        scenario="combat spell="+std::to_string(spell)+" quick="+std::to_string(quick)+" path="+std::to_string(path);
        auto s=source(in,spell,true);const auto owner=s.activeRosterIds[0],recipient=s.activeRosterIds[1];
        if(path==3) {if(spell==57)s.characters[recipient].miscellaneous[0].id=53;else s.characters[recipient].weapons[0]={0,34,0,0};}
        Fixture f(in,s);f.act(WaitAction{});
        const auto fight=[&]()->const XeenCombat &{return *f.flow->encounter()->combat();};
        for(unsigned n=0;n<1000 && (!f.flow->encounter()->combat() || fight().phase()!=XeenCombatPhase::PlayerReady);++n)tick(f);
        check(fight().participant()==0,"Item-spell caster initiative");const auto random=fight().random();
        f.act(quick?PlayerAction{QuickFightAction{}}:PlayerAction{CastSpellAction{}});
        if(fight().cast() && fight().cast()->phase==XeenCombatCastPhase::Confirm)key(f,'c');
        if(spell==19) {
            check(!fight().cast() || !fight().cast()->committed,"Learned Enchant forbidden in combat/Quick Fight");
            check(f.p.roster.at(owner).currentSp==1000 && f.p.purse->gems==1000 && fight().random()==random,"Forbidden Enchant costs nothing");continue;
        }
        check(fight().cast() && fight().cast()->phase==XeenCombatCastPhase::PartyTarget,"Combat original member prompt");
        if(path==0)key(f,InputKey::Escape);
        else {
            key(f,InputKey::F1+1,quick);check(fight().cast()->phase==XeenCombatCastPhase::Items,"Combat Items mode");
            if(path==1)key(f,InputKey::Escape);
            else {key(f,spell==57?'r':'g',quick);key(f,'1',quick);}
        }
        for(unsigned n=0;n<1000 && fight().cast();++n) {
            if(auto cast=fight().cast();cast && cast->phase==XeenCombatCastPhase::Result && cast->result.failed)f.act(AcknowledgeAction{});
            tick(f);
        }
        check(!fight().cast() && fight().phase()!=XeenCombatPhase::Failed,"Combat item spell settles into its successor");
        const auto cost=R::cost(spell,12);
        check(f.p.roster.at(owner).currentSp==1000-int(path?cost.sp:0) && f.p.purse->gems==1000-(path?cost.gems:0),"Combat own-ID refund and kept late cost");
        auto expected=random;
        if(path==2) {
            const C category=spell==57?C::Miscellaneous:C::Weapons;
            const auto selected=xeenInventoryItems(s.characters[recipient],category)->at(0);
            XeenItemSpellPreparation work(static_cast<S>(spell),12,category,selected);XeenConsequenceDraw draw{expected,64,{}};check(work.service(draw),"Combat expected preparation");
            check(xeenSameItem(xeenInventoryItems(f.p.roster.at(recipient),category)->at(0),work.gold?s.characters[recipient].weapons[1]:work.item),"Combat item effect/compaction");
        }
        check(fight().random()==expected,"Combat item spell exact RNG");
    }
}
}
int main(int argc,char **argv) {try {
    check(argc==2,"usage: item-spell-original <installation>");const auto installation=xeenTestInstallationDetector().detect(argv[1]);check(bool(installation),"Missing installation");Inputs in(*installation);
    std::ostringstream captured;auto *old=std::cout.rdbuf(captured.rdbuf());
    try {exploration(in);combat(in);}catch(...) {std::cout.rdbuf(old);throw;}
    std::cout.rdbuf(old);std::cout<<"M76 original-data item spells, book/items, combat/Quick Fight, mouse/keys and exact restore passed\n";return 0;
}catch(const std::exception &e){std::cerr<<scenario<<": "<<e.what()<<'\n';return 1;}}
