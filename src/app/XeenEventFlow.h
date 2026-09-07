#ifndef MMODERN_APP_XEEN_EVENT_FLOW_H
#define MMODERN_APP_XEEN_EVENT_FLOW_H

#include "app/XeenNavigationFlow.h"
#include "games/xeen/XeenEventPresenter.h"
#include "games/xeen/XeenWorld.h"

namespace mmodern {

// Presentation/composition boundary shared by Application and integration tests.
// All gameplay state remains owned by the caller's existing session graph.
// The caller's mutable party must outlive this flow and pending presentations.
class XeenEventFlow {
public:
	using Compose = std::function<IndexedFrame()>;
	XeenEventFlow(XeenWorld &world, XeenEventSystem &events,
		XeenPartyState &party, XeenCamera &camera, XeenGameFlags &flags,
		const XeenFontFormat &font, Compose compose);
	IndexedFrame initial();
	IndexedFrame handle(const PlayerAction &action);
	IndexedFrame refresh(bool reconstruct = false);
	IndexedFrame acceptManual(XeenManualEventResult result);
	IndexedFrame acceptAutomatic(XeenAutomaticEventResult result);
	const IndexedFrame &frame() const { return _frame; }
	bool blocksGameplay() const { return _pending.has_value(); }
	std::function<void(const XeenManualEventResult &)> reportManual;
	std::function<void(const XeenAutomaticEventResult &)> reportAutomatic;
	std::function<void(const std::string &)> reportText;
	std::function<void(XeenMovementResult)> reportMovement;
private:
	template<class Result> IndexedFrame drive(Result result, bool automatic);
	struct Pending { XeenEventExecutionState state; bool automatic; };
	XeenWorld &_world;
	XeenEventSystem &_events;
	XeenPartyState &_party;
	XeenCamera &_camera;
	XeenGameFlags &_flags;
	XeenNavigationFlow _navigation;
	XeenEventPresenter _presenter;
	Compose _compose;
	std::optional<Pending> _pending;
	IndexedFrame _frame;
	XeenCamera _renderedCamera;
	std::size_t _disabledObjects = 0;
};
}
#endif
