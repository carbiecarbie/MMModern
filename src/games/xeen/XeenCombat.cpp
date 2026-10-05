// Adapted bounded rules: ScummVM developers (upstream COPYRIGHT), GPL-3.0-or-later.
// Pin 6814ee9ba54582f5b5adcffab49efbbd8f589edd, engines/mm/xeen/{combat,
// character,interface,party}.cpp and create_xeen/constants.cpp. No commercial data.
#include "games/xeen/XeenCombat.h"
#include "formats/xeen/XeenCharacterFormat.h"
#include "games/xeen/XeenCharacterRules.h"
#include "games/xeen/XeenCombatRules.h"
#include "games/xeen/XeenJourneyRules.h"
#include "games/xeen/XeenJourneyProgression.h"
#include "games/xeen/XeenRestoreGuard.h"
#include "games/xeen/XeenRegionalRules.h"
#include "games/xeen/XeenIndoorScene.h"
#include "games/xeen/XeenEventTrigger.h"
#include <algorithm>
#include <limits>
#include <stdexcept>
#include <tuple>
#include <type_traits>

namespace mmodern {
namespace {
using Phase=XeenCombatPhase; using Work=XeenCombatWork; using Status=XeenCombatStatus;
using Failure=XeenCombatFailure; using Rules=XeenCharacterRules;
using Operation=XeenCombatOperation; using AttackOutcome=XeenCombatAttackOutcome;
struct IntegrityError : std::runtime_error { using std::runtime_error::runtime_error; };
void require(bool yes,const char *what) { if(!yes) throw std::invalid_argument(what); }
void advance(std::uint64_t &n) {
	if(n==std::numeric_limits<std::uint64_t>::max()) throw std::overflow_error("combat generation exhausted"); ++n;
}
bool same(XeenAttributeValue a,XeenAttributeValue b) { return a.permanent==b.permanent && a.temporary==b.temporary; }
bool same(const XeenCombatInputs &a,const XeenCombatInputs &b) {
	return same(a.might,b.might)&&same(a.speed,b.speed)&&same(a.accuracy,b.accuracy)&&a.temporaryAc==b.temporaryAc&&a.experience==b.experience&&
		bool(a.luck)==bool(b.luck)&&(!a.luck||same(*a.luck,*b.luck))&&a.resistances==b.resistances&&
		bool(a.poisonResistance)==bool(b.poisonResistance)&&(!a.poisonResistance||same(*a.poisonResistance,*b.poisonResistance));
}
bool same(const XeenItemCategory &a,const XeenItemCategory &b) {
	for(unsigned i=0;i<9;++i) if(!xeenSameItem(a[i],b[i])) return false; return true;
}
bool same(const XeenCharacter &a,const XeenCharacter &b) {
	return a.rosterId==b.rosterId && a.name==b.name && a.sex==b.sex && a.race==b.race && a.characterClass==b.characterClass &&
		same(a.intellect,b.intellect)&&same(a.personality,b.personality)&&same(a.endurance,b.endurance)&&
		a.permanentLevel==b.permanentLevel&&a.temporaryLevel==b.temporaryLevel&&a.temporaryAge==b.temporaryAge&&
		a.maxStatSkills.astrologer==b.maxStatSkills.astrologer&&a.maxStatSkills.bodybuilder==b.maxStatSkills.bodybuilder&&
		a.maxStatSkills.prayerMaster==b.maxStatSkills.prayerMaster&&a.maxStatSkills.prestidigitation==b.maxStatSkills.prestidigitation&&
		a.hasSpells==b.hasSpells&&a.learnedSpells==b.learnedSpells&&same(a.weapons,b.weapons)&&same(a.armor,b.armor)&&same(a.accessories,b.accessories)&&
		same(a.miscellaneous,b.miscellaneous)&&a.currentHp==b.currentHp&&a.currentSp==b.currentSp&&a.conditions==b.conditions&&a.birthYear==b.birthYear;
}
bool same(const XeenCamera &a,const XeenCamera &b) { return a.mapId==b.mapId&&a.x==b.x&&a.y==b.y&&a.direction==b.direction; }
bool same(const XeenGameplayContext &a,const XeenGameplayContext &b) {
	return a.profile==b.profile&&a.difficulty==b.difficulty&&a.day==b.day&&a.year==b.year&&a.minutes==b.minutes&&
		a.ctr24==b.ctr24&&a.rested==b.rested&&a.newDay==b.newDay&&a.effects==b.effects&&a.lightAndResistances==b.lightAndResistances;
}
bool same(const XeenActor &a,const XeenActor &b) {
	return a.id==b.id&&a.original.x==b.original.x&&a.original.y==b.original.y&&a.original.direction==b.original.direction&&
		a.original.tableIndex==b.original.tableIndex&&a.original.resourceId==b.original.resourceId&&a.x==b.x&&a.y==b.y&&
		a.hp==b.hp&&a.activated==b.activated&&a.lifecycle==b.lifecycle&&a.status==b.status&&
		bool(a.statistics)==bool(b.statistics)&&(!a.statistics||a.statistics->raw==b.statistics->raw);
}
bool terminal(Phase p) { return p==Phase::Disengaged||p==Phase::Victory||p==Phase::Defeat||p==Phase::SupportStopped||p==Phase::Failed; }
struct Busy { bool &b; explicit Busy(bool &v):b(v){b=true;} ~Busy(){b=false;} };
}

std::uint64_t XeenCombatBoundary::hold(Work work) {
	const auto i=static_cast<unsigned>(work); require(i<leases.size(),"invalid boundary work");
	advance(epoch); leases[i]=epoch; return epoch;
}
void XeenCombatBoundary::release(Work work,std::uint64_t lease) {
	const auto i=static_cast<unsigned>(work); require(i<leases.size(),"invalid boundary work");
	if(work==Work::PresentationFailure || leases[i]!=lease || !lease) return;
	advance(epoch); leases[i]=0;
}
bool XeenCombatBoundary::only(Work work,std::uint64_t lease) const noexcept {
    if (!holds(work,lease)) return false;
    for (unsigned i=0;i<leases.size();++i) if (i!=static_cast<unsigned>(work) && leases[i]) return false;
    return true;
}
bool XeenCombatBoundary::quiet() const noexcept { for(auto l:leases) if(l) return false; return true; }
bool XeenCombatBoundary::preparationReady() const noexcept {
	for(unsigned i=1;i<leases.size();++i)if(leases[i])return false;return true;
}

struct XeenCombat::Impl {
	XeenCombat *owner;
	XeenWorld &world; XeenPartyState &party; XeenCamera &camera; XeenCombatBoundary &boundary;
	// Exact preimage certificate only: never assigned back to a live owner.
	struct PartyPreimage {
		struct Characters {
			std::array<XeenCharacter,30> values;
			XeenCharacter &at(std::size_t i) { return values.at(i); }
			const XeenCharacter &at(std::size_t i) const { return values.at(i); }
		} roster;
		XeenParty party;
		XeenCloudsQuestItems questItems;
		XeenCloudsQuestFlags questFlags;
		std::optional<XeenRegionalRecoveryState> recovery;
		std::optional<XeenGameplayContext> encounterContext;
		std::optional<XeenMonsterTreasure> treasure;
		std::optional<XeenServiceEconomy> serviceEconomy;
		std::uint8_t firstSerializedCount, effectiveSerializedCount;
		std::vector<std::string> diagnostics;
		explicit PartyPreimage(const XeenPartyState &p) : roster{p.roster.characters()}, party(p.party),
			questItems(p.questItems), questFlags(p.questFlags), recovery(p.regionalRecovery), encounterContext(p.encounterContext), treasure(p.monsterTreasure),
			serviceEconomy(p.serviceEconomy),
			firstSerializedCount(p.firstSerializedCount), effectiveSerializedCount(p.effectiveSerializedCount), diagnostics(p.diagnostics) {}
	} expected;
	bool journey = false, ended = false, episodeLethal = false;
	std::uint32_t journeySeed = 0;
	std::optional<XeenJourneyRandomState> expectedRandom;
	bool moveDue=false, chargeRound=false;
    XeenMovementCountdown countdown;
    bool stepped=false;
	std::uint8_t participants=0x3f;
	XeenCombatExitCause exitCause=XeenCombatExitCause::None;
	bool finishedDisengagement=false;
	XeenActorView destinationView;
	std::array<std::optional<XeenMonsterIdentity>,3> contact{};
	std::optional<XeenMonsterIdentity> selected;
	std::array<int,6> playerSpeeds{};
	const void *journeyOwner = nullptr;
	std::uint64_t journeyGeneration = 0;
	std::array<std::optional<XeenCombatInputs>,30> allInputs{};
	std::set<XeenMonsterIdentity> accounted;
	std::unique_ptr<XeenRestoreGuard> lifetime;
	std::unique_ptr<XeenWorld::GameplayBorrow> borrow;
	const XeenGameFlags *flags = nullptr;
	XeenGameFlags::Storage expectedFlags{};
	XeenCamera expectedCamera;
	XeenGameplayContext initialContext;
	std::array<XeenCombatInputs,6> inputs{};
	std::vector<XeenActor> actors;
	std::optional<std::vector<XeenActor>> inactiveActors;
	std::set<XeenObjectIdentity> objects;
	std::set<XeenEventIdentity> removedEvents;
	std::vector<XeenMonsterRecord> statistics;
	XeenEventFile events;
	XeenEncounterState approach;
	XeenCombatRandom rng;
	std::function<void()> probe;
	Phase phase=Phase::Engaged; Work work=Work::None;
	std::uint64_t generation=1;
	bool busy=false;
	std::array<int,9> order{};
	std::array<bool,9> acted{};
	std::array<bool,6> blocked{};
	int turn=-1;
	XeenCombatResult last;
	struct Consequences {
		XeenCombatRandom rng;
		std::optional<XeenRunCandidate> run;
		std::optional<XeenPhysicalPlayerCandidate> player;
		std::optional<XeenEnemyAttackCandidate> enemy;
		std::optional<XeenRegionalOpportunityCandidate> movement;
		std::optional<XeenConditionTimeCandidate> time;
		std::optional<XeenMonsterDropCandidate> drop;
		XeenCombatResult result;
		bool physicalDone = false;
	};
	std::optional<Consequences> consequences;
    struct Casting {
        // Private selection/phase state; cast() exports a copy, never this object.
        XeenCombatCastView view;
        struct Reservation {
            unsigned participant=0,owner=0,slot=0,spell=0,cost=0;
            int originalSp=0;
            std::uint8_t participants=0;
            std::array<bool,9> acted{};
            std::array<bool,6> blocked{};
            std::optional<XeenActor> enemy;
        };
        std::optional<Reservation> reservation;
        std::optional<unsigned> partyTarget;
        Ticket authority;
        XeenCombatResult operation;
        std::uint64_t lease=0,deadline=0;
        bool presented=false;
        XeenCombatRandom random;
        std::optional<XeenMagicArrowCandidate> arrow;
        std::optional<XeenMonsterDropCandidate> drop;
        bool arrowDone=false;
        std::optional<XeenJourneyRandomState> impactRandom;
        std::unique_ptr<XeenRestoreGuard> guard;
        Phase successorPhase=Phase::Failed;
        Work successorWork=Work::None;
        int successorTurn=-1;
    };
    std::unique_ptr<Casting> casting;
    void checkCast(const Ticket &t) {
        require(casting && owner->current(t) && phase==Phase::Casting &&
            owner->current(casting->authority) &&
            boundary.only(XeenCombatBoundary::Work::Casting,casting->lease),"Foreign combat cast lease");
        casting->guard->check();require(exact(),"Combat cast preimage changed");
    }
    void checkCastReservation(const Ticket &t,const Casting::Reservation &r,bool paid) {
        checkCast(t);
        require(r.participant<6 && turn==int(r.participant) &&
            party.party.activeRosterIds().at(r.participant)==r.owner && kXeenCombatOwners[r.participant]==r.owner &&
            participants==r.participants && acted==r.acted && blocked==r.blocked &&
            !acted[r.participant] && (participants&(1u<<r.participant)),"Cast participant reservation changed");
        const auto &ch=party.roster.at(r.owner);
        const auto category=XeenLearnedSpellRules::categoryForClass(ch.characterClass);
        const auto spell=category ? XeenLearnedSpellRules::spellForSlot(*category,r.slot) : std::nullopt;
        require(ch.canAct() && playerSpeeds[r.participant]>0 && ch.hasSpells &&
            XeenLearnedSpellRules::known(ch,r.slot) && spell && *spell==r.spell &&
            ch.currentSp==r.originalSp-(paid ? int(r.cost) : 0),"Cast payer/spell reservation changed");
        if(r.enemy) {
            const auto &actor=*r.enemy;
            auto bound=actor;
            if(casting->view.phase==XeenCombatCastPhase::PostImpact)bound.hp=casting->operation.actorHpAfter;
            require(actor.statistics && actor.hp>0 && actor.x==camera.x && actor.y==camera.y &&
                actor.lifecycle==XeenActorLifecycle::Present && actor.status==XeenActorStatus::Physical &&
                std::find(contact.begin(),contact.end(),std::optional<XeenMonsterIdentity>{actor.id})!=contact.end() &&
                same(bound,actors.at(actor.id.recordIndex)) && same(bound,activeActors().at(actor.id.recordIndex)),
                "Cast monster/statistics/HP reservation changed");
        }
    }
    void reserveCastSuccessor() noexcept {
        reconcile();sortOrder();selectNext();
        casting->successorPhase=casting->view.successorPhase=phase;casting->successorWork=casting->view.successorWork=work;casting->successorTurn=casting->view.successorParticipant=turn;
        phase=Phase::Casting;work=Work::None;turn=int(casting->reservation->participant);
    }
    std::unique_ptr<XeenRestoreGuard> prepareCastGuard() {
        auto guard=std::make_unique<XeenRestoreGuard>(world,party,camera,*flags);
        guard->retainResources(*casting->guard);
        return guard;
    }
	XeenConsequenceCharacters characters() const {
		XeenConsequenceCharacters result;
		for (unsigned i=0;i<6;++i) result[i]=party.roster.at(kXeenCombatOwners[i]);
		return result;
	}
	void publishCharacters(const XeenConsequenceCharacters &values) noexcept {
		for (const auto &value:values) {
			auto &live=party.roster.at(value.rosterId);auto &before=expected.roster.at(value.rosterId);
			live.currentHp=before.currentHp=value.currentHp;
			live.conditions=before.conditions=value.conditions;
			live.armor=before.armor=value.armor;
		}
	}
	void refreshSpeeds() {
		for(unsigned i=0;i<6;++i) playerSpeeds[i]=Rules::effectivePhysical(character(i),inputs[i],Rules::PhysicalAttribute::Speed,{party.encounterContext->year});
	}
	Impl(XeenCombat *o,XeenWorld &w,XeenPartyState &p,XeenCamera &c,XeenCombatBoundary &b,
		const XeenGameplayContext &ctx,const std::vector<XeenMonsterRecord> &s,const XeenEventFile &e,XeenCombatRandom r):
		owner(o),world(w),party(p),camera(c),boundary(b),expected(p),expectedCamera(c),initialContext(ctx),
		objects(w.sessionState().disabledObjects()),removedEvents(w.sessionState().disabledEvents()),statistics(s),events(e),rng(std::move(r)) {}
	auto &session() { return world._sessionState; }
	std::vector<XeenActor> &activeActors() {
		return journey && camera.mapId==XeenMapIdentity(28) ? *session()._vertigoActors : session()._actors;
	}
	bool indoor() const noexcept { return journey && camera.mapId==XeenMapIdentity(28); }
	XeenActorView classify(const std::vector<XeenActor> &values) {
		return indoor() ? XeenIndoorScene().classifyActors(world,camera,values) : XeenActorApproach::classify(values,camera);
	}
    XeenActorView classify(const std::vector<XeenActor> &values,const XeenCamera &at) {
        return indoor() ? XeenIndoorScene().classifyActors(world,at,values) : XeenActorApproach::classify(values,at);
    }
    Consequences prepareMovement(const Ticket &t,const XeenCamera &at,Operation operation) {
        Consequences c;c.rng=rng;c.result=observation(Status::Pending);
        c.result.operation=operation;c.result.oldRevision=revision();c.result.participant=turn;
        if(indoor())c.movement.emplace(world,actors,at,characters(),inputs,party.encounterContext->year,participants,blocked);
        else {const auto map=world.map(at.mapId);c.movement.emplace(map,actors,at,characters(),inputs,party.encounterContext->year,participants,blocked);}
        c.movement->staged=true;probeFor(t);return c;
    }
	const std::vector<XeenActor> &activeActors() const {
		const auto &s=world._sessionState;
		return journey && camera.mapId==XeenMapIdentity(28) ? *s._vertigoActors : s._actors;
	}
	std::uint64_t revision() const { return journey && lifetime && !lifetime->worldAlive() ? 0 : world._sessionState._encounterRevision; }
	bool exact() const {
		if (journey && (!lifetime || !lifetime->ownersAlive() || !lifetime->mutationHistoryCurrent() || !lifetime->cachesCurrent() || flags->values() != expectedFlags ||
			!world.sessionState().journey() || world.sessionState().skeletonSeed() != journeySeed ||
			world.sessionState().journeyRandom()!=expectedRandom ||
			world._sessionState._journeyOwner != journeyOwner || world._sessionState._journeyGeneration != journeyGeneration ||
			!world._sessionState._encounterInitialized || !world._sessionState._encounterMarked ||
			!world._sessionState._encounterTerminal || world._sessionState._combatApproachState != &approach ||
			world._sessionState._combatEntered != (phase != Phase::Engaged) ||
			world.sessionState().accountedMonsters() != accounted || world.sessionState().journeyActivity() != XeenJourneyActivity::Combat)) return false;
		if(!same(camera,expectedCamera)||party.monsterTreasure!=expected.treasure||party.serviceEconomy!=expected.serviceEconomy||party.party.activeRosterIds()!=expected.party.activeRosterIds()||
			party.questItems.counts()!=expected.questItems.counts()||party.questFlags.values()!=expected.questFlags.values()||party.regionalRecovery!=expected.recovery||
			party.firstSerializedCount!=expected.firstSerializedCount||party.effectiveSerializedCount!=expected.effectiveSerializedCount||
			party.diagnostics!=expected.diagnostics||bool(party.encounterContext)!=bool(expected.encounterContext)) return false;
		if(party.encounterContext&&!same(*party.encounterContext,*expected.encounterContext)) return false;
		for(unsigned i=0;i<30;++i) if(!same(party.roster.at(i),expected.roster.at(i))) return false;
		if(!party.roster.combatMarked()||world._sessionState._combatOwner!=owner) return false;
		for(unsigned i=0;i<30;++i) {
			const auto &v=party.roster.combatInputs(i);
			if (!v || !allInputs[i] || !same(*v,*allInputs[i])) return false;
		}

		const auto &s=world.sessionState();
		if(s.disabledObjects()!=objects||s.disabledEvents()!=removedEvents||activeActors().size()!=actors.size()) return false;
		for(unsigned i=0;i<actors.size();++i) if(!same(actors[i],activeActors()[i])) return false;
		if(inactiveActors) {
			const auto &inactive=camera.mapId==XeenMapIdentity(28) ? s.actors() : s.regionalActors(28);
			if(inactive.size()!=inactiveActors->size())return false;
			for(unsigned i=0;i<inactive.size();++i)if(!same(inactive[i],(*inactiveActors)[i]))return false;
		}
		return true;
	}
	XeenCharacter &character(int participant) { return party.roster.at(kXeenCombatOwners.at(participant)); }
	void reconcile() noexcept {
		const auto old=contact; const auto oldActed=acted;
		const auto currentEnemy=turn>=6 ? contact[turn-6] : std::optional<XeenMonsterIdentity>{};
		contact.fill({}); unsigned count=0;
		for (const auto &a:activeActors()) if (a.x==camera.x && a.y==camera.y && a.hp>0 && a.lifecycle==XeenActorLifecycle::Present && a.status==XeenActorStatus::Physical && count<3) contact[count++]=a.id;
		for (unsigned i=0;i<3;++i) {
			acted[6+i]=false;
			for (unsigned j=0;j<3;++j) if (contact[i] && contact[i]==old[j]) acted[6+i]=oldActed[6+j];
			if (currentEnemy && contact[i]==currentEnemy) turn=6+i;
		}
		bool retained=false; for (auto id:contact) if (id && id==selected) retained=true;
		if (!retained) selected=contact[0];
	}
	void sortOrder() noexcept {
		std::array<int,9> speeds{};
		for(int i=0;i<6;++i) speeds[i]=playerSpeeds[i];
		for(int i=6;i<9;++i) speeds[i]=contact[i-6] ? activeActors()[contact[i-6]->recordIndex].statistics->speed() : 0;
		for(int i=0;i<9;++i) order[i]=i;
		std::sort(order.begin(),order.end(),[&](int a,int b){return speeds[a]!=speeds[b]?speeds[a]>speeds[b]:a<b;});
	}
	bool continuingParticipants() {
		for(unsigned i=0;i<6;++i) if((participants & (1u<<i)) && xeenCombatTargetable(character(i))) return true;
		return false;
	}
	bool disengagementDue() noexcept {
		if(!contact[0] || continuingParticipants()) return false;
		if(participants==0x3f) { phase=Phase::Failed;work=Work::None;return true; }
		if(exitCause==XeenCombatExitCause::None) exitCause=XeenCombatExitCause::AttritionAfterEscape;
		turn=-1;phase=Phase::DisengagementPending;work=moveDue?Work::Round:Work::FinishDisengagement;return true;
	}
	void selectNext() noexcept {
		if(defeated()) { turn=-1;phase=Phase::Defeat;work=Work::None;return; }
		if(disengagementDue()) return;
		if (!contact[0] && !moveDue) { turn=-1; phase=Phase::VictoryAwaitingEnd; work=Work::End; return; }
		for(auto i:order) if(!acted[i] && (i>=6 ? bool(contact[i-6]) : ((participants & (1u<<i)) && character(i).canAct() && (playerSpeeds[i]>0)))) {
			turn=i; phase=i>=6?Phase::PendingEnemy:moveDue?Phase::PendingRound:Phase::PlayerReady;
			work=i>=6?Work::Enemy:moveDue?Work::Round:Work::None; return;
		}
		turn=-1; phase=Phase::PendingRound; work=Work::Round;
	}
	bool defeated() { for(int i=0;i<6;++i) if(xeenCombatTargetable(character(i))) return false; return true; }
	void terrainAdmission() {
		// Immutable approach admission separated from Good-only character admission.
		// Only the retained Arrow post-HP frame may contain a present actor with
		// zero HP. Validate its original bound image; exact() still checks the
		// complete live transient state and quiet validation remains unchanged.
		auto admission=actors;
		if(casting && casting->view.phase==XeenCombatCastPhase::PostImpact && casting->reservation && casting->reservation->enemy)
			admission.at(casting->reservation->enemy->id.recordIndex)=*casting->reservation->enemy;
		if(journey && camera.mapId==XeenMapIdentity(28)) {
			xeenValidateVertigoActors(world,admission);return;
		}
		{ XeenActorApproach::validateEnvironment(world,admission,events); return; }
	}
	void probeFor(const Ticket &t) {
		if(probe && journey) {
			if(!exact())throw IntegrityError("combat preimage changed before callback");
			XeenRestoreGuard guard(world,party,camera,*flags);
			const auto check=[&] {
				if(!guard.current()) {
					lifetime->failed=true;
					throw IntegrityError("combat callback changed owner mutation history");
				}
			};
			try {probe();check();}catch(...){check();throw;}
		} else if(probe) probe();
		require(owner->current(t),"stale combat callback");
		if(!exact())throw IntegrityError("combat preimage changed");
	}
	void resourcesFor(const Ticket &t) {
		try {
			world._combatCheck=[&]{require(owner->current(t)&&exact(),"combat resource callback changed authority");};
			if (journey) {
				XeenRestoreGuard guard(world,party,camera,*flags);
				guard.retainResources(*lifetime);
				XeenRestoreGuard::Providers providers(guard,world);
				terrainAdmission(); guard.check();
				if (!exact()) throw IntegrityError("combat retained resource preimage changed");
			} else terrainAdmission();
			clearResourceCheck();
		}catch(...){clearResourceCheck();throw;}
	}
	void clearResourceCheck() noexcept { if (!journey || lifetime->worldAlive()) world._combatCheck = {}; }
	XeenCombatResult observation(Status status) const noexcept {
		XeenCombatResult r; r.status=status;r.phase=phase;r.work=work;r.revision=revision();r.generation=generation;
        for(unsigned i=0;i<6;++i)if(blocked[i])r.blockedMembers|=1u<<i;
		if (journey && lifetime && !lifetime->ownersAlive()) return r;
		r.participant=turn;r.minutes=party.encounterContext?std::uint16_t(party.encounterContext->minutes):480; return r;
	}
	XeenCombatResult adopt(XeenCombatResult r,Status status=Status::Advanced) noexcept {
        r.blockedMembers=0;for(unsigned i=0;i<6;++i)if(blocked[i])r.blockedMembers|=1u<<i;
		r.status=status;r.phase=phase;r.work=work;r.revision=revision();r.generation=generation;
		r.minutes=party.encounterContext?std::uint16_t(party.encounterContext->minutes):480;r.participantsAfter=participants;r.exitCause=exitCause;last=r;
        if(casting)casting->authority=owner->ticket();
        return r;
	}
	void capacity() { require(revision()<std::numeric_limits<std::uint64_t>::max()&&generation<std::numeric_limits<std::uint64_t>::max()-2,"combat revision exhausted"); }
	void published() noexcept {
		++session()._encounterRevision;++generation;
		// Every callback/preimage check precedes these callback-free owned stores.
		// Renew observation for their authorized delta, never a failed history.
		if (journey && !lifetime->failed) lifetime->adoptMutationBoundary();
	}

};

XeenCombat::XeenCombat(XeenWorld &w, XeenPartyState &p, XeenCamera &c, XeenCombatBoundary &b,
		const XeenGameFlags &flags, const XeenEncounterState &state, const std::vector<XeenMonsterRecord> &statistics,
		const XeenEventFile &events) : impl(std::make_unique<Impl>(this,w,p,c,b,p.encounterContext.value(),statistics,events,
			XeenCombatRandom(*w.sessionState().journeyRandom()))) {
	auto &d = *impl; auto &s = d.session();
	require(s.journey() && s._journeyOwner && s._journeyActivity == XeenJourneyActivity::Attachment &&
		!s._combatOwner && !s._combatEntered && b.quiet() && b.world == &w && b.party == &p && b.camera == &c,
		"Journey attachment requires current coordination");
	require(XeenActorApproach::authoritative(w,p,c,state) && state.phase() == XeenEncounterPhase::Engaged,
		"Journey attachment requires genuine engagement");
	xeenValidateJourneyMelee(p);
	 d.expectedRandom=s.journeyRandom();
	d.journey = true; d.flags = &flags; d.expectedFlags = flags.values();
	d.journeySeed = s._skeletonSeed; d.journeyOwner = s._journeyOwner; d.journeyGeneration = s._journeyGeneration;
	if(c.mapId==XeenMapIdentity(28)) {
		require(bool(s._vertigoActors),"Vertigo combat actors are absent");
		d.inactiveActors=s._actors;
	} else if(s._vertigoActors) d.inactiveActors=*s._vertigoActors;
	d.actors = d.activeActors(); d.accounted = s._accountedMonsters; d.approach = state;
	for (unsigned i = 0; i < 30; ++i) d.allInputs[i] = p.roster.combatInputs(i);
	for (unsigned i = 0; i < 6; ++i) d.inputs[i] = *d.allInputs[kXeenCombatOwners[i]];
	d.borrow.reset(new XeenWorld::GameplayBorrow(w,p,c,flags));
	d.lifetime = std::make_unique<XeenRestoreGuard>(w,p,c,flags);
	// All fallible preparation precedes attachment; no CHR/PTY mutable value is installed.
	s._combatOwner = this; s._combatApproachState = &d.approach; s._journeyActivity = XeenJourneyActivity::Combat;
	d.phase = Phase::Engaged; d.last = d.observation(Status::Accepted);
}

XeenCombat::~XeenCombat() {
	if (!impl) return;
	auto &d=*impl;
	if (d.journey && !d.lifetime->worldAlive()) return;
	if (d.world._sessionState._combatOwner==this) {
		if (d.journey) d.world._sessionState._journeyActivity = XeenJourneyActivity::Failed;
		d.world._combatCheck={};d.world._combatAuthorized={};
		d.world._sessionState._combatOwner=nullptr;
		d.world._sessionState._combatApproachState=nullptr;
	}
}
XeenCombat::Ticket XeenCombat::ticket() const noexcept {
	Ticket t;const auto &d=*impl;
	if (d.journey && !d.lifetime->ownersAlive()) return t;
	t.owner=this;t.incarnation=d.world._incarnation;
	t.generation=d.generation;t.revision=d.revision();
	t.boundary=d.boundary.generation();t.phase=d.phase;t.work=d.work;return t;
}
bool XeenCombat::ticketCurrent(const Ticket &t) const noexcept {
	const auto &d=*impl;if (d.journey && !d.lifetime->ownersAlive()) return false;
	return t.owner==this&&t.incarnation==d.world._incarnation&&
		t.generation==d.generation&&t.revision==d.revision()&&t.boundary==d.boundary.generation()&&
		t.phase==d.phase&&t.work==d.work&&d.world._sessionState._combatOwner==this;
}
bool XeenCombat::current(const Ticket &t) const noexcept {
	if (!ticketCurrent(t)) return false;
	if (impl->journey && !impl->exact()) {
		const_cast<XeenCombat *>(this)->fail(t,Failure::Integrity);
		return false;
	}
	return true;
}
XeenCombatResult XeenCombat::result() const noexcept {return impl->last;}
bool XeenCombat::boundTo(const XeenWorld &w,const XeenPartyState &p,const XeenCamera &c,const XeenCombatBoundary &b) const noexcept {
	return &impl->world==&w && &impl->party==&p && &impl->camera==&c && &impl->boundary==&b;
}

const XeenEncounterState &XeenCombat::approachState() const noexcept {return impl->approach;}
XeenCombatPhase XeenCombat::phase() const noexcept {return impl->phase;}
XeenCombatWork XeenCombat::pending() const noexcept {return impl->work;}
int XeenCombat::participant() const noexcept {return impl->turn;}
XeenCombatRandom XeenCombat::random() const noexcept {return impl->rng;}
unsigned XeenCombat::movementCountdown() const noexcept {return impl->countdown.remaining();}
bool XeenCombat::stepped() const noexcept {return impl->stepped;}
XeenMovementCountdown &XeenCombat::countdownForScheduling() noexcept {return impl->countdown;}
void XeenCombat::setProbe(std::function<void()> p) { require(!impl->busy,"cannot replace an in-flight probe");impl->probe=std::move(p); }
void XeenCombat::preparePresentation(const Ticket &t,const std::function<void()> &compose) {
 guardCallback(t,compose);
}
void XeenCombat::guardCallback(const Ticket &t,const std::function<void()> &callback) {
 auto &d=*impl;
 require(current(t)&&!d.busy,"Stale combat callback");
 if(!d.journey) { callback();return; }
 require(d.exact(),"Combat callback preimage changed");
 if(d.casting)d.checkCast(t);
 Busy busy(d.busy);
 auto guard=std::make_unique<XeenRestoreGuard>(d.world,d.party,d.camera,*d.flags);
 guard->retainResources(*d.lifetime);
 const auto check=[&] {guard->check();require(current(t),"Reentrant combat callback");if(d.casting)d.checkCast(t);};
 try {
  { XeenRestoreGuard::Providers providers(*guard,d.world,check);callback();check(); }
  if(d.casting)d.casting->guard->retainResources(*guard);
  d.lifetime.swap(guard);
 } catch(...) {
  // Compatible I/O failure retains all successfully admitted resource bytes.
  // A changed owner or resource fails closed and cannot renew authority.
  try {check();d.lifetime.swap(guard);}catch(...){d.lifetime->failed=true;fail(ticket(),Failure::Integrity);}
  throw;
 }
}
void XeenCombat::inheritResources(const Ticket &t,const XeenRestoreGuard &source) {
    auto &d=*impl;
    require(current(t) && d.journey && d.phase==Phase::Engaged && !d.busy,"Stale combat resource handoff");
    auto guard=std::make_unique<XeenRestoreGuard>(d.world,d.party,d.camera,*d.flags);
    guard->retainResources(source); // Immutable union only; no mutable copyback.
    guard->check();d.lifetime.swap(guard);
}
void XeenCombat::retainResources(XeenRestoreGuard &guard) const {
 if(impl->journey)guard.retainResources(*impl->lifetime);
}
XeenCombatResult XeenCombat::fail(const Ticket &t,Failure f) noexcept {
	auto &d=*impl;if(!ticketCurrent(t))return d.observation(Status::Stale);
	if (d.journey && f==Failure::Integrity) d.lifetime->failed=true;
	if(terminal(d.phase) && !(d.journey && (d.phase==Phase::Disengaged || (d.phase == Phase::Victory && f == Failure::Integrity)))) {
		return d.last;
	}
	auto r=d.observation(Status::Failed);r.oldRevision=d.revision();r.failure=f;r.operation=Operation::Failure;
	d.phase=f==Failure::Time?Phase::SupportStopped:Phase::Failed;d.work=Work::None;d.consequences.reset();
	if (d.journey) { d.ended = false; d.finishedDisengagement=false; d.session()._journeyActivity = XeenJourneyActivity::Failed; }
	d.session()._encounterTerminal=true;
	// Preparation authority uses coordinator generations, including its failures.
	// World revision zero is reserved until real M26 initialization publishes one.
	if(d.session()._encounterInitialized&&d.revision()!=std::numeric_limits<std::uint64_t>::max())++d.session()._encounterRevision;
	if(d.generation!=std::numeric_limits<std::uint64_t>::max())++d.generation;
	return d.adopt(r,f==Failure::Time?Status::SupportStopped:Status::Failed);
}
void XeenCombat::invalidate() noexcept { auto t=ticket();fail(t,Failure::Integrity); }

XeenCombatResult XeenCombat::beginCombat(const Ticket &t) {
	auto &d=*impl;if(!current(t))return d.observation(Status::Stale);
	if(d.busy||d.phase!=Phase::Engaged)return d.observation(Status::Refused);
	if(!d.exact()||!d.boundary.quiet())return fail(t,Failure::Integrity);
	Busy busy(d.busy);
	try {
		d.capacity();require(!d.session()._combatEntered&&d.session()._encounterTerminal&&
			XeenActorApproach::authoritative(d.world,d.party,d.camera,d.approach)&&
			d.approach.reason()==XeenEncounterStop::None,"combat handoff is not authoritative");
		const auto view=d.classify(d.activeActors());
		require(view.engaged(),"combat requires contact");
		for (unsigned i=0;i<3;++i) if (view.slots[i]) {
			const auto &a=d.activeActors().at(view.slots[i]->recordIndex);
			require(a.id==XeenMonsterIdentity{d.camera.mapId,view.slots[i]->recordIndex} &&
				a.statistics && a.hp>0 && a.lifecycle==XeenActorLifecycle::Present &&
				a.status==XeenActorStatus::Physical && a.statistics->supportsGroundMovement(),"Invalid contact actor");
			a.statistics->validateAttackCapabilities();

		}
		for (unsigned i=0;i<6;++i) { d.playerSpeeds[i]=Rules::effectivePhysical(d.character(i),d.inputs[i],Rules::PhysicalAttribute::Speed,{d.party.encounterContext->year});  }
		d.resourcesFor(t);d.reconcile();d.sortOrder();d.probeFor(t);
		d.countdown.arm(d.approach.pending());
		d.moveDue=d.approach.pending()!=0; d.chargeRound=false; d.approach._pending=0;
		auto r=d.observation(Status::Advanced);r.oldRevision=d.revision();
		r.operation=Operation::BeginCombat;
		d.session()._combatEntered=true;d.acted.fill(false);d.blocked.fill(false);d.selectNext();d.published();
		return d.adopt(r);
	}catch(...){return fail(t,d.journey && !d.exact() ? Failure::Integrity : Failure::Preparation);}
}
std::array<std::optional<XeenMonsterIdentity>,3> XeenCombat::contacts() const noexcept { return impl->contact; }
std::uint8_t XeenCombat::participants() const noexcept { return impl->participants; }
XeenCombatExitCause XeenCombat::exitCause() const noexcept { return impl->exitCause; }
std::optional<XeenMonsterIdentity> XeenCombat::selectedTarget() const noexcept { return impl->selected; }
XeenCombatResult XeenCombat::selectTarget(const Ticket &t,unsigned row) {
	auto &d=*impl;
	if (!current(t)) return d.observation(Status::Stale);
	if (d.busy || d.phase!=Phase::PlayerReady || row>=3 || !d.contact[row]) return d.observation(Status::Refused);
	if (!d.exact() || !d.boundary.quiet()) return fail(t,Failure::Integrity);
	try { d.capacity(); } catch (...) { return fail(t,Failure::Overflow); }
	d.selected=d.contact[row]; ++d.generation; return d.adopt(d.observation(Status::Advanced));
}
XeenCombatResult XeenCombat::rotate(const Ticket &t,NavigationAction action) {
    auto &d=*impl;if(!current(t))return d.observation(Status::Stale);
    if(d.busy || d.phase!=Phase::PlayerReady || d.casting || d.consequences || !d.boundary.quiet() ||
        (action!=NavigationAction::TurnLeft && action!=NavigationAction::TurnRight))return d.observation(Status::Refused);
    Busy busy(d.busy);
    try {
        d.capacity();d.resourcesFor(t);
        auto prepared=std::make_unique<XeenRestoreGuard>(d.world,d.party,d.camera,*d.flags);
        prepared->retainResources(*d.lifetime);
        auto facing=d.camera;facing.direction=static_cast<XeenDirection>((unsigned(facing.direction)+(action==NavigationAction::TurnLeft?3:1))%4);
        auto r=d.observation(Status::Advanced);r.operation=Operation::Rotate;r.oldRevision=d.revision();
        r.approachAction=action==NavigationAction::TurnLeft?XeenEncounterAction::Left:XeenEncounterAction::Right;
        r.origin=XeenCombatLocation{d.camera.mapId,d.camera.x,d.camera.y,d.camera.direction};
        r.destination=XeenCombatLocation{facing.mapId,facing.x,facing.y,facing.direction};
        const bool flush=d.countdown.remaining()!=0;
        std::optional<Impl::Consequences> movement;
        std::vector<XeenActor> visible,expectedActors;
        {
            XeenRestoreGuard::Providers providers(*prepared,d.world,[&]{require(current(t) && d.exact(),"Stale combat rotation resource");});
            if(flush) {movement=d.prepareMovement(t,facing,Operation::Rotate);movement->result=r;}
            else {
                visible=d.actors;const auto view=d.classify(visible,facing);
                for(unsigned i=0;i<visible.size();++i)visible[i].activated=visible[i].activated || view.activation[i];
                expectedActors=visible;
            }
            d.probeFor(t);prepared->check();
        }
        if(!flush) {auto &values=d.indoor()?*prepared->s._vertigoActors:prepared->s._actors;values=visible;}
        prepared->cameraValue.direction=facing.direction;
        require(current(t) && d.exact(),"Combat rotation preimage changed before stores");
        if(movement)d.consequences.emplace(std::move(*movement));
        // No provider, allocation, time charge or initiative change after this point.
        d.camera.direction=d.expectedCamera.direction=facing.direction;
        d.countdown.clear();
        if(flush) {d.phase=Phase::PendingRound;d.work=Work::Movement;}
        else {d.activeActors().swap(visible);d.actors.swap(expectedActors);d.stepped=true;d.reconcile();}
        d.published();prepared->adoptJourneyCoordination();d.lifetime.swap(prepared);
        return d.adopt(r,flush?Status::Pending:Status::Advanced);
    }catch(...){return fail(t,!d.exact()?Failure::Integrity:Failure::Preparation);}
}
XeenCombatResult XeenCombat::drawBeat(const Ticket &t,std::uint64_t now) {
    auto &d=*impl;if(!current(t))return d.observation(Status::Stale);
    if(d.busy || d.phase!=Phase::PlayerReady || d.casting || d.consequences || !d.boundary.quiet())return d.observation(Status::Refused);
    if(!d.countdown.advance(now,true,false))return d.observation(Status::Refused);
    try {
        Busy busy(d.busy);d.capacity();d.resourcesFor(t);
        auto prepared=std::make_unique<XeenRestoreGuard>(d.world,d.party,d.camera,*d.flags);
        prepared->retainResources(*d.lifetime);
        {XeenRestoreGuard::Providers providers(*prepared,d.world,[&]{require(current(t) && d.exact(),"Stale combat movement resource");});
            d.consequences.emplace(d.prepareMovement(t,d.camera,Operation::Movement));prepared->check();}
        d.phase=Phase::PendingRound;d.work=Work::Movement;++d.generation;
        d.lifetime.swap(prepared);
    }catch(...){return fail(t,!d.exact()?Failure::Integrity:Failure::Preparation);}
    return serviceConsequences(ticket());
}
XeenCombatResult XeenCombat::command(const Ticket &t,XeenCombatCommand action) {
	auto &d=*impl;if(!current(t))return d.observation(Status::Stale);
	if(d.busy||d.phase!=Phase::PlayerReady||(action!=XeenCombatCommand::Attack&&action!=XeenCombatCommand::Block&&action!=XeenCombatCommand::Run))return d.observation(Status::Refused);
	if(action==XeenCombatCommand::Run && d.indoor())return d.observation(Status::Refused);
	if(action==XeenCombatCommand::Run && (d.turn<0 || d.turn>=6 || !(d.participants&(1u<<d.turn)) || !d.character(d.turn).canAct() || d.playerSpeeds[d.turn]<=0)) return d.observation(Status::Refused);
	if(!d.exact()||!d.boundary.quiet())return fail(t,Failure::Integrity);
	try {d.capacity();}catch(...){return fail(t,Failure::Overflow);}
	// Consume this intent before any random/provider callback.
	d.phase=Phase::PreparingAction;d.work=Work::Action;++d.generation;const auto entry=ticket();
	Busy busy(d.busy);
	try {
		d.resourcesFor(entry);
		if(action==XeenCombatCommand::Run) {
			const auto map=d.world.map(d.camera.mapId);d.probeFor(entry);
			d.consequences.emplace();auto &c=*d.consequences;c.rng=d.rng;c.result=d.observation(Status::Pending);
			c.result.oldRevision=d.revision();c.result.participant=d.turn;c.result.operation=Operation::PlayerRun;
			c.result.actingOwner=kXeenCombatOwners[d.turn];c.result.participantsBefore=c.result.participantsAfter=d.participants;
			c.run.emplace(map.geometry.difficulties[7]);return d.adopt(c.result,Status::Pending);
		}
		if(action==XeenCombatCommand::Block) {
			d.probeFor(entry);auto r=d.observation(Status::Advanced);r.oldRevision=d.revision();r.participant=d.turn;
			r.operation=Operation::Block;r.actingOwner=kXeenCombatOwners[d.turn];
			d.blocked[d.turn]=true;d.acted[d.turn]=true;d.selectNext();d.published();return d.adopt(r);
		}
		{
			d.consequences.emplace();auto &c=*d.consequences;c.rng=d.rng;c.result=d.observation(Status::Pending);
			c.result.oldRevision=d.revision();c.result.participant=d.turn;c.result.operation=Operation::PlayerAttack;
			c.result.actingOwner=kXeenCombatOwners[d.turn];c.result.targetMonster=d.selected;c.result.monster=*d.selected;
			const auto &actor=d.actors.at(d.selected->recordIndex);
			c.result.actorHpBefore=c.result.actorHpAfter=actor.hp;
			c.player.emplace(d.character(d.turn),d.inputs[d.turn],*actor.statistics,actor.original.resourceId,d.party.encounterContext->year,false);
			return d.adopt(c.result,Status::Pending);
		}
	}catch(...){return fail(entry,d.journey && !d.exact() ? Failure::Integrity : Failure::Preparation);}
}

XeenCombatResult XeenCombat::service(const Ticket &t) {
	auto &d=*impl;if(!current(t))return d.observation(Status::Stale);
	if(d.busy||d.phase==Phase::Casting||d.work==Work::None||terminal(d.phase))return d.observation(Status::Refused);
	if(!d.exact()||!d.boundary.quiet())return fail(t,Failure::Integrity);
	if(d.work==Work::FinishDisengagement) return finishDisengagement(t);
	return serviceConsequences(t);
}

std::optional<XeenCombatCastView> XeenCombat::cast() const {
    return impl->casting ? std::optional<XeenCombatCastView>{impl->casting->view} : std::nullopt;
}
bool XeenCombat::consumeCast(CastResponse &response) {
    auto &d=*impl;
    if(response.consumed || d.busy || !current(response.source))return false;
    response.consumed=true;
    return true;
}
XeenCombatResult XeenCombat::beginCast(CastResponse &response,const std::function<XeenLearnedSpellNames()> &prepare) {
    auto &d=*impl;const auto t=response.source;
    if(!consumeCast(response))return d.observation(Status::Stale);
    if(!d.journey || d.phase!=Phase::PlayerReady ||
        d.turn<0 || d.turn>=6 || !(d.participants&(1u<<d.turn)))return d.observation(Status::Refused);
    if(!d.exact() || !d.boundary.quiet())return fail(t,Failure::Integrity);
    const auto &ch=d.character(d.turn);
    if(!ch.hasSpells || !ch.learnedSpells || !XeenLearnedSpellRules::categoryForClass(ch.characterClass) ||
        !ch.canAct() || d.playerSpeeds[d.turn]<=0)return d.observation(Status::Refused);
    unsigned first=39;
    for(unsigned slot=0;slot<39;++slot)if(XeenLearnedSpellRules::known(ch,slot)){first=slot;break;}
    if(first==39)return d.observation(Status::Refused);
    Busy busy(d.busy);
    std::unique_ptr<Impl::Casting> next;
    try {
        d.capacity();d.resourcesFor(t);d.probeFor(t);
        xeenValidateJourneyMelee(d.party);
        next=std::make_unique<Impl::Casting>();
        next->view.participant=unsigned(d.turn);next->view.owner=kXeenCombatOwners[d.turn];next->view.slot=first;
        next->guard=std::make_unique<XeenRestoreGuard>(d.world,d.party,d.camera,*d.flags);
        next->guard->retainResources(*d.lifetime);
        if(prepare) {
            XeenRestoreGuard::Providers providers(*next->guard,d.world);
            next->guard->admitLearnedSpellNames(prepare());
        }
        next->guard->check();d.probeFor(t);next->guard->check();
        // Reservation and its lease are one callback-free coordination transition.
        next->lease=d.boundary.hold(XeenCombatBoundary::Work::Casting);
        d.casting=std::move(next);d.phase=Phase::Casting;d.work=Work::None;++d.generation;
        d.casting->guard->adoptJourneyCoordination();
        return d.adopt(d.observation(Status::Advanced));
    }catch(...) {
        // Only an unchanged, uncommitted reservation can retry. Retained
        // resource/owner integrity failures never reacquire authority.
        if(next && next->guard && current(t) && next->guard->current() && d.exact()) {
            try {d.lifetime->retainResources(*next->guard);++d.generation;auto refusal=d.observation(Status::Refused);refusal.failure=Failure::Preparation;return d.adopt(refusal,Status::Refused);}
            catch(...) {}
        }
        return fail(t,!d.exact()?Failure::Integrity:Failure::Preparation);
    }
}
XeenCombatResult XeenCombat::respondCast(CastResponse &response,XeenCombatCastInput action,unsigned index,
        const std::function<XeenLearnedSpellNames()> &prepare) {
    auto &d=*impl;const auto t=response.source;
    if(!consumeCast(response))return d.observation(Status::Stale);
    if(d.phase!=Phase::Casting || !d.casting)return d.observation(Status::Refused);
    Busy busy(d.busy);
    using CP=XeenCombatCastPhase;using CI=XeenCombatCastInput;
    std::unique_ptr<XeenRestoreGuard> prepared;
    try {
        d.checkCast(t);d.capacity();auto &c=*d.casting;auto &v=c.view;
        if(v.phase==CP::Preparing || v.phase==CP::Projectile || v.phase==CP::Impact || v.phase==CP::PostImpact)return d.observation(Status::Refused);
        if(v.phase==CP::Result) {
            if(action!=CI::Enter && action!=CI::Escape)return d.observation(Status::Refused);
            const auto lease=c.lease;const auto result=c.operation;
            d.phase=c.successorPhase;d.work=c.successorWork;d.turn=c.successorTurn;
            d.casting.reset();++d.generation;
            d.boundary.release(XeenCombatBoundary::Work::Casting,lease);
            return d.adopt(result);
        }
        if(v.phase==CP::PartyTarget) {
            if(action!=CI::Escape && action!=CI::PartyTarget)return d.observation(Status::Refused);
            if(action==CI::PartyTarget && (index>=6 || !(d.participants&(1u<<index))))return d.observation(Status::Refused);
            return settleCast(t,action==CI::Escape ? std::nullopt : std::optional<unsigned>{index});
        }
        if(action==CI::Escape) {
            if(v.phase==CP::Learned) {
                const auto lease=c.lease;d.casting.reset();d.phase=Phase::PlayerReady;d.work=Work::None;++d.generation;
                d.boundary.release(XeenCombatBoundary::Work::Casting,lease);
            }else { v.phase=v.phase==CP::Confirm && v.enemy ? CP::Enemy : CP::Learned;++d.generation; }
            return d.adopt(d.observation(Status::Advanced));
        }
        const auto &ch=d.character(int(v.participant));
        const auto category=XeenLearnedSpellRules::categoryForClass(ch.characterClass);
        require(category.has_value(),"Cast category changed");
        if(v.phase==CP::Learned && (action==CI::Up || action==CI::Down)) {
            unsigned chosen=v.slot;
            if(action==CI::Up) {for(unsigned s=0;s<v.slot;++s)if(XeenLearnedSpellRules::known(ch,s))chosen=s;}
            else {for(unsigned s=v.slot+1;s<39;++s)if(XeenLearnedSpellRules::known(ch,s)){chosen=s;break;}}
            v.slot=chosen;v.enemy.reset();v.refusal.clear();++d.generation;
            return d.adopt(d.observation(Status::Advanced));
        }
        if(v.phase==CP::Enemy && action==CI::EnemyTarget) {
            if(index>=3 || !d.contact[index])return d.observation(Status::Refused);
            v.enemy=d.contact[index];++d.generation;return d.adopt(d.observation(Status::Advanced));
        }
        if(action!=CI::Enter)return d.observation(Status::Refused);
        const auto id=XeenLearnedSpellRules::spellForSlot(*category,v.slot);
        const auto spell=id ? XeenLearnedSpellRules::supportedIn(*id,true) : std::nullopt;
        if(!spell || !XeenLearnedSpellRules::eligible(d.party,v.participant,v.slot,true)) {
            v.refusal="Unsupported spell or insufficient SP";++d.generation;return d.adopt(d.observation(Status::Advanced));
        }
        if(v.phase==CP::Learned) {
            v.phase=*spell==XeenLearnedSpell::MagicArrow ? CP::Enemy : CP::Confirm;
            if(v.phase==CP::Enemy)v.enemy=d.selected;
            v.refusal.clear();++d.generation;return d.adopt(d.observation(Status::Advanced));
        }
        if(v.phase==CP::Enemy) {v.phase=CP::Confirm;++d.generation;return d.adopt(d.observation(Status::Advanced));}
        require(v.phase==CP::Confirm,"Invalid cast confirmation phase");
        xeenValidateJourneyMelee(d.party);
        require(ch.canAct() && d.playerSpeeds[v.participant]>0 && (d.participants&(1u<<v.participant)) &&
            d.party.party.activeRosterIds()[v.participant]==v.owner,"Cast actor reservation changed");
        Impl::Casting::Reservation reservation;
        reservation.participant=v.participant;reservation.owner=v.owner;reservation.slot=v.slot;reservation.spell=*id;
        reservation.cost=XeenLearnedSpellRules::cost(*spell);reservation.originalSp=ch.currentSp;
        reservation.participants=d.participants;reservation.acted=d.acted;reservation.blocked=d.blocked;
        std::optional<XeenMagicArrowCandidate> arrow;
        if(*spell==XeenLearnedSpell::MagicArrow) {
            require(v.enemy && std::find(d.contact.begin(),d.contact.end(),v.enemy)!=d.contact.end(),"Arrow target left contact");
            const auto &a=d.actors.at(v.enemy->recordIndex);
            require(a.id==*v.enemy && a.hp>0 && a.statistics && a.x==d.camera.x && a.y==d.camera.y &&
                a.lifecycle==XeenActorLifecycle::Present && a.status==XeenActorStatus::Physical,"Arrow target changed");
            // The retained Journey admission owns the full original profile;
            // validateCombat is the narrower historical Diagnostic27 validator.
            arrow.emplace(ch.permanentLevel,ch.temporaryLevel,a.statistics->magicResistance(),a.original.resourceId);
            reservation.enemy=a;
        }
        prepared=d.prepareCastGuard();
        const auto after=std::int64_t(reservation.originalSp)-reservation.cost;
        require(after>=std::numeric_limits<std::int16_t>::min() && after<=std::numeric_limits<std::int16_t>::max(),"Cast SP overflow");
        if(prepare) {
            XeenRestoreGuard::Providers providers(*prepared,d.world);
            prepared->admitLearnedSpellNames(prepare());
        }
        d.checkCast(t);d.probeFor(t);d.checkCastReservation(t,reservation,false);prepared->check();
        prepared->characters[reservation.owner].currentSp=std::int16_t(after);
        c.reservation=std::move(reservation);const auto &reserved=*c.reservation;
        c.random=d.rng;c.arrow=std::move(arrow);
        v.result.spell=reserved.spell;v.result.spBefore=reserved.originalSp;v.result.spAfter=int(after);
        d.party.roster.at(reserved.owner).currentSp=d.expected.roster.at(reserved.owner).currentSp=std::int16_t(after);
        v.committed=true;v.phase=*spell==XeenLearnedSpell::FirstAid ? CP::PartyTarget : CP::Preparing;
        d.work=v.phase==CP::Preparing ? Work::Cast : Work::None;
        d.published();prepared->adoptJourneyCoordination();c.guard.swap(prepared);
        auto r=d.observation(Status::Pending);r.operation=Operation::Cast;r.actingOwner=std::uint8_t(reserved.owner);r.participant=int(reserved.participant);
        if(reserved.enemy){r.monster=reserved.enemy->id;r.targetMonster=reserved.enemy->id;r.actorHpBefore=r.actorHpAfter=reserved.enemy->hp;}
        c.operation=r;
        return d.adopt(r,Status::Pending);
    }catch(...) {
        if(prepared && d.casting && !d.casting->view.committed && current(t) &&
                prepared->current() && d.casting->guard->current() && d.exact()) {
            try {
                d.casting->guard->retainResources(*prepared);
                d.casting->view.refusal="Preparation failed; Enter retries / Esc returns";
                ++d.generation;return d.adopt(d.observation(Status::Advanced));
            }catch(...) {}
        }
        return fail(t,!d.exact()?Failure::Integrity:Failure::Preparation);
    }
}
XeenCombatResult XeenCombat::settleCast(const Ticket &t,std::optional<unsigned> target) {
    auto &d=*impl;
    try {
        d.checkCast(t);auto &c=*d.casting;
        require(c.reservation.has_value(),"Missing paid cast reservation");
        const auto &reserved=*c.reservation;d.checkCastReservation(t,reserved,true);
        c.partyTarget=target;
        const auto targetOwner=target ? std::optional<std::uint8_t>{d.party.party.activeRosterIds().at(*target)} : std::nullopt;
        if(target)require(reserved.spell==26 && *target<6 && (reserved.participants&(1u<<*target)),"Cast recovery target changed");
        XeenCombatCastResult result;result.spell=reserved.spell;
        result.spBefore=reserved.originalSp;result.spAfter=reserved.originalSp-int(reserved.cost);
        const auto effect=reserved.spell==26 && target ? XeenLearnedSpellRules::prepareFirstAid(d.party,*target,d.party.encounterContext->year) :
            reserved.spell==1 ? XeenLearnedSpellRules::prepareAwaken(d.party) : XeenSpellPreparation{};
        auto prepared=d.prepareCastGuard();
        result.failed=effect.failed;
        if(result.spell==26 && !target) {
            prepared->characters[reserved.owner].currentSp=reserved.originalSp;
            result.refunded=true;result.spAfter=reserved.originalSp;
        }
        for(const auto &value:effect.effects) {
            const auto &before=d.party.roster.at(value.owner);
            auto &row=result.effects.at(result.count++);
            row={value.owner,before.currentHp,value.hp,before.conditions,value.conditions};
            result.noop=result.noop && row.beforeHp==row.afterHp && row.before==row.after;
            prepared->characters[value.owner].currentHp=value.hp;prepared->characters[value.owner].conditions=value.conditions;
        }
        if(effect.failed && target) {
            const auto &before=d.character(int(*target));
            result.effects[result.count++]={before.rosterId,before.currentHp,before.currentHp,before.conditions,before.conditions};
        }
        auto speeds=d.playerSpeeds;
        for(unsigned i=0;i<6;++i)speeds[i]=Rules::effectivePhysical(prepared->characters[kXeenCombatOwners[i]],d.inputs[i],Rules::PhysicalAttribute::Speed,{d.party.encounterContext->year});
        d.probeFor(t);d.checkCastReservation(t,reserved,true);
        require(c.partyTarget==target && (!target || d.party.party.activeRosterIds().at(*target)==*targetOwner),"Cast recovery reservation changed");
        if(result.refunded)d.party.roster.at(reserved.owner).currentSp=d.expected.roster.at(reserved.owner).currentSp=reserved.originalSp;
        for(const auto &value:effect.effects) {
            auto &live=d.party.roster.at(value.owner);auto &before=d.expected.roster.at(value.owner);
            live.currentHp=before.currentHp=value.hp;live.conditions=before.conditions=value.conditions;
        }
        d.acted[reserved.participant]=true;d.playerSpeeds=speeds;c.view.result=result;
        d.reserveCastSuccessor();c.view.phase=XeenCombatCastPhase::Result;
        d.published();prepared->adoptJourneyCoordination();c.guard.swap(prepared);
        return d.adopt(c.operation);
    }catch(...){return fail(t,!d.exact()?Failure::Integrity:Failure::Preparation);}
}
XeenCombatResult XeenCombat::serviceCast(const Ticket &t,std::uint64_t now) {
    auto &d=*impl;
    if(!current(t))return d.observation(Status::Stale);
    if(d.busy || !d.casting || d.phase!=Phase::Casting)return d.observation(Status::Refused);
    Busy busy(d.busy);
    try {
        d.checkCast(t);d.capacity();auto &c=*d.casting;
        const auto stage=c.view.phase;
        if(stage!=XeenCombatCastPhase::Preparing && stage!=XeenCombatCastPhase::Projectile &&
            stage!=XeenCombatCastPhase::Impact && stage!=XeenCombatCastPhase::PostImpact)return d.observation(Status::Refused);
        if(stage!=XeenCombatCastPhase::Preparing && !c.presented)return d.observation(Status::Refused);
        require(c.reservation.has_value(),"Missing paid cast reservation");
        const auto &reserved=*c.reservation;d.checkCastReservation(t,reserved,true);
        if(reserved.spell!=45)return settleCast(t,std::nullopt);
        require(c.arrow && reserved.enemy,"Missing bound Arrow candidate");
        const auto &bound=*reserved.enemy;
        XeenConsequenceDraw draw{c.random,64,[&]{d.probeFor(t);d.checkCastReservation(t,reserved,true);}};
        if(stage==XeenCombatCastPhase::Preparing && !c.arrowDone) {
            if(!c.arrow->service(draw)){++d.generation;return d.adopt(c.operation,Status::Pending);}
            c.impactRandom=c.random.continuation();
            if(!c.arrow->resisted && std::int64_t(bound.hp)-c.arrow->damage<=0 && bound.original.resourceId==6)
                c.drop.emplace(*d.party.monsterTreasure,bound.id.recordIndex);
            c.arrowDone=true;
        }
        if(stage==XeenCombatCastPhase::Preparing && c.drop && !c.drop->service(draw)){++d.generation;return d.adopt(c.operation,Status::Pending);}
        auto r=c.operation;r.actorHpBefore=bound.hp;r.damage=c.arrow->damage;
        r.actorHpAfter=std::max<std::int64_t>(std::int64_t(bound.hp)-r.damage,0);
        r.attackOutcome=r.damage ? AttackOutcome::HitPositiveDamage : AttackOutcome::HitZeroDamage;
        auto actor=bound;actor.hp=r.actorHpAfter;
        std::optional<XeenJourneyLethal> lethal;std::set<XeenMonsterIdentity> expectedAccounting;
        if(!r.actorHpAfter) {
            std::array<const XeenCharacter *,6> owners{};for(unsigned i=0;i<6;++i)owners[i]=&d.character(i);
            lethal=xeenPrepareJourneyLethal(bound,owners,d.inputs,d.accounted,reserved.participants);
            actor=lethal->actor;expectedAccounting=lethal->accounted;
        }
        if(stage==XeenCombatCastPhase::Preparing || (stage==XeenCombatCastPhase::Projectile && r.damage)) {
            d.probeFor(t);d.checkCastReservation(t,reserved,true);
            c.view.phase=stage==XeenCombatCastPhase::Preparing ? XeenCombatCastPhase::Projectile :
                r.damage ? XeenCombatCastPhase::Impact : XeenCombatCastPhase::PostImpact;
            c.presented=false;c.operation=r;++d.generation;return d.adopt(r,Status::Pending);
        }
        if(stage==XeenCombatCastPhase::Impact) {
            auto prepared=d.prepareCastGuard();
            auto &values=d.indoor() ? *prepared->s._vertigoActors : prepared->s._actors;
            values.at(bound.id.recordIndex).hp=r.actorHpAfter;
            prepared->s._journeyRandom=c.impactRandom;
            d.probeFor(t);d.checkCastReservation(t,reserved,true);
            d.activeActors().at(bound.id.recordIndex).hp=d.actors.at(bound.id.recordIndex).hp=r.actorHpAfter;
            d.expectedRandom=d.session()._journeyRandom=c.impactRandom;
            d.rng=XeenCombatRandom(*c.impactRandom);
            c.view.phase=XeenCombatCastPhase::PostImpact;c.presented=false;c.operation=r;
            d.published();prepared->adoptJourneyCoordination();c.guard.swap(prepared);
            return d.adopt(r,Status::Pending);
        }
        if(lethal) {
            for(unsigned i=0;i<6;++i)if(lethal->experience[i]!=d.inputs[i].experience)
                r.xp.at(r.xpCount++)={kXeenCombatOwners[i],d.inputs[i].experience,lethal->experience[i]};
        }
        auto prepared=d.prepareCastGuard();
        auto &preparedActors=d.indoor() ? *prepared->s._vertigoActors : prepared->s._actors;
        preparedActors.at(actor.id.recordIndex)=actor;
        const auto cursor=c.random.continuation();prepared->s._journeyRandom=cursor;
        if(lethal){prepared->s._accountedMonsters=expectedAccounting;for(unsigned i=0;i<6;++i)prepared->inputs[kXeenCombatOwners[i]]->experience=lethal->experience[i];}
        if(c.drop){prepared->treasure=c.drop->treasure;r.monsterDrop=c.drop->outcome;r.generatedItem=c.drop->generated;r.generatedArmor=c.drop->armor;}
        d.probeFor(t);d.checkCastReservation(t,reserved,true);
        d.activeActors().at(actor.id.recordIndex)=actor;d.actors.at(actor.id.recordIndex)=actor;
        if(lethal) {
            d.session()._accountedMonsters.swap(lethal->accounted);d.accounted.swap(expectedAccounting);d.episodeLethal=true;
            for(unsigned i=0;i<6;++i){const auto id=kXeenCombatOwners[i];d.inputs[i].experience=lethal->experience[i];d.party.roster._combatInputs[id]->experience=d.allInputs[id]->experience=lethal->experience[i];}
            if(c.drop)d.party.monsterTreasure=d.expected.treasure=c.drop->treasure;
        }
        d.rng=c.random;d.expectedRandom=d.session()._journeyRandom=cursor;
        d.acted[reserved.participant]=true;c.view.result.resisted=c.arrow->resisted;c.view.result.noop=c.arrow->resisted;
        d.reserveCastSuccessor();c.view.phase=XeenCombatCastPhase::Result;
        d.published();prepared->adoptJourneyCoordination();c.guard.swap(prepared);
        c.operation=r;return d.adopt(r);
    }catch(...){return fail(t,!d.exact()?Failure::Integrity:Failure::Preparation);}
}
void XeenCombat::castPresented(const Ticket &t,std::uint64_t now) {
    auto &d=*impl;if(!d.casting)return;
    d.checkCast(t);
    if((d.casting->view.phase==XeenCombatCastPhase::Projectile ||
        d.casting->view.phase==XeenCombatCastPhase::Impact || d.casting->view.phase==XeenCombatCastPhase::PostImpact) && !d.casting->presented) {
        require(now<=std::numeric_limits<std::uint64_t>::max()-100,"Cast cosmetic clock exhausted");
        d.casting->deadline=now+100;d.casting->presented=true;
    }
}
std::optional<XeenActor> XeenCombat::castImpactSnapshot() const {
    const auto &c=impl->casting;
    return c && c->view.phase==XeenCombatCastPhase::PostImpact && c->reservation ? c->reservation->enemy : std::nullopt;
}
void XeenCombat::rangedPresented(const Ticket &t,bool travelComplete) {
    auto &d=*impl;require(current(t) && d.exact(),"Stale ranged frame acknowledgment");
    if(!d.consequences || !d.consequences->movement)return;
    auto &op=*d.consequences->movement;
    if(op.travelStarted && travelComplete)op.travelPresented=true;
    if(op.impactOwner)op.impactPresented=true;
}

// Contract 4 uses the same combat authority, initiative and publication boundary.
// Each retained value below is an unpublished bounded operation, never a live owner.
XeenCombatResult XeenCombat::serviceConsequences(const Ticket &t) {
	auto &d=*impl;Busy busy(d.busy);
	try {
		d.capacity();d.resourcesFor(t);
		const bool end=d.work==Work::End;
		if ((end || d.work==Work::Round) && !d.consequences) {
			if (!end && !d.moveDue) {
				bool awake=false;for(unsigned i=0;i<6;++i) awake=awake || ((d.participants&(1u<<i)) && d.character(i).canAct() && d.playerSpeeds[i]>0);
				d.acted.fill(false);
				// Interface::nextChar inner cycle retains Block and owes no round.
				if (!awake) {
					d.refreshSpeeds();d.reconcile();d.sortOrder();d.selectNext();++d.generation;
					return d.adopt(d.observation(Status::Pending),Status::Pending);
				}
				d.blocked.fill(false);d.moveDue=true;d.chargeRound=true;
				d.refreshSpeeds();d.reconcile();d.sortOrder();d.selectNext();
				if(d.work==Work::Enemy) { ++d.generation;return d.adopt(d.observation(Status::Pending),Status::Pending); }
			}
			if(end) require(!d.contact[0] && !d.moveDue && d.episodeLethal,"End requires an actual lethal and no owed work");
			if(end || d.chargeRound) {
				const auto time=xeenPrepareTime(*d.party.encounterContext,1);
				if(time.dusks || time.dawns || time.dailyProcessing || time.midnights || time.yearRollovers) return fail(t,Failure::Time);
			}
			d.consequences.emplace();auto &c=*d.consequences;c.rng=d.rng;
			c.result=d.observation(Status::Pending);c.result.oldRevision=d.revision();c.result.operation=end?Operation::End:Operation::Round;
			if(!end) {
				if(d.indoor())c.movement.emplace(d.world,d.actors,d.camera,d.characters(),d.inputs,d.party.encounterContext->year,d.participants,d.blocked);
				else {const auto map=d.world.map(23);c.movement.emplace(map,d.actors,d.camera,d.characters(),d.inputs,d.party.encounterContext->year,d.participants,d.blocked);}
				c.movement->staged=true;
				d.probeFor(t);
			} else c.time.emplace(*d.party.encounterContext,1,d.characters(),d.inputs);
		}
		if(!d.consequences) {
			require(d.work==Work::Enemy,"Missing physical continuation");
			d.consequences.emplace();auto &c=*d.consequences;c.rng=d.rng;
			c.result=d.observation(Status::Pending);c.result.oldRevision=d.revision();c.result.participant=d.turn;
			c.result.operation=Operation::EnemyAttack;c.result.actingMonster=d.contact.at(d.turn-6);c.result.monster=*c.result.actingMonster;
			c.enemy.emplace(d.characters(),d.inputs,*d.actors.at(c.result.monster.recordIndex).statistics,d.party.encounterContext->year,d.participants,d.blocked);
		}
		auto &c=*d.consequences;
		XeenConsequenceDraw draw{c.rng,64,[&]{d.probeFor(t);}};
		auto pending=[&] { ++d.generation;return d.adopt(c.result,Status::Pending); };
		if(c.movement) {
			if(!c.physicalDone) {
				if(!c.movement->service(draw)) {
                    auto &op=*c.movement;auto r=c.result;
                    if((!op.travelPresented && !op.travelPublished) || (op.impactOwner && !op.portraitPublished))r.ranged=op.presentation();
                    if((!op.travelPresented && !op.travelPublished) || op.impactApplied) {
                        auto liveActors=op.actors,expectedActors=op.actors;d.probeFor(t);
                        d.activeActors().swap(liveActors);d.actors.swap(expectedActors);
                        if(op.impactApplied)d.publishCharacters(op.characters);
                        d.rng=c.rng;d.expectedRandom=d.session()._journeyRandom=c.rng.continuation();
                        d.published();
                        if(!op.travelPresented)op.travelPublished=true;
                    }else ++d.generation;
                    if(op.impactOwner && r.ranged)op.portraitPublished=true;
                    return d.adopt(r,Status::Pending);
                }
				c.physicalDone=true;
				if(d.chargeRound) c.time.emplace(*d.party.encounterContext,1,c.movement->characters,d.inputs);
			}
		}
		if(c.run && !c.run->service(draw)) return pending();
		if(c.time && !c.time->service(draw)) return pending();
		if(c.player && !c.physicalDone) {
			if(!c.player->service(draw)) return pending();
			c.result.damage=c.player->damage;c.result.actorHpAfter=std::max(c.result.actorHpBefore-c.player->damage,0);
			c.result.attackOutcome=!c.player->hit?AttackOutcome::Miss:c.player->damage?AttackOutcome::HitPositiveDamage:AttackOutcome::HitZeroDamage;
			if(!c.result.actorHpAfter && d.actors.at(c.result.monster.recordIndex).original.resourceId==6)
				c.drop.emplace(*d.party.monsterTreasure,c.result.monster.recordIndex);
			c.physicalDone=true;
		}
		if(c.drop && !c.drop->service(draw)) return pending();
		if(c.enemy && !c.enemy->service(draw)) return pending();
		auto r=c.result;
		if(c.run) { r.runRoll=c.run->roll;r.runSuccess=c.run->success;r.participantsAfter=r.runSuccess ? d.participants & ~(1u<<d.turn) : d.participants; }
		if(c.drop){r.monsterDrop=c.drop->outcome;r.generatedItem=c.drop->generated;r.generatedArmor=c.drop->armor;}
		std::optional<XeenJourneyLethal> lethal;std::set<XeenMonsterIdentity> expectedAccounting;
		if(c.player && !r.actorHpAfter) {
			std::array<const XeenCharacter *,6> owners{};for(unsigned i=0;i<6;++i) owners[i]=&d.character(i);
			lethal=xeenPrepareJourneyLethal(d.actors.at(r.monster.recordIndex),owners,d.inputs,d.accounted,d.participants);
			expectedAccounting=lethal->accounted;
			for(unsigned i=0;i<6;++i) if(lethal->experience[i]!=d.inputs[i].experience)
				r.xp.at(r.xpCount++)={kXeenCombatOwners[i],d.inputs[i].experience,lethal->experience[i]};
		}
		std::vector<XeenActor> expectedActors;
		if(c.movement) {
			expectedActors=c.movement->actors;auto observation=std::make_shared<XeenRegionalObservation>();
			observation->count=c.movement->shotCount;observation->after=c.time?c.time->characters:c.movement->characters;
			for(unsigned i=0;i<observation->count;++i) observation->shots[i]=c.movement->shots[i];
			r.ranged=std::move(observation);
		}
		if(c.enemy) {
			const auto &a=c.enemy->result;r.damage=a.damage;r.injuries=a.injuries;r.additionalInjuries=a.additionalInjuries;r.injuryCount=a.injuryCount;
			r.armor=a.armor;r.armorCount=a.armorCount;r.attackOutcome=a.attackOutcome;r.targetOwner=a.targetOwner;r.critical=a.critical;r.targetedMembers=a.targetedMembers;
		}
		// All allocations, draws and provider callbacks precede the atomic stores.
		d.probeFor(t);
		if(c.run) {
			d.participants=r.participantsAfter;d.acted[d.turn]=true;
			if(r.runSuccess && !d.continuingParticipants()) d.exitCause=XeenCombatExitCause::DirectRun;
		}
		if(c.player) {
			auto &actor=d.activeActors().at(r.monster.recordIndex);actor.hp=r.actorHpAfter;
			if(lethal) {
				actor=lethal->actor;d.session()._accountedMonsters.swap(lethal->accounted);d.accounted.swap(expectedAccounting);d.episodeLethal=true;
				for(unsigned i=0;i<6;++i) {
					const auto id=kXeenCombatOwners[i];d.inputs[i].experience=lethal->experience[i];
					d.party.roster._combatInputs[id]->experience=d.allInputs[id]->experience=lethal->experience[i];
				}
				if(c.drop) d.party.monsterTreasure=d.expected.treasure=c.drop->treasure;
			}
			d.actors.at(r.monster.recordIndex)=actor;d.acted[d.turn]=true;
		}
		if(c.enemy) d.publishCharacters(c.enemy->characters);
		if(c.movement) { d.activeActors().swap(c.movement->actors);d.actors.swap(expectedActors);d.publishCharacters(c.movement->characters); }
		if(c.time) { d.publishCharacters(c.time->characters);d.party.encounterContext=d.expected.encounterContext=c.time->context; }
		d.rng=c.rng;d.expectedRandom=d.session()._journeyRandom=d.rng.continuation();
		const auto operation=r.operation;
		const int selected=d.turn;
		const bool nextEnemy=c.enemy && c.enemy->nextAttack();
        if(c.movement) {d.moveDue=d.chargeRound=false;d.countdown.clear();}
		if(nextEnemy) {c.result=d.observation(Status::Pending);c.result.oldRevision=d.revision()+1;c.result.operation=Operation::EnemyAttack;c.result.actingMonster=r.actingMonster;c.result.monster=r.monster;}
        else d.consequences.reset();
        d.refreshSpeeds();d.reconcile();d.sortOrder();
		if(nextEnemy) {d.phase=Phase::PendingEnemy;d.work=Work::Enemy;}
		else if(d.defeated()) { d.phase=Phase::Defeat;d.work=Work::None; }
		else if(end) { d.phase=Phase::Victory;d.work=Work::None;d.ended=true; }
		else if(d.disengagementDue()) {}
		else {
			if(operation==Operation::EnemyAttack) d.acted[d.turn]=true;
			if((operation==Operation::Round || operation==Operation::Rotate || operation==Operation::Movement) && d.contact[0] && selected>=0 && selected<6 && (d.participants&(1u<<selected)) && d.character(selected).canAct() && d.playerSpeeds[selected]>0) {
				d.turn=selected;d.phase=Phase::PlayerReady;d.work=Work::None;
			} else d.selectNext();
		}
        if(operation==Operation::Rotate)d.stepped=true;
		d.published();return d.adopt(r,d.phase==Phase::Defeat?Status::Defeat:end?Status::Victory:Status::Advanced);
	} catch(...) { d.clearResourceCheck();return fail(t,!d.exact()?Failure::Integrity:Failure::Preparation); }
}

XeenCombatResult XeenCombat::finishDisengagement(const Ticket &t) {
 auto &d=*impl;Busy busy(d.busy);
 try {
  d.capacity();d.resourcesFor(t);
	  require(d.journey && d.phase==Phase::DisengagementPending &&
   d.work==Work::FinishDisengagement && !d.consequences && !d.moveDue && !d.chargeRound &&
   d.contact[0] && !d.continuingParticipants() && !d.defeated() && d.participants!=0x3f &&
   d.exitCause!=XeenCombatExitCause::None,"Incomplete disengagement obligations");
  const auto map=d.world.map(d.camera.mapId);d.probeFor(t);
  const auto &g=map.geometry;
  require(d.camera.mapId==XeenMapIdentity(23) && g.runX==10 && g.runY==12 && g.difficulties[7]==100,
   "Original Run metadata changed");
  XeenCamera destination=d.camera;destination.x=g.runX;destination.y=g.runY;
  require(XeenMovement::component(map,9,11,xeenJourneyContent().traversal)[destination.y*16+destination.x] &&
   !xeenRegionalEvent(d.events,destination) && !hasAutomaticTrigger(g,destination.x,destination.y),
   "Original Run destination is incompatible");
  auto characters=d.characters();auto r=d.observation(Status::Advanced);
  r.oldRevision=d.revision();r.operation=Operation::FinishDisengagement;r.exitCause=d.exitCause;
  r.origin=XeenCombatLocation{d.camera.mapId,d.camera.x,d.camera.y,d.camera.direction};
  r.destination=XeenCombatLocation{destination.mapId,destination.x,destination.y,destination.direction};
  r.originAutomaticSuperseded=hasAutomaticTrigger(g,d.camera.x,d.camera.y) && xeenRegionalEvent(d.events,d.camera).has_value();
  for(unsigned i=0;i<6;++i) if(d.participants&(1u<<i)) {
   const auto condition=characters[i].worstCondition();
   if(condition==XeenCondition::Asleep || condition==XeenCondition::Paralyzed ||
    condition==XeenCondition::Unconscious || condition==XeenCondition::Stoned || condition==XeenCondition::Eradicated) {
    characters[i].conditions[13]=1;r.casualties|=1u<<i;
   }
  }
  auto treasure=*d.party.monsterTreasure;
  if(d.exitCause==XeenCombatExitCause::DirectRun) {
   r.forfeitedGold=treasure.pendingGold;r.forfeitedMask=treasure.pendingMask;treasure=xeenPrepareMonsterGoldForfeiture(treasure);
  }
  // Validation only: this detached party is never assigned to a live owner.
  XeenPartyState candidate;
  for(unsigned id=0;id<30;++id) candidate.roster.at(id)=d.party.roster.at(id);
  for(unsigned id=0;id<30;++id)candidate.roster._combatInputs[id]=d.allInputs[id];
  candidate.roster._combatMarked=true;
  candidate.party=d.party.party;candidate.encounterContext=d.party.encounterContext;
  candidate.questItems=d.party.questItems;candidate.questFlags=d.party.questFlags;
  candidate.regionalRecovery=d.party.regionalRecovery;
  candidate.serviceEconomy=d.party.serviceEconomy;
  candidate.firstSerializedCount=d.party.firstSerializedCount;candidate.effectiveSerializedCount=d.party.effectiveSerializedCount;
  for(const auto &c:characters) candidate.roster.at(c.rosterId).conditions=c.conditions;
  candidate.monsterTreasure=treasure;xeenValidateJourneyParty(candidate);
  auto actors=d.actors;const auto view=XeenActorApproach::classify(actors,destination);
  for(unsigned i=0;i<actors.size();++i) actors[i].activated=actors[i].activated || view.activation[i];
  auto expectedActors=actors;
  d.probeFor(t);
  // No callbacks or fallible owner assignment after publication begins.
  d.publishCharacters(characters);d.party.monsterTreasure=d.expected.treasure=treasure;
  d.camera.x=d.expectedCamera.x=destination.x;d.camera.y=d.expectedCamera.y=destination.y;
  d.activeActors().swap(actors);d.actors.swap(expectedActors);d.destinationView=view;
  d.finishedDisengagement=true;d.phase=Phase::Disengaged;d.work=Work::None;d.published();
  return d.adopt(r);
 } catch(...) { d.clearResourceCheck();return fail(t,!d.exact()?Failure::Integrity:Failure::Preparation); }
}
void XeenCombat::retireDisengagedJourney(const Ticket &t,XeenEncounterState &state) {
 auto &d=*impl;
	 require(current(t) && d.journey && !d.busy && d.phase==Phase::Disengaged &&
  d.finishedDisengagement && d.work==Work::None && !d.consequences &&
  !d.moveDue && !d.chargeRound && d.boundary.quiet(),"Disengagement retirement requires successful finish");
 if(!d.exact()) { fail(t,Failure::Integrity);throw IntegrityError("Disengagement retirement preimage changed"); }
 d.capacity();
 auto &s=d.session();const bool engaged=d.destinationView.engaged();
 s._journeyActivity=engaged?XeenJourneyActivity::Attachment:XeenJourneyActivity::Presentation;
 s._combatOwner=nullptr;s._combatApproachState=&state;s._combatEntered=false;s._encounterTerminal=engaged;
 d.finishedDisengagement=false;d.published();
 state._world=&d.world;state._party=&d.party;state._camera=&d.camera;
 state._revision=s._encounterRevision;state._pending=0;
 state._phase=engaged?XeenEncounterPhase::Engaged:XeenEncounterPhase::Exploring;state._reason=XeenEncounterStop::None;
 d.world._combatCheck={};d.world._combatAuthorized={};d.borrow.reset();
}

void XeenCombat::retireJourney(const Ticket &t, XeenEncounterState &state) {
	auto &d = *impl;
	require(current(t) && d.journey && !d.busy && d.phase == Phase::Victory && d.ended &&
		d.work == Work::None && !d.consequences && d.boundary.quiet(), "Journey retirement requires successful quiescent End");
	if (!d.exact()) { fail(t,Failure::Integrity); throw IntegrityError("Journey retirement preimage changed"); }
	d.capacity();
	require(!d.contact[0] && !d.moveDue && (d.episodeLethal),
		"Journey retirement requires published lethal accounting");
	// No callbacks, allocation or gameplay mutation after this point.
	auto &s = d.session();
	s._journeyActivity = XeenJourneyActivity::Presentation;
	s._combatOwner = nullptr; s._combatApproachState = &state;
	s._combatEntered = false; s._encounterTerminal = false;
	d.ended = false; d.published();
	state._world = &d.world; state._party = &d.party; state._camera = &d.camera;
	state._revision = s._encounterRevision; state._pending = 0;
	state._phase = XeenEncounterPhase::Exploring; state._reason = XeenEncounterStop::None;
	d.world._combatCheck = {}; d.world._combatAuthorized = {};
	d.borrow.reset();
}

static_assert(std::is_nothrow_copy_assignable_v<XeenCombatResult>);
static_assert(std::is_nothrow_copy_assignable_v<XeenActor>);
} // namespace mmodern
