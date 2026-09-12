#include "XeenCombatTestSupport.h"
#include "games/xeen/XeenCombatRules.h"
#include "formats/xeen/XeenCharacterFormat.h"
#include <iostream>
using namespace combat_test;
void inputsAndOwnership() {
	auto b=chr();const auto base=29*354;
	for(auto offset:{20,21,28,29,30,31,34})b[base+offset]=255;
	b[base+348]=0x78;b[base+349]=0x56;b[base+350]=0x34;b[base+351]=0xf2;
	const auto v=XeenCharacterFormat::parseCombatInputs(b,29);
	check(v.might.permanent==255&&v.might.temporary==255&&v.speed.permanent==255&&v.speed.temporary==255&&
		v.accuracy.permanent==255&&v.accuracy.temporary==255&&v.temporaryAc==255&&v.experience==0xf2345678u,"unsigned CHR widths and LE XP");
	rejects([&]{XeenCharacterFormat::parseCombatInputs(b,30);});b.pop_back();rejects([&]{XeenCharacterFormat::parseCombatInputs(b,29);});
	b.push_back(0);b.push_back(0);rejects([&]{XeenCharacterFormat::parseCombatInputs(b,0);});
	auto mon=statistics()[8];mon.validateCombat();
	for(unsigned offset:{20,22,23,24,25,26,28,29,30,31,32,33,40,42,44,45,47}){auto changed=mon;changed.raw[offset]^=1;rejects([&]{changed.validateCombat();});}
	mon.raw[27]=1;check(mon.strikes()==258,"MON strikes u16");rejects([&]{mon.validateCombat();});
	rejects([]{XeenMonsterFormat::parse(Bytes(59));});
	CombatFixture f;
	for(unsigned id=0;id<30;++id)check(bool(f.p.roster.combatInputs(id))==(std::find(kXeenCombatOwners.begin(),kXeenCombatOwners.end(),id)!=kXeenCombatOwners.end()),"six original owner attachment");
	auto plain=XeenPartyLoader().loadFromResources(chr(),pty());const auto copy=plain;
	rejects([&]{XeenRoster r(f.p.roster);});rejects([&]{XeenRoster r(std::move(f.p.roster));});
	rejects([&]{plain.roster=f.p.roster;});rejects([&]{f.p.roster=plain.roster;});rejects([&]{f.p.roster=std::move(plain.roster);});
	rejects([&]{std::swap(plain.roster,f.p.roster);});rejects([&]{std::swap(f.p.roster,plain.roster);});
	rejects([&]{XeenPartyState p(f.p);});rejects([&]{XeenPartyState p(std::move(f.p));});
	rejects([&]{plain=f.p;});rejects([&]{f.p=std::move(plain);});rejects([&]{std::swap(plain,f.p);});
	sameParty(plain,copy);check(f.p.roster.combatMarked(),"marked ownership survived all refused replacements");
	f.combat.reset();check(f.p.roster.combatMarked(),"destruction preserves marker");
	rejects([&]{XeenCombat another(f.w,f.p,f.camera,f.boundary,f.bytes,XeenGameplayContextFormat::parse(pty()),statistics(),events());});
	XeenWorld ordinary([](XeenMapIdentity){return map();});XeenGameFlags flags;
	f.p.encounterContext.reset();rejects([&]{XeenSaveState::capture(save_test::sample().resources,f.p,f.camera,flags,ordinary);},"encounter");
	check(xeenCombatXpEligible(XeenCondition::Unconscious)&&!xeenCombatXpEligible(XeenCondition::Dead),"separate XP predicate");
	check(xeenCombatExperience(250,6,1,0)==82&&xeenCombatExperience(250,5,1,0)==100&&xeenCombatExperience(250,6,15,0)==41,"XP arithmetic controls");
	rejects([]{xeenCombatExperience(250,0,1,0);});rejects([]{xeenCombatExperience(250,6,1,0xffffffffu);});
	const unsigned counts[]{9,7,7,6,6,7,9,11,6,7};for(unsigned i=0;i<10;++i)check(xeenCombatAttackCount(static_cast<XeenCharacterClass>(i),40)==counts[i],"attack class divisors");
}
void preparation() {
	for(unsigned destination=0;destination<6;++destination) {
		CombatFixture f;unsigned slot=1;
		if(destination!=5){const auto r=f.combat->transfer(f.combat->ticket(),5,destination,XeenInventoryCategory::Accessories,1);check(r.status==XeenTransferStatus::Success,"ring transfer");slot=r.destinationSlot;}
		const auto id=kXeenCombatOwners[destination];const auto before=f.p.roster.at(id).currentHp;
		const auto beforeAc=XeenCharacterRules::combatArmorClass(f.p.roster.at(id),*f.p.roster.combatInputs(id),{610});
		check(f.combat->equipment(f.combat->ticket(),destination,XeenInventoryCategory::Accessories,slot,XeenEquipmentOperation::Equip).status==XeenEquipmentStatus::Success,"ring equip on legal owner");
		const auto &c=f.p.roster.at(id);const auto &v=*f.p.roster.combatInputs(id);
		const int speeds[]{19,19,18,18,17,17};check(XeenCharacterRules::effectivePhysical(c,v,XeenCharacterRules::PhysicalAttribute::Speed,{610})==speeds[destination],"ring actual owner Speed");
		const int acIncrease[]{2,2,1,1,2,2};
		check(XeenCharacterRules::combatArmorClass(c,v,{610})==beforeAc+acIncrease[destination],"ring actual owner AC");
		check(c.currentHp==before,"preparation never heals");f.enter();check(f.combat->participant()==int(destination),"ring initiative and tie order");
	}
	CombatFixture f;
	const auto old=f.combat->ticket();check(f.combat->transfer(old,3,4,XeenInventoryCategory::Weapons,1).status==XeenTransferStatus::Success,"carry prohibited-to-equip dagger");
	check(f.combat->transfer(old,3,4,XeenInventoryCategory::Weapons,0).status==XeenTransferStatus::StaleSelection,"preparation replay refused");
	check(f.combat->equipment(f.combat->ticket(),4,XeenInventoryCategory::Weapons,1,XeenEquipmentOperation::Equip).status==XeenEquipmentStatus::NotProficient,"carrying is not equip legality");
	f.enter(true);check(f.p.encounterContext->minutes==500,"delayed actual handoff500");
	CombatFixture legacy;
	const auto original=legacy.p.roster.at(1).accessories[1];check(original.id==5&&original.frame==8,"original legacy frame");
	check(legacy.combat->transfer(legacy.combat->ticket(),4,0,XeenInventoryCategory::Accessories,0).status==XeenTransferStatus::Success,"legacy compaction transfer");
	check(xeenSameItem(legacy.p.roster.at(1).accessories[0],original),"legacy medal survived compaction");legacy.enter();
	CombatFixture busy;auto lease=busy.boundary.hold(XeenCombatBoundary::Work::Event);
	check(busy.combat->beginApproach(busy.combat->ticket()).status==Status::Failed,"pending event denies begin irreversibly");
	check(busy.combat->result().revision==0,"failed preparation keeps world revision zero");
	busy.boundary.release(XeenCombatBoundary::Work::Event,lease);check(busy.combat->beginApproach(busy.combat->ticket()).status==Status::Refused,"cannot reopen failed begin");
	CombatFixture changed;changed.p.roster.at(0).weapons[0].frame=0;
	check(changed.combat->beginApproach(changed.combat->ticket()).status==Status::Failed,"unaccounted item edit fails");
}
void randomAndFailures() {
	std::vector<Draw> tape(4,{1,2,1});for(unsigned i=0;i<130;++i)tape.push_back({1,20,20});tape.push_back({1,20,1});
	CombatFixture f{XeenCombatRandom(tape)};f.enter();auto action=f.combat->ticket();f.combat->command(action,Command::Attack);
	check(f.combat->command(action,Command::Attack).status==Status::Stale,"consumed player intent");
	unsigned probes=0;f.combat->setProbe([&]{++probes;});auto first=f.combat->ticket();f.service();
	check(probes==64&&f.combat->random().position()==0&&f.w.sessionState().actors()[5].hp==20,"64-draw suspension without live RNG");
	check(f.combat->service(first).status==Status::Stale,"old continuation cannot run twice");f.service();check(probes==128,"second retained chunk");
	const auto result=f.service();check(result.damage==7&&f.combat->random().position()==135,"exploding prefix resumes without reroll");
	std::vector<Draw> reject(130,{1,2,0,true}); // span2 threshold0 cannot reject: use d20 interval
	reject.assign(4,{1,2,1});for(unsigned i=0;i<130;++i)reject.push_back({1,20,0,true});reject.push_back({1,20,29,true});
	CombatFixture rejection{XeenCombatRandom(reject)};rejection.enter();rejection.combat->command(rejection.combat->ticket(),Command::Attack);
	rejection.service();rejection.service();check(rejection.combat->random().position()==0,"rejected raw prefix not live");
	check(rejection.service().damage==7&&rejection.combat->random().position()==135,"raw rejection conversion continuation");
	CombatFixture mutation{XeenCombatRandom(mixedTape())};mutation.enter();mutation.combat->command(mutation.combat->ticket(),Command::Attack);
	mutation.combat->setProbe([&]{mutation.p.roster.at(18).currentSp=123;});
	check(mutation.service().status==Status::Failed&&mutation.p.roster.at(18).currentSp==123&&mutation.w.sessionState().actors()[5].hp==20&&mutation.combat->random().position()==0,"current callback mutation fails without rollback or RNG adoption");
	CombatFixture stale{XeenCombatRandom(mixedTape())};stale.enter();stale.combat->command(stale.combat->ticket(),Command::Attack);
	stale.combat->setProbe([&]{stale.combat->invalidate();throw std::runtime_error("old callback failure");});
	check(stale.service().status==Status::Stale&&stale.combat->result().failure==XeenCombatFailure::Integrity,"old exception cannot replace newer failure");
	CombatFixture critical{XeenCombatRandom(defeatTape())};critical.enter();critical.blockRound();unsigned n=0;
	critical.combat->setProbe([&]{if(++n==4)throw std::runtime_error("after first critical candidate application");});
	check(critical.service().status==Status::Failed&&critical.p.roster.at(1).currentHp==7&&critical.combat->random().position()==0,"critical failure publishes neither half");
	CombatFixture replaced;replaced.enter();replaced.combat->invalidate();check(replaced.combat->phase()==Phase::Failed,"explicit byte-identical replacement invalidation");
}
void timeBoundary() {
	for(bool lethal:{false,true}) {
		std::vector<Draw> tape;for(unsigned r=0;r<469;++r)tape.push_back({1,20,1});
		if(lethal){tape.insert(tape.end(),{{1,2,2},{1,2,2},{1,2,2},{1,2,2},{1,20,10},{1,3,3},{1,3,3},{1,20,10}});}
		else tape.insert(tape.end(),{{1,2,1},{1,2,1},{1,2,1},{1,2,1},{1,20,10},{1,20,20},{1,6,6},{1,6,6},{1,4,4},{1,6,6},{1,6,6}});
		CombatFixture f{XeenCombatRandom(tape)};f.enter();
		for(unsigned r=0;r<469;++r){f.blockRound();f.service();f.service();}
		check(f.p.encounterContext->minutes==959&&f.p.encounterContext->ctr24==1,"959 reached through 469 real rounds");
		if(lethal){f.action(Command::Attack);f.action(Command::Attack);check(f.combat->phase()==Phase::VictoryAwaitingEnd&&f.p.roster.combatInputs(0)->experience==82,"959 lethal accounting retained");}
		else{f.action(Command::Attack);f.blockRound();f.service();check(f.combat->pending()==Work::Round&&f.w.sessionState().actors()[5].hp==13&&f.p.roster.at(1).currentHp==-17,"959 earlier player/enemy injuries precede refusal");}
		const auto pos=f.combat->random().position();check(f.service().status==Status::SupportStopped,"960 refuses round or end");
		XeenGameFlags flags;rejects([&]{XeenSaveState::capture(save_test::sample().resources,f.p,f.camera,flags,f.w);},"encounter");
		check(f.p.encounterContext->minutes==959&&f.combat->random().position()==pos,"no condition RNG or unsupported minute");
	}
}
void delegatedApproachAuthorization() {
	for(bool objectCallback:{false,true}) for(unsigned mode=0;mode<4;++mode) {
		auto bytes=chr();auto p=XeenPartyLoader().loadFromResources(bytes,pty());auto camera=XeenActorApproach::kEntry;
		std::function<void()> callback;bool badDomain=false;unsigned calls=0;
		XeenWorld w([&](XeenMapIdentity){if(!objectCallback&&callback)callback();auto value=map();
			if(badDomain&&!objectCallback)value.geometry.flags=1;return value;},
			[&](XeenMapIdentity){if(objectCallback&&callback)callback();auto value=objects();
				if(badDomain&&objectCallback)value.resourcePresent=false;return value;});
		XeenCombatBoundary boundary(w,p,camera);
		XeenCombat fight(w,p,camera,boundary,bytes,XeenGameplayContextFormat::parse(pty()),statistics(),events());
		fight.beginApproach(fight.ticket());fight.approachAction(fight.ticket(),XeenEncounterAction::Right);
		fight.approachAction(fight.ticket(),XeenEncounterAction::Forward);
		check(fight.approachState().revision()==3&&fight.approachState().pending()==3&&boundary.generation()==0,
			"review oracle: revision3 pending3 boundary0 before callback");
		const auto actors=w.sessionState().actors();const auto context=p.encounterContext;const auto beforeCamera=camera;
		const auto ticket=fight.ticket();const auto generation=fight.result().generation;
		w.discardMapCache();callback=[&]{++calls;
			if(mode==0||mode==3){const auto lease=boundary.hold(XeenCombatBoundary::Work::Inventory);boundary.release(XeenCombatBoundary::Work::Inventory,lease);}
			if(mode==3) {
				auto &state=const_cast<XeenEncounterState &>(fight.approachState());
				check(XeenActorApproach::stop(w,state,XeenEncounterStop::Reporting).outcome==XeenEncounterOutcome::Stale,
					"direct stop also refuses expired delegated authorization");
			}
			if(mode==1)throw std::runtime_error("current delegated provider failure");
			if(mode==2)badDomain=true;
		};
		const auto result=fight.approachPulse(ticket);callback={};
		check(calls==1,"selected map/object callback reached exactly once");
		sameActors(actors,w.sessionState().actors());
		check(p.encounterContext==context&&save_test::sameCamera(beforeCamera,camera),"delegated failure changed gameplay facts");
		if(mode==0||mode==3) {
			check(result.status==Status::Stale&&result.revision==3&&boundary.generation()==2&&fight.phase()==Phase::Approach&&
				XeenActorApproach::authoritative(w,p,camera,fight.approachState())&&
				fight.approachState().phase()==XeenEncounterPhase::Exploring&&fight.approachState().pending()==3&&
				fight.approachState().revision()==3&&!w.sessionState().encounterTerminal()&&
				fight.result().revision==3&&fight.result().generation==generation,
				"stale delegated pulse must preserve revision3 pending3 Exploring and false terminal latch");
			check(fight.approachPulse(ticket).status==Status::Stale,"old callback authorization remains stale");
			const auto resumed=fight.approachPulse(fight.ticket());
			check(resumed.revision==4&&fight.approachState().pending()==2&&fight.phase()==Phase::Approach,
				"fresh authorization can service the preserved pending work");
		} else {
			check(result.status==Status::SupportStopped&&fight.phase()==Phase::SupportStopped&&
				fight.approachState().revision()==4&&fight.approachState().pending()==0&&w.sessionState().encounterTerminal()&&
				fight.approachState().reason()==(mode==1?XeenEncounterStop::Preparation:XeenEncounterStop::Domain),
				"current delegated provider/domain failure must still publish its stop");
		}
	}
}
void resourceContinuation() {
	for(bool mutateOwner:{false,true}) {
		auto bytes=chr();auto p=XeenPartyLoader().loadFromResources(bytes,pty());auto camera=XeenActorApproach::kEntry;
		bool changed=false;
		XeenWorld w([&](XeenMapIdentity){auto value=map();if(changed){
			if(mutateOwner)p.roster.at(18).currentSp=123;else value.geometry.flags=1;
		}return value;},[](XeenMapIdentity){return objects();});
		XeenCombatBoundary boundary(w,p,camera);
		XeenCombat fight(w,p,camera,boundary,bytes,XeenGameplayContextFormat::parse(pty()),statistics(),events(),XeenCombatRandom(mixedTape()));
		fight.beginApproach(fight.ticket());const auto before=fight.result().revision;
		const auto engagement=fight.approachAction(fight.ticket(),XeenEncounterAction::Wait);
		check(engagement.oldRevision==before&&engagement.revision==before+1,"typed approach revision transition");
		fight.beginCombat(fight.ticket());fight.command(fight.ticket(),Command::Attack);
		changed=true;w.discardMapCache();
		check(fight.service(fight.ticket()).status==Status::Failed&&fight.random().position()==0&&w.sessionState().actors()[5].hp==20,
			"resource reconstruction refuses pending action without damage or RNG adoption");
		if(mutateOwner)check(p.roster.at(18).currentSp==123,"resource callback mutation is not concealed");
	}
}
void handoffAndObservation() {
	{
		auto bytes=chr();auto p=XeenPartyLoader().loadFromResources(bytes,pty());auto camera=XeenActorApproach::kEntry;
		unsigned maps=0;XeenWorld w([&](XeenMapIdentity){++maps;return map();},[&](XeenMapIdentity){p.roster.at(0).currentHp=11;return objects();});
		XeenCombatBoundary b(w,p,camera);
		rejects([&]{XeenCombat combat(w,p,camera,b,bytes,XeenGameplayContextFormat::parse(pty()),statistics(),events());});
		check(maps==0&&w.hasEncounterState()&&p.roster.combatMarked()&&!p.roster.combatInputs(0)&&p.roster.at(0).currentHp==11,"admission callback failure is immediate and irreversible without partial attachment");
		w.map(20); // Failed construction left no dangling resource-check callback.
	}
	CombatFixture reserved;XeenEncounterState rogue;
	rejects([&]{XeenActorApproach::initialize(reserved.w,reserved.p,reserved.camera,rogue,statistics(),XeenGameplayContextFormat::parse(pty()),events());});
	check(reserved.combat->beginCombat(reserved.combat->ticket()).status==Status::Refused,"handoff cannot manufacture engagement");
	reserved.enter();const auto revision=reserved.combat->result().revision;
	check(reserved.w.sessionState().encounterTerminal()&&revision==reserved.combat->approachState().revision()+1,"latched terminal and shared handoff revision");
	auto old=reserved.combat->approachState();XeenActorApproach::action(reserved.w,reserved.p,reserved.camera,old,XeenEncounterAction::Wait,events());
	XeenActorApproach::pulse(reserved.w,reserved.p,reserved.camera,old,events());XeenActorApproach::stop(reserved.w,old,XeenEncounterStop::Reporting);
	check(reserved.combat->result().revision==revision&&reserved.p.encounterContext->minutes==490,"old approach cannot mutate combat");
	CombatFixture other;check(other.combat->command(reserved.combat->ticket(),Command::Block).status==Status::Stale,"wrong owner ticket");
	encounter_test::Fixture diagnostic26;diagnostic26.start();XeenCombatBoundary b26(diagnostic26.world,diagnostic26.p,diagnostic26.camera);
	rejects([&]{XeenCombat c(diagnostic26.world,diagnostic26.p,diagnostic26.camera,b26,chr(),diagnostic26.context,statistics(),events());});
	CombatFixture failed;failed.combat->beginApproach(failed.combat->ticket());failed.combat->approachAction(failed.combat->ticket(),XeenEncounterAction::Wait);
	failed.boundary.hold(XeenCombatBoundary::Work::PresentationFailure);
	check(failed.combat->beginCombat(failed.combat->ticket()).status==Status::Failed,"failed presentation denies handoff");
	CombatFixture inventory;const auto lease=inventory.boundary.hold(XeenCombatBoundary::Work::Inventory);
	check(inventory.combat->equipment(inventory.combat->ticket(),0,XeenInventoryCategory::Armor,0,XeenEquipmentOperation::Remove).status==XeenEquipmentStatus::Success,"current preparation inventory mutation");
	inventory.boundary.release(XeenCombatBoundary::Work::Inventory,lease);inventory.enter();
	CombatFixture observed;
	observed.combat->setProbe([&]{check(observed.combat->preparationEquipmentResult().has_value(),"receipt adopted before observer");throw std::runtime_error("equipment observer");});
	check(observed.combat->equipment(observed.combat->ticket(),0,XeenInventoryCategory::Armor,0,XeenEquipmentOperation::Remove).status==XeenEquipmentStatus::Success&&
		observed.p.roster.at(0).armor[0].frame==0&&observed.combat->phase()==Phase::Failed,"failed observer preserves truthful preparation receipt");
	for(bool failBeforeAccounting:{false,true}) {
		CombatFixture f{XeenCombatRandom(mixedTape())};f.enter();f.action(Command::Attack);f.action(Command::Block);f.action(Command::Attack);f.action(Command::Attack);f.action(Command::Block);f.action(Command::Block);f.service();f.service();
		if(failBeforeAccounting){unsigned calls=0;f.combat->setProbe([&]{if(++calls==5)throw std::runtime_error("lethal prepublication");});
			f.combat->command(f.combat->ticket(),Command::Attack);check(f.service().status==Status::Failed,"lethal preparation failure");
			check(f.w.sessionState().actors()[5].hp==9&&f.p.roster.combatInputs(0)->experience==0&&f.combat->random().position()==13,"lethal failure preserves earlier damage and all XP preimages");
		}else{f.action(Command::Attack);f.combat->fail(f.combat->ticket());
			check(f.combat->phase()==Phase::Failed&&f.w.sessionState().actors()[5].hp==0&&f.p.roster.combatInputs(0)->experience==82,"PendingEnd failure retains accounting without false Victory");}
	}
}
int main(){try{inputsAndOwnership();preparation();randomAndFailures();timeBoundary();delegatedApproachAuthorization();resourceContinuation();handoffAndObservation();std::cout<<"Combat ownership, preparation, continuations, failures and time boundaries passed\n";return 0;}catch(const std::exception &e){std::cerr<<e.what()<<'\n';return 1;}}
