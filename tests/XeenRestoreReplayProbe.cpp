#include "XeenRestoreReplayProbe.h"
#include "games/xeen/XeenCombat.h"
#include "games/xeen/XeenEventInterpreter.h"

namespace replay_test {
using namespace mmodern;
void observe();
unsigned journeyInitializations=0, journeyConstructions=0, actions=0, pulses=0, retirements=0, commands=0, draws=0;
XeenEncounterResult real_journeyInitialize(XeenWorld &w, XeenPartyState &p, XeenCamera &c, XeenEncounterState &s, const std::vector<std::uint8_t> &chr, const XeenGameplayContext &ctx, const std::vector<XeenMonsterRecord> &mon, const XeenEventFile &evt, std::uint32_t seed) asm("__real_" XEEN_REPLAY_JOURNEY_INITIALIZE);
XeenEncounterResult probe_journeyInitialize(XeenWorld &w, XeenPartyState &p, XeenCamera &c, XeenEncounterState &s, const std::vector<std::uint8_t> &chr, const XeenGameplayContext &ctx, const std::vector<XeenMonsterRecord> &mon, const XeenEventFile &evt, std::uint32_t seed) asm("__wrap_" XEEN_REPLAY_JOURNEY_INITIALIZE);
XeenEncounterResult probe_journeyInitialize(XeenWorld &w, XeenPartyState &p, XeenCamera &c, XeenEncounterState &s, const std::vector<std::uint8_t> &chr, const XeenGameplayContext &ctx, const std::vector<XeenMonsterRecord> &mon, const XeenEventFile &evt, std::uint32_t seed) { observe(); ++journeyInitializations; return real_journeyInitialize(w,p,c,s,chr,ctx,mon,evt,seed); }
XeenEncounterResult real_approachAction(XeenWorld &w, XeenPartyState &p, XeenCamera &c, XeenEncounterState &s, XeenEncounterAction a, const XeenEventFile &evt) asm("__real_" XEEN_REPLAY_ACTION);
XeenEncounterResult probe_approachAction(XeenWorld &w, XeenPartyState &p, XeenCamera &c, XeenEncounterState &s, XeenEncounterAction a, const XeenEventFile &evt) asm("__wrap_" XEEN_REPLAY_ACTION);
XeenEncounterResult probe_approachAction(XeenWorld &w, XeenPartyState &p, XeenCamera &c, XeenEncounterState &s, XeenEncounterAction a, const XeenEventFile &evt) { observe(); ++actions; return real_approachAction(w,p,c,s,a,evt); }
XeenEncounterResult real_approachPulse(XeenWorld &w, XeenPartyState &p, XeenCamera &c, XeenEncounterState &s, const XeenEventFile &evt) asm("__real_" XEEN_REPLAY_PULSE);
XeenEncounterResult probe_approachPulse(XeenWorld &w, XeenPartyState &p, XeenCamera &c, XeenEncounterState &s, const XeenEventFile &evt) asm("__wrap_" XEEN_REPLAY_PULSE);
XeenEncounterResult probe_approachPulse(XeenWorld &w, XeenPartyState &p, XeenCamera &c, XeenEncounterState &s, const XeenEventFile &evt) { observe(); ++pulses; return real_approachPulse(w,p,c,s,evt); }
struct JourneyReal {
	void construct(XeenWorld &w, XeenPartyState &p, XeenCamera &c, XeenCombatBoundary &b, const XeenGameFlags &f, const XeenEncounterState &s, const std::vector<XeenMonsterRecord> &mon, const XeenEventFile &evt) asm("__real_" XEEN_REPLAY_JOURNEY_CONSTRUCT);
	void retire(const XeenCombat::Ticket &t, XeenEncounterState &s) asm("__real_" XEEN_REPLAY_RETIRE);
	XeenCombatResult command(const XeenCombat::Ticket &t, XeenCombatCommand c) asm("__real_" XEEN_REPLAY_COMMAND);
	std::optional<std::uint32_t> draw(std::uint32_t lo, std::uint32_t hi) asm("__real_" XEEN_REPLAY_DRAW);
};
struct JourneyProbe {
	void construct(XeenWorld &w, XeenPartyState &p, XeenCamera &c, XeenCombatBoundary &b, const XeenGameFlags &f, const XeenEncounterState &s, const std::vector<XeenMonsterRecord> &mon, const XeenEventFile &evt) asm("__wrap_" XEEN_REPLAY_JOURNEY_CONSTRUCT);
	void retire(const XeenCombat::Ticket &t, XeenEncounterState &s) asm("__wrap_" XEEN_REPLAY_RETIRE);
	XeenCombatResult command(const XeenCombat::Ticket &t, XeenCombatCommand c) asm("__wrap_" XEEN_REPLAY_COMMAND);
	std::optional<std::uint32_t> draw(std::uint32_t lo, std::uint32_t hi) asm("__wrap_" XEEN_REPLAY_DRAW);
};
void JourneyProbe::construct(XeenWorld &w, XeenPartyState &p, XeenCamera &c, XeenCombatBoundary &b, const XeenGameFlags &f, const XeenEncounterState &s, const std::vector<XeenMonsterRecord> &mon, const XeenEventFile &evt) { observe(); ++journeyConstructions; return reinterpret_cast<JourneyReal *>(this)->construct(w,p,c,b,f,s,mon,evt); }
void JourneyProbe::retire(const XeenCombat::Ticket &t, XeenEncounterState &s) { observe(); ++retirements; return reinterpret_cast<JourneyReal *>(this)->retire(t,s); }
XeenCombatResult JourneyProbe::command(const XeenCombat::Ticket &t, XeenCombatCommand c) { observe(); ++commands; return reinterpret_cast<JourneyReal *>(this)->command(t,c); }
std::optional<std::uint32_t> JourneyProbe::draw(std::uint32_t lo, std::uint32_t hi) { observe(); ++draws; return reinterpret_cast<JourneyReal *>(this)->draw(lo,hi); }
}

