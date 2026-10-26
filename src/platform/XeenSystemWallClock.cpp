#include "platform/XeenSystemWallClock.h"
#include <windows.h>
namespace mmodern {
XeenWallClock xeenSystemWallClock() {
 return XeenWallClock::withHundredths([] {
  SYSTEMTIME t;GetLocalTime(&t);
  try {return XeenWallClock::Sample::fromCivil({t.wYear,t.wMonth,t.wDay,t.wHour,t.wMinute,t.wSecond},unsigned(t.wMilliseconds/10));}
  catch(const std::exception &e){throw std::runtime_error(std::string("Host wall clock error: ")+e.what());}
 });
}
}
