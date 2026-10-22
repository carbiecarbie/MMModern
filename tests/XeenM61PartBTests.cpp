#include "XeenTestInstallation.h"
#include "XeenTrainingTestSupport.h"
#include "games/xeen/XeenItemServiceRules.h"
#include "games/xeen/XeenJumpRules.h"
#include "games/xeen/XeenItemGeneration.h"
#include "XeenM42Evidence.h"
#include "XeenMonsterTestAccess.h"
#include <queue>
#include <exception>
#include <iostream>
#include <sstream>
using namespace training_test;
namespace mmodern {
struct XeenMonsterAbilityTestAccess {
    static auto ranged(const XeenEncounterFlow &flow) { return flow._rangedObservation; }
    static auto portrait(const XeenEncounterFlow &flow) { return flow.rangedPortrait(); }
};
}
namespace {
constexpr unsigned powers[]{2,3,4,5,6,7,8,9,10,11,12,13,15,16,17,19,21,31,37,40,43,45,61,65};
void tick(Fixture &f) {
    f.now+=125;f.flow->beginCycle(++f.cycle);
    if(auto frame=f.flow->updatePresentation())f.present(*frame);
}
void quiet(Fixture &f) {
    for(unsigned n=0;n<1000 && !f.flow->canSave();++n) {
        const auto input=f.flow->inputContext(f.flow->frame().presentation());
        if(f.flow->presentationGeneration() || f.w.sessionState().journeyActivity()==XeenJourneyActivity::Reward ||
            (input.dialog && input.dialog->anyKey))f.act(AcknowledgeAction{});else tick(f);
    }
    check(f.flow->canSave(),"Item did not settle to Quiet");
}
XeenSaveSnapshot source(Inputs &in,unsigned power,bool indoor,unsigned charges=1) {
    auto s=in.service();s.camera=indoor?XeenCamera{28,18,4,XeenDirection::West}:XeenCamera{23,9,11,XeenDirection::East};
    for(auto *actors:{&s.map(23).monsters->slots,&s.map(28).monsters->slots})for(auto &a:*actors) {
        a.x=a.y=-128;a.lifecycle=XeenActorLifecycle::Defeated;a.accounted=true;
    }
    for(auto &c:s.characters)c.skills[4]=0;
    auto &c=s.characters[s.activeRosterIds[0]];
    c.characterClass=XeenCharacterClass::Knight;c.hasSpells=false;c.learnedSpells.emplace();c.learnedSpells->fill(0);c.currentSp=0;
    c.miscellaneous={};c.miscellaneous[0]={1,std::uint8_t(power),std::uint8_t(charges),17};
    c.miscellaneous[1]={2,1,3,19}; // exact compaction witness
    s.journey->purse->gems=0;s.journey->context->ctr24=23;save_test::clearExplored(s);
    return s;
}
void key(Fixture &f,unsigned k,bool mouse=false) {
    const auto dialog=f.flow->inputContext(f.flow->frame().presentation()).dialog;
    check(bool(dialog),"Item dialog absent");
    if(mouse) {
        const auto hit=std::find_if(dialog->hits.begin(),dialog->hits.end(),[&](auto h){return h.key==k;});
        check(hit!=dialog->hits.end(),"Item mouse control absent");
        f.act(*dialog->click(hit->left,hit->top));
    } else f.act(DialogKeyAction{k});
}
void open(Fixture &f,bool mouse=false) {
    f.act(SelectMemberAction{0});key(f,'i',mouse);key(f,'m',mouse);key(f,'1',mouse);
}
void replay(Fixture &f,const IndexedFrame::Presentation &frame,std::optional<std::uint64_t> input,PlayerAction action) {
    const auto &c=f.p.roster.at(f.p.party.activeRosterIds()[0]);const auto items=c.miscellaneous;
    const auto rng=f.w.sessionState().journeyRandom();const auto context=*f.p.encounterContext;
    f.flow->beginCycle(++f.cycle);f.present(f.flow->handle(action,input,frame));
    for(unsigned n=0;n<9;++n)check(xeenSameItem(items[n],c.miscellaneous[n]),"Stale input replayed a charge");
    check(rng==f.w.sessionState().journeyRandom() && context==*f.p.encounterContext,"Stale input changed RNG/time");
}
void effects(Inputs &in) {
    for(bool indoor:{false,true})for(bool mouse:{false,true})for(unsigned power:powers)for(unsigned charges:{1u,2u}) {
        auto s=source(in,power,indoor,charges);auto &target=s.characters[s.activeRosterIds[1]];
        target.currentHp=1;target.conditions[2]=3;target.conditions[3]=2;target.conditions[8]=1;
        Fixture f(in,s);const auto rng=f.w.sessionState().journeyRandom();const auto context=*f.p.encounterContext;
        XeenCombatRandom expected(*rng);if(power==6)while(!expected.draw(2,6)){}
        open(f,mouse);const auto before=f.flow->frame().presentation();const auto input=f.flow->displayedInput();key(f,'u',mouse);
        const bool member=power==4 || power==8 || power==9 || power==37;
        if(member) {
            check(f.flow->encounter()->itemUseNeedsTarget(),"Supported item target missing");
            const auto items=f.p.roster.at(s.activeRosterIds[0]).miscellaneous;
            check(charges==1?xeenSameItem(items[0],s.characters[s.activeRosterIds[0]].miscellaneous[1]):items[0].state==1,
                "Charge/removal did not precede target prompt");
            replay(f,before,input,UseItemAction{});
            const auto targetFrame=f.flow->frame().presentation();const auto targetInput=f.flow->displayedInput();
            key(f,InputKey::F1+1,mouse);replay(f,targetFrame,targetInput,SelectMemberAction{1});
        }
        quiet(f);
        const auto &c=f.p.roster.at(s.activeRosterIds[0]);
        check(c.currentSp==0 && !c.hasSpells && std::all_of(c.learnedSpells->begin(),c.learnedSpells->end(),[](auto v){return v==0;}) && f.p.purse->gems==0,"Item required/debited a book/SP/gem");
        check(*f.p.encounterContext==context || (power==21 && f.p.encounterContext->minutes==context.minutes),"Item charged time/counter");
        check(f.p.encounterContext->minutes==context.minutes && f.p.encounterContext->ctr24==23,"Item charged indoor/outdoor time");
        check(f.flow->encounter()->result().movementOpportunities==1 && !f.flow->encounter()->state().pending(),"Item response was not exactly one");
        check(f.w.sessionState().journeyRandom()==expected.continuation(),"Empty/non-damage item preparation RNG differs");
        check(charges==1?xeenSameItem(c.miscellaneous[0],s.characters[s.activeRosterIds[0]].miscellaneous[1]):c.miscellaneous[0].state==1,"Settlement repeated charge/compaction");
        const auto &t=f.p.roster.at(s.activeRosterIds[1]);
        if(power==2)check(!t.conditions[8],"Awaken retained Sleep");
        if(power==4 || power==9)check(t.currentHp==(power==4?7:16),"Shared healing amount differs");
        if(power==8)check(!t.conditions[2],"Revitalize retained Weak");
        if(power==37)check(!t.conditions[3],"Antidote retained Poison");
        if(power==21)check(f.p.encounterContext->effects[XeenPartyEffect::WizardEye]==1 && f.p.encounterContext->effects[XeenPartyEffect::AutomapOn]==0,"Eye flags differ");
        if(power==16 && !indoor)check(f.c.x==11 && f.c.y==11,"Jump destination differs");
        const auto saved=XeenSaveFormat::encode(f.snapshot());Fixture restored(in,XeenSaveFormat::decode(saved));
        check(saved==XeenSaveFormat::encode(restored.snapshot()),"Item save/load replayed effects/response");
    }
}
void refusal(Inputs &in) {
    constexpr unsigned damagePowers[]{24,33,35,36,38,46,47,52,54,59,60,62,64,67,68,69,70,71,72};
    for(unsigned power=0;power<75;++power) {
        const bool m75=power==26 || power==27 || power==41 || power==44 || power==51 || power==56 || power==57 || power==66 || power==73;
        const bool partyBuff=power==22 || power==30 || power==25 || power==29 || power==14 || power==49 || power==50;
        const bool senses=power==1 || power==20 || power==32 || power==18 || power==34 || power==23;
        const bool travel=power==28 || power==55 || power==39 || power==42 || power==48 || power==58;
        const bool supported=power==63 || power==53 || senses || partyBuff || m75 || travel || std::find(std::begin(powers),std::end(powers),power)!=std::end(powers) ||
            std::find(std::begin(damagePowers),std::end(damagePowers),power)!=std::end(damagePowers);
        check(xeenItemUseSupported({1,std::uint8_t(power),1,0})==supported,"Power support boundary differs");
        if(supported)continue;
        auto s=source(in,power,true);Fixture f(in,s);const auto before=XeenSaveFormat::encode(f.snapshot());
        open(f);key(f,'u');check(!f.flow->encounter()->itemUseActive(),"Deferred/later power admitted");
        check(f.flow->inputContext(f.flow->frame().presentation()).dialog->anyKey,"Unsupported power notice absent");
        f.act(AcknowledgeAction{});key(f,InputKey::Escape);key(f,InputKey::Escape);quiet(f);
        check(before==XeenSaveFormat::encode(f.snapshot()),"Refused power spent item/time/RNG");
    }
    for(bool indoor:{false,true})for(unsigned power:{4u,8u,9u,37u})for(bool cancel:{false,true}) {
        auto s=source(in,power,indoor);auto &target=s.characters[s.activeRosterIds[1]];target.currentHp=0;target.conditions[13]=1;target.conditions[2]=3;
        Fixture f(in,s);const auto context=*f.p.encounterContext;const auto rng=f.w.sessionState().journeyRandom();
        open(f);key(f,'u');f.act(cancel?PlayerAction{CancelInteractionAction{}}:PlayerAction{SelectMemberAction{1}});quiet(f);
        check(f.p.roster.at(s.activeRosterIds[0]).miscellaneous[0].id==1,"Admitted cancellation/failure refunded charge");
        check(*f.p.encounterContext==context && f.w.sessionState().journeyRandom()==rng &&
            f.flow->encounter()->result().movementOpportunities==1,"Admitted cancellation/failure lost response or charged time");
        check(f.p.roster.at(s.activeRosterIds[1]).currentHp==0,"Terminal member healed");
        check(f.p.roster.at(s.activeRosterIds[0]).currentSp==(cancel?(power==4?1:power==37?8:0):0),"Verified healing-item cancellation, not a failed effect, refunds SP");
    }
    auto s=source(in,16,false);s.camera={23,15,11,XeenDirection::East};
    const auto original=in.mapLoader();
    Fixture f(in,s,false,nullptr,[original](auto id){auto m=original(id);if(id==XeenMapIdentity(23))m.geometry.neighbors[1]=0;return m;});
    const auto before=XeenSaveFormat::encode(f.snapshot());open(f);key(f,'u');
    check(!f.flow->encounter()->itemUseActive(),"Unsafe item Jump admitted");f.act(AcknowledgeAction{});key(f,InputKey::Escape);key(f,InputKey::Escape);quiet(f);
    check(before==XeenSaveFormat::encode(f.snapshot()),"Unsafe item Jump spent anything");
    s=source(in,4,true);Fixture cancelled(in,s);const auto initial=XeenSaveFormat::encode(cancelled.snapshot());
    open(cancelled);key(cancelled,InputKey::Escape);key(cancelled,InputKey::Escape);quiet(cancelled);
    check(initial==XeenSaveFormat::encode(cancelled.snapshot()),"Pre-admission cancellation changed state");
}
void itemCancellation(Inputs &in) {
    for(bool indoor:{false,true})for(unsigned power:{4u,8u,9u,37u})for(unsigned sp:{0u,7u})for(unsigned charges:{1u,2u}) {
        auto s=source(in,power,indoor,charges);const auto owner=s.activeRosterIds[0];
        s.characters[owner].currentSp=sp;
        Fixture f(in,s);const auto context=*f.p.encounterContext;const auto rng=f.w.sessionState().journeyRandom();
        check(XeenCharacterRules::maxSp(f.p.roster.at(owner),{context.year})==0,"Cancellation fixture must have zero maximum SP");
        open(f);key(f,'u');const auto frame=f.flow->frame().presentation();const auto input=f.flow->displayedInput();
        key(f,InputKey::Escape);const auto expected=sp+(power==4?1:power==37?8:0);
        check(f.p.roster.at(owner).currentSp==expected,"First Aid and Cure Poison Escape grant their uncapped own-cost refunds");
        replay(f,frame,input,CancelInteractionAction{});quiet(f);
        const auto &user=f.p.roster.at(owner);
        check(user.currentSp==expected && f.p.purse->gems==0,"Cancellation replay changed refund or gems");
        check(charges==1?xeenSameItem(user.miscellaneous[0],s.characters[owner].miscellaneous[1]):user.miscellaneous[0].state==1,
            "Escape must spend exactly one charge and compact exhausted item");
        check(*f.p.encounterContext==context && f.w.sessionState().journeyRandom()==rng &&
            f.flow->encounter()->result().movementOpportunities==1,"Escape changed time/RNG or repeated response");
        const auto saved=XeenSaveFormat::encode(f.snapshot());Fixture restored(in,XeenSaveFormat::decode(saved));
        check(saved==XeenSaveFormat::encode(restored.snapshot()),"Item refund did not restore exactly");
    }
}
void miscTreasure(Inputs &in) {
    auto s=source(in,4,false);const auto owner=s.activeRosterIds[0];
    s.characters[owner].miscellaneous={};
    XeenTreasureStaging treasure;treasure.gold=10;treasure.ready=true;treasure.miscellaneous[0]={3,8,7,0};
    // A visible distant survivor keeps pending treasure at a legal Quiet boundary.
    auto &survivor=s.map(23).monsters->slots[8];survivor.x=12;survivor.y=11;
    survivor.lifecycle=XeenActorLifecycle::Present;survivor.accounted=false;
    const auto gold=s.journey->purse->gold;const auto rng=s.journey->random;
    Fixture f(in,s);XeenMonsterTestAccess::stageTreasureLive(*f.flow,f.w,treasure);f.act(NavigationAction::TurnLeft);quiet(f);
    check(xeenSameItem(f.p.roster.at(owner).miscellaneous[0],treasure.miscellaneous[0]),"Misc receipt did not publish original item bytes");
    check(f.p.purse->gold==gold+10 && !f.w.treasureStaging().value().storedItems() &&
        !f.w.treasureStaging().value().pending() && f.w.sessionState().journeyRandom()==rng,"Misc receipt repeated gold or RNG");
    const auto saved=XeenSaveFormat::encode(f.snapshot());Fixture restored(in,XeenSaveFormat::decode(saved));
    check(saved==XeenSaveFormat::encode(restored.snapshot()),"Delivered Misc did not restore exactly");
}
void impacts(Inputs &in) {
    for(unsigned power:{3u,5u,6u,11u,15u,61u})for(unsigned distance:{1u,2u,3u}) {
        auto s=source(in,power,true);s.camera={28,15,0,XeenDirection::North};
        const auto spell=*xeenItemUseSpell(power);const unsigned count=power==11 || power==15 || power==61?3:1;
        for(unsigned n=0;n<count;++n) {
            auto &a=s.map(28).monsters->slots.at(35+n);a.x=15;a.y=distance;a.lifecycle=XeenActorLifecycle::Present;a.accounted=false;
        }
        Fixture f(in,s);
        if(power==61)for(unsigned n=0;n<count;++n)XeenMonsterTestAccess::setLive(*f.flow,f.w,28,35+n,1,true);
        XeenCombatRandom expected(*f.w.sessionState().journeyRandom());
        const auto &caster=f.p.roster.at(s.activeRosterIds[0]);
        XeenSpellDamagePreparation preparation(spell,caster.currentLevel());
        XeenConsequenceDraw preparationDraw{expected};check(preparation.service(preparationDraw),"Item preparation did not settle");
        for(unsigned n=0;n<count;++n) {
            const auto &a=f.w.sessionState().regionalActors(28).at(35+n);
            XeenSpellDamageCandidate damage(caster.permanentLevel,caster.temporaryLevel,a.statistics->magicResistance(),a.original.resourceId,spell,a.statistics->physicalResistance(),a.status,a.statistics->monsterType(),&preparation,a.hp);
            XeenConsequenceDraw draw{expected};check(damage.service(draw) && damage.damage>0,"Lethal item fixture failed");
        }
        open(f);key(f,'u');quiet(f);
        for(unsigned n=0;n<count;++n)check(f.w.sessionState().regionalActors(28).at(35+n).lifecycle==XeenActorLifecycle::Defeated,"Item projectile missed/replayed impact");
        check(f.w.sessionState().journeyRandom()==expected.continuation(),"Item damage/resistance draw order differs from shared casting");
        check(f.flow->encounter()->result().movementOpportunities==1 && f.p.encounterContext->minutes==s.journey->context->minutes,"Projectile response/time differs");
        const auto saved=XeenSaveFormat::encode(f.snapshot());Fixture restored(in,XeenSaveFormat::decode(saved));
        check(saved==XeenSaveFormat::encode(restored.snapshot()),"Item projectile restore replayed damage/rewards");
    }
}
void targetRetry(Inputs &in) {
    for(unsigned power:{4u,8u,9u,37u}) {
        auto s=source(in,power,true);s.characters[s.activeRosterIds[1]].currentHp=1;
        s.characters[s.activeRosterIds[1]].conditions[2]=3;s.characters[s.activeRosterIds[1]].conditions[3]=2;
        Fixture f(in,s);open(f);unsigned retries=0;
        const auto before=f.flow->frame().presentation();const auto input=f.flow->displayedInput();
        f.flow->beforeEncounterFrameCopy=[&] {
            if(!retries && f.flow->inventorySelection().mode==XeenInventoryMode::UseTarget) {++retries;throw std::bad_alloc();}
        };
        key(f,'u');f.flow->beforeEncounterFrameCopy={};
        check(retries==1 && f.flow->encounter()->itemUseNeedsTarget(),"Target presentation retry lost the admitted use");
        check(xeenSameItem(f.p.roster.at(s.activeRosterIds[0]).miscellaneous[0],s.characters[s.activeRosterIds[0]].miscellaneous[1]),"Target presentation retry repeated/refunded removal");
        replay(f,before,input,SelectMemberAction{1});key(f,InputKey::F1+1);quiet(f);
        check(f.flow->encounter()->result().movementOpportunities==1 && f.p.encounterContext->minutes==s.journey->context->minutes,"Target retry repeated response/time");
        const auto &target=f.p.roster.at(s.activeRosterIds[1]);
        check(power==4?target.currentHp==7:power==9?target.currentHp==16:power==8?!target.conditions[2]:!target.conditions[3],"Target retry lost/shared effect differs");
    }
}
void castingParity(Inputs &in) {
    for(unsigned power:powers) {
        if(power==37)continue;
        const auto spell=*xeenItemUseSpell(power);auto s=source(in,power,false,2);
        auto &c=s.characters[s.activeRosterIds[0]];c.characterClass=spell==XeenLearnedSpell::DragonSleep?XeenCharacterClass::Sorcerer:spell==XeenLearnedSpell::Hypnotize?XeenCharacterClass::Cleric:XeenCharacterClass::Druid;c.hasSpells=true;c.currentSp=100;c.learnedSpells.emplace();
        for(auto *items:{&c.weapons,&c.armor,&c.accessories})for(auto &item:*items)item.frame=0;
        if(!XeenLearnedSpellRules::slotForSpell(*XeenLearnedSpellRules::categoryForClass(c.characterClass),std::uint8_t(spell)))
            c.characterClass=XeenLearnedSpellRules::slotForSpell(XeenSpellCategory::Wizardry,std::uint8_t(spell))?XeenCharacterClass::Sorcerer:XeenCharacterClass::Cleric;
        c.currentSpell=*XeenLearnedSpellRules::slotForSpell(*XeenLearnedSpellRules::categoryForClass(c.characterClass),static_cast<std::uint8_t>(spell));c.learnedSpells->at(c.currentSpell)=1;
        s.journey->purse->gems=100;s.characters[s.activeRosterIds[1]].currentHp=1;s.characters[s.activeRosterIds[1]].conditions[2]=3;s.characters[s.activeRosterIds[1]].conditions[8]=1;
        Fixture item(in,s),cast(in,s);open(item);key(item,'u');
        if(item.flow->encounter()->itemUseNeedsTarget())item.act(SelectMemberAction{1});quiet(item);
        cast.act(CastSpellAction{});cast.act(SelectMemberAction{0});cast.act(DialogKeyAction{'c'});
        if(XeenLearnedSpellRules::target(static_cast<std::uint8_t>(spell))==XeenSpellTarget::Member)cast.act(SelectMemberAction{1});quiet(cast);
        for(auto owner:s.activeRosterIds)check(item.p.roster.at(owner).currentHp==cast.p.roster.at(owner).currentHp &&
            item.p.roster.at(owner).conditions==cast.p.roster.at(owner).conditions,"Item/casting HP or condition parity differs");
        check(item.c.mapId==cast.c.mapId && item.c.x==cast.c.x && item.c.y==cast.c.y && item.c.direction==cast.c.direction && item.p.encounterContext->effects==cast.p.encounterContext->effects &&
            item.w.sessionState().journeyRandom()==cast.w.sessionState().journeyRandom(),"Item/casting destination/effect/RNG parity differs");
        check(item.p.roster.at(s.activeRosterIds[0]).currentSp==100 && item.p.purse->gems==100 &&
            item.p.encounterContext->minutes==s.journey->context->minutes,"Parity item used casting costs/time");
    }
}
void survivingResponse(Inputs &in) {
    for(bool retry:{false,true}) {
        auto s=source(in,3,true,2);s.camera={28,15,0,XeenDirection::North};
        const auto objects=in.maps.loadObjects(in.assets,28);
        unsigned actorSlot=0;
        for(unsigned n=1;n<objects.entities.monsters.size();++n)
            if(in.statistics.at(objects.entities.monsters[n].resourceId).baseHp()>in.statistics.at(objects.entities.monsters[actorSlot].resourceId).baseHp())actorSlot=n;
        auto &a=s.map(28).monsters->slots.at(actorSlot);
        a.x=15;a.y=3;a.lifecycle=XeenActorLifecycle::Present;a.accounted=false;
        Fixture f(in,s);const auto live=f.w.sessionState().regionalActors(28);std::vector<XeenActor> actors(live.begin(),live.end());const auto &caster=f.p.roster.at(s.activeRosterIds[0]);
        XeenCombatRandom expected(*f.w.sessionState().journeyRandom());
        const auto &actor=actors.at(actorSlot);
        XeenSpellDamageCandidate damage(caster.permanentLevel,caster.temporaryLevel,actor.statistics->magicResistance(),actor.original.resourceId,
            XeenLearnedSpell::MagicArrow,actor.statistics->physicalResistance());
        XeenConsequenceDraw draw{expected};check(damage.service(draw),"Surviving damage oracle incomplete");
        check(damage.damage>0 && damage.damage<actor.hp,"Response fixture must survive item damage");actors[actorSlot].hp-=damage.damage;
        XeenConsequenceCharacters characters;XeenConsequenceInputs inputs;
        for(unsigned n=0;n<6;++n){characters[n]=f.p.roster.at(s.activeRosterIds[n]);inputs[n]=*f.p.roster.combatInputs(s.activeRosterIds[n]);}
        const auto impactRandom=expected.continuation();
        XeenRegionalOpportunityCandidate response(f.w,actors,f.c,characters,inputs,f.p.encounterContext->year,0x3f,{});
        check(response.service(draw),"Surviving response oracle incomplete");
        check(response.actors[actorSlot].x!=actors[actorSlot].x || response.actors[actorSlot].y!=actors[actorSlot].y || expected.continuation()!=impactRandom,
            "Surviving response fixture neither moved nor fired");
        open(f);unsigned retries=0;
        if(retry)f.flow->beforeEncounterFrameCopy=[&]{if(!retries && !f.flow->encounter()->appearance().projectiles.empty()){++retries;throw std::bad_alloc();}};
        key(f,'u');quiet(f);f.flow->beforeEncounterFrameCopy={};
        check(retries==(retry?1u:0u),"Item projectile presentation fault not exercised");
        const auto &actual=f.w.sessionState().regionalActors(28).at(actorSlot);
        check(actual.hp==response.actors[actorSlot].hp && actual.x==response.actors[actorSlot].x && actual.y==response.actors[actorSlot].y &&
            f.w.sessionState().journeyRandom()==expected.continuation(),"Surviving monster response after impact differs");
        for(unsigned n=0;n<6;++n)check(f.p.roster.at(s.activeRosterIds[n]).currentHp==response.characters[n].currentHp &&
            f.p.roster.at(s.activeRosterIds[n]).conditions==response.characters[n].conditions,"Surviving response damage/condition differs");
        check(f.p.roster.at(s.activeRosterIds[0]).miscellaneous[0].state==1 && f.flow->encounter()->result().movementOpportunities==1 &&
            f.p.encounterContext->minutes==s.journey->context->minutes,"Response/retry repeated charge, response or time");
    }
}
void stagedRangedItemResponse(Inputs &in) {
    for(bool retry:{false,true}) {
        // Power 3 (Magic Arrow) was admitted in M61. The original Orc survives
        // its eight damage and must fire, rather than merely move in response.
        auto s=source(in,3,false,2);s.camera={23,9,11,XeenDirection::East};s.journey->random=XeenCombatRandom(5).continuation();
        auto &saved=s.map(23).monsters->slots.at(8);
        saved.x=12;saved.y=11;saved.lifecycle=XeenActorLifecycle::Present;saved.accounted=false;
        Fixture f(in,s);
        const auto live=f.w.sessionState().regionalActors(23);
        std::vector<XeenActor> actors(live.begin(),live.end());
        check(actors[8].original.resourceId==6 && actors[8].statistics->raw[32],"Staged fixture needs original ranged Orc");
        const auto &caster=f.p.roster.at(s.activeRosterIds[0]);
        XeenCombatRandom expected(*f.w.sessionState().journeyRandom());
        XeenSpellDamageCandidate arrow(caster.permanentLevel,caster.temporaryLevel,actors[8].statistics->magicResistance(),6,
            XeenLearnedSpell::MagicArrow,actors[8].statistics->physicalResistance());
        XeenConsequenceDraw draw{expected};check(arrow.service(draw) && arrow.damage==8 && actors[8].hp>8,"Staged Orc must survive Arrow");
        actors[8].hp-=8;
        XeenConsequenceCharacters characters;XeenConsequenceInputs inputs;
        for(unsigned i=0;i<6;++i){characters[i]=f.p.roster.at(s.activeRosterIds[i]);inputs[i]=*f.p.roster.combatInputs(s.activeRosterIds[i]);}
        XeenRegionalOpportunityCandidate response(f.w.map(23),actors,f.c,characters,inputs,f.p.encounterContext->year,63);
        check(response.shotCount==1 && response.shots[0].source==actors[8].id,"Item response must queue exactly one ranged shot");
        const auto travelRandom=expected.continuation();
        check(travelRandom==XeenJourneyRandomState{1,1351845u,1},"Staged travel RNG vector differs");
        check(response.service(draw),"Response oracle incomplete");
        check(response.shots[0].attack.injuryCount==1 && response.shots[0].attack.damage==2,
            "Staged response must inflict one positive injury");
        const auto injury=response.shots[0].attack.injury(0);
        check(injury.owner==18 && injury.beforeHp==f.p.roster.at(18).currentHp && injury.afterHp==injury.beforeHp-2,
            "Staged fixture must wound roster 18 by exactly two HP");
        const auto finalRandom=expected.continuation();
        check(finalRandom==XeenJourneyRandomState{1,1234782181u,5},"Staged fixture exact RNG vector differs");
        unsigned travel=0,portraits=0,retries=0;
        std::exception_ptr observationFailure;
        std::shared_ptr<const XeenRegionalObservation> last;
        const auto observe=[&] {
            const auto observation=XeenMonsterAbilityTestAccess::ranged(*f.flow->encounter());
            if(!observation || observation==last)return;
            last=observation;
            if(observation->stage!=XeenRegionalObservation::Stage::Published) {
                check(f.flow->encounter()->result().outcome==XeenEncounterOutcome::Pending &&
                    f.flow->encounter()->itemUseActive() && f.flow->encounter()->result().movementOpportunities==0 &&
                    caster.miscellaneous[0].state==1,"Staged item response lost its pending continuation or repeated charge/opportunity");
            }
            if(observation->stage==XeenRegionalObservation::Stage::Travel) {
                ++travel;
                check(observation->count==1 && observation->shots[0].source==actors[8].id,"Wrong staged travel source/count");
                check(f.w.sessionState().journeyRandom()==travelRandom,"Travel consumed attack RNG early");
                check(!f.flow->encounter()->appearance().projectiles.empty(),"Travel has no acquired projectile");
            } else if(observation->stage==XeenRegionalObservation::Stage::Portrait) {
                ++portraits;
                check(observation->impactSource==actors[8].id && observation->impactOwner==18,"Portrait lost ranged injury identity");
                const auto portrait=XeenMonsterAbilityTestAccess::portrait(*f.flow->encounter());
                check(portrait && portrait->first==18 && !portrait->second &&
                    f.p.roster.at(18).currentHp==injury.beforeHp && f.w.sessionState().journeyRandom()==travelRandom,
                    "Portrait did not precede HP and RNG publication");
            }
        };
        open(f);
        f.flow->beforeEncounterFrameCopy=[&] {
            // Composition retries may catch observer exceptions. Retain a failed
            // assertion so a successful redraw cannot hide a regression.
            try {observe();} catch(...) {observationFailure=std::current_exception();throw;}
            const auto observation=XeenMonsterAbilityTestAccess::ranged(*f.flow->encounter());
            if(retry && !retries && observation && observation->stage==XeenRegionalObservation::Stage::Portrait) {
                ++retries;throw std::bad_alloc();
            }
        };
        key(f,'u');quiet(f);f.flow->beforeEncounterFrameCopy={};
        if(observationFailure)std::rethrow_exception(observationFailure);
        check(travel==1 && portraits==1 && retries==unsigned(retry),("Travel/portrait/retry counts="+std::to_string(travel)+","+std::to_string(portraits)+","+std::to_string(retries)).c_str());
        check(f.w.sessionState().journeyRandom()==finalRandom,"Staged response/retry changed exact RNG continuation");
        for(unsigned i=0;i<6;++i)check(f.p.roster.at(s.activeRosterIds[i]).currentHp==response.characters[i].currentHp &&
            f.p.roster.at(s.activeRosterIds[i]).conditions==response.characters[i].conditions,"Staged response/retry lost or repeated injury");
        const auto &actual=f.w.sessionState().regionalActors(23).at(8);
        check(actual.hp==response.actors[8].hp && actual.x==response.actors[8].x && actual.y==response.actors[8].y,"Staged response repeated movement or item damage");
        check(caster.miscellaneous[0].state==1 && f.flow->encounter()->result().movementOpportunities==1 &&
            !f.flow->encounter()->state().pending() && *f.p.encounterContext==*s.journey->context,
            "Staged response/retry repeated charge, opportunity or time");
    }
}
void jumpArrival(Inputs &in) {
    auto s=source(in,16,false);s.camera={23,5,7,XeenDirection::North};s.characters[s.activeRosterIds[0]].skills[4]=1;
    Fixture f(in,s);unsigned signs=0;
    f.flow->reportManual=[&](const XeenManualEventResult &result) {
        if(const auto *v=std::get_if<XeenEventExecutionSuspended>(&result))if(v->request.kind==XeenPresentationKind::SceneLabelSign)++signs;
    };
    open(f);key(f,'u');quiet(f);
    check(f.c.x==5 && f.c.y==9 && signs==1 && f.p.encounterContext->minutes==s.journey->context->minutes,"Item Jump arrival Event/destination/time differs");
    const auto saved=XeenSaveFormat::encode(f.snapshot());Fixture restored(in,XeenSaveFormat::decode(saved));
    check(saved==XeenSaveFormat::encode(restored.snapshot()),"Item Jump arrival replayed on restore");
}
void walk(Fixture &f,int x,int y) {
    std::array<int,1024> parent;parent.fill(-1);std::array<XeenDirection,1024> directions{};
    const int start=f.c.y*32+f.c.x,end=y*32+x;parent[start]=start;std::queue<int> queue;queue.push(start);
    while(!queue.empty() && parent[end]<0) {
        const int cell=queue.front();queue.pop();
        for(unsigned d=0;d<4;++d) {
            XeenCamera next{28,cell%32,cell/32,XeenDirection(d)};
            if(XeenMovement().apply(f.w,next,NavigationAction::MoveForward)!=XeenMovementResult::Moved) {
                const auto source=f.w.sampleCell(28,cell%32,cell/32);const auto wall=wallAt(*source->cell,XeenDirection(d));
                if(wall!=9 && wall!=13)continue;
                constexpr int dx[]{0,1,0,-1},dy[]{1,0,-1,0};next.x+=dx[d];next.y+=dy[d];
                if(!xeenIndoorCoordinate(next.x,next.y))continue;
                const auto target=f.w.sampleCell(28,next.x,next.y);if(!target || target->cell->surfaceIndex==4)continue;
            }
            const int index=next.y*32+next.x;if(parent[index]>=0)continue;
            parent[index]=cell;directions[index]=XeenDirection(d);queue.push(index);
        }
    }
    check(parent[end]>=0,"Found-item route unavailable");
    std::vector<XeenDirection> route;for(int cell=end;cell!=start;cell=parent[cell])route.push_back(directions[cell]);
    for(auto d=route.rbegin();d!=route.rend();++d) {
        while(f.c.direction!=*d){f.act(NavigationAction::TurnRight);quiet(f);}
        for(unsigned attempt=0;attempt<10;++attempt) {
            auto query=f.c;if(XeenMovement().apply(f.w,query,NavigationAction::MoveForward)==XeenMovementResult::Moved)break;
            f.act(InteractionAction{});if(f.flow->canCancelInteraction())f.act(SelectMemberAction{5});quiet(f);
        }
        f.act(NavigationAction::MoveForward);quiet(f);
    }
    check(f.c.x==x && f.c.y==y,"Found-item route did not arrive");
}
void found(Inputs &in) {
    auto s=source(in,1,true);s.camera={28,9,28,XeenDirection::North};
    for(auto owner:s.activeRosterIds)s.characters[owner].miscellaneous={};
    // Fixed original-crate vector; no production generator supplies expected
    // bytes. Mutations: wrong charge offset/power/container, extra draw, or
    // missing/duplicate Event RNG publication before normal delivery.
    const XeenItem expected{8,4,10,0};
    s.journey->random=XeenJourneyRandomState{1,10,0};
    m42_test::StockOracle oracle{10,0};const auto item=oracle.item(1);
    check(item.first==3 && xeenSameItem(item.second,expected) && xeenItemUseSupported(expected) &&
        oracle.state==1794213167u && oracle.count==6,"Independent original-crate item vector differs");
    XeenWorld query(in.mapLoader());const auto cell=query.sampleCell(28,9,28);
    for(unsigned d=0;d<4;++d) {const auto wall=wallAt(*cell->cell,XeenDirection(d));if(wall!=1 && wall!=6 && wall!=9 && wall!=13){s.camera.direction=XeenDirection(d);break;}}
    Fixture f(in,s);f.flow->drawSmithArt=[&in](auto &image){in.assets.drawSmith(image);};
    f.act(InteractionAction{});check(f.flow->canCancelInteraction(),"Original crate selector absent");f.act(SelectMemberAction{0});quiet(f);
    check(f.w.sessionState().journeyRandom()==XeenJourneyRandomState{1,oracle.state,oracle.count},"Original crate RNG publication differs");
    unsigned member=6,slot=0;
    for(unsigned m=0;m<6;++m)for(unsigned n=0;n<9;++n)if(xeenSameItem(f.p.party.member(f.p.roster,m).miscellaneous[n],expected)){member=m;slot=n;}
    check(member<6,"Original crate did not deliver the generated supported item");
    const auto owner=f.p.party.activeRosterIds()[member];
    walk(f,8,4);while(f.c.direction!=XeenDirection::West){f.act(NavigationAction::TurnRight);quiet(f);}
    f.act(InteractionAction{});f.prepare();f.act(DialogKeyAction{'b'});f.act(SelectMemberAction{member});f.act(DialogKeyAction{'m'});f.act(DialogKeyAction{'i'});
    f.act(SelectInventorySlotAction{slot});f.act(YesAction{});
    check(f.flow->inputContext(f.flow->frame().presentation()).dialog->anyKey && xeenSameItem(f.p.roster.at(owner).miscellaneous[slot],expected),"Found item Identify lost details/bytes");
    f.act(AcknowledgeAction{});for(unsigned n=0;n<100 && !f.flow->canSave();++n)f.act(CancelInteractionAction{});quiet(f);
    const auto minute=f.p.encounterContext->minutes;
    f.act(SelectMemberAction{member});key(f,'i');key(f,'m');key(f,'1'+slot);key(f,'u');
    if(f.flow->encounter()->itemUseNeedsTarget())f.act(SelectMemberAction{member});quiet(f);
    auto used=expected;--used.state;
    check(xeenSameItem(f.p.roster.at(owner).miscellaneous[slot],used) && f.p.encounterContext->minutes==minute,"Found item Use charge/time differs");
    f.act(InteractionAction{});f.prepare();f.act(DialogKeyAction{'b'});f.act(SelectMemberAction{member});f.act(DialogKeyAction{'m'});f.act(DialogKeyAction{'s'});
    const auto gold=f.p.purse->gold;
    const auto price=*xeenItemPrice(XeenInventoryCategory::Miscellaneous,used,XeenItemService::Sell,f.p.roster.at(owner).skills[8]);
    f.act(SelectInventorySlotAction{slot});f.act(YesAction{});
    check(!f.p.roster.at(owner).miscellaneous[slot].id && f.p.purse->gold==gold+price,"Found used item Sell differs");
    for(unsigned n=0;n<100 && !f.flow->canSave();++n)f.act(CancelInteractionAction{});quiet(f);
    const auto saved=XeenSaveFormat::encode(f.snapshot());Fixture restored(in,XeenSaveFormat::decode(saved));
    check(saved==XeenSaveFormat::encode(restored.snapshot()),"Found-item run did not restore exactly");
}
}
int main(int argc,char **argv) {try {
    check(argc==2,"usage: m61-part-b <installation>");auto installation=xeenTestInstallationDetector().detect(argv[1]);check(bool(installation),"Installation unavailable");Inputs in(*installation);
    std::ostringstream captured;auto *old=std::cout.rdbuf(captured.rdbuf());
    try {effects(in);refusal(in);itemCancellation(in);miscTreasure(in);impacts(in);targetRetry(in);castingParity(in);survivingResponse(in);stagedRangedItemResponse(in);jumpArrival(in);found(in);}catch(...){std::cout.rdbuf(old);throw;}
    std::cout.rdbuf(old);std::cout<<"M61 Part B supported powers, item settlement, parity, refusals, live projectile RNG and restore passed\n";return 0;
}catch(const std::exception &e){std::cerr<<e.what()<<'\n';return 1;}}
