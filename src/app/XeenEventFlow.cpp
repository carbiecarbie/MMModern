#include "app/XeenEventFlow.h"
#include <type_traits>
#include <utility>

namespace mmodern {
namespace {
bool sameCamera(const XeenCamera &a, const XeenCamera &b) {
	return a.mapId == b.mapId && a.x == b.x && a.y == b.y && a.direction == b.direction;
}
}

XeenEventFlow::XeenEventFlow(XeenWorld &world, XeenEventSystem &events,
		XeenPartyState &party, XeenCamera &camera, XeenGameFlags &flags,
		const XeenFontFormat &font, Compose compose, XeenEventPresenter::NpcDraw npcDraw,
		XeenEventPresenter::Clock clock, XeenEventPresenter::RandomFrame randomFrame) :
	_world(world), _events(events), _party(party), _camera(camera), _flags(flags),
	_navigation(events), _presenter(font, std::move(npcDraw), std::move(clock), std::move(randomFrame)),
	_compose(std::move(compose)) {
	refresh(true);
}

IndexedFrame XeenEventFlow::refresh(bool reconstruct) {
	const bool cameraChanged = !sameCamera(_camera, _renderedCamera);
	// M15's disabled set only grows during a session. Its size is an exact,
	// constant-time change detector for the only supported visual mutation.
	const auto count = _world.sessionState().disabledObjectCount();
	if (reconstruct || !_frame.isValid() || cameraChanged || count != _disabledObjects) {
		// Always use the committed camera, never logicalAddress/workingCamera.
		const auto base = _compose();
		if (cameraChanged && !_pending) _presenter.clear();
		try { _frame = _presenter.rebase(base); }
		catch (const std::exception &e) {
			if (!pendingNpc()) throw;
			presentationFailed(e);
			_frame = _presenter.rebase(base);
		}
		_renderedCamera = _camera;
		_disabledObjects = count;
	}
	return _frame;
}

template<class Result> IndexedFrame XeenEventFlow::drive(Result result, bool automatic) {
	for (;;) {
		// Includes mutation preceding suspension/error, and immediate continuations.
		refresh();
		const auto *suspended = std::get_if<XeenEventExecutionSuspended>(&result);
		if (!suspended) _pending.reset();
		if constexpr (std::is_same_v<Result, XeenManualEventResult>) {
			if (reportManual) reportManual(result);
		} else {
			if (reportAutomatic) reportAutomatic(result);
		}
		if (!suspended) return _frame;
		_frame = _presenter.dismissSelection();
		_pending = Pending{suspended->state, automatic, ++_generation};
		XeenPresentationUpdate update;
		try { update = _presenter.present(_frame, suspended->request); }
		catch (const std::exception &e) {
			if (!pendingNpc()) throw;
			return presentationFailed(e);
		}
		_frame = std::move(update.frame);
		if (reportText) for (const auto &message : _presenter.diagnostics()) reportText(message);
		if (!update.response) return _frame;
		auto pending = std::move(*_pending);
		_pending.reset(); // Consume before calling into the execution system.
		if constexpr (std::is_same_v<Result, XeenManualEventResult>)
			result = _events.resumeManualEvent(std::move(pending.state), *update.response,
				_world, _party, _camera, _flags);
		else
			result = _events.resumeAutomaticEvent(std::move(pending.state), *update.response,
				_world, _party, _camera, _flags);
	}
}

IndexedFrame XeenEventFlow::acceptManual(XeenManualEventResult result) {
	_pending.reset();
	_frame = _presenter.dismissSelection();
	return drive(std::move(result), false);
}
IndexedFrame XeenEventFlow::acceptAutomatic(XeenAutomaticEventResult result) {
	_pending.reset();
	_frame = _presenter.dismissSelection();
	return drive(std::move(result), true);
}
IndexedFrame XeenEventFlow::initial() {
	return acceptAutomatic(_navigation.processInitialEvent(_world, _party, _camera, _flags));
}
bool XeenEventFlow::canCancelInteraction() const {
	return _pending && _pending->state.pendingPresentation &&
		_pending->state.pendingPresentation->request.response ==
			XeenPresentationResponseRequirement::CharacterSelection;
}
bool XeenEventFlow::pendingNpc() const {
	return _pending && _pending->state.pendingPresentation &&
		_pending->state.pendingPresentation->request.kind == XeenPresentationKind::NpcAcknowledgment;
}
bool XeenEventFlow::handlesEscape() const { return canCancelInteraction() || pendingNpc(); }
void XeenEventFlow::abandonPresentation() {
	_pending.reset();
	_frame = _presenter.dismissSelection();
}
IndexedFrame XeenEventFlow::presentationFailed(const std::exception &exception) {
	const auto pending = std::move(*_pending);
	_pending.reset();
	_frame = _presenter.discardNpc();
	const auto &request = pending.state.pendingPresentation->request;
	XeenEventExecutionError error{XeenEventExecutionErrorKind::PresentationFailed,
		std::string("NPC presentation: ") + exception.what(), pending.state.instructionCount,
		pending.state.logicalAddress, request.source, std::nullopt};
	if (pending.automatic) { if (reportAutomatic) reportAutomatic(error); }
	else if (reportManual) reportManual(error);
	return _frame;
}
std::optional<IndexedFrame> XeenEventFlow::updatePresentation() {
	if (!pendingNpc()) return std::nullopt;
	try {
		auto changed = _presenter.updateNpc();
		if (changed) _frame = *changed;
		return changed;
	} catch (const std::exception &e) { return presentationFailed(e); }
}
std::optional<std::uint64_t> XeenEventFlow::presentationGeneration() const {
	return _pending ? std::optional<std::uint64_t>{_pending->generation} : std::nullopt;
}
bool XeenEventFlow::respond(std::uint64_t generation, XeenPresentationResponse response) {
	if (!_pending || _pending->generation != generation) return false;
	auto pending = std::move(*_pending);
	_pending.reset();
	_frame = _presenter.finishPresentation();
	if (pending.automatic)
		acceptAutomatic(_events.resumeAutomaticEvent(std::move(pending.state), response,
			_world, _party, _camera, _flags));
	else
		acceptManual(_events.resumeManualEvent(std::move(pending.state), response,
			_world, _party, _camera, _flags));
	return true;
}
IndexedFrame XeenEventFlow::handle(const PlayerAction &action) {
	if (std::holds_alternative<SelectMemberAction>(action) && !canCancelInteraction())
		return _frame;
	const bool hadPending = _pending.has_value();
	refresh();
	if (hadPending && !_pending) return _frame; // Rebase failure must not dispatch this input.
	if (_pending) {
		const auto generation = _pending->generation;
		XeenPresentationUpdate update;
		try { update = _presenter.handle(action); }
		catch (const std::exception &e) {
			if (!pendingNpc()) throw;
			return presentationFailed(e);
		}
		_frame = std::move(update.frame);
		if (!update.response) return _frame;
		respond(generation, *update.response);
		return _frame;
	}
	_presenter.clear(); // M14 labels last until the next gameplay action.
	if (const auto *navigation = std::get_if<NavigationAction>(&action)) {
		const auto result = _navigation.processNavigationAction(_world, _party, _camera,
			_flags, *navigation);
		if (reportMovement) reportMovement(result.movementResult);
		refresh(true);
		return acceptAutomatic(result.automaticEvent);
	}
	refresh(true);
	if (std::holds_alternative<InteractionAction>(action))
		return acceptManual(_navigation.processInteraction(_world, _party, _camera, _flags));
	return _frame;
}
}
