#ifndef MMODERN_PLATFORM_XEEN_MAIN_SCREEN_INPUT_H
#define MMODERN_PLATFORM_XEEN_MAIN_SCREEN_INPUT_H

#include "core/InputContext.h"
#include "core/PlayerAction.h"
#include <optional>

namespace mmodern {
// Rectangles and meanings adapted from ScummVM 6814ee9ba54582f5b5adcffab49efbbd8f589edd:
// engines/mm/xeen/interface.cpp Interface::setMainButtons/doCombat, and
// dialogs/dialogs.cpp ButtonContainer::addPartyButtons/setWaitBounds.
// GPL-3.0-or-later; ScummVM developers (upstream COPYRIGHT).
// Coordinates are SDL's logical 320x200 coordinates; right/bottom are exclusive.
inline std::optional<PlayerAction> xeenMainScreenClick(int x, int y, MainScreen screen) {
    if (screen == MainScreen::None || x < 0 || y < 0 || x >= 320 || y >= 200) return {};
    const auto contains = [=](int l, int t, int r, int b) { return x >= l && x < r && y >= t && y < b; };
    const bool combat = screen == MainScreen::Combat;
    constexpr int columns[] = {235,260,286};
    for (unsigned row=0; row<3; ++row) for (unsigned col=0; col<3; ++col) {
        if (!contains(columns[col],75+21*row,columns[col]+24,95+21*row)) continue;
        const unsigned index=row*3+col;
        if (index==1) return CastSpellAction{}; // C
        if (!combat && index==0) return ShootAction{}; // MMModern F
        if (combat && index==2) return InteractionAction{}; // MMModern Space -> Attack
        if (combat && index==4) return RevisitCompletedAction{}; // R -> Run
        if (combat && index==5) return BlockAction{}; // B
        constexpr const char *exploration[] = {"Shoot","Cast","Rest","Bash","Dismiss","View Quests","Map","Info","Quick Ref"};
        constexpr const char *fighting[] = {"Quick Fight","Cast","Attack","Use","Run","Block","Quick Fight Options","Info","Quick Ref"};
        return UnsupportedMainScreenAction{combat ? fighting[index] : exploration[index]};
    }
    if (contains(109,137,122,147)) return UnsupportedMainScreenAction{"Control panel"};
    for (unsigned col=0; col<3; ++col) {
        if (contains(columns[col],148,columns[col]+24,168)) {
            constexpr NavigationAction movement[] = {NavigationAction::TurnLeft,NavigationAction::MoveForward,NavigationAction::TurnRight};
            return movement[col];
        }
        if (contains(columns[col],169,columns[col]+24,189)) {
            if (col==1) return NavigationAction::MoveBackward;
            return UnsupportedMainScreenAction{"Strafe"};
        }
    }
    if (combat) for (unsigned row=0; row<3; ++row)
        if (contains(239,27+10*row,312,37+10*row)) return SelectInventorySlotAction{row}; // 1-3
    constexpr int faces[] = {10,45,81,117,153,189};
    for (const auto left : faces) if (contains(left,150,left+32,182))
        return UnsupportedMainScreenAction{"Character sheet"}; // Original F1-F6, deferred to M47.
    if (contains(8,8,224,140)) return InteractionAction{}; // Space, after buttons (Tab overlaps).
    return {};
}
}
#endif
