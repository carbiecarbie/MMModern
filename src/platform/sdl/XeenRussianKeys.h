#ifndef MMODERN_PLATFORM_XEEN_RUSSIAN_KEYS_H
#define MMODERN_PLATFORM_XEEN_RUSSIAN_KEYS_H

#include "core/XeenKeyboard.h"
#include <cstdint>

namespace mmodern {
// M110 Part C: the Russian xeen.dat code of a printable key, by its position
// (a USB HID usage, as SDL scancodes are), independent of the host layout.
// Zero for keys outside the printable block, which keep the CD's codes.
namespace xeen_russian_keys {
// Set-1 scancode of a printable position.
inline unsigned set1(unsigned usage) {
    static constexpr std::uint8_t letters[26]{0x1e,0x30,0x2e,0x20,0x12,0x21,0x22,0x23,0x17,0x24,0x25,0x26,0x32,
        0x31,0x18,0x19,0x10,0x13,0x1f,0x14,0x16,0x2f,0x11,0x2d,0x15,0x2c};
    if(usage>=4 && usage<=29)return letters[usage-4];   // A-Z
    if(usage>=30 && usage<=39)return 0x02+(usage-30);   // 1-9, 0
    switch(usage) {
    case 44:return 0x39;case 45:return 0x0c;case 46:return 0x0d;case 47:return 0x1a;case 48:return 0x1b;
    case 49:return 0x2b;case 51:return 0x27;case 52:return 0x28;case 53:return 0x29;case 54:return 0x33;
    case 55:return 0x34;case 56:return 0x35;
    default:return 0;
    }
}
// The CD table's code at a printable Set-1 position.
inline unsigned plain(unsigned scan) {
    static constexpr char codes[]="\0\0" "1234567890-=" "\0\0" "QWERTYUIOP[]" "\0\0" "ASDFGHJKL;'`" "\0" "\\ZXCVBNM,./" "\0\0\0" " ";
    return scan<sizeof(codes)-1?static_cast<unsigned char>(codes[scan]):0u;
}
// The CD's Shift list at a Set-1 position (CD 0x4549c..), as the Russian
// program applies it with Ctrl. Minus stays minus: the program's inverse
// lookup finds the keypad duplicate first.
inline unsigned symbol(unsigned scan) {
    if(scan>=0x02 && scan<=0x0b)return static_cast<unsigned char>("!@#$%^&*()"[scan-0x02]);
    switch(scan) {
    case 0x0d:return '+';case 0x1a:return '{';case 0x1b:return '}';case 0x27:return ':';case 0x28:return '"';
    case 0x29:return '~';case 0x2b:return '|';case 0x33:return '<';case 0x34:return '>';case 0x35:return '?';
    default:return 0;
    }
}
}
// RU 0x43f2a..0x43fd7: the letter rows from the Russian table; Shift first
// (Cyrillic capitals only), then Ctrl (the Shift list's symbol).
inline unsigned xeenRussianKeyCode(const XeenKeyboard &keyboard,unsigned usage,bool shift,bool ctrl) {
    const auto scan=xeen_russian_keys::set1(usage);
    if(!scan)return 0;
    const unsigned code=scan<keyboard.letters.size() && keyboard.letters[scan]?keyboard.letters[scan]:xeen_russian_keys::plain(scan);
    if(shift)return xeenRussianShift(code);
    if(ctrl)if(const auto symbol=xeen_russian_keys::symbol(scan))return symbol;
    return code;
}
}
#endif
