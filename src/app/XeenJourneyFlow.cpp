#include "app/XeenEncounterFlow.h"
#include "games/xeen/XeenJourneyRules.h"
#include "games/xeen/XeenJourneyCapture.h"
#include "games/xeen/XeenInventoryView.h"
#include <limits>
#include <stdexcept>
#include <sstream>

namespace mmodern {
namespace {
struct BusyJourney { bool &value; explicit BusyJourney(bool &v) : value(v) { value = true; } ~BusyJourney() { value = false; } };
}
XeenEncounterFlow::XeenEncounterFlow(XeenWorld &w, XeenPartyState &p, XeenCamera &c, const XeenGameFlags &flags,
		const XeenEventPresenter::Clock &clock, const XeenJourneySetup &setup) :
	_journey(true), _journeyStatistics(setup.statistics), _world(w), _party(p), _camera(c), _flags(flags),
	_clock(clock), _events(_journeyEvents), _boundary(w,p,c) {
	_journeyEvents = setup.events;
	_journeyCapture.reset(new XeenJourneyCapture(w,p,c,_state,_boundary,_busy,_journeyPreimage));
	if (w.hasEncounterState() || p.roster.combatMarked() || p.encounterContext || w._combatCheck || w._combatAuthorized)
		throw std::invalid_argument("Journey requires fresh uncoordinated owners");
	try {
		// Retain lifetime controls before installing callbacks, including allocation failure paths.
		retainJourney();
		w._sessionState._encounterMarked = true;
		w._sessionState._journeyOwner = this;
		w._sessionState._journeyActivity = XeenJourneyActivity::Attachment;
		w._combatCheck = [this] { _journeyPreimage->check(); };
		retainJourney();
		{
			XeenRestoreGuard::Providers providers(*_journeyPreimage,w);
			_result = XeenActorApproach::initializeJourney(w,p,c,_state,setup.characters,setup.context,
				_journeyStatistics,_events,setup.seed);
		}
		w._combatCheck = {};
		_journeyCapture->admittedActors = w.sessionState().actors();
		auto &s = w._sessionState;
		s._journeyOwner = this; s._combatApproachState = &_state;
		s._journeyActivity = XeenJourneyActivity::Presentation;
		++s._journeyGeneration; ++_generation;
		retainJourney();
		w._journeyCapture = _journeyCapture;
	} catch (...) { closeJourney(); throw; }
}
XeenEncounterFlow::~XeenEncounterFlow() { if (_journey) closeJourney(); }
XeenEncounterFlow::XeenEncounterFlow(XeenWorld &w, XeenPartyState &p, XeenCamera &c, const XeenGameFlags &f,
		const XeenEventPresenter::Clock &clock, XeenJourneyRestoreTag) :
	_journey(true), _world(w), _party(p), _camera(c), _flags(f), _clock(clock), _events(_journeyEvents), _boundary(w,p,c) {
	const auto binding = w._journeyRestoration;
	if (!binding || !binding->guard || !binding->guard->current() ||
		&binding->guard->w != &w || &binding->guard->p != &p || &binding->guard->c != &c || &binding->guard->f != &f ||
		!w.sessionState().journey() ||
		w._sessionState._journeyActivity != XeenJourneyActivity::Unbound || w._sessionState._journeyOwner)
		throw std::logic_error("Journey restoration binding is unavailable or stale");
	// SaveState prepared all storage before publication. This handoff only binds
	// fresh final-owner coordination; it performs no allocation or resource work.
	binding->guard->check();
	_journeyCapture.swap(binding->capture);
	_journeyCapture->bind(w,p,c,_state,_boundary,_busy,_journeyPreimage);
	_journeyStatistics.swap(binding->statistics); std::swap(_journeyEvents,binding->events);
	_journeyPreimage.swap(binding->guard);
	_state._world = &w; _state._party = &p; _state._camera = &c; _state._revision = 1;
	w._sessionState._journeyOwner = this; w._sessionState._combatApproachState = &_state;
	w._sessionState._journeyActivity = XeenJourneyActivity::Presentation;
	++w._sessionState._journeyGeneration; ++_generation;
	_journeyPreimage->adoptJourneyCoordination();
	w._journeyCapture = _journeyCapture; w._journeyRestoration.reset();
}
void XeenEncounterFlow::retainJourney() {
	_journeyPreimage = std::make_shared<XeenRestoreGuard>(_world,_party,_camera,_flags);
	if (_journeyCapture) _journeyCapture->generation = _boundary.generation();
}
void XeenEncounterFlow::adoptJourneyFlowBorrow() {
	// EventFlow checked this guard immediately before acquiring its one borrow.
	// Admit exactly that revision increment, retaining every value and identity.
	_journeyPreimage->adoptJourneyBorrowRelease();
	_journeyPreimage->check();
}
bool XeenEncounterFlow::journeyCapacity() noexcept {
	if (_generation < std::numeric_limits<std::uint64_t>::max()-2 &&
		_world._sessionState._journeyGeneration < std::numeric_limits<std::uint64_t>::max()-2) return true;
	closeJourney(); return false;
}
void XeenEncounterFlow::closeJourney() noexcept {
	_failure = true;
	if (_journeyCapture) _journeyCapture->closed = true;
	if (!_journeyPreimage || !_journeyPreimage->worldAlive()) return;
	auto &s = _world._sessionState;
	if (s._journeyOwner && s._journeyOwner != this) return;
	s._journeyActivity = XeenJourneyActivity::Failed;
	_world._combatCheck = {}; _world._combatAuthorized = {};
	// Keep the failed binding: destruction is never a new quiet boundary.
}
bool XeenEncounterFlow::journeyQuiet() const noexcept {
	return _journey && !_busy && !_combat && !_failure && _boundary.quiet() && current(ticket()) &&
		_world.journeyCaptureEligible(_party,_camera);
}
bool XeenEncounterFlow::journeyMutable() const noexcept {
	return _journey && !_busy && !_combat && !_failure && current(ticket()) &&
		_state.pending() == 0 && _state.phase() == XeenEncounterPhase::Exploring &&
		_world.sessionState().journeyActivity() == XeenJourneyActivity::Quiet;
}
std::uint64_t XeenEncounterFlow::holdJourneyWork(XeenCombatBoundary::Work work) {
	if (!journeyMutable()) throw std::logic_error("Journey modal boundary unavailable");
	return _boundary.hold(work);
}
void XeenEncounterFlow::releaseJourneyWork(XeenCombatBoundary::Work work, std::uint64_t lease) {
	if (!_journey || _busy || _combat || !current(ticket())) throw std::logic_error("Stale Journey modal release");
	_boundary.release(work,lease);
	_journeyCapture->generation = _boundary.generation();
}
void XeenEncounterFlow::holdJourneyFrame() {
	if (!_journey || _busy || _combat || !current(ticket()) || !journeyCapacity()) throw std::logic_error("Journey frame boundary unavailable");
	auto &s = _world._sessionState;
	if (s._journeyActivity == XeenJourneyActivity::Presentation) return;
	if (s._journeyActivity != XeenJourneyActivity::Quiet && s._journeyActivity != XeenJourneyActivity::Approach)
		throw std::logic_error("Journey frame cannot replace active work");
	s._journeyActivity = XeenJourneyActivity::Presentation;
	++s._journeyGeneration; ++_generation;
	_journeyFramePrepared = false;
	_journeyPreimage->adoptJourneyCoordination();
}
void XeenEncounterFlow::journeyRead(const std::function<void()> &operation) {
	if (!journeyMutable() || !_boundary.quiet()) throw std::logic_error("Journey interaction unavailable");
	holdJourneyFrame();
	BusyJourney busy(_busy);
	try {
		XeenRestoreGuard::Providers providers(*_journeyPreimage,_world);
		operation(); _journeyPreimage->check();
	} catch (...) { if (!_journeyPreimage->current()) closeJourney(); throw; }
}
std::string XeenEncounterFlow::journeyInspection() const {
	std::ostringstream out;
	out << "Journey " << xeenInventoryInspection(_party);
	out << "Camera " << _camera.mapId << ' ' << _camera.x << ' ' << _camera.y << ' ' << unsigned(_camera.direction)
		<< " pending=" << _state.pending() << " combat=" << bool(_combat) << " seed=" << _world.sessionState().skeletonSeed() << '\n';
	const auto &c = *_party.encounterContext;
	out << "Context profile=" << unsigned(c.profile) << " difficulty=" << unsigned(c.difficulty)
		<< " minutes=" << c.minutes << " ctr24=" << c.ctr24 << " day=" << c.day << " year=" << c.year
		<< " rested=" << c.rested << " newDay=" << c.newDay << " effects=";
	for (auto v:c.effects) out << unsigned(v) << ',';
	out << " light/resistances="; for (auto v:c.lightAndResistances) out << v << ','; out << '\n';
	for (unsigned owner=0;owner<30;++owner) {
		const auto &v = *_party.roster.combatInputs(owner);
		out << "Supplement " << owner << " Might=" << v.might.permanent << '/' << v.might.temporary
			<< " Speed=" << v.speed.permanent << '/' << v.speed.temporary << " Accuracy=" << v.accuracy.permanent << '/' << v.accuracy.temporary
			<< " temporaryAC=" << v.temporaryAc << " XP=" << v.experience << '\n';
	}
	for (const auto &a : _world.sessionState().actors())
		out << "Actor " << a.id.recordIndex << ' ' << a.x << ' ' << a.y << " HP=" << a.hp
			<< " active=" << a.activated << " lifecycle=" << unsigned(a.lifecycle) << " status=" << unsigned(a.status)
			<< " accounted=" << _world.sessionState().accountedMonsters().count(a.id) << '\n';
	out << "Game flags="; for (auto v:_flags.values()) out << (v?'1':'0');
	out << "\nQuest flags="; for (auto v:_party.questFlags.values()) out << (v?'1':'0');
	out << "\nQuest counts="; for (auto v:_party.questItems.counts()) out << v << ',';
	out << "\nDisabled objects="; for (auto id:_world.sessionState().disabledObjects()) out << id.mapId << ':' << id.recordIndex << ',';
	out << "\nDisabled events="; for (auto id:_world.sessionState().disabledEvents()) out << id.mapId << ':' << id.recordIndex << ',';
	out << '\n';
	return out.str();
}
XeenEncounterFlow::Ticket XeenEncounterFlow::beginJourneySave() {
	if (!journeyQuiet() || !journeyCapacity()) throw std::logic_error("Journey save boundary unavailable");
	_world._sessionState._journeyActivity = XeenJourneyActivity::Saving;
	++_world._sessionState._journeyGeneration; ++_generation;
	_journeyPreimage->adoptJourneyCoordination();
	return ticket();
}
bool XeenEncounterFlow::journeySaveCurrent(const Ticket &t) const noexcept {
	return _journey && !_busy && !_combat && !_failure && current(t) && _boundary.quiet() &&
		_world.sessionState().journeyActivity() == XeenJourneyActivity::Saving;
}
bool XeenEncounterFlow::endJourneySave(const Ticket &t) noexcept {
	if (!journeySaveCurrent(t)) return false;
	_world._sessionState._journeyActivity = XeenJourneyActivity::Quiet;
	++_world._sessionState._journeyGeneration; ++_generation;
	_journeyPreimage->adoptJourneyCoordination();
	_journeyCapture->generation = _boundary.generation();
	return true;
}
bool XeenEncounterFlow::presentJourney(const Ticket &entry) {
	if (!_journey || _busy || _combat || !_journeyFramePrepared || !current(entry) ||
		_world.sessionState().journeyActivity() != XeenJourneyActivity::Presentation) return false;
	if (!journeyCapacity()) return false;
	auto &s = _world._sessionState;
	s._journeyActivity = _state.pending() ? XeenJourneyActivity::Approach : XeenJourneyActivity::Quiet;
	++s._journeyGeneration; ++_generation;
	_journeyFramePrepared = _journeyFrameRetry = false;
	_journeyPreimage->adoptJourneyCoordination();
	_journeyCapture->generation = _boundary.generation();
	return true;
}
bool XeenEncounterFlow::prepareJourneyFrame(const Ticket &entry, const std::function<void()> &compose) {
	if (!_journey || _busy || _combat || !current(entry) || !compose ||
		_world.sessionState().journeyActivity() != XeenJourneyActivity::Presentation) return false;
	BusyJourney busy(_busy);
	_journeyFramePrepared = false;
	try {
		XeenRestoreGuard::Providers providers(*_journeyPreimage,_world);
		compose(); _journeyPreimage->check();
		_journeyFramePrepared = true; return true;
	} catch (...) {
		if (!_journeyPreimage->current() || _journeyFrameRetry) closeJourney();
		else _journeyFrameRetry = true;
		return false;
	}
}
XeenEncounterResult XeenEncounterFlow::journeyAction(const Ticket &entry, XeenEncounterAction action) {
	return advanceJourney(entry,action);
}
XeenEncounterResult XeenEncounterFlow::journeyPulse(const Ticket &entry) { return advanceJourney(entry,{}); }
XeenEncounterResult XeenEncounterFlow::advanceJourney(const Ticket &entry, std::optional<XeenEncounterAction> action) {
	XeenEncounterResult refused;
	if (!_journey || _busy || _combat || !current(entry) || !_boundary.quiet() ||
		_state.phase() != XeenEncounterPhase::Exploring) return refused;
	auto &s = _world._sessionState;
	if (s._journeyActivity != XeenJourneyActivity::Quiet && s._journeyActivity != XeenJourneyActivity::Approach &&
		!(s._journeyActivity == XeenJourneyActivity::Presentation && !action && !_journeyFramePrepared && !_journeyFrameRetry)) return refused;
	if (!journeyCapacity()) return refused;
	try {
		xeenValidateJourneyParty(_party);
		if (s._actors.at(5).lifecycle == XeenActorLifecycle::Present) xeenValidateJourneyMelee(_party);
	} catch (const std::invalid_argument &e) { _journeyRefusal = e.what(); refused.reason = XeenEncounterStop::Domain; return refused; }
	_journeyRefusal.clear();
	const auto boundaryGeneration = _boundary.generation();
	BusyJourney busy(_busy);
	try {
		s._journeyActivity = XeenJourneyActivity::Approach; ++s._journeyGeneration;
		_world._combatCheck = [this, boundaryGeneration] {
			if (_boundary.generation() != boundaryGeneration) throw std::logic_error("stale Journey approach boundary");
			_journeyPreimage->check();
		};
		_world._combatAuthorized = [this, generation = _generation, boundaryGeneration] {
			return !_failure && _generation == generation && _boundary.generation() == boundaryGeneration && _journeyPreimage->ownersAlive();
		};
		_journeyPreimage->adoptJourneyCoordination();
		XeenEncounterResult result;
		{
			XeenRestoreGuard::Providers providers(*_journeyPreimage,_world);
			result = action ? XeenActorApproach::action(_world,_party,_camera,_state,*action,_events) :
				XeenActorApproach::pulse(_world,_party,_camera,_state,_events);
		}
		if (!_journeyPreimage->ownersAlive()) { closeJourney(); return refused; }
		// A replaced boundary belongs to newer work. Leave its coordination alone.
		if (_boundary.generation() != boundaryGeneration) return refused;
		_world._combatCheck = {}; _world._combatAuthorized = {};
		if (result.outcome == XeenEncounterOutcome::Stale) { closeJourney(); return refused; }
		if (_state.phase() == XeenEncounterPhase::SupportStopped) { closeJourney(); return result; }
		if (result.outcome == XeenEncounterOutcome::Refused) {
			s._journeyActivity = _state.pending() ? XeenJourneyActivity::Approach : XeenJourneyActivity::Quiet;
			_journeyPreimage->adoptJourneyCoordination(); _journeyPreimage->check(); return result;
		}
		_result = result; ++_generation; _journeyFramePrepared = false;
		s._journeyActivity = _state.phase() == XeenEncounterPhase::Engaged ? XeenJourneyActivity::Attachment :
			_state.pending() ? XeenJourneyActivity::Approach : XeenJourneyActivity::Presentation;
		retainJourney(); return result;
	} catch (...) {
		if (_boundary.generation() != boundaryGeneration) return refused;
		closeJourney(); throw;
	}
}
XeenEquipmentResult XeenEncounterFlow::journeyEquipment(const Ticket &entry, std::size_t active,
		XeenInventoryCategory category, std::size_t slot, XeenEquipmentOperation operation) {
	if (!journeyMutable() || !_boundary.preparationReady() || !current(entry) || !journeyCapacity()) return {};
	BusyJourney busy(_busy);
	try {
		xeenValidateJourneyParty(_party);
		const auto result = xeenSetEquipment(_party,active,category,slot,operation);
		if (result.status == XeenEquipmentStatus::Success) _world._sessionState._journeyActivity = XeenJourneyActivity::Presentation;
		++_generation; retainJourney(); return result;
	} catch (...) { closeJourney(); throw; }
}
XeenTransferResult XeenEncounterFlow::journeyTransfer(const Ticket &entry, std::size_t from, std::size_t to,
		XeenInventoryCategory category, std::size_t slot) {
	if (!journeyMutable() || !_boundary.preparationReady() || !current(entry) || !journeyCapacity()) return {};
	BusyJourney busy(_busy);
	try {
		xeenValidateJourneyParty(_party);
		const auto result = xeenTransferItem(_party,from,to,category,slot);
		if (result.status == XeenTransferStatus::Success) _world._sessionState._journeyActivity = XeenJourneyActivity::Presentation;
		++_generation; retainJourney(); return result;
	} catch (...) { closeJourney(); throw; }
}
bool XeenEncounterFlow::attachJourney(const Ticket &entry, const std::function<void()> &prepareSprites) {
	if (!_journey || _busy || _combat || !current(entry) || !_boundary.quiet() ||
		_world.sessionState().journeyActivity() != XeenJourneyActivity::Attachment) return false;
	if (!journeyCapacity()) return false;
	const auto boundaryGeneration = _boundary.generation();
	const auto checkBoundary = [this, boundaryGeneration] {
		if (_boundary.generation() != boundaryGeneration) throw std::logic_error("stale Journey attachment boundary");
	};
	BusyJourney busy(_busy);
	try {
		xeenValidateJourneyMelee(_party);
		{
			XeenRestoreGuard::Providers providers(*_journeyPreimage,_world,checkBoundary);
			XeenActorApproach::validateEnvironment(_world,_world.sessionState().actors(),_events);
			checkBoundary();
			_journeyPreimage->check();
			if (prepareSprites) prepareSprites();
			checkBoundary();
			_journeyPreimage->check();
			// Sprite preparation may discard geometry/MOB caches. Rebuild under the
			// same entry authority before combat construction can publish a borrow.
			XeenActorApproach::validateEnvironment(_world,_world.sessionState().actors(),_events);
		}
		checkBoundary();
		_journeyPreimage->check();
		_combat.reset(new XeenCombat(_world,_party,_camera,_boundary,_flags,_state,_journeyStatistics,_events));
		++_generation;
		if (!acceptCombatResult(_combat->beginCombat(_combat->ticket()))) return false;
		observeCombat(); _deadline.reset(); _frame = 0; _appearanceStep = 0;
		std::uint64_t now;
		if (!prepareTime(ticket(),now)) throw std::runtime_error("Journey attachment scheduling failed");
		_lastTime = now; _cosmeticDeadline = now + 100; scheduleCombat(now);
		return true;
	} catch (...) {
		// As in approach, obsolete work cannot fail or release a newer boundary.
		if (_boundary.generation() != boundaryGeneration) return false;
		closeJourney(); throw;
	}
}
bool XeenEncounterFlow::retireJourney(const Ticket &entry) {
	if (!_journey || _busy || !_combat || !current(entry) || !entry.combat) return false;
	if (!journeyCapacity()) return false;
	BusyJourney busy(_busy);
	try {
		// Allocate the returned preimage before consuming End. Only runtime fields change below.
		auto prepared = std::make_shared<XeenRestoreGuard>(_world,_party,_camera,_flags);
		_combat->retireJourney(*entry.combat,_state);
		_retiredCombatResult = _combat->result(); _combat.reset(); ++_generation;
		_deadline.reset(); _frame = 0; _appearanceStep = 0;
		_scheduleAfterFrame = _appearanceAfterFrame = false;
		prepared->adoptJourneyCoordination(); prepared->adoptJourneyBorrowRelease(); _journeyPreimage.swap(prepared);
		return true;
	} catch (const std::invalid_argument &) { return false; }
}
}
