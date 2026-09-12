// Adapted bounded rules: ScummVM developers (upstream COPYRIGHT), GPL-3.0-or-later.
// Pin 6814ee9ba54582f5b5adcffab49efbbd8f589edd, engines/mm/xeen/{combat,
// character,interface,party}.cpp and create_xeen/constants.cpp. No commercial data.
#include "games/xeen/XeenCombat.h"
#include "formats/xeen/XeenCharacterFormat.h"
#include "games/xeen/XeenCharacterRules.h"
#include "games/xeen/XeenCombatRules.h"
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
	return same(a.might,b.might)&&same(a.speed,b.speed)&&same(a.accuracy,b.accuracy)&&a.temporaryAc==b.temporaryAc&&a.experience==b.experience;
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
bool terminal(Phase p) { return p==Phase::Victory||p==Phase::Defeat||p==Phase::SupportStopped||p==Phase::Failed; }
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
XeenCombatRandom::XeenCombatRandom(std::uint32_t seed):value(seed) { require(seed!=0,"combat seed must be nonzero"); }
XeenCombatRandom::XeenCombatRandom(std::vector<Draw> values):tape(std::make_shared<const std::vector<Draw>>(std::move(values))) {}
std::optional<std::uint32_t> XeenCombatRandom::draw(std::uint32_t lo,std::uint32_t hi) {
	require(lo<=hi && std::uint64_t(hi)-lo+1<=std::numeric_limits<std::uint32_t>::max(),"invalid random interval");
	std::uint32_t raw;
	if(offset==std::numeric_limits<std::size_t>::max())throw std::overflow_error("combat random cursor exhausted");
	if(tape) {
		require(offset<tape->size(),"combat random tape exhausted"); const auto d=(*tape)[offset++];
		require(d.lo==lo&&d.hi==hi,"combat random request differs from tape");
		if(!d.raw) { require(d.value>=lo&&d.value<=hi,"combat random value outside request"); return d.value; }
		raw=d.value;
	} else { value^=value<<13; value^=value>>17; value^=value<<5; raw=value; ++offset; }
	const std::uint32_t span=hi-lo+1,threshold=(0u-span)%span;
	if(raw<threshold) return {}; return lo+raw%span;
}

