#include "app/XeenEncounterFlow.h"
#include <limits>
#include <stdexcept>
#include <type_traits>

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

XeenEncounterFlow::XeenEncounterFlow(XeenWorld &w, XeenPartyState &p, XeenCamera &c,
		const XeenEventPresenter::Clock &clock, const XeenEncounterSetup &setup) :
	_world(w), _party(p), _camera(c), _clock(clock), _events(setup.events), _boundary(w,p,c) {
	if (setup.prepareCombat) {
		_combat = setup.prepareCombat(w,p,c,_boundary);
		if (!_combat || !_combat->boundTo(w,p,c,_boundary) || !preparation() || !setup.validateNormalSprite)
			throw std::invalid_argument("Missing combat preparation providers");
		const auto entry = ticket();
		try {
			setup.validateNormalSprite(8);
			if (!current(entry)) throw std::runtime_error("Stale combat sprite admission");
		} catch (...) { fail(entry, XeenEncounterStop::Preparation); throw; }
		return;
	}
	w.markEncounterSession();
	if (!setup.initialize || !setup.validateNormalSprite) throw std::invalid_argument("Missing encounter admission providers");
	const auto r = setup.initialize(w, p, c, _state);
	if (!adopt(r, 0) || r.outcome != XeenEncounterOutcome::Started)
		throw std::runtime_error("Encounter initialization did not publish current authority");
	const auto entry = ticket();
	try {
		// Own the image number before any provider can invalidate actor storage.
		const auto image = w.sessionState().actors().at(5).statistics.value().image();
		setup.validateNormalSprite(image);
		if (!current(entry)) throw std::runtime_error("Stale encounter sprite admission");
		const auto now = _clock();
		if (!current(entry)) throw std::runtime_error("Stale encounter clock admission");
		if (now > std::numeric_limits<std::uint64_t>::max() - 100)
			throw std::overflow_error("Encounter clock deadline overflow");
		_lastTime = now;
		_cosmeticDeadline = now + 100;
		++_generation;
	} catch (...) { fail(entry, XeenEncounterStop::Preparation); throw; }
}

