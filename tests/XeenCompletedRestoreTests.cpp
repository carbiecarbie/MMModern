#include "XeenRegionalTestSupport.h"
#include "XeenRegionalJourneyTestSupport.h"
#include "XeenSaveGameplayTestSupport.h"
#include <iostream>
#include "XeenRestoreReplayProbe.h"
#include <new>
using namespace mmodern;
using save_test::check;using save_test::rejects;using save_test::Bytes;
namespace {
inline Bytes chr(){return regional_test::characterBytes();}
inline Bytes pty(){return regional_test::partyBytes();}
inline XeenMap map(){return regional_test::map();}
inline XeenObjectFile objects(){return regional_test::objects();}
inline XeenEventFile events(){return regional_test::events(23);}
enum class Seam { Initial, Chr, Pty, Mon, Map, Mob, Evt, Preflight };
struct Destination {
 XeenPartyState party=XeenPartyLoader().loadFromResources(chr(),pty());
 XeenCamera camera{23,9,11,XeenDirection::West};XeenGameFlags flags;
 XeenMap terrain=map();XeenObjectFile mob=objects();
 std::array<unsigned,8> calls{};std::function<void(Seam)> observer;
 void observe(Seam s){++calls[unsigned(s)];if(observer)observer(s);}
 XeenWorld world{[&](auto id){observe(Seam::Map);auto m=terrain;m.geometry.id=id.number;return m;},[&](auto id){observe(Seam::Mob);auto m=mob;m.mapId=id;return m;}};
 XeenEventPresenter::Clock clock=[]{return 0;};std::unique_ptr<XeenEncounterFlow> flow;
 XeenSaveState::Resources resources(){auto r=regional_test::resources();
  r.loadInitialParty=[&]{observe(Seam::Initial);return XeenPartyLoader().loadFromResources(chr(),pty());};
  r.loadInitialCharacters=[&]{observe(Seam::Chr);return chr();};
  r.loadInitialContext=[&]{observe(Seam::Pty);return XeenGameplayContextFormat::parse(pty());};
  r.loadMonsterStatistics=[&]{observe(Seam::Mon);return regional_test::statistics();};
  r.loadEvents=[&](auto id){observe(Seam::Evt);return regional_test::events(id);};return r;
 }
 XeenSaveSnapshot capture(){
  if(world.sessionState().journey()){
   if(!flow){flow=std::make_unique<XeenEncounterFlow>(world,party,camera,flags,clock,XeenJourneyRestoreTag{});
    check(flow->prepareJourneyFrame(flow->ticket(),[]{}),"Restored frame prepared");check(flow->presentJourney(flow->ticket()),"Restored frame presented");}
   return XeenSaveState::capture(regional_test::signature(),party,camera,flags,world);
  }
  check(!world.hasEncounterState()&&!party.encounterContext&&!party.roster.combatMarked(),"Failed restore leaked encounter publication");
  for(unsigned owner=0;owner<30;++owner)check(!party.roster.combatInputs(owner),"Failed restore leaked supplements");
  XeenSaveSnapshot s;s.camera=camera;s.characters=party.roster.characters();s.activeRosterIds=party.party.activeRosterIds();
  s.questItems=party.questItems.counts();s.questFlags=party.questFlags.values();s.gameFlags=flags.values();
  s.disabledObjects.assign(world.sessionState().disabledObjects().begin(),world.sessionState().disabledObjects().end());
  s.disabledEvents.assign(world.sessionState().disabledEvents().begin(),world.sessionState().disabledEvents().end());return s;
 }
 void restore(const XeenSaveSnapshot &s,XeenSaveState::Preflight extra={}){
  replay_test::Scope noReplay;
  XeenSaveState::restoreBeforeGameplay(s,resources(),party,camera,flags,world,[&](auto &w,const auto &p,const auto &c,const auto &f){
   observe(Seam::Preflight);check(!XeenSaveState::canCapture(p,c,w),"Unpublished candidate cannot capture");if(extra)extra(w,p,c,f);});
 }
};
void crossWorldBorrowFreshness() {
	const auto snapshot = regional_test::snapshot();
	XeenFontFormat font(Bytes(XeenFontFormat::kMinimumSize));
	for (unsigned alias = 0; alias < 4; ++alias) {
		Destination d, other;
		auto &party = alias == 0 || alias == 1 ? d.party : other.party;
		auto &camera = alias == 0 || alias == 2 ? d.camera : other.camera;
		auto &flags = alias == 0 || alias == 3 ? d.flags : other.flags;
		XeenEventSystem eventSystem([](XeenMapIdentity id) { auto e = events(); e.mapId = id; return XeenEventScript(e); },
			[](XeenMapIdentity id) { return XeenEventTextFile{id, "synthetic.txt", true, {}}; });
		{
			XeenEventFlow flow(other.world, eventSystem, party, camera, flags, font,
				[](std::uint64_t) { return XeenEventFlow::Composition{IndexedFrame{320, 200, Bytes(64000)}, false}; });
			const auto before = d.capture(); const auto calls = d.calls;
			rejects([&] { d.restore(snapshot); }, "unborrowed");
			check(d.calls == calls, "cross-world borrowed restore entered a provider/preflight");
			save_test::sameSnapshot(before, d.capture());
			const auto direction = camera.direction;
			flow.handle(NavigationAction::TurnRight);
			check(camera.direction == XeenDirection((unsigned(direction) + 1) % 4), "refusal disrupted ordinary Flow ownership");
		}
		d.restore(snapshot); save_test::sameSnapshot(snapshot, d.capture());
	}
	// Old leases outlive replaced owners, but cannot poison or release new leases.
	for (bool newerBorrow : {false, true}) {
		Destination d, other;
		XeenEventSystem eventSystem([](XeenMapIdentity id) { auto e = events(); e.mapId = id; return XeenEventScript(e); },
			[](XeenMapIdentity id) { return XeenEventTextFile{id, "synthetic.txt", true, {}}; });
		auto flow = [&] { return std::make_unique<XeenEventFlow>(other.world, eventSystem, d.party, d.camera, d.flags, font,
			[](std::uint64_t) { return XeenEventFlow::Composition{IndexedFrame{320, 200, Bytes(64000)}, false}; }); };
		auto old = flow();
		d.party.~XeenPartyState(); new (&d.party) XeenPartyState(XeenPartyLoader().loadFromResources(chr(), pty()));
		d.camera.~XeenCamera(); new (&d.camera) XeenCamera{23,9,11,XeenDirection::West};
		d.flags.~XeenGameFlags(); new (&d.flags) XeenGameFlags;
		other.world.~XeenWorld(); new (&other.world) XeenWorld([](XeenMapIdentity) { return map(); }, [](XeenMapIdentity) { return objects(); });
		if (!newerBorrow) {
			d.restore(snapshot); old.reset(); save_test::sameSnapshot(snapshot, d.capture()); continue;
		}
		auto newer = newerBorrow ? flow() : nullptr;
		old.reset();
		if (newer) {
			const auto calls = d.calls; rejects([&] { d.restore(snapshot); }, "unborrowed");
			check(d.calls == calls, "old borrower cleared newer owner borrow"); newer.reset();
		}
		d.restore(snapshot); save_test::sameSnapshot(snapshot, d.capture());
	}
}
void staleFlowOwnerLifetimes() {
	const auto snapshot = regional_test::snapshot();
	XeenFontFormat font(Bytes(XeenFontFormat::kMinimumSize));
	const auto compose = [](std::uint64_t) { return XeenEventFlow::Composition{IndexedFrame{320,200,Bytes(64000)}, false}; };
	const auto eventProvider = [](XeenMapIdentity id) { auto e = events(); e.mapId = id; return XeenEventScript(e); };
	const auto textProvider = [](XeenMapIdentity id) { return XeenEventTextFile{id,"synthetic.txt",true,{}}; };
	{
		Destination d;
		XeenEventSystem eventSystem(eventProvider, textProvider);
		auto camera = std::make_unique<XeenCamera>(d.camera);
		XeenEventFlow old(d.world, eventSystem, d.party, *camera, d.flags, font, compose);
		camera.reset(); // No replacement storage exists to inspect.
		rejects([&] { old.handle(NavigationAction::TurnRight); }, "borrowed owner lifetime");
	}
	{
		Destination d, other;
		XeenEventSystem eventSystem(eventProvider, textProvider);
		auto old = std::make_unique<XeenEventFlow>(other.world, eventSystem, other.party, d.camera, other.flags, font, compose);
		d.camera.~XeenCamera(); new (&d.camera) XeenCamera{23,9,11,XeenDirection::West};
		d.restore(snapshot);
		const auto before = d.capture(); const auto calls = d.calls;
		rejects([&] { old->handle(NavigationAction::TurnRight); }, "borrowed owner lifetime");
		rejects([&] { old->initial(); }, "borrowed owner lifetime");
		rejects([&] { old->refresh(true); }, "borrowed owner lifetime");
		rejects([&] { old->updatePresentation(); }, "borrowed owner lifetime");
		rejects([&] { old->acceptManual(XeenManualEventResult{}); }, "borrowed owner lifetime");
		rejects([&] { old->acceptAutomatic(XeenAutomaticEventResult{}); }, "borrowed owner lifetime");
		rejects([&] { old->respond(1, CharacterSelectionCancelled{}); }, "borrowed owner lifetime");
		rejects([&] { old->invalidateInventory(); }, "borrowed owner lifetime");
		rejects([&] { old->refuseInventorySave(); }, "borrowed owner lifetime");
		rejects([&] { old->abandonPresentation(); }, "borrowed owner lifetime");
		rejects([&] { old->beginCycle(1); }, "borrowed owner lifetime");
		check(!old->encounterFrameCurrent(), "stale borrow reported a current frame");
		check(d.calls == calls, "stale Flow entered destination resources");
		save_test::sameSnapshot(before, d.capture());
		old.reset(); save_test::sameSnapshot(before, d.capture());
	}
	// Replace each owner separately. Unchanged sibling lifetimes cannot mask it.
	for (unsigned owner = 0; owner < 5; ++owner) {
		Destination d;
		XeenEventSystem eventSystem(eventProvider, textProvider);
		auto makeFlow = [&] { return std::make_unique<XeenEventFlow>(d.world, eventSystem, d.party, d.camera, d.flags, font, compose); };
		auto old = makeFlow();
		if (owner == 0) { d.world.~XeenWorld(); new (&d.world) XeenWorld([](XeenMapIdentity) { return map(); }, [](XeenMapIdentity) { return objects(); }); }
		if (owner == 1) { d.party.~XeenPartyState(); new (&d.party) XeenPartyState(XeenPartyLoader().loadFromResources(chr(), pty())); }
		if (owner == 2) { auto roster = d.party.roster; d.party.roster.~XeenRoster(); new (&d.party.roster) XeenRoster(roster); }
		if (owner == 3) { auto camera = d.camera; d.camera.~XeenCamera(); new (&d.camera) XeenCamera(camera); }
		if (owner == 4) { auto flags = d.flags; d.flags.~XeenGameFlags(); new (&d.flags) XeenGameFlags(flags); }
		const auto before = d.capture();
		rejects([&] { old->handle(NavigationAction::TurnRight); }, "borrowed owner lifetime");
		save_test::sameSnapshot(before, d.capture());
		auto current = makeFlow(); old.reset();
		const auto direction = d.camera.direction;
		current->handle(NavigationAction::TurnRight);
		check(d.camera.direction == XeenDirection((unsigned(direction) + 1) % 4), "old lease destruction damaged current Flow");
		const auto calls = d.calls; rejects([&] { d.restore(snapshot); }, "unborrowed");
		check(d.calls == calls, "old lease destruction cleared a current borrow");
		current.reset(); d.restore(snapshot); save_test::sameSnapshot(snapshot, d.capture());
	}
}
void retainedNestedPreimages() {
	const auto snapshot = regional_test::snapshot();
	for (bool swallow : {false, true}) {
		Destination d; const auto before = d.capture();
		bool nestedRefused = false, providerEntered = false;
		rejects([&] { d.restore(snapshot, [&](XeenWorld &w, const XeenPartyState &p, const XeenCamera &, const XeenGameFlags &) {
			auto &sp = const_cast<XeenPartyState &>(p).roster.at(29).currentSp;
			const auto calls = d.calls; ++sp;
			try { w.map(21); } catch (...) {
				--sp; nestedRefused = true; providerEntered = d.calls != calls;
				if (!swallow) throw; return;
			}
			--sp; providerEntered = d.calls != calls;
		}); });
		check(nestedRefused && !providerEntered, "nested restore entered provider with corrupt SP");
		save_test::sameSnapshot(before, d.capture());
	}
}
void newlyPopulatedCachePreimages() {
	const auto snapshot = regional_test::snapshot();
	for (unsigned mutation = 0; mutation < 3; ++mutation) {
		Destination d;
		const auto before = d.capture();
		const auto mapCount = d.world.cachedMapCount(), objectCount = d.world.cachedObjectFileCount();
		bool loaded = false, corrupted = false;
		auto preflight = [&](XeenWorld &w, const XeenPartyState &p, const XeenCamera &, const XeenGameFlags &) {
			check(p.roster.combatMarked(), "preflight did not follow legitimate completed phase transition");
			const auto calls = d.calls;
			const auto &m = w.map(21); const auto &o = w.objectFile(21);
			check(d.calls[unsigned(Seam::Map)] == calls[unsigned(Seam::Map)] + 1 &&
				d.calls[unsigned(Seam::Mob)] == calls[unsigned(Seam::Mob)] + 1, "valid nested resources were not loaded");
			loaded = true;
			if (mutation == 1) const_cast<XeenMap &>(m).geometry.trapDamage = 231;
			if (mutation == 2) const_cast<XeenObjectFile &>(o).entities.monsters[0].x = 3;
			corrupted = mutation != 0;
		};
		auto operation = [&] { d.restore(snapshot, preflight); };
		if (mutation) {
			rejects(operation); save_test::sameSnapshot(before, d.capture());
			check(loaded && corrupted, "cache rejection did not exercise newly populated corruption");
			check(d.world.cachedMapCount() == mapCount &&
				d.world.cachedObjectFileCount() == objectCount, "corrupt new cache reached live publication");
			check(d.world.map(21).geometry.trapDamage != 231 && d.world.objectFile(21).entities.monsters[0].x != 3,
				"failed preparation polluted retained loaders/caches");
		} else {
			operation(); const auto calls = d.calls;
			check(xeen_state::sameMap(d.world.map(21), [&] { auto m = d.terrain; m.geometry.id = 21; return m; }()), "valid new map differs");
			d.world.objectFile(21); check(d.calls == calls, "valid prepared caches were not published");
			auto expected = snapshot;
			save_test::sameSnapshot(expected, d.capture());
		}
	}
}
}
int main(){try{
 {regional_journey_test::Fixture producer;producer.engage();
  auto *combat=producer.flow->combat();while(combat->phase()==XeenCombatPhase::PlayerReady)producer.command(XeenCombatCommand::Block);
  combat->service(combat->ticket());}
 check(replay_test::journeyInitializations&&replay_test::journeyConstructions&&replay_test::services,"Replay probes observed genuine regional gameplay");
 crossWorldBorrowFreshness();staleFlowOwnerLifetimes();retainedNestedPreimages();newlyPopulatedCachePreimages();check(replay_test::unexpected==0,"Restore invoked a gameplay replay seam");std::cout<<"Regional lifetime/cache guards passed\n";return 0;}catch(const std::exception &e){std::cerr<<e.what()<<"\n";return 1;}}
