#include "games/xeen/XeenCombatRules.h"
#include "games/xeen/XeenLearnedSpellRules.h"
#include "games/xeen/XeenArmorRepair.h"
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
    for(unsigned schema=0;schema<=14;++schema)for(unsigned content=0;content<=14;++content) {
        const bool accepted=(schema>=1 && schema<=8 && schema==content) || (schema==8 && (content==9 || content==10)) || (schema==9 && (content==11 || content==12 || content==13 || content==14));
        check(xeenSupportedJourneyPair(schema,content)==accepted,"Journey admitted an unknown/crossed pair");
    }
    for(unsigned content=1;content<=14;++content) {
        const auto &policy=xeenJourneyContent(content);
        check(policy.combatCasting()==(content>=10),"Combat capability differs from explicit admitted domains");
        check(policy.armorRepair()==(content>=9),"Inherited Armor Repair capability differs");
        check(policy.serviceDays()==(content>=11),"Service-day continuation differs");
        check(policy.training()==(content>=12),"Training capability differs");
        check(policy.equipmentPurchase()==(content>=13),"Equipment purchase capability differs");
        for(unsigned id=0;id<256;++id)for(bool combat:{false,true}) {
            const bool accepted=combat ? content>=10 && (id==1 || id==26 || id==45) : content>=7 && (id==1 || id==26);
            check(bool(Rules::supportedIn(id,content,combat))==accepted,"Context support admission widened");
        }
    }
    for(unsigned id=0;id<256;++id)for(bool combat:{false,true})
        check(!Rules::supportedIn(id,15,combat),"Unknown content gained casting support");
    for(unsigned id=0;id<256;++id)check(bool(Rules::supported(id))==(id==1 || id==26 || id==45),"Effect recognition changed");
    check(Rules::cost(XeenLearnedSpell::Awaken)==1 && Rules::cost(XeenLearnedSpell::FirstAid)==1 && Rules::cost(XeenLearnedSpell::MagicArrow)==2,"Fixed costs differ");
    check(Rules::spellForSlot(XeenSpellCategory::Wizardry,25)==45 && Rules::spellForSlot(XeenSpellCategory::Druidic,23)==45,"Magic Arrow class slots differ");
    XeenPartyState party;party.party=XeenParty::fromRosterIds({6});auto &caster=party.roster.at(6);
    caster.rosterId=6;caster.characterClass=XeenCharacterClass::Sorcerer;caster.hasSpells=true;caster.currentSp=2;
    check(!Rules::eligible(party,0,25,10,true) && !Rules::eligible(party,0,25,13,true),"Absent book cast admitted");
    caster.learnedSpells=XeenCharacter::XeenLearnedSpells{};
    check(!Rules::eligible(party,0,25,10,true) && !Rules::eligible(party,0,25,13,true),"All-zero book cast admitted");
    for(unsigned raw=1;raw<256;++raw) {
        caster.learnedSpells->at(25)=raw;
        check(Rules::eligible(party,0,25,10,true) && Rules::eligible(party,0,25,11,true) && Rules::eligible(party,0,25,12,true) && Rules::eligible(party,0,25,13,true) && !Rules::eligible(party,0,25,9,true) &&
            !Rules::eligible(party,0,25,10,false) && !Rules::eligible(party,0,25,13,false) && caster.learnedSpells->at(25)==raw,"Raw knowledge/context semantics changed");
    }
    for(int sp:{-32768,-1,0,1,2,32767}) {
        caster.currentSp=sp;
        check(Rules::eligible(party,0,25,10,true)==(sp>=2) && Rules::eligible(party,0,25,13,true)==(sp>=2),"Signed SP eligibility wrong");
    }
    caster.currentSp=2;caster.conditions[8]=1;
    check(!Rules::eligible(party,0,25,10,true) && !Rules::eligible(party,0,25,13,true),"Sleeping caster admitted");
    caster.conditions.fill(0);caster.hasSpells=false;
    check(!Rules::eligible(party,0,25,10,true) && !Rules::eligible(party,0,25,13,true),"Capability ignored");
}
}
int main(){try{arrow();domainsAndBooks();std::cout<<"M39 pure rule/context/RNG tests passed\n";return 0;}catch(const std::exception &e){std::cerr<<e.what()<<'\n';return 1;}}
