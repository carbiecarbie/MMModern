#include "platform/XeenSystemWallClock.h"
#ifdef _WIN32
#include <windows.h>
#else
#include <chrono>
#include <ctime>
#endif
namespace mmodern {
XeenWallClock xeenSystemWallClock() {
 return XeenWallClock::withHundredths([] {
#ifdef _WIN32
  SYSTEMTIME t;GetLocalTime(&t);
  try {return XeenWallClock::Sample::fromCivil({t.wYear,t.wMonth,t.wDay,t.wHour,t.wMinute,t.wSecond},unsigned(t.wMilliseconds/10));}
#else
  const auto now=std::chrono::system_clock::now();const auto seconds=std::chrono::system_clock::to_time_t(now);
  std::tm t{};if(!localtime_r(&seconds,&t))throw std::runtime_error("Host wall clock error");
  const auto hundredths=unsigned(std::chrono::duration_cast<std::chrono::milliseconds>(now.time_since_epoch()).count()%1000/10);
  try{return XeenWallClock::Sample::fromCivil({t.tm_year+1900,t.tm_mon+1,t.tm_mday,t.tm_hour,t.tm_min,t.tm_sec},hundredths);}
#endif
  catch(const std::exception &e){throw std::runtime_error(std::string("Host wall clock error: ")+e.what());}
 });
}
}
