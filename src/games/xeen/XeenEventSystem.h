#ifndef MMODERN_GAMES_XEEN_XEEN_EVENT_SYSTEM_H
#define MMODERN_GAMES_XEEN_XEEN_EVENT_SYSTEM_H

#include "games/xeen/XeenEventInterpreter.h"

#include <cstddef>
#include <cstdint>
#include <map>
#include <variant>

namespace mmodern {

class XeenWorld;

struct XeenAutomaticEventNoTrigger {};

struct XeenAutomaticEventCompleted {
	std::size_t instructionCount = 0;
	bool cameraChanged = false;
	bool flagsChanged = false;
};

struct XeenManualEventNoEvent {};

struct XeenManualEventCompleted {
	std::size_t instructionCount = 0;
	bool cameraChanged = false;
	bool flagsChanged = false;
};

struct XeenManualSpecialInteractionUnsupported {
	std::uint8_t wallValue = 0;
};

using XeenAutomaticEventResult = std::variant<
	XeenAutomaticEventNoTrigger,
	XeenAutomaticEventCompleted,
	XeenEventExecutionSuspended,
	XeenEventExecutionError>;

using XeenManualEventResult = std::variant<
	XeenManualEventNoEvent,
	XeenManualEventCompleted,
	XeenManualSpecialInteractionUnsupported,
	XeenEventExecutionSuspended,
	XeenEventExecutionError>;

class XeenEventSystem {
public:
	using ScriptProvider = XeenEventInterpreter::ScriptProvider;
	using TextProvider = XeenEventInterpreter::TextProvider;

	// References captured by the provider must outlive this system.
	explicit XeenEventSystem(ScriptProvider scriptProvider,
		TextProvider textProvider = {});

	// The supplied state is already current. A failure rolls back only changes
	// attempted by this event; it does not undo earlier movement or rotation.
	XeenAutomaticEventResult runAutomaticEvent(XeenWorld &world,
		const XeenPartyState &partyState, XeenCamera &camera,
		XeenGameFlags &gameFlags);

	XeenManualEventResult runManualEvent(XeenWorld &world,
		const XeenPartyState &partyState, XeenCamera &camera,
		XeenGameFlags &gameFlags);

	XeenAutomaticEventResult resumeAutomaticEvent(XeenEventExecutionState state,
		XeenPresentationResponse response, XeenWorld &world,
		const XeenPartyState &partyState, XeenCamera &camera,
		XeenGameFlags &gameFlags);

	XeenManualEventResult resumeManualEvent(XeenEventExecutionState state,
		XeenPresentationResponse response, XeenWorld &world,
		const XeenPartyState &partyState, XeenCamera &camera,
		XeenGameFlags &gameFlags);

	std::size_t cachedScriptCount() const { return _scripts.size(); }
	std::size_t cachedTextCount() const { return _texts.size(); }
	// Suspended execution owns values, not pointers into these caches.
	void discardScriptCache() { _scripts.clear(); }
	void discardTextCache() { _texts.clear(); }

private:
	XeenEventScript scriptForMap(XeenMapIdentity mapId);
	XeenEventTextFile textForMap(XeenMapIdentity mapId);

	ScriptProvider _scriptProvider;
	TextProvider _textProvider;
	std::map<XeenMapIdentity, XeenEventScript> _scripts;
	std::map<XeenMapIdentity, XeenEventTextFile> _texts;
	XeenEventInterpreter _interpreter;
};

} // namespace mmodern

#endif
