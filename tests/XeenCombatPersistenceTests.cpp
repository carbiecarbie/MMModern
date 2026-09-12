#include "XeenCombatTestSupport.h"
#include <iostream>
#include <optional>
using namespace combat_test;
struct Ordinary {
	XeenPartyState p=XeenPartyLoader().loadFromResources(chr(),pty());XeenCamera camera=XeenActorApproach::kEntry;XeenGameFlags flags;
	XeenWorld world{[](XeenMapIdentity){return map();},[](XeenMapIdentity){return objects();}};
	XeenSaveResourceSignature signature=save_test::sample().resources;
	XeenSaveSnapshot capture(){return XeenSaveState::capture(signature,p,camera,flags,world);}
	XeenSaveState::Resources resources(){return {signature,[]{return XeenPartyLoader().loadFromResources(chr(),pty());},[](XeenMapIdentity){return events();}};}
};

std::vector<Draw> victoryTape() {
	return {{1,2,2},{1,2,2},{1,2,2},{1,2,2},{1,20,10},
		{1,3,3},{1,3,3},{1,20,10}};
}

void driveSuccessfulEnd(CombatFixture &f) {
	f.enter();
	check(f.action(Command::Attack).status==Status::Advanced,"first genuine attack");
	check(f.action(Command::Attack).status==Status::Advanced,"lethal genuine attack");
	check(f.combat->phase()==Phase::VictoryAwaitingEnd&&f.combat->pending()==Work::End&&
		f.w.sessionState().combatAccounted()&&f.w.sessionState().completion()==XeenEncounterCompletion::None,
		"lethal publication incorrectly completed End");
	const auto end=f.combat->service(f.combat->ticket());
	check(end.status==Status::Victory,"successful End status");
	check(f.combat->phase()==Phase::Victory&&f.combat->pending()==Work::None,"successful End phase/work");
	check(f.w.sessionState().completion()==XeenEncounterCompletion::VictoryEnded,"successful End authority");
	check(f.p.encounterContext->minutes==491,"successful End minute");
}

