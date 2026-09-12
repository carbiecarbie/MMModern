#include "app/XeenEncounterFlow.h"
#include "games/xeen/XeenPartyVisualState.h"
#include <sstream>
#include <limits>

namespace mmodern {
void XeenEncounterFlow::retainCompleted() {
	if (_generation == std::numeric_limits<std::uint64_t>::max()) throw std::overflow_error("Completed Flow generation exhausted");
	++_generation;
	_completedPreimage = std::make_shared<XeenRestoreGuard>(_world, _party, _camera, _flags);
}

void XeenEncounterFlow::retireVictory() {
	if (_journey) throw std::logic_error("Journey requires mutable retirement");
	if (!_combat || _combat->phase() != XeenCombatPhase::Victory) return;
	if (_busy || !_boundary.quiet()) throw std::logic_error("Victory retirement requires a quiet Flow boundary");
	_completed = _combat->retireCompletedVictory(_combat->ticket());
	_retiredCombatResult = _combat->result();
	_state = _combat->approachState();
	_combat.reset();
	_deadline.reset(); _scheduleAfterFrame = _appearanceAfterFrame = false; _frame = 0;
	retainCompleted();
}

void XeenEncounterFlow::holdCompleted() {
	if (!completed() || !current(ticket())) throw std::logic_error("Stale completed operation");
	if (_completedLease) return;
	_completedLeaseKind = XeenCompletedGuard::Operation;
	_completedLease = _world.holdCompletedGuard(*_completed, _completedLeaseKind, _party, _camera);
	retainCompleted();
}

void XeenEncounterFlow::releaseCompleted() {
	if (!completed() || !current(ticket())) throw std::logic_error("Stale completed operation release");
	if (!_completedLease) return;
	if (!_world.releaseCompletedGuard(*_completed, _completedLeaseKind, _completedLease))
		throw std::logic_error("Completed lease release refused");
	_completedLease = 0;
	_completed = _world.completedTicket(_party, _camera);
	retainCompleted();
}

void XeenEncounterFlow::closeCompleted() noexcept {
	if (!completed() || !_completedPreimage || !_completedPreimage->ownersAlive()) return;
	if (_completedLease) _world.escalateCompletedGuard(*_completed, _completedLeaseKind, _completedLease, XeenCompletedGuard::Fatal);
	else _world.latchCompletedGuard(*_completed, XeenCompletedGuard::Fatal, _party, _camera);
}

XeenCompletedReentry XeenEncounterFlow::reenter(const XeenWorld::MonsterLoader &monsters,
		const XeenWorld::EventLoader &events, const XeenWorld::CompletedPreflight &preflight, const std::function<void()> &boundary) {
	if (!canSave()) throw std::logic_error("Completed re-entry is unavailable");
	// The domain operation owns its exclusive lease. No outer lease is stacked.
	std::optional<XeenCompletedEncounterTicket> released;
	try {
		auto result = _world.reenterCompletedEncounter(*_completed, _party, _camera, _flags, monsters, events, preflight, &released, boundary);
		_completed = _world.completedTicket(_party, _camera);
		retainCompleted();
		return result;
	} catch (...) {
		// Only the domain's checked release receipt authorizes this transition.
		// Never acquire a newer ticket after an arbitrary provider exception.
		if (released) { _completed = *released; retainCompleted(); }
		throw;
	}
}

std::string XeenEncounterFlow::completedNotice(const XeenWorld &w, const XeenPartyState &p,
		const XeenCamera &c, const std::string &feedback) {
	std::ostringstream out;
	out << "Victory completed - F9 save, I inspect\nR revisit, Escape exit\n"
		<< "Map 20 (" << c.x << ',' << c.y << ") T=" << p.encounterContext->minutes
		<< " ctr24=" << p.encounterContext->ctr24 << "\n20/5 defeated; XP accounted once\n";
	if (!feedback.empty()) out << feedback << '\n';
	out << '\n';
	for (auto owner : kXeenCombatOwners) {
		const auto &ch = p.roster.at(owner);
		out << ch.name << " HP " << ch.currentHp << '\n';
		if (ch.conditions[12]) out << "Uncon.";
		if (ch.conditions[12] && ch.conditions[13]) out << " + ";
		if (ch.conditions[13]) out << "Dead";
		if (!ch.conditions[12] && !ch.conditions[13]) out << "Good";
		out << "\nXP " << p.roster.combatInputs(owner)->experience << '\n';
	}
	return out.str();
}

std::string XeenEncounterFlow::completedInspection(const XeenWorld &w, const XeenPartyState &p, const XeenCamera &c) {
	std::ostringstream out;
	out << "Completed Diagnostic27: VictoryQuiescent; accounted Clouds/20/5; entry generation "
		<< w.completedEntryGeneration() << "; camera " << c.mapId << " (" << c.x << ',' << c.y << ") facing " << unsigned(c.direction) << '\n';
	const auto &context = *p.encounterContext;
	out << "Context minute=" << context.minutes << " ctr24=" << context.ctr24 << " day=" << context.day << " year=" << context.year << '\n';
	for (const auto &a : w.sessionState().actors())
		out << "Monster " << a.id.mapId << '/' << a.id.recordIndex << " type=" << a.original.resourceId
			<< " spawn=(" << int(a.original.x) << ',' << int(a.original.y) << ") live=(" << a.x << ',' << a.y << ") HP=" << a.hp
			<< " lifecycle=" << unsigned(a.lifecycle) << " active=" << a.activated << " status=" << unsigned(a.status) << '\n';
	for (unsigned owner = 0; owner < XeenRoster::kCharacterCount; ++owner) {
		const auto &ch = p.roster.at(owner); const auto &inputs = p.roster.combatInputs(owner);
		out << "Owner " << owner << " supplement=" << bool(inputs);
		if (inputs) out << " XP=" << inputs->experience << " Might=" << inputs->might.permanent << '/' << inputs->might.temporary
			<< " Speed=" << inputs->speed.permanent << '/' << inputs->speed.temporary
			<< " Accuracy=" << inputs->accuracy.permanent << '/' << inputs->accuracy.temporary << " tempAC=" << inputs->temporaryAc;
		out << " HP=" << ch.currentHp << " SP=" << ch.currentSp << " conditions=[";
		for (auto condition : ch.conditions) out << unsigned(condition) << ',';
		out << "]\n";
	}
	out << xeenInventoryInspection(p);
	return out.str();
}
}
