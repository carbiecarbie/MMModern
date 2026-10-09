#ifndef MMODERN_XEEN_DOS_TEXT_H
#define MMODERN_XEEN_DOS_TEXT_H

#include <cstddef>
#include <cstdint>
#include <string>
#include <string_view>
#include <unordered_map>
#include <utility>
#include <vector>

namespace mmodern {
// Named fields in the installed, uncompressed English DOS load image. Offsets
// describe its layout, not a GOG installer or a whole-file identity checksum.
class XeenDosText {
public:
 struct Field {
  const char *name;
  std::size_t begin, end, count;
  bool pointers;
  const char *arguments;
 };
 static const std::vector<Field> &layout();
 static const std::vector<std::pair<std::size_t,std::size_t>> &targets(std::string_view name);
 explicit XeenDosText(const std::vector<std::uint8_t> &bytes);
 std::string_view scalar(std::string_view name) const;
 const std::vector<std::string> &table(std::string_view name) const;
 static void validateControls(std::string_view text);
private:
 std::unordered_map<std::string,std::vector<std::string>> _fields;
};
}
#endif
