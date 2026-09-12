#include "XeenCombatTestSupport.h"
#include "XeenRestoreReplayProbe.h"
#include "games/xeen/CloudsUiComposer.h"
#include "games/xeen/XeenOutdoorScene.h"
#include "app/XeenEventFlow.h"
#include "games/xeen/XeenStateEquality.h"
#include <iostream>
#include <new>
using namespace combat_test;

namespace {
enum class Seam { Initial, Chr, Pty, Mon, Map, Mob, Evt, Preflight };
struct Destination {
	XeenPartyState party = XeenPartyLoader().loadFromResources(chr(), pty());
	XeenCamera camera{20, 14, 2, XeenDirection::West};
	XeenGameFlags flags;
	XeenMap terrain = map();
	XeenObjectFile mob = objects();
	std::function<void(Seam)> observer;
	std::array<unsigned, 8> calls{};
	void observe(Seam seam) { ++calls[unsigned(seam)]; if (observer) observer(seam); }
	XeenWorld world{[&](XeenMapIdentity id) { observe(Seam::Map); auto m = terrain; m.geometry.id = id.number; return m; },
		[&](XeenMapIdentity id) { observe(Seam::Mob); auto m = mob; m.mapId = id; return m; }};
	XeenSaveState::Resources resources() {
		return {save_test::sample().resources,
			[&] { observe(Seam::Initial); return XeenPartyLoader().loadFromResources(chr(), pty()); },
			[&](XeenMapIdentity id) { observe(Seam::Evt); auto e = events(); e.mapId = id; e.records.emplace_back(); return e; },
			[&] { observe(Seam::Chr); return chr(); },
			[&] { observe(Seam::Pty); return XeenGameplayContextFormat::parse(pty()); },
			[&] { observe(Seam::Mon); return statistics(); }};
	}
	XeenSaveSnapshot capture() { return XeenSaveState::capture(save_test::sample().resources, party, camera, flags, world); }
	static void compose(XeenWorld &w, const XeenPartyState &p, const XeenCamera &c, const XeenGameFlags &) {
		check(w.sessionState().completion() == XeenEncounterCompletion::VictoryQuiescent, "preflight did not receive completed facts");
		check(!XeenSaveState::canCapture(p, c, w), "preflight acquired unpublished capture authority");
		const auto faces = CloudsUiComposer::buildPortraitPlacements(p);
		const auto hp = CloudsUiComposer::buildHpPlacements(p, {610});
		check(faces.size() == 6 && hp.size() == 6, "completed portrait/HP composition");
		const auto scene = XeenOutdoorScene().build(w, c);
		check(!scene.empty(), "completed scene preflight");
		for (auto owner : kXeenCombatOwners) {
			check(p.roster.combatInputs(owner).has_value(), "completed inspection supplement missing");
			const auto &character = p.roster.at(owner);
			static_cast<void>(XeenCharacterRules::maxHp(character, {610}));
			static_cast<void>(XeenCharacterRules::maxSp(character, {610}));
			check(character.portraitResourceName().has_value(), "completed portrait resource missing");
		}
	}
	void restore(const XeenSaveSnapshot &snapshot, XeenSaveState::Preflight extra = {}) {
		replay_test::Scope noReplay;
		XeenSaveState::restoreBeforeGameplay(snapshot, resources(), party, camera, flags, world,
			[&](XeenWorld &w, const XeenPartyState &p, const XeenCamera &c, const XeenGameFlags &f) {
				observe(Seam::Preflight); compose(w, p, c, f); if (extra) extra(w, p, c, f);
			});
	}
	XeenCompletedReentry reenter(const XeenCompletedEncounterTicket &ticket, XeenWorld::CompletedPreflight extra = {}) {
		replay_test::Scope noReplay;
		auto r = resources();
		return world.reenterCompletedEncounter(ticket, party, camera, flags, r.loadMonsterStatistics, r.loadEvents,
			[&](XeenWorld &w, const XeenPartyState &p, const XeenCamera &c, const XeenGameFlags &f) {
				observe(Seam::Preflight); compose(w, p, c, f); if (extra) extra(w, p, c, f);
			});
	}
};

std::vector<XeenActor> expectedActors(const XeenObjectFile &mob = objects());
XeenSaveSnapshot completed(unsigned seed = 1, bool prepared = false, XeenObjectFile mob = objects()) {
	CombatFixture f{XeenCombatRandom(seed), mob};
	if (prepared) {
		check(f.combat->transfer(f.combat->ticket(), 5, 0, XeenInventoryCategory::Accessories, 1).status == XeenTransferStatus::Success,
			"genuine preparation transfer");
		check(f.combat->equipment(f.combat->ticket(), 0, XeenInventoryCategory::Accessories, 1, XeenEquipmentOperation::Equip).status == XeenEquipmentStatus::Success,
			"genuine preparation equip");
	}
	f.enter(); f.blockRound();
	unsigned budget = 10000;
	while (f.combat->phase() != Phase::Victory && --budget) {
		if (f.combat->phase() == Phase::PlayerReady) f.action(Command::Attack);
		else f.service();
		check(f.combat->phase() != Phase::Defeat && f.combat->phase() != Phase::SupportStopped && f.combat->phase() != Phase::Failed,
			"genuine completed synthetic control stopped");
	}
	check(budget != 0, "completed synthetic control exceeded budget");
	f.combat->retireCompletedVictory(f.combat->ticket());
	const std::array<int, 6> hp = seed == 1 ? std::array<int, 6>{12,16,12,10,-4,5} : std::array<int, 6>{12,16,12,10,-17,5};
	for (unsigned i = 0; i < 6; ++i) {
		const auto owner = kXeenCombatOwners[i];
		check(f.p.roster.at(owner).currentHp == hp[i], "independent injured victory HP oracle");
		check(f.p.roster.combatInputs(owner)->experience == (seed == 1 ? 82u : owner == 1 ? 0u : 100u),
			"independent victory XP oracle");
	}
	check(f.p.encounterContext->minutes == (seed == 1 ? 493 : 492) && f.p.encounterContext->ctr24 == 1,
		"independent completed context oracle");
	check(f.p.roster.at(1).conditions[12] == 1 && f.p.roster.at(1).conditions[13] == (seed == 56),
		"simultaneous injury oracle");
	if (seed == 56) {
		check(xeenSameItem(f.p.roster.at(1).armor[0], {0,2,128,3}) &&
			xeenSameItem(f.p.roster.at(1).armor[1], {38,10,128,9}), "broken equipped armor oracle");
		if (prepared) check(xeenSameItem(f.p.roster.at(0).accessories[1], {86,1,0,8}) &&
			xeenSameItem(f.p.roster.at(6).accessories[1], {}), "prepared transfer/equip oracle");
	}
	sameActors(expectedActors(mob), f.w.sessionState().actors());
	return XeenSaveFormat::decode(XeenSaveFormat::encode(XeenSaveState::capture(save_test::sample().resources, f.p, f.camera, {}, f.w)));
}

std::vector<XeenActor> expectedActors(const XeenObjectFile &mob) {
	// Independent constructor oracle: no saved actor data and no production conversion.
	std::vector<XeenActor> result;
	const auto mon = statistics();
	for (unsigned i = 0; i < 27; ++i) {
		XeenActor actor;
		actor.id = {20, i}; actor.original = mob.entities.monsters[i];
		actor.x = actor.original.x; actor.y = actor.original.y;
		if (actor.original.hasResource()) {
			actor.statistics = mon.at(actor.original.resourceId); actor.hp = actor.statistics->baseHp();
			actor.lifecycle = XeenActorLifecycle::Present;
		}
		if (actor.original.isDisabled()) actor.lifecycle = XeenActorLifecycle::Disabled;
		if (i == 5) { actor.x = actor.y = -128; actor.hp = 0; actor.lifecycle = XeenActorLifecycle::Defeated; }
		result.push_back(actor);
	}
	return result;
}

void roundtripAndEntry() {
	for (auto seed : {1u, 56u}) {
		auto snapshot = completed(seed, seed == 56);
		// Transfer data may describe an already completed re-entry camera.
		snapshot.camera = {20, 14, 2, XeenDirection::West};
		snapshot.disabledObjects = {{20, 0}, {21, 0}};
		snapshot.disabledEvents = {{21, 0}};
		snapshot.questItems[3] = 42; snapshot.questFlags[5] = true; snapshot.gameFlags[8] = true;
		Destination d;
		XeenCompletedEncounterTicket candidateTicket;
		d.restore(snapshot, [&](XeenWorld &w, const XeenPartyState &p, const XeenCamera &c, const XeenGameFlags &) {
			candidateTicket = w.completedTicket(p, c);
			rejects([&] { w.holdCompletedGuard(candidateTicket, XeenCompletedGuard::Operation, p, c); }, "stale");
		});
		save_test::sameSnapshot(snapshot, d.capture()); sameActors(expectedActors(), d.world.sessionState().actors());
		const auto metadata = XeenPartyLoader().loadFromResources(chr(), pty());
		check(d.party.firstSerializedCount == metadata.firstSerializedCount &&
			d.party.effectiveSerializedCount == metadata.effectiveSerializedCount && d.party.diagnostics == metadata.diagnostics,
			"completed restore discarded loader metadata");
		for (unsigned owner = 0; owner < 30; ++owner)
			check(bool(d.party.roster.combatInputs(owner)) == (std::find(kXeenCombatOwners.begin(), kXeenCombatOwners.end(), owner) != kXeenCombatOwners.end()),
				"completed supplemental owner presence mismatch");
		rejects([&] { d.world.holdCompletedGuard(candidateTicket, XeenCompletedGuard::Operation, d.party, d.camera); }, "stale");
		const auto initialGeneration = d.world.completedEntryGeneration();
		d.world.discardMapCache(); d.world.map(20); d.world.objectFile(20);
		check(d.world.completedEntryGeneration() == initialGeneration, "cache rebuild advanced entry generation");
		save_test::sameSnapshot(snapshot, d.capture()); sameActors(expectedActors(), d.world.sessionState().actors());
		for (unsigned i = 0; i < 2; ++i) {
			const auto old = d.world.completedTicket(d.party, d.camera);
			const auto counts = d.calls;
			const auto receipt = d.reenter(old, [&](XeenWorld &, const XeenPartyState &, const XeenCamera &, const XeenGameFlags &) {
				rejects([&] { d.reenter(old); }, "stale");
			});
			check(receipt.oldGeneration == initialGeneration + i && receipt.newGeneration == initialGeneration + i + 1,
				"true entry generation did not advance");
			for (auto seam : {Seam::Map, Seam::Mob, Seam::Mon, Seam::Evt, Seam::Preflight})
				check(d.calls[unsigned(seam)] > counts[unsigned(seam)], "warm-cache re-entry skipped resource construction");
			rejects([&] { d.reenter(old); }, "stale");
			snapshot.camera = XeenActorApproach::kEntry;
			save_test::sameSnapshot(snapshot, d.capture()); sameActors(expectedActors(), d.world.sessionState().actors());
		}
		Destination second; second.restore(d.capture()); save_test::sameSnapshot(snapshot, second.capture());
		rejects([&] { second.reenter(d.world.completedTicket(d.party, d.camera)); }, "stale");
	}
	auto mob = objects();
	mob.entities.monsters[0] = {0,15,2,1,2};
	mob.entities.monsters[1] = {1,15,5,2,-1};
	const auto snapshot = completed(1, false, mob);
	Destination d; d.mob = mob; d.restore(snapshot);
	sameActors(expectedActors(mob), d.world.sessionState().actors());
	d.world.discardMapCache(); d.world.map(20); d.world.objectFile(20);
	sameActors(expectedActors(mob), d.world.sessionState().actors());
	d.reenter(d.world.completedTicket(d.party, d.camera));
	sameActors(expectedActors(mob), d.world.sessionState().actors());
	check(d.world.sessionState().actors()[0].hp == statistics()[2].baseHp() &&
		d.world.sessionState().actors()[1].lifecycle == XeenActorLifecycle::Unresolved &&
		!d.world.sessionState().actors()[1].statistics, "completed surviving/unresolved resource semantics");
}

void restoreFailureMatrix() {
	const auto snapshot = completed();
	for (unsigned seam = 0; seam < 8; ++seam) for (unsigned mode = 0; mode < 4; ++mode) {
		Destination d; d.world.map(20); d.world.objectFile(20);
		const auto before = d.capture(); const auto maps = d.world.cachedMapCount(), mobs = d.world.cachedObjectFileCount();
		d.observer = [&](Seam at) {
			if (unsigned(at) != seam) return;
			if (mode == 1) ++d.party.roster.at(29).currentSp;
			if (mode == 2) { auto replacement = XeenPartyLoader().loadFromResources(chr(), pty()); d.party = replacement; }
			if (mode == 3) {
				d.world.~XeenWorld();
				new (&d.world) XeenWorld([](XeenMapIdentity) { return map(); }, [](XeenMapIdentity) { return objects(); });
			}
			if (mode != 2) throw std::runtime_error("provider fault");
		};
		rejects([&] { d.restore(snapshot); });
		d.observer = {};
		check(!d.party.roster.combatMarked() && !d.world.hasEncounterState(), "failed candidate leaked completed publication");
		if (mode == 0 || mode == 2) {
			save_test::sameSnapshot(before, d.capture());
			check(d.world.cachedMapCount() == maps && d.world.cachedObjectFileCount() == mobs, "provider failure published candidate caches");
		}
		if (mode == 1) check(d.party.roster.at(29).currentSp == before.characters[29].currentSp + 1, "restore rolled back external mutation");
		if (mode == 3) check(d.world.cachedMapCount() == 0 && d.world.cachedObjectFileCount() == 0, "restore overwrote replacement world");
		// Calling retained destination loaders after failure must use their original closures.
		d.world.discardMapCache(); d.world.map(20); d.world.objectFile(20);
	}
	for (unsigned mode = 0; mode < 9; ++mode) {
		Destination d; const auto before = d.capture();
		rejects([&] { d.restore(snapshot, [&](XeenWorld &w, const XeenPartyState &p, const XeenCamera &c, const XeenGameFlags &f) {
			if (mode == 0) ++const_cast<XeenPartyState &>(p).roster.at(29).currentSp;
			if (mode == 1) ++const_cast<XeenCamera &>(c).x;
			if (mode == 2) const_cast<XeenGameFlags &>(f).set(4);
			if (mode == 3) ++const_cast<std::vector<XeenActor> &>(w.sessionState().actors())[0].hp;
			if (mode == 4) ++const_cast<XeenMap &>(w.map(20)).geometry.trapDamage;
			if (mode == 5) { w.~XeenWorld(); new (&w) XeenWorld([](XeenMapIdentity) { return map(); }); }
			if (mode == 6) { auto &owner = const_cast<XeenPartyState &>(p); owner.~XeenPartyState(); new (&owner) XeenPartyState; }
			if (mode == 7) const_cast<std::optional<XeenCombatInputs> &>(p.roster.combatInputs(0)).reset();
			if (mode == 8) const_cast<XeenPartyState &>(p).encounterContext.reset();
		}); });
		save_test::sameSnapshot(before, d.capture());
	}
}

void invalidDomain() {
	const auto original = completed();
	for (unsigned mode = 0; mode < 9; ++mode) {
		auto snapshot = original;
		if (mode == 0) ++snapshot.completedEncounter->supplements[0].inputs.might.permanent;
		if (mode == 1) ++snapshot.completedEncounter->supplements[0].inputs.experience;
		if (mode == 2) snapshot.characters[0].currentHp = 0;
		if (mode == 3) snapshot.characters[0].conditions[0] = 1;
		if (mode == 4) snapshot.characters[0].weapons[8].material = 2;
		if (mode == 5) snapshot.characters[0].weapons[0].frame = 13;
		if (mode == 6) ++snapshot.characters[29].currentSp;
		if (mode == 7) snapshot.characters[0].weapons[0].state |= 0x80;
		if (mode == 8) ++snapshot.characters[0].armor[0].material;
		XeenSaveFormat::validate(snapshot);
		Destination d; const auto before = d.capture(); rejects([&] { d.restore(snapshot); }); save_test::sameSnapshot(before, d.capture());
	}
	for (unsigned mode = 0; mode < 4; ++mode) {
		Destination d; auto r = d.resources(); const auto before = d.capture();
		if (mode == 0) r.loadInitialCharacters = [] { auto b = chr(); ++b[20]; return b; };
		if (mode == 1) r.loadInitialContext = [] { auto c = XeenGameplayContextFormat::parse(pty()); ++c.minutes; return c; };
		if (mode == 2) r.loadMonsterStatistics = [] { auto s = statistics(); ++s[8].raw[20]; return s; };
		if (mode == 3) r.loadInitialParty = [] { auto p = XeenPartyLoader().loadFromResources(chr(), pty()); p.encounterContext.emplace(); return p; };
		rejects([&] { XeenSaveState::restoreBeforeGameplay(original, r, d.party, d.camera, d.flags, d.world, Destination::compose); });
		save_test::sameSnapshot(before, d.capture());
	}
	for (unsigned mode = 0; mode < 4; ++mode) {
		Destination d;
		if (mode == 0) d.terrain.geometry.flags = 1;
		if (mode == 1) d.mob.entities.monsters.pop_back();
		if (mode == 2) d.mob.entities.monsters[5].x = 14;
		if (mode == 3) d.mob.entities.monsters[0].x = 13;
		const auto before = d.capture(); rejects([&] { d.restore(original); }); save_test::sameSnapshot(before, d.capture());
	}
}

void actorIntegrityAndNestedProviders() {
	const auto snapshot = completed();
	for (unsigned field = 0; field < 15; ++field) for (unsigned owner : {0u, 5u}) {
		Destination d; d.restore(snapshot);
		const auto ticket = d.world.completedTicket(d.party, d.camera);
		auto &actor = const_cast<std::vector<XeenActor> &>(d.world.sessionState().actors())[owner];
		const auto original = actor;
		switch (field) {
		case 0: ++actor.id.recordIndex; break;
		case 1: ++actor.original.x; break;
		case 2: ++actor.original.y; break;
		case 3: ++actor.original.direction; break;
		case 4: ++actor.original.tableIndex; break;
		case 5: ++actor.original.resourceId; break;
		case 6: ++actor.x; break;
		case 7: ++actor.y; break;
		case 8: ++actor.hp; break;
		case 9: actor.activated = true; break;
		case 10: actor.lifecycle = XeenActorLifecycle::Present; break;
		case 11: actor.statistics.reset(); break;
		case 12: ++actor.statistics->raw[0]; break;
		case 13: actor.status = XeenActorStatus::Unsupported; break;
		case 14: ++actor.id.mapId.number; break;
		}
		rejects([&] { sameActors(expectedActors(), d.world.sessionState().actors()); });
		rejects([&] { d.reenter(ticket); }, "stale");
		actor = original;
		check(!XeenSaveState::canCapture(d.party, d.camera, d.world), "restoring actor bytes cleared unsafe authority");
	}
	for (auto seam : {Seam::Map, Seam::Mob}) {
		Destination d; const auto before = d.capture();
		rejects([&] { d.restore(snapshot, [&](XeenWorld &w, const XeenPartyState &p, const XeenCamera &, const XeenGameFlags &) {
			d.observer = [&](Seam at) { if (at == seam) ++const_cast<XeenPartyState &>(p).roster.at(29).currentSp; };
			if (seam == Seam::Map) w.map(21); else w.objectFile(21);
		}); });
		d.observer = {}; save_test::sameSnapshot(before, d.capture());
	}
}

void crossWorldBorrowFreshness() {
	const auto snapshot = completed();
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
		d.camera.~XeenCamera(); new (&d.camera) XeenCamera{20,14,2,XeenDirection::West};
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
	const auto snapshot = completed();
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
		d.camera.~XeenCamera(); new (&d.camera) XeenCamera{20,14,2,XeenDirection::West};
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
	const auto snapshot = completed();
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
	for (bool live : {false, true}) {
		Destination d; d.restore(snapshot); const auto before = d.capture();
		bool nestedRefused = false, providerEntered = false;
		const auto generation = d.world.completedEntryGeneration();
		rejects([&] { d.reenter(d.world.completedTicket(d.party, d.camera), [&](XeenWorld &w, const XeenPartyState &, const XeenCamera &, const XeenGameFlags &) {
			auto &hp = const_cast<std::vector<XeenActor> &>((live ? d.world : w).sessionState().actors())[5].hp;
			const auto calls = d.calls; ++hp;
			try { w.map(21); } catch (...) {
				--hp; nestedRefused = true; providerEntered = d.calls != calls; return;
			}
			--hp; providerEntered = d.calls != calls;
		}); });
		check(nestedRefused && !providerEntered, "nested re-entry entered provider with corrupt HP");
		check(d.world.completedEntryGeneration() == generation, "nested corruption advanced entry");
		sameActors(expectedActors(), d.world.sessionState().actors());
		check(xeen_state::sameCamera(d.camera, before.camera), "nested corruption moved live camera");
		if (live) check(!XeenSaveState::canCapture(d.party, d.camera, d.world), "nested live integrity violation did not latch");
		else save_test::sameSnapshot(before, d.capture());
	}
}

void newlyPopulatedCachePreimages() {
	const auto snapshot = completed();
	for (bool entry : {false, true}) for (unsigned mutation = 0; mutation < 3; ++mutation) {
		Destination d; if (entry) d.restore(snapshot);
		const auto before = d.capture(); const auto generation = d.world.completedEntryGeneration();
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
		auto operation = [&] { if (entry) d.reenter(d.world.completedTicket(d.party, d.camera), preflight); else d.restore(snapshot, preflight); };
		if (mutation) {
			rejects(operation); save_test::sameSnapshot(before, d.capture());
			check(loaded && corrupted, "cache rejection did not exercise newly populated corruption");
			check(d.world.completedEntryGeneration() == generation && d.world.cachedMapCount() == mapCount &&
				d.world.cachedObjectFileCount() == objectCount, "corrupt new cache reached live publication");
			check(d.world.map(21).geometry.trapDamage != 231 && d.world.objectFile(21).entities.monsters[0].x != 3,
				"failed preparation polluted retained loaders/caches");
		} else {
			operation(); const auto calls = d.calls;
			check(xeen_state::sameMap(d.world.map(21), [&] { auto m = d.terrain; m.geometry.id = 21; return m; }()), "valid new map differs");
			d.world.objectFile(21); check(d.calls == calls, "valid prepared caches were not published");
			auto expected = snapshot; if (entry) expected.camera = XeenActorApproach::kEntry;
			save_test::sameSnapshot(expected, d.capture());
		}
	}
}

void entryFailures() {
	const auto snapshot = completed();
	for (auto seam : {Seam::Map, Seam::Mob, Seam::Mon, Seam::Evt, Seam::Preflight}) for (bool mutate : {false, true}) {
		Destination d; d.restore(snapshot); const auto before = d.capture();
		const auto generation = d.world.completedEntryGeneration();
		const auto old = d.world.completedTicket(d.party, d.camera);
		d.observer = [&](Seam at) { if (at == seam) { if (mutate) ++d.party.roster.at(29).currentSp; throw std::runtime_error("entry fault"); } };
		rejects([&] { d.reenter(old); }); d.observer = {};
		check(d.world.completedEntryGeneration() == generation, "failed re-entry advanced generation");
		sameActors(expectedActors(), d.world.sessionState().actors());
		if (!mutate) { save_test::sameSnapshot(before, d.capture()); d.reenter(d.world.completedTicket(d.party, d.camera)); }
		else {
			check(!XeenSaveState::canCapture(d.party, d.camera, d.world), "entry integrity mutation was not latched");
			--d.party.roster.at(29).currentSp;
			check(!XeenSaveState::canCapture(d.party, d.camera, d.world), "repair cleared entry integrity latch");
		}
	}
}
}
int main() { try {
	roundtripAndEntry(); restoreFailureMatrix(); invalidDomain(); entryFailures(); actorIntegrityAndNestedProviders();
	crossWorldBorrowFreshness(); retainedNestedPreimages(); newlyPopulatedCachePreimages();
	staleFlowOwnerLifetimes();
	check(replay_test::constructions && replay_test::services && replay_test::preparations,
		"gameplay replay probes did not observe the genuine producer");
	check(replay_test::unexpected == 0, "restore/re-entry invoked a gameplay replay seam");
	std::cout << "Completed restore and bounded re-entry tests passed\n"; return 0;
} catch (const std::exception &e) { std::cerr << e.what() << '\n'; return 1; } }
