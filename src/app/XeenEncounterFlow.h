#ifndef MMODERN_APP_XEEN_ENCOUNTER_FLOW_H
#define MMODERN_APP_XEEN_ENCOUNTER_FLOW_H

#include "core/PlayerAction.h"
#include "games/xeen/XeenActorApproach.h"
#include "games/xeen/XeenEventPresenter.h"

namespace mmodern {
enum class XeenEncounterEntry { Ordinary, Diagnostic26 };

// Borrowed providers used once, before EventFlow's first refresh.
struct XeenEncounterSetup {
	const XeenEventFile &events;
	std::function<XeenEncounterResult(XeenWorld &, XeenPartyState &, XeenCamera &, XeenEncounterState &)> initialize;
	std::function<void(std::uint8_t image)> validateNormalSprite;
};

// Bounded coordinator. World/party/camera and the normalized clock remain borrowed.
class XeenEncounterFlow {
public:
	struct Ticket { XeenEncounterState state; std::uint64_t generation; };
	XeenEncounterFlow(XeenWorld &, XeenPartyState &, XeenCamera &,
		const XeenEventPresenter::Clock &, const XeenEncounterSetup &);
	XeenEncounterFlow(const XeenEncounterFlow &) = delete;
	XeenEncounterFlow &operator=(const XeenEncounterFlow &) = delete;
	Ticket ticket() const noexcept { return {_state, _generation}; }
	bool current(const Ticket &) const noexcept;
	bool handle(const PlayerAction &, std::optional<std::uint64_t> cycle = {});
	bool idle(std::optional<std::uint64_t> cycle = {});
	bool fail(const Ticket &, XeenEncounterStop = XeenEncounterStop::Reporting) noexcept;
	const XeenEncounterState &state() const noexcept { return _state; }
	const XeenEncounterResult &result() const noexcept { return _result; }
	const XeenEncounterResult &actionResult() const noexcept { return _actionResult; }
	unsigned actionPending() const noexcept { return _actionPending; }
	std::uint8_t frame() const noexcept { return _frame; }
	std::optional<std::uint64_t> deadline() const noexcept { return _deadline; }
	std::uint64_t cosmeticDeadline() const noexcept { return _cosmeticDeadline; }
	std::string notice() const;
private:
	bool adopt(const XeenEncounterResult &, std::uint64_t generation) noexcept;
	bool prepareTime(const Ticket &, std::uint64_t &now);
	void schedule(std::uint64_t now) noexcept;
	XeenWorld &_world;
	XeenPartyState &_party;
	XeenCamera &_camera;
	const XeenEventPresenter::Clock &_clock;
	const XeenEventFile &_events;
	XeenEncounterState _state;
	XeenEncounterResult _result, _actionResult;
	unsigned _actionPending = 0;
	std::uint64_t _generation = 0, _lastTime = 0, _cosmeticDeadline = 0;
	std::optional<std::uint64_t> _deadline, _inputCycle;
	std::uint8_t _frame = 0;
	bool _busy = false, _failure = false;
	std::optional<std::pair<int, int>> _attempted;
};
}
#endif
