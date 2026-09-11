#ifndef MMODERN_XEEN_COMBAT_H
#define MMODERN_XEEN_COMBAT_H
#include "games/xeen/XeenActorApproach.h"
#include "games/xeen/XeenEquipment.h"
#include "games/xeen/XeenItemTransfer.h"
#include <memory>

namespace mmodern {
// Retained headless boundary authority. A later Flow binds its real pending work
// to these leases; it cannot assert that an unrelated owner graph is quiescent.
class XeenCombatBoundary {
public:
	enum class Work { Inventory, Certificate, Event, Reward, PresentationFailure };
	XeenCombatBoundary(const XeenWorld &w,const XeenPartyState &p,const XeenCamera &c):world(&w),party(&p),camera(&c) {}
	XeenCombatBoundary(const XeenCombatBoundary &)=delete;
	XeenCombatBoundary &operator=(const XeenCombatBoundary &)=delete;
	std::uint64_t hold(Work);
	void release(Work,std::uint64_t);
	std::uint64_t generation() const noexcept { return epoch; }
	bool quiet() const noexcept;
	bool preparationReady() const noexcept;
private:
	friend class XeenCombat;
	const XeenWorld *world; const XeenPartyState *party; const XeenCamera *camera;
	std::uint64_t epoch=0;
	std::array<std::uint64_t,5> leases{};
};

// A value cursor: cloning never shares mutable position/state. The immutable tape
// is diagnostic input; no history is appended by retained preparation.
class XeenCombatRandom {
public:
	struct Draw { std::uint32_t lo,hi,value; bool raw=false; };
	explicit XeenCombatRandom(std::uint32_t seed=1);
	explicit XeenCombatRandom(std::vector<Draw> tape);
	// One provider/raw draw, including a rejected conversion. nullopt means reject.
	std::optional<std::uint32_t> draw(std::uint32_t lo,std::uint32_t hi);
	std::uint32_t state() const noexcept { return value; }
	std::size_t position() const noexcept { return offset; }
private:
	std::uint32_t value=1;
	std::size_t offset=0;
	std::shared_ptr<const std::vector<Draw>> tape;
};

enum class XeenCombatPhase { Preparation, Approach, Engaged, PlayerReady, PreparingAction,
	PendingEnemy, PendingRound, VictoryAwaitingEnd, Victory, Defeat, SupportStopped, Failed };
enum class XeenCombatWork { None, Action, Enemy, Round, End };
enum class XeenCombatCommand { Attack, Block };
enum class XeenCombatStatus { Accepted, Pending, Advanced, Refused, Stale, Failed, SupportStopped, Victory, Defeat };
enum class XeenCombatFailure { None, Integrity, Preparation, Time, Observation, Overflow };
enum class XeenCombatOperation { None, Equipment, Transfer, BeginApproach, ApproachAction,
	ApproachPulse, BeginCombat, PlayerAttack, Block, EnemyAttack, Round, End, Failure };
enum class XeenCombatAttackOutcome { NotApplicable, Pending, Miss, HitZeroDamage, HitPositiveDamage };
struct XeenCombatDamage {
	std::uint8_t owner=0;
	int amount=0, beforeHp=0, afterHp=0, beforeAc=0, afterAc=0;
	std::array<std::uint8_t,16> conditions{};
};
struct XeenCombatArmorChange { std::uint8_t owner=0,slot=0; XeenItem before,after; };
struct XeenCombatXp { std::uint8_t owner=0; std::uint32_t before=0,after=0; };
struct XeenCombatResult {
	// Owned observations only. Pending is not a published attack outcome.
	XeenCombatOperation operation=XeenCombatOperation::None;
	XeenCombatAttackOutcome attackOutcome=XeenCombatAttackOutcome::NotApplicable;
	std::optional<std::uint8_t> actingOwner, targetOwner;
	std::optional<XeenMonsterIdentity> actingMonster, targetMonster;
	std::optional<XeenEncounterAction> approachAction;
	bool critical=false;
	XeenCombatStatus status=XeenCombatStatus::Refused;
	XeenCombatPhase phase=XeenCombatPhase::Preparation;
	XeenCombatFailure failure=XeenCombatFailure::None;
	XeenCombatWork work=XeenCombatWork::None;
	std::uint64_t oldRevision=0,revision=0,generation=0;
	int participant=-1, actorHpBefore=0,actorHpAfter=0, damage=0;
	XeenMonsterIdentity monster{20,5};
	std::uint16_t minutes=480;
	std::array<XeenCombatDamage,2> injuries{}; unsigned injuryCount=0;
	std::array<XeenCombatArmorChange,9> armor{}; unsigned armorCount=0;
	std::array<XeenCombatXp,6> xp{}; unsigned xpCount=0;
};

class XeenCombat {
public:
	class Ticket {
		friend class XeenCombat;
		const XeenCombat *owner=nullptr;
		std::uint64_t generation=0,revision=0,boundary=0;
		XeenCombatPhase phase=XeenCombatPhase::Failed;
		XeenCombatWork work=XeenCombatWork::None;
	};
	// Internal diagnostic entry only. Original immutable resources are admitted
	// before attachment; no Application/SDL entry or copied live combat party.
	XeenCombat(XeenWorld &,XeenPartyState &,XeenCamera &,XeenCombatBoundary &,
		const std::vector<std::uint8_t> &chr,const XeenGameplayContext &,
		const std::vector<XeenMonsterRecord> &,const XeenEventFile &,XeenCombatRandom random=XeenCombatRandom(1));
	~XeenCombat();
	XeenCombat(const XeenCombat &)=delete;
	XeenCombat &operator=(const XeenCombat &)=delete;
	Ticket ticket() const noexcept;
	bool current(const Ticket &) const noexcept;
	const XeenCombatResult &result() const noexcept;
	const std::optional<XeenEquipmentResult> &preparationEquipmentResult() const noexcept;
	const std::optional<XeenTransferResult> &preparationTransferResult() const noexcept;
	const XeenEncounterState &approachState() const noexcept;
	XeenCombatPhase phase() const noexcept;
	XeenCombatWork pending() const noexcept;
	int participant() const noexcept;
	const XeenCombatRandom &random() const noexcept;
	XeenEquipmentResult equipment(const Ticket &,std::size_t,XeenInventoryCategory,std::size_t,XeenEquipmentOperation);
	XeenTransferResult transfer(const Ticket &,std::size_t,std::size_t,XeenInventoryCategory,std::size_t);
	XeenCombatResult beginApproach(const Ticket &);
	XeenCombatResult approachAction(const Ticket &,XeenEncounterAction);
	XeenCombatResult approachPulse(const Ticket &);
	XeenCombatResult beginCombat(const Ticket &);
	XeenCombatResult command(const Ticket &,XeenCombatCommand);
	XeenCombatResult service(const Ticket &);
	XeenCombatResult fail(const Ticket &,XeenCombatFailure=XeenCombatFailure::Observation) noexcept;
	// Single-writer replacement notification, including byte-identical ABA.
	void invalidate() noexcept;
	// Optional fallible observation/probe seam. Called after each draw or before
	// publication and checked immediately. An observer never receives a capability.
	void setProbe(std::function<void()>);
private:
	struct Impl;
	std::unique_ptr<Impl> impl;
	XeenCombatResult runApproach(const Ticket &,std::optional<XeenEncounterAction>);
};
}
#endif
