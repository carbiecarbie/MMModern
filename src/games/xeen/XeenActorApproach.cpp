#include "games/xeen/XeenActorApproach.h"
#include "games/xeen/XeenJourneyRules.h"
#include "games/xeen/XeenRegionalRules.h"
#include "games/xeen/XeenEventTrigger.h"
#include "games/xeen/XeenCharacterRules.h"
#include "formats/xeen/XeenCharacterFormat.h"
#include "formats/xeen/XeenAssetSource.h"
#include "formats/xeen/XeenGameplayContextFormat.h"
#include "games/xeen/XeenEventLoader.h"
#include "games/xeen/XeenMovement.h"
#include "games/xeen/XeenOutdoorSceneTables.h"
#include "games/xeen/XeenIndoorScene.h"
#include "games/xeen/XeenMerchantGeneration.h"
#include <algorithm>
#include <limits>
#include <stdexcept>
#include <type_traits>

// Adapted from ScummVM developers' GPL-3.0-or-later Xeen combat.cpp,
// interface_scene.cpp and interface.cpp at 6814ee9ba54582f5b5adcffab49efbbd8f589edd.
namespace mmodern {
namespace {
bool coordinate(int x, int y) { return x >= 0 && x < 32 && y >= 0 && y < 32; }
void require(bool yes, const char *why) { if (!yes) throw std::invalid_argument(why); }
void bounded(const std::vector<XeenActor> &actors, const XeenCamera &camera) {
	require(actors.size() <= XeenActorApproach::kCapacity, "monster simulation capacity exceeded");
	require(static_cast<unsigned>(camera.direction) < 4 && coordinate(camera.x, camera.y),
		"invalid actor camera");
	for (std::size_t i = 0; i < actors.size(); ++i)
		require(actors[i].id.mapId == camera.mapId && actors[i].id.recordIndex == i,
			"actor identity/order differs from original record order");
}
struct Query { int sample; std::array<int, 3> slots; XeenActorPlacement placement; };
constexpr Query queries[] = {
	{2,{0,1,2},XeenActorPlacement::SameCell}, {7,{3,4,5},XeenActorPlacement::Forward},
	{5,{12,-1,-1},XeenActorPlacement::ForwardLeft}, {9,{13,-1,-1},XeenActorPlacement::ForwardRight},
	{14,{6,7,8},XeenActorPlacement::Other}, {12,{14,20,-1},XeenActorPlacement::Other},
	{16,{15,21,-1},XeenActorPlacement::Other}, {27,{9,10,11},XeenActorPlacement::Other},
	{25,{16,22,24},XeenActorPlacement::Other}, {23,{18,-1,-1},XeenActorPlacement::Other},
	{29,{17,23,25},XeenActorPlacement::Other}, {31,{19,-1,-1},XeenActorPlacement::Other}
};
std::pair<int,int> offset(XeenDirection d, int sample) {
	const int x = xeen_scene_tables::kSouthX[sample], y = xeen_scene_tables::kSouthY[sample];
	switch (d) {
	case XeenDirection::North: return {-x,-y};
	case XeenDirection::East: return {-y,x};
	case XeenDirection::South: return {x,y};
	case XeenDirection::West: return {y,-x};
	}
	throw std::invalid_argument("invalid actor facing");
}
void activate(std::vector<XeenActor> &actors, const XeenActorView &view) noexcept {
	for (std::size_t i = 0; i < actors.size(); ++i)
		actors[i].activated = actors[i].activated || view.activation[i];
}
}

std::vector<XeenActor> XeenActorApproach::actorsFromResources(const XeenObjectFile &mob,
		const std::vector<XeenMonsterRecord> &statistics) {
	require(mob.resourcePresent && bool(mob.mapId), "missing original monster MOB");
	require(mob.entities.monsters.size() <= kCapacity, "monster simulation capacity exceeded");
	require(!statistics.empty() && statistics.size() <= XeenMonsterFormat::kMaximumBytes / 60,
		"invalid monster statistics count");
	std::vector<XeenActor> actors;
	actors.reserve(mob.entities.monsters.size());
	for (std::size_t i = 0; i < mob.entities.monsters.size(); ++i) {
		const auto &original = mob.entities.monsters[i];
		XeenActor a;
		a.id = {mob.mapId, i}; a.original = original; a.x = original.x; a.y = original.y;
		if (original.hasResource()) {
			require(static_cast<std::size_t>(original.resourceId) < statistics.size(), "missing monster type statistics");
			a.statistics = statistics[original.resourceId];
			a.hp = a.statistics->baseHp();
			a.lifecycle = XeenActorLifecycle::Present;
		}
		if (original.isDisabled()) a.lifecycle = XeenActorLifecycle::Disabled;
		actors.push_back(a);
	}
	return actors;
}

XeenActorView XeenActorApproach::classify(const std::vector<XeenActor> &actors,
		const XeenCamera &camera) {
	bounded(actors, camera);
	XeenActorView view;
	for (std::size_t i = 0; i < actors.size(); ++i) {
		const auto &actor = actors[i];
		for (const auto &q : queries) {
			const auto delta = offset(camera.direction, q.sample);
			if (actor.x != camera.x + delta.first || actor.y != camera.y + delta.second) continue;
			view.activation[i] = true; // Independent of full selection groups and pixel visibility.
			view.placements[i] = q.placement;
			for (int slot : q.slots) {
				if (slot >= 0 && !view.slots[slot]) { view.slots[slot] = actor.id; break; }
			}
		}
	}
	return view;
}

std::array<unsigned, 1024> XeenActorApproach::occupancy(const std::vector<XeenActor> &actors) {
	require(actors.size() <= kCapacity, "monster simulation capacity exceeded");
	std::array<unsigned, 1024> counts{};
	for (const auto &a : actors)
		if (a.statistics && a.lifecycle==XeenActorLifecycle::Present && coordinate(a.x,a.y))
			++counts[a.y * 32 + a.x];
	return counts;
}

std::vector<XeenActor> XeenActorApproach::move(const std::vector<XeenActor> &actors,
		const XeenCamera &camera, const Terrain &terrain, bool movementEnabled) {
	return move(actors,camera,terrain,movementEnabled,{});
}

std::vector<XeenActor> XeenActorApproach::move(const std::vector<XeenActor> &actors,
		const XeenCamera &camera, const Terrain &terrain, bool movementEnabled,
		const BeforeMovement &beforeMovement, XeenActorOpportunityContext context) {
	if(!context.sleeping && context.movementEnabled && !context.charactersShooting)
		return move(actors,camera,terrain,movementEnabled,beforeMovement);
	return moveWithContext(actors,camera,terrain,movementEnabled,beforeMovement,context);
}
std::vector<XeenActor> XeenActorApproach::move(const std::vector<XeenActor> &actors,
		const XeenCamera &camera,const Terrain &terrain,bool movementEnabled,const BeforeMovement &beforeMovement) {
	return moveWithContext(actors,camera,terrain,movementEnabled,beforeMovement,{});
}
std::vector<XeenActor> XeenActorApproach::moveWithContext(const std::vector<XeenActor> &actors,
		const XeenCamera &camera,const Terrain &terrain,bool movementEnabled,const BeforeMovement &beforeMovement,XeenActorOpportunityContext context) {
	bounded(actors, camera);
	auto result = actors;
	if (!movementEnabled || !context.movementEnabled || context.charactersShooting) return result;
	require(bool(terrain), "missing monster terrain predicate");
	auto counts = occupancy(actors);
	std::array<bool, kCapacity> moved{};
	for (int pass = 0; pass < 2; ++pass) {
		for (int dy = 3; dy >= -3; --dy) for (int dx = -3; dx <= 3; ++dx) {
			for (std::size_t i = 0; i < result.size(); ++i) {
				auto &a = result[i];
				if(context.sleeping && a.lifecycle!=XeenActorLifecycle::Present)continue;
				if (a.x != camera.x + dx || a.y != camera.y + dy ||
					(!a.activated && !context.sleeping) || moved[i]) continue;
				if (beforeMovement) beforeMovement(result,i);
				require(coordinate(a.x, a.y) && a.lifecycle == XeenActorLifecycle::Present &&
					a.status == XeenActorStatus::Physical && a.statistics &&
					(beforeMovement ? a.statistics->supportsGroundMovement() : a.statistics->supportsMovement()) &&
					a.original.resourceId != 59, "unsupported relevant actor movement");
				const int sx = dx < 0 ? 1 : dx > 0 ? -1 : 0;
				const int sy = dy < 0 ? 1 : dy > 0 ? -1 : 0;
				// Literal MONSTER_GRID_X/Y and GRID3, including same-column/row zero deltas.
				std::pair<int,int> primary{sx, dx == 0 ? sy : 0};
				std::pair<int,int> fallback{dy == 0 ? sx : 0, sy};
				if (camera.direction == XeenDirection::East || camera.direction == XeenDirection::West)
					std::swap(primary, fallback);
				auto predicate = [&](std::pair<int,int> d) {
					const auto t = terrain(a, a.x + d.first, a.y + d.second);
					require(t != XeenMonsterTerrain::Unsupported, "unsupported monster terrain");
					return t == XeenMonsterTerrain::Allowed;
				};
				std::pair<int,int> delta = primary;
				if (!predicate(primary)) { delta = fallback; if (!predicate(fallback)) continue; }
				const int x = a.x + delta.first, y = a.y + delta.second;
				// Occupancy refusal after allowed terrain never tries the terrain fallback.
				if (!coordinate(x,y) || counts[y * 32 + x] >= 3) continue;
				++counts[y * 32 + x]; --counts[a.y * 32 + a.x];
				a.x = x; a.y = y; moved[i] = true;
			}
		}
	}
	return result;
}

void XeenActorApproach::validateDomain(XeenWorld &world, const XeenPartyState &party,
		const XeenGameplayContext &context, const std::vector<XeenActor> &actors,
		const XeenEventFile &events) {
	{
		require(xeenRegionalContext(context),"Unsupported regional context");
		xeenValidateJourneyParty(party);
		if(events.mapId==XeenMapIdentity(28)) {
			validateEnvironment(world,actors,events);
			std::set<XeenMonsterIdentity> mainland;
			for(const auto id:world.sessionState().accountedMonsters())if(id.mapId==XeenMapIdentity(23))mainland.insert(id);
			xeenValidateRegionalActors(world.map(23),world.objectFile(23),world.sessionState().actors(),mainland,world._cityStatistics);
			return;
		}
		validateEnvironment(world,actors,events);
		return;
	}
}

void XeenActorApproach::validateEnvironment(XeenWorld &world,
		const std::vector<XeenActor> &actors, const XeenEventFile &events, const std::vector<XeenMonsterRecord> *statistics) {
	if(events.mapId==XeenMapIdentity(28)) {
		require(events.resourcePresent,"Vertigo Event resource is absent");
		xeenValidateVertigoActors(world,actors);
		return;
	}
	{
		bounded(actors,xeenJourneyContent().entry);
		require(events.mapId==XeenMapIdentity(23) && events.resourcePresent && !events.records.empty(),"Regional event topology changed");
		const auto &map=world.map(23);
		require(map.geometry.flags==0 && map.geometry.isOutdoors(),"Regional geometry flags changed");
		(void)XeenMovement::component(map,9,11,xeenJourneyContent().traversal);
		std::set<XeenMonsterIdentity> mainland;
		for(const auto id:world.sessionState().accountedMonsters())if(id.mapId==XeenMapIdentity(23))mainland.insert(id);
		xeenValidateRegionalActors(map,world.objectFile(23),actors,mainland,statistics ? *statistics : world._cityStatistics);
		return;
	}
}

XeenEncounterResult XeenActorApproach::initializeJourney(XeenWorld &w,XeenPartyState &p,XeenCamera &c,
 XeenEncounterState &s,const std::vector<std::uint8_t> &chr,const XeenGameplayContext &ctx,
 const std::vector<XeenMonsterRecord> &m,const XeenEventFile &e,std::uint32_t seed) {
 return initializeJourney(w,p,c,s,chr,ctx,m,e,seed,{});
}
XeenEncounterResult XeenActorApproach::initializeJourney(XeenWorld &world, XeenPartyState &party,
		XeenCamera &camera, XeenEncounterState &state, const std::vector<std::uint8_t> &chr,
		const XeenGameplayContext &context, const std::vector<XeenMonsterRecord> &statistics,
		const XeenEventFile &events, std::uint32_t seed,
		const std::optional<XeenMonsterTreasure> &purse) {
	return initializeJourney(world,party,camera,state,chr,context,statistics,events,seed,purse,{});
}
XeenEncounterResult XeenActorApproach::initializeJourney(XeenWorld &world, XeenPartyState &party,
		XeenCamera &camera, XeenEncounterState &state, const std::vector<std::uint8_t> &chr,
		const XeenGameplayContext &context, const std::vector<XeenMonsterRecord> &statistics,
		const XeenEventFile &events, std::uint32_t seed,
		const std::optional<XeenMonsterTreasure> &purse, const std::optional<XeenBankBalances> &bankInput) {
	return initializeJourney(world,party,camera,state,chr,context,statistics,events,seed,purse,bankInput,{});
}
XeenEncounterResult XeenActorApproach::initializeJourney(XeenWorld &world, XeenPartyState &party,
		XeenCamera &camera, XeenEncounterState &state, const std::vector<std::uint8_t> &chr,
		const XeenGameplayContext &context, const std::vector<XeenMonsterRecord> &statistics,
		const XeenEventFile &events, std::uint32_t seed,
		const std::optional<XeenMonsterTreasure> &purse, const std::optional<XeenBankBalances> &bankInput,
		const FreshPublicationPreparation &beforePublication) {
	return initializeStart(world,party,camera,state,chr,context,statistics,events,events,seed,purse,bankInput,true,party.regionalRecovery,
		[&](const auto &candidate,const auto &actors,const auto &random,const auto &,const auto &) {
			if(beforePublication)beforePublication(candidate,actors,random);
		});
}
XeenEncounterResult XeenActorApproach::initializeStart(XeenWorld &world, XeenPartyState &party,
		XeenCamera &camera, XeenEncounterState &state, const std::vector<std::uint8_t> &chr,
		const XeenGameplayContext &context, const std::vector<XeenMonsterRecord> &statistics,
		const XeenEventFile &events, const XeenEventFile &mainlandEvents, std::uint32_t seed,
		const std::optional<XeenMonsterTreasure> &purse, const std::optional<XeenBankBalances> &bankInput,
		bool prepared, const std::optional<XeenRegionalRecoveryState> &recovery, const StartPublicationPreparation &beforePublication) {
	const auto bank=bankInput; // Detach before all further compatibility callbacks.
	const auto preparePublication=beforePublication;
	const auto &policy=xeenJourneyContent();
	const auto &reservation = world._sessionState;
	require(reservation._entry == XeenEncounterEntry::Ordinary && reservation._encounterMarked &&
		!reservation._encounterInitialized && !reservation._encounterTerminal && reservation._actors.empty() &&
		reservation._journeyOwner && reservation._journeyActivity == XeenJourneyActivity::Attachment &&
		!party.roster.combatMarked() && !party.encounterContext &&
		!state._world && seed && world._combatCheck, "Journey requires guarded fresh owners");
	require(!party.serviceEconomy && bool(bank), "Fresh service economy input mismatch");
	if (bank) require(!bank->gold && !bank->gems,"Unsupported original bank balances");
	const XeenCamera entry=prepared ? policy.entry : XeenCamera{28,18,4,XeenDirection::West};
	require(camera.mapId == entry.mapId && camera.x == entry.x && camera.y == entry.y && camera.direction == entry.direction &&
		context.minutes == 480 && context.ctr24 == 0, "Journey requires fresh entry context");
	require(xeenRegionalContext(context) && (!prepared || context.difficulty==XeenDifficulty::Adventurer),"Invalid start difficulty");
	require(events.resourcePresent && events.mapId==camera.mapId,"Start Event binding mismatch");
	if(!prepared)require(reservation._objects.empty() && reservation._events.empty() && reservation._barriers.empty() &&
		reservation._accountedMonsters.empty() && !reservation._vertigoActors && world._cityStatistics.empty(),"Original start requires clean resource state");
	XeenPartyState candidate(party);
	if(!prepared) {require(bool(recovery) && !party.regionalRecovery,"Original recovery preparation mismatch");candidate.regionalRecovery=recovery;}
	require(bool(purse),"Fresh consequence purse presence mismatch");
	candidate.monsterTreasure=purse;
	{
		require(chr.size()==30*354 && context.year==610 && context.day==1 && !context.rested && !context.newDay &&
			context.effects==std::array<std::uint8_t,9>{} && context.lightAndResistances==std::array<std::uint16_t,6>{},"Regional original CHR/PTY prerequisites changed");
		unsigned swimming=0,mountaineer=0,navigator=0,pathfinder=0;
		for (auto id:kXeenCombatOwners) {
			swimming+=chr[id*354+39+14]!=0;mountaineer+=chr[id*354+39+9]!=0;
			navigator+=chr[id*354+39+10]!=0;pathfinder+=chr[id*354+39+11]!=0;
		}
		require(swimming<6 && mountaineer<2 && navigator==0 && pathfinder<2,"Regional effective traversal prerequisites changed");
	}
	for (unsigned id = 0; id < 30; ++id) candidate.roster._combatInputs[id] = XeenCharacterFormat::parseCombatInputs(chr, id, true, true, true);
	for (unsigned id = 0; id < 30; ++id)
		candidate.roster.at(id).learnedSpells = XeenCharacterFormat::parseLearnedSpells(chr, id);
	candidate.roster._combatMarked = true;
	candidate.encounterContext = context;
	if(prepared) {
		candidate.encounterContext->day=8;
		constexpr int levels[]{3,3,3,4,3,3},xp[]{1000,2000,1000,1000,2000,1000};
		for (unsigned i=0;i<6;++i) {
			const auto id=kXeenCombatOwners[i]; auto &c=candidate.roster.at(id); auto &input=*candidate.roster._combatInputs[id];
			c.permanentLevel=levels[i]; c.temporaryLevel=c.temporaryAge=0; c.conditions.fill(0);
			c.intellect.temporary=c.personality.temporary=c.endurance.temporary=0;
			input.might.temporary=input.speed.temporary=input.accuracy.temporary=input.luck->temporary=input.temporaryAc=0;
			input.experience=xp[i];
			c.currentHp=XeenCharacterRules::maxHp(c,{610}); c.currentSp=XeenCharacterRules::maxSp(c,{610});
		}
	}
	XeenCombatRandom preparedRandom(seed);
	{
		XeenMerchantStockCandidate stock;
		// Fresh detached initialization precedes any playable owner graph. Each
		// invocation keeps the same bounded draw servicing as idle preparation.
		while (!stock.complete()) {
			XeenConsequenceDraw draw{preparedRandom,64,world._combatCheck};
			stock.service(draw);
		}
		xeenValidateMerchantWares(stock.wares());
		candidate.serviceEconomy=XeenServiceEconomy{stock.wares(),*bank};
	}
	xeenValidateJourneyParty(candidate);
	const auto detachedStatistics = statistics;
	const auto detachedEvents = events;
	auto actors = actorsFromResources(world.objectFile(policy.entry.mapId), detachedStatistics);
	require(!actors.empty() && actors.size()==world.objectFile(policy.entry.mapId).entities.monsters.size(), "Journey requires complete original actor collection");
	for (const auto &a:actors) {
		require(bool(a.statistics),"Missing influencing statistics");

	}
	validateEnvironment(world, actors, mainlandEvents,&detachedStatistics);
	std::optional<std::vector<XeenActor>> cityActors;
	std::vector<XeenMonsterRecord> cityStatistics=detachedStatistics;
	XeenEncounterResult result;
	result.outcome = XeenEncounterOutcome::Started; result.revision = 1;
	if(prepared) {result.view = classify(actors, camera); activate(actors, result.view);}
	else {
		// Detached resource validation uses the existing city's World owner.
		// No candidate gameplay values are installed in the destination yet.
		XeenWorld staged(world._loader,world._objectLoader);staged._detachedEventCandidate=true;
		staged.stageVertigoActors(world.objectFile(28),detachedStatistics);
		cityActors=staged._sessionState._vertigoActors;cityStatistics=staged._cityStatistics;
		require(cityActors->size()==46,"Original start requires all Vertigo records");
		validateEnvironment(staged,*cityActors,detachedEvents);
		result.view=XeenIndoorScene().classifyActors(world,camera,*cityActors);
		activate(*cityActors,result.view);
		require(!result.view.engaged(),"Original start has immediate contact");
	}
	std::optional<XeenJourneyRandomState> finalRandom;
	finalRandom=preparedRandom.continuation();
	world._combatCheck();
	try { if (preparePublication) preparePublication(candidate,actors,finalRandom,cityActors,cityStatistics); }
	catch (...) {world._combatCheck();throw;}
	world._combatCheck();
	static_assert(std::is_nothrow_copy_assignable_v<decltype(party.serviceEconomy)> &&
		std::is_nothrow_copy_assignable_v<decltype(party.encounterContext)> &&
		std::is_nothrow_copy_assignable_v<decltype(party.roster._combatInputs)> &&
		std::is_nothrow_copy_assignable_v<decltype(party.monsterTreasure)> &&
		std::is_nothrow_copy_assignable_v<decltype(world._sessionState._journeyRandom)>);
	auto &s = world._sessionState;
	if(prepared)for (auto id:kXeenCombatOwners) {
		auto &to=party.roster.at(id); const auto &from=candidate.roster.at(id);
		to.permanentLevel=from.permanentLevel; to.temporaryLevel=to.temporaryAge=0; to.conditions=from.conditions;
		to.intellect.temporary=to.personality.temporary=to.endurance.temporary=0; to.currentHp=from.currentHp; to.currentSp=from.currentSp;
	}
	party.roster._combatInputs = candidate.roster._combatInputs;
	for (unsigned id = 0; id < 30; ++id)
		party.roster.at(id).learnedSpells = candidate.roster.at(id).learnedSpells;
	party.roster._combatMarked = true; party.encounterContext = candidate.encounterContext;
	party.monsterTreasure=candidate.monsterTreasure;
	party.serviceEconomy=candidate.serviceEconomy;
	if(!prepared)party.regionalRecovery=candidate.regionalRecovery;
	world._cityStatistics.swap(cityStatistics);
	s._actors.swap(actors); s._entry = XeenEncounterEntry::Journey;
	if(cityActors) {
		s._vertigoActors.swap(cityActors);
		world._cityOriginalActorCount=46;
	}
	s._encounterMarked = s._encounterInitialized = true; s._encounterRevision = 1;

	s._skeletonSeed = 0;
	s._journeyRandom=finalRandom;
	state._world = &world; state._party = &party; state._camera = &camera; state._revision = 1;
	return result;
}

bool XeenActorApproach::authoritative(const XeenWorld &world, const XeenPartyState &party,
		const XeenCamera &camera, const XeenEncounterState &state) noexcept {
	const auto &s = world._sessionState;
	return s._encounterMarked && s._encounterInitialized && state._world == &world &&
		state._party == &party && state._camera == &camera && state._revision == s._encounterRevision &&
		s._encounterTerminal == (state._phase != XeenEncounterPhase::Exploring);
}

XeenEncounterResult XeenActorApproach::stop(XeenWorld &world, XeenEncounterState &state,
		XeenEncounterStop reason) noexcept {
	auto &s = world._sessionState;
	XeenEncounterResult r;
	r.revision = s._encounterRevision;
	if (s.journey() && (!world._combatCheck || !world._combatAuthorized || !world._combatAuthorized() ||
		s._combatApproachState != &state)) { r.outcome = XeenEncounterOutcome::Refused; return r; }
	if (state._world != &world || state._revision != s._encounterRevision) {
		r.outcome = XeenEncounterOutcome::Stale; return r;
	}
	if (s._encounterTerminal) { r.outcome = XeenEncounterOutcome::Terminal; return r; }
	state._pending = 0; state._phase = XeenEncounterPhase::SupportStopped; state._reason = reason;
	s._encounterTerminal = true;
	if (s._encounterRevision != std::numeric_limits<std::uint64_t>::max()) ++s._encounterRevision;
	state._revision = s._encounterRevision;
	r.revision = state._revision; r.reason = reason; r.outcome = XeenEncounterOutcome::Stopped;
	return r;
}

XeenEncounterResult XeenActorApproach::action(XeenWorld &w, XeenPartyState &p, XeenCamera &c,
		XeenEncounterState &s, XeenEncounterAction a, const XeenEventFile &e) {
	return transition(w,p,c,s,a,e,false);
}
XeenEncounterResult XeenActorApproach::pulse(XeenWorld &w, XeenPartyState &p, XeenCamera &c,
		XeenEncounterState &s, const XeenEventFile &e) {
	return transition(w,p,c,s,XeenEncounterAction::Unsupported,e,true);
}

XeenEncounterResult XeenActorApproach::transition(XeenWorld &world, XeenPartyState &party,
		XeenCamera &camera, XeenEncounterState &state, XeenEncounterAction action,
		const XeenEventFile &events, bool pulse) {
	auto &session = world._sessionState;
	XeenEncounterResult result;
	result.revision = session._encounterRevision;
	if (session.journey() && (!world._combatCheck || !world._combatAuthorized || !world._combatAuthorized() ||
		session._combatApproachState != &state || session._journeyActivity != XeenJourneyActivity::Approach)) return result;
	if (state._world != &world || state._party != &party || state._camera != &camera ||
		state._revision != session._encounterRevision) { result.outcome = XeenEncounterOutcome::Stale; return result; }
	if (session._encounterTerminal) { result.outcome = XeenEncounterOutcome::Terminal; return result; }
	if (!pulse && (action == XeenEncounterAction::Unsupported || static_cast<unsigned>(action) > 5)) return result;
	if (state._revision == std::numeric_limits<std::uint64_t>::max()) return stop(world,state,XeenEncounterStop::Overflow);
	const auto entry = state; // Keep authorization facts from before any provider callback.
	const auto combatAuthorized = world._combatAuthorized;
	const bool guardedCoordination = session.journey();
	const auto entryCurrent = [&] {
		return (!guardedCoordination || (combatAuthorized && combatAuthorized())) &&
			!session._encounterTerminal && session._encounterRevision == entry._revision &&
			state._revision == entry._revision && state._world == entry._world &&
			state._party == entry._party && state._camera == entry._camera &&
			state._pending == entry._pending && state._phase == entry._phase && state._reason == entry._reason;
	};
	const auto refusal = [&] {
		XeenEncounterResult refused;
		if (guardedCoordination && (!combatAuthorized || !combatAuthorized())) {
			refused.outcome = XeenEncounterOutcome::Stale; return refused;
		}
		refused.outcome = session._encounterTerminal ? XeenEncounterOutcome::Terminal : XeenEncounterOutcome::Stale;
		refused.revision = session._encounterRevision;
		refused.reason = state._reason;
		return refused;
	};
	const auto stopForEntry = [&](XeenEncounterStop reason) {
		// Failure does not authorize this operation to stop a newer operation's work.
		return entryCurrent() ? stop(world,state,reason) : refusal();
	};
	try {
		require(session._encounterInitialized && party.encounterContext && state._pending <= 3,
			"incomplete encounter owners");
		validateDomain(world,party,*party.encounterContext,session._actors,events);
		return stopForEntry(XeenEncounterStop::Domain);
	} catch (const std::invalid_argument &) {
		return stopForEntry(XeenEncounterStop::Domain);
	} catch (...) {
		return stopForEntry(XeenEncounterStop::Preparation);
	}
}

XeenEncounterResult XeenActorApproach::regionalTransition(XeenWorld &world,XeenPartyState &party,
		XeenCamera &camera,XeenEncounterState &state,const XeenEventFile &events,
		std::optional<XeenEncounterAction> action,std::unique_ptr<XeenRegionalActionCandidate> &work) {
	auto &session=world._sessionState;XeenEncounterResult refused;
	if(action && static_cast<unsigned>(*action)>static_cast<unsigned>(XeenEncounterAction::StrafeRight))return refused;
	if(!world._combatCheck || !world._combatAuthorized || !world._combatAuthorized() ||
		session._combatApproachState!=&state || session._journeyActivity!=XeenJourneyActivity::Approach ||
		!authoritative(world,party,camera,state) || session._encounterTerminal || (work && action)) return refused;
	const auto entry=state;
	const auto authorized=world._combatAuthorized;
	const auto check=[&] {
		require(authorized() && state._revision==entry._revision && state._pending==entry._pending &&
			state._phase==entry._phase && session._encounterRevision==entry._revision,"Stale regional publication");
		world._combatCheck();
	};
	try {
		const bool indoor=camera.mapId==XeenMapIdentity(28);
		if(indoor && !session._vertigoActors)throw std::invalid_argument("Vertigo actors are absent");
		auto &regionalActors=indoor ? *session._vertigoActors : session._actors;
		check();
        if(!work)validateDomain(world,party,*party.encounterContext,regionalActors,events);
        else validateEnvironment(world,regionalActors,events);
		require(state._revision<std::numeric_limits<std::uint64_t>::max(),"Regional revision exhausted");
		const auto map=world.map(camera.mapId);check();
		if(!work) {
			auto candidate=std::make_unique<XeenRegionalActionCandidate>();auto &c=*candidate;
			c.camera=camera;c.context=*party.encounterContext;c.actors=regionalActors;
			c.random=XeenCombatRandom(*session._journeyRandom);c.revision=state._revision;c.pending=state._pending;
			for(unsigned i=0;i<6;++i) { c.characters[i]=party.roster.at(kXeenCombatOwners[i]);c.inputs[i]=*party.roster.combatInputs(kXeenCombatOwners[i]); }
			bool charge=false,stepTime=false;
			c.result.outcome=action?XeenEncounterOutcome::Accepted:XeenEncounterOutcome::Pulsed;
			if(action) {
				if(*action==XeenEncounterAction::Unsupported) return refused;
				if(*action==XeenEncounterAction::Wait) charge=stepTime=true;
				else {
					XeenMovementResult result;
					if(*action==XeenEncounterAction::Forward || *action==XeenEncounterAction::Backward ||
						*action==XeenEncounterAction::StrafeLeft || *action==XeenEncounterAction::StrafeRight) {
						const auto navigation=*action==XeenEncounterAction::Forward?NavigationAction::MoveForward:
							*action==XeenEncounterAction::Backward?NavigationAction::MoveBackward:
							*action==XeenEncounterAction::StrafeLeft?NavigationAction::StrafeLeft:NavigationAction::StrafeRight;
						const unsigned d=(unsigned(camera.direction)+(*action==XeenEncounterAction::Backward?2u:
							*action==XeenEncounterAction::StrafeLeft?3u:*action==XeenEncounterAction::StrafeRight?1u:0u))&3u;
						constexpr int dx[]{0,1,0,-1},dy[]{1,0,-1,0};const int x=camera.x+dx[d],y=camera.y+dy[d];
						if(indoor) {
							result=XeenMovement().apply(world,c.camera,navigation);
						} else {
							result=XeenMovement::localOutdoor(map,camera.x,camera.y,x,y,xeenJourneyContent().traversal);
							if(result==XeenMovementResult::Moved) { c.camera.x=x;c.camera.y=y; }
						}
					} else result=XeenMovement().apply(world,c.camera,*action==XeenEncounterAction::Left?NavigationAction::TurnLeft:NavigationAction::TurnRight);
					charge=result==XeenMovementResult::Moved;stepTime=charge || result==XeenMovementResult::Turned;
					if(!stepTime) c.result.outcome=XeenEncounterOutcome::Blocked;
					if(indoor ? !xeenIndoorCoordinate(c.camera.x,c.camera.y) :
						(c.camera.mapId!=XeenMapIdentity(23) || c.camera.x<0 || c.camera.x>=16 || c.camera.y<0 || c.camera.y>=16 ||
						 !XeenMovement::component(map,9,11,xeenJourneyContent().traversal)[c.camera.y*16+c.camera.x]))
						{ refused.reason=XeenEncounterStop::Envelope;return refused; }
					const auto sampled=indoor ? world.sampleCell(28,c.camera.x,c.camera.y) : std::optional<XeenCellSample>{};
					if(indoor ? (sampled && (sampled->cell->rawAttributes & kXeenAutomaticEventFlag)!=0) :
						hasAutomaticTrigger(map.geometry,c.camera.x,c.camera.y)) {
						const auto event=xeenRegionalEvent(events,c.camera);
						if(!indoor && event && !xeenRegionalSign(events,c.camera))
							{ refused.reason=XeenEncounterStop::Domain;return refused; }
						c.result.automaticEvent=event.has_value();
					}
				}
			}
			if(charge) {
				static_cast<void>(xeenPrepareTime(c.context,indoor?1:10));
				c.time.emplace(c.context,indoor?1:10,c.characters,c.inputs,&*party.serviceEconomy);
				if(c.pending) ++c.remaining;
				c.pending=3;
				if(*action==XeenEncounterAction::Wait) { ++c.remaining;c.pending=0; }
			}
			if(stepTime) c.context.ctr24=(c.context.ctr24+1)%24;
			if(!action && c.pending && --c.pending==0) ++c.remaining;
			c.classify=!action || *action==XeenEncounterAction::Wait;
			check();work.swap(candidate);
		}
		auto &c=*work;require(c.revision==entry._revision,"Stale regional continuation");
		XeenConsequenceDraw draw{c.random,64,check};
		const auto pending=[&] { auto r=c.result;r.outcome=XeenEncounterOutcome::Pending;r.revision=entry._revision;c.result.needsRest=false;return r; };
		if(c.time && !c.timeDone) {
			if(!c.time->service(draw)) return pending();
			c.inputs=c.time->inputs;c.characters.swap(c.time->characters);const auto ctr=c.context.ctr24;c.context=c.time->context;c.context.ctr24=ctr;c.timeDone=true;
			bool living=false;for(const auto &owner:c.characters) living=living || xeenCombatTargetable(owner);
			if(!living && !c.sleeping) c.remaining=0;
		}
		while(c.remaining) {
			if(!c.opportunity) {
				if(indoor)c.opportunity.emplace(world,c.actors,c.camera,c.characters,c.inputs,c.context.year,0x3f,std::array<bool,6>{},XeenActorOpportunityContext{c.sleeping});
				else c.opportunity.emplace(map,c.actors,c.camera,c.characters,c.inputs,c.context.year,0x3f,std::array<bool,6>{},XeenActorOpportunityContext{c.sleeping});
				c.opportunity->staged=true;
			}
			if(!c.opportunity->service(draw)) {
                auto &op=*c.opportunity;
                c.result.consequences.reset();
                if((!op.travelPresented && !op.travelPublished) || (op.impactOwner && !op.portraitPublished))c.result.consequences=op.presentation();
                if((!op.travelPresented && !op.travelPublished) || op.impactApplied) {
                    // Movement precedes travel; each injury follows its acquired portrait.
                    auto visibleActors=op.actors;check();
                    regionalActors.swap(visibleActors);camera=c.camera;party.encounterContext=c.context;
                    if(c.time && c.timeDone)party.serviceEconomy=c.time->economy;
                    if(c.time && c.timeDone)for(unsigned n=0;n<6;++n)party.roster._combatInputs[kXeenCombatOwners[n]]=c.inputs[n];
                    const auto &values=op.impactApplied ? op.characters : c.characters;
                    for(const auto &v:values) {auto &live=party.roster.at(v.rosterId);live.currentHp=v.currentHp;live.conditions=v.conditions;live.armor=v.armor;
                     if(c.time && c.timeDone && c.time->resetTemps)xeenResetCharacterTemps(live,*party.roster._combatInputs[v.rosterId]);}
                    session._journeyRandom=c.random.continuation();
                    c.result.needsRest=c.time && c.timeDone && !c.timeNoticePublished && c.time->needsRest;
                    c.timeNoticePublished=true;
                    state._revision=session._encounterRevision=++c.revision;
                    if(!op.travelPresented)op.travelPublished=true;
                }
                if(op.impactOwner && c.result.consequences)op.portraitPublished=true;
                auto result=pending();result.revision=state._revision;return result;
            }
			for(unsigned i=0;i<c.opportunity->shotCount;++i) c.shots.at(c.shotCount++)=c.opportunity->shots[i];
			c.noTargets=c.noTargets || c.opportunity->noTargets;
			c.actors.swap(c.opportunity->actors);c.characters.swap(c.opportunity->characters);c.opportunity.reset();
			--c.remaining;++c.result.movementOpportunities;
		}
		if(c.classify) { c.result.view=indoor ? XeenIndoorScene().classifyActors(world,c.camera,c.actors) : classify(c.actors,c.camera);activate(c.actors,c.result.view); }
		bool living=false;for(const auto &owner:c.characters) living=living || xeenCombatTargetable(owner);
		const bool engaged=living && c.classify && c.result.view.engaged();
		if(engaged) c.result.outcome=XeenEncounterOutcome::Engaged;
		// Rest's condition deaths are checked after its completion scroll.
		// Preserve early defeat for ordinary play, contact, or a ranged source
		// that actually returns control because no target remains.
		const bool defeat=!living && (!c.sleeping || c.result.view.engaged() || c.noTargets);
		if(defeat) { c.result.outcome=XeenEncounterOutcome::Stopped;c.result.reason=XeenEncounterStop::Defeat;c.pending=0; }
		validateEnvironment(world,c.actors,events);
		auto observation=std::make_shared<XeenRegionalObservation>();observation->shots=c.shots;observation->count=c.shotCount;observation->after=c.characters;
		c.result.consequences=std::move(observation);check();
		c.result.revision=entry._revision+1;
		// One nonthrowing publication spans both opportunities and the full tick.
		regionalActors.swap(c.actors);session._journeyRandom=c.random.continuation();
		camera=c.camera;party.encounterContext=c.context;
                    if(c.time && c.timeDone)party.serviceEconomy=c.time->economy;
                    if(c.time && c.timeDone)for(unsigned n=0;n<6;++n)party.roster._combatInputs[kXeenCombatOwners[n]]=c.inputs[n];
		for(const auto &value:c.characters) { auto &owner=party.roster.at(value.rosterId);owner.currentHp=value.currentHp;owner.conditions=value.conditions;owner.armor=value.armor;
         if(c.time && c.timeDone && c.time->resetTemps)xeenResetCharacterTemps(owner,*party.roster._combatInputs[value.rosterId]); }
		state._pending=c.pending;state._revision=session._encounterRevision=c.result.revision;
		if(engaged || defeat) { state._phase=engaged?XeenEncounterPhase::Engaged:XeenEncounterPhase::SupportStopped;state._reason=c.result.reason;session._encounterTerminal=true; }
		c.result.needsRest=c.time && c.timeDone && !c.timeNoticePublished && c.time->needsRest;auto result=c.result;work.reset();return result;
	} catch(...) {
		if(!authorized() || state._revision!=entry._revision) { refused.outcome=XeenEncounterOutcome::Stale;return refused; }
		work.reset();return stop(world,state,XeenEncounterStop::Preparation);
	}
}
} // namespace mmodern
