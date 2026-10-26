#include "platform/XeenSystemWallClock.h"
#include <windows.h>
namespace mmodern {
XeenWallClock xeenSystemWallClock() {
 return XeenWallClock([] {
  SYSTEMTIME t;GetLocalTime(&t);
  try {return xeenDosTime({t.wYear,t.wMonth,t.wDay,t.wHour,t.wMinute,t.wSecond});}
  catch(const std::exception &e){throw std::runtime_error(std::string("Host wall clock error: ")+e.what());}
 });
}
}
