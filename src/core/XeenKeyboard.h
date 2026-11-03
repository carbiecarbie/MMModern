#ifndef MMODERN_CORE_XEEN_KEYBOARD_H
#define MMODERN_CORE_XEEN_KEYBOARD_H
#include <array>
#include <cstdint>
#include <map>
#include <string>
#include <string_view>
#include <utility>
#include <vector>
namespace mmodern {
// M110 Part C: the Russian Darkside program's keyboard and key sets, read
// from the Russian xeen.dat by XeenDosText. The program has one scancode
// table (RU 0x44176) whose three letter rows are lowercase JCUKEN cp866;
// there is no second table and no switch. Dialogs and the main screen keep
// their English internal keys: a key set maps each Russian program key to the
// internal key of the CD key at the same index. English has no keyboard here.
struct XeenKeyboard {
    // cp866 code of each Set-1 scancode below 0x36 that the Russian table
    // changes; zero keeps the CD code.
    std::array<std::uint8_t,0x36> letters{};
    struct Keys {
        // A typed program key and the internal key it stands for.
        std::vector<std::pair<unsigned,unsigned>> aliases;
        // An internal hit key whose Russian button value matches no case of
        // the Russian dispatch, and that value: the click does nothing.
        std::vector<std::pair<unsigned,unsigned>> clicks;
    };
    std::map<std::string,Keys,std::less<>> sets;
    // Combat target rows (main-screen button entries 17-19) and their
    // highlight: CD y 27/37/47, height 10; the Russian program's own.
    unsigned targetTop=27,targetStride=10,targetHeight=10;
    unsigned key(std::string_view set,unsigned code) const {
        if(const auto it=sets.find(set);it!=sets.end())
            for(const auto &[from,to]:it->second.aliases)if(from==code)return to;
        return code;
    }
    unsigned click(std::string_view set,unsigned key) const {
        if(const auto it=sets.find(set);it!=sets.end())
            for(const auto &[from,to]:it->second.clicks)if(from==key)return to;
        return key;
    }
};
// The Russian read routine (RU 0x43f2a..0x43fd7) tests Shift first: Shift
// uppercases cp866 letters (0xa0-0xaf minus 0x20, 0xe0-0xef minus 0x50) and
// keeps every other code, digits included. Without Shift, Ctrl gives the
// CD's shifted symbols of the same physical key (the CD's Shift list).
inline unsigned xeenRussianShift(unsigned code) {
    if(code>=0xa0 && code<=0xaf)return code-0x20;
    if(code>=0xe0 && code<=0xef)return code-0x50;
    return code;
}
}
#endif
