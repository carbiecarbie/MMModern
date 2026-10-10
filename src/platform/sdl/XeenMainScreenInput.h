#ifndef MMODERN_PLATFORM_XEEN_MAIN_SCREEN_INPUT_H
#define MMODERN_PLATFORM_XEEN_MAIN_SCREEN_INPUT_H

#include "core/InputContext.h"
#include "core/PlayerAction.h"
#include <optional>
#include <cstring>

namespace mmodern {
inline std::optional<PlayerAction> xeenMainScreenMemberKey(unsigned key) {
    if(key>=InputKey::F1 && key<InputKey::F1+6)return SelectMemberAction{key-InputKey::F1};
    return {};
}
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
        if (!combat && index==0) return ShootAction{}; // S
        if (!combat && index==2) return RestAction{}; // R
        if (combat && index==2) return AttackAction{}; // A
        if (combat && index==4) return RevisitCompletedAction{}; // R -> Run
        if (combat && index==5) return BlockAction{}; // B
        if (!combat && index==3) return BashAction{};
        if (combat && index==3) return UseItemAction{}; // U
        if (index==7) return InfoAction{};
        if (index==8) return QuickReferenceAction{};
        constexpr const char *exploration[] = {"Shoot","Cast","Rest","Bash","Dismiss","View Quests","Map","Info","Quick Ref"};
        constexpr const char *fighting[] = {"Quick Fight","Cast","Attack","Use","Run","Block","Quick Fight Options","Info","Quick Ref"};
        return UnsupportedMainScreenAction{combat ? fighting[index] : exploration[index]};
    }
    if (contains(109,137,122,147)) return ControlPanelAction{};
    for (unsigned col=0; col<3; ++col) {
        if (contains(columns[col],148,columns[col]+24,168)) {
            constexpr NavigationAction movement[] = {NavigationAction::TurnLeft,NavigationAction::MoveForward,NavigationAction::TurnRight};
            return movement[col];
        }
        if (contains(columns[col],169,columns[col]+24,189)) {
            if (col==1) return NavigationAction::MoveBackward;
            return col==0?NavigationAction::StrafeLeft:NavigationAction::StrafeRight;
        }
    }
    if (combat) for (unsigned row=0; row<3; ++row)
        if (contains(239,27+10*row,312,37+10*row)) return SelectInventorySlotAction{row}; // 1-3
    constexpr int faces[] = {10,45,81,117,153,189};
    for (unsigned index=0;index<6;++index) if (contains(faces[index],150,faces[index]+32,182))
        return SelectMemberAction{index};
    if (contains(8,8,224,140)) return combat ? PlayerAction{AttackAction{}} : PlayerAction{InteractionAction{}};
    return {};
}
inline std::optional<InputButton> xeenMainScreenButtonAt(int x,int y,MainScreen screen) {
    if(screen==MainScreen::None) return {};
    const char *resource=screen==MainScreen::Combat?"combat.icn":"main.icn";
    constexpr int columns[]{235,260,286};
    for(unsigned index=0;index<16;++index) {
        const int left=index==9?109:columns[index<9?index%3:(index-10)%3];
        const int top=index<9?75+21*(index/3):index==9?137:index<13?148:169;
        if(x>=left && x<left+(index==9?13:24) && y>=top && y<top+(index==9?10:20))
            return InputButton{resource,index*2,left,top};
    }
    return {};
}
inline std::optional<InputButton> xeenMainScreenButton(const PlayerAction &action,MainScreen screen) {
    constexpr int columns[]{235,260,286};
    for(unsigned index=0;index<16;++index) {
        const int x=index==9?109:columns[index<9?index%3:(index-10)%3];
        const int y=index<9?75+21*(index/3):index==9?137:index<13?148:169;
        const auto candidate=xeenMainScreenClick(x,y,screen);
        if(!candidate || candidate->index()!=action.index()) continue;
        if(const auto *nav=std::get_if<NavigationAction>(&action);nav && *nav!=std::get<NavigationAction>(*candidate)) continue;
        if(const auto *notice=std::get_if<UnsupportedMainScreenAction>(&action);
            notice && std::strcmp(notice->label,std::get<UnsupportedMainScreenAction>(*candidate).label)) continue;
        return xeenMainScreenButtonAt(x,y,screen);
    }
    return {};
}
inline std::optional<InputButton> xeenMainScreenKeyButton(const std::optional<PlayerAction> &action,unsigned key,MainScreen screen) {
    if(action) if(const auto button=xeenMainScreenButton(*action,screen)) return button;
    // Existing MMModern shortcuts take precedence. An otherwise ignored
    // original shortcut can still provide its cosmetic button feedback.
    constexpr unsigned exploration[]{'s','c','r','b','d','v','m','i','q',9,InputKey::Left,InputKey::Up,InputKey::Right,0,InputKey::Down,0};
    constexpr unsigned combat[]{'f','c','a','u','r','b','o','i','q',9,InputKey::Left,InputKey::Up,InputKey::Right,0,InputKey::Down,0};
    const auto &keys=screen==MainScreen::Combat?combat:exploration;
    constexpr int columns[]{235,260,286};
    for(unsigned index=0;index<16;++index) if(keys[index] && keys[index]==key) {
        const int x=index==9?109:columns[index<9?index%3:(index-10)%3];
        const int y=index<9?75+21*(index/3):index==9?137:index<13?148:169;
        return xeenMainScreenButtonAt(x,y,screen);
    }
    return {};
}
}
#endif