void completedAuthorityAndCapture() {
	const auto signature=save_test::sample().resources;XeenGameFlags flags;
	CombatFixture f{XeenCombatRandom(victoryTape())};
	rejects([&]{XeenSaveState::capture(signature,f.p,f.camera,flags,f.w);},"encounter");
	f.combat->beginApproach(f.combat->ticket());
	rejects([&]{XeenSaveState::capture(signature,f.p,f.camera,flags,f.w);},"encounter");
	f.combat->approachAction(f.combat->ticket(),XeenEncounterAction::Wait);
	rejects([&]{XeenSaveState::capture(signature,f.p,f.camera,flags,f.w);},"encounter");
	f.combat->beginCombat(f.combat->ticket());
	rejects([&]{XeenSaveState::capture(signature,f.p,f.camera,flags,f.w);},"encounter");
	f.combat->command(f.combat->ticket(),Command::Attack);
	rejects([&]{XeenSaveState::capture(signature,f.p,f.camera,flags,f.w);},"encounter");
	while(f.combat->pending()==Work::Action)f.combat->service(f.combat->ticket());
	f.action(Command::Attack);
	rejects([&]{XeenSaveState::capture(signature,f.p,f.camera,flags,f.w);},"encounter");
	f.combat->service(f.combat->ticket());
	rejects([&]{XeenSaveState::capture(signature,f.p,f.camera,flags,f.w);},"encounter");
	const auto old=f.combat->ticket();const auto completed=f.combat->retireCompletedVictory(old);
	rejects([&]{f.combat->retireCompletedVictory(old);},"stale");
	auto snapshot=XeenSaveState::capture(signature,f.p,f.camera,flags,f.w);
	check(snapshot.completedEncounter&&snapshot.completedEncounter->context==*f.p.encounterContext&&
		snapshot.completedEncounter->monster==XeenMonsterIdentity{{XeenSide::Clouds,20},5},"completed capture fields");
	constexpr std::array<std::uint8_t,6> owners{0,1,6,11,14,18};
	for(unsigned i=0;i<6;++i)check(snapshot.completedEncounter->supplements[i].owner==owners[i]&&
		save_test::sameInputs(snapshot.completedEncounter->supplements[i].inputs,*f.p.roster.combatInputs(owners[i])),
		"completed capture supplement");
	const auto bytes=XeenSaveFormat::encode(snapshot);check(bytes[8]==3,"completed writer did not select v3");
	save_test::sameSnapshot(snapshot,XeenSaveFormat::decode(bytes));
	auto detached=XeenPartyLoader().loadFromResources(chr(),pty());auto copiedCamera=f.camera;
	check(!XeenSaveState::canCapture(detached,f.camera,f.w)&&!XeenSaveState::canCapture(f.p,copiedCamera,f.w)&&
		XeenSaveState::canCapture(f.p,f.camera,f.w),"detached owners affected bound completed authority");

	const auto operationLease=f.w.holdCompletedGuard(completed,XeenCompletedGuard::Operation,f.p,f.camera);
	rejects([&]{XeenSaveState::capture(signature,f.p,f.camera,flags,f.w);},"encounter");
	check(!f.w.releaseCompletedGuard(completed,XeenCompletedGuard::Presentation,operationLease),
		"presentation release cleared operation lease");
	rejects([&]{XeenSaveState::capture(signature,f.p,f.camera,flags,f.w);},"encounter");
	check(f.w.releaseCompletedGuard(completed,XeenCompletedGuard::Operation,operationLease),"operation lease release");
	check(!f.w.releaseCompletedGuard(completed,XeenCompletedGuard::Operation,operationLease),"duplicate operation release");
	save_test::sameSnapshot(snapshot,XeenSaveState::capture(signature,f.p,f.camera,flags,f.w));
	const auto presentationTicket=f.w.completedTicket(f.p,f.camera);
	const auto lease=f.w.holdCompletedGuard(presentationTicket,XeenCompletedGuard::Presentation,f.p,f.camera);
	rejects([&]{XeenSaveState::capture(signature,f.p,f.camera,flags,f.w);},"encounter");
	check(!f.w.releaseCompletedGuard(presentationTicket,XeenCompletedGuard::Operation,lease),
		"operation release cleared presentation lease");
	check(!f.w.releaseCompletedGuard(presentationTicket,XeenCompletedGuard::Presentation,lease+1),
		"wrong presentation token released lease");
	rejects([&]{XeenSaveState::capture(signature,f.p,f.camera,flags,f.w);},"encounter");
	check(f.w.releaseCompletedGuard(presentationTicket,XeenCompletedGuard::Presentation,lease),"presentation lease release");
	save_test::sameSnapshot(snapshot,XeenSaveState::capture(signature,f.p,f.camera,flags,f.w));
	f.combat.reset();
	save_test::sameSnapshot(snapshot,XeenSaveState::capture(signature,f.p,f.camera,flags,f.w));
	f.w.discardMapCache();static_cast<void>(f.w.map(20));
	save_test::sameSnapshot(snapshot,XeenSaveState::capture(signature,f.p,f.camera,flags,f.w));

	Ordinary destination;const auto before=destination.capture();unsigned calls=0;
	auto resources=destination.resources();resources.signature=signature;
	resources.loadInitialParty=[&]{++calls;return XeenPartyLoader().loadFromResources(chr(),pty());};
	resources.loadEvents=[&](XeenMapIdentity){++calls;return events();};
	rejects([&]{XeenSaveState::restoreBeforeGameplay(snapshot,resources,destination.p,destination.camera,
		destination.flags,destination.world,[&](XeenWorld &,const XeenPartyState &,const XeenCamera &,const XeenGameFlags &){++calls;});},"28B");
	check(calls==0,"v3 restoration refusal invoked a provider");save_test::sameSnapshot(before,destination.capture());
}

