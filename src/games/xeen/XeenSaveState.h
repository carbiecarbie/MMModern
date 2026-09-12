#ifndef MMODERN_GAMES_XEEN_SAVE_STATE_H
#define MMODERN_GAMES_XEEN_SAVE_STATE_H

#include "games/xeen/XeenSaveSnapshot.h"
#include "games/xeen/XeenWorld.h"

namespace mmodern {

class XeenSaveState {
public:
	struct Resources {
		// From the currently installed archives, not the decoded save header.
		XeenSaveResourceSignature signature;
		std::function<XeenPartyState()> loadInitialParty;
		XeenWorld::EventLoader loadEvents;
		std::function<std::vector<std::uint8_t>()> loadInitialCharacters = {};
		std::function<XeenGameplayContext()> loadInitialContext = {};
		std::function<std::vector<XeenMonsterRecord>()> loadMonsterStatistics = {};
	};
	// Must check needed disposable presentation resources on the candidate.
	// It must not mutate gameplay or retain candidate references after returning.
	using Preflight = std::function<void(XeenWorld &, const XeenPartyState &,
		const XeenCamera &, const XeenGameFlags &)>;

	static bool canCapture(const XeenPartyState &, const XeenCamera &, const XeenWorld &) noexcept;

	// Caller enforces the idle boundary. This layer never acknowledges or cancels
	// an interaction, and deliberately has no EventFlow/Application dependency.
	static XeenSaveSnapshot capture(const XeenSaveResourceSignature &resources,
		const XeenPartyState &party, const XeenCamera &camera,
		const XeenGameFlags &flags, const XeenWorld &world);

	// Startup-only: no flow, suspended execution or callbacks may refer to these
	// destination owners yet. All throwing preparation/preflight happens on local
	// candidates; success publishes through nonthrowing swaps/assignments only.
	// On failure even the destination's existing caches remain untouched.
	static void restoreBeforeGameplay(const XeenSaveSnapshot &snapshot,
		const Resources &resources, XeenPartyState &party, XeenCamera &camera,
		XeenGameFlags &flags, XeenWorld &world, const Preflight &preflight);
private:
	static void validateJourneyValues(const XeenSaveSnapshot &);
	static void restoreJourney(const XeenSaveSnapshot &, const Resources &,
		XeenPartyState &, XeenCamera &, XeenGameFlags &, XeenWorld &, const Preflight &);
	static void restoreCompleted(const XeenSaveSnapshot &, const Resources &,
		XeenPartyState &, XeenCamera &, XeenGameFlags &, XeenWorld &, const Preflight &);
};

} // namespace mmodern
#endif
