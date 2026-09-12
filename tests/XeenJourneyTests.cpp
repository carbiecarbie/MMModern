#include "XeenJourneyTestSupport.h"
#include "XeenCombatTestSupport.h"
#include "XeenRemoveTestSupport.h"
#include "app/XeenEncounterFlow.h"
#include "games/xeen/XeenJourneyRules.h"
#include "games/xeen/XeenJourneyProgression.h"
#include "games/xeen/XeenEventInterpreter.h"
#include "games/xeen/XeenStateEquality.h"
#include <iostream>
#include <limits>
#include <new>
using namespace combat_test;
namespace {
using journey_test::Fixture;
void lifecycle() {
	Fixture f;
	check(f.flow->journeyQuiet() && !f.flow->combat(), "quiet initial Journey has no combat");
	for (unsigned i = 0; i < 30; ++i) check(f.p.roster.combatInputs(i).has_value() &&
		f.p.roster.combatInputs(i)->experience == 0, "all owner zero XP records present");
	rejects([&] { XeenPartyState copy(f.p); }, "marked");
	const auto old = f.flow->ticket();
	check(f.flow->journeyTransfer(old,5,0,XeenInventoryCategory::Accessories,1).status == XeenTransferStatus::Success,
		"current owners transfer before combat exists");
	check(!f.flow->current(old), "item mutation consumes old input");
	check(!f.flow->journeyQuiet() && !f.flow->presentJourney(old), "item presentation requires the current generation");
	f.present();
	check(f.flow->journeyEquipment(f.flow->ticket(),0,XeenInventoryCategory::Accessories,1,XeenEquipmentOperation::Equip).status ==
		XeenEquipmentStatus::Success, "equip before combat exists");
	f.present();
	check(f.p.encounterContext->minutes == 480 && f.p.encounterContext->ctr24 == 0, "items consume no time");
	f.engage();
	for (unsigned i = 0; i < 6; ++i) f.command(Command::Block);
	auto *combat = f.flow->combat();
	while (combat->pending() == Work::Enemy) combat->service(combat->ticket());
	check(combat->pending() == Work::Round, "seed56 pending round");
	combat->service(combat->ticket());
	f.command(Command::Attack); f.command(Command::Attack);
	check(combat->phase() == Phase::VictoryAwaitingEnd, "seed56 literal lethal trace");
	check(!f.flow->retireJourney(f.flow->ticket()), "lethal is not End");
	check(f.w.sessionState().accountedMonsters().count({20,5}) == 1 && !f.w.sessionState().combatAccounted(),
		"Journey uses identity accounting, not diagnostic once-ever boolean");
	const auto stale = combat->ticket();
	check(combat->service(stale).status == Status::Victory, "genuine End");
	const auto retirement = f.flow->ticket();
	check(f.flow->retireJourney(retirement), "guarded Journey retirement");
	check(!f.flow->retireJourney(retirement) && !f.flow->journeyQuiet(), "retirement is once-only and frame guarded");
	f.present();
	const int hp[]{12,16,12,10,-17,5}, sp[]{2,0,2,0,7,9};
	for (unsigned i = 0; i < 6; ++i) {
		const auto &c = f.p.roster.at(kXeenCombatOwners[i]);
		check(c.currentHp == hp[i] && c.currentSp == sp[i], "literal seed56 HP/SP");
		check(f.p.roster.combatInputs(c.rosterId)->experience == (c.rosterId == 1 ? 0u : 100u), "literal seed56 XP");
	}
	check(f.p.encounterContext->minutes == 492 && f.p.roster.at(1).conditions[12] == 1 && f.p.roster.at(1).conditions[13] == 1,
		"End time and simultaneous injury flags");
	check(f.flow->journeyTransfer(f.flow->ticket(),0,5,XeenInventoryCategory::Accessories,1).status == XeenTransferStatus::Success,
		"real post-retirement transfer");
	f.present();
	check(f.action(XeenEncounterAction::Right).outcome == XeenEncounterOutcome::Accepted, "mutable return facing");
	check(f.action(XeenEncounterAction::Forward).outcome == XeenEncounterOutcome::Accepted, "mutable return movement");
	check(f.flow->state().pending() == 3 && !f.flow->journeyQuiet(), "post-victory pending authority");
	f.pulse(); f.pulse(); f.pulse();
	check(f.flow->journeyQuiet() && f.camera.x == 14 && f.camera.y == 1 && f.p.encounterContext->minutes == 502,
		"post-victory context charge");
	check(f.w.sessionState().skeletonSeed() == 56, "stored seed is unchanged");
	check(f.w.sessionState().completion() == XeenEncounterCompletion::None, "no completed diagnostic result model");
	check(f.flow->journeyEquipment(f.flow->ticket(),4,XeenInventoryCategory::Armor,0,XeenEquipmentOperation::Remove).status == XeenEquipmentStatus::Success,
		"dead owner may remove broken armor"); f.present();
	check(f.flow->journeyTransfer(f.flow->ticket(),4,1,XeenInventoryCategory::Armor,0).status == XeenTransferStatus::Success,
		"broken armor transfers without repair"); f.present();
	check(xeenSameItem(f.p.roster.at(18).armor[4],{0,2,128,0}), "broken armor raw state after transfer");
	check(f.flow->journeyEquipment(f.flow->ticket(),1,XeenInventoryCategory::Armor,0,XeenEquipmentOperation::Remove).status == XeenEquipmentStatus::Success,
		"remove destination chest armor"); f.present();
	check(f.flow->journeyEquipment(f.flow->ticket(),1,XeenInventoryCategory::Armor,4,XeenEquipmentOperation::Equip).status == XeenEquipmentStatus::Success,
		"broken armor can be equipped again"); f.present();
	check(xeenSameItem(f.p.roster.at(18).armor[4],{0,2,128,3}) && f.p.roster.at(1).currentHp == -17,
		"re-equipping changes only frame, preserving historical injury");
}
void approachAndGuards() {
	Fixture f;
	f.action(XeenEncounterAction::Right); f.action(XeenEncounterAction::Forward);
	check(f.flow->state().pending() == 3, "step arms three opportunities");
	for (unsigned n : {3u,2u,1u}) {
		check(f.flow->state().pending() == n && !f.flow->journeyQuiet(), "pending blocks quiet");
		check(f.flow->journeyTransfer(f.flow->ticket(),5,0,XeenInventoryCategory::Accessories,1).status != XeenTransferStatus::Success,
			"pending blocks items without draining work");
		f.pulse();
	}
	check(f.flow->journeyQuiet() && f.w.sessionState().actors()[5].x == 13 && f.w.sessionState().actors()[5].y == 1 &&
		f.camera.x == 14 && f.camera.y == 1 && f.p.encounterContext->minutes == 490 && f.p.encounterContext->ctr24 == 2,
		"independent moved-anchor oracle");
	const auto actors = f.w.sessionState().actors();
	f.w.discardMapCache();
	f.pulse();
	for (unsigned i = 0; i < 27; ++i) check(xeen_state::sameActor(actors[i],f.w.sessionState().actors()[i]), "cache rebuild preserves every actor field");
	check(f.action(XeenEncounterAction::Forward).outcome == XeenEncounterOutcome::Refused && f.flow->journeyQuiet(),
		"content edge is recoverable");
	f.flow.reset();
	check(f.w.sessionState().journeyActivity() == XeenJourneyActivity::Failed, "adapter destruction never opens quiet boundary");
	Fixture stale;
	stale.p.roster.at(0).currentSp = -9;
	check(!stale.flow->journeyQuiet(), "unadopted owner mutation latches integrity");
	stale.p.roster.at(0).currentSp = 2;
	check(!stale.flow->journeyQuiet(), "restoring bytes cannot undo integrity latch");
}
void currentValues() {
	auto bytes = chr();
	for (unsigned id = 0; id < 30; ++id) { bytes[id*354+348] = 0xe8; bytes[id*354+349] = 3; }
	bytes[1*354+342] = 0xf3; bytes[1*354+343] = 0xff; // HP -13, historical unconscious injury.
	bytes[1*354+323+12] = 1;
	Fixture f(bytes);
	check(f.p.roster.at(1).currentHp == -13, "historical HP installed exactly");
	f.engage(); f.lethal();
	for (auto id : kXeenCombatOwners) check(f.p.roster.combatInputs(id)->experience == 1082, "nonzero XP additive with unconscious eligible");
	check(f.p.roster.at(1).currentHp == -13, "historical injury retained");
}
void readinessAndHistory() {
	auto ordinary = XeenPartyLoader().loadFromResources(chr(),pty());
	check(!ordinary.encounterContext && !ordinary.roster.combatMarked(), "ordinary loading has no Journey context");
	for (unsigned i = 0; i < 30; ++i) check(!ordinary.roster.combatInputs(i), "ordinary loading has no supplements");
	auto bytes = chr();
	bytes[342] = 0xf3; bytes[343] = 0xff; bytes[323+12] = 1;
	bytes[344] = 0xf9; bytes[345] = 0xff;
	bytes[166+72+4] = 105; bytes[166+72+5] = 1; // Carried +4 HP ring.
	Fixture f(bytes);
	check(f.flow->journeyEquipment(f.flow->ticket(),0,XeenInventoryCategory::Accessories,1,XeenEquipmentOperation::Equip).status ==
		XeenEquipmentStatus::Success, "unsupported combat contribution remains equipable Journey data");
	f.present();
	check(XeenCharacterRules::maxHp(f.p.roster.at(0),{610}) == 16, "independent +4 maximum HP");
	xeenValidateJourneyParty(f.p);
	rejects([&] { xeenValidateJourneyMelee(f.p); }, "accessory");
	const auto actors = f.w.sessionState().actors();
	check(f.action(XeenEncounterAction::Wait).outcome == XeenEncounterOutcome::Refused && f.flow->journeyQuiet() &&
		f.p.encounterContext->minutes == 480, "unready action refuses before publication");
	check(f.flow->journeyRefusal().find("owner 0, slot 1, M/ID/S/F=105/1/0/8") != std::string::npos,
		"readiness refusal identifies the exact current record");
	sameActors(actors,f.w.sessionState().actors());
	check(f.flow->journeyEquipment(f.flow->ticket(),0,XeenInventoryCategory::Accessories,1,XeenEquipmentOperation::Remove).status ==
		XeenEquipmentStatus::Success, "repair loadout through actual equipment removal");
	f.present();
	const auto &c = f.p.roster.at(0);
	check(XeenCharacterRules::maxHp(c,{610}) == 12 && c.currentHp == -13 && c.currentSp == -7 &&
		c.conditions[12] == 1 && c.conditions[13] == 0, "historical injury never reclassified against reduced maximum");
	xeenValidateJourneyMelee(f.p); f.engage();
	check(f.p.roster.at(0).currentHp == -13 && f.p.roster.at(0).currentSp == -7, "attachment retains current HP/SP");
	Fixture raw;
	auto &owner = raw.p.roster.at(0);
	owner.miscellaneous[5] = {255,255,255,255}; owner.weapons[8] = {255,254,255,0};
	xeenValidateJourneyMelee(raw.p);
	owner.weapons[8] = {0,0,0,1};
	xeenValidateJourneyParty(raw.p); rejects([&] { xeenValidateJourneyMelee(raw.p); }, "weapon");
	owner.weapons[8] = {}; owner.armor[8] = {38,0,0,9}; owner.accessories[8] = {86,0,0,8};
	xeenValidateJourneyMelee(raw.p);
	owner.conditions[3] = 1; rejects([&] { xeenValidateJourneyParty(raw.p); }, "condition");
	owner.conditions[3] = 0; owner.conditions[12] = 2; rejects([&] { xeenValidateJourneyParty(raw.p); }, "condition");
	owner.conditions[12] = 1; rejects([&] { xeenValidateJourneyParty(raw.p); }, "signs");
	for (auto id : kXeenCombatOwners) { raw.p.roster.at(id).currentHp = 0; raw.p.roster.at(id).conditions[12] = 1; }
	rejects([&] { xeenValidateJourneyParty(raw.p); }, "acting");
	Fixture partial;
	const_cast<std::optional<XeenCombatInputs> &>(partial.p.roster.combatInputs(29)).reset();
	rejects([&] { xeenValidateJourneyParty(partial.p); }, "supplement");
	check(!partial.flow->journeyQuiet(), "partial supplement presence cannot be replenished");
}
void progression() {
	Fixture f;
	std::array<XeenCharacter,6> values;
	std::array<const XeenCharacter *,6> owners;
	std::array<XeenCombatInputs,6> inputs;
	for (unsigned i = 0; i < 6; ++i) {
		values[i] = f.p.roster.at(kXeenCombatOwners[i]); owners[i] = &values[i];
		inputs[i] = *f.p.roster.combatInputs(kXeenCombatOwners[i]); inputs[i].experience = 1000;
	}
	values[4].currentHp = -17; values[4].conditions[12] = values[4].conditions[13] = 1;
	values[2].currentHp = -13; values[2].conditions[12] = 1;
	const auto original = f.w.sessionState().actors()[5];
	const auto first = xeenPrepareJourneyLethal(original,owners,inputs,{});
	for (unsigned i = 0; i < 6; ++i) { check(first.experience[i] == (i == 4 ? 1000u : 1100u), "literal first identity XP"); inputs[i].experience = first.experience[i]; }
	rejects([&] { xeenPrepareJourneyLethal(original,owners,inputs,first.accounted); }, "identity");
	auto secondActor = original; secondActor.id.recordIndex = 6; // Synthetic identity, never production target selection.
	const auto second = xeenPrepareJourneyLethal(secondActor,owners,inputs,first.accounted);
	for (unsigned i = 0; i < 6; ++i) check(second.experience[i] == (i == 4 ? 1000u : 1200u), "literal composed identity XP");
	check(second.accounted.size() == 2 && values[2].currentHp == -13 && values[4].conditions[13] == 1, "identity composition preserves prior facts");
	values[0].permanentLevel = 15;
	const auto level15 = xeenPrepareJourneyLethal(secondActor,owners,inputs,first.accounted);
	check(level15.experience[0] == 1150, "literal permanent-level15 non-doubled arithmetic");
	inputs[0].experience = std::numeric_limits<std::uint32_t>::max();
	bool overflow = false; try { xeenPrepareJourneyLethal(secondActor,owners,inputs,first.accounted); } catch (const std::exception &) { overflow = true; }
	check(overflow && first.accounted.size() == 1 && original.hp == 20, "overflow publishes no lethal candidate");
	auto bytes = chr();
	// The first five recipients can be prepared; only the last recipient overflows.
	for (unsigned k = 0; k < 4; ++k) bytes[6*354+348+k] = 255;
	Fixture live(bytes); live.engage();
	for (unsigned i = 0; i < 100 && live.flow->combat()->phase() != Phase::Failed; ++i) {
		auto *combat = live.flow->combat();
		if (combat->phase() == Phase::PlayerReady) live.command(Command::Attack); else combat->service(combat->ticket());
	}
	check(live.flow->combat()->phase() == Phase::Failed && live.w.sessionState().accountedMonsters().empty() &&
		live.w.sessionState().actors()[5].lifecycle == XeenActorLifecycle::Present && live.w.sessionState().actors()[5].hp > 0,
		"real overflow does not publish defeat or partial XP");
	for (auto id : kXeenCombatOwners) check(live.p.roster.combatInputs(id)->experience ==
		(id == 6 ? std::numeric_limits<std::uint32_t>::max() : 0u), "later-recipient overflow preserves every XP");
}
void postEndInvalidation() {
	Fixture f; f.engage(); f.lethal();
	auto *combat = f.flow->combat();
	check(combat->service(combat->ticket()).status == Status::Victory, "post-End control genuinely ends");
	const auto old = combat->ticket(); const auto retirement = f.flow->ticket();
	const auto actors = f.w.sessionState().actors(); const auto characters = f.p.roster.characters();
	std::array<unsigned,6> xp{};
	for (unsigned i=0;i<6;++i) xp[i] = f.p.roster.combatInputs(kXeenCombatOwners[i])->experience;
	combat->invalidate(); combat->invalidate();
	check(!combat->current(old) && combat->service(old).status == Status::Stale, "post-End invalidation consumes old combat ticket");
	check(!f.flow->retireJourney(retirement) && !f.flow->retireJourney(f.flow->ticket()) && !f.flow->journeyQuiet(),
		"post-End integrity permanently refuses retirement");
	sameActors(actors,f.w.sessionState().actors());
	for(unsigned i=0;i<30;++i) check(xeen_state::sameCharacter(characters[i],f.p.roster.at(i)), "invalidation preserves injuries and items");
	for(unsigned i=0;i<6;++i) check(xp[i] == f.p.roster.combatInputs(kXeenCombatOwners[i])->experience, "invalidation preserves XP");
	Fixture failedEnd; failedEnd.engage(); failedEnd.lethal();
	auto *ending = failedEnd.flow->combat();
	ending->setProbe([&] { ending->invalidate(); throw std::runtime_error("invalidated End callback"); });
	ending->service(ending->ticket());
	check(!failedEnd.flow->retireJourney(failedEnd.flow->ticket()) &&
		failedEnd.w.sessionState().accountedMonsters().size() == 1 &&
		failedEnd.w.sessionState().actors()[5].lifecycle == XeenActorLifecycle::Defeated,
		"End callback invalidation and exception preserve lethal facts without retirement");
}
void approachBoundaryReplacement() {
	for (bool throws : {false,true}) {
		Fixture f; const auto old = f.flow->ticket(); const auto actors = f.w.sessionState().actors();
		f.w.discardMapCache();
		f.onMap = [&] {
			const auto lease = f.flow->boundary().hold(XeenCombatBoundary::Work::Inventory);
			f.flow->boundary().release(XeenCombatBoundary::Work::Inventory,lease);
			if (throws) throw std::runtime_error("provider after boundary replacement");
		};
		check(f.flow->journeyAction(old,XeenEncounterAction::Right).outcome != XeenEncounterOutcome::Accepted,
			"quiet boundary ABA refuses obsolete approach");
		check(f.p.encounterContext->ctr24 == 0 && f.flow->state().phase() == XeenEncounterPhase::Exploring &&
			!f.flow->current(old), "stale approach cannot charge or stop coordination");
		sameActors(actors,f.w.sessionState().actors());
		f.onMap = {};
		check(f.action(XeenEncounterAction::Right).outcome == XeenEncounterOutcome::Accepted, "new boundary can authorize fresh work");
		check(!f.flow->fail(old,XeenEncounterStop::Preparation) && f.flow->journeyQuiet(), "old operation cannot stop newer coordination");
	}
	Fixture held; held.w.discardMapCache();
	std::uint64_t newerLease = 0;
	held.onMap = [&] { newerLease = held.flow->boundary().hold(XeenCombatBoundary::Work::Inventory); };
	held.flow->journeyAction(held.flow->ticket(),XeenEncounterAction::Right);
	check(!held.flow->boundary().quiet() && held.p.encounterContext->ctr24 == 0 &&
		held.flow->state().phase() == XeenEncounterPhase::Exploring, "stale action cannot release or stop newer held work");
	held.flow->boundary().release(XeenCombatBoundary::Work::Inventory,newerLease);
}
void attachmentBoundaryReplacement() {
	// Sprite, initial map/MOB, and reconstruction after sprite cache discard.
	for (unsigned seam = 0; seam < 5; ++seam) for (bool throws : {false,true}) for (bool held : {false,true}) {
		Fixture f;
		check(f.action(XeenEncounterAction::Wait).outcome == XeenEncounterOutcome::Engaged, "attachment regression genuinely engages");
		const auto old = f.flow->ticket(); const auto actors = f.w.sessionState().actors();
		const auto context = *f.p.encounterContext; const auto revision = f.flow->state().revision();
		std::uint64_t lease = 0; unsigned callbacks = 0, sprites = 0;
		const auto replace = [&] {
			++callbacks;
			lease = f.flow->boundary().hold(XeenCombatBoundary::Work::Inventory);
			if (!held) f.flow->boundary().release(XeenCombatBoundary::Work::Inventory,lease);
			if (throws) throw std::runtime_error("attachment callback after boundary replacement");
		};
		if (seam == 1 || seam == 3) f.onMap = replace;
		if (seam == 2 || seam == 4) f.onObjects = replace;
		if (seam == 1 || seam == 2) f.w.discardMapCache();
		check(!f.flow->attachJourney(old,[&] {
			++sprites;
			if (seam == 0) replace();
			if (seam >= 3) f.w.discardMapCache();
		}), "obsolete attachment refuses normal and throwing callbacks");
		check(callbacks == 1 && !f.flow->combat() && !f.flow->current(old), "stale attachment never constructs combat or renews old ticket");
		check(sprites == ((seam == 1 || seam == 2) ? 0u : 1u), "stale provider prevents later preparation callbacks");
		check(f.w.sessionState().journeyActivity() == XeenJourneyActivity::Attachment &&
			f.flow->state().phase() == XeenEncounterPhase::Engaged && f.flow->state().pending() == 0 &&
			f.flow->state().revision() == revision && *f.p.encounterContext == context,
			"obsolete attachment does not stop coordination or alter published context");
		sameActors(actors,f.w.sessionState().actors());
		check(f.flow->boundary().quiet() == !held, "obsolete attachment preserves newer held lease");
		if (held) {
			check(!f.flow->attachJourney(f.flow->ticket(),[] {}), "fresh attachment waits for newer work completion");
			f.flow->boundary().release(XeenCombatBoundary::Work::Inventory,lease);
		}
		f.onMap = {}; f.onObjects = {};
		check(!f.flow->attachJourney(old,[] {}), "old attachment capability remains unusable after newer lease release");
		check(f.flow->attachJourney(f.flow->ticket(),[] {}), "genuinely fresh attachment succeeds on the same owners");
		check(!f.flow->attachJourney(old,[] {}), "old capability cannot attach again after fresh success");
	}
	Fixture currentFailure; currentFailure.action(XeenEncounterAction::Wait);
	bool rejected = false;
	try { currentFailure.flow->attachJourney(currentFailure.flow->ticket(),[] { throw std::runtime_error("current sprite failure"); }); }
	catch (const std::runtime_error &) { rejected = true; }
	check(rejected && !currentFailure.flow->combat() &&
		currentFailure.w.sessionState().journeyActivity() == XeenJourneyActivity::Failed,
		"current attachment exception still closes its own coordination");
}
void retainedCombatCaches() {
	for (bool objectsChanged : {false,true}) for (bool throws : {false,true}) {
		Fixture f; f.engage(); auto *combat = f.flow->combat();
		const auto actors = f.w.sessionState().actors();
		combat->setProbe([&] {
			if (objectsChanged) const_cast<XeenObjectFile &>(f.w.objectFile(20)).resourcePresent = false;
			else const_cast<XeenMap &>(f.w.map(20)).geometry.cells[0].rawWord ^= 1;
			if (throws) throw std::runtime_error("cache mutation callback");
		});
		const auto result = combat->command(combat->ticket(),Command::Block);
		check(result.status == Status::Failed && result.failure == XeenCombatFailure::Integrity,
			"admitted cache mutation refuses command publication as integrity failure");
		sameActors(actors,f.w.sessionState().actors());
		f.w.discardMapCache(); // Matching bytes cannot revive failed combat authority.
		check(!f.flow->retireJourney(f.flow->ticket()) && !f.flow->journeyQuiet(), "cache integrity failure cannot retire");
	}
	Fixture good; good.engage(); good.w.discardMapCache();
	check(good.command(Command::Block).status == Status::Advanced, "matching active cache reconstruction is legal");
	good.lethal(); good.w.discardMapCache();
	check(good.flow->combat()->service(good.flow->combat()->ticket()).status == Status::Victory, "matching reconstruction during End");
	check(good.flow->retireJourney(good.flow->ticket()), "matching caches preserve retirement authority");
	Fixture retirement; retirement.engage(); retirement.lethal();
	retirement.flow->combat()->service(retirement.flow->combat()->ticket());
	const_cast<XeenMap &>(retirement.w.map(20)).geometry.cells[0].rawWord ^= 1;
	bool retired = false;
	try { retired = retirement.flow->retireJourney(retirement.flow->ticket()); } catch (const std::exception &) {}
	check(!retired && !retirement.flow->journeyQuiet(), "retirement checks retained cache values after successful End");
}
void initializationAliases() {
	auto bytes = chr(); auto party = XeenPartyLoader().loadFromResources(bytes,pty());
	auto camera = XeenActorApproach::kEntry; XeenGameFlags flags;
	auto monsters = statistics(); auto event = events();
	XeenJourneySetup setup{bytes,XeenGameplayContextFormat::parse(pty()),monsters,event,56};
	XeenWorld world([&](XeenMapIdentity) {
		setup.context.minutes = 960; bytes[348] = 99; monsters.clear();
		return map();
	},[](XeenMapIdentity) { return objects(); });
	XeenEventPresenter::Clock clock = [] { return 0; };
	XeenEncounterFlow flow(world,party,camera,flags,clock,setup);
	check(setup.context.minutes == 960 && party.encounterContext->minutes == 480 && party.roster.combatInputs(0)->experience == 0,
		"initialization publishes detached validated context and supplements");
	check(world.sessionState().actors().size() == 27 && world.sessionState().actors()[5].hp == 20,
		"initialization retains detached statistics");
	check(flow.prepareJourneyFrame(flow.ticket(),[] {}) && flow.presentJourney(flow.ticket()) && flow.journeyQuiet(),
		"validated detached initialization reaches presented boundary");
}
void callbackAuthority() {
	Fixture f;
	f.action(XeenEncounterAction::Wait);
	const auto entry = f.flow->ticket();
	bool nested = true;
	check(f.flow->attachJourney(entry,[&] { nested = f.flow->attachJourney(entry,[] {}); }), "outer attachment");
	check(!nested, "reentrant attachment refuses");
	Fixture changed;
	changed.action(XeenEncounterAction::Wait);
	bool rejected = false;
	try { changed.flow->attachJourney(changed.flow->ticket(),[&] { changed.p.roster.at(0).currentSp = -12; }); }
	catch (const std::exception &) { rejected = true; }
	check(rejected && !changed.flow->combat() && !changed.flow->journeyQuiet() && changed.p.roster.at(0).currentSp == -12,
		"attachment detects callback mutation without rollback");
	Fixture provider;
	provider.w.discardMapCache();
	provider.onMap = [&] { provider.flags.set(4); };
	provider.pulse();
	check(!provider.flow->journeyQuiet() && provider.flags.isSet(4), "nested map callback mutation closes Journey");
	Fixture other;
	Fixture original;
	check(!original.flow->current(other.flow->ticket()), "same revision from another graph is not authority");
	Fixture aba;
	aba.action(XeenEncounterAction::Wait);
	bool abaRejected = false;
	try { aba.flow->attachJourney(aba.flow->ticket(),[&] {
		const auto values = aba.flags.values(); aba.flags.~XeenGameFlags(); new (&aba.flags) XeenGameFlags(values);
	}); } catch (const std::exception &) { abaRejected = true; }
	check(abaRejected && !aba.flow->combat() && !aba.flow->journeyQuiet(), "same-address flag owner replacement refuses attachment");
	Fixture activeAba;
	activeAba.engage();
	auto *active = activeAba.flow->combat();
	const auto old = active->ticket();
	active->setProbe([&] {
		const auto value = activeAba.camera;
		activeAba.camera.~XeenCamera(); new (&activeAba.camera) XeenCamera(value);
	});
	check(active->command(old,Command::Block).status == Status::Stale && !active->current(old),
		"same-address active camera replacement cannot publish a command");
}
void endBoundary() {
	auto bytes = chr();
	for (auto id : kXeenCombatOwners) { bytes[id*354+342] = 255; bytes[id*354+343] = 127; }
	Fixture f(bytes);
	f.engage();
	auto *combat = f.flow->combat();
	for (unsigned n = 0; f.p.encounterContext->minutes < 959 && n < 10000; ++n) {
		if (combat->phase() == Phase::PlayerReady) f.command(Command::Block);
		else check(combat->service(combat->ticket()).status != Status::Failed, "bounded time control remains active");
	}
	check(f.p.encounterContext->minutes == 959, "real Round reaches minute959 without normalization");
	for (unsigned n = 0; n < 50 && combat->phase() != Phase::VictoryAwaitingEnd; ++n) {
		if (combat->phase() == Phase::PlayerReady) f.command(Command::Attack);
		else combat->service(combat->ticket());
	}
	check(combat->phase() == Phase::VictoryAwaitingEnd, "genuine lethal publication at minute959");
	const auto owners = f.p.roster.characters();
	check(combat->service(combat->ticket()).status == Status::SupportStopped && f.p.encounterContext->minutes == 959,
		"End959 stops before charge");
	check(!f.flow->retireJourney(f.flow->ticket()) && !f.flow->journeyQuiet() &&
		f.w.sessionState().actors()[5].lifecycle == XeenActorLifecycle::Defeated && f.w.sessionState().accountedMonsters().size() == 1,
		"failed End preserves lethal consequences without retirement");
	for (unsigned i = 0; i < 30; ++i) check(xeen_state::sameCharacter(owners[i],f.p.roster.at(i)), "failed End preserves character facts");
}
void carriedConsequences() {
	Fixture f(chr(),[](Fixture &f) {
		using remove_test::record;
		f.event.records = {record(10,10,0,12,{0,0,21,99}),record(10,10,1,12,{0,0,104,2}),record(10,10,2,12,{0,0,20,7})};
		XeenEventInterpreter interpreter;
		const auto result = interpreter.begin({20,10,10,XeenDirection::North},f.p,f.flags,f.w,
			[&](XeenMapIdentity) { return XeenEventScript(f.event); },{});
		check(std::holds_alternative<XeenEventExecutionCompleted>(result), "controlled prior event completes");
		f.flags = std::get<XeenEventExecutionCompleted>(result).finalGameFlags;
		XeenPendingRewards rewards; rewards.enqueue({10,37,1,0});
		check(xeenDeliverRewards(rewards,f.p,0).delivered == 1, "controlled prior reward delivered");
		f.w.disableObject({20,0}); f.w.disableEventsAtCell({20,10,10,XeenDirection::North},f.event);
		f.p.roster.at(29).intellect.permanent = -12345; // Existing inactive storage is not an active rule input.
	});
	f.engage(); f.lethal();
	auto *combat = f.flow->combat();
	bool busyRetirement = true;
	combat->setProbe([&] { busyRetirement = f.flow->retireJourney(f.flow->ticket()); });
	check(combat->service(combat->ticket()).status == Status::Victory && !busyRetirement, "End cannot retire while service is busy");
	const auto lease = f.flow->boundary().hold(XeenCombatBoundary::Work::Inventory);
	check(!f.flow->retireJourney(f.flow->ticket()), "external boundary work blocks retirement");
	f.flow->boundary().release(XeenCombatBoundary::Work::Inventory,lease);
	check(f.flow->retireJourney(f.flow->ticket()), "retirement after busy boundary unwinds"); f.present();
	check(f.p.questItems.at(17) == 1 && f.p.questFlags.isSet(2) && f.flags.isSet(7) &&
		xeenSameItem(f.p.roster.at(0).miscellaneous[0],{10,37,1,0}) && f.p.roster.at(29).intellect.permanent == -12345,
		"combat preserves independent prior quest, reward, flag and inactive owner state");
	check(f.w.sessionState().disabledObjects().size() == 1 && f.w.sessionState().disabledEvents().size() == 3,
		"combat preserves complete independent overlays");
}
void approachTimeLimit() {
	Fixture f;
	f.action(XeenEncounterAction::Right);
	for (unsigned i = 0; i < 47; ++i)
		check(f.action(i%2 ? XeenEncounterAction::Backward : XeenEncounterAction::Forward).outcome == XeenEncounterOutcome::Accepted,
			"rapid bounded actions retain independent pending work");
	check(f.p.encounterContext->minutes == 950 && f.flow->state().pending() == 3, "literal approach time boundary");
	const auto actors = f.w.sessionState().actors(); const auto camera = f.camera;
	check(f.action(XeenEncounterAction::Backward).outcome == XeenEncounterOutcome::Stopped && !f.flow->journeyQuiet(),
		"charge at950 terminates before dependent work");
	check(f.p.encounterContext->minutes == 950 && xeen_state::sameCamera(camera,f.camera), "time stop preserves prior camera and clock");
	sameActors(actors,f.w.sessionState().actors());
}
void frameFailure() {
	Fixture f;
	f.flow->journeyAction(f.flow->ticket(),XeenEncounterAction::Right);
	const auto entry = f.flow->ticket();
	check(!f.flow->prepareJourneyFrame(entry,[] { throw std::runtime_error("composition failure"); }) &&
		!f.flow->presentJourney(entry) && !f.flow->journeyQuiet(), "failed composition retains presentation guard");
	check(f.flow->prepareJourneyFrame(entry,[] {}) && f.flow->presentJourney(entry), "one guarded rebuild and matching frame handoff");
	f.flow->journeyAction(f.flow->ticket(),XeenEncounterAction::Left);
	const auto failed = f.flow->ticket();
	for (unsigned i = 0; i < 2; ++i) check(!f.flow->prepareJourneyFrame(failed,[] { throw std::runtime_error("composition failure"); }), "composition failure");
	check(!f.flow->current(failed) && !f.flow->journeyQuiet(), "failed recovery is terminal");
}
}
int main() {
	try { lifecycle(); approachAndGuards(); currentValues(); readinessAndHistory(); progression(); callbackAuthority(); postEndInvalidation(); approachBoundaryReplacement(); attachmentBoundaryReplacement(); retainedCombatCaches(); initializationAliases(); endBoundary(); carriedConsequences(); approachTimeLimit(); frameFailure(); std::cout << "Journey domain tests passed\n"; return 0; }
	catch (const std::exception &e) { std::cerr << e.what() << '\n'; return 1; }
}
