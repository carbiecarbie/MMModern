#include "XeenWrapSymbols.h"
#ifndef MMODERN_RESTORE_REPLAY_PROBE_H
#define MMODERN_RESTORE_REPLAY_PROBE_H
#include <functional>
#include <optional>
#include "games/xeen/XeenJourneyContent.h"
// Executable-only GNU linker probes for the compiler-derived test ABI.
// No callbacks or counters are installed in production gameplay owners.
#define XEEN_REPLAY_SERVICE XEEN_WRAP_COMBAT_SERVICE
#define XEEN_REPLAY_MOVE XEEN_WRAP_ACTOR_MOVE
#define XEEN_REPLAY_REGIONAL_MOVE XEEN_WRAP_ACTOR_REGIONAL_MOVE
#define XEEN_REPLAY_TIME XEEN_WRAP_PREPARE_TIME
#define XEEN_REPLAY_TRANSFER XEEN_WRAP_TRANSFER_ITEM
#define XEEN_REPLAY_EQUIPMENT XEEN_WRAP_SET_EQUIPMENT
#define XEEN_REPLAY_EVENT XEEN_WRAP_EVENT_EXECUTE
#define XEEN_REPLAY_EVENT_BEGIN XEEN_WRAP_EVENT_BEGIN
namespace replay_test {
#define XEEN_REPLAY_JOURNEY_INITIALIZE XEEN_WRAP_INITIALIZE_JOURNEY
#define XEEN_REPLAY_FRESH_PUBLICATION_INITIALIZE XEEN_WRAP_INITIALIZE_FRESH_JOURNEY
#define XEEN_REPLAY_JOURNEY_CONSTRUCT XEEN_WRAP_JOURNEY_CONSTRUCTOR
#define XEEN_REPLAY_ACTION XEEN_WRAP_ACTOR_ACTION
#define XEEN_REPLAY_PULSE XEEN_WRAP_ACTOR_PULSE
#define XEEN_REPLAY_RETIRE XEEN_WRAP_RETIRE_JOURNEY
#define XEEN_REPLAY_COMMAND XEEN_WRAP_COMBAT_COMMAND
#define XEEN_REPLAY_DRAW XEEN_WRAP_RANDOM_DRAW
extern std::function<std::optional<std::uint32_t>(std::optional<std::uint32_t>)> filterDraw; // Artificial bounded-rejection fixture only.
extern std::function<void(std::uint32_t,std::uint32_t,std::optional<std::uint32_t>,mmodern::XeenJourneyRandomState)> observeDraw;
extern unsigned journeyInitializations, journeyConstructions, actions, pulses, retirements, commands, draws;
extern unsigned depth, unexpected, constructions, services, preparations;
extern unsigned timePreparations, eventExecutions, transfers, equipmentChanges;
struct Scope { Scope() { ++depth; } ~Scope() { --depth; } };
}
#endif