bool XeenEncounterFlow::current(const Ticket &t) const noexcept {
	if (_combat) return t.combat && t.generation == _generation && _combat->current(*t.combat);
	return t.generation == _generation && t.state.revision() == _state.revision() &&
		t.state.pending() == _state.pending() && t.state.phase() == _state.phase() &&
		t.state.reason() == _state.reason() &&
		XeenActorApproach::authoritative(_world, _party, _camera, t.state);
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

bool XeenEncounterFlow::fail(const Ticket &entry, XeenEncounterStop reason) noexcept {
	if (!current(entry)) return false;
	if (_combat) {
		_combat->fail(*entry.combat);
		try { _boundary.hold(XeenCombatBoundary::Work::PresentationFailure); } catch (...) {}
		_failure = true;
		_deadline.reset();
		if (_generation != std::numeric_limits<std::uint64_t>::max()) ++_generation;
		return true;
	}
	if (_state.phase() == XeenEncounterPhase::Exploring) {
		const auto r = XeenActorApproach::stop(_world, _state, reason);
		if (!adopt(r, entry.generation)) return false;
	} else if (_generation != std::numeric_limits<std::uint64_t>::max()) ++_generation;
	_failure = true;
	_deadline.reset();
	return true;
}

bool XeenEncounterFlow::prepareTime(const Ticket &entry, std::uint64_t &now) {
	try { now = _clock(); }
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
	_deadline = _state.phase() == XeenEncounterPhase::Exploring && _state.pending() ?
		std::optional<std::uint64_t>{now + 100} : std::nullopt;
}

bool XeenEncounterFlow::handle(const PlayerAction &input, std::optional<std::uint64_t> cycle, std::optional<XeenCombat::Ticket> displayed) {
	if (_combat) return displayed && _combat->current(*displayed) && handleCombat(input, cycle);
	const auto action = mapped(input);
	if (_busy || !action || _state.phase() != XeenEncounterPhase::Exploring) return false;
	Busy busy(_busy);
	const auto entry = ticket();
	if (!current(entry)) return false;
	// Diagnostic arithmetic only, never a movement/collision query.
	std::optional<std::pair<int,int>> attempted;
	if (*action == XeenEncounterAction::Forward || *action == XeenEncounterAction::Backward) {
		constexpr int dx[]{0,1,0,-1}, dy[]{1,0,-1,0};
		const auto d = static_cast<unsigned>(_camera.direction);
		if (d < 4 && _camera.x >= 13 && _camera.x <= 14 && _camera.y >= 1 && _camera.y <= 2) {
			const int sign = *action == XeenEncounterAction::Forward ? 1 : -1;
			attempted = std::make_pair(_camera.x + sign * dx[d], _camera.y + sign * dy[d]);
		}
	}
	std::uint64_t now;
	if (!prepareTime(entry, now)) return !current(entry);
	_attempted = attempted;
	_lastTime = now;
	const auto r = XeenActorApproach::action(_world, _party, _camera, _state, *action, _events);
	if (!adopt(r, entry.generation)) return false;
	// Retain the intermediate publication before pulse preparation invokes providers.
	_actionResult = r;
	_actionPending = _state.pending();
	_inputCycle = cycle;
	if (_state.phase() == XeenEncounterPhase::Exploring &&
		(r.outcome == XeenEncounterOutcome::Accepted || r.outcome == XeenEncounterOutcome::Blocked)) {
		// The action is already adopted. A fallible clock observation/preparation for
		// its separate pulse cannot roll it back or authorize a stale continuation.
		const auto pulseEntry = ticket();
		if (!prepareTime(pulseEntry, now)) {
			if (current(pulseEntry)) fail(pulseEntry, XeenEncounterStop::Preparation);
			return true;
		}
		_lastTime = now;
		const auto generation = _generation;
		const auto pulse = XeenActorApproach::pulse(_world, _party, _camera, _state, _events);
		if (!adopt(pulse, generation)) return false;
	}
	schedule(now);
	return true;
}

bool XeenEncounterFlow::idle(std::optional<std::uint64_t> cycle) {
	if (_combat) return idleCombat(cycle);
	if (_busy || _state.phase() != XeenEncounterPhase::Exploring) return false;
	Busy busy(_busy);
	const auto entry = ticket();
	if (!current(entry)) return false;
	std::uint64_t now;
	if (!prepareTime(entry, now)) return !current(entry);
	const bool cosmetic = now >= _cosmeticDeadline;
	const bool pulseDue = _state.pending() && _deadline && now >= *_deadline &&
		!(cycle && _inputCycle && *cycle == *_inputCycle);
	_lastTime = now;
	if (pulseDue) {
		const auto r = XeenActorApproach::pulse(_world, _party, _camera, _state, _events);
		if (!adopt(r, entry.generation)) return false;
		schedule(now);
	}
	if (cosmetic && _state.phase() == XeenEncounterPhase::Exploring) {
		_frame = (_frame + 1) % 8;
		_cosmeticDeadline = now + 100;
		++_generation;
	}
	return pulseDue || cosmetic;
}

std::string XeenEncounterFlow::notice() const {
	if (_combat) return combatNotice();
	std::string text = "M26: Arrows move/turn, . Wait, Esc exit\n";
	text += "Map 20 (" + std::to_string(_camera.x) + "," + std::to_string(_camera.y) + ") ";
	constexpr const char *directions[]{"N","E","S","W"};
	const auto direction = static_cast<unsigned>(_camera.direction);
	text += direction < 4 ? directions[direction] : "?";
	text += " | T=" + std::to_string(_party.encounterContext ? _party.encounterContext->minutes : 0) + " | Unsaveable\n";
	if (_state.phase() == XeenEncounterPhase::Engaged) {
		const auto actor = _world.sessionState().actors().at(5);
		std::string name = actor.statistics ? actor.statistics->name() : "";
		for (unsigned char ch : name) if (ch < 32 || ch > 126) { name.clear(); break; }
		if (name.empty()) name = "Monster " + std::to_string(actor.original.resourceId);
		text += "Engaged: " + name + ". M26 stops before combat.";
	} else if (_state.phase() == XeenEncounterPhase::SupportStopped) {
		text += "Support stop: ";
		switch (_state.reason()) {
		case XeenEncounterStop::Envelope:
			text += "diagnostic envelope";
			if (_attempted) text += " (" + std::to_string(_attempted->first) + "," + std::to_string(_attempted->second) + ")";
			break;
		case XeenEncounterStop::Time: text += "unsupported 960-minute boundary"; break;
		case XeenEncounterStop::Domain: text += "unsupported domain"; break;
		case XeenEncounterStop::Overflow: text += "scheduling overflow"; break;
		case XeenEncounterStop::Preparation: text += "resource/clock preparation failed"; break;
		default: text += "presentation failed"; break;
		}
	} else if (_actionResult.outcome == XeenEncounterOutcome::Blocked) {
		// Retained action facts only; its supplied pulse may already have made a
		// terminal notice above authoritative. No movement query or modal is needed.
		text += "Movement blocked by terrain.";
	} else text += "Events, inventory and saving unavailable.";
	return text;
}

bool XeenEncounterFlow::terminal() const noexcept {
	if (!_combat) return _state.phase() != XeenEncounterPhase::Exploring;
	const auto p = _combat->phase();
	return _failure || p == XeenCombatPhase::Victory || p == XeenCombatPhase::Defeat ||
		p == XeenCombatPhase::SupportStopped || p == XeenCombatPhase::Failed;
}
void XeenEncounterFlow::scheduleCombat(std::uint64_t now) {
	_scheduleAfterFrame = true;
	_deadline.reset();
	if (!terminal() && (_combat->pending() != XeenCombatWork::None ||
		(_combat->phase() == XeenCombatPhase::Approach && state().pending()))) _deadline = now + 100;
}
void XeenEncounterFlow::presented(const Ticket &entry) {
	if (!_combat || !_scheduleAfterFrame || terminal() || preparation()) return;
	std::uint64_t now;
	if (!prepareTime(entry,now)) {
		if (current(entry)) fail(entry,XeenEncounterStop::Preparation);
		throw std::runtime_error("Combat frame scheduling failed");
	}
	_lastTime=now;
	scheduleCombat(now);
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
	return true;
}
bool XeenEncounterFlow::handoffCombat() {
	if (_combat->phase() == XeenCombatPhase::Engaged) {
		if (!acceptCombatResult(_combat->beginCombat(_combat->ticket()))) return false;
		_deadline.reset();
	}
	return true;
}
void XeenEncounterFlow::observeCombat() noexcept {
	const auto &r = _combat->result();
	if (r.xpCount) _combatAward = r;
	if (r.operation == XeenCombatOperation::PlayerAttack || r.operation == XeenCombatOperation::Block ||
		r.operation == XeenCombatOperation::EnemyAttack) _combatObservation = r;
}
bool XeenEncounterFlow::handleCombat(const PlayerAction &input, std::optional<std::uint64_t> cycle) {
	using P = XeenCombatPhase;
	_combatOperationStale = false;
	if (_busy || terminal()) return false;
	const auto phase = _combat->phase();
	const auto movement = mapped(input);
	const bool begin = phase == P::Preparation && std::holds_alternative<BeginEncounterAction>(input);
	const bool command = phase == P::PlayerReady &&
		(std::holds_alternative<AttackAction>(input) || std::holds_alternative<BlockAction>(input));
	if (!begin && !command && !(phase == P::Approach && movement)) return false;
	Busy busy(_busy);
	const auto entry = ticket();
	std::uint64_t now;
	if (!prepareTime(entry,now)) {
		if (!current(entry) && !terminal()) _combatOperationStale = true;
		return !current(entry);
	}
	_lastTime = now;
	if (begin) {
		if (!acceptCombatResult(_combat->beginApproach(*entry.combat))) return false;
	} else if (command) {
		if (!acceptCombatResult(_combat->command(*entry.combat,
			std::holds_alternative<AttackAction>(input) ? XeenCombatCommand::Attack : XeenCombatCommand::Block))) return false;
	}
	else {
		const auto action = _combat->approachAction(*entry.combat,*movement);
		if (!acceptCombatResult(action)) return false;
		_actionPending = state().pending();
		if (action.status == XeenCombatStatus::Advanced && _combat->phase() == P::Approach) {
			const auto pulse = ticket();
			if (!prepareTime(pulse,now)) {
				if (!current(pulse)) {
					if (!terminal()) _combatOperationStale = true;
				} else fail(pulse);
				return true;
			}
			if (!acceptCombatResult(_combat->approachPulse(*pulse.combat))) return true;
		}
	}
	if (!handoffCombat()) return true;
	observeCombat();
	_inputCycle = cycle;
	_lastTime = now;
	if (begin) _cosmeticDeadline = now + 100;
	scheduleCombat(now);
	return true;
}
bool XeenEncounterFlow::idleCombat(std::optional<std::uint64_t> cycle) {
	_combatOperationStale = false;
	if (_busy || preparation() || terminal()) return false;
	Busy busy(_busy);
	const auto entry = ticket();
	std::uint64_t now;
	if (!prepareTime(entry,now)) {
		if (!current(entry) && !terminal()) _combatOperationStale = true;
		return !current(entry);
	}
	const bool due = _deadline && now >= *_deadline && !(cycle && _inputCycle == cycle);
	const bool cosmetic = now >= _cosmeticDeadline;
	_lastTime = now;
	if (due) {
		const auto result = _combat->phase() == XeenCombatPhase::Approach ?
			_combat->approachPulse(*entry.combat) : _combat->service(*entry.combat);
		if (!acceptCombatResult(result)) return false;
		if (!handoffCombat()) return false;
		_inputCycle = cycle;
		observeCombat();
		scheduleCombat(now);
	}
	if (cosmetic && !terminal()) {
		_frame = (_frame + 1) % 8;
		_cosmeticDeadline = now + 100;
		++_generation;
	}
	return due || cosmetic;
}
std::string XeenEncounterFlow::combatNotice() const {
	using P = XeenCombatPhase;
	const auto phase = _combat->phase();
	const auto &r = _combatObservation;
	std::string text = "M27 | Unsaveable | Esc exits | T=" + std::to_string(_combat->result().minutes) + "\n";
	auto name = [&](unsigned owner) { return _party.roster.at(owner).name; };
	if (phase == P::Preparation)
		return text + "Preparation: actors have not begun.\nI inventory / equipment; Enter begins.\nNormal appearance; attack visuals pending 27C.";
	if (phase == P::Approach) return text + "Approach: arrows move/turn, . Wait\nNormal appearance; attack visuals pending 27C.";
	if (phase == P::Victory) text += "VICTORY\n";
	else if (phase == P::Defeat) text += "DEFEAT - no healing or XP\n";
	else if (phase == P::Failed) text += "FAILED - encounter cannot continue\n";
	else if (phase == P::SupportStopped) text += "SUPPORT STOP - encounter cannot continue\n";
	else if (phase == P::PlayerReady) {
		const auto slot = _combat->participant();
		text += "F" + std::to_string(slot+1) + " " + name(kXeenCombatOwners[slot]) + ": Space Attack / B Block\n";
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
	if (r.critical) text += "Critical hit\n";
	for (unsigned i=0;i<r.injuryCount;++i) {
		const auto &d=r.injuries[i];
		text += "Hit " + std::to_string(i+1) + ": -" + std::to_string(d.amount) + " HP " + std::to_string(d.afterHp) + "\n";
	}
	if (r.armorCount) text += "Broken armor: " + std::to_string(r.armorCount) + " slots\n";
	text += "\n"; // Separate retained feedback from the live roster panel.
	for (auto owner:kXeenCombatOwners) {
		const auto &c=_party.roster.at(owner);
		text += name(owner) + " HP " + std::to_string(c.currentHp);
		for (unsigned i=0;i<c.conditions.size();++i) if(c.conditions[i])
			text += " " + std::string(xeenConditionName(static_cast<XeenCondition>(i)));
		if (_combatAward.xpCount) {
			std::uint32_t delta=0;
			for (unsigned i=0;i<_combatAward.xpCount;++i) if (_combatAward.xp[i].owner==owner)
				delta=_combatAward.xp[i].after-_combatAward.xp[i].before;
			text += " XP +" + std::to_string(delta);
		}
		text += "\n";
	}
	if (!text.empty() && text.back() == '\n') text.pop_back();
	return text;
}
}
