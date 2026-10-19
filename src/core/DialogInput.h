#ifndef MMODERN_CORE_DIALOG_INPUT_H
#define MMODERN_CORE_DIALOG_INPUT_H
#include "core/PlayerAction.h"
#include <optional>
#include <vector>
namespace mmodern {
// Portable key symbols used by the presentation hit/key tables. ASCII letters
// and digits retain their values; SDL only translates platform key symbols.
namespace InputKey {
inline constexpr unsigned Escape=27, Enter=13, Space=32;
inline constexpr unsigned F1=256, Up=272, Down=273, Left=274, Right=275;
inline constexpr unsigned PageUp=276, PageDown=277, Home=278, End=279;
}
// DOS keyboard feedback has no tick dwell; mouse feedback waits for release.
// DAT 0x0e1ca..0x0e2b1 and 0x0e41e..0x0e430.
struct InputButton {
    const char *resource;
    unsigned frame;
    int x,y;
    unsigned pressedFrame() const noexcept { return frame | 1u; }
};
struct DialogHit {
    int left,top,right,bottom;
    unsigned key;
    std::optional<InputButton> button;
};
struct DialogInput {
    std::vector<DialogHit> hits;
    std::vector<unsigned> keys;
    bool anyKey=false, anyClick=false;
    bool textEntry=false;
    // Original Events::isKeyMousePressed waits accept either mouse button
    // and ignore modifier-only keys. Opt in for dialogs using that wait.
    bool keyMouseWait=false;
    std::optional<InputButton> button(unsigned key) const {
        for(const auto &hit:hits) if(hit.key==key) return hit.button;
        return {};
    }
    std::optional<PlayerAction> key(unsigned key) const {
        for(auto allowed:keys) if(key==allowed) return DialogKeyAction{key};
        if(anyKey) return AcknowledgeAction{};
        return {};
    }
    std::optional<PlayerAction> rightClick() const {
        return keyMouseWait?std::optional<PlayerAction>{AcknowledgeAction{}}:key(InputKey::Escape);
    }
    std::optional<PlayerAction> click(int x,int y) const {
        if(x<0 || y<0 || x>=320 || y>=200) return {};
        if(anyClick) return AcknowledgeAction{};
        for(const auto &hit:hits) if(x>=hit.left && x<hit.right && y>=hit.top && y<hit.bottom)
            return DialogKeyAction{hit.key};
        return {};
    }
};
}
#endif
