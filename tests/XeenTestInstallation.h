#ifndef MMODERN_TEST_XEEN_INSTALLATION_H
#define MMODERN_TEST_XEEN_INSTALLATION_H
#include "games/xeen/XeenInstallationDetector.h"
#include <algorithm>
#ifndef MMODERN_TEST_UI_DATA
#define MMODERN_TEST_UI_DATA ""
#endif
namespace mmodern {
// Build configuration selects only test input; the application uses --ui-data.
inline XeenInstallationDetector xeenTestInstallationDetector() {
 const std::filesystem::path ui(MMODERN_TEST_UI_DATA);
 return XeenInstallationDetector(ui);
}
inline std::vector<std::filesystem::path> xeenTestCues(const std::filesystem::path &root) {
 std::vector<std::filesystem::path> result;
 for(const auto &entry:std::filesystem::directory_iterator(root)) {
  auto extension=entry.path().extension().u8string();
  for(auto &c:extension)if(c>='A' && c<='Z')c+=32;
  if(entry.is_regular_file() && extension==".ins")result.push_back(entry.path());
 }
 std::sort(result.begin(),result.end());return result;
}
}
#endif
