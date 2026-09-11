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
	_world(w), _party(p), _camera(c), _clock(clock), _events(setup.events) {
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

bool XeenEncounterFlow::handle(const PlayerAction &input, std::optional<std::uint64_t> cycle) {
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
}
