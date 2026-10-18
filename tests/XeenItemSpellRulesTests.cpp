#include "games/xeen/XeenLearnedSpellRules.h"
#include "games/xeen/XeenItemServiceRules.h"
#include "games/xeen/XeenCombatRules.h"
#include <iostream>
#include <stdexcept>
using namespace mmodern;
namespace {
void check(bool value,const char *message) {if(!value)throw std::runtime_error(message);}
using S=XeenLearnedSpell;using C=XeenInventoryCategory;using R=XeenLearnedSpellRules;
XeenItemSpellPreparation run(S spell,unsigned level,C category,XeenItem item,XeenCombatRandom &random) {
    XeenItemSpellPreparation work(spell,level,category,item);
    for(unsigned i=0;i<100 && !work.complete;++i) {XeenConsequenceDraw draw{random,1,{}};work.service(draw);}
    check(work.complete,"Item handler must finish with one-draw slices");return work;
}
void admission() {
    for(unsigned id:{19u,39u,57u}) {
        check(R::supported(id) && R::target(id)==XeenSpellTarget::Item,"Supported item handler/target");
        XeenCharacter c;c.characterClass=XeenCharacterClass::Sorcerer;c.hasSpells=true;
        c.learnedSpells.emplace();c.learnedSpells->fill(1);c.permanentLevel=7;c.currentSp=1000;
        c.currentSpell=*R::slotForSpell(XeenSpellCategory::Wizardry,id);
        check(R::check(c,c.currentSpell,1000,false)==XeenQuickSpellCheck::Supported,"Exploration learned admission");
        check(R::quickSpellCheck(c,1000)==(id==19?XeenQuickSpellCheck::CombatForbidden:XeenQuickSpellCheck::Supported),"Quick Fight admission");
        check(bool(R::supportedIn(id,true))==(id!=19),"Learned combat admission");
    }
    check(R::cost(19,7).sp==30 && R::cost(19,7).gems==20 && R::cost(57,7).sp==15 && R::cost(57,7).gems==10 && R::cost(39,7).sp==20 && R::cost(39,7).gems==10,"Own-ID costs/refunds");
    for(bool combat:{false,true}) {
        check(xeenItemUseSpell(63,combat)==S::EnchantItem && xeenItemUseSpell(53,combat)==S::RechargeItem,"Item power combat admission differs from learned Enchant");
        XeenCombatRandom random(1);const auto recharge=run(*xeenItemUseSpell(53,combat),7,C::Miscellaneous,{1,4,0x82,9},random);
        check(recharge.item.state==0xbf && random.position()==1,"Admitted item Recharge effect, including broken fill");
        XeenCombatRandom other(1);const auto enchant=run(*xeenItemUseSpell(63,combat),1,C::Weapons,{0,1,0,2},other);
        check(!enchant.failed && enchant.item.id==1 && enchant.item.frame==2 && other.position()==5,"Admitted item Enchant effect");
    }
}
void enchant() {
    for(C category:{C::Weapons,C::Armor,C::Accessories,C::Miscellaneous})
        for(unsigned material:{0u,1u,37u})for(unsigned state:{0u,1u,0x40u,0x80u,0xc0u})for(unsigned id:{0u,1u,34u,35u}) {
            const XeenItem item{std::uint8_t(material),std::uint8_t(id),std::uint8_t(state),7};
            XeenCombatRandom random(121);const auto work=run(S::EnchantItem,1,category,item,random);
            const bool eligible=(category==C::Weapons || category==C::Armor) && id && !(category==C::Weapons && id==34) && !material && !state;
            check(work.failed==!eligible,"Enchant category/material/state/ID eligibility");
            check(random.position()==(eligible?5u:1u),"DOS level draw before every eligibility refusal; full generator even zero properties");
            check(xeenSameItem(work.item,item),"Level-one zero-property copy succeeds and keeps selected base and frame");
        }
    // Exact M71 ordinary generator tape: category, subcategory, base, enchantment,
    // selector, material value, effectiveness test and effectiveness value.
    const std::vector<XeenCombatRandom::Draw> generator{{1,100,10},{1,100,10},{1,6,4},{1,100,50},{1,100,1},{1,4,2},{1,20,10},{1,6,6}};
    auto tape=generator;tape.insert(tape.begin(),{1,3,2});XeenCombatRandom enchantRandom(tape),ordinaryRandom(generator);
    const auto work=run(S::EnchantItem,12,C::Weapons,{0,7,0,4},enchantRandom);
    XeenItemCandidate ordinary(2);XeenConsequenceDraw draw{ordinaryRandom,64,{}};
    check(ordinary.service(draw),"M71 tape completes");
    check(work.item.material==ordinary.item().material && work.item.state==ordinary.item().state && work.item.id==7 && work.item.frame==4,"Enchant copies only the shared generator material/state");
    check(enchantRandom.position()==ordinaryRandom.position()+1 && work.item.state==6,"Enchant retains every M71 draw");
    auto suspended=generator;
    suspended.insert(suspended.begin(),64,{1,100,0,true});suspended.insert(suspended.begin(),{1,3,2});
    XeenCombatRandom suspendedRandom(suspended);XeenItemSpellPreparation pending(S::EnchantItem,12,C::Weapons,{0,7,0,4});
    XeenConsequenceDraw first{suspendedRandom,64,{}};
    check(!pending.service(first) && suspendedRandom.position()==64,"Rejected generator draws suspend after the single level roll");
    XeenConsequenceDraw next{suspendedRandom,64,{}};
    check(pending.service(next) && suspendedRandom.position()==73 && pending.item.material==work.item.material && pending.item.state==work.item.state,"Resume retains Enchant level and shared generator state");
    for(bool armor:{false,true})for(unsigned level:{1u,7u,24u,100u})for(unsigned seed=1;seed<=120;++seed) {
        XeenCombatRandom actual(seed),expected(seed);const auto amount=expected.draw(1,level/5+1);
        XeenItemCandidate generator(std::min(*amount,5u),armor?XeenItemCandidate::Enchant::Armor:XeenItemCandidate::Enchant::Weapon);
        XeenConsequenceDraw draws{expected,64,{}};check(generator.service(draws),"Forced generator completes");
        const auto result=run(S::EnchantItem,level,armor?C::Armor:C::Weapons,{0,1,0,3},actual);
        check(actual==expected && result.item.material==generator.item().material && result.item.state==generator.item().state,"Caster level/cap and forced shared generator draws");
    }
}
void recharge() {
    for(unsigned flags:{0u,0x40u,0x80u,0xc0u})for(unsigned count:{0u,2u,58u,62u,63u})for(unsigned roll=1;roll<=6;++roll) {
        XeenCombatRandom random(std::vector<XeenCombatRandom::Draw>{{1,6,roll}});
        const auto work=run(S::RechargeItem,7,C::Miscellaneous,{9,4,std::uint8_t(flags|count),8},random);
        const auto expected=flags|((flags&0x80)?63u:std::min(63u,count+roll));
        check(!work.failed && work.item.state==expected && work.item.material==9 && work.item.id==4 && work.item.frame==8,"Recharge one draw, charge cap, broken fill, cursed normal addition, flags preserved");
        check(random.position()==1,"No Recharge break/destruction draw");
    }
    for(C category:{C::Weapons,C::Armor,C::Accessories,C::Miscellaneous})for(unsigned material:{0u,1u,9u,10u})for(unsigned power:{4u,53u}) {
        XeenCombatRandom random(1);const XeenItem before{std::uint8_t(material),std::uint8_t(power),2,0};
        const auto work=run(S::RechargeItem,7,category,before,random);
        const bool eligible=category==C::Miscellaneous && material && material<=9 && power!=53;
        check(work.failed==!eligible && random.position()==unsigned(eligible),"Recharge eligibility before its only draw");
        if(!eligible)check(xeenSameItem(work.item,before),"Refused Recharge preserves item");
    }
}
void gold() {
    for(C category:{C::Weapons,C::Armor,C::Accessories,C::Miscellaneous})for(unsigned state:{0u,12u,0x40u,0x80u,0xffu})for(unsigned frame:{0u,3u}) {
        const XeenItem before{std::uint8_t(category==C::Miscellaneous?1:0),1,std::uint8_t(state),std::uint8_t(frame)};
        XeenCombatRandom random(1);const auto result=run(S::ItemToGold,7,category,before,random);
        check(!result.failed && !result.item.id && !result.item.material && result.gold==*xeenItemPrice(category,before,XeenItemService::Buy),"Gold uses full shared DOS price ignoring flags, charges and equipment");
        check(!random.position(),"Gold draws nothing");
    }
    XeenCombatRandom random(1);check(run(S::ItemToGold,7,C::Weapons,{0,34,0,0},random).failed,"Gold excludes weapon exactly 34");
    check(run(S::ItemToGold,7,C::Weapons,{0,1,0,0},random).gold==50,"Plain weapon full price");
    check(run(S::ItemToGold,7,C::Weapons,{59,1,255,3},random).gold==250,"DOS attribute price addend");
    check(run(S::ItemToGold,7,C::Weapons,{37,7,0,0},random).gold==1,"Gold minimum after division");
    check(run(S::ItemToGold,7,C::Armor,{37,1,0,0},random).gold==2,"DOS metal integer division");
}
}
int main() {try {admission();enchant();recharge();gold();std::cout<<"M76 item handlers, DOS eligibility, prices and draw order passed\n";return 0;}catch(const std::exception &e){std::cerr<<e.what()<<'\n';return 1;}}
