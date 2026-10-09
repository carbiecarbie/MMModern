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
}
// ButtonContainer: selected sprite frame = normal frame | 1, held for two
// 50 ms presentation frames. This has no game-clock or action state.
struct InputButton {
    const char *resource;
    unsigned frame;
    int x,y;
    unsigned pressedFrame() const noexcept { return frame | 1u; }
};
inline constexpr unsigned kButtonFeedbackMilliseconds=100;
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
    std::optional<InputButton> button(unsigned key) const {
        for(const auto &hit:hits) if(hit.key==key) return hit.button;
        return {};
    }
    std::optional<PlayerAction> key(unsigned key) const {
        for(auto allowed:keys) if(key==allowed) return DialogKeyAction{key};
        if(anyKey) return AcknowledgeAction{};
        return {};
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
