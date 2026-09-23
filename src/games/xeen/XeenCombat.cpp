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
		bool(a.luck)==bool(b.luck)&&(!a.luck||same(*a.luck,*b.luck))&&a.resistances==b.resistances;
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
		a.hasSpells==b.hasSpells&&same(a.weapons,b.weapons)&&same(a.armor,b.armor)&&same(a.accessories,b.accessories)&&
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
int checked(std::int64_t v) { require(v>=0&&v<=std::numeric_limits<int>::max(),"combat arithmetic overflow"); return int(v); }
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
		std::uint8_t firstSerializedCount, effectiveSerializedCount;
		std::vector<std::string> diagnostics;
		explicit PartyPreimage(const XeenPartyState &p) : roster{p.roster.characters()}, party(p.party),
			questItems(p.questItems), questFlags(p.questFlags), recovery(p.regionalRecovery), encounterContext(p.encounterContext), treasure(p.monsterTreasure),
			firstSerializedCount(p.firstSerializedCount), effectiveSerializedCount(p.effectiveSerializedCount), diagnostics(p.diagnostics) {}
	} expected;
	bool journey = false, ended = false, episodeLethal = false;
	std::uint32_t journeySeed = 0;
	std::uint16_t contract=1;
	std::optional<XeenJourneyRandomState> expectedRandom;
	bool moveDue=false, chargeRound=false;
	std::uint8_t participants=0x3f;
	XeenCombatExitCause exitCause=XeenCombatExitCause::None;
	bool finishedDisengagement=false;
	XeenActorView destinationView;
	unsigned enemyOrdinal=0;
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
	std::set<XeenObjectIdentity> objects;
	std::set<XeenEventIdentity> removedEvents;
	std::vector<XeenMonsterRecord> statistics;
	XeenEventFile events;
	XeenEncounterState approach;
	XeenCombatRandom rng;
	std::function<void()> probe;
	Phase phase=Phase::Preparation; Work work=Work::None;
	std::uint64_t generation=1;
	bool busy=false,attached=false;
	std::array<int,9> order{};
	std::array<bool,9> acted{};
	std::array<bool,6> blocked{};
	int turn=-1;
	XeenCombatResult last;
	std::optional<XeenEquipmentResult> equipmentResult;
	std::optional<XeenTransferResult> transferResult;
	enum class Step { Weapon, PlayerHit, Target, Fallback, EnemyRoll, FirstDice, HitParameter, SecondDice, Disease, Done };
	struct Candidate {
		XeenCombatRandom rng;
		std::uint64_t boundary=0;
		Step step=Step::Weapon, afterDisease=Step::Done;
		int target=-1,slot=0,dice=0,sides=0,weapon=0,hit=0,damage=0,roll=0,applications=0;
		unsigned attacks=1;
		int hitBase=0,accumulated=0;
		// One target's candidate, not an authoritative combat party.
		std::optional<XeenCharacter> injured;
		XeenCombatResult result;
	};
	std::optional<Candidate> candidate;
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
	std::uint64_t revision() const { return journey && lifetime && !lifetime->worldAlive() ? 0 : world._sessionState._encounterRevision; }
	bool exact() const {
		if (journey && (!lifetime || !lifetime->ownersAlive() || !lifetime->cachesCurrent() || flags->values() != expectedFlags ||
			!world.sessionState().journey() || world.sessionState().skeletonSeed() != journeySeed ||
			world.sessionState().journeyContract()!=contract || world.sessionState().journeyRandom()!=expectedRandom ||
			world._sessionState._journeyOwner != journeyOwner || world._sessionState._journeyGeneration != journeyGeneration ||
			!world._sessionState._encounterInitialized || !world._sessionState._encounterMarked ||
			!world._sessionState._encounterTerminal || world._sessionState._combatApproachState != &approach ||
			world._sessionState._combatEntered != (phase != Phase::Engaged) || world._sessionState._combatAccounted ||
			world._sessionState._diagnostic27 || world._sessionState._completion != XeenEncounterCompletion::None ||
			world.sessionState().accountedMonsters() != accounted || world.sessionState().journeyActivity() != XeenJourneyActivity::Combat)) return false;
		if(!same(camera,expectedCamera)||party.monsterTreasure!=expected.treasure||party.party.activeRosterIds()!=expected.party.activeRosterIds()||
			party.questItems.counts()!=expected.questItems.counts()||party.questFlags.values()!=expected.questFlags.values()||party.regionalRecovery!=expected.recovery||
			party.firstSerializedCount!=expected.firstSerializedCount||party.effectiveSerializedCount!=expected.effectiveSerializedCount||
			party.diagnostics!=expected.diagnostics||bool(party.encounterContext)!=bool(expected.encounterContext)) return false;
		if(party.encounterContext&&!same(*party.encounterContext,*expected.encounterContext)) return false;
		for(unsigned i=0;i<30;++i) if(!same(party.roster.at(i),expected.roster.at(i))) return false;
		if(!party.roster.combatMarked()||(!world._sessionState._diagnostic27 && !journey)||world._sessionState._combatOwner!=owner) return false;
		for(unsigned i=0;i<30;++i) {
			const auto pos=std::find(kXeenCombatOwners.begin(),kXeenCombatOwners.end(),i);
			const auto &v=party.roster.combatInputs(i);
			if (journey) { if (!v || !allInputs[i] || !same(*v,*allInputs[i])) return false; continue; }
			if(pos==kXeenCombatOwners.end() || !attached) { if(v) return false; }
			else if(!v||!same(*v,inputs[pos-kXeenCombatOwners.begin()])) return false;
		}
		const auto &s=world.sessionState();
		if(s.disabledObjects()!=objects||s.disabledEvents()!=removedEvents||s.actors().size()!=actors.size()) return false;
		for(unsigned i=0;i<actors.size();++i) if(!same(actors[i],s.actors()[i])) return false;
		return true;
	}
	XeenCharacter &character(int participant) { return party.roster.at(kXeenCombatOwners.at(participant)); }
	int armor(const XeenCharacter &c,int participant) { return Rules::combatArmorClass(c,inputs.at(participant),{party.encounterContext->year}); }
	void reconcile() noexcept {
		const auto old=contact; const auto oldActed=acted;
		const auto currentEnemy=turn>=6 ? contact[turn-6] : std::optional<XeenMonsterIdentity>{};
		contact.fill({}); unsigned count=0;
		for (const auto &a:session()._actors) if (a.x==camera.x && a.y==camera.y && a.hp>0 && a.lifecycle==XeenActorLifecycle::Present && a.status==XeenActorStatus::Physical && count<3) contact[count++]=a.id;
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
		for(int i=6;i<9;++i) speeds[i]=contact[i-6] ? session()._actors[contact[i-6]->recordIndex].statistics->speed() : 0;
		for(int i=0;i<9;++i) order[i]=i;
		std::sort(order.begin(),order.end(),[&](int a,int b){return speeds[a]!=speeds[b]?speeds[a]>speeds[b]:a<b;});
	}
	bool continuingParticipants() {
		for(unsigned i=0;i<6;++i) if((participants & (1u<<i)) && xeenCombatTargetable(character(i))) return true;
		return false;
	}
	bool disengagementDue() noexcept {
		if(!xeenJourneyContent(contract).disengagement() || !contact[0] || continuingParticipants()) return false;
		if(participants==0x3f) { phase=Phase::Failed;work=Work::None;return true; }
		if(exitCause==XeenCombatExitCause::None) exitCause=XeenCombatExitCause::AttritionAfterEscape;
		turn=-1;phase=Phase::DisengagementPending;work=moveDue?Work::Round:Work::FinishDisengagement;return true;
	}
	void selectNext() noexcept {
		if(xeenJourneyContent(contract).disengagement() && defeated()) { turn=-1;phase=Phase::Defeat;work=Work::None;return; }
		if(disengagementDue()) return;
		if (!contact[0] && !moveDue) { turn=-1; phase=Phase::VictoryAwaitingEnd; work=Work::End; return; }
		for(auto i:order) if(!acted[i] && (i>=6 ? bool(contact[i-6]) : ((participants & (1u<<i)) && character(i).canAct() && (!xeenJourneyContent(contract).consequences() || playerSpeeds[i]>0)))) {
			turn=i; phase=i>=6?Phase::PendingEnemy:moveDue?Phase::PendingRound:Phase::PlayerReady;
			work=i>=6?Work::Enemy:moveDue?Work::Round:Work::None; return;
		}
		turn=-1; phase=Phase::PendingRound; work=Work::Round;
	}
	bool defeated() { for(int i=0;i<6;++i) if(xeenJourneyContent(contract).consequences() ? xeenCombatTargetable(character(i)) : character(i).canAct()) return false; return true; }
	void terrainAdmission() {
		// Immutable approach admission separated from Good-only character admission.
		if (contract>=2) { XeenActorApproach::validateEnvironment(world,actors,events,contract); return; }
		const auto geometry=world.map(20).geometry;
		require(geometry.isOutdoors()&&geometry.flags==0,"combat terrain flags changed");
		for(int y=1;y<=2;++y)for(int x=13;x<=14;++x){const auto &cell=geometry.cells[y*16+x];
			const auto *layer=std::get_if<XeenOutdoorLayers>(&cell.geometry);
			require(layer&&layer->surface<16&&geometry.surfaceTypes[layer->surface]==1&&layer->middle==3&&
				cell.rawWord==0x31&&cell.rawAttributes==0&&cell.flags==0,"combat envelope terrain changed");
		}
		const auto mob=world.objectFile(20);
		require(mob.resourcePresent&&mob.entities.monsters.size()==actors.size(),"combat original identities changed");
		for(unsigned i=0;i<actors.size();++i){const auto &a=actors[i].original,&b=mob.entities.monsters[i];
			require(a.x==b.x&&a.y==b.y&&a.direction==b.direction&&a.tableIndex==b.tableIndex&&a.resourceId==b.resourceId,"combat original metadata changed");}
	}
	void probeFor(const Ticket &t) {
		if(probe) probe();
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
		if (journey && lifetime && !lifetime->ownersAlive()) return r;
		r.participant=turn;r.minutes=party.encounterContext?party.encounterContext->minutes:480; return r;
	}
	XeenCombatResult adopt(XeenCombatResult r,Status status=Status::Advanced) noexcept {
		r.status=status;r.phase=phase;r.work=work;r.revision=revision();r.generation=generation;
		r.minutes=party.encounterContext?party.encounterContext->minutes:480;r.participantsAfter=participants;r.exitCause=exitCause;last=r;return r;
	}
	void capacity() { require(revision()<std::numeric_limits<std::uint64_t>::max()&&generation<std::numeric_limits<std::uint64_t>::max()-2,"combat revision exhausted"); }
	void published() noexcept { ++session()._encounterRevision;++generation; }
	void latchIntegrity() noexcept {
		session()._completedIntegrityUnsafe=true;
		if(revision()!=std::numeric_limits<std::uint64_t>::max())++session()._encounterRevision;
		if(generation!=std::numeric_limits<std::uint64_t>::max())++generation;
	}
	void injury(Candidate &c) {
		auto &ch=*c.injured; auto &r=c.result;
		XeenCombatDamage d;d.owner=ch.rosterId;d.amount=c.damage;d.beforeHp=ch.currentHp;d.beforeAc=armor(ch,c.target);
		xeenApplyPhysicalInjury(ch,c.damage,party.encounterContext->year);
		d.afterHp=ch.currentHp;d.afterAc=armor(ch,c.target);d.conditions=ch.conditions;
		r.injuries.at(r.injuryCount++)=d;c.damage=0;c.dice=0;++c.applications;
	}
};