using namespace mmodern;
namespace replay_test {
unsigned depth = 0, unexpected = 0, constructions = 0, services = 0, preparations = 0;
void observe() { if (depth) ++unexpected; }
// Member thunks preserve the target ABI's hidden result/this argument ordering.
struct RealCombat {
	void construct(XeenWorld &, XeenPartyState &, XeenCamera &, XeenCombatBoundary &,
		const std::vector<std::uint8_t> &, const XeenGameplayContext &, const std::vector<XeenMonsterRecord> &,
		const XeenEventFile &, XeenCombatRandom) asm("__real_" XEEN_REPLAY_CONSTRUCT);
	XeenCombatResult service(const XeenCombat::Ticket &) asm("__real_" XEEN_REPLAY_SERVICE);
};
struct ProbeCombat {
	void construct(XeenWorld &, XeenPartyState &, XeenCamera &, XeenCombatBoundary &,
		const std::vector<std::uint8_t> &, const XeenGameplayContext &, const std::vector<XeenMonsterRecord> &,
		const XeenEventFile &, XeenCombatRandom) asm("__wrap_" XEEN_REPLAY_CONSTRUCT);
	XeenCombatResult service(const XeenCombat::Ticket &) asm("__wrap_" XEEN_REPLAY_SERVICE);
};
void ProbeCombat::construct(XeenWorld &w, XeenPartyState &p, XeenCamera &c, XeenCombatBoundary &b,
		const std::vector<std::uint8_t> &chr, const XeenGameplayContext &ctx, const std::vector<XeenMonsterRecord> &mon,
		const XeenEventFile &evt, XeenCombatRandom random) {
	observe(); ++constructions;
	reinterpret_cast<RealCombat *>(this)->construct(w, p, c, b, chr, ctx, mon, evt, std::move(random));
}
XeenCombatResult ProbeCombat::service(const XeenCombat::Ticket &ticket) {
	observe(); ++services; return reinterpret_cast<RealCombat *>(this)->service(ticket);
}
XeenEncounterResult realInitialize(XeenWorld &, XeenPartyState &, XeenCamera &, XeenEncounterState &,
	const std::vector<XeenMonsterRecord> &, const XeenGameplayContext &, const XeenEventFile &) asm("__real_" XEEN_REPLAY_INITIALIZE);
XeenEncounterResult probeInitialize(XeenWorld &, XeenPartyState &, XeenCamera &, XeenEncounterState &,
	const std::vector<XeenMonsterRecord> &, const XeenGameplayContext &, const XeenEventFile &) asm("__wrap_" XEEN_REPLAY_INITIALIZE);
XeenEncounterResult probeInitialize(XeenWorld &w, XeenPartyState &p, XeenCamera &c, XeenEncounterState &s,
		const std::vector<XeenMonsterRecord> &m, const XeenGameplayContext &ctx, const XeenEventFile &e) {
	observe(); ++preparations; return realInitialize(w, p, c, s, m, ctx, e);
}
std::vector<XeenActor> realMove(const std::vector<XeenActor> &, const XeenCamera &, const XeenActorApproach::Terrain &, bool) asm("__real_" XEEN_REPLAY_MOVE);
std::vector<XeenActor> probeMove(const std::vector<XeenActor> &, const XeenCamera &, const XeenActorApproach::Terrain &, bool) asm("__wrap_" XEEN_REPLAY_MOVE);
std::vector<XeenActor> probeMove(const std::vector<XeenActor> &a, const XeenCamera &c, const XeenActorApproach::Terrain &t, bool enabled) {
	observe(); return realMove(a, c, t, enabled);
}
XeenTransferResult realTransfer(XeenPartyState &, std::size_t, std::size_t, XeenInventoryCategory, std::size_t) asm("__real_" XEEN_REPLAY_TRANSFER);
XeenTransferResult probeTransfer(XeenPartyState &, std::size_t, std::size_t, XeenInventoryCategory, std::size_t) asm("__wrap_" XEEN_REPLAY_TRANSFER);
XeenTransferResult probeTransfer(XeenPartyState &p, std::size_t from, std::size_t to, XeenInventoryCategory c, std::size_t slot) {
	observe(); return realTransfer(p, from, to, c, slot);
}
XeenEquipmentResult realEquipment(XeenPartyState &, std::size_t, XeenInventoryCategory, std::size_t, XeenEquipmentOperation) asm("__real_" XEEN_REPLAY_EQUIPMENT);
XeenEquipmentResult probeEquipment(XeenPartyState &, std::size_t, XeenInventoryCategory, std::size_t, XeenEquipmentOperation) asm("__wrap_" XEEN_REPLAY_EQUIPMENT);
XeenEquipmentResult probeEquipment(XeenPartyState &p, std::size_t who, XeenInventoryCategory c, std::size_t slot, XeenEquipmentOperation op) {
	observe(); return realEquipment(p, who, c, slot, op);
}
struct RealEvent {
	XeenEventExecutionResult execute(const XeenCamera &, XeenPartyState &, const XeenGameFlags &, XeenWorld &,
		const XeenEventInterpreter::ScriptProvider &) const asm("__real_" XEEN_REPLAY_EVENT);
	XeenEventExecutionStepResult begin(const XeenCamera &, XeenPartyState &, const XeenGameFlags &, XeenWorld &,
		const XeenEventInterpreter::ScriptProvider &, const XeenEventInterpreter::TextProvider &, std::uint8_t) const asm("__real_" XEEN_REPLAY_EVENT_BEGIN);
};
struct ProbeEvent {
	XeenEventExecutionResult execute(const XeenCamera &, XeenPartyState &, const XeenGameFlags &, XeenWorld &,
		const XeenEventInterpreter::ScriptProvider &) const asm("__wrap_" XEEN_REPLAY_EVENT);
	XeenEventExecutionStepResult begin(const XeenCamera &, XeenPartyState &, const XeenGameFlags &, XeenWorld &,
		const XeenEventInterpreter::ScriptProvider &, const XeenEventInterpreter::TextProvider &, std::uint8_t) const asm("__wrap_" XEEN_REPLAY_EVENT_BEGIN);
};
XeenEventExecutionResult ProbeEvent::execute(const XeenCamera &c, XeenPartyState &p, const XeenGameFlags &f,
		XeenWorld &w, const XeenEventInterpreter::ScriptProvider &scripts) const {
	observe(); return reinterpret_cast<const RealEvent *>(this)->execute(c, p, f, w, scripts);
}
XeenEventExecutionStepResult ProbeEvent::begin(const XeenCamera &c, XeenPartyState &p, const XeenGameFlags &f,
		XeenWorld &w, const XeenEventInterpreter::ScriptProvider &scripts, const XeenEventInterpreter::TextProvider &text, std::uint8_t line) const {
	observe(); return reinterpret_cast<const RealEvent *>(this)->begin(c, p, f, w, scripts, text, line);
}
}
