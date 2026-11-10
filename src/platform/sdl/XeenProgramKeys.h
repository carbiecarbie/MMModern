#ifndef MMODERN_PLATFORM_XEEN_PROGRAM_KEYS_H
#define MMODERN_PLATFORM_XEEN_PROGRAM_KEYS_H

#include "core/InputContext.h"

namespace mmodern {

// The internal key (core/DialogInput.h InputKey codes, lowercase ASCII) of a
// program key code, shared by SdlWindow's translation and the controller
// mapping. F10's code is consumed by its DOS-prompt wrapper; Russian letter
// codes in these ranges keep the meaning this gives them.
inline unsigned xeenProgramInputKey(unsigned code) {
    if (code == 0xd2) return 0;
    if (code >= 0xc9 && code <= 0xd1) return InputKey::F1 + code - 0xc9;
    switch (code) {
    case 0xf0: return InputKey::Left; case 0xf1: return InputKey::Right;
    case 0xf2: return InputKey::Up; case 0xf3: return InputKey::Down;
    case 0xdc: return InputKey::Home; case 0xdd: return InputKey::End;
    case 0xde: return InputKey::PageUp; case 0xdf: return InputKey::PageDown;
    default: return code >= 'A' && code <= 'Z' ? code + ('a' - 'A') : code;
    }
}

// The Russian program key set (XeenKeyboard) that translates keys in this
// context, or null for none: a dialog's own set (possibly none), otherwise
// the main screen's.
inline const char *xeenContextKeySet(const InputContext &context) {
    if (context.dialog) return context.dialog->keySet;
    return context.mainScreen == MainScreen::Combat ? "COMBAT" :
        context.mainScreen == MainScreen::Exploration ? "MAIN" : "YESNO";
}

} // namespace mmodern

#endif
