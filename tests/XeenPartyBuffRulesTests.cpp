#include "games/xeen/XeenCombatRules.h"
#include "games/xeen/XeenItemServiceRules.h"
#include <iostream>
#include <stdexcept>
using namespace mmodern;
namespace {
using R=XeenLearnedSpellRules;
using D=XeenCombatRandom::Draw;
void check(bool value,const char *message) {if(!value)throw std::runtime_error(message);}
template<class T> void finish(T &candidate,XeenCombatRandom &random) {
    for(unsigned n=0;n<100;++n) {XeenConsequenceDraw draw{random,1};if(candidate.service(draw))return;}
    throw std::runtime_error("Buff candidate did not finish");
}
void handlers() {
    constexpr unsigned ids[]{3,30,31,53,55,12,13},powers[]{22,30,25,29,14,49,50};
    constexpr unsigned gems[]{1,3,1,2,1,10,10};
    for(unsigned n=0;n<7;++n)for(unsigned level:{0u,1u,7u,99u,255u,256u,300u}) {
        const auto spell=static_cast<XeenLearnedSpell>(ids[n]);
        check(R::supportedIn(ids[n],false)==spell && R::supportedIn(ids[n],true)==spell,"Book combat/exploration admission");
        check(xeenItemUseSpell(powers[n])==spell && xeenItemUseSpell(powers[n],true)==spell,"Item power dispatch");
        XeenCharacter caster;caster.characterClass=n==3 || n==6?XeenCharacterClass::Sorcerer:XeenCharacterClass::Cleric;
        caster.hasSpells=true;caster.permanentLevel=level?level-1:0;caster.temporaryLevel=level?1:0;
        caster.learnedSpells.emplace();caster.learnedSpells->fill(0);
        caster.currentSpell=*R::slotForSpell(*R::categoryForClass(caster.characterClass),ids[n]);
        caster.learnedSpells->at(caster.currentSpell)=1;caster.currentSp=32767;
        const auto cost=R::cost(ids[n],caster.currentLevel());
        check(cost.sp==(n<4?2*level:n==4?level:n==5?75:40) && cost.gems==gems[n],"Cost/current level source");
        check(R::quickSpellCheck(caster,1000)==XeenQuickSpellCheck::Supported,"Quick Fight admission");
        if(cost.sp) {caster.currentSp=cost.sp-1;check(R::check(caster,caster.currentSpell,1000,true)==XeenQuickSpellCheck::InsufficientSp,"SP guard");}
        caster.currentSp=32767;check(R::check(caster,caster.currentSpell,0,true)==XeenQuickSpellCheck::InsufficientGems,"Gem guard");
        XeenGameplayContext context;context.effects.fill(99);context.lightAndResistances.fill(999);
        R::applyPartyEffect(spell,context,caster.currentLevel(),3);
        const unsigned byte=level%256;
        if(n<4) {constexpr unsigned index[]{8,5,7,6};check(context.effects[index[n]]==byte,"Byte assignment replaces stronger bonus");}
        if(n==4)check(context.lightAndResistances[5]==std::min(2*level+5,200u) && context.lightAndResistances[2]==999,"Acid assigns Poison only, capped");
        if(n==5) {
            for(unsigned i:{4u,5u,7u,8u})check(context.effects[i]==byte,"Day of Protection byte flags");
            for(unsigned i:{0u,2u,3u,4u,5u})check(context.lightAndResistances[i]==byte,"Day of Protection word assignments");
            check(context.effects[6]==99 && context.effects[1]==99 && context.lightAndResistances[1]==999,"Day of Protection unrelated fields");
        }
        if(n==6) {
            for(unsigned i:{0u,2u,3u,6u})check(context.effects[i]==byte,"Day of Sorcery byte flags/count");
            check(context.lightAndResistances[0]==byte && !context.effects[1] && context.effects[5]==99 && context.lightAndResistances[2]==999,"Day of Sorcery light/automap and exclusions");
        }
    }
    for(unsigned element=0;element<4;++element) {
        XeenGameplayContext context;R::applyPartyEffect(XeenLearnedSpell::ProtectionFromElements,context,7,element);
        for(unsigned n=0;n<4;++n)check(context.lightAndResistances[n+2]==(n==element?19:0),"Element selection order");
        const auto before=context;R::applyPartyEffect(XeenLearnedSpell::ProtectionFromElements,context,7);
        check(context==before,"Element cancellation makes no assignment");
    }
}
XeenConsequenceCharacters characters() {
    XeenConsequenceCharacters c;
    for(unsigned i=0;i<6;++i) {c[i].rosterId=kXeenCombatOwners[i];c[i].permanentLevel=3;c[i].birthYear=592;c[i].currentHp=100;c[i].endurance=c[i].intellect=c[i].personality={15,0};}
    c[0].characterClass=XeenCharacterClass::Cleric;return c;
}
XeenConsequenceInputs inputs() {
    XeenConsequenceInputs values;
    for(auto &i:values) {i.might=i.speed={15,0};i.accuracy={12,0};i.luck=XeenAttributeValue{15,0};i.resistances=XeenCombatResistances{};i.poisonResistance=XeenAttributeValue{0,0};}
    return values;
}
void consumers() {
    auto c=characters();auto in=inputs();
    for(unsigned condition:{0u,8u,13u}) {
        c[0].conditions.fill(0);c[0].conditions[condition]=1;
        const auto base=XeenCharacterRules::sheetArmorClass(c[0],&in[0],{610},true);
        in[0].temporaryAc=-3;
        check(XeenCharacterRules::sheetArmorClass(c[0],&in[0],{610,7})==std::max(base+4,0),"Bless adds before zero clamp, regardless of condition");
        check(XeenCharacterRules::sheetArmorClass(c[0],&in[0],{610,7},true)==base,"Base-only omits Bless and temporary AC");
    }
    c=characters();c[0].permanentLevel=1;c[0].characterClass=XeenCharacterClass::Knight;
    XeenMonsterRecord monster;monster.raw[22]=5;
    for(bool shoot:{false,true})for(auto difficulty:{XeenDifficulty::Adventurer,XeenDifficulty::Warrior}) {
        c[0].weapons[0]={0,std::uint8_t(shoot?30:1),0,std::uint8_t(shoot?4:1)};
        XeenGameplayContext context;context.effects[5]=4;context.effects[7]=7;
        std::vector<D> tape(3,{1,shoot?2u:3u,2});tape.push_back({1,20,10});if(shoot)tape.push_back({1,56,56});
        XeenCombatRandom random(tape);XeenPhysicalPlayerCandidate attack(c[0],in[0],monster,6,610,shoot,difficulty,XeenActorStatus::Physical,context);finish(attack,random);
        check(attack.hit && attack.damage==13*(difficulty==XeenDifficulty::Adventurer?3:1)+(shoot?0:2),"Heroism hit and Holy Bonus before difficulty, melee/Shoot");
        if(shoot) {
            XeenCombatRandom poisonRandom(tape);XeenSpellDamageCandidate poison(c[0],in[0],monster,6,610,difficulty,XeenActorStatus::Physical,true,context);finish(poison,poisonRandom);
            check(poison.damage==10 && !poison.miss,"Poison Volley uses Heroism accuracy and overrides Holy Bonus damage");
        }
    }
    c=characters();in=inputs();
    {
        XeenGameplayContext context;context.effects[6]=3;context.lightAndResistances[2]=4;
        monster={};monster.raw[24]=1;monster.raw[25]=3;monster.raw[26]=1;monster.raw[28]=20;monster.raw[29]=2;monster.raw[30]=5;
        XeenCombatRandom random(std::vector<D>{{1,20,20},{1,25,1},{1,40,40},{1,25,25}});
        XeenEnemyAttackCandidate attack(c,in,monster,610,63,{},XeenActorStatus::Physical,context);finish(attack,random);
        check(attack.result.damage==3 && attack.characters[0].conditions[3]==1 && random.position()==4,"Positive shielded monster damage retains secondary save after shield");
        c[0].conditions[8]=1;monster.raw[29]=0;monster.raw[31]=5;context.effects[6]=20;
        XeenCombatRandom physicalRandom(std::vector<D>{{1,20,20}});
        XeenEnemyAttackCandidate physical(c,in,monster,610,63,{},XeenActorStatus::Physical,context);finish(physical,physicalRandom);
        check(!physical.result.damage && physicalRandom.position()==1,"Physical shield skips elemental and zero-damage secondary saves");
        c[0].conditions[8]=0;
    }
    for(unsigned type=2;type<=5;++type)for(unsigned shield:{3u,20u}) {
        monster={};monster.raw[24]=1;monster.raw[25]=3;monster.raw[26]=1;monster.raw[28]=20;monster.raw[29]=type;monster.raw[30]=5;
        // Initial Physical success: 20 -> 10; party -4 -> 6;
        // personal resistance 10 succeeds once: 6 -> 3; fail; shield -> 0.
        in[0].resistances->firePermanent=in[0].resistances->electricalPermanent=in[0].resistances->coldPermanent=10;
        in[0].poisonResistance=XeenAttributeValue{10,0};
        XeenGameplayContext context;context.lightAndResistances[type]=4;context.effects[6]=shield;
        XeenCombatRandom random(std::vector<D>{{1,20,20},{1,25,1},{1,50,1},{1,50,50}});
        XeenEnemyAttackCandidate attack(c,in,monster,610,63,{},XeenActorStatus::Physical,context);finish(attack,random);
        check(!attack.result.damage && !attack.characters[0].conditions[3] && random.position()==4,"Monster protection/save/shield order and suppressed secondary save");
        XeenDamageProtection protection;protection.resistances[type-2]=4;protection.powerShield=shield;
        XeenCombatRandom trapRandom(std::vector<D>{{1,50,1},{1,50,50}});
        XeenTypedDamageCandidate trap(c,in,10,XeenDamageType(type),610,1,protection);finish(trap,trapRandom);
        check(!trap.result.damage && trapRandom.position()==2,"Trap subtracts protection before personal saves and shield after");
    }
}
}
int main() {try {handlers();consumers();std::cout<<"M76 party buff handlers and consumers passed\n";return 0;}catch(const std::exception &e){std::cerr<<e.what()<<'\n';return 1;}}
