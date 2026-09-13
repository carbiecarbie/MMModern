#include "app/Application.h"
#include "app/XeenGameplayServices.h"
#include "platform/XeenSaveFile.h"
#include <iostream>
#include <stdexcept>
#include <exception>
#include <random>
namespace mmodern {
namespace {
struct GameplayScope {
 bool &busy;
 explicit GameplayScope(bool &value) : busy(value) { busy = true; }
 ~GameplayScope() { busy = false; }
};
// Retain the authorization of the frame handed to the window. A later failed
// callback cannot recapture newer domain authority to legitimize its failure.
struct EncounterHandoff {
 XeenEventFlow &flow;
 std::optional<XeenEncounterFlow::Ticket> ticket;
 int exceptions = std::uncaught_exceptions();
 explicit EncounterHandoff(XeenEventFlow &f) : flow(f) { retain(); }
 void retain() {
  if (!flow.encounterFrameCurrent()) throw std::runtime_error("Stale encounter handoff");
  if (flow.encounter()) ticket = flow.encounter()->ticket();
 }
 void verify() const {
  if (ticket && (!flow.encounter()->current(*ticket) || !flow.encounterFrameCurrent()))
   throw std::runtime_error("Stale encounter startup callback");
 }
 void fail() noexcept { if (ticket) flow.failEncounterHandoff(*ticket); }
 ~EncounterHandoff() { if (std::uncaught_exceptions() > exceptions) fail(); }
};
}
void xeenSaveGameplay(const XeenGameplayServices &services, XeenWorld &world, XeenPartyState &party,
	XeenCamera &camera, XeenGameFlags &flags, XeenEventFlow &flow, const std::filesystem::path &target,
	const XeenSaveState::Preflight &preflight, std::function<void()> *nestedSourceCheck) {
	if (!flow.canSave() || !XeenSaveState::canCapture(party,camera,world))
		throw std::logic_error("Save boundary is unavailable");
	XeenRestoreGuard before(world,party,camera,flags);
	const auto snapshot = XeenSaveState::capture(services.resources.signature,party,camera,flags,world);
	before.check();
	const auto boundary = flow.beginSave();
	struct Lease {
		XeenEventFlow &flow; XeenEventFlow::SaveBoundary boundary;
		~Lease() { try { flow.endSave(boundary); } catch (...) { flow.closeGameplay(); } }
	} lease{flow,boundary};
	// Journey and completed Flow retained this preimage before capture and admit
	// only their own explicit lease transition. Ordinary beginSave changes no owner.
	const auto heldPreimage = flow.encounter() ? flow.encounter()->retainSavePreimage() : nullptr;
	auto &retained = heldPreimage ? *heldPreimage : before;
	const auto check = [&] { retained.check(); if (!flow.saveCurrent(boundary)) throw std::logic_error("Save UI authorization changed"); };
	XeenRestoreGuard::Providers providers(retained,world,check);
	struct NestedSource {
		std::function<void()> *slot;
		std::function<void()> previous;
		~NestedSource() { if (slot) slot->swap(previous); }
	} nested{nestedSourceCheck,check};
	if (nested.slot) nested.slot->swap(nested.previous);
	const auto callback = [&](auto &&provider) {
		check();
		try { auto value = provider(); check(); const auto detached = value; return detached; }
		catch (...) { check(); throw; }
	};
	auto resources = services.resources;
	if (resources.loadInitialParty) resources.loadInitialParty = [&] { return callback(services.resources.loadInitialParty); };
	if (resources.loadInitialCharacters) resources.loadInitialCharacters = [&] { return callback(services.resources.loadInitialCharacters); };
	if (resources.loadInitialContext) resources.loadInitialContext = [&] { return callback(services.resources.loadInitialContext); };
	if (resources.loadMonsterStatistics) resources.loadMonsterStatistics = [&] { return callback(services.resources.loadMonsterStatistics); };
	if (resources.loadEvents) resources.loadEvents = [&](XeenMapIdentity id) { return callback([&] { return services.resources.loadEvents(id); }); };
	const auto stage = [&](XeenGameplayServices::SaveStage stage) {
		check(); try { if (services.observeSaveStage) services.observeSaveStage(stage); }
		catch (...) { check(); throw; } check();
	};
	stage(XeenGameplayServices::SaveStage::Capture);
	XeenWorld candidate([&](XeenMapIdentity id) { return callback([&] { return services.maps(id); }); },
		[&](XeenMapIdentity id) { return callback([&] { return services.objects(id); }); });
	XeenPartyState p; XeenCamera c; XeenGameFlags f;
	stage(XeenGameplayServices::SaveStage::Preflight);
	XeenSaveState::restoreBeforeGameplay(snapshot,resources,p,c,f,candidate,[&](auto &w,const auto &p,const auto &c,const auto &f) {
		check(); try { preflight(w,p,c,f); } catch (...) { check(); throw; } check();
	});
	stage(XeenGameplayServices::SaveStage::Write);
	check(); XeenSaveFile::write(target,snapshot);
}
int Application::journeySkeleton(const std::filesystem::path &directory, std::optional<std::uint32_t> seed,
  std::optional<std::filesystem::path> save) const {
 return gameplay(directory,XeenActorApproach::kEntry,save,false,XeenEncounterEntry::Journey,seed);
}
int Application::playGameplay(const XeenGameplayServices &supplied, XeenCamera camera,
  const std::optional<std::filesystem::path> &target, bool resume, XeenEncounterEntry entry, std::optional<std::uint32_t> seed) const {
 try {
  XeenGameplayServices services = supplied;
  std::function<void()> sourceCheck;
  const auto callback = [&](auto &&provider) {
   if (sourceCheck) sourceCheck();
   try { auto value = provider(); if (sourceCheck) sourceCheck(); return value; }
   catch (...) { if (sourceCheck) sourceCheck(); throw; }
  };
  services.maps = [&](XeenMapIdentity id) { return callback([&] { return supplied.maps(id); }); };
  services.objects = [&](XeenMapIdentity id) { return callback([&] { return supplied.objects(id); }); };
  services.resources.loadInitialParty = [&] { return callback(supplied.resources.loadInitialParty); };
  services.resources.loadEvents = [&](XeenMapIdentity id) { return callback([&] { return supplied.resources.loadEvents(id); }); };
  if (supplied.resources.loadInitialCharacters) services.resources.loadInitialCharacters = [&] { return callback(supplied.resources.loadInitialCharacters); };
  if (supplied.resources.loadInitialContext) services.resources.loadInitialContext = [&] { return callback(supplied.resources.loadInitialContext); };
  if (supplied.resources.loadMonsterStatistics) services.resources.loadMonsterStatistics = [&] { return callback(supplied.resources.loadMonsterStatistics); };
  services.compose = [&](auto &w, const auto &p, const auto &c, auto phase) { return callback([&] { return supplied.compose(w,p,c,phase); }); };
  if (supplied.composeEncounter) services.composeEncounter = [&](auto &w, const auto &p, const auto &c, auto phase, auto actor) {
   return callback([&] { return supplied.composeEncounter(w,p,c,phase,actor); });
  };
  bool encounter = entry != XeenEncounterEntry::Ordinary;
  if (encounter && resume) throw std::invalid_argument("Encounter entry cannot resume");
  if (seed && (resume || entry != XeenEncounterEntry::Journey || !*seed)) throw std::invalid_argument("Invalid Journey seed override");
  XeenWorld world(services.maps, services.objects);
  if (encounter && entry != XeenEncounterEntry::Journey) world.markEncounterSession(entry);
  XeenPartyState party;
  XeenGameFlags flags;
  const auto preflight = [&](XeenWorld &w, const XeenPartyState &p, const XeenCamera &c, const XeenGameFlags &) {
   if (w.sessionState().journey()) {
    if (!services.composeEncounter) throw std::invalid_argument("Missing Journey presentation provider");
    if (!services.composeEncounter(w,p,c,0,XeenMonsterAppearance{0}).frame.isValid())
     throw std::runtime_error("Invalid Journey first frame");
   } else if (w.sessionState().completion() == XeenEncounterCompletion::VictoryQuiescent) {
    if (!services.composeEncounter) throw std::invalid_argument("Missing completed presentation provider");
    auto frame = services.composeEncounter(w,p,c,0,XeenMonsterAppearance{0}).frame;
    // Candidate facts are intentionally unpublished. Presentation neither
    // captures them nor creates GameplayBorrow/combat preparation authority.
    static_cast<void>(XeenEventFlow::preflightCompleted(std::move(frame), services.font, services.catalog, w,p,c));
   } else if (!services.compose(w, p, c, std::uint64_t{0}).frame.isValid()) throw std::runtime_error("Invalid first gameplay frame");
   if (sourceCheck) sourceCheck();
  };
  if (resume) {
   if (!target) throw std::runtime_error("Resume requires a save path");
   const auto saved = XeenSaveFile::read(*target);
   XeenSaveState::restoreBeforeGameplay(saved, services.resources, party, camera, flags, world, preflight);
   entry = world.sessionState().encounterEntry();
   encounter = entry != XeenEncounterEntry::Ordinary;
  } else {
   party = services.resources.loadInitialParty();
   flags = services.initialFlags();
  }
  for (const auto &diagnostic : party.diagnostics) std::cerr << "Party warning: " << diagnostic << '\n';
  XeenEventSystem events([&](XeenMapIdentity id) { return XeenEventScript(services.resources.loadEvents(id)); }, services.texts);
  const auto encounterEvents = encounter && !resume ? services.resources.loadEvents(20) : XeenEventFile{};
  std::optional<XeenEncounterSetup> setup;
  if (encounter && entry != XeenEncounterEntry::Journey) setup.emplace(XeenEncounterSetup{encounterEvents, services.initializeEncounter, services.validateEncounterSprite});
  std::vector<std::uint8_t> journeyCharacters;
  std::vector<XeenMonsterRecord> journeyStatistics;
  std::optional<XeenJourneySetup> journeySetup;
  if (entry == XeenEncounterEntry::Journey && !resume) {
   if (!services.resources.loadInitialCharacters || !services.resources.loadInitialContext || !services.resources.loadMonsterStatistics)
    throw std::invalid_argument("Missing Journey initialization providers");
   journeyCharacters = services.resources.loadInitialCharacters();
   journeyStatistics = services.resources.loadMonsterStatistics();
   auto value = seed ? *seed : (services.sampleJourneySeed ? services.sampleJourneySeed() : std::random_device{}());
   if (!value) value = 1;
   journeySetup.emplace(XeenJourneySetup{journeyCharacters,services.resources.loadInitialContext(),journeyStatistics,encounterEvents,value});
  }
  if (entry == XeenEncounterEntry::Diagnostic27 && !resume) {
   if (!services.prepareCombat) throw std::invalid_argument("Missing combat preparation provider");
   setup->prepareCombat = services.prepareCombat;
   setup->validateAttackSprite = services.validateCombatSprite;
  }
  XeenEventFlow flow(world, events, party, camera, flags, services.font,
   [&](std::uint64_t phase) { return services.compose(world, party, camera, phase); }, services.npcDraw, services.clock, {}, services.catalog,
   setup ? &*setup : nullptr, [&](std::uint64_t ordinary, XeenMonsterAppearance actor) {
    const auto observedCamera = camera;
    if (entry == XeenEncounterEntry::Journey) {
     XeenRestoreGuard guard(world, party, camera, flags);
     XeenRestoreGuard::Providers providers(guard, world);
     try {
      auto frame = services.composeEncounter(world, party, observedCamera, ordinary, actor);
      guard.check();
      return frame;
     } catch (...) { guard.check(); throw; }
    }
    if (entry == XeenEncounterEntry::Diagnostic27)
     return services.composeEncounter(world, party, observedCamera, ordinary, actor);
    const auto observedParty = party;
    return services.composeEncounter(world, observedParty, observedCamera, ordinary, actor);
   }, journeySetup ? &*journeySetup : nullptr);
  EncounterHandoff handoff(flow);
  flow.completedMonsters = services.resources.loadMonsterStatistics;
  flow.completedEvents = services.resources.loadEvents;
  flow.completedPreflight = preflight;
  flow.prepareJourneySprites = [&] {
   if (!services.validateEncounterSprite || !services.validateCombatSprite) throw std::invalid_argument("Missing Journey sprite providers");
   services.validateEncounterSprite(8);
   if (!flow.encounter()->current(flow.encounter()->ticket())) throw std::logic_error("Stale Journey normal sprite preparation");
   services.validateCombatSprite(8);
  };
  if (services.configureFlow) services.configureFlow(flow, camera);
  handoff.verify();
  if (!flow.frame().isValid()) throw std::runtime_error("Invalid first gameplay frame");
  std::cout << "Setup " << xeenInventoryInspection(party);
  if (flow.journey()) std::cout << flow.encounter()->journeyInspection()
   << "Journey objective: survive the Skeleton, then continue and save. Four cells only: x=13..14, y=1..2. "
      "Time must stay below 960 minutes. No healing, rest, recovery or disengagement. "
      "Arrows/WASD move and turn; period waits; Space interacts outside combat and attacks in combat; B blocks. "
      "I inventory; F1-F6 owner; arrows category; 1-9 slot; T transfer; Enter confirms; E equips/removes; "
      "F9 saves only when quiet with inventory closed. Enter never starts combat; R has no Journey action. Escape cancels/closes or exits.\n";
  // This is the only production new-session/resume initialization choice.
  const auto first = resume || encounter ? flow.frame() : flow.initial();
  if (!first.isValid()) throw std::runtime_error("Invalid first gameplay frame");
  if (services.observeGameplay) services.observeGameplay(world, events, party, camera, flags);
  handoff.verify();
  std::cout << "Map " << camera.mapId << ": camera X=" << camera.x << " Y=" << camera.y
      << " direction=" << static_cast<unsigned>(camera.direction) << '\n';
  const auto &geometry = world.map(camera.mapId).geometry;
  handoff.verify();
  if (!geometry.isOutdoors() && (geometry.flags2 & 0x4000))
   std::cout << "Warning: dark indoor map is rendered illuminated for diagnostics.\n";
  std::string status = "MMModern - Map " + std::to_string(camera.mapId.number);
  status += " - " + xeenInventorySummary(party);
  if (target && entry != XeenEncounterEntry::Diagnostic26) {
   status += " - F9 saves and replaces " + target->u8string();
   std::cout << "F9 saves and replaces " << target->u8string() << '\n';
  }
  if (resume) std::cout << "Resumed " << target->u8string() << '\n';
  bool dispatching = false;
  bool active = true;
  const auto dispatch = [&](const PlayerAction &action, std::optional<std::uint64_t> input) -> std::optional<IndexedFrame> {
   // F9 is intercepted here, so it must pass the same displayed authority gate
   // as every Journey input before capture, providers, or target work.
   if (flow.journey() && !flow.journeyInputCurrent(input)) return std::nullopt;
   // This irreversible entry decision needs no access to possibly closed owners.
   if (std::holds_alternative<SaveGameAction>(action) && encounter && !flow.completed() && !flow.journey()) {
    status = "MMModern - Cannot save: encounter session is unsaveable.";
    return std::nullopt;
   }
   if (!active || dispatching) {
    if (std::holds_alternative<SaveGameAction>(action))
     status = "MMModern - Cannot save outside an idle gameplay boundary.";
    return std::nullopt;
   }
   // Unsafe encounter refusal precedes target handling and all save work.
   if (std::holds_alternative<SaveGameAction>(action) && !flow.completed() &&
       !XeenSaveState::canCapture(party,camera,world)) {
    try { status = "MMModern - Cannot save: encounter session is unsaveable."; }
    catch (...) { handoff.fail(); active = false; throw; }
    return std::nullopt;
   }
   if (std::holds_alternative<SaveGameAction>(action) && flow.completed() && !flow.canSave()) {
    status = flow.inventoryOpen() ? "MMModern - Cannot save while inspection is open. Close it and press F9 again." :
     "MMModern - Cannot save outside a completed idle frame boundary.";
    return std::nullopt;
   }
   GameplayScope scope(dispatching);
   try {
   if (std::holds_alternative<SaveGameAction>(action)) {
    std::string message;
    bool success = false;
    if (flow.inventoryOpen()) message = "Cannot save while inventory is open. Close it and press F9 again.";
    else if (!flow.canSave()) message = "Cannot save while an interaction is pending.";
    else if (!target) message = "No save target configured. Use --save-file <path>.";
    else try {
     xeenSaveGameplay(services,world,party,camera,flags,flow,*target,preflight,&sourceCheck);
     success = true; message = "Saved";
    } catch (const std::exception &e) { message = std::string("Save failed: ") + e.what(); }

    handoff.retain();
    if (target) message += " [" + target->u8string() + "]";
    status = "MMModern - " + message;
    (success ? std::cout : std::cerr) << message << '\n';
    if (flow.inventoryOpen()) return flow.refuseInventorySave();
    if (flow.completed()) {
     auto frame = flow.completedFeedback(success ? "Saved; Escape exits without autosave" : "Save refused/failed; press F9 again");
     handoff.retain();
     return frame;
    }
    return std::nullopt; // Never forward Save to the presenter or clear a label.
   }
   auto mapped = action;
   if (flow.journey() && flow.encounter()->combat() && std::holds_alternative<InteractionAction>(action)) mapped = AttackAction{};
   if (entry == XeenEncounterEntry::Diagnostic27) {
    if (std::holds_alternative<InteractionAction>(action)) mapped = AttackAction{};
    if (std::holds_alternative<AcknowledgeAction>(action) && !flow.inventoryOpen()) mapped = BeginEncounterAction{};
   }
   auto next = flow.handle(mapped,input);
   handoff.retain();
   return next;
   } catch (...) { handoff.fail(); active = false; throw; }
  };
  SdlWindow::FrameUpdateHandler handler = [&](const PlayerAction &action) { return dispatch(action,{}); };
  handler.displayedInput = [&] { return flow.displayedInput(); };
  handler.protectAllKeys = flow.journey();
  handler.withDisplayedInput = [&](const PlayerAction &action,std::uint64_t input) { return dispatch(action,input); };
  handler.beginCycle = [&](std::uint64_t cycle) {
   if (!active) throw std::runtime_error("Gameplay session is closed");
   flow.beginCycle(cycle);
  };
  handler.frameCurrent = [&] { return active && flow.encounterFrameCurrent(); };
  handler.framePresented = [&] { flow.framePresented(); handoff.retain(); };
  handler.failed = [&] { handoff.fail(); active = false; };
  handler.closed = [&] { flow.closeGameplay(); active = false; };
  const auto idle = [&]() -> std::optional<IndexedFrame> {
   if (!active || dispatching) return std::nullopt;
   GameplayScope scope(dispatching);
   try { auto next = flow.updatePresentation(); handoff.retain(); return next; }
   catch (...) { handoff.fail(); active = false; throw; }
  };
  handoff.verify();
  const bool ok = services.show(first, handler, [&] { return flow.handlesEscape(); }, idle, [&] {
   try { return status; } catch (...) { handoff.fail(); active = false; throw; }
  });
  if (!ok) handler.failed();
  flow.closeGameplay();
  active = false;
  flow.abandonPresentation();
  return ok ? 0 : 4;
 } catch (const std::exception &e) {
  std::cerr << "Gameplay startup failed";
  if (target) std::cerr << " [" << target->u8string() << ']';
  std::cerr << ": " << e.what() << '\n';
  return 3;
 }
}
}