struct XeenCombat::Impl {
	XeenCombat *owner;
	XeenWorld &world; XeenPartyState &party; XeenCamera &camera; XeenCombatBoundary &boundary;
	// Exact preimage certificate only: never assigned back to a live owner.
	XeenPartyState expected;
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
	std::array<int,7> order{};
	std::array<bool,7> acted{};
	std::array<bool,6> blocked{};
	int turn=-1;
	XeenCombatResult last;
	std::optional<XeenEquipmentResult> equipmentResult;
	std::optional<XeenTransferResult> transferResult;
	enum class Step { Weapon, PlayerHit, Target, Fallback, EnemyRoll, FirstDice, HitParameter, SecondDice, Done };
	struct Candidate {
		XeenCombatRandom rng;
		std::uint64_t boundary=0;
		Step step=Step::Weapon;
		int target=-1,slot=0,dice=0,sides=0,weapon=0,hit=0,damage=0,roll=0,applications=0;
		unsigned attacks=1;
		int hitBase=0,accumulated=0;
		// One target's candidate, not an authoritative combat party.
		std::optional<XeenCharacter> injured;
		XeenCombatResult result;
	};
	std::optional<Candidate> candidate;
	Impl(XeenCombat *o,XeenWorld &w,XeenPartyState &p,XeenCamera &c,XeenCombatBoundary &b,
		const XeenGameplayContext &ctx,const std::vector<XeenMonsterRecord> &s,const XeenEventFile &e,XeenCombatRandom r):
		owner(o),world(w),party(p),camera(c),boundary(b),expected(p),expectedCamera(c),initialContext(ctx),
		objects(w.sessionState().disabledObjects()),removedEvents(w.sessionState().disabledEvents()),statistics(s),events(e),rng(std::move(r)) {}
	auto &session() { return world._sessionState; }
	std::uint64_t revision() const { return world._sessionState._encounterRevision; }
	bool exact() const {
		if(!same(camera,expectedCamera)||party.party.activeRosterIds()!=expected.party.activeRosterIds()||
			party.questItems.counts()!=expected.questItems.counts()||party.questFlags.values()!=expected.questFlags.values()||
			party.firstSerializedCount!=expected.firstSerializedCount||party.effectiveSerializedCount!=expected.effectiveSerializedCount||
			party.diagnostics!=expected.diagnostics||bool(party.encounterContext)!=bool(expected.encounterContext)) return false;
		if(party.encounterContext&&!same(*party.encounterContext,*expected.encounterContext)) return false;
		for(unsigned i=0;i<30;++i) if(!same(party.roster.at(i),expected.roster.at(i))) return false;
		if(!party.roster.combatMarked()||!world._sessionState._diagnostic27||world._sessionState._combatOwner!=owner) return false;
		for(unsigned i=0;i<30;++i) {
			const auto pos=std::find(kXeenCombatOwners.begin(),kXeenCombatOwners.end(),i);
			const auto &v=party.roster.combatInputs(i);
			if(pos==kXeenCombatOwners.end() || !attached) { if(v) return false; }
			else if(!v||!same(*v,inputs[pos-kXeenCombatOwners.begin()])) return false;
		}
		const auto &s=world.sessionState();
		if(s.disabledObjects()!=objects||s.disabledEvents()!=removedEvents||s.actors().size()!=actors.size()) return false;
		for(unsigned i=0;i<actors.size();++i) if(!same(actors[i],s.actors()[i])) return false;
		return true;
	}
	XeenCharacter &character(int participant) { return party.roster.at(kXeenCombatOwners.at(participant)); }
	int armor(const XeenCharacter &c,int participant) { return Rules::combatArmorClass(c,inputs.at(participant),{610}); }
	void sortOrder() {
		std::array<int,7> speeds{};
		for(int i=0;i<6;++i) speeds[i]=Rules::effectivePhysical(character(i),inputs[i],Rules::PhysicalAttribute::Speed,{610});
		speeds[6]=session()._actors.at(5).statistics->speed();
		for(int i=0;i<7;++i) order[i]=i;
		std::sort(order.begin(),order.end(),[&](int a,int b){return speeds[a]!=speeds[b]?speeds[a]>speeds[b]:a<b;});
	}
	void selectNext() noexcept {
		for(auto i:order) if(!acted[i] && (i==6 || character(i).canAct())) {
			turn=i; phase=i==6?Phase::PendingEnemy:Phase::PlayerReady; work=i==6?Work::Enemy:Work::None; return;
		}
		turn=-1; phase=Phase::PendingRound; work=Work::Round;
	}
	bool defeated() { for(int i=0;i<6;++i) if(character(i).canAct()) return false; return true; }
	void terrainAdmission() {
		// Immutable approach admission separated from Good-only character admission.
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
			terrainAdmission();world._combatCheck={};
		}catch(...){world._combatCheck={};throw;}
	}
	XeenCombatResult observation(Status status) const noexcept {
		XeenCombatResult r; r.status=status;r.phase=phase;r.work=work;r.revision=revision();r.generation=generation;
		r.participant=turn;r.minutes=party.encounterContext?party.encounterContext->minutes:480; return r;
	}
	XeenCombatResult adopt(XeenCombatResult r,Status status=Status::Advanced) noexcept {
		r.status=status;r.phase=phase;r.work=work;r.revision=revision();r.generation=generation;
		r.minutes=party.encounterContext?party.encounterContext->minutes:480;last=r;return r;
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
		const int hp=ch.currentHp-c.damage;
		require(hp>=std::numeric_limits<std::int16_t>::min(),"combat HP outside i16");ch.currentHp=static_cast<std::int16_t>(hp);
		if(hp<1) {
			const bool dead=Rules::maxHp(ch,{610})+hp<1;
			ch.conditions[dead?13:12]=1;
			if(dead||hp<=-10) for(auto &item:ch.armor) if(item.id&&item.frame) item.state|=0x80;
		}
		d.afterHp=hp;d.afterAc=armor(ch,c.target);d.conditions=ch.conditions;
		r.injuries.at(r.injuryCount++)=d;c.damage=0;c.dice=0;++c.applications;
	}
};