void xeenValidateInitialCombatParty(const XeenPartyState &p,const std::vector<std::uint8_t> &chr,const std::array<XeenCombatInputs,6> &inputs) {
	require(!p.monsterTreasure,"Legacy initial combat cannot own monster treasure");
	const auto parsed=XeenCharacterFormat::parseRoster(chr);
	for(unsigned i=0;i<30;++i) require(same(p.roster.at(i),parsed.at(i)),"party does not match initial CHR");
	require(p.party.activeRosterIds()==std::vector<std::uint8_t>(kXeenCombatOwners.begin(),kXeenCombatOwners.end())&&
		p.firstSerializedCount==6&&p.effectiveSerializedCount==6,"combat requires original ordered membership");
	constexpr int might[]{17,19,15,14,12,8},speed[]{16,16,15,15,14,14},accuracy[]{15,16,12,18,13,15};
	constexpr int hp[]{12,16,12,10,7,5},sp[]{2,0,2,0,7,9},classes[]{1,0,9,5,3,4},weapons[]{6,2,8,12,15,7};
	for(int i=0;i<6;++i) {
		const auto &c=p.roster.at(kXeenCombatOwners[i]);const auto &v=inputs[i];
		require(v.might.permanent==might[i]&&v.speed.permanent==speed[i]&&v.accuracy.permanent==accuracy[i]&&
			v.might.temporary==0&&v.speed.temporary==0&&v.accuracy.temporary==0&&v.temporaryAc==0&&v.experience==0,
			"unsupported original combat inputs");
		require(c.currentHp==hp[i]&&c.currentSp==sp[i]&&int(c.characterClass)==classes[i]&&c.permanentLevel==1&&
			c.temporaryLevel==0&&c.temporaryAge==0&&c.birthYear==592&&c.conditions==std::array<std::uint8_t,16>{},"unsupported initial combat character");
		Rules::validateForUse(c,{610});require(Rules::maxHp(c,{610})==hp[i],"original maximum HP mismatch");
		using Record=std::array<unsigned,5>;std::vector<Record> expected,actual;
		auto add=[&](unsigned category,unsigned material,unsigned id,unsigned frame){expected.push_back({category,material,id,0,frame});};
		add(0,0,weapons[i],1);if(i==2)add(0,0,30,4);if(i==3)add(0,0,12,0);
		add(1,0,i==0?3:i==5?1:2,3);if(i==0)add(1,0,8,2);if(i==1)add(1,0,9,5);
		if(i<3)add(1,0,13,6);add(1,38,10,9);if(i==3)add(1,38,11,10);
		add(2,38,2,12);if(i==3)add(2,42,1,8);if(i==4)add(2,42,5,8);if(i==5)add(2,86,1,0);
		for(unsigned cat=0;cat<4;++cat) for(const auto &item:*xeenInventoryItems(c,static_cast<XeenInventoryCategory>(cat))) {
			if(item.id) actual.push_back({cat,item.material,item.id,item.state,item.frame});
			else require(item.material==0&&item.state==0&&item.frame==0,"changed original empty item metadata");
		}
		std::sort(expected.begin(),expected.end());std::sort(actual.begin(),actual.end());
		require(actual==expected,"original combat item multiset differs");
	}
}

