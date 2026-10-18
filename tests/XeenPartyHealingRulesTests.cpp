#include "games/xeen/XeenCombatRules.h"
#include "games/xeen/XeenCharacterRules.h"
#include <iostream>
#include <stdexcept>
using namespace mmodern;
namespace {
void check(bool ok,const char *message) {if(!ok)throw std::runtime_error(message);}
XeenPartyState party() {
    XeenPartyState p;p.party=XeenParty::fromRosterIds({0,1,2,3,4,5});
    for(unsigned i=0;i<30;++i) {
        auto &c=p.roster.at(i);c.rosterId=i;c.characterClass=XeenCharacterClass::Cleric;
        c.permanentLevel=20;c.endurance.permanent=18;c.birthYear=592;c.currentHp=10;
    }
    return p;
}
void rules() {
    using R=XeenLearnedSpellRules;using S=XeenLearnedSpell;
    for(auto row:{std::array<unsigned,3>{6,20,5},{7,10,0},{8,12,0},{9,8,0},{49,6,0},{52,14,3},{64,35,5},{67,5,0},{68,4,0}}) {
        const auto cost=R::cost(row[0],7);check(cost.sp==row[1] && cost.gems==row[2],"M75 own-ID cost table");
    }
    auto p=party();p.roster.at(0).currentHp=0;check(p.roster.at(0).canAct(),"HP alone denied admission");auto &target=p.roster.at(1);const auto maximum=XeenCharacterRules::maxHp(target,{610});
    for(auto spell:{S::NaturesCure,S::CurePoison,S::CureDisease,S::CureParalysis,S::StoneToFlesh,S::SuppressPoison,S::SuppressDisease}) {
        const auto id=static_cast<std::uint8_t>(spell);
        check(R::supportedIn(id,true)==spell && R::supportedIn(id,false)==spell && R::target(id)==XeenSpellTarget::Member,"M75 member admission/target ID");
        check(R::prepareEffect(spell,p,{},610).effects.empty(),"Cancelled spell applied an effect");
        for(int hp:{-30,0,10,maximum+3})for(unsigned terminal:{0u,13u,14u,15u}) {
            target.currentHp=hp;target.conditions.fill(0);target.conditions[12]=1;
            if(terminal)target.conditions[terminal]=1;
            const auto result=R::prepareEffect(spell,p,1,610);
            if(spell==S::NaturesCure && terminal) {check(result.failed && result.effects.empty(),"Nature's Cure terminal guard");continue;}
            const auto &effect=result.effects.at(0);
            const int expected=spell==S::NaturesCure && hp<=maximum?std::min(hp+25,maximum):hp;
            check(!result.failed && effect.hp==expected && effect.owner==1,"Healing amount/maximum/owner");
            check(effect.conditions[12]==(expected>0 && !terminal?0:1),"Healing(0)/terminal/Unconscious order");
            if(spell==S::StoneToFlesh)check(!effect.conditions[14],"Stone to Flesh failed to clear Stone");
        }
    }
    for(auto pair:{std::pair<S,unsigned>{S::CurePoison,3},{S::CureDisease,4},{S::CureParalysis,11},{S::StoneToFlesh,14}}) {
        target.currentHp=10;target.conditions.fill(7);const auto result=R::prepareEffect(pair.first,p,1,610);
        auto expected=target.conditions;expected[pair.second]=0;
        check(result.effects[0].conditions==expected,"Cure must clear only its named condition after terminal healing refusal");
    }
    for(auto pair:{std::pair<S,unsigned>{S::SuppressPoison,3},{S::SuppressDisease,4}})
    for(unsigned count:{0u,1u,2u,3u,4u,7u,128u,255u})for(bool terminal:{false,true}) {
        target.currentHp=10;target.conditions.fill(0);target.conditions[pair.second]=count;target.conditions[12]=1;
        if(terminal)target.conditions[13]=1;
        const auto effect=R::prepareEffect(pair.first,p,1,610).effects.at(0);
        check(effect.conditions[pair.second]==(count==0?0:count<4?1:count-3),"DOS suppression count");
        check(effect.conditions[12]==(terminal?1:0) && effect.hp==10,"Suppression must heal zero even at count zero");
    }
    for(unsigned terminal:{0u,13u,14u,15u})for(unsigned level:{0u,1u,7u})for(unsigned amount:{2u,5u,12u}) {
        target.currentHp=1;target.conditions.fill(0);target.conditions[12]=1;if(terminal)target.conditions[terminal]=1;
        XeenCombatRandom rng(std::vector<XeenCombatRandom::Draw>{{2,12,amount}});XeenConsequenceDraw draw{rng};
        const auto result=R::preparePowerCure(p,1,610,level,draw);
        check(result && result->failed==bool(terminal) && rng.position()==(terminal?0u:1u),"Power Cure guard/single draw");
        if(!terminal)check(result->effects[0].hp==std::min(1+int(amount*level),maximum),"Power Cure is one scalar times caster level");
    }
    target.conditions.fill(0);target.currentHp=maximum-2;
    check(R::prepareEffect(S::NaturesCure,p,1,610).effects[0].hp==maximum,"Nature's Cure maximum clamp");
    for(int hp:{-14,-3,0,maximum-2,maximum+3}) {
        target.currentHp=hp;target.conditions[12]=hp<=0?1:0;
        XeenCombatRandom random(std::vector<XeenCombatRandom::Draw>{{2,12,2}});XeenConsequenceDraw draw{random};
        const auto effect=R::preparePowerCure(p,1,610,7,draw)->effects.at(0);
        const int expected=hp>maximum?hp:std::min(hp+14,maximum);
        check(effect.hp==expected && effect.conditions[12]==(expected>0?0:1) && random.position()==1,"Power Cure HP clamp/Unconscious/single draw");
    }
    target.conditions.fill(0);target.currentHp=1;
    std::vector<XeenCombatRandom::Draw> tape(64,{2,12,0,true});tape.push_back({2,12,5});
    XeenCombatRandom rng(tape);XeenConsequenceDraw first{rng};
    check(!R::preparePowerCure(p,1,610,7,first) && rng.position()==64,"Power Cure rejected-draw budget");
    XeenConsequenceDraw next{rng};check(R::preparePowerCure(p,1,610,7,next)->effects[0].hp==36 && rng.position()==65,"Power Cure resumes without extra accepted dice");
    for(unsigned food:{0u,65529u,65530u,65531u,65535u}) {
        p.food=food;for(unsigned i=0;i<6;++i)p.roster.at(i).conditions[13+i%3]=1;
        const auto result=R::prepareEffect(S::CreateFood,p,{},610);
        check(result.food==std::uint16_t(food+6) && result.effects.empty(),"Food must count all six and wrap as a word");
    }
    for(unsigned id:{16u,56u,58u})check(!R::supported(id),"Part B admitted early");
}
}
int main() {try {rules();std::cout<<"M75 healing guards, conditions, suppression, Power Cure draws and food passed\n";return 0;}
catch(const std::exception &e){std::cerr<<e.what()<<'\n';return 1;}}
