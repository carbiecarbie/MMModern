#include "app/Application.h"
#include "app/XeenGameplayServices.h"
#include "platform/XeenSaveFile.h"
#include <iostream>
#include <stdexcept>
namespace mmodern {
int Application::playGameplay(const XeenGameplayServices &services, XeenCamera camera,
  const std::optional<std::filesystem::path> &target, bool resume) const {
 try {
  XeenWorld world(services.maps, services.objects);
  XeenPartyState party;
  XeenGameFlags flags;
  const auto preflight = [&](XeenWorld &w, const XeenPartyState &p, const XeenCamera &c, const XeenGameFlags &) {
   if (!services.compose(w, p, c).isValid()) throw std::runtime_error("Invalid first gameplay frame");
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
  XeenEventFlow flow(world, events, party, camera, flags, services.font,
   [&] { return services.compose(world, party, camera); }, services.npcDraw);
  if (services.configureFlow) services.configureFlow(flow, camera);
  // This is the only production new-session/resume initialization choice.
  const auto first = resume ? flow.frame() : flow.initial();
  if (!first.isValid()) throw std::runtime_error("Invalid first gameplay frame");
  if (services.observeGameplay) services.observeGameplay(world, events, party, camera, flags);
  std::cout << "Map " << camera.mapId << ": camera X=" << camera.x << " Y=" << camera.y
      << " direction=" << static_cast<unsigned>(camera.direction) << '\n';
  const auto &geometry = world.map(camera.mapId).geometry;
  if (!geometry.isOutdoors() && (geometry.flags2 & 0x4000))
   std::cout << "Warning: dark indoor map is rendered illuminated for diagnostics.\n";
  std::string status = "MMModern - Map " + std::to_string(camera.mapId.number);
  if (target) {
   status += " - F9 saves and replaces " + target->u8string();
   std::cout << "F9 saves and replaces " << target->u8string() << '\n';
  }
  if (resume) std::cout << "Resumed " << target->u8string() << '\n';
  bool dispatching = false;
  bool active = true;
  const auto handler = [&](const PlayerAction &action) -> std::optional<IndexedFrame> {
   if (std::holds_alternative<SaveGameAction>(action)) {
    std::string message;
    bool success = false;
    if (!active || dispatching) message = "Cannot save outside an idle gameplay boundary.";
    else if (flow.blocksGameplay()) message = "Cannot save while an interaction is pending.";
    else if (!target) message = "No save target configured. Use --save-file <path>.";
    else try {
     const auto snapshot = XeenSaveState::capture(services.resources.signature, party, camera, flags, world);
     // Validate capture against original resources without touching live owners/caches.
     XeenWorld candidate(services.maps, services.objects);
     XeenPartyState p; XeenCamera c; XeenGameFlags f;
     XeenSaveState::restoreBeforeGameplay(snapshot, services.resources, p, c, f, candidate, preflight);
     XeenSaveFile::write(*target, snapshot);
     success = true; message = "Saved";
    } catch (const std::exception &e) { message = std::string("Save failed: ") + e.what(); }
    if (target) message += " [" + target->u8string() + "]";
    status = "MMModern - " + message;
    (success ? std::cout : std::cerr) << message << '\n';
    return std::nullopt; // Never forward Save to the presenter or clear a label.
   }
   if (!active || dispatching) return std::nullopt;
   dispatching = true;
   try { auto frame = flow.handle(action); dispatching = false; return frame; }
   catch (...) { active = false; dispatching = false; throw; }
  };
  const auto idle = [&]() -> std::optional<IndexedFrame> {
   dispatching = true;
   try { auto frame = flow.updatePresentation(); dispatching = false; return frame; }
   catch (...) { active = false; dispatching = false; throw; }
  };
  const bool ok = services.show(first, handler, [&] { return flow.handlesEscape(); }, idle, [&] { return status; });
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
