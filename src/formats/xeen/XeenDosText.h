#ifndef MMODERN_XEEN_DOS_TEXT_H
#define MMODERN_XEEN_DOS_TEXT_H

#include <cstddef>
#include <array>
#include <cstdint>
#include <memory>
#include <string>
#include <string_view>
#include <unordered_map>
#include <utility>
#include <vector>

#include "core/XeenKeyboard.h"
#include "formats/xeen/XeenLanguage.h"

namespace mmodern {
// One field of the Russian program layout (M110 Part B): `count` [begin,end)
// string extents starting at `first` in the entry list, optionally named by a
// far-pointer table at file offset `table`.
struct XeenDosRussianField {
 const char *name;
 std::size_t table;
 const char *arguments;
 std::size_t first, count;
};

// Named fields in the installed, uncompressed English DOS load image. Offsets
// describe its layout, not a GOG installer or a whole-file identity checksum.
// With the Russian translation, the same names are read from the identified
// Russian Darkside program; CD-only fields keep the CD value.
class XeenDosText {
public:
 struct Field {
  const char *name;
  std::size_t begin, end, count;
  bool pointers;
  const char *arguments;
 };
 static const std::vector<Field> &layout();
 // `slot` is the index of the CD button this one stands for: the Russian
 // control panel has no Text and Speech buttons.
 struct Button {unsigned x,y,width,height,key;bool painted;unsigned slot=0;};
 struct ButtonLayout {const char *name;std::size_t x,y,width,height,key,painted,count;};
 static const std::vector<ButtonLayout> &buttonLayouts();
 // Other Options builds its keys with MOV byte [BP+offset], immediate:
 // D and C, then the E, V and W ending rows.
 static constexpr std::array<std::size_t,2> kOtherButtonKeyOffsets{0xc2d1,0xc2ea};
 static constexpr std::array<std::size_t,3> kEndingButtonKeyOffsets{0xc311,0xc340,0xc36e};
 // Characters the program passes as MOV AX,immediate (M110 decision 9).
 struct Characters {const char *name;std::array<std::size_t,2> at;};
 static const std::vector<Characters> &characterLayout(XeenLanguage language=XeenLanguage::English);
 const std::vector<Button> &buttons(std::string_view name) const;
 static const std::vector<std::pair<std::size_t,std::size_t>> &targets(std::string_view name);
 explicit XeenDosText(const std::vector<std::uint8_t> &bytes);
 // Russian program text over the CD program: every field of the Russian
 // layout replaces the CD value; the CD-only fields keep it.
 XeenDosText(const std::vector<std::uint8_t> &cd,const std::vector<std::uint8_t> &russian);
 XeenLanguage language() const {return _language;}
 std::string_view scalar(std::string_view name) const;
 const std::vector<std::string> &table(std::string_view name) const;
 // The argument signature the field was validated with.
 std::string_view arguments(std::string_view name) const;
 // Two characters, in the order of characterLayout().
 const std::string &characters(std::string_view name) const;
 // The program's window table (four parallel arrays of 48 words): the CD's
 // at file 0x3dbfe, the Russian program's at 0x3c6fa.
 struct Window {unsigned x,y,width,height;};
 const Window &window(unsigned index) const;
 static const std::vector<XeenDosRussianField> &russianLayout();
 static const std::vector<std::pair<std::size_t,std::size_t>> &russianEntries();
 // English fields the Russian program does not have (CD-only text).
 static const std::vector<std::string_view> &russianAbsent();
 static const std::vector<ButtonLayout> &russianButtonLayouts();
 // Text controls in either language: Russian keeps bytes 0x80-0xff as
 // characters instead of masking them.
 static void validateControls(std::string_view text, XeenLanguage language = XeenLanguage::English);
 // The printf conversions of a text, as layout signatures name them ("s,lu").
 static std::string formatSignature(std::string_view text);
 // M110 Part C: the Russian program's keyboard and key sets; null in English.
 const std::shared_ptr<const XeenKeyboard> &keyboard() const {return _keyboard;}
 // The combat target rows and highlight (CD 0x2e777: row*10+27, height 10).
 struct TargetRows {unsigned top=27,stride=10,height=10;};
 TargetRows combatTargets() const {
  return _keyboard?TargetRows{_keyboard->targetTop,_keyboard->targetStride,_keyboard->targetHeight}:TargetRows{};
 }
private:
 XeenLanguage _language=XeenLanguage::English;
 std::unordered_map<std::string,std::vector<std::string>> _fields;
 std::unordered_map<std::string,std::string> _arguments,_characters;
 std::unordered_map<std::string,std::vector<Button>> _buttons;
 std::vector<Window> _windows;
 std::shared_ptr<const XeenKeyboard> _keyboard;
};
}
#endif
