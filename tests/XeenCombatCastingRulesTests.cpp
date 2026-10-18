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
void specialDamage() {
    using S=XeenLearnedSpell;using A=XeenActorStatus;using D=XeenCombatRandom::Draw;
    for(auto spell:{S::FingerOfDeath,S::HolyWord,S::MassDistortion,S::GolemStopper}) {
        const auto id=std::uint8_t(spell);
        check(XeenLearnedSpellRules::supportedIn(id,true)==spell && XeenLearnedSpellRules::supportedIn(id,false)==spell,"Special admission");
        check(XeenLearnedSpellRules::range(id)==(spell==S::GolemStopper?XeenSpellRange::Single:XeenSpellRange::Group),"Special range");
        XeenSpellDamagePreparation prep(spell,7);
        check(prep.ready() && prep.damage==0,"Special preparation must dispatch per target");
        check(xeenSpellProjectile(spell)==(spell==S::FingerOfDeath?14u:spell==S::HolyWord?13u:6u),"Special projectile");
        XeenActor actor;actor.id={23,0};actor.statistics=XeenMonsterRecord{};XeenScenePresentation scene;
        scene.spellImpact(actor,spell,prep.type);
        check(scene.animation(actor.id) && scene.animation(actor.id)->effect2==(spell==S::HolyWord?14u:spell==S::GolemStopper?3u:5u),"Special damage tint");
        {
            std::vector<D> tape;if(spell==S::FingerOfDeath)tape.push_back({1,56,7});tape.push_back({1,56,6});
            XeenCombatRandom random(tape);
            const unsigned species=spell==S::HolyWord?4:spell==S::GolemStopper?5:1;
            XeenSpellDamageCandidate candidate(7,0,0,6,spell,100,A::Sleep,species,&prep,3);
            XeenConsequenceDraw draw{random};
            const int expected=spell==S::HolyWord?3:spell==S::GolemStopper?100:5;
            check(candidate.service(draw) && candidate.damage==expected && random.position()==tape.size(),"Special zero resistance or small damage override");
        }
        for(unsigned species=0;species<7;++species)for(unsigned resource:{6u,89u})
        for(unsigned gate:{49u,50u})for(unsigned first:{1u,6u,89u,139u})for(unsigned save:{1u,6u,89u,139u})
        for(int hp:{1,3,49,50,101})for(bool clouds:{false,true}) {
            const bool finger=spell==S::FingerOfDeath;
            const bool compatible=spell==S::MassDistortion || (finger?(species==1 || species==3):spell==S::HolyWord?species==4:species==5);
            const unsigned bound=50+resource;
            std::vector<D> tape{{1,107,gate}};
            const bool firstDraw=gate==50 && compatible && finger;
            if(firstDraw)tape.push_back({1,bound,std::min(first,bound)});
            const bool dispatch=gate==50 && compatible && (!finger || first>resource);
            const bool damageStage=dispatch && !(clouds && resource==89);
            if(damageStage)tape.push_back({1,bound,std::min(save,bound)});
            XeenCombatRandom random(tape);
            XeenSpellDamageCandidate candidate(7,0,50,resource,spell,100,A::Sleep,species,&prep,hp,clouds);
            bool done=false;
            for(unsigned n=0;n<4 && !done;++n) {XeenConsequenceDraw draw{random,1};done=candidate.service(draw);}
            const int amount=finger?std::min(hp,50):spell==S::HolyWord?hp:spell==S::MassDistortion?std::max(hp/2,1):100;
            const int expected=damageStage ? ((finger || spell==S::MassDistortion) && save<=resource ? 5 : amount) : 0;
            check(done && random.position()==tape.size() && candidate.damage==expected,"Special gate/species/two-save/ID89 order");
            check(candidate.status==(damageStage?A::Physical:A::Sleep) && candidate.impact==damageStage,"Special refusal woke target or skipped damage entry");
            check(candidate.miss==(gate==50 && spell==S::HolyWord && species!=4),"Holy Word DOS zero-damage miss path");
            check(candidate.resisted==(gate<50),"Special resistance gate");
            XeenConsequenceDraw again{random};check(candidate.service(again) && random.position()==tape.size(),"Special replayed draw");
        }
    }
    // ID89 also refuses ordinary Single projectiles before waking or saving.
    XeenCombatRandom rng(std::vector<D>{{1,107,50}});
    XeenSpellDamageCandidate arrow(7,0,50,89,S::MagicArrow,0,A::Sleep);
    XeenConsequenceDraw draw{rng};check(arrow.service(draw) && !arrow.damage && !arrow.impact && arrow.status==A::Sleep && rng.position()==1,"ID89 ordinary refusal");
    check(XeenLearnedSpellRules::supported(51)==XeenLearnedSpell::PoisonVolley &&
        XeenLearnedSpellRules::range(51)==XeenSpellRange::All && xeenSpellProjectile(XeenLearnedSpell::PoisonVolley)==11,"Poison Volley admission/range/projectile");
}
void domainsAndBooks() {
    using Rules=XeenLearnedSpellRules;
    for(unsigned id=0;id<256;++id)for(bool combat:{false,true})
        check(bool(Rules::supportedIn(id,combat))==(bool(Rules::supported(id)) && (!combat || (id!=40 && id!=75))),"Context spell admission differs");
    for(unsigned id=0;id<256;++id)check(bool(Rules::supported(id))==(id==4 || id==15 || id==34 || id==41 || id==42 || id==74 || id==3 || id==12 || id==13 || id==30 || id==31 || id==53 || id==55 || id==16 || id==56 || id==58 || id==6 || id==7 || id==8 || id==9 || id==49 || id==52 || id==64 || id==67 || id==68 || id==51 || id==24 || id==29 || id==32 || id==46 || id==0 || id==5 || id==11 || id==14 || id==18 || id==22 || id==23 || id==25 || id==28 || id==35 || id==36 || id==37 || id==43 || id==47 || id==48 || id==54 || id==63 || id==65 || id==1 || id==10 || id==20 || id==26 || id==27 || id==40 || id==45 || id==50 || id==59 || id==60 || id==75 || id==2 || id==17 || id==33 || id==38 || id==61 || id==62 || id==72 || id==73),"Effect recognition differs");
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
void m74Spells() {
    using S=XeenLearnedSpell;using R=XeenSpellRange;using D=XeenCombatRandom::Draw;
    struct Rule {S spell;int base;unsigned type,lo,hi,sp,gems,pow;R range;};
    const Rule rules[]{
        {S::AcidSpray,15,5,0,0,8,0,10,R::All}, {S::ColdRay,0,4,2,4,14,4,8,R::All},
        {S::DancingSword,0,0,6,14,21,10,14,R::Group}, {S::DeadlySwarm,40,0,0,0,12,0,15,R::Group},
        {S::ElementalStorm,150,2,2,5,100,10,0,R::All}, {S::FantasticFreeze,40,4,0,0,15,5,8,R::Group},
        {S::FieryFlail,100,2,0,0,25,5,2,R::Single}, {S::Fireball,0,2,3,7,14,2,0,R::Group},
        {S::Frostbite,35,4,0,0,7,0,8,R::Single}, {S::Implosion,1000,6,0,0,100,20,6,R::Single},
        {S::Incinerate,250,2,0,0,35,10,1,R::Single}, {S::Inferno,250,2,0,0,75,10,1,R::Group},
        {S::LightningBolt,0,3,4,6,14,2,3,R::Group}, {S::MegaVolts,150,3,0,0,40,10,4,R::Group},
        {S::MoonRay,30,6,0,0,60,10,13,R::All}, {S::PrismaticLight,80,0,0,6,60,10,14,R::All},
        {S::StarBurst,500,2,0,0,200,20,15,R::All}, {S::SunRay,200,6,0,0,150,20,13,R::All}
    };
    for(const auto &r:rules)for(unsigned level:{0u,7u})for(unsigned roll=r.lo;roll<=r.hi;++roll) {
        const bool typeDraw=r.spell==S::ElementalStorm || r.spell==S::PrismaticLight;
        const int base=r.base?r.base:int(roll*level);
        const auto type=typeDraw?roll:r.type;
        std::vector<D> initial;if(r.hi)initial.push_back({r.lo,r.hi,roll});
        XeenCombatRandom empty(initial);XeenConsequenceDraw emptyDraw{empty};
        XeenSpellDamagePreparation prep(r.spell,level);
        check(prep.service(emptyDraw) && prep.damage==base && prep.type==type && empty.position()==initial.size(),"M74 empty-target preparation differs");
        check(prep.service(emptyDraw) && empty.position()==initial.size(),"M74 preparation replayed");
        const auto id=std::uint8_t(r.spell);
        check(XeenLearnedSpellRules::range(id)==r.range && XeenLearnedSpellRules::target(id)==XeenSpellTarget::Contact,"M74 range/admission differs");
        if(level==7) {const auto cost=XeenLearnedSpellRules::cost(id,level);check(cost.sp==r.sp && cost.gems==r.gems,"M74 cost differs");}
        const unsigned storm[]{0,5,9,10};
        check(xeenSpellProjectile(r.spell,type)==(r.spell==S::ElementalStorm?storm[type-2]:r.pow),"M74 projectile differs");
        XeenCombatRandom ungated(base?std::vector<D>{{1,56,7}}:std::vector<D>{});XeenConsequenceDraw ungatedDraw{ungated};
        XeenSpellDamageCandidate noGate(level,0,0,6,r.spell,0,XeenActorStatus::Physical,0,&prep);
        check(noGate.service(ungatedDraw) && noGate.damage==base && ungated.position()==unsigned(base!=0),"M74 zero resistance added a gate draw");
        XeenScenePresentation scene;XeenActor actor;actor.statistics=XeenMonsterRecord{};
        scene.spellImpact(actor,r.spell,type);constexpr unsigned tints[]{3,10,4,11,1,2,5};
        check(scene.animation(actor.id) && scene.animation(actor.id)->effect2==tints[type],"M74 damage tint differs");
        for(unsigned physical:{0u,33u,100u})for(unsigned gate:{49u,50u})for(unsigned save:{6u,7u}) {
            auto tape=initial;tape.push_back({1,100+level,gate});if(gate==50 && base)tape.push_back({1,56,save});
            XeenCombatRandom random(tape);XeenConsequenceDraw draw{random};XeenSpellDamagePreparation cast(r.spell,level);
            check(cast.service(draw),"M74 preparation incomplete");
            XeenSpellDamageCandidate hit(level,0,50,6,r.spell,physical,XeenActorStatus::Sleep,0,&cast);
            check(hit.service(draw),"M74 target incomplete");
            int expected=type==0?base*(100-physical)/100:base;if(save<=6)expected/=2;if(gate<50)expected=0;
            check(hit.damage==expected && hit.resisted==(gate<50) && random.position()==tape.size(),"M74 preparation/gate/physical/save order differs");
            check(hit.impact==(gate==50 && base!=0),"M74 zero-level/immunity entry differs");
        }
    }
    XeenPartyState party;party.party=XeenParty::fromRosterIds({0,1,2});
    for(unsigned i=0;i<3;++i){auto &c=party.roster.at(i);c.rosterId=i;c.currentHp=1;c.conditions[13]=1;}
    XeenCombatRandom random(std::vector<D>{{1,30,1},{1,30,15},{1,30,30}});XeenMoonRayHealing healing;
    for(unsigned i=0;i<3;++i){XeenConsequenceDraw draw{random,1};check(healing.service(draw,party,0)==(i==2),"Moon Ray healing continuation differs");}
    check(healing.result.effects.empty() && random.position()==3,"Moon Ray must draw even for unhealable active members");
    std::vector<D> rejected(64,{3,7,0,true});rejected.push_back({3,7,5});
    XeenCombatRandom delayed(rejected);XeenSpellDamagePreparation retained(S::Fireball,7);
    XeenConsequenceDraw first{delayed};check(!retained.service(first),"Preparation ignored draw budget");
    XeenConsequenceDraw second{delayed};check(retained.service(second) && retained.damage==35 && delayed.position()==65,"Preparation replayed after rejected draws");
    check(xeenItemUseSpell(33)==S::Frostbite && xeenSpellProjectile(S::Frostbite,4,33)==9 &&
        xeenItemUseSpell(64)==S::Incinerate && xeenSpellProjectile(S::Incinerate,2,64)==2,"DOS item variants differ");
}
void m73Spells() {
    using S=XeenLearnedSpell;using A=XeenActorStatus;using D=XeenCombatRandom::Draw;
    struct Book {unsigned id,mask;};
    for(auto b:{Book{2,5},Book{17,2},Book{33,1},Book{38,6},Book{61,6},Book{62,5},Book{72,6},Book{73,5}})
        for(unsigned category=0;category<3;++category) {
            const auto slot=XeenLearnedSpellRules::slotForSpell(XeenSpellCategory(category),b.id);
            check(bool(slot)==bool(b.mask&(1u<<category)),"M73 spell book differs");
            check(XeenLearnedSpellRules::range(b.id)==(b.id!=17?XeenSpellRange::Group:XeenSpellRange::Single),"M73 group/single range differs");
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
    // 33 shares Frostbite damage but selects the item projectile.
    constexpr unsigned expected[]{42,1,45,26,27,20,61,59,10,62,
        60,38,72,55,50,40,2,4,73,41,
        75,3,34,43,31,52,49,44,53,30,
        33,74,28,15,25,5,9,0,70,17,
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
int main(){try{specialDamage();arrow();spellDamage();m73Spells();m74Spells();itemPowerMapping();domainsAndBooks();std::cout<<"Learned spell rule/context/RNG tests passed\n";return 0;}catch(const std::exception &e){std::cerr<<e.what()<<'\n';return 1;}}
