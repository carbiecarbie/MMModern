#include "games/xeen/XeenProgramState.h"
#include <iostream>
#include <type_traits>
#include <filesystem>
#include <fstream>
using namespace mmodern;
namespace {
void check(bool v,const char *message) {if(!v)throw std::runtime_error(message);}
void rules() {
 static_assert(!std::is_default_constructible_v<XeenWallClock>);
 static_assert(!std::is_default_constructible_v<XeenProgramState>);
 check(xeenDosTime({1980,1,1,0,0,0})==315550800u,"DOS date range starts in 1980 with EST offset");
 check(xeenDosTime({2099,1,1,0,0,0})==4070926800u,"DOS 2099 date");
 check(xeenDosTime({2100,1,1,0,0,0})==4102462800u,"DOS 2100 date");
 check(xeenDosTime({2107,1,1,0,0,0})==28420304u,"DOS 2107 date and uint32 wrap");
 check(xeenDosTime({2100,2,29,0,0,0})-xeenDosTime({2100,2,28,0,0,0})==86400 &&
  xeenDosTime({2100,3,1,0,0,0})-xeenDosTime({2100,2,29,0,0,0})==86400,"Borland year & 3 treats 2100 as leap");
 xeenDosTime({2107,12,31,23,59,59});
 for(int year:{1979,2108}) {
  bool refused=false;
  try {xeenDosTime({year,1,1,0,0,0});}
  catch(const std::invalid_argument &e){refused=std::string(e.what()).find("1980 through 2107")!=std::string::npos;}
  check(refused,"out-of-range DOS date must explain its supported range");
 }
 check(xeenDosTime({2000,1,1,0,0,0})==946702800,"leap-century epoch");
 check(xeenDosTime({2026,7,1,0,0,0})==1782878400,"DOS EDT epoch offset");
 check(!xeenDosDaylightTime({1986,4,6,12,0,0}),"pre-1987 April rule");
 check(!xeenDosDaylightTime({1986,4,27,1,59,59}) && xeenDosDaylightTime({1986,4,27,2,0,0}),"last April Sunday boundary");
 check(!xeenDosDaylightTime({1987,4,5,1,59,59}) && xeenDosDaylightTime({1987,4,5,2,0,0}),"first April Sunday boundary");
 check(!xeenDosDaylightTime({2026,3,29,12,0,0}),"modern March DST must not leak in");
 check(xeenDosDaylightTime({2026,10,25,1,59,59}) && !xeenDosDaylightTime({2026,10,25,2,0,0}),"last October Sunday boundary");
 check(!xeenDosDaylightTime({2026,11,1,0,0,0}),"modern November DST must not leak in");
 const auto before=xeenDosTime({2026,4,5,1,59,59}),after=xeenDosTime({2026,4,5,2,0,0});
 check(after-before==std::uint32_t(-3599),"civil DST transition affects unsigned seconds");
 for(int month=1;month<=12;++month)for(int hour:{0,5,12,23}) {
  XeenCivilTime t{2026,month,15,hour,42,37};const auto back=xeenDosLocalTime(xeenDosTime(t));
  check(back.year==t.year && back.month==t.month && back.day==t.day && back.hour==t.hour &&
   back.minute==t.minute && back.second==t.second,"DOS civil inverse");
 }
 check(xeenPanelCivilTime(xeenDosTime({2026,1,1,0,1,2}))=="12:01:02a","midnight");
 check(xeenPanelCivilTime(xeenDosTime({2026,7,1,12,1,2}))=="12:01:02p","noon");
 check(xeenPanelCivilTime(xeenDosTime({2026,7,1,9,1,2}))==" 9:01:02a","leading blank and lower-case morning");
 check(xeenPanelCivilTime(0)=="12:00:00a" && xeenPanelCivilTime(18000)=="12:00:00a","DOS signed-negative local calendar clamp");
 check(xeenPanelCivilTime(18000u+0x80000000u)=="12:00:00a","DOS high-bit local calendar clamp");
 check(xeenPanelPlayTime(62545)=="17:22:25","calibrated elapsed seconds");
 check(xeenPanelPlayTime(68400)=="19:1092:16","DOS low-word product");
 check(xeenPanelPlayTime(0xfffffff9u)=="13398:16412:09","DOS minutes local also narrows to 16 bits after rollback");
 check(xeenPanelPlayTime(0)=="00:00:00","minimum field widths");
 std::uint32_t seconds=100;XeenProgramState p{XeenWallClock([&]{return seconds;})};
 seconds=90;check(p.playTime(3)==0xfffffff9u && p.playTime(20)==10,"rollback without clamp");
 seconds=110;check(p.playTime(0xfffffffeu)==8,"uint32 overflow");
 p.resetPlayTime();check(p.playTime(77)==77,"reset process mark");
}
void bindings() {
 // Architecture guard: real local civil time has exactly one production binding.
 unsigned calls=0;
 for(const auto *root:{"src","tools"})for(const auto &entry:std::filesystem::recursive_directory_iterator(std::filesystem::path(MMODERN_SOURCE_ROOT)/root)) {
  const auto path=entry.path();if(path.extension()!=".cpp" && path.extension()!=".h")continue;
  std::ifstream in(path);const std::string source{std::istreambuf_iterator<char>(in),{}};
  const auto relative=std::filesystem::relative(path,MMODERN_SOURCE_ROOT).generic_string();
  if(source.find("xeenSystemWallClock(")!=std::string::npos) {
   check(relative=="src/main.cpp" || relative=="src/platform/XeenSystemWallClock.cpp" || relative=="src/platform/XeenSystemWallClock.h","system clock bound outside player startup");
   ++calls;
  }
  if(source.find("GetLocalTime(")!=std::string::npos)check(relative=="src/platform/XeenSystemWallClock.cpp","host civil reader escaped platform owner");
 }
 check(calls==3,"system clock binding inventory changed");
}
}
int main() {try {rules();bindings();std::cout<<"DOS wall clock, DST, unsigned play time, panel formats and bindings passed\n";return 0;}
 catch(const std::exception &e){std::cerr<<e.what()<<'\n';return 1;}}
