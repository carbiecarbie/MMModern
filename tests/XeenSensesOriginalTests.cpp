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
    check(f.flow->canSave(),"Senses cast/item did not settle");
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
    caster.characterClass=spell!=74?XeenCharacterClass::Sorcerer:XeenCharacterClass::Cleric;
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
    check(bytes==XeenSaveFormat::encode(restored.snapshot()),"Senses restore replayed effects/time/RNG");
}
void booksAndItems(Inputs &in) {
    constexpr unsigned spells[]{42,41,74,4,15,34},powers[]{1,20,32,18,34,23};
    for(bool item:{false,true})for(unsigned n=0;n<6;++n) {
        const unsigned spell=spells[n];auto s=source(in,spell);const auto owner=s.activeRosterIds[0];
        if(item) {auto &c=s.characters[owner];c.characterClass=XeenCharacterClass::Knight;c.hasSpells=false;c.learnedSpells->fill(0);c.currentSp=3;c.miscellaneous={};c.miscellaneous[0]={1,std::uint8_t(powers[n]),1,0};}
        Fixture f(in,s);const auto random=f.w.sessionState().journeyRandom();
        if(item) {f.act(SelectMemberAction{0});for(unsigned key:{unsigned('i'),unsigned('m'),unsigned('1'),unsigned('u')})f.act(DialogKeyAction{key});}
        else {f.act(CastSpellAction{});f.act(SelectMemberAction{0});f.act(DialogKeyAction{'c'});}
        if(spell==15) {
            const auto input=f.flow->inputContext(f.flow->frame().presentation());
            check(input.dialog && input.dialog->anyKey && input.dialog->anyClick,"Detection key/mouse wait");
            const auto held=f.p.encounterContext->minutes;
            tick(f);tick(f);check(f.p.encounterContext->minutes==held,"Detection waits before time/monster response");
            f.act(n%2?PlayerAction{DialogKeyAction{'z'}}:PlayerAction{AcknowledgeAction{}});
        }
        quiet(f);const auto cost=R::cost(spell,7);
        check(f.p.roster.at(owner).currentSp==(item?3:1000-int(cost.sp)),"Senses debit, including Identify failure, no refund");
        check(f.p.monsterTreasure->gems==1000-(item?0:cost.gems),"Senses gems");
        check(f.p.encounterContext->minutes==(item?600:601) && f.w.sessionState().journeyRandom()==random,"Senses time/RNG");
        if(item)check(!f.p.roster.at(owner).miscellaneous[0].id,"Information failure still consumes last charge");
        if(spell==42)check(f.p.encounterContext->lightAndResistances[0]==1,"Published Light");
        if(spell==41)check(f.p.encounterContext->effects[XeenPartyEffect::Levitate]==1,"Published Levitate");
        if(spell==74)check(f.p.encounterContext->effects[XeenPartyEffect::WalkOnWater]==1,"Published Walk on Water");
        if(spell==4)check(f.p.encounterContext->effects[XeenPartyEffect::Clairvoyance]==1,"Published Clairvoyance");
        roundtrip(in,f);
    }
}
void combat(Inputs &in) {
    for(bool quick:{false,true})for(unsigned spell:{42u,41u,74u,4u,15u,34u}) {
        auto s=source(in,spell,true);Fixture f(in,s);f.act(WaitAction{});
        const auto fight=[&]()->const XeenCombat &{return *f.flow->encounter()->combat();};
        for(unsigned n=0;n<1000 && (!f.flow->encounter()->combat() || fight().phase()!=XeenCombatPhase::PlayerReady);++n)tick(f);
        check(fight().participant()==0,"Mainland caster initiative");
        const auto random=fight().random();const auto minute=f.p.encounterContext->minutes;
        f.act(quick?PlayerAction{QuickFightAction{}}:PlayerAction{CastSpellAction{}});
        if(fight().cast()->phase==XeenCombatCastPhase::Confirm)f.act(DialogKeyAction{'c'});
        check(fight().cast()->committed,"Combat senses admission");
        for(unsigned n=0;n<1000 && fight().cast() && !fight().cast()->information;++n)tick(f);
        if(spell==15 || spell==34) {
            check(fight().cast() && fight().cast()->information,"Real mainland information screen");
            const auto info=*fight().cast()->information;
            if(spell==15)check(info.grid[24]>=1,"Detect shows real mainland contact monster");
            else {check(info.monsters[0] && !info.monsters[0]->name.empty() && info.monsters[0]->hp>0,"Identify shows real mainland monster fields");}
            const auto input=f.flow->inputContext(f.flow->frame().presentation());
            check(input.dialog && input.dialog->anyKey && input.dialog->anyClick,"Information input");
            tick(f);tick(f);check(fight().cast() && fight().cast()->information,"Information screen retained");
            f.act(quick?PlayerAction{AcknowledgeAction{}}:PlayerAction{DialogKeyAction{'z'}});
        }
        for(unsigned n=0;n<1000 && fight().cast();++n)tick(f);
        check(!fight().cast() && fight().phase()!=XeenCombatPhase::Failed,"Combat senses settles");
        const auto cost=R::cost(spell,7);
        check(f.p.roster.at(s.activeRosterIds[0]).currentSp==1000-int(cost.sp) && f.p.monsterTreasure->gems==1000-cost.gems,"Combat senses cost");
        check(fight().random()==random && f.p.encounterContext->minutes==minute,"Combat senses consumes no RNG/round time");
        if(spell==42)check(fight().stepped(),"Light marks party stepped");
    }
}
void water(Inputs &in) {
    auto s=source(in,74);s.camera={23,12,9,XeenDirection::West};
    Fixture f(in,s);const auto &map=f.w.map(23);
    check(map.geometry.surfaceTypes[map.geometry.cells[9*16+11].surfaceIndex]==0,"Original shallow water witness");
    f.act(NavigationAction::MoveForward);quiet(f);check(f.c.x==12 && f.c.y==9,"Unbuffed party cannot enter water");
    f.act(CastSpellAction{});f.act(SelectMemberAction{0});f.act(DialogKeyAction{'c'});quiet(f);
    f.act(NavigationAction::MoveForward);quiet(f);check(f.c.x==11 && f.c.y==9,"Walk on Water reaches original shallow water");
    roundtrip(in,f);
    auto expired=f.snapshot();expired.journey->context->effects[XeenPartyEffect::WalkOnWater]=0;
    Fixture restored(in,expired);check(XeenSaveFormat::encode(restored.snapshot())==XeenSaveFormat::encode(expired),"Water position remains restorable after effect expiry");
    // Scan loaded open components only; deep water/cloud must use synthetic consumers if absent.
    unsigned deep=0,cloud=0;
    for(unsigned id:{23u,28u,109u,110u,111u})for(const auto &cell:f.w.map(id).geometry.cells) {
        const auto type=f.w.map(id).geometry.surfaceTypes[cell.surfaceIndex];deep+=type==8;cloud+=type==13;
    }
    check(!deep && !cloud,"Update documented open-map deep/cloud availability");
}
void presentation(Inputs &in) {
    auto s=source(in,15);Fixture f(in,s);const auto base=f.flow->frame();
    XeenSpellInformation detect;detect.detection=true;detect.facing=XeenDirection::West;detect.grid[0]=3;detect.grid[48]=2;
    std::vector<std::tuple<std::string,unsigned,int,int>> sprites;
    const XeenDialogSpriteDraw draw=[&](auto &frame,const char *name,unsigned index,int x,int y) {
        sprites.emplace_back(name,index,x,y);in.assets.drawDialogSprite(frame,name,index,x,y);
    };
    (void)drawXeenSpellInformation(in.assets.uiText(),base,in.font,detect,draw);
    check(sprites.size()==4 && sprites[1]==std::make_tuple(std::string("detctmon.icn"),3u,244,81) && sprites[2]==std::make_tuple(std::string("detctmon.icn"),2u,298,123),"Detection original marker geometry");
    XeenSpellInformation identify;identify.monsters[0]=XeenSpellInformation::Monster{"Goblin",1234,12,3,0};
    (void)drawXeenSpellInformation(in.assets.uiText(),base,in.font,identify,draw);
    XeenGameplayContext context;context.lightAndResistances[0]=4;context.effects[XeenPartyEffect::Clairvoyance]=context.effects[XeenPartyEffect::Levitate]=context.effects[XeenPartyEffect::WalkOnWater]=1;
    const auto popup=xeenInfoPopup(in.assets.uiText(),context);
    for(const char *key:{"LIGHT_COUNT_TEXT","CLAIRVOYANCE_TEXT","LEVITATE_TEXT","WALK_ON_WATER_TEXT"})check(!in.assets.uiText().scalar(key).empty(),"Original DOS Info effect text");
    check(popup.bounds.bottom==161,"Info four-effect height");
}
}
int main(int argc,char **argv) {try {
    check(argc==2,"usage: senses-original <installation>");const auto installation=xeenTestInstallationDetector().detect(argv[1]);check(bool(installation),"Missing installation");Inputs in(*installation);
    std::ostringstream captured;auto *old=std::cout.rdbuf(captured.rdbuf());
    try {booksAndItems(in);combat(in);water(in);presentation(in);}catch(...) {std::cout.rdbuf(old);throw;}
    std::cout.rdbuf(old);std::cout<<"M76 senses book/items, combat/Quick Fight, real water/monsters, dialogs and restore passed\n";return 0;
}catch(const std::exception &e) {std::cerr<<e.what()<<'\n';return 1;}}
