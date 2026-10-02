#include "XeenRegionalCombatTestSupport.h"
#include "games/xeen/XeenCombatRules.h"
#include "formats/xeen/XeenCharacterFormat.h"
#include <iostream>
using namespace regional_combat_test;
using regional_combat_test::RegionalCombatFixture;
void inputsAndOwnership() {
	auto b=chr();const auto base=29*354;
	for(auto offset:{20,21,28,29,30,31,34})b[base+offset]=255;
	b[base+348]=0x78;b[base+349]=0x56;b[base+350]=0x34;b[base+351]=0xf2;
	const auto v=XeenCharacterFormat::parseCombatInputs(b,29);
	check(v.might.permanent==255&&v.might.temporary==255&&v.speed.permanent==255&&v.speed.temporary==255&&
		v.accuracy.permanent==255&&v.accuracy.temporary==255&&v.temporaryAc==255&&v.experience==0xf2345678u,"unsigned CHR widths and LE XP");
	rejects([&]{XeenCharacterFormat::parseCombatInputs(b,30);});b.pop_back();rejects([&]{XeenCharacterFormat::parseCombatInputs(b,29);});
	b.push_back(0);b.push_back(0);rejects([&]{XeenCharacterFormat::parseCombatInputs(b,0);});
	auto mon=regional_test::statistics()[8];mon.raw[27]=1;
    check(mon.strikes()==258,"MON strikes u16");
    rejects([]{XeenMonsterFormat::parse(Bytes(59));});
	RegionalCombatFixture f;
	for(unsigned id=0;id<30;++id)check(bool(f.p.roster.combatInputs(id)),"all regional owner attachments");
	auto plain=XeenPartyLoader().loadFromResources(chr(),pty());const auto copy=plain;
	rejects([&]{XeenRoster r(f.p.roster);});rejects([&]{XeenRoster r(std::move(f.p.roster));});
	rejects([&]{plain.roster=f.p.roster;});rejects([&]{f.p.roster=plain.roster;});rejects([&]{f.p.roster=std::move(plain.roster);});
	rejects([&]{std::swap(plain.roster,f.p.roster);});rejects([&]{std::swap(f.p.roster,plain.roster);});
	rejects([&]{XeenPartyState p(f.p);});rejects([&]{XeenPartyState p(std::move(f.p));});
	rejects([&]{plain=f.p;});rejects([&]{f.p=std::move(plain);});rejects([&]{std::swap(plain,f.p);});
	sameParty(plain,copy);check(f.p.roster.combatMarked(),"marked ownership survived all refused replacements");
	f.flow.reset();check(f.p.roster.combatMarked(),"destruction preserves marker");

	XeenWorld ordinary([](XeenMapIdentity){return map();});XeenGameFlags flags;
	f.p.encounterContext.reset();rejects([&]{XeenSaveState::capture(save_test::sample().resources,f.p,f.camera,flags,ordinary);},"encounter");
	check(xeenCombatXpEligible(XeenCondition::Unconscious)&&!xeenCombatXpEligible(XeenCondition::Dead),"separate XP predicate");
	check(xeenCombatExperience(250,6,1,0)==82&&xeenCombatExperience(250,5,1,0)==100&&xeenCombatExperience(250,6,15,0)==41,"XP arithmetic controls");
	rejects([]{xeenCombatExperience(250,0,1,0);});rejects([]{xeenCombatExperience(250,6,1,0xffffffffu);});
	const unsigned counts[]{9,7,7,6,6,7,9,11,6,7};for(unsigned i=0;i<10;++i)check(xeenCombatAttackCount(static_cast<XeenCharacterClass>(i),40)==counts[i],"attack class divisors");
}
void preparation() {
	for(unsigned destination=0;destination<6;++destination) {
		RegionalCombatFixture f;unsigned slot=1;
		if(destination!=5){const auto r=f.transfer(5,destination,XeenInventoryCategory::Accessories,1);check(r.status==XeenTransferStatus::Success,"ring transfer");slot=r.destinationSlot;}
		const auto id=kXeenCombatOwners[destination];const auto before=f.p.roster.at(id).currentHp;
		const auto beforeAc=XeenCharacterRules::combatArmorClass(f.p.roster.at(id),*f.p.roster.combatInputs(id),{610});
		check(f.equipment(destination,XeenInventoryCategory::Accessories,slot,XeenEquipmentOperation::Equip).status==XeenEquipmentStatus::Success,"ring equip on legal owner");
		const auto &c=f.p.roster.at(id);const auto &v=*f.p.roster.combatInputs(id);
		const int speeds[]{19,19,18,18,17,17};check(XeenCharacterRules::effectivePhysical(c,v,XeenCharacterRules::PhysicalAttribute::Speed,{610})==speeds[destination],"ring actual owner Speed");
		const int acIncrease[]{2,2,1,1,2,2};
		check(XeenCharacterRules::combatArmorClass(c,v,{610})==beforeAc+acIncrease[destination],"ring actual owner AC");
		check(c.currentHp==before,"preparation never heals");f.enter();check(f.combat->participant()==int(destination),"ring initiative and tie order");
	}
	RegionalCombatFixture f;
	const auto old=f.flow->ticket();check(f.flow->journeyTransfer(old,3,4,XeenInventoryCategory::Weapons,1).status==XeenTransferStatus::Success,"carry prohibited-to-equip dagger");
	check(f.flow->journeyTransfer(old,3,4,XeenInventoryCategory::Weapons,0).status==XeenTransferStatus::StaleSelection,"preparation replay refused");
	f.present();check(f.equipment(4,XeenInventoryCategory::Weapons,1,XeenEquipmentOperation::Equip).status==XeenEquipmentStatus::NotProficient,"carrying is not equip legality");
	f.present();f.enter(true);check(f.p.encounterContext->minutes==500,"delayed actual handoff500");
	RegionalCombatFixture legacy;
	const auto original=legacy.p.roster.at(1).accessories[1];check(original.id==5&&original.frame==8,"original legacy frame");
	check(legacy.transfer(4,0,XeenInventoryCategory::Accessories,0).status==XeenTransferStatus::Success,"legacy compaction transfer");
	check(xeenSameItem(legacy.p.roster.at(1).accessories[0],original),"legacy medal survived compaction");legacy.enter();
    RegionalCombatFixture busy;const auto lease=busy.flow->boundary().hold(XeenCombatBoundary::Work::Event);
    check(busy.flow->journeyAction(busy.flow->ticket(),XeenEncounterAction::Wait).outcome==XeenEncounterOutcome::Refused,"pending event denies action");
    check(busy.p.encounterContext->minutes==480,"busy action consumes no time");
    busy.flow->boundary().release(XeenCombatBoundary::Work::Event,lease);
    RegionalCombatFixture changed;changed.p.roster.at(0).weapons[0].frame=0;
    check(!changed.flow->journeyQuiet(),"unaccounted item edit fails");

}
void randomAndFailures() {
	std::vector<Draw> tape(4,{1,2,1});for(unsigned i=0;i<130;++i)tape.push_back({1,20,20});tape.push_back({1,20,1});
	RegionalCombatFixture f{XeenCombatRandom(tape)};f.enter();auto action=f.combat->ticket();f.combat->command(action,Command::Attack);
	check(f.combat->command(action,Command::Attack).status==Status::Stale,"consumed player intent");
	unsigned probes=0;f.combat->setProbe([&]{++probes;});auto first=f.combat->ticket();f.service();
	check(probes==64&&f.combat->random().position()==0&&f.w.sessionState().actors()[5].hp==20,"64-draw suspension without live RNG");
	check(f.combat->service(first).status==Status::Stale,"old continuation cannot run twice");f.service();check(probes==128,"second retained chunk");
	const auto result=f.service();check(result.damage==7&&f.combat->random().position()==135&&f.rng.position()==135,"exploding prefix resumes without reroll");
	std::vector<Draw> reject(130,{1,2,0,true}); // span2 threshold0 cannot reject: use d20 interval
	reject.assign(4,{1,2,1});for(unsigned i=0;i<130;++i)reject.push_back({1,20,0,true});reject.push_back({1,20,29,true});
	RegionalCombatFixture rejection{XeenCombatRandom(reject)};rejection.enter();rejection.combat->command(rejection.combat->ticket(),Command::Attack);
	rejection.service();rejection.service();check(rejection.combat->random().position()==0,"rejected raw prefix not live");
	check(rejection.service().damage==7&&rejection.combat->random().position()==135&&rejection.rng.position()==135,"raw rejection conversion continuation");
	RegionalCombatFixture mutation{XeenCombatRandom(mixedTape())};mutation.enter();mutation.combat->command(mutation.combat->ticket(),Command::Attack);
	mutation.combat->setProbe([&]{mutation.p.roster.at(18).currentSp=123;});
	check(mutation.service().status==Status::Failed&&mutation.p.roster.at(18).currentSp==123&&mutation.w.sessionState().actors()[5].hp==20&&mutation.combat->random().position()==0,"current callback mutation fails without rollback or RNG adoption");
	RegionalCombatFixture stale{XeenCombatRandom(mixedTape())};stale.enter();stale.combat->command(stale.combat->ticket(),Command::Attack);
	stale.combat->setProbe([&]{stale.combat->invalidate();throw std::runtime_error("old callback failure");});
	check(stale.service().status==Status::Stale&&stale.combat->result().failure==XeenCombatFailure::Integrity,"old exception cannot replace newer failure");
	RegionalCombatFixture critical{XeenCombatRandom(defeatTape())};critical.enter();critical.blockRound();unsigned n=0;
	critical.combat->setProbe([&]{if(++n==4)throw std::runtime_error("after first critical candidate application");});
	check(critical.service().status==Status::Failed&&critical.p.roster.at(1).currentHp==7&&critical.combat->random().position()==0,"critical failure publishes neither half");
	RegionalCombatFixture replaced;replaced.enter();replaced.combat->invalidate();check(replaced.combat->phase()==Phase::Failed,"explicit byte-identical replacement invalidation");
}
void timeBoundary(){
    for(bool lethal:{false,true}){
        std::vector<Draw> draws;
        if(lethal)draws={{1,2,2},{1,2,2},{1,2,2},{1,2,2},{1,20,10},{1,3,3},{1,3,3},{1,20,10}};
        else draws={{1,2,1},{1,2,1},{1,2,1},{1,2,1},{1,20,10},{1,20,20},{1,6,6},{1,6,6},{1,4,4},{1,6,6},{1,6,6}};
        RegionalCombatFixture f{XeenCombatRandom(draws),1249};f.enter();
        check(f.p.encounterContext->minutes==1259,"current dusk boundary prestate");
        f.action(Command::Attack);
        if(lethal){f.action(Command::Attack);check(f.combat->phase()==Phase::VictoryAwaitingEnd&&f.p.roster.combatInputs(0)->experience==82,"lethal accounting precedes dusk refusal");}
        else{f.blockRound();f.service();check(f.combat->pending()==Work::Round&&f.w.sessionState().actors()[5].hp==13&&f.p.roster.at(1).currentHp==-17,"earlier injuries precede round refusal");}
        const auto actors=f.w.sessionState().actors();const auto characters=f.p.roster.characters();const auto rng=f.combat->random().position();
        const auto r=f.service();check(r.status==Status::SupportStopped&&r.failure==XeenCombatFailure::Time,"dusk refuses unsupported Round or End");
        check(f.p.encounterContext->minutes==1259&&f.combat->random().position()==rng,"time refusal consumes neither minute nor RNG");
        sameActors(actors,f.w.sessionState().actors());
        for(unsigned i=0;i<30;++i)remove_test::checkSameCharacter(characters[i],f.p.roster.at(i));
        rejects([&]{f.snapshot();},"encounter");
    }
}
void resourceContinuation() {
    for(bool mutateOwner:{false,true}){
        RegionalCombatFixture f{XeenCombatRandom(mixedTape())};f.enter();
        f.combat->command(f.combat->ticket(),Command::Attack);
        f.transformMap=[&](auto &value){if(mutateOwner)f.p.roster.at(18).currentSp=123;else value.geometry.flags=1;};
        f.w.discardMapCache();
        check(f.service().status==Status::Failed&&f.combat->random().position()==0&&f.w.sessionState().actors()[5].hp==20,
            "resource reconstruction refuses pending action without damage or RNG adoption");
        if(mutateOwner)check(f.p.roster.at(18).currentSp==123,"resource callback mutation is not concealed");
    }
}
void handoffAndObservation() {
    RegionalCombatFixture reserved;
    check(!reserved.flow->attachJourney(reserved.flow->ticket(),[]{}),"handoff cannot manufacture engagement");
    reserved.enter();const auto revision=reserved.combat->result().revision;
    auto old=reserved.flow->state();
    XeenActorApproach::action(reserved.w,reserved.p,reserved.camera,old,XeenEncounterAction::Wait,reserved.event);
    XeenActorApproach::pulse(reserved.w,reserved.p,reserved.camera,old,reserved.event);
    XeenActorApproach::stop(reserved.w,old,XeenEncounterStop::Reporting);
    check(reserved.combat->result().revision==revision&&reserved.p.encounterContext->minutes==490,"old approach cannot mutate combat");
    RegionalCombatFixture other;other.enter();
    check(other.combat->command(reserved.combat->ticket(),Command::Block).status==Status::Stale,"wrong owner ticket");
    RegionalCombatFixture failed;failed.actionBefore(XeenEncounterAction::Wait);
    failed.flow->boundary().hold(XeenCombatBoundary::Work::PresentationFailure);
    check(!failed.flow->attachJourney(failed.flow->ticket(),[]{}),"failed presentation denies handoff");
    RegionalCombatFixture observed;
    const auto receipt=observed.flow->journeyEquipment(observed.flow->ticket(),0,XeenInventoryCategory::Armor,0,XeenEquipmentOperation::Remove);
    check(receipt.status==XeenEquipmentStatus::Success,"equipment mutation publishes a truthful receipt");
    unsigned observers=0;const auto frame=observed.flow->ticket();
    const auto failObserver=[&]{++observers;check(observed.p.roster.at(0).armor[0].frame==0,"equipment fact precedes observer");throw std::runtime_error("equipment observer failure");};
    check(!observed.flow->prepareJourneyFrame(frame,failObserver)&&!observed.flow->prepareJourneyFrame(frame,failObserver),"observer and recovery failures refused");
    check(observers==2&&observed.p.roster.at(0).armor[0].frame==0&&!observed.flow->journeyQuiet(),"failed observer preserves equipment receipt and mutation");
	for(bool failBeforeAccounting:{false,true}) {
		RegionalCombatFixture f{XeenCombatRandom(mixedTape())};f.enter();f.action(Command::Attack);f.action(Command::Block);f.action(Command::Attack);f.action(Command::Attack);f.action(Command::Block);f.action(Command::Block);f.service();f.service();
		if(failBeforeAccounting){unsigned calls=0;f.combat->setProbe([&]{if(++calls==5)throw std::runtime_error("lethal prepublication");});
			f.combat->command(f.combat->ticket(),Command::Attack);check(f.service().status==Status::Failed,"lethal preparation failure");
			check(f.w.sessionState().actors()[5].hp==9&&f.p.roster.combatInputs(0)->experience==0&&f.combat->random().position()==13,"lethal failure preserves earlier damage and all XP preimages");
		}else{f.action(Command::Attack);f.combat->fail(f.combat->ticket());
			check(f.combat->phase()==Phase::Failed&&f.w.sessionState().actors()[5].hp==0&&f.p.roster.combatInputs(0)->experience==82,"PendingEnd failure retains accounting without false Victory");}
	}
}
int main(){try{inputsAndOwnership();preparation();randomAndFailures();timeBoundary();resourceContinuation();handoffAndObservation();std::cout<<"Combat ownership, preparation, continuations, failures and time boundaries passed\n";return 0;}catch(const std::exception &e){std::cerr<<e.what()<<'\n';return 1;}}