void retirementAndIntegrityRefusals() {
	const auto signature=save_test::sample().resources;XeenGameFlags flags;
	{
		auto bytes=chr();auto party=XeenPartyLoader().loadFromResources(bytes,pty());auto camera=XeenActorApproach::kEntry;
		XeenWorld world([](XeenMapIdentity){return map();},[](XeenMapIdentity){return objects();});
		auto boundary=std::make_unique<XeenCombatBoundary>(world,party,camera);
		auto combat=std::make_unique<XeenCombat>(world,party,camera,*boundary,bytes,XeenGameplayContextFormat::parse(pty()),
			statistics(),events(),XeenCombatRandom(victoryTape()));
		combat->beginApproach(combat->ticket());combat->approachAction(combat->ticket(),XeenEncounterAction::Wait);
		combat->beginCombat(combat->ticket());
		for(unsigned i=0;i<2;++i){combat->command(combat->ticket(),Command::Attack);while(combat->pending()==Work::Action)combat->service(combat->ticket());}
		combat->service(combat->ticket());combat->retireCompletedVictory(combat->ticket());
		const auto snapshot=XeenSaveState::capture(signature,party,camera,flags,world);
		combat.reset();boundary.reset();
		save_test::sameSnapshot(snapshot,XeenSaveState::capture(signature,party,camera,flags,world));
	}
	{
		CombatFixture f{XeenCombatRandom(victoryTape())};driveSuccessfulEnd(f);f.combat.reset();
		rejects([&]{XeenSaveState::capture(signature,f.p,f.camera,flags,f.w);},"encounter");
		f.w.discardMapCache();static_cast<void>(f.w.map(20));
	}
	{
		CombatFixture f{XeenCombatRandom(victoryTape())};driveSuccessfulEnd(f);
		auto lease=f.boundary.hold(XeenCombatBoundary::Work::Event);
		rejects([&]{f.combat->retireCompletedVictory(f.combat->ticket());});
		f.boundary.release(XeenCombatBoundary::Work::Event,lease);
		f.combat->retireCompletedVictory(f.combat->ticket());
		auto &actors=const_cast<std::vector<XeenActor>&>(f.w.sessionState().actors());actors[0].hp--;
		rejects([&]{XeenSaveState::capture(signature,f.p,f.camera,flags,f.w);},"encounter");
		actors[0].hp++;
		rejects([&]{XeenSaveState::capture(signature,f.p,f.camera,flags,f.w);},"encounter");
	}
	{
		CombatFixture f{XeenCombatRandom(victoryTape())};driveSuccessfulEnd(f);f.combat->retireCompletedVictory(f.combat->ticket());
		f.p.roster.at(29).currentSp++;
		rejects([&]{XeenSaveState::capture(signature,f.p,f.camera,flags,f.w);},"encounter");
	}
	{
		CombatFixture f{XeenCombatRandom(victoryTape())};driveSuccessfulEnd(f);f.combat->retireCompletedVictory(f.combat->ticket());
		f.camera.x++;
		rejects([&]{XeenSaveState::capture(signature,f.p,f.camera,flags,f.w);},"encounter");
	}
	{
		CombatFixture f{XeenCombatRandom(victoryTape())};driveSuccessfulEnd(f);
		const auto old=f.combat->ticket();const auto xp=f.p.roster.combatInputs(0)->experience;
		f.combat->invalidate();
		check(f.combat->phase()==Phase::Victory&&f.w.sessionState().completion()==XeenEncounterCompletion::VictoryEnded&&
			f.w.sessionState().combatAccounted()&&f.p.roster.combatInputs(0)->experience==xp&&!f.combat->current(old),
			"post-End integrity invalidation changed victory accounting or retained its ticket");
		rejects([&]{f.combat->retireCompletedVictory(old);},"stale");
		rejects([&]{f.combat->retireCompletedVictory(f.combat->ticket());},"integrity");
		rejects([&]{XeenSaveState::capture(signature,f.p,f.camera,flags,f.w);},"encounter");
		f.combat.reset();
		rejects([&]{XeenSaveState::capture(signature,f.p,f.camera,flags,f.w);},"encounter");
	}
	{
		CombatFixture f{XeenCombatRandom(victoryTape())};driveSuccessfulEnd(f);
		const auto old=f.combat->ticket();const auto sp=f.p.roster.at(29).currentSp;
		++f.p.roster.at(29).currentSp;
		rejects([&]{f.combat->retireCompletedVictory(old);},"preimage");
		check(!f.combat->current(old),"retirement preimage mismatch retained its ticket");
		f.p.roster.at(29).currentSp=sp;
		rejects([&]{f.combat->retireCompletedVictory(f.combat->ticket());},"integrity");
		rejects([&]{XeenSaveState::capture(signature,f.p,f.camera,flags,f.w);},"encounter");
	}
	{
		CombatFixture f{XeenCombatRandom(victoryTape())};driveSuccessfulEnd(f);
		const auto stale=f.combat->ticket();const auto boundaryLease=f.boundary.hold(XeenCombatBoundary::Work::Event);
		f.boundary.release(XeenCombatBoundary::Work::Event,boundaryLease);
		check(f.combat->fail(stale,XeenCombatFailure::Integrity).status==Status::Stale,
			"stale post-End integrity request was not stale");
		f.combat->retireCompletedVictory(f.combat->ticket());
		static_cast<void>(XeenSaveState::capture(signature,f.p,f.camera,flags,f.w));
	}
	for(auto guard:{XeenCompletedGuard::Integrity,XeenCompletedGuard::Fatal}) {
		CombatFixture f{XeenCombatRandom(victoryTape())};driveSuccessfulEnd(f);
		const auto ticket=f.combat->retireCompletedVictory(f.combat->ticket());
		check(f.w.latchCompletedGuard(ticket,guard,f.p,f.camera),"irreversible completed guard latch");
		rejects([&]{XeenSaveState::capture(signature,f.p,f.camera,flags,f.w);},"encounter");
		check(!f.w.latchCompletedGuard(ticket,guard,f.p,f.camera),"stale irreversible guard relatched");
		rejects([&]{XeenSaveState::capture(signature,f.p,f.camera,flags,f.w);},"encounter");
	}
	for(auto held:{XeenCompletedGuard::Operation,XeenCompletedGuard::Presentation}) {
		for(auto escalation:{XeenCompletedGuard::Integrity,XeenCompletedGuard::Fatal}) {
			CombatFixture f{XeenCombatRandom(victoryTape())};driveSuccessfulEnd(f);
			const auto ticket=f.combat->retireCompletedVictory(f.combat->ticket());
			const auto lease=f.w.holdCompletedGuard(ticket,held,f.p,f.camera);
			check(f.w.escalateCompletedGuard(ticket,held,lease,escalation),"active lease escalation");
			check(!f.w.releaseCompletedGuard(ticket,held,lease),"release cleared an escalated lease");
			rejects([&]{XeenSaveState::capture(signature,f.p,f.camera,flags,f.w);},"encounter");
		}
	}
	{
		CombatFixture f{XeenCombatRandom(victoryTape())};driveSuccessfulEnd(f);
		const auto ticket=f.combat->retireCompletedVictory(f.combat->ticket());
		const auto lease=f.w.holdCompletedGuard(ticket,XeenCompletedGuard::Presentation,f.p,f.camera);
		check(!f.w.escalateCompletedGuard(ticket,XeenCompletedGuard::Operation,lease,XeenCompletedGuard::Fatal),
			"wrong held kind escalated presentation lease");
		check(!f.w.escalateCompletedGuard(ticket,XeenCompletedGuard::Presentation,lease+1,XeenCompletedGuard::Integrity),
			"wrong token escalated presentation lease");
		CombatFixture foreign{XeenCombatRandom(victoryTape())};driveSuccessfulEnd(foreign);
		const auto foreignTicket=foreign.combat->retireCompletedVictory(foreign.combat->ticket());
		check(!f.w.escalateCompletedGuard(foreignTicket,XeenCompletedGuard::Presentation,lease,XeenCompletedGuard::Fatal),
			"foreign capability escalated presentation lease");
		check(f.w.releaseCompletedGuard(ticket,XeenCompletedGuard::Presentation,lease),"valid release after rejected escalations");
		check(!f.w.escalateCompletedGuard(ticket,XeenCompletedGuard::Presentation,lease,XeenCompletedGuard::Fatal),
			"consumed capability escalated completed authority");
		static_cast<void>(XeenSaveState::capture(signature,f.p,f.camera,flags,f.w));
	}
}