XeenCombat::XeenCombat(XeenWorld &w,XeenPartyState &p,XeenCamera &c,XeenCombatBoundary &b,
		const std::vector<std::uint8_t> &chr,const XeenGameplayContext &ctx,const std::vector<XeenMonsterRecord> &stats,
		const XeenEventFile &events,XeenCombatRandom rng):impl(std::make_unique<Impl>(this,w,p,c,b,ctx,stats,events,std::move(rng))) {
	auto &d=*impl;auto &s=d.session();
	const bool reserved = s._entry == XeenEncounterEntry::Diagnostic27 && !s._encounterInitialized &&
		!s._encounterTerminal && s._actors.empty() && !s._combatOwner && !s._diagnostic27;
	require((!w.hasEncounterState() || reserved)&&!p.encounterContext&&!p.roster.combatMarked(),"Diagnostic27 requires fresh owners");
	s._entry=XeenEncounterEntry::Diagnostic27;
	s._encounterMarked=true;s._diagnostic27=true;s._combatOwner=this;s._combatApproachState=&d.approach;p.roster._combatMarked=true;
	require(b.world==&w&&b.party==&p&&b.camera==&c,"combat boundary owner mismatch");
	require(b.quiet()&&same(c,XeenActorApproach::kEntry),"combat preparation boundary is not quiescent");
	for(unsigned i=0;i<6;++i)d.inputs[i]=XeenCharacterFormat::parseCombatInputs(chr,kXeenCombatOwners[i]);
	xeenValidateInitialCombatParty(p,chr,d.inputs);
	require(ctx.minutes==480&&ctx.ctr24==0,"combat requires initial PTY time");
	const auto boundaryGeneration=b.generation();
	try {
		w._combatCheck=[&]{require(d.exact()&&b.generation()==boundaryGeneration,"combat admission callback changed owners");};
		const auto initial=XeenActorApproach::actorsFromResources(w.objectFile(20),stats);
		require(initial.size()==27&&initial[5].original.resourceId==8&&initial[5].statistics.has_value(),"combat original actor collection mismatch");
		initial[5].statistics->validateCombat();
		XeenActorApproach::validateDomain(w,p,ctx,initial,events);
		w._combatCheck={};
	}catch(...){w._combatCheck={};throw;}
	require(d.exact()&&b.quiet(),"combat owners changed during admission");
	for(unsigned i=0;i<6;++i)p.roster._combatInputs[kXeenCombatOwners[i]]=d.inputs[i];
	d.attached=true;d.last=d.observation(Status::Accepted);
}
XeenCombat::XeenCombat(XeenWorld &w, XeenPartyState &p, XeenCamera &c, XeenCombatBoundary &b,
		const XeenGameFlags &flags, const XeenEncounterState &state, const std::vector<XeenMonsterRecord> &statistics,
		const XeenEventFile &events) : impl(std::make_unique<Impl>(this,w,p,c,b,p.encounterContext.value(),statistics,events,
			w.sessionState().journeyContract()>=2 ? XeenCombatRandom(*w.sessionState().journeyRandom()) : XeenCombatRandom(w.sessionState().skeletonSeed()))) {
	auto &d = *impl; auto &s = d.session();
	require(s.journey() && s._journeyOwner && s._journeyActivity == XeenJourneyActivity::Attachment &&
		!s._combatOwner && !s._combatEntered && b.quiet() && b.world == &w && b.party == &p && b.camera == &c,
		"Journey attachment requires current coordination");
	require(XeenActorApproach::authoritative(w,p,c,state) && state.phase() == XeenEncounterPhase::Engaged && (s.journeyContract()>=2 || !state.pending()),
		"Journey attachment requires genuine engagement");
	xeenValidateJourneyMelee(p,s.journeyContract());
	d.contract=s.journeyContract(); d.expectedRandom=s.journeyRandom();
	d.journey = true; d.flags = &flags; d.expectedFlags = flags.values();
	d.journeySeed = s._skeletonSeed; d.journeyOwner = s._journeyOwner; d.journeyGeneration = s._journeyGeneration;
	d.actors = s._actors; d.accounted = s._accountedMonsters; d.approach = state;
	for (unsigned i = 0; i < 30; ++i) d.allInputs[i] = p.roster.combatInputs(i);
	for (unsigned i = 0; i < 6; ++i) d.inputs[i] = *d.allInputs[kXeenCombatOwners[i]];
	d.borrow.reset(new XeenWorld::GameplayBorrow(w,p,c,flags));
	d.lifetime = std::make_unique<XeenRestoreGuard>(w,p,c,flags);
	// All fallible preparation precedes attachment; no CHR/PTY mutable value is installed.
	s._combatOwner = this; s._combatApproachState = &d.approach; s._journeyActivity = XeenJourneyActivity::Combat;
	d.attached = true; d.phase = Phase::Engaged; d.last = d.observation(Status::Accepted);
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
bool XeenCombat::current(const Ticket &t) const noexcept {
	const auto &d=*impl;if (d.journey && !d.lifetime->ownersAlive()) return false;
	return t.owner==this&&t.incarnation==d.world._incarnation&&
		t.generation==d.generation&&t.revision==d.revision()&&t.boundary==d.boundary.generation()&&
		t.phase==d.phase&&t.work==d.work&&d.world._sessionState._combatOwner==this;
}
const XeenCombatResult &XeenCombat::result() const noexcept {return impl->last;}
bool XeenCombat::boundTo(const XeenWorld &w,const XeenPartyState &p,const XeenCamera &c,const XeenCombatBoundary &b) const noexcept {
	return &impl->world==&w && &impl->party==&p && &impl->camera==&c && &impl->boundary==&b;
}
const std::optional<XeenEquipmentResult> &XeenCombat::preparationEquipmentResult() const noexcept {return impl->equipmentResult;}
const std::optional<XeenTransferResult> &XeenCombat::preparationTransferResult() const noexcept {return impl->transferResult;}
const XeenEncounterState &XeenCombat::approachState() const noexcept {return impl->approach;}
XeenCombatPhase XeenCombat::phase() const noexcept {return impl->phase;}
XeenCombatWork XeenCombat::pending() const noexcept {return impl->work;}
int XeenCombat::participant() const noexcept {return impl->turn;}
const XeenCombatRandom &XeenCombat::random() const noexcept {return impl->rng;}
void XeenCombat::setProbe(std::function<void()> p) { require(!impl->busy,"cannot replace an in-flight probe");impl->probe=std::move(p); }
void XeenCombat::preparePresentation(const Ticket &t,const std::function<void()> &compose) {
 auto &d=*impl;
 require(current(t)&&!d.busy,"Stale combat presentation");
 if(!d.journey) { compose();return; }
 require(d.exact(),"Combat presentation preimage changed");
 Busy busy(d.busy);
 auto guard=std::make_unique<XeenRestoreGuard>(d.world,d.party,d.camera,*d.flags);
 guard->retainResources(*d.lifetime);
 const auto check=[&] {require(current(t),"Reentrant combat presentation");guard->check();};
 try {
  { XeenRestoreGuard::Providers providers(*guard,d.world,check);compose();check(); }
  d.lifetime.swap(guard);
 } catch(...) {
  // Compatible I/O failure retains all successfully admitted resource bytes.
  // A changed owner or resource fails closed and cannot renew authority.
  try {check();d.lifetime.swap(guard);}catch(...){fail(t,Failure::Integrity);}
  throw;
 }
}
void XeenCombat::retainResources(XeenRestoreGuard &guard) const {
 if(impl->journey)guard.retainResources(*impl->lifetime);
}
XeenCombatResult XeenCombat::fail(const Ticket &t,Failure f) noexcept {
	auto &d=*impl;if(!current(t))return d.observation(Status::Stale);
	if(terminal(d.phase) && !(d.journey && (d.phase==Phase::Disengaged || (d.phase == Phase::Victory && f == Failure::Integrity)))) {
		if(d.phase==Phase::Victory&&f==Failure::Integrity&&
			d.session()._completion==XeenEncounterCompletion::VictoryEnded&&
			!d.session()._completedIntegrityUnsafe)d.latchIntegrity();
		return d.last;
	}
	auto r=d.observation(Status::Failed);r.oldRevision=d.revision();r.failure=f;r.operation=Operation::Failure;
	d.phase=f==Failure::Time?Phase::SupportStopped:Phase::Failed;d.work=Work::None;d.candidate.reset();d.consequences.reset();
	if (d.journey) { d.ended = false; d.finishedDisengagement=false; d.session()._journeyActivity = XeenJourneyActivity::Failed; }
	d.session()._encounterTerminal=true;
	// Preparation authority uses coordinator generations, including its failures.
	// World revision zero is reserved until real M26 initialization publishes one.
	if(d.session()._encounterInitialized&&d.revision()!=std::numeric_limits<std::uint64_t>::max())++d.session()._encounterRevision;
	if(d.generation!=std::numeric_limits<std::uint64_t>::max())++d.generation;
	return d.adopt(r,f==Failure::Time?Status::SupportStopped:Status::Failed);
}
void XeenCombat::invalidate() noexcept { auto t=ticket();fail(t,Failure::Integrity); }

XeenEquipmentResult XeenCombat::equipment(const Ticket &t,std::size_t active,XeenInventoryCategory cat,std::size_t slot,XeenEquipmentOperation op) {
	auto &d=*impl;XeenEquipmentResult refused;
	if(!current(t)||d.busy||d.phase!=Phase::Preparation||!d.boundary.preparationReady())return refused;
	if(!d.exact()){fail(t,Failure::Integrity);return refused;}
	try {d.capacity();}catch(...){fail(t,Failure::Overflow);return refused;}
	Busy busy(d.busy);advance(d.generation);const auto entry=ticket();
	XeenEquipmentResult r;
	try {
		r=xeenSetEquipment(d.party,active,cat,slot,op);
		if(r.status==XeenEquipmentStatus::Success) {
			const auto id=*r.owner;*xeenInventoryItems(d.expected.roster.at(id),cat)=*xeenInventoryItems(d.party.roster.at(id),cat);
		}
		d.equipmentResult=r;d.transferResult.reset();d.last=d.observation(Status::Accepted);d.last.operation=Operation::Equipment;d.probeFor(entry);return r;
	}catch(...){fail(entry,Failure::Preparation);return r;}
}
XeenTransferResult XeenCombat::transfer(const Ticket &t,std::size_t from,std::size_t to,XeenInventoryCategory cat,std::size_t slot) {
	auto &d=*impl;XeenTransferResult refused;
	if(!current(t)||d.busy||d.phase!=Phase::Preparation||!d.boundary.preparationReady())return refused;
	if(!d.exact()){fail(t,Failure::Integrity);return refused;}
	try {d.capacity();}catch(...){fail(t,Failure::Overflow);return refused;}
	Busy busy(d.busy);advance(d.generation);const auto entry=ticket();
	XeenTransferResult r;
	try {
		r=xeenTransferItem(d.party,from,to,cat,slot);
		if(r.status==XeenTransferStatus::Success) for(auto id:{r.sourceOwner,r.destinationOwner})
			*xeenInventoryItems(d.expected.roster.at(id),cat)=*xeenInventoryItems(d.party.roster.at(id),cat);
		d.transferResult=r;d.equipmentResult.reset();d.last=d.observation(Status::Accepted);d.last.operation=Operation::Transfer;d.probeFor(entry);return r;
	}catch(...){fail(entry,Failure::Preparation);return r;}
}

XeenCombatResult XeenCombat::beginApproach(const Ticket &t) {
	auto &d=*impl;if(!current(t))return d.observation(Status::Stale);
	if(d.busy||d.phase!=Phase::Preparation)return d.observation(Status::Refused);
	if(!d.boundary.quiet()||!d.exact())return fail(t,Failure::Integrity);
	try {d.capacity();}catch(...){return fail(t,Failure::Overflow);}
	Busy busy(d.busy);advance(d.generation);const auto entry=ticket();
	try {
		d.actors.reserve(27);d.probeFor(entry);
		d.world._combatCheck=[&]{require(current(entry)&&d.exact(),"combat approach preparation authority changed");};
		const auto r=XeenActorApproach::initialize(d.world,d.party,d.camera,d.approach,d.statistics,d.initialContext,d.events);
		d.world._combatCheck={};
		if(d.generation!=entry.generation)return d.observation(Status::Stale);
		d.actors=d.session()._actors;d.expected.encounterContext=d.party.encounterContext;d.expectedCamera=d.camera;
		require(r.outcome==XeenEncounterOutcome::Started,"combat approach initialization failed");
		d.phase=Phase::Approach;++d.generation;auto result=d.observation(Status::Accepted);result.operation=Operation::BeginApproach;
		return d.adopt(result,Status::Accepted);
	}catch(...){d.world._combatCheck={};return fail(entry,Failure::Preparation);}
}
XeenCombatResult XeenCombat::runApproach(const Ticket &t,std::optional<XeenEncounterAction> action) {
	auto &d=*impl;if(!current(t))return d.observation(Status::Stale);
	if(d.busy||d.phase!=Phase::Approach)return d.observation(Status::Refused);
	if(action&&(*action==XeenEncounterAction::Unsupported||static_cast<unsigned>(*action)>5))return d.observation(Status::Refused);
	if(!d.boundary.quiet()||!d.exact())return fail(t,Failure::Integrity);
	Busy busy(d.busy);const auto entry=t;
	try {
		d.capacity();d.probeFor(entry);
		d.world._combatAuthorized=[this,entry]() noexcept { return current(entry); };
		d.world._combatCheck=[&]{require(current(entry)&&d.exact(),"combat approach owner preimage changed");};
		const auto r=action?XeenActorApproach::action(d.world,d.party,d.camera,d.approach,*action,d.events):
			XeenActorApproach::pulse(d.world,d.party,d.camera,d.approach,d.events);
		d.world._combatCheck={};d.world._combatAuthorized={};
		// M26 may advance its revision, but cannot replace the retained Diagnostic27
		// authority, including the external boundary generation.
		if(d.generation!=entry.generation||d.boundary.generation()!=entry.boundary||
			d.phase!=entry.phase||d.work!=entry.work||d.session()._combatOwner!=this||
			r.outcome==XeenEncounterOutcome::Stale||r.revision!=d.revision())return d.observation(Status::Stale);
		// M26 adopted the world revision. Retire this operation before observers.
		d.actors=d.session()._actors;d.expected.encounterContext=d.party.encounterContext;d.expectedCamera=d.camera;++d.generation;
		if(d.approach.phase()==XeenEncounterPhase::Engaged)d.phase=Phase::Engaged;
		else if(d.approach.phase()==XeenEncounterPhase::SupportStopped)d.phase=Phase::SupportStopped;
		auto observation=d.observation(Status::Advanced);observation.oldRevision=entry.revision;
		observation.operation=action?Operation::ApproachAction:Operation::ApproachPulse;observation.approachAction=action;
		return d.adopt(observation,d.phase==Phase::SupportStopped?Status::SupportStopped:Status::Advanced);
	}catch(...){d.world._combatCheck={};d.world._combatAuthorized={};return fail(entry,Failure::Preparation);}
}
XeenCombatResult XeenCombat::approachAction(const Ticket &t,XeenEncounterAction a){return runApproach(t,a);}
XeenCombatResult XeenCombat::approachPulse(const Ticket &t){return runApproach(t,{});}
XeenCombatResult XeenCombat::beginCombat(const Ticket &t) {
	auto &d=*impl;if(!current(t))return d.observation(Status::Stale);
	if(d.busy||d.phase!=Phase::Engaged)return d.observation(Status::Refused);
	if(!d.exact()||!d.boundary.quiet())return fail(t,Failure::Integrity);
	Busy busy(d.busy);
	try {
		d.capacity();require(!d.session()._combatEntered&&d.session()._encounterTerminal&&
			XeenActorApproach::authoritative(d.world,d.party,d.camera,d.approach)&&(d.contract>=2 || d.approach.pending()==0)&&
			d.approach.reason()==XeenEncounterStop::None,"combat handoff is not authoritative");
		const auto view=XeenActorApproach::classify(d.session()._actors,d.camera);
		require(view.engaged(),"combat requires contact");
		for (unsigned i=0;i<3;++i) if (view.slots[i]) {
			const auto &a=d.session()._actors.at(view.slots[i]->recordIndex);
			require(xeenJourneyContent(d.contract).influences(a.id.recordIndex) && a.hp>0 && a.lifecycle==XeenActorLifecycle::Present && a.status==XeenActorStatus::Physical,"Invalid contact actor");
			if (!xeenJourneyContent(d.contract).consequences()) a.statistics->validateCombat();
		}
		for (unsigned i=0;i<6;++i) { d.playerSpeeds[i]=Rules::effectivePhysical(d.character(i),d.inputs[i],Rules::PhysicalAttribute::Speed,{d.party.encounterContext->year}); require(xeenJourneyContent(d.contract).consequences() || !d.character(i).canAct() || d.playerSpeeds[i]>0,"Nonpositive combat Speed"); }
		d.resourcesFor(t);d.reconcile();d.sortOrder();d.probeFor(t);
		d.moveDue=d.contract>=2 && d.approach.pending()!=0; d.chargeRound=false; d.approach._pending=0;
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
XeenCombatResult XeenCombat::command(const Ticket &t,XeenCombatCommand action) {
	auto &d=*impl;if(!current(t))return d.observation(Status::Stale);
	if(d.busy||d.phase!=Phase::PlayerReady||(action!=XeenCombatCommand::Attack&&action!=XeenCombatCommand::Block&&action!=XeenCombatCommand::Run))return d.observation(Status::Refused);
	if(action==XeenCombatCommand::Run && (!xeenJourneyContent(d.contract).disengagement() || d.turn<0 || d.turn>=6 || !(d.participants&(1u<<d.turn)) || !d.character(d.turn).canAct() || d.playerSpeeds[d.turn]<=0)) return d.observation(Status::Refused);
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
		if (xeenJourneyContent(d.contract).consequences()) {
			d.consequences.emplace();auto &c=*d.consequences;c.rng=d.rng;c.result=d.observation(Status::Pending);
			c.result.oldRevision=d.revision();c.result.participant=d.turn;c.result.operation=Operation::PlayerAttack;
			c.result.actingOwner=kXeenCombatOwners[d.turn];c.result.targetMonster=d.selected;c.result.monster=*d.selected;
			const auto &actor=d.actors.at(d.selected->recordIndex);
			c.result.actorHpBefore=c.result.actorHpAfter=actor.hp;
			c.player.emplace(d.character(d.turn),d.inputs[d.turn],*actor.statistics,actor.original.resourceId,d.party.encounterContext->year,false);
			return d.adopt(c.result,Status::Pending);
		}
		d.candidate.emplace();auto &c=*d.candidate;c.rng=d.rng;c.boundary=d.boundary.generation();c.result=d.observation(Status::Pending);
		c.result.oldRevision=d.revision();c.result.participant=d.turn;
		c.result.operation=Operation::PlayerAttack;c.result.attackOutcome=AttackOutcome::Pending;
		c.result.actingOwner=kXeenCombatOwners[d.turn];c.result.targetMonster=d.selected;
		c.result.monster=*d.selected;
		c.result.actorHpBefore=d.session()._actors.at(d.selected->recordIndex).hp;c.result.actorHpAfter=c.result.actorHpBefore;
		constexpr int divisors[]{1,2,2,3,4,2,2,1,3,2};
		const auto &ch=d.character(d.turn);
		c.hit=Rules::physicalBonus(Rules::effectivePhysical(ch,d.inputs[d.turn],Rules::PhysicalAttribute::Accuracy,{610}))+5+
			ch.currentLevel()/divisors[static_cast<unsigned>(ch.characterClass)];
		c.hitBase=c.hit;c.attacks=xeenCombatAttackCount(ch.characterClass,ch.currentLevel());
		return d.adopt(c.result,Status::Pending);
	}catch(...){return fail(entry,d.journey && !d.exact() ? Failure::Integrity : Failure::Preparation);}
}

XeenCombatResult XeenCombat::service(const Ticket &t) {
	auto &d=*impl;if(!current(t))return d.observation(Status::Stale);
	if(d.busy||d.work==Work::None||terminal(d.phase))return d.observation(Status::Refused);
	if(!d.exact()||!d.boundary.quiet())return fail(t,Failure::Integrity);
	if(d.work==Work::FinishDisengagement) return finishDisengagement(t);
	if (xeenJourneyContent(d.contract).consequences()) return serviceConsequences(t);
	Busy busy(d.busy);
	try {
		d.capacity();
		if(d.candidate&&d.candidate->boundary!=d.boundary.generation())return fail(t,Failure::Integrity);
		if(d.work==Work::Round||d.work==Work::End) {
			const bool end=d.work==Work::End;
			if ((end || !d.moveDue || d.chargeRound) && d.party.encounterContext->minutes>=959) return fail(t,Failure::Time);
			if (!end && !d.moveDue) {
				d.moveDue=true; d.chargeRound=true; d.acted.fill(false);d.blocked.fill(false);d.enemyOrdinal=0;
				d.reconcile();d.sortOrder();d.selectNext();
				if (d.work==Work::Enemy) { ++d.generation; return d.adopt(d.observation(Status::Pending),Status::Pending); }
			}
			d.resourcesFor(t); d.probeFor(t);
			auto r=d.observation(Status::Advanced);r.oldRevision=d.revision();r.operation=end?Operation::End:Operation::Round;
			if (!end) {
				const auto prepareMovement=[&] {
					std::optional<XeenRestoreGuard> guard;
					std::unique_ptr<XeenRestoreGuard::Providers> providers;
					if (d.journey) {
						guard.emplace(d.world,d.party,d.camera,*d.flags);
						providers=std::make_unique<XeenRestoreGuard::Providers>(*guard,d.world,[&] { require(current(t)&&d.exact(),"Round provider authority changed"); });
					}
					auto moved=XeenActorApproach::move(d.session()._actors,d.camera,[&](const XeenActor &a,int x,int y) {
					const auto &policy=xeenJourneyContent(d.contract);
					if (!policy.movementContains(x,y)) return XeenMonsterTerrain::Unsupported;
					const auto cell=d.world.sampleCell(a.id.mapId,x,y);
					if (policy.blockedTerrain(x,y)) return XeenMonsterTerrain::Blocked;
					return cell && (cell->cell->rawWord==0x31 || (d.contract==2 && cell->cell->rawWord==1)) && cell->cell->rawAttributes==0 ? XeenMonsterTerrain::Allowed : XeenMonsterTerrain::Unsupported;
				});
				const auto view=XeenActorApproach::classify(moved,d.camera);
				for (unsigned i=0;i<moved.size();++i) moved[i].activated=moved[i].activated || view.activation[i];
				XeenActorApproach::validateEnvironment(d.world,moved,d.events,d.contract);
					if (guard) guard->check();
					return moved;
				};
				auto moved=prepareMovement();
				auto expected=moved; d.probeFor(t);
				d.session()._actors.swap(moved);d.actors.swap(expected);
				const bool charge=d.chargeRound; d.moveDue=d.chargeRound=false;
				const int selectedParticipant=d.turn;
				d.reconcile();d.sortOrder();
				if (d.contact[0] && selectedParticipant>=0 && selectedParticipant<6 && d.character(selectedParticipant).canAct()) { d.turn=selectedParticipant;d.phase=Phase::PlayerReady;d.work=Work::None; } else d.selectNext();
				if (charge) { ++d.party.encounterContext->minutes;++d.expected.encounterContext->minutes; }
			} else {
				require(!XeenActorApproach::classify(d.actors,d.camera).engaged() && !d.moveDue,"End still has mandatory contact work");
				d.phase=Phase::Victory;d.work=Work::None;
				if(d.journey) d.ended=true;
				else { d.session()._completion=XeenEncounterCompletion::VictoryEnded;d.session()._completedMonster=XeenMonsterIdentity{20,5}; }
				++d.party.encounterContext->minutes;++d.expected.encounterContext->minutes;
			}
			d.published();return d.adopt(r,end?Status::Victory:Status::Advanced);
		}
		d.resourcesFor(t);
		if(!d.candidate) {
			require(d.work==Work::Enemy,"missing action continuation");d.candidate.emplace();auto &c=*d.candidate;
			c.rng=d.rng;c.boundary=d.boundary.generation();c.step=Impl::Step::Target;c.result=d.observation(Status::Pending);c.result.oldRevision=d.revision();c.result.participant=d.turn;
			c.result.operation=Operation::EnemyAttack;c.result.attackOutcome=AttackOutcome::Pending;
			c.result.actingMonster=d.contact[d.turn-6]; c.result.monster=*c.result.actingMonster;
		}
		auto &c=*d.candidate;using Step=Impl::Step;
		unsigned draws=0;
		auto draw=[&](unsigned lo,unsigned hi)->std::optional<unsigned>{
			if(draws==64)return {}; ++draws;auto n=c.rng.draw(lo,hi);d.probeFor(t);return n;
		};
		const auto monsterIndex=c.result.monster.recordIndex;
		const auto &monster=*d.session()._actors.at(monsterIndex).statistics;
		while(c.step!=Step::Done && draws<64) {
			switch(c.step) {
			case Step::Weapon: {
				if(c.dice) {auto n=draw(1,c.sides);if(n){c.weapon=checked(std::int64_t(c.weapon)+*n);--c.dice;}break;}
				if(c.slot==9){c.weapon=checked(std::int64_t(c.weapon)*3);c.step=Step::PlayerHit;break;}
				const auto item=d.character(d.turn).weapons[c.slot++];if(item.frame!=1&&item.frame!=13)break;
				require(item.material==0&&item.state==0,"combat weapon helper requires admitted good weapon");
				switch(item.id){case 6:c.dice=4;c.sides=2;break;case 2:case 8:c.dice=2;c.sides=3;break;
				case 12:c.dice=2;c.sides=2;break;case 15:c.dice=1;c.sides=6;break;case 7:c.dice=1;c.sides=3;break;
				default:throw std::invalid_argument("unsupported melee weapon");}break;
			}
			case Step::PlayerHit: {
				auto n=draw(1,20);if(!n)break;c.hit=checked(std::int64_t(c.hit)+*n);if(*n==20)break;
				if(c.hit>=int(monster.armorClass()+10)) {
					const int might=Rules::physicalBonus(Rules::effectivePhysical(d.character(d.turn),d.inputs[d.turn],Rules::PhysicalAttribute::Might,{610}));
					c.accumulated=checked(std::int64_t(c.accumulated)+std::max(might+c.weapon,1));
				}
				if(--c.attacks){c.slot=0;c.weapon=0;c.hit=c.hitBase;c.step=Step::Weapon;break;}
				c.damage=checked(std::int64_t(c.accumulated)*(100-monster.physicalResistance())/100);
				c.result.attackOutcome=c.accumulated==0?AttackOutcome::Miss:
					c.damage==0?AttackOutcome::HitZeroDamage:AttackOutcome::HitPositiveDamage;
				c.result.damage=c.damage;c.result.actorHpAfter=std::max(c.result.actorHpBefore-c.damage,0);c.step=Step::Done;break;
			}
			case Step::Target:
				for(int i=0;i<6;++i)if(d.character(i).canAct()&&unsigned(d.character(i).characterClass)==monster.preferredClass()){c.target=i;break;}
				if(c.target<0){auto n=draw(0,5);if(!n)break;c.target=*n;}
				c.step=d.character(c.target).canAct()?Step::EnemyRoll:Step::Fallback;break;
			case Step::Fallback: {
				std::array<int,6> able{};unsigned count=0;for(int i=0;i<6;++i)if(d.character(i).canAct())able[count++]=i;
				require(count!=0,"enemy has no eligible target");auto n=draw(0,count-1);if(n){c.target=able[*n];c.step=Step::EnemyRoll;}break;
			}
			case Step::EnemyRoll: {
				c.result.targetOwner=kXeenCombatOwners[c.target];
				if(!c.injured)c.injured=d.character(c.target);auto n=draw(1,20);if(!n)break;c.roll=*n;
				c.result.critical=*n==20;
				c.step=*n==1?Step::Done:*n==20?Step::FirstDice:Step::HitParameter;break;
			}
			case Step::FirstDice: case Step::SecondDice: {
				auto n=draw(1,monster.damageDie());if(!n)break;c.damage+=*n;
				if(++c.dice==int(monster.strikes())) { c.afterDisease=c.step==Step::FirstDice?Step::HitParameter:Step::Done;
					if (c.damage>0 && monster.raw[30]==7) c.step=Step::Disease;
					else { d.injury(c); c.step=c.afterDisease; } } break;
			}
			case Step::HitParameter: {
				auto n=draw(1,monster.hitParameter());if(!n)break;
				const int threshold=d.armor(*c.injured,c.target)+(d.blocked[c.target]?d.character(c.target).currentLevel()/2+15:10);
				c.step=c.roll+int(monster.hitParameter()/4+*n)>=threshold?Step::SecondDice:Step::Done;break;
			}
			case Step::Disease: {
				const auto wide=std::int64_t(Rules::physicalBonus(Rules::effectiveLuck(*c.injured,d.inputs[c.target])))+c.injured->currentLevel();
				require(wide>=std::numeric_limits<int>::min() && wide<=std::numeric_limits<int>::max(),"Physical save arithmetic overflow");
				const int v=int(wide); const int hi=checked(wide+20); auto n=draw(1,hi); if (!n) break;
				if (std::int64_t(*n)>v) { require(c.injured->conditions[4]<255,"Disease byte domain exhausted"); ++c.injured->conditions[4]; }
				d.injury(c);c.step=c.afterDisease;break;
			}
			case Step::Done:break;
			}
		}
		if(c.step!=Step::Done){++d.generation;return d.adopt(c.result,Status::Pending);}
		auto r=c.result;
		if(d.work==Work::Enemy) {
			for(unsigned i=0;i<r.injuryCount;++i)r.damage+=r.injuries[i].amount;
			r.attackOutcome=r.injuryCount==0?AttackOutcome::Miss:
				r.damage==0?AttackOutcome::HitZeroDamage:AttackOutcome::HitPositiveDamage;
		}
		const bool lethal=d.work==Work::Action&&r.actorHpAfter==0;
		std::set<XeenMonsterIdentity> preparedAccounting, preparedExpectedAccounting;
		std::optional<XeenJourneyLethal> journeyLethal;
		if(lethal) {
			require(d.journey ? !d.accounted.count(d.actors[monsterIndex].id) : !d.session()._combatAccounted,
				"monster accounting already consumed");unsigned eligible=0;
			for(int i=0;i<6;++i)if(xeenCombatXpEligible(d.character(i).worstCondition()))++eligible;
			require(eligible!=0,"lethal action has no XP recipient");
			if (d.journey) {
				std::array<const XeenCharacter *,6> characters{};
				for (unsigned i = 0; i < 6; ++i) characters[i] = &d.character(i);
				journeyLethal = xeenPrepareJourneyLethal(d.actors[monsterIndex],characters,d.inputs,d.accounted,d.participants);
				preparedAccounting = journeyLethal->accounted;
				preparedExpectedAccounting = preparedAccounting;
			}
			for(int i=0;i<6;++i){if(!xeenCombatXpEligible(d.character(i).worstCondition()))continue;
				const auto xp=d.journey ? journeyLethal->experience[i] :
					xeenCombatExperience(monster.experience(),eligible,d.character(i).permanentLevel,d.inputs[i].experience);
				r.xp[r.xpCount++]={kXeenCombatOwners[i],d.inputs[i].experience,xp};
			}
		}
		if(d.work==Work::Enemy && c.injured)for(unsigned slot=0;slot<9;++slot) {
			const auto before=d.character(c.target).armor[slot],after=c.injured->armor[slot];
			if(!xeenSameItem(before,after))r.armor[r.armorCount++]={kXeenCombatOwners[c.target],static_cast<std::uint8_t>(slot),before,after};
		}
		d.probeFor(t);
		// Publication: scalar/fixed stores only; all callbacks and allocations ended.
		if(d.work==Work::Action) {
			auto &a=d.session()._actors[monsterIndex];a.hp=r.actorHpAfter;
			if(lethal) {
				a.lifecycle=XeenActorLifecycle::Defeated;a.x=a.y=-128;a.activated=false;
				if (d.journey) {
					a = journeyLethal->actor;
					d.session()._accountedMonsters.swap(preparedAccounting); d.accounted.swap(preparedExpectedAccounting);
				}
				else d.session()._combatAccounted=true;
				for(unsigned k=0;k<r.xpCount;++k) {
					const auto &xp=r.xp[k];d.party.roster._combatInputs[xp.owner]->experience=xp.after;
					if (d.journey) d.allInputs[xp.owner]->experience = xp.after;
					for(unsigned i=0;i<6;++i)if(kXeenCombatOwners[i]==xp.owner)d.inputs[i].experience=xp.after;
				}
			}
			d.actors[monsterIndex]=a; d.acted[d.turn]=true;d.reconcile();d.sortOrder();d.selectNext();
		} else {
			if(c.injured) {
				auto &ch=d.character(c.target);auto &expected=d.expected.roster.at(ch.rosterId);
				ch.currentHp=c.injured->currentHp;ch.conditions=c.injured->conditions;ch.armor=c.injured->armor;
				expected.currentHp=ch.currentHp;expected.conditions=ch.conditions;expected.armor=ch.armor;
			}
			if(d.defeated()){d.phase=Phase::Defeat;d.work=Work::None;}else if (++d.enemyOrdinal>=monster.attacks()) { d.enemyOrdinal=0;d.acted[d.turn]=true;d.selectNext(); }
		}
		d.rng=c.rng;
		if (d.contract==2) { d.session()._journeyRandom=d.rng.continuation(); d.expectedRandom=d.session()._journeyRandom; }
		d.candidate.reset();d.published();return d.adopt(r,d.phase==Phase::Defeat?Status::Defeat:Status::Advanced);
	}catch(const IntegrityError &){d.clearResourceCheck();return fail(t,Failure::Integrity);}
	catch(...){d.clearResourceCheck();return fail(t,d.journey && !d.exact() ? Failure::Integrity : Failure::Preparation);}
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
				d.acted.fill(false);d.enemyOrdinal=0;
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
				const auto map=d.world.map(23);d.probeFor(t);
				c.movement.emplace(map,d.actors,d.camera,d.characters(),d.inputs,d.party.encounterContext->year,d.participants,d.blocked);
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
				if(!c.movement->service(draw)) return pending();
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
				c.drop.emplace(*d.party.monsterTreasure,c.result.monster.recordIndex,d.contract);
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
			const auto &a=c.enemy->result;r.damage=a.damage;r.injuries=a.injuries;r.injuryCount=a.injuryCount;
			r.armor=a.armor;r.armorCount=a.armorCount;r.attackOutcome=a.attackOutcome;r.targetOwner=a.targetOwner;r.critical=a.critical;r.targetedMembers=a.targetedMembers;
		}
		// All allocations, draws and provider callbacks precede the atomic stores.
		d.probeFor(t);
		if(c.run) {
			d.participants=r.participantsAfter;d.acted[d.turn]=true;
			if(r.runSuccess && !d.continuingParticipants()) d.exitCause=XeenCombatExitCause::DirectRun;
		}
		if(c.player) {
			auto &actor=d.session()._actors.at(r.monster.recordIndex);actor.hp=r.actorHpAfter;
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
		if(c.movement) { d.session()._actors.swap(c.movement->actors);d.actors.swap(expectedActors);d.publishCharacters(c.movement->characters); }
		if(c.time) { d.publishCharacters(c.time->characters);d.party.encounterContext=d.expected.encounterContext=c.time->context; }
		d.rng=c.rng;d.expectedRandom=d.session()._journeyRandom=d.rng.continuation();
		const auto operation=r.operation;
		const int selected=d.turn;
		if(c.movement) d.moveDue=d.chargeRound=false;
		d.consequences.reset();d.refreshSpeeds();d.reconcile();d.sortOrder();
		if(d.defeated()) { d.phase=Phase::Defeat;d.work=Work::None; }
		else if(end) { d.phase=Phase::Victory;d.work=Work::None;d.ended=true; }
		else if(d.disengagementDue()) {}
		else if(operation==Operation::EnemyAttack && ++d.enemyOrdinal<d.actors.at(r.monster.recordIndex).statistics->attacks()) {
			d.phase=Phase::PendingEnemy;d.work=Work::Enemy;
		} else {
			if(operation==Operation::EnemyAttack) { d.enemyOrdinal=0;d.acted[d.turn]=true; }
			if(operation==Operation::Round && d.contact[0] && selected>=0 && selected<6 && (d.participants&(1u<<selected)) && d.character(selected).canAct() && d.playerSpeeds[selected]>0) {
				d.turn=selected;d.phase=Phase::PlayerReady;d.work=Work::None;
			} else d.selectNext();
		}
		d.published();return d.adopt(r,d.phase==Phase::Defeat?Status::Defeat:end?Status::Victory:Status::Advanced);
	} catch(...) { d.clearResourceCheck();return fail(t,!d.exact()?Failure::Integrity:Failure::Preparation); }
}

XeenCombatResult XeenCombat::finishDisengagement(const Ticket &t) {
 auto &d=*impl;Busy busy(d.busy);
 try {
  d.capacity();d.resourcesFor(t);
	  require(xeenJourneyContent(d.contract).disengagement() && d.journey && d.phase==Phase::DisengagementPending &&
   d.work==Work::FinishDisengagement && !d.candidate && !d.consequences && !d.moveDue && !d.chargeRound &&
   d.contact[0] && !d.continuingParticipants() && !d.defeated() && d.participants!=0x3f &&
   d.exitCause!=XeenCombatExitCause::None,"Incomplete disengagement obligations");
  const auto map=d.world.map(d.camera.mapId);d.probeFor(t);
  const auto &g=map.geometry;
  require(d.camera.mapId==XeenMapIdentity(23) && g.runX==10 && g.runY==12 && g.difficulties[7]==100,
   "Original Run metadata changed");
  XeenCamera destination=d.camera;destination.x=g.runX;destination.y=g.runY;
  require(XeenMovement::component(map,9,11,xeenJourneyContent(d.contract).traversal)[destination.y*16+destination.x] &&
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
   r.forfeitedGold=treasure.pendingGold;r.forfeitedMask=treasure.pendingMask;treasure=xeenPrepareMonsterGoldForfeiture(treasure,d.contract);
  }
  // Validation only: this detached party is never assigned to a live owner.
  XeenPartyState candidate;
  for(unsigned id=0;id<30;++id) candidate.roster.at(id)=d.party.roster.at(id);
  candidate.roster._combatInputs=d.allInputs;candidate.roster._combatMarked=true;
  candidate.party=d.party.party;candidate.encounterContext=d.party.encounterContext;
  candidate.questItems=d.party.questItems;candidate.questFlags=d.party.questFlags;
  candidate.regionalRecovery=d.party.regionalRecovery;
  candidate.firstSerializedCount=d.party.firstSerializedCount;candidate.effectiveSerializedCount=d.party.effectiveSerializedCount;
  for(const auto &c:characters) candidate.roster.at(c.rosterId).conditions=c.conditions;
  candidate.monsterTreasure=treasure;xeenValidateJourneyParty(candidate,d.contract);
  auto actors=d.actors;const auto view=XeenActorApproach::classify(actors,destination);
  for(unsigned i=0;i<actors.size();++i) actors[i].activated=actors[i].activated || view.activation[i];
  auto expectedActors=actors;
  d.probeFor(t);
  // No callbacks or fallible owner assignment after publication begins.
  d.publishCharacters(characters);d.party.monsterTreasure=d.expected.treasure=treasure;
  d.camera.x=d.expectedCamera.x=destination.x;d.camera.y=d.expectedCamera.y=destination.y;
  d.session()._actors.swap(actors);d.actors.swap(expectedActors);d.destinationView=view;
  d.finishedDisengagement=true;d.phase=Phase::Disengaged;d.work=Work::None;d.published();
  return d.adopt(r);
 } catch(...) { d.clearResourceCheck();return fail(t,!d.exact()?Failure::Integrity:Failure::Preparation); }
}
void XeenCombat::retireDisengagedJourney(const Ticket &t,XeenEncounterState &state) {
 auto &d=*impl;
	 require(current(t) && d.journey && xeenJourneyContent(d.contract).disengagement() && !d.busy && d.phase==Phase::Disengaged &&
  d.finishedDisengagement && d.work==Work::None && !d.candidate && !d.consequences &&
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
		d.work == Work::None && !d.candidate && !d.consequences && d.boundary.quiet(), "Journey retirement requires successful quiescent End");
	if (!d.exact()) { fail(t,Failure::Integrity); throw IntegrityError("Journey retirement preimage changed"); }
	d.capacity();
	require(!d.contact[0] && !d.moveDue && (xeenJourneyContent(d.contract).consequences() ? d.episodeLethal : !d.accounted.empty()),
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

XeenCompletedEncounterTicket XeenCombat::retireCompletedVictory(const Ticket &t) {
	auto &d=*impl;auto &s=d.session();
	require(current(t),"completed combat ticket is stale");
	require(!d.busy&&d.phase==Phase::Victory&&d.work==Work::None&&
		!d.candidate&&d.boundary.quiet(),"completed combat is not current and quiescent");
	require(!s._completedIntegrityUnsafe,"completed combat integrity is unsafe");
	if(!d.exact()) {
		d.latchIntegrity();
		throw std::invalid_argument("completed combat preimage changed during retirement");
	}
	require(s._completion==XeenEncounterCompletion::VictoryEnded&&s._combatAccounted&&
		s._encounterMarked&&s._encounterInitialized&&s._encounterTerminal&&s._combatEntered&&
		s._entry==XeenEncounterEntry::Diagnostic27&&s._diagnostic27&&d.approach.pending()==0,
		"completed encounter invariants are incomplete");
	require(s._actors.size()==27,"completed actor collection is incomplete");
	const auto &target=s._actors.at(5);
	require(target.id==XeenMonsterIdentity{{XeenSide::Clouds,20},5}&&target.hp==0&&target.x==-128&&target.y==-128&&
		!target.activated&&target.lifecycle==XeenActorLifecycle::Defeated&&target.status==XeenActorStatus::Physical,
		"completed target is not canonical");

	XeenCompletedEncounterAuthority prepared;
	prepared.party=&d.party;prepared.roster=&d.party.roster;prepared.camera=&d.camera;
	prepared.characters=d.party.roster.characters();
	for(std::size_t i=0;i<prepared.combatInputs.size();++i)prepared.combatInputs[i]=d.party.roster.combatInputs(i);
	prepared.activeRosterIds=d.party.party.activeRosterIds();prepared.questItems=d.party.questItems.counts();
	prepared.questFlags=d.party.questFlags.values();prepared.context=d.party.encounterContext;
	prepared.firstSerializedCount=d.party.firstSerializedCount;prepared.effectiveSerializedCount=d.party.effectiveSerializedCount;
	prepared.diagnostics=d.party.diagnostics;prepared.cameraValue=d.camera;prepared.actors=s._actors;
	prepared.objects=s._objects;prepared.events=s._events;prepared.monster=s._completedMonster;

	require(current(t),"completed combat ticket became stale during retirement");
	require(!d.busy&&d.boundary.quiet()&&!s._completedIntegrityUnsafe&&
		s._completion==XeenEncounterCompletion::VictoryEnded,"completed authority changed during retirement");
	if(!d.exact()) {
		d.latchIntegrity();
		throw std::invalid_argument("completed combat preimage changed during retirement");
	}
	static_assert(std::is_nothrow_move_constructible_v<XeenCompletedEncounterAuthority>);
	s._completedAuthority.emplace(std::move(prepared));
	s._completedPublished=true;
	s._completion=XeenEncounterCompletion::VictoryQuiescent;
	s._completedIntegrityUnsafe=false;s._completedFatal=false;s._completedLease=0;s._completedLeaseKind.reset();
	s._combatOwner=nullptr;s._combatApproachState=nullptr;
	d.world._combatCheck={};d.world._combatAuthorized={};
	++s._encounterRevision;++d.generation;
	return d.world.completedTicket(d.party,d.camera);
}
static_assert(std::is_nothrow_copy_assignable_v<XeenCombatResult>);
static_assert(std::is_nothrow_copy_assignable_v<XeenActor>);
} // namespace mmodern
