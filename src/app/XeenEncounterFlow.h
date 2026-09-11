#ifndef MMODERN_APP_XEEN_ENCOUNTER_FLOW_H
#define MMODERN_APP_XEEN_ENCOUNTER_FLOW_H

#include "core/PlayerAction.h"
#include "formats/xeen/XeenMonsterAppearance.h"
#include "games/xeen/XeenActorApproach.h"
#include "games/xeen/XeenCombat.h"
#include "games/xeen/XeenEventPresenter.h"

namespace mmodern {

// Borrowed providers used once, before EventFlow's first refresh.
struct XeenEncounterSetup {
	const XeenEventFile &events;
	std::function<XeenEncounterResult(XeenWorld &, XeenPartyState &, XeenCamera &, XeenEncounterState &)> initialize;
	std::function<void(std::uint8_t image)> validateNormalSprite;
	std::function<std::unique_ptr<XeenCombat>(XeenWorld &, XeenPartyState &, XeenCamera &, XeenCombatBoundary &)> prepareCombat;
	std::function<void(std::uint8_t image)> validateAttackSprite;
};

// Bounded coordinator. World/party/camera and the normalized clock remain borrowed.
class XeenEncounterFlow {
public:
	struct Ticket { XeenEncounterState state; std::uint64_t generation; std::optional<XeenCombat::Ticket> combat; };
	XeenEncounterFlow(XeenWorld &, XeenPartyState &, XeenCamera &,
		const XeenEventPresenter::Clock &, const XeenEncounterSetup &);
	XeenEncounterFlow(const XeenEncounterFlow &) = delete;
	XeenEncounterFlow &operator=(const XeenEncounterFlow &) = delete;
	Ticket ticket() const noexcept { return {state(), _generation, _combat ? std::optional<XeenCombat::Ticket>{_combat->ticket()} : std::nullopt}; }
	XeenCombat *combat() noexcept { return _combat.get(); }
	const XeenCombat *combat() const noexcept { return _combat.get(); }
	// Fixed observations for downstream presentation, never continuation authority.
	const XeenCombatResult &combatObservation() const noexcept { return _combatObservation; }
	const XeenCombatResult &combatAward() const noexcept { return _combatAward; }
	XeenCombatBoundary &boundary() noexcept { return _boundary; }
	bool preparation() const noexcept { return _combat && _combat->phase() == XeenCombatPhase::Preparation; }
	bool terminal() const noexcept;
	bool combatOperationStale() const noexcept { return _combatOperationStale; }
	void presented(const Ticket &);
	bool current(const Ticket &) const noexcept;
	bool handle(const PlayerAction &, std::optional<std::uint64_t> cycle = {}, std::optional<XeenCombat::Ticket> displayed = {});
	bool idle(std::optional<std::uint64_t> cycle = {});
	bool fail(const Ticket &, XeenEncounterStop = XeenEncounterStop::Reporting) noexcept;
	const XeenEncounterState &state() const noexcept { return _combat ? _combat->approachState() : _state; }
	const XeenEncounterResult &result() const noexcept { return _result; }
	const XeenEncounterResult &actionResult() const noexcept { return _actionResult; }
	unsigned actionPending() const noexcept { return _actionPending; }
	std::uint8_t frame() const noexcept { return _frame; }
	XeenMonsterAppearance appearance() const noexcept {
		return _frame < 8 ? XeenMonsterAppearance{_frame} :
			XeenMonsterAppearance{XeenMonsterSpriteKind::Attack, static_cast<std::uint8_t>(_frame - 8)};
	}
	std::optional<std::uint64_t> deadline() const noexcept { return _deadline; }
	std::uint64_t cosmeticDeadline() const noexcept { return _cosmeticDeadline; }
	std::string notice() const;
private:
	bool handleCombat(const PlayerAction &, std::optional<std::uint64_t>);
	bool idleCombat(std::optional<std::uint64_t>);
	bool acceptCombatResult(const XeenCombatResult &);
	void scheduleCombat(std::uint64_t);
	bool handoffCombat();
	std::string combatNotice() const;
	bool observeCombat() noexcept;
	void advanceAppearance() noexcept;
	XeenCombatResult _combatObservation, _combatAward;
	bool _scheduleAfterFrame = false, _combatOperationStale = false;
	bool adopt(const XeenEncounterResult &, std::uint64_t generation) noexcept;
	bool prepareTime(const Ticket &, std::uint64_t &now);
	void schedule(std::uint64_t now) noexcept;
	XeenWorld &_world;
	XeenPartyState &_party;
	XeenCamera &_camera;
	const XeenEventPresenter::Clock &_clock;
	const XeenEventFile &_events;
	XeenCombatBoundary _boundary;
	std::unique_ptr<XeenCombat> _combat;
	XeenEncounterState _state;
	XeenEncounterResult _result, _actionResult;
	unsigned _actionPending = 0;
	std::uint64_t _generation = 0, _lastTime = 0, _cosmeticDeadline = 0;
	std::optional<std::uint64_t> _deadline, _inputCycle;
	std::uint8_t _frame = 0;
	std::uint8_t _appearanceStep = 0;
	bool _appearanceAfterFrame = false;
	bool _busy = false, _failure = false;
	std::optional<std::pair<int, int>> _attempted;
};
}
#endif
