#include "games/xeen/XeenCombatRules.h"
#include "games/xeen/XeenLearnedSpellRules.h"
#include "games/xeen/XeenArmorRepair.h"
#include "games/xeen/XeenItemServiceRules.h"
#include "games/xeen/XeenScenePresentation.h"
#include <iostream>
#include <limits>
#include <stdexcept>
#include <type_traits>
#include <utility>
using namespace mmodern;
namespace {
static_assert(std::is_same_v<decltype(std::declval<const XeenCombat &>().cast()),std::optional<XeenCombatCastView>>,
    "Cast observations must not expose retained continuation memory");
static_assert(std::is_same_v<decltype(std::declval<const XeenCombat &>().result()),XeenCombatResult>,
    "Combat results must be detached observations");
static_assert(std::is_same_v<decltype(std::declval<const XeenCombat &>().random()),XeenCombatRandom>,
    "RNG observations must not expose a retained mutable cursor");
void check(bool v,const char *m){if(!v)throw std::runtime_error(m);}
template<class F> void rejects(F f){bool rejected=false;try{f();}catch(const std::exception &){rejected=true;}check(rejected,"invalid Arrow input accepted");}
void arrow() {
    using Draw=XeenCombatRandom::Draw;
    for(unsigned index:{0u,3u,6u,8u,9u,13u})for(unsigned save:{1u,50u}) {
        XeenCombatRandom random(std::vector<Draw>{{1,50+index,save}});
        XeenMagicArrowCandidate candidate(3,0,0,index);
        XeenConsequenceDraw draw{random};
        check(candidate.service(draw) && candidate.damage==8 && !candidate.resisted && random.position()==1,
            "Zero resistance must skip gate but consume one exact resource-index save draw");
    }
    for(unsigned resistance:{1u,50u,100u})for(unsigned gate:{1u,49u,50u,99u,100u,103u}) {
        std::vector<Draw> tape{{1,103,gate}};if(gate>=resistance)tape.push_back({1,56,6});
        XeenCombatRandom random(tape);XeenMagicArrowCandidate candidate(3,0,resistance,6);XeenConsequenceDraw draw{random};
        check(candidate.service(draw),"Arrow bounded service did not complete");
        check(candidate.resisted==(gate<resistance) && candidate.damage==(gate<resistance?0:8) && random.position()==tape.size(),
            "Resistance equality, R100, or saving outcome differs");
    }
    for(unsigned save:{6u,7u,56u}) {
        XeenCombatRandom random(std::vector<Draw>{{1,103,100},{1,56,save}});
        XeenMagicArrowCandidate candidate(3,0,100,6);XeenConsequenceDraw draw{random};
        check(candidate.service(draw) && candidate.damage==8,"Saving success/failure changed eight damage");
    }
    for(auto levels:{std::pair<int,int>{-3,0},{3,-8},{0,0},{1000,1000}}) {
        const auto bound=100+unsigned(std::max(levels.first+levels.second,0));
        XeenCombatRandom random(std::vector<Draw>{{1,bound,100},{1,50,50}});
        XeenMagicArrowCandidate candidate(levels.first,levels.second,100,0);XeenConsequenceDraw draw{random};
        check(candidate.service(draw) && candidate.damage==8,"Signed level bound or fixed damage changed");
    }
    std::vector<Draw> rejected(64,{1,103,0,true});rejected.push_back({1,103,50});rejected.push_back({1,56,56});
    XeenCombatRandom source(rejected),detached=source;
    XeenMagicArrowCandidate candidate(3,0,50,6);XeenConsequenceDraw first{detached};
    check(!candidate.service(first) && first.remaining==0 && detached.position()==64 && source.position()==0,"Rejected draws escaped budget/publication boundary");
    XeenConsequenceDraw second{detached};check(candidate.service(second) && candidate.damage==8 && detached.position()==66,"Resumed Arrow repeated accepted work");
    XeenConsequenceDraw done{detached};check(candidate.service(done) && detached.position()==66,"Completed Arrow replayed draw");
    rejects([]{XeenMagicArrowCandidate c(3,0,101,6);});
    rejects([]{XeenMagicArrowCandidate c(std::numeric_limits<int>::max(),0,0,6);});
    rejects([]{XeenMagicArrowCandidate c(3,0,0,std::numeric_limits<unsigned>::max());});
    XeenJourneyRandomState exhausted{1,1,std::numeric_limits<std::uint64_t>::max()};XeenCombatRandom random(exhausted);
    XeenMagicArrowCandidate overflow(3,0,0,0);XeenConsequenceDraw budget{random};rejects([&]{overflow.service(budget);});
    check(random.position()==std::numeric_limits<std::uint64_t>::max(),"Exhausted cursor wrapped");
    XeenMonsterRecord monster;monster.raw[39]=100;monster.raw[40]=50;
    check(monster.magicResistance()==100 && monster.physicalResistance()==50,"Resistance byte identities conflated");
}
void domainsAndBooks() {
    using Rules=XeenLearnedSpellRules;
    for(unsigned id=0;id<256;++id)for(bool combat:{false,true})
        check(bool(Rules::supportedIn(id,combat))==(bool(Rules::supported(id)) && (!combat || (id!=40 && id!=75))),"Context spell admission differs");
    for(unsigned id=0;id<256;++id)check(bool(Rules::supported(id))==(id==1 || id==10 || id==20 || id==26 || id==27 || id==40 || id==45 || id==50 || id==59 || id==60 || id==75 || id==2 || id==17 || id==33 || id==38 || id==61 || id==62 || id==72 || id==73),"Effect recognition differs");
    check(Rules::cost(XeenLearnedSpell::Awaken)==1 && Rules::cost(XeenLearnedSpell::FirstAid)==1 && Rules::cost(XeenLearnedSpell::MagicArrow)==2,"Fixed costs differ");
    check(Rules::spellForSlot(XeenSpellCategory::Wizardry,25)==45 && Rules::spellForSlot(XeenSpellCategory::Druidic,23)==45,"Magic Arrow class slots differ");
    XeenPartyState party;party.party=XeenParty::fromRosterIds({6});auto &caster=party.roster.at(6);
    caster.rosterId=6;caster.characterClass=XeenCharacterClass::Sorcerer;caster.hasSpells=true;caster.currentSp=2;
    check(!Rules::eligible(party,0,25,true),"Absent book cast admitted");
    caster.learnedSpells=XeenCharacter::XeenLearnedSpells{};
    check(!Rules::eligible(party,0,25,true),"All-zero book cast admitted");
    for(unsigned raw=1;raw<256;++raw) {
        caster.learnedSpells->at(25)=raw;
        check(Rules::eligible(party,0,25,true) && Rules::eligible(party,0,25,false) && caster.learnedSpells->at(25)==raw,"Raw knowledge/context semantics changed");
    }
    for(int sp:{-32768,-1,0,1,2,32767}) {
        caster.currentSp=sp;
        check(Rules::eligible(party,0,25,true)==(sp>=2),"Signed SP eligibility wrong");
    }
    caster.currentSp=2;caster.conditions[8]=1;
    check(!Rules::eligible(party,0,25,true),"Sleeping caster admitted");
    caster.conditions.fill(0);caster.hasSpells=false;
    check(!Rules::eligible(party,0,25,true),"Capability ignored");
}
void spellDamage() {
    using S=XeenLearnedSpell;using D=XeenCombatRandom::Draw;
    for(auto spell:{S::EnergyBlast,S::Shrapmetal,S::FlyingFist,S::Pain}) {
        const int base=spell==S::FlyingFist?6:spell==S::Pain?8:14;
        for(unsigned physical:{0u,33u,50u,100u})for(unsigned save:{6u,7u}) {
            XeenCombatRandom random(std::vector<D>{{1,107,50},{1,56,save}});
            XeenSpellDamageCandidate candidate(5,2,50,6,spell,physical);
            XeenConsequenceDraw gate{random,1};
            check(!candidate.service(gate) && random.position()==1,"Spell did not retain accepted gate across yield");
            XeenConsequenceDraw finish{random};
            int expected=spell==S::EnergyBlast?base:base*(100-physical)/100;
            if(save<=6)expected/=2;
            check(candidate.service(finish) && !candidate.resisted && candidate.damage==expected && random.position()==2,"Spell physical/save order or exact draw tape differs");
            check(candidate.service(finish) && random.position()==2,"Completed damage replayed a draw");
        }
        XeenCombatRandom blocked(std::vector<D>{{1,107,49}});
        XeenSpellDamageCandidate candidate(7,0,50,6,spell,100);XeenConsequenceDraw draw{blocked};
        check(candidate.service(draw) && candidate.resisted && candidate.damage==0 && blocked.position()==1,"Failed magic gate drew save");
        XeenCombatRandom noGate(std::vector<D>{{1,56,6}});
        XeenSpellDamageCandidate freeGate(7,0,0,6,spell);XeenConsequenceDraw freeDraw{noGate};
        check(freeGate.service(freeDraw) && freeGate.damage==base/2 && noGate.position()==1,"Zero magic resistance drew gate");
    }
    for(auto spell:{S::EnergyBlast,S::Shrapmetal}) {
        XeenCombatRandom random(std::vector<D>{{1,100,50}});
        XeenSpellDamageCandidate zero(-3,2,50,6,spell);XeenConsequenceDraw draw{random};
        check(zero.service(draw) && zero.damage==0 && random.position()==1,"Zero base damage must gate without saving draw");
    }
    check(xeenSpellProjectile(S::MagicArrow)==11 && xeenSpellProjectile(S::EnergyBlast)==13 &&
        xeenSpellProjectile(S::FlyingFist)==14 && xeenSpellProjectile(S::Pain)==14 &&
        xeenSpellProjectile(S::Shrapmetal)==15,"Original projectile resource IDs differ");
    rejects([]{XeenSpellDamageCandidate c(3,0,0,6,S::Pain,101);});
    rejects([]{XeenSpellDamageCandidate c(3,0,0,6,S::FirstAid);});
}
void m73Spells() {
    using S=XeenLearnedSpell;using A=XeenActorStatus;using D=XeenCombatRandom::Draw;
    struct Book {unsigned id,mask;};
    for(auto b:{Book{2,5},Book{17,2},Book{33,1},Book{38,6},Book{61,6},Book{62,5},Book{72,6},Book{73,5}})
        for(unsigned category=0;category<3;++category) {
            const auto slot=XeenLearnedSpellRules::slotForSpell(XeenSpellCategory(category),b.id);
            check(bool(slot)==bool(b.mask&(1u<<category)),"M73 spell book differs");
            check(XeenLearnedSpellRules::group(b.id)==(b.id!=17),"M73 group/single range differs");
            if(slot)check(XeenLearnedSpellRules::spellForSlot(XeenSpellCategory(category),*slot)==b.id &&
                XeenLearnedSpellRules::supportedIn(b.id,true) && XeenLearnedSpellRules::supportedIn(b.id,false),"M73 contexts/slot identity differ");
        }
    XeenActor actor;actor.id={23,0};actor.statistics.emplace();actor.statistics->raw[20]=20;
    actor.statistics->raw[22]=1;actor.statistics->raw[47]=8;
    XeenScenePresentation scene(1);
    for(auto entry:{std::pair<S,unsigned>{S::InsectSpray,2},{S::TurnUndead,14},{S::Sparks,11},{S::ToxicCloud,2}}) {
        scene.spellImpact(actor,entry.first);
        check(scene.animation(actor.id) && scene.animation(actor.id)->effect2==entry.second,"M73 impact FX selector differs");
    }
    struct Control {S spell;A status;unsigned sp,gems,projectile;};
    for(auto c:{Control{S::Sleep,A::Sleep,3,1,7},Control{S::BeastMaster,A::BeastMaster,5,2,7},
        Control{S::Hypnotize,A::Hypnotize,15,4,7},Control{S::DragonSleep,A::DragonSleep,10,4,7}}) {
        const auto cost=XeenLearnedSpellRules::cost(unsigned(c.spell),7);
        check(cost.sp==c.sp && cost.gems==c.gems && xeenSpellProjectile(c.spell)==c.projectile,"Control cost/projectile differs");
        for(unsigned species=0;species<=6;++species)for(unsigned r:{0u,50u})for(unsigned gate:{49u,50u})for(unsigned save:{6u,7u}) {
            const bool compatible=c.spell==S::DragonSleep?species==6:species==1 || species==3;
            const bool passes=!r || gate>=r;
            std::vector<D> tape;if(r)tape.push_back({1,107,gate});
            if(passes && compatible)tape.push_back({1,56,save});
            XeenCombatRandom rng(tape);XeenSpellDamageCandidate cast(5,2,r,6,c.spell,100,A::Hypnotize,species);
            for(unsigned n=0;n<8;++n){XeenConsequenceDraw draw{rng,1};if(cast.service(draw))break;}
            XeenConsequenceDraw done{rng};check(cast.service(done),"Control did not finish after yields");
            check(cast.status==(passes && compatible && save>6?c.status:A::Hypnotize) && !cast.damage &&
                cast.resisted==!passes && !cast.impact && rng.position()==tape.size(),"Control gate/species/save/overwrite tape differs");
        }
    }
    for(auto spell:{S::InsectSpray,S::TurnUndead,S::Sparks,S::ToxicCloud})for(unsigned species=0;species<=6;++species)
    for(unsigned level:{0u,7u})for(unsigned r:{0u,50u})for(unsigned gate:{49u,50u})for(unsigned save:{6u,7u}) {
        const bool passes=!r || gate>=r;
        const int base=spell==S::Sparks?2*level:spell==S::ToxicCloud?10:
            spell==S::InsectSpray?(species==2?25:0):(species==4?25:0);
        std::vector<D> tape;if(r)tape.push_back({1,100+level,gate});if(passes && base)tape.push_back({1,56,save});
        XeenCombatRandom rng(tape);XeenSpellDamageCandidate cast(level,0,r,6,spell,100,A::DragonSleep,species);
        for(unsigned n=0;n<8;++n){XeenConsequenceDraw draw{rng,1};if(cast.service(draw))break;}
        XeenConsequenceDraw done{rng};check(cast.service(done),"Damage did not finish after yields");
        const bool halves=spell==S::Sparks || spell==S::ToxicCloud;
        check(cast.damage==(passes?(halves && save<=6?base/2:base):0) && rng.position()==tape.size() &&
            cast.status==(passes && base?A::Physical:A::DragonSleep),"Damage formula/species/save/wake tape differs");
        const auto cost=XeenLearnedSpellRules::cost(unsigned(spell),level);
        check(cost.sp==(spell==S::Sparks?level:spell==S::ToxicCloud?4u:5u) && cost.gems==(spell==S::TurnUndead?2u:1u),"Damage cost differs");
    }
    for(auto spell:{S::Sparks,S::Sleep,S::InsectSpray,S::TurnUndead}) {
        XeenCombatRandom rng(std::vector<D>{{1,100,50}});
        XeenSpellDamageCandidate noSave(0,0,50,6,spell,0,A::Sleep,0);
        XeenConsequenceDraw one{rng,1};
        check(noSave.service(one) && rng.position()==1 && !noSave.impact && noSave.status==A::Sleep,
            "Draw-free dispatch must finish at the exact gate budget without another presentation slice");
    }
    check(xeenSpellProjectile(S::InsectSpray)==10 && xeenSpellProjectile(S::ToxicCloud)==10 &&
        xeenSpellProjectile(S::Sparks)==5 && xeenSpellProjectile(S::TurnUndead)==13,"M73 projectile IDs differ");
}
void itemPowerMapping() {
    // Independent transcription of magic.md's named power ranges, ten per row.
    // 33 is the special Frostbite handler, represented by no ordinary ID.
    constexpr unsigned expected[]{42,1,45,26,27,20,61,59,10,62,
        60,38,72,55,50,40,2,4,73,41,
        75,3,34,43,31,52,49,44,53,30,
        33,74,76,15,25,5,9,0,70,17,
        7,69,24,8,29,51,14,66,12,13,
        6,23,57,22,71,64,56,21,11,48,
        46,54,19,36,32,58,18,47,37,65,
        35,63,16};
    for(unsigned power=1;power<=73;++power) {
        const auto raw=xeenItemPowerSpellId(power);
        check(expected[power-1]==76?!raw:raw==expected[power-1],"Original item-power destination differs");
        check(xeenItemUseSpell(power)==(raw?XeenLearnedSpellRules::supportedIn(*raw,false):std::nullopt),"Unsupported item effect was admitted/misrouted");
    }
    check(!xeenItemPowerSpellId(0) && !xeenItemPowerSpellId(74) && !xeenItemUseSpell(255),"Invalid item power admitted");
}

}
int main(){try{arrow();spellDamage();m73Spells();itemPowerMapping();domainsAndBooks();std::cout<<"Learned spell rule/context/RNG tests passed\n";return 0;}catch(const std::exception &e){std::cerr<<e.what()<<'\n';return 1;}}