void crossLifetimeCapabilityRefusals() {
	const auto signature=save_test::sample().resources;XeenGameFlags flags;
	{
		std::optional<CombatFixture> storage;
		storage.emplace(XeenCombatRandom(victoryTape()));driveSuccessfulEnd(*storage);
		const auto oldTicket=storage->combat->retireCompletedVictory(storage->combat->ticket());
		const auto oldLease=storage->w.holdCompletedGuard(oldTicket,XeenCompletedGuard::Presentation,
			storage->p,storage->camera);
		const auto address=reinterpret_cast<std::uintptr_t>(&storage->w);
		storage.reset();
		storage.emplace(XeenCombatRandom(victoryTape()));
		check(reinterpret_cast<std::uintptr_t>(&storage->w)==address,"fixture storage did not reuse the world address");
		driveSuccessfulEnd(*storage);
		const auto liveTicket=storage->combat->retireCompletedVictory(storage->combat->ticket());
		const auto liveLease=storage->w.holdCompletedGuard(liveTicket,XeenCompletedGuard::Presentation,
			storage->p,storage->camera);
		check(oldLease==liveLease,"same-address release regression did not reproduce the token collision");
		check(!storage->w.releaseCompletedGuard(oldTicket,XeenCompletedGuard::Presentation,oldLease),
			"destroyed world capability released a replacement world's lease");
		rejects([&]{XeenSaveState::capture(signature,storage->p,storage->camera,flags,storage->w);},"encounter");
		check(storage->w.releaseCompletedGuard(liveTicket,XeenCompletedGuard::Presentation,liveLease),
			"replacement world's valid release failed after cross-lifetime refusal");
		static_cast<void>(XeenSaveState::capture(signature,storage->p,storage->camera,flags,storage->w));
	}
	for(auto escalation:{XeenCompletedGuard::Fatal,XeenCompletedGuard::Integrity}) {
		std::optional<CombatFixture> storage;
		storage.emplace(XeenCombatRandom(victoryTape()));driveSuccessfulEnd(*storage);
		const auto oldTicket=storage->combat->retireCompletedVictory(storage->combat->ticket());
		const auto oldLease=storage->w.holdCompletedGuard(oldTicket,XeenCompletedGuard::Presentation,
			storage->p,storage->camera);
		const auto address=reinterpret_cast<std::uintptr_t>(&storage->w);
		storage.reset();
		storage.emplace(XeenCombatRandom(victoryTape()));
		check(reinterpret_cast<std::uintptr_t>(&storage->w)==address,"fixture storage did not reuse the world address");
		driveSuccessfulEnd(*storage);
		const auto liveTicket=storage->combat->retireCompletedVictory(storage->combat->ticket());
		const auto liveLease=storage->w.holdCompletedGuard(liveTicket,XeenCompletedGuard::Presentation,
			storage->p,storage->camera);
		check(oldLease==liveLease,"same-address escalation regression did not reproduce the token collision");
		check(!storage->w.escalateCompletedGuard(oldTicket,XeenCompletedGuard::Presentation,oldLease,escalation),
			"destroyed world capability poisoned a replacement world");
		rejects([&]{XeenSaveState::capture(signature,storage->p,storage->camera,flags,storage->w);},"encounter");
		check(storage->w.releaseCompletedGuard(liveTicket,XeenCompletedGuard::Presentation,liveLease),
			"replacement world's valid release failed after cross-lifetime escalation refusal");
		static_cast<void>(XeenSaveState::capture(signature,storage->p,storage->camera,flags,storage->w));
	}
}

