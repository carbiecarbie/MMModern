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
		const XeenPartyState &party, XeenCamera &camera, XeenGameFlags &flags,
		const XeenFontFormat &font, Compose compose) :
	_world(world), _events(events), _party(party), _camera(camera), _flags(flags),
	_navigation(events), _presenter(font), _compose(std::move(compose)) {
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
		if (cameraChanged) _presenter.clear();
		_frame = _presenter.rebase(base);
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
		_pending = Pending{suspended->state, automatic};
		auto update = _presenter.present(_frame, suspended->request);
		_frame = std::move(update.frame);
		if (reportText) for (const auto &message : _presenter.diagnostics()) reportText(message);
		if (!update.response) return _frame;
		if constexpr (std::is_same_v<Result, XeenManualEventResult>)
			result = _events.resumeManualEvent(_pending->state, *update.response,
				_world, _party, _camera, _flags);
		else
			result = _events.resumeAutomaticEvent(_pending->state, *update.response,
				_world, _party, _camera, _flags);
	}
}

IndexedFrame XeenEventFlow::acceptManual(XeenManualEventResult result) {
	return drive(std::move(result), false);
}
IndexedFrame XeenEventFlow::acceptAutomatic(XeenAutomaticEventResult result) {
	return drive(std::move(result), true);
}
IndexedFrame XeenEventFlow::initial() {
	return acceptAutomatic(_navigation.processInitialEvent(_world, _party, _camera, _flags));
}
IndexedFrame XeenEventFlow::handle(const PlayerAction &action) {
	refresh();
	if (_pending) {
		auto update = _presenter.handle(action);
		_frame = std::move(update.frame);
		if (!update.response) return _frame;
		if (_pending->automatic)
			return acceptAutomatic(_events.resumeAutomaticEvent(_pending->state,
				*update.response, _world, _party, _camera, _flags));
		return acceptManual(_events.resumeManualEvent(_pending->state,
			*update.response, _world, _party, _camera, _flags));
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