namespace {
void validateOriginal(const XeenPartyState &p,const std::vector<std::uint8_t> &chr,const std::array<XeenCombatInputs,6> &inputs) {
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
	validateOriginal(d.expected,chr,d.inputs);
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
XeenCombat::~XeenCombat() {
	if (!impl) return;
	auto &d=*impl;
	if (d.world._sessionState._combatOwner==this) {
		d.world._combatCheck={};d.world._combatAuthorized={};
		d.world._sessionState._combatOwner=nullptr;
		d.world._sessionState._combatApproachState=nullptr;
	}
}
XeenCombat::Ticket XeenCombat::ticket() const noexcept {
	Ticket t;const auto &d=*impl;t.owner=this;t.incarnation=d.world._incarnation;
	t.generation=d.generation;t.revision=d.revision();
	t.boundary=d.boundary.generation();t.phase=d.phase;t.work=d.work;return t;
}
bool XeenCombat::current(const Ticket &t) const noexcept {
	const auto &d=*impl;return t.owner==this&&t.incarnation==d.world._incarnation&&
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
XeenCombatResult XeenCombat::fail(const Ticket &t,Failure f) noexcept {
	auto &d=*impl;if(!current(t))return d.observation(Status::Stale);
	if(terminal(d.phase)) {
		if(d.phase==Phase::Victory&&f==Failure::Integrity&&
			d.session()._completion==XeenEncounterCompletion::VictoryEnded&&
			!d.session()._completedIntegrityUnsafe)d.latchIntegrity();
		return d.last;
	}
	auto r=d.observation(Status::Failed);r.oldRevision=d.revision();r.failure=f;r.operation=Operation::Failure;
	d.phase=f==Failure::Time?Phase::SupportStopped:Phase::Failed;d.work=Work::None;d.candidate.reset();
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
			XeenActorApproach::authoritative(d.world,d.party,d.camera,d.approach)&&d.approach.pending()==0&&
			d.approach.reason()==XeenEncounterStop::None,"combat handoff is not authoritative");
		const auto view=XeenActorApproach::classify(d.session()._actors,d.camera);
		require(view.slots[0]&&*view.slots[0]==XeenMonsterIdentity{20,5},"combat requires selected original record5");
		const auto &a=d.session()._actors.at(5);
		require(a.x==d.camera.x&&a.y==d.camera.y&&a.hp>0&&a.lifecycle==XeenActorLifecycle::Present&&a.status==XeenActorStatus::Physical,"invalid live combat target");
		d.resourcesFor(t);d.sortOrder();d.probeFor(t);
		auto r=d.observation(Status::Advanced);r.oldRevision=d.revision();
		r.operation=Operation::BeginCombat;
		d.session()._combatEntered=true;d.acted.fill(false);d.blocked.fill(false);d.selectNext();d.published();
		return d.adopt(r);
	}catch(...){return fail(t,Failure::Preparation);}
}
XeenCombatResult XeenCombat::command(const Ticket &t,XeenCombatCommand action) {
	auto &d=*impl;if(!current(t))return d.observation(Status::Stale);
	if(d.busy||d.phase!=Phase::PlayerReady||(action!=XeenCombatCommand::Attack&&action!=XeenCombatCommand::Block))return d.observation(Status::Refused);
	if(!d.exact()||!d.boundary.quiet())return fail(t,Failure::Integrity);
	try {d.capacity();}catch(...){return fail(t,Failure::Overflow);}
	// Consume this intent before any random/provider callback.
	d.phase=Phase::PreparingAction;d.work=Work::Action;++d.generation;const auto entry=ticket();
	Busy busy(d.busy);
	try {
		d.resourcesFor(entry);
		if(action==XeenCombatCommand::Block) {
			d.probeFor(entry);auto r=d.observation(Status::Advanced);r.oldRevision=d.revision();r.participant=d.turn;
			r.operation=Operation::Block;r.actingOwner=kXeenCombatOwners[d.turn];
			d.blocked[d.turn]=true;d.acted[d.turn]=true;d.selectNext();d.published();return d.adopt(r);
		}
		d.candidate.emplace();auto &c=*d.candidate;c.rng=d.rng;c.boundary=d.boundary.generation();c.result=d.observation(Status::Pending);
		c.result.oldRevision=d.revision();c.result.participant=d.turn;
		c.result.operation=Operation::PlayerAttack;c.result.attackOutcome=AttackOutcome::Pending;
		c.result.actingOwner=kXeenCombatOwners[d.turn];c.result.targetMonster=XeenMonsterIdentity{20,5};
		c.result.actorHpBefore=d.session()._actors.at(5).hp;c.result.actorHpAfter=c.result.actorHpBefore;
		constexpr int divisors[]{1,2,2,3,4,2,2,1,3,2};
		const auto &ch=d.character(d.turn);
		c.hit=Rules::physicalBonus(Rules::effectivePhysical(ch,d.inputs[d.turn],Rules::PhysicalAttribute::Accuracy,{610}))+5+
			ch.currentLevel()/divisors[static_cast<unsigned>(ch.characterClass)];
		c.hitBase=c.hit;c.attacks=xeenCombatAttackCount(ch.characterClass,ch.currentLevel());
		return d.adopt(c.result,Status::Pending);
	}catch(...){return fail(entry,Failure::Preparation);}
}

XeenCombatResult XeenCombat::service(const Ticket &t) {
	auto &d=*impl;if(!current(t))return d.observation(Status::Stale);
	if(d.busy||d.work==Work::None||terminal(d.phase))return d.observation(Status::Refused);
	if(!d.exact()||!d.boundary.quiet())return fail(t,Failure::Integrity);
	Busy busy(d.busy);
	try {
		d.capacity();
		if(d.candidate&&d.candidate->boundary!=d.boundary.generation())return fail(t,Failure::Integrity);
		if(d.work==Work::Round||d.work==Work::End) {
			if(d.party.encounterContext->minutes>=959)return fail(t,Failure::Time);
			if(d.work==Work::End)d.resourcesFor(t);
			d.probeFor(t);auto r=d.observation(Status::Advanced);r.oldRevision=d.revision();
			r.operation=d.work==Work::Round?Operation::Round:Operation::End;
			if(d.work==Work::Round) {
				// Same pure M26 kernel, immutable bounded terrain contract, all actors.
				d.world._combatCheck=[&]{require(current(t)&&d.exact(),"round resource preimage changed");};
				d.terrainAdmission();
				auto moved=XeenActorApproach::move(d.session()._actors,d.camera,[&](const XeenActor &a,int x,int y){
					const auto cell=d.world.sampleCell(a.id.mapId,x,y);
					return cell&&cell->cell->rawWord==0x31&&cell->cell->rawAttributes==0?XeenMonsterTerrain::Allowed:XeenMonsterTerrain::Unsupported;
				});
				d.world._combatCheck={};
				const auto view=XeenActorApproach::classify(moved,d.camera);
				require(view.slots[0]&&*view.slots[0]==XeenMonsterIdentity{20,5},"round lost original target");
				for(unsigned i=0;i<moved.size();++i)require(same(moved[i],d.actors[i]),"round isolation changed");
				d.sortOrder();d.probeFor(t);
				d.session()._actors.swap(moved);d.acted.fill(false);d.blocked.fill(false);d.selectNext();
			} else {
				d.phase=Phase::Victory;d.work=Work::None;
				d.session()._completion=XeenEncounterCompletion::VictoryEnded;
				d.session()._completedMonster=XeenMonsterIdentity{{XeenSide::Clouds,20},5};
			}
			++d.party.encounterContext->minutes;++d.expected.encounterContext->minutes;d.published();
			return d.adopt(r,d.phase==Phase::Victory?Status::Victory:Status::Advanced);
		}
		d.resourcesFor(t);
		if(!d.candidate) {
			require(d.work==Work::Enemy,"missing action continuation");d.candidate.emplace();auto &c=*d.candidate;
			c.rng=d.rng;c.boundary=d.boundary.generation();c.step=Impl::Step::Target;c.result=d.observation(Status::Pending);c.result.oldRevision=d.revision();c.result.participant=6;
			c.result.operation=Operation::EnemyAttack;c.result.attackOutcome=AttackOutcome::Pending;
			c.result.actingMonster=XeenMonsterIdentity{20,5};
		}
		auto &c=*d.candidate;using Step=Impl::Step;
		unsigned draws=0;
		auto draw=[&](unsigned lo,unsigned hi)->std::optional<unsigned>{
			if(draws==64)return {}; ++draws;auto n=c.rng.draw(lo,hi);d.probeFor(t);return n;
		};
		const auto &monster=*d.session()._actors.at(5).statistics;
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
				if(++c.dice==int(monster.strikes())){const auto previous=c.step;d.injury(c);c.step=previous==Step::FirstDice?Step::HitParameter:Step::Done;}break;
			}
			case Step::HitParameter: {
				auto n=draw(1,monster.hitParameter());if(!n)break;
				const int threshold=d.armor(*c.injured,c.target)+(d.blocked[c.target]?d.character(c.target).currentLevel()/2+15:10);
				c.step=c.roll+int(monster.hitParameter()/4+*n)>=threshold?Step::SecondDice:Step::Done;break;
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
		if(lethal) {
			require(!d.session()._combatAccounted,"monster accounting already consumed");unsigned eligible=0;
			for(int i=0;i<6;++i)if(xeenCombatXpEligible(d.character(i).worstCondition()))++eligible;
			require(eligible!=0,"lethal action has no XP recipient");
			for(int i=0;i<6;++i){if(!xeenCombatXpEligible(d.character(i).worstCondition()))continue;
				const auto xp=xeenCombatExperience(monster.experience(),eligible,d.character(i).permanentLevel,d.inputs[i].experience);
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
			auto &a=d.session()._actors[5];a.hp=r.actorHpAfter;
			if(lethal) {
				a.lifecycle=XeenActorLifecycle::Defeated;a.x=a.y=-128;a.activated=false;d.session()._combatAccounted=true;
				for(unsigned k=0;k<r.xpCount;++k) {
					const auto &xp=r.xp[k];d.party.roster._combatInputs[xp.owner]->experience=xp.after;
					for(unsigned i=0;i<6;++i)if(kXeenCombatOwners[i]==xp.owner)d.inputs[i].experience=xp.after;
				}
				d.phase=Phase::VictoryAwaitingEnd;d.work=Work::End;
			} else {d.acted[d.turn]=true;d.selectNext();}
			d.actors[5]=a;
		} else {
			if(c.injured) {
				auto &ch=d.character(c.target);auto &expected=d.expected.roster.at(ch.rosterId);
				ch.currentHp=c.injured->currentHp;ch.conditions=c.injured->conditions;ch.armor=c.injured->armor;
				expected.currentHp=ch.currentHp;expected.conditions=ch.conditions;expected.armor=ch.armor;
			}
			if(d.defeated()){d.phase=Phase::Defeat;d.work=Work::None;}else{d.acted[6]=true;d.selectNext();}
		}
		d.rng=c.rng;d.candidate.reset();d.published();return d.adopt(r,d.phase==Phase::Defeat?Status::Defeat:Status::Advanced);
	}catch(const IntegrityError &){d.world._combatCheck={};return fail(t,Failure::Integrity);}
	catch(...){d.world._combatCheck={};return fail(t,Failure::Preparation);}
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
