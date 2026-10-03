#ifndef MMODERN_CORE_INPUT_CONTEXT_H
#define MMODERN_CORE_INPUT_CONTEXT_H
#include <cstdint>
namespace mmodern {
enum class MainScreen { None, Exploration, Combat };
// Flow supplies authority; SDL retains only actions and this context incarnation.
struct InputContext {
    std::uint64_t contextId = 0;
    bool acceptsQueuedInput = false;
    bool readyForAction = false;
    MainScreen mainScreen = MainScreen::None;
};
}
#endif
