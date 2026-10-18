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
        if(input.dialog && input.dialog->anyKey)f.act(AcknowledgeAction{});
        tick(f);
    }
    check(f.flow->canSave(),"Buff cast/item did not settle");
}
XeenSaveSnapshot source(Inputs &in,unsigned spell,bool combat=false) {
    auto s=in.service();s.camera=combat?XeenCamera{23,9,11,XeenDirection::West}:XeenCamera{28,18,4,XeenDirection::West};
    for(auto *actors:{&s.map(23).monsters->slots,&s.map(28).monsters->slots})for(auto &a:*actors) {
        a.x=a.y=-128;a.lifecycle=XeenActorLifecycle::Defeated;a.accounted=true;
    }
    for(auto id:s.activeRosterIds) {
        auto &c=s.characters[id];c.conditions.fill(0);c.currentHp=1000;
        s.journey->supplements[id].inputs.speed={int(180-id),0};
        for(auto *items:{&c.weapons,&c.armor,&c.accessories})for(auto &item:*items)item.frame=0;
    }
    auto &caster=s.characters[s.activeRosterIds[0]];
    caster.characterClass=spell==13 || spell==53?XeenCharacterClass::Sorcerer:XeenCharacterClass::Cleric;
    caster.hasSpells=true;caster.permanentLevel=5;caster.temporaryLevel=2;caster.currentSp=1000;
    caster.learnedSpells.emplace();caster.learnedSpells->fill(0);
    caster.currentSpell=*R::slotForSpell(*R::categoryForClass(caster.characterClass),spell);
    caster.learnedSpells->at(caster.currentSpell)=1;caster.quickOption=1;
    s.journey->supplements[caster.rosterId].inputs.speed={250,0};
    s.journey->treasure->gems=1000;s.journey->context->ctr24=23;s.journey->context->minutes=600;
    s.journey->context->effects.fill(0);s.journey->context->lightAndResistances.fill(0);
    if(combat) {auto &enemy=s.map(23).monsters->slots[5];enemy.x=8;enemy.y=11;enemy.lifecycle=XeenActorLifecycle::Present;enemy.accounted=false;}
    save_test::clearExplored(s);return s;
}
void roundtrip(Inputs &in,Fixture &f) {
    const auto bytes=XeenSaveFormat::encode(f.snapshot());Fixture restored(in,XeenSaveFormat::decode(bytes));
    check(bytes==XeenSaveFormat::encode(restored.snapshot()),"Buff restore replayed effects/time/RNG");
}
void effects(const XeenGameplayContext &context,unsigned spell,int element,bool cancel) {
    XeenGameplayContext expected;R::applyPartyEffect(*R::supported(spell),expected,7,
        element>=0?std::optional<unsigned>{unsigned(element)}:std::nullopt);
    if(cancel) {expected.effects.fill(0);expected.lightAndResistances.fill(0);}
    check(context.effects==expected.effects && context.lightAndResistances==expected.lightAndResistances,"Published party effect assignment");
}
void chooseElement(Fixture &f,int element,bool mouse) {
    const auto input=f.flow->inputContext(f.flow->frame().presentation());
    check(input.dialog && !input.dialog->anyKey,"Select Element modal input");
    if(element<0)f.act(CancelInteractionAction{});
    else if(mouse)f.act(*input.dialog->click(61+30*element,93));
    else {constexpr unsigned keys[]{'f','e','c','a'};f.act(DialogKeyAction{keys[element]});}
}
void booksAndItems(Inputs &in) {
    for(bool item:{false,true})for(unsigned spell:{3u,30u,31u,53u,55u,12u,13u})
    for(int element:spell==55?std::vector<int>{-1,0,1,2,3}:std::vector<int>{0}) {
        auto s=source(in,spell);const auto owner=s.activeRosterIds[0];
        constexpr unsigned spells[]{3,30,31,53,55,12,13},powers[]{22,30,25,29,14,49,50};
        unsigned power=0;for(unsigned i=0;i<7;++i)if(spells[i]==spell)power=powers[i];
        if(item) {auto &c=s.characters[owner];c.characterClass=XeenCharacterClass::Knight;c.hasSpells=false;c.learnedSpells->fill(0);c.currentSp=3;c.miscellaneous={};c.miscellaneous[0]={1,std::uint8_t(power),1,0};}
        Fixture f(in,s);const auto random=f.w.sessionState().journeyRandom();
        if(item) {f.act(SelectMemberAction{0});for(unsigned key:{unsigned('i'),unsigned('m'),unsigned('1'),unsigned('u')})f.act(DialogKeyAction{key});}
        else {
            f.act(CastSpellAction{});f.act(SelectMemberAction{0});const auto sidebar=f.flow->frame();
            f.act(DialogKeyAction{'c'});
            if(spell==55)for(int y=0;y<146;++y)for(int x=226;x<320;++x)
                check(sidebar.pixels[y*320+x]==f.flow->frame().pixels[y*320+x],"Select Element erased or changed the Cast sidebar");
        }
        if(spell==55)chooseElement(f,element,element%2==1);
        quiet(f);const bool cancel=spell==55 && element<0;const auto cost=R::cost(spell,7);
        check(f.p.roster.at(owner).currentSp==(item?3+int(cancel?cost.sp:0):1000-int(cancel?0:cost.sp)),"Book/item SP debit/refund");
        check(f.p.monsterTreasure->gems==1000+(item?int(cancel?cost.gems:0):-int(cancel?0:cost.gems)),"Book/item gem debit/refund");
        check(f.p.encounterContext->minutes==(item?600:601) && f.w.sessionState().journeyRandom()==random,"Buff time/RNG");
        if(item)check(!f.p.roster.at(owner).miscellaneous[0].id,"Last item charge consumed even on cancellation");
        effects(*f.p.encounterContext,spell,element,cancel);roundtrip(in,f);
    }
}
void combat(Inputs &in) {
    for(bool quick:{false,true})for(unsigned spell:{3u,30u,31u,53u,55u,12u,13u})
    for(int element:spell==55?std::vector<int>{-1,0,1,2,3}:std::vector<int>{0}) {
        auto s=source(in,spell,true);Fixture f(in,s);f.act(WaitAction{});
        const auto fight=[&]()->const XeenCombat &{return *f.flow->encounter()->combat();};
        for(unsigned n=0;n<1000 && (!f.flow->encounter()->combat() || fight().phase()!=XeenCombatPhase::PlayerReady);++n)tick(f);
        check(fight().participant()==0,"Original mainland caster initiative");
        const auto random=fight().random();const auto minute=f.p.encounterContext->minutes;
        f.act(quick?PlayerAction{QuickFightAction{}}:PlayerAction{CastSpellAction{}});
        if(fight().cast()->phase==XeenCombatCastPhase::Confirm)f.act(DialogKeyAction{'c'});
        check(fight().cast()->committed,"Combat buff admission");
        if(spell==55)chooseElement(f,element,element%2==0);
        for(unsigned n=0;n<1000 && fight().cast();++n)tick(f);
        check(!fight().cast() && fight().phase()!=XeenCombatPhase::Failed,"Combat buff did not settle");
        const bool cancel=spell==55 && element<0;const auto cost=R::cost(spell,7);
        check(f.p.roster.at(s.activeRosterIds[0]).currentSp==1000-int(cancel?0:cost.sp) && f.p.monsterTreasure->gems==1000-(cancel?0:cost.gems),"Combat cost/refund");
        check(fight().random()==random && f.p.encounterContext->minutes==minute,"Combat buff drew or advanced round time");
        effects(*f.p.encounterContext,spell,element,cancel);
    }
}
int fightDamage(Inputs &in,unsigned spell) {
    auto s=source(in,spell,true);const auto owner=s.activeRosterIds[1];
    auto &fighter=s.characters[owner];fighter.characterClass=XeenCharacterClass::Knight;fighter.permanentLevel=1;fighter.temporaryLevel=0;
    fighter.weapons[0]={0,1,0,1};s.journey->supplements[owner].inputs.accuracy={200,0};
    s.journey->supplements[owner].inputs.speed={240,0};s.journey->supplements[owner].inputs.might={15,0};
    Fixture f(in,s);f.act(WaitAction{});
    for(unsigned n=0;n<1000 && (!f.flow->encounter()->combat() || f.flow->encounter()->combat()->phase()!=XeenCombatPhase::PlayerReady);++n)tick(f);
    f.act(CastSpellAction{});f.act(DialogKeyAction{'c'});
    for(unsigned n=0;n<1000 && f.flow->encounter()->combat()->cast();++n)tick(f);
    check(f.flow->encounter()->combat()->participant()==1,"Buffed mainland fight next fighter");
    f.act(AttackAction{});
    for(unsigned n=0;n<1000;++n) {
        const auto result=f.flow->encounter()->combatResult();
        if(result.operation==XeenCombatOperation::PlayerAttack && result.damage>0)return result.damage;
        tick(f);
    }
    throw std::runtime_error("Mainland weapon attack did not publish damage");
}
void presentation(Inputs &in) {
    auto s=source(in,3);Fixture f(in,s);
    const auto &text=in.assets.uiText();
    const auto base=f.flow->frame();
    std::vector<std::tuple<std::string,unsigned,int,int>> sprites;
    const XeenDialogSpriteDraw draw=[&](auto &,const char *name,unsigned frame,int x,int y) {sprites.emplace_back(name,frame,x,y);};
    (void)drawXeenElement(text,base,in.font,draw);
    check(sprites.size()==4,"Select Element button sprites");
    for(unsigned i=0;i<4;++i)check(sprites[i]==std::make_tuple(std::string("element.icn"),2*i,int(60+30*i),92),"Select Element sprite frame/position");
    XeenGameplayContext context;for(unsigned i=2;i<6;++i)context.lightAndResistances[i]=7;
    sprites.clear();(void)drawXeenProtectionCorners(base,context,draw);
    for(unsigned i=0;i<4;++i)check(sprites[i]==std::make_tuple(std::string("fecp.brd"),9+2*i,i%2?219:8,i<2?8:134),"Alternate protection frame/position");
    // Character sheet condition expansion keeps buffs even on an ill member.
    f.p.encounterContext->effects[8]=3;f.p.encounterContext->effects[6]=4;
    f.p.encounterContext->effects[7]=5;f.p.encounterContext->effects[5]=6;
    f.p.roster.at(s.activeRosterIds[0]).conditions[3]=1;
    const auto popup=xeenSheetPopup(text,f.p,0,19);
    std::string expected;constexpr const char *names[]{"BLESSED","POWER_SHIELD","HOLY_BONUS","HEROISM"};
    for(unsigned i=0;i<4;++i)expected+=xeenDialogFormat(text.scalar(names[i]),{std::to_string(i+3)});
    check(popup.text.find(expected)!=std::string::npos,"Condition popup buff lines/order/values");
}
}
int main(int argc,char **argv) {try {
    check(argc==2,"usage: party-buff-original <installation>");const auto installation=xeenTestInstallationDetector().detect(argv[1]);check(bool(installation),"Missing installation");Inputs in(*installation);
    std::ostringstream captured;auto *old=std::cout.rdbuf(captured.rdbuf());
    try {booksAndItems(in);combat(in);presentation(in);check(fightDamage(in,31)>fightDamage(in,1),"Original mainland Holy Bonus fight did not differ from unbuffed fight");}catch(...) {std::cout.rdbuf(old);throw;}
    std::cout.rdbuf(old);std::cout<<"M76 original book, Quick Fight, element keys/mouse/refunds, charged items and exact restore passed\n";return 0;
}catch(const std::exception &e) {std::cerr<<e.what()<<'\n';return 1;}}
