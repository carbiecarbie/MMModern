#include "app/XeenEncounterFlow.h"
#include <limits>
#include <stdexcept>
#include <type_traits>
#include "games/xeen/XeenCharacterRules.h"

namespace mmodern {
namespace {
struct Busy {
	bool &value;
	explicit Busy(bool &v) : value(v) { value = true; }
	~Busy() { value = false; }
};
std::optional<XeenEncounterAction> mapped(const PlayerAction &input) {
	if (std::holds_alternative<WaitAction>(input)) return XeenEncounterAction::Wait;
	if (const auto *n = std::get_if<NavigationAction>(&input)) switch (*n) {
	case NavigationAction::MoveForward: return XeenEncounterAction::Forward;
	case NavigationAction::MoveBackward: return XeenEncounterAction::Backward;
	case NavigationAction::TurnLeft: return XeenEncounterAction::Left;
	case NavigationAction::TurnRight: return XeenEncounterAction::Right;
	}
	return {};
}
}

bool XeenEncounterFlow::current(const Ticket &t) const noexcept {

	if (_failure || !_journeyPreimage) return false;
	if (!_journeyPreimage->ownersAlive()) { const_cast<XeenEncounterFlow *>(this)->closeJourney(); return false; }
	if (t.generation != _generation || t.boundaryGeneration != _boundary.generation() ||
		_world._sessionState._journeyOwner != this || _world._sessionState._journeyActivity == XeenJourneyActivity::Failed) return false;
	if (_combat) {
		const bool valid=t.combat && _combat->current(*t.combat);
		if (!valid && _combat->phase()==XeenCombatPhase::Failed &&
			_combat->result().failure==XeenCombatFailure::Integrity)
			const_cast<XeenEncounterFlow *>(this)->closeJourney();
		return valid;
	}
	if (!_journeyPreimage->current()) { const_cast<XeenEncounterFlow *>(this)->closeJourney(); return false; }
	return !t.combat && t.state.revision() == _state.revision() && t.state.pending() == _state.pending() &&
		t.state.phase() == _state.phase() && t.state.reason() == _state.reason() &&
		XeenActorApproach::authoritative(_world,_party,_camera,t.state);

}

bool XeenEncounterFlow::adopt(const XeenEncounterResult &r, std::uint64_t generation) noexcept {
	static_assert(std::is_nothrow_copy_assignable<XeenEncounterResult>::value);
	if (generation != _generation || r.revision != _state.revision() ||
		!XeenActorApproach::authoritative(_world, _party, _camera, _state) ||
		r.outcome == XeenEncounterOutcome::Stale || r.outcome == XeenEncounterOutcome::Terminal ||
		r.outcome == XeenEncounterOutcome::Refused) return false;
	_result = r;
	if (_generation != std::numeric_limits<std::uint64_t>::max()) ++_generation;
	if (_state.phase() != XeenEncounterPhase::Exploring) _deadline.reset();
	return true;
}

bool XeenEncounterFlow::fail(const Ticket &entry, XeenEncounterStop) noexcept {
	if (!current(entry)) return false;
	closeJourney();
	return true;
}

void XeenEncounterFlow::guardCallback(const Ticket &entry, const std::function<void()> &callback) {
	if (!current(entry)) throw std::logic_error("Stale encounter callback preimage");
	try {
		if (_combat) _combat->guardCallback(*entry.combat,callback);
		else callback();
		if (!current(entry)) throw std::logic_error("Encounter callback changed owner preimage");
	} catch (...) {
		// Exceptional callbacks must prove the same retained history as returns.
		if (!current(entry)) throw std::logic_error("Encounter callback changed owner preimage");
		throw;
	}
}

bool XeenEncounterFlow::prepareTime(const Ticket &entry, std::uint64_t &now) {
	if (!current(entry)) return false;
	try { guardCallback(entry,[&] { now = _clock(); }); }
	catch (...) { if (!fail(entry, XeenEncounterStop::Preparation)) throw; return false; }
	if (!current(entry)) return false;
	if (now < _lastTime) return false;
	if (now > std::numeric_limits<std::uint64_t>::max() - 100 ||
		_generation > std::numeric_limits<std::uint64_t>::max() - 8) {
		fail(entry, XeenEncounterStop::Overflow); return false;
	}
	return true;
}

void XeenEncounterFlow::schedule(std::uint64_t now) noexcept {
	_deadline = _state.phase() == XeenEncounterPhase::Exploring && (_state.pending() || _regionalWork) ?
		std::optional<std::uint64_t>{now + 100} : std::nullopt;
}

bool XeenEncounterFlow::handle(const PlayerAction &input, std::optional<std::uint64_t> cycle, std::optional<XeenCombat::Ticket> displayed) {
	if (projectilesPending() || _shootIntent || _castingSettlement) return false;
	if (_journey && !_combat && std::holds_alternative<ShootAction>(input)) { const bool accepted=beginShoot();schedule(_lastTime);return accepted; }
	if (_combat) return displayed && _combat->current(*displayed) && handleCombat(input, cycle);
	const auto action = mapped(input);
	if (_busy || !action || _state.phase() != XeenEncounterPhase::Exploring) return false;

	std::uint64_t now;
	if (!prepareTime(ticket(),now)) return false;
	_lastTime = now;
	const auto r = journeyAction(ticket(),*action);
	_actionResult = r; _actionPending = _state.pending(); _inputCycle = cycle;
	if (r.outcome == XeenEncounterOutcome::Refused) {
		if (r.reason == XeenEncounterStop::Envelope) _journeyRefusal = "Four-cell boundary: x=13..14, y=1..2";
		return true;
	}
	if (_state.phase() == XeenEncounterPhase::Exploring &&
		(r.outcome == XeenEncounterOutcome::Accepted || r.outcome == XeenEncounterOutcome::Blocked)) {
		if (!prepareTime(ticket(),now)) return true;
		journeyPulse(ticket());
	}
	if (r.outcome==XeenEncounterOutcome::Blocked) _journeyRefusal="Movement blocked by terrain.";
	schedule(now); return true;

}

bool XeenEncounterFlow::idle(std::optional<std::uint64_t> cycle) {
	if(projectilesPending()) return animateProjectiles();
	if (_combat) return idleCombat(cycle);
	if (_shoot && !monsterReward()) { const bool changed=serviceShoot();schedule(_lastTime);return changed; }
	if (_casting && _casting->effectDone) { const bool changed=serviceCasting();schedule(_lastTime);return changed; }
	if (_busy || _state.phase() != XeenEncounterPhase::Exploring) return false;

	if (itemUseReady()) return serviceItemUse();
	if (_world.sessionState().journeyActivity() == XeenJourneyActivity::Presentation || !_boundary.quiet()) return false;
	std::uint64_t now;
	if (!prepareTime(ticket(),now)) return false;
	const bool due = (_state.pending() || _regionalWork) && _deadline && now >= *_deadline && !(cycle && _inputCycle == cycle);
	const bool cosmetic = now >= _cosmeticDeadline;
	_lastTime = now;
	if (due) { journeyPulse(ticket()); schedule(now); _inputCycle = cycle; }
	if (cosmetic && _state.phase() == XeenEncounterPhase::Exploring) {
		_frame = (_frame + 1) % 8; _cosmeticDeadline = now + 100;
	}
	return due || cosmetic;

}

std::string XeenEncounterFlow::notice() const {
	if(_journey) return consequenceNotice();
	return {};
}

bool XeenEncounterFlow::terminal() const noexcept {
	if (_journey && _failure) return true;
	if (!_combat) return _state.phase() != XeenEncounterPhase::Exploring;
	const auto p = _combat->phase();
	return _failure || p == XeenCombatPhase::Disengaged || p == XeenCombatPhase::Victory || p == XeenCombatPhase::Defeat ||
		p == XeenCombatPhase::SupportStopped || p == XeenCombatPhase::Failed;
}

void XeenEncounterFlow::authorizeCombatCastFrame(const Ticket &t,std::uint64_t input,
        const IndexedFrame::Presentation &frame) {
    if(!_combat || !current(t) || !frame || !input)throw std::logic_error("Combat cast frame unavailable");
    _castFrameTicket=t;_castFrame=frame;_castInput=input;
}
bool XeenEncounterFlow::respondCombatCast(const PlayerAction &action,std::uint64_t input,
        const IndexedFrame::Presentation &frame,const std::function<XeenLearnedSpellNames()> &prepare) {
    if(_busy || !_combat || !_castFrameTicket || !_castFrame || _castFrame!=frame ||
        _castInput!=input || !current(*_castFrameTicket))return false;
    using CI=XeenCombatCastInput;
    const bool begin=std::holds_alternative<CastSpellAction>(action) && !_combat->cast();
    std::optional<CI> response;unsigned index=0;
    if(_combat->cast()) {
        const auto phase=_combat->cast()->phase;
        if(std::holds_alternative<AcknowledgeAction>(action) ||
            (phase==XeenCombatCastPhase::Result && std::holds_alternative<InteractionAction>(action)))response=CI::Enter;
        if(std::holds_alternative<CancelInteractionAction>(action))response=CI::Escape;
        if(const auto *nav=std::get_if<NavigationAction>(&action)) {
            if(*nav==NavigationAction::MoveForward)response=CI::Up;
            if(*nav==NavigationAction::MoveBackward)response=CI::Down;
        }
        if(phase==XeenCombatCastPhase::PartyTarget)if(const auto *member=std::get_if<SelectMemberAction>(&action)) {
            if(member->partyIndex<6){response=CI::PartyTarget;index=unsigned(member->partyIndex);}
        }
        if(phase==XeenCombatCastPhase::Enemy) {
            if(const auto *row=std::get_if<SelectInventorySlotAction>(&action)){response=CI::EnemyTarget;index=unsigned(row->slot);}
            if(const auto *row=std::get_if<SelectCombatTargetAction>(&action)){response=CI::EnemyTarget;index=row->row;}
        }
    }
    if(!begin && !response)return false;
    Busy busy(_busy);
    auto source=*_castFrameTicket->combat;
    _castFrame.reset();_castFrameTicket.reset();_castInput=0; // Consume before any provider.
    XeenCombat::CastResponse authorization(source);
    const auto result=begin ? _combat->beginCast(authorization,prepare) :
        _combat->respondCast(authorization,*response,index,prepare);
    if(result.status==XeenCombatStatus::Refused) {
        _combatCastRefusal=result.failure==XeenCombatFailure::Preparation ? "Cast preparation unavailable; C retries" : "Cast unavailable for this domain or acting book";
        return true;
    }
    _combatCastRefusal.clear();
    if(!acceptCombatResult(result))return false;
    if(_combat->cast() && (_combat->cast()->phase==XeenCombatCastPhase::Projectile ||
        _combat->cast()->phase==XeenCombatCastPhase::Result))observeCombat();
    scheduleCombat(_lastTime);return true;
}

void XeenEncounterFlow::scheduleCombat(std::uint64_t now) {
	_scheduleAfterFrame = true;
	_deadline.reset();
	if (!terminal() && _combat->pending() != XeenCombatWork::None) _deadline = now + 100;
}
void XeenEncounterFlow::presented(const Ticket &entry) {
	if (!_combat || !_scheduleAfterFrame || terminal()) return;
	std::uint64_t now;
	if (!prepareTime(entry,now)) {
		if (current(entry)) fail(entry,XeenEncounterStop::Preparation);
		throw std::runtime_error("Combat frame scheduling failed");
	}
	_lastTime=now;
	_combat->castPresented(*entry.combat,now);
	scheduleCombat(now);
	if (_appearanceAfterFrame) {
		_cosmeticDeadline = now + 100;
		_appearanceAfterFrame = false;
	}
	_scheduleAfterFrame=false;
}
bool XeenEncounterFlow::acceptCombatResult(const XeenCombatResult &result) {
	if (result.status == XeenCombatStatus::Stale) {
		_combatOperationStale = true;
		return false;
	}
	if (result.status == XeenCombatStatus::Refused) return false;
	const auto &adopted = _combat->result();
	if (result.status != adopted.status || result.phase != adopted.phase || result.work != adopted.work ||
		result.revision != adopted.revision || result.generation != adopted.generation)
		throw std::logic_error("Combat operation result was not adopted");
	if(result.ranged && result.status!=XeenCombatStatus::Pending) observeRanged(result.ranged);
	return true;
}
bool XeenEncounterFlow::handoffCombat() {
	if (_combat->phase() == XeenCombatPhase::Engaged) {
		if (!acceptCombatResult(_combat->beginCombat(_combat->ticket()))) return false;
		_deadline.reset();
	}
	return true;
}
bool XeenEncounterFlow::observeCombat() noexcept {
	const auto &r = _combat->result();
	if (r.xpCount) _combatAward = r;
	if (r.operation == XeenCombatOperation::PlayerAttack || r.operation == XeenCombatOperation::Cast || r.operation == XeenCombatOperation::Block ||
		r.operation == XeenCombatOperation::PlayerRun || r.operation == XeenCombatOperation::FinishDisengagement ||
		r.operation == XeenCombatOperation::EnemyAttack) _combatObservation = r;
	// Only a published attack starts an effect. Pending RNG prefixes, retained
	// feedback, round work and redraws cannot restart it.
	if (r.attackOutcome == XeenCombatAttackOutcome::Pending) return false;
	if (r.operation == XeenCombatOperation::EnemyAttack) {
		_appearanceIdentity = r.actingMonster;
		_frame = 8; _appearanceStep = 0; _appearanceAfterFrame = true; return true;
	}
	if ((r.operation == XeenCombatOperation::PlayerAttack || r.operation == XeenCombatOperation::Cast) && r.damage > 0) {
		_appearanceIdentity = r.targetMonster;
		_frame = r.actorHpAfter > 0 ? 11 : 0;
		_appearanceStep = 0; _appearanceAfterFrame = true; return true;
	}
	return false;
}
void XeenEncounterFlow::advanceAppearance() noexcept {
	// Pinned animate3d: enemy 8,9,10,10,10,0; physical hit 11
	// for five advances, then 0. One step per observed 100 ms, no backlog.
	if (_frame == 11) {
		if (++_appearanceStep == 5) _frame = 0;
	} else if (_frame >= 8) {
		constexpr std::uint8_t sequence[]{9,10,10,10,0};
		_frame = sequence[_appearanceStep++];
	} else _frame = (_frame + 1) % 8;
}
bool XeenEncounterFlow::handleCombat(const PlayerAction &input, std::optional<std::uint64_t> cycle) {
	using P = XeenCombatPhase;
	_combatOperationStale = false;
	if (_busy || terminal()) return false;
	const auto phase = _combat->phase();
	if(_combat->cast())return false;
	const bool command = phase == P::PlayerReady &&
		(std::holds_alternative<AttackAction>(input) || std::holds_alternative<BlockAction>(input) || std::holds_alternative<RunAction>(input));
	const auto *target = phase == P::PlayerReady ? std::get_if<SelectCombatTargetAction>(&input) : nullptr;
	if (!command && !target) return false;
	Busy busy(_busy);
	const auto entry = ticket();
	std::uint64_t now;
	if (!prepareTime(entry,now)) {
		if (!current(entry) && !terminal()) _combatOperationStale = true;
		return !current(entry);
	}
	_lastTime = now;
	if (target) {
		if (!acceptCombatResult(_combat->selectTarget(*entry.combat,target->row))) return false;
	} else if (command) {
		if (!acceptCombatResult(_combat->command(*entry.combat,
			std::holds_alternative<AttackAction>(input) ? XeenCombatCommand::Attack : std::holds_alternative<RunAction>(input) ? XeenCombatCommand::Run : XeenCombatCommand::Block))) return false;
	}
	retireCastingFeedback();
	if (!handoffCombat()) return true;
	if (observeCombat()) _cosmeticDeadline = now + 100;
	_inputCycle = cycle;
	_lastTime = now;
	scheduleCombat(now);
	return true;
}
bool XeenEncounterFlow::idleCombat(std::optional<std::uint64_t> cycle) {
	_combatOperationStale = false;
	if (_busy || terminal()) return false;
	Busy busy(_busy);
	const auto entry = ticket();
	std::uint64_t now;
	if (!prepareTime(entry,now)) {
		if (!current(entry) && !terminal()) _combatOperationStale = true;
		return !current(entry);
	}
	const bool due = _deadline && now >= *_deadline && !(cycle && _inputCycle == cycle);
	const bool cosmetic = now >= _cosmeticDeadline;
	bool startedAppearance = false;
	_lastTime = now;
	if (_combat->cast()) {
        const auto phase=_combat->cast()->phase;
        if(_scheduleAfterFrame || (phase!=XeenCombatCastPhase::Preparing && phase!=XeenCombatCastPhase::Projectile))return false;
        const auto result=_combat->serviceCast(*entry.combat,now);
        if(!acceptCombatResult(result))return false;
        if(phase==XeenCombatCastPhase::Preparing && _combat->cast() && _combat->cast()->phase==XeenCombatCastPhase::Projectile && observeCombat())_cosmeticDeadline=now+100;
        scheduleCombat(now);return true;
    }
	if (due) {
		const auto result = _combat->service(*entry.combat);
		if (!acceptCombatResult(result)) return false;
		if (!handoffCombat()) return false;
		_inputCycle = cycle;
		startedAppearance = observeCombat();
		if (startedAppearance) _cosmeticDeadline = now + 100;
		scheduleCombat(now);
	}
	if (cosmetic && !startedAppearance && !terminal()) {
		advanceAppearance();
		_cosmeticDeadline = now + 100;
		// Appearance has its own concrete presentation identity. Retain the
		// gameplay ticket so the acquired frame stays usable during its redraw.
	}
	return due || cosmetic;
}
std::string XeenEncounterFlow::combatNotice() const {
	using P = XeenCombatPhase;
	const auto phase = _combat->phase();
	const auto &r = _combatObservation;
	std::string text = (_combatCastRefusal.empty() ? "T=" : _combatCastRefusal+"\nT=") + std::to_string(_combat->result().minutes) + " | Unsaveable | Esc exits\n";
	auto name = [&](unsigned owner) { return _party.roster.at(owner).name; };
	if (phase == P::Victory) text += "VICTORY\n";
	else if (phase == P::Defeat) text += "DEFEAT - no healing or XP\n";
	else if (phase == P::Failed) text += "FAILED - encounter cannot continue\n";
	else if (phase == P::SupportStopped) text += "SUPPORT STOP - encounter cannot continue\n";
	else if (phase == P::PlayerReady) {
		const auto slot = _combat->participant();
		text += "F" + std::to_string(slot+1) + " " + name(kXeenCombatOwners[slot]) + ": Space Attack / B Block / C Cast\n";
	} else if (phase == P::PendingEnemy) text += "Enemy attack pending (automatic)\n";
	else if (phase == P::PendingRound) text += "Next round pending (automatic)\n";
	else if (phase == P::VictoryAwaitingEnd) text += "Enemy defeated; victory end pending\n";
	else text += "Attack preparation pending (automatic)\n";
	const auto &actors = _world.sessionState().actors();
	if (actors.size() > 5) text += actors[5].statistics->name() + " HP " + std::to_string(actors[5].hp) + "\n";
	if (r.operation == XeenCombatOperation::Block && r.actingOwner) text += name(*r.actingOwner) + " blocks.\n";
	if (r.attackOutcome != XeenCombatAttackOutcome::NotApplicable) {
		text += r.actingOwner ? name(*r.actingOwner) : "Enemy";
		if (r.targetOwner) text += " -> " + name(*r.targetOwner);
		if (r.attackOutcome == XeenCombatAttackOutcome::Pending) text += " attack pending\n";
		else if (r.attackOutcome == XeenCombatAttackOutcome::Miss) text += " misses\n";
		else text += " hits: " + std::to_string(r.damage) + " damage\n";
	}
	if (r.critical) text += "Critical: ";
	for (unsigned i=0;i<r.injuryCount;++i) {
		const auto &d=r.injuries[i];
		if (i) text += "; ";
		text += "-" + std::to_string(d.amount) + " HP " + std::to_string(d.afterHp);
	}
	if (r.injuryCount || r.critical) text += "\n";
	if (r.armorCount) text += "Broken armor: " + std::to_string(r.armorCount) + " slots\n";
	text += "\n"; // Separate retained feedback from the live roster panel.
	for (auto owner:kXeenCombatOwners) {
		const auto &c=_party.roster.at(owner);
		text += name(owner) + " HP " + std::to_string(c.currentHp) + "\n";
		bool condition = false;
		for (unsigned i=0;i<c.conditions.size();++i) if(c.conditions[i]) {
			if (condition) text += " + ";
			// Both flags can survive a critical hit. Keep them visible on the
			// bounded roster row rather than hiding the earlier unconscious flag.
			if (static_cast<XeenCondition>(i) == XeenCondition::Unconscious &&
				c.conditions[static_cast<unsigned>(XeenCondition::Dead)]) text += "Uncon.";
			else text += xeenConditionName(static_cast<XeenCondition>(i));
			condition = true;
		}
		if (!condition) text += "Good";
		if (_combatAward.xpCount) {
			std::uint32_t delta=0;
			for (unsigned i=0;i<_combatAward.xpCount;++i) if (_combatAward.xp[i].owner==owner)
				delta=_combatAward.xp[i].after-_combatAward.xp[i].before;
			text += "\nXP +" + std::to_string(delta);
		}
		text += _combatAward.xpCount ? "\n" : "\n\n";
	}
	if (!text.empty() && text.back() == '\n') text.pop_back();
	return text;
}
}
