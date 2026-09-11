#include "app/Application.h"
#include "app/XeenGameplayServices.h"
#include "platform/XeenSaveFile.h"
#include <iostream>
#include <stdexcept>
#include <exception>
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
int Application::playGameplay(const XeenGameplayServices &services, XeenCamera camera,
  const std::optional<std::filesystem::path> &target, bool resume, XeenEncounterEntry entry) const {
 try {
  const bool encounter = entry == XeenEncounterEntry::Diagnostic26;
  if (encounter && resume) throw std::invalid_argument("Encounter entry cannot resume");
  XeenWorld world(services.maps, services.objects);
  if (encounter) world.markEncounterSession();
  XeenPartyState party;
  XeenGameFlags flags;
  const auto preflight = [&](XeenWorld &w, const XeenPartyState &p, const XeenCamera &c, const XeenGameFlags &) {
   if (!services.compose(w, p, c, std::uint64_t{0}).frame.isValid()) throw std::runtime_error("Invalid first gameplay frame");
  };
  if (resume) {
   if (!target) throw std::runtime_error("Resume requires a save path");
   const auto saved = XeenSaveFile::read(*target);
   XeenSaveState::restoreBeforeGameplay(saved, services.resources, party, camera, flags, world, preflight);
  } else {
   party = services.resources.loadInitialParty();
   flags = services.initialFlags();
  }
  for (const auto &diagnostic : party.diagnostics) std::cerr << "Party warning: " << diagnostic << '\n';
  XeenEventSystem events([&](XeenMapIdentity id) { return XeenEventScript(services.resources.loadEvents(id)); }, services.texts);
  const auto encounterEvents = encounter ? services.resources.loadEvents(20) : XeenEventFile{};
  std::optional<XeenEncounterSetup> setup;
  if (encounter) setup.emplace(XeenEncounterSetup{encounterEvents, services.initializeEncounter, services.validateEncounterSprite});
  XeenEventFlow flow(world, events, party, camera, flags, services.font,
   [&](std::uint64_t phase) { return services.compose(world, party, camera, phase); }, services.npcDraw, services.clock, {}, services.catalog,
   setup ? &*setup : nullptr, [&](std::uint64_t ordinary, std::uint8_t actor) {
    const auto observedParty = party;
    const auto observedCamera = camera;
    return services.composeEncounter(world, observedParty, observedCamera, ordinary, actor);
   });
  EncounterHandoff handoff(flow);
  if (services.configureFlow) services.configureFlow(flow, camera);
  handoff.verify();
  if (!flow.frame().isValid()) throw std::runtime_error("Invalid first gameplay frame");
  std::cout << "Setup " << xeenInventoryInspection(party);
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
  if (target && !encounter) {
   status += " - F9 saves and replaces " + target->u8string();
   std::cout << "F9 saves and replaces " << target->u8string() << '\n';
  }
  if (resume) std::cout << "Resumed " << target->u8string() << '\n';
  bool dispatching = false;
  bool active = true;
  SdlWindow::FrameUpdateHandler handler = [&](const PlayerAction &action) -> std::optional<IndexedFrame> {
   // Irreversible session policy precedes busy/modal/target checks and all save work.
   if (std::holds_alternative<SaveGameAction>(action) && (world.hasEncounterState() || party.encounterContext)) {
    try { status = "MMModern - Cannot save: M26 encounter session is unsaveable."; }
    catch (...) { handoff.fail(); active = false; throw; }
    return std::nullopt;
   }
   if (!active || dispatching) {
    if (std::holds_alternative<SaveGameAction>(action))
     status = "MMModern - Cannot save outside an idle gameplay boundary.";
    return std::nullopt;
   }
   GameplayScope scope(dispatching);
   try {
   if (std::holds_alternative<SaveGameAction>(action)) {
    std::string message;
    bool success = false;
    if (flow.inventoryOpen()) message = "Cannot save while inventory is open. Close it and press F9 again.";
    else if (flow.blocksGameplay()) message = "Cannot save while an interaction is pending.";
    else if (!target) message = "No save target configured. Use --save-file <path>.";
    else try {
     if (services.observeSaveStage) services.observeSaveStage(XeenGameplayServices::SaveStage::Capture);
     const auto snapshot = XeenSaveState::capture(services.resources.signature, party, camera, flags, world);
     // Validate capture against original resources without touching live owners/caches.
     XeenWorld candidate(services.maps, services.objects);
     XeenPartyState p; XeenCamera c; XeenGameFlags f;
     if (services.observeSaveStage) services.observeSaveStage(XeenGameplayServices::SaveStage::Preflight);
     XeenSaveState::restoreBeforeGameplay(snapshot, services.resources, p, c, f, candidate, preflight);
     if (services.observeSaveStage) services.observeSaveStage(XeenGameplayServices::SaveStage::Write);
     XeenSaveFile::write(*target, snapshot);
     success = true; message = "Saved";
    } catch (const std::exception &e) { message = std::string("Save failed: ") + e.what(); }
    if (target) message += " [" + target->u8string() + "]";
    status = "MMModern - " + message;
    (success ? std::cout : std::cerr) << message << '\n';
    if (flow.inventoryOpen()) return flow.refuseInventorySave();
    return std::nullopt; // Never forward Save to the presenter or clear a label.
   }
   auto next = flow.handle(action);
   handoff.retain();
   return next;
   } catch (...) { handoff.fail(); active = false; throw; }
  };
  handler.beginCycle = [&](std::uint64_t cycle) {
   if (!active) throw std::runtime_error("Gameplay session is closed");
   flow.beginCycle(cycle);
  };
  handler.frameCurrent = [&] { return active && flow.encounterFrameCurrent(); };
  handler.failed = [&] { handoff.fail(); active = false; };
  handler.closed = [&] { active = false; };
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