void persistence() {
	Ordinary source;const auto snapshot=source.capture();
	for(unsigned mode=0;mode<3;++mode) {
		Ordinary destination;const auto before=destination.capture();auto resources=destination.resources();
		XeenSaveState::Preflight preflight=[](XeenWorld &,const XeenPartyState &,const XeenCamera &,const XeenGameFlags &){};
		if(mode==0)resources.loadInitialParty=[] {
			auto p=XeenPartyLoader().loadFromResources(chr(),pty());auto c=XeenActorApproach::kEntry;
			XeenWorld w([](XeenMapIdentity){return map();},[](XeenMapIdentity){return objects();});XeenCombatBoundary b(w,p,c);
			XeenCombat combat(w,p,c,b,chr(),XeenGameplayContextFormat::parse(pty()),statistics(),events());return p;
		};
		if(mode==1)preflight=[](XeenWorld &,const XeenPartyState &party,const XeenCamera &,const XeenGameFlags &) {
			// Attack the candidate graph through a detached marked roster, leaving
			// the actual candidate world unmarked and context absent.
			auto &p=const_cast<XeenPartyState &>(party);auto c=XeenActorApproach::kEntry;
			XeenWorld w([](XeenMapIdentity){return map();},[](XeenMapIdentity){return objects();});XeenCombatBoundary b(w,p,c);
			XeenCombat combat(w,p,c,b,chr(),XeenGameplayContextFormat::parse(pty()),statistics(),events());
		};
		if(mode==2)preflight=[&](XeenWorld &,const XeenPartyState &,const XeenCamera &,const XeenGameFlags &){destination.world.markEncounterSession();};
		rejects([&]{XeenSaveState::restoreBeforeGameplay(snapshot,resources,destination.p,destination.camera,destination.flags,destination.world,preflight);});
		for(unsigned i=0;i<30;++i)remove_test::checkSameCharacter(before.characters[i],destination.p.roster.at(i));
		check(destination.camera.x==before.camera.x&&!destination.p.roster.combatMarked(),"failed restore preserves destination graph");
	}
	// Direct world restoration rechecks each map/object/event callback and never
	// publishes overlays after a callback irreversibly marks that same world.
	for(unsigned mode=0;mode<3;++mode) {
		XeenWorld *ptr=nullptr;XeenWorld w([&](XeenMapIdentity){if(mode==0)ptr->markEncounterSession();return map();},
			[&](XeenMapIdentity){if(mode==1)ptr->markEncounterSession();return objects();});ptr=&w;
		const auto load=[&](XeenMapIdentity){if(mode==2)w.markEncounterSession();auto e=events();e.records.emplace_back();return e;};
		rejects([&]{w.restoreSessionState({{20,0}},{{20,0}},load);},"encounter");
		check(w.hasEncounterState()&&w.sessionState().disabledObjectCount()==0&&w.sessionState().disabledEventCount()==0,"callback cannot overwrite encounter overlays");
	}
	// Ordinary v2 still round-trips here; existing save tests retain v1 coverage.
	const auto bytes=XeenSaveFormat::encode(snapshot);const auto decoded=XeenSaveFormat::decode(bytes);
	Ordinary destination;auto resources=destination.resources();
	XeenSaveState::restoreBeforeGameplay(decoded,resources,destination.p,destination.camera,destination.flags,destination.world,
		[](XeenWorld &,const XeenPartyState &,const XeenCamera &,const XeenGameFlags &){});
	save_test::sameSnapshot(snapshot,destination.capture());
	CombatFixture marked;marked.combat.reset();marked.p.encounterContext.reset();unsigned calls=0;
	resources.loadInitialParty=[&]{++calls;return XeenPartyLoader().loadFromResources(chr(),pty());};
	rejects([&]{XeenSaveState::restoreBeforeGameplay(snapshot,resources,marked.p,destination.camera,destination.flags,destination.world,
		[&](XeenWorld &,const XeenPartyState &,const XeenCamera &,const XeenGameFlags &){++calls;});},"encounter");
	check(calls==0,"detached destination marker refuses before providers");
}
int main(){try{persistence();completedAuthorityAndCapture();retirementAndIntegrityRefusals();crossLifetimeCapabilityRefusals();std::cout<<"Combat completion authority, capture and restore guards passed\n";return 0;}catch(const std::exception &e){std::cerr<<e.what()<<'\n';return 1;}}
