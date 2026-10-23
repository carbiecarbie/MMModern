#ifndef MMODERN_CORE_INPUT_CONTEXT_H
#define MMODERN_CORE_INPUT_CONTEXT_H
#include <cstdint>
#include "core/DialogInput.h"
#include <memory>
namespace mmodern {
enum class MainScreen { None, Exploration, Combat };
// Flow supplies authority; SDL retains only actions and this context incarnation.
struct InputContext {
    std::uint64_t contextId = 0;
    bool acceptsQueuedInput = false;
    bool readyForAction = false;
    MainScreen mainScreen = MainScreen::None;
    std::shared_ptr<const DialogInput> dialog;
    bool hideCursor=false;
    bool restAvailable=false;
    // Only ordinary navigation may bypass a simple pending actor countdown.
    bool readyForNavigation=false;
    // Navigation has every authority fence; only the current draw wait remains.
    bool navigationDrawPending=false;
    // Explicit DOS keyboard-clear sites, independent of presentation changes.
    std::uint64_t keyboardClear=0;
    bool retainEventKeys=false;
    unsigned partySize=6;
    std::uint64_t mouseClear=0;
    // Ordinary actions have every authority fence; only the draw wait remains.
    bool actionDrawPending=false;
};
}
#endif
