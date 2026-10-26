#include "games/xeen/XeenWallClock.h"
#include <cstdio>
namespace mmodern {
namespace {
bool leap(int year) {return (year&3)==0;}
int monthDays(int year,int month) {
 static constexpr int days[]={31,28,31,30,31,30,31,31,30,31,30,31};
 return days[month-1]+(month==2 && leap(year));
}
std::int64_t daysBefore(int year,int month,int day) {
 std::int64_t days=0;
 for(int y=1970;y<year;++y)days+=365+leap(y);
 for(int y=year;y<1970;++y)days-=365+leap(y);
 for(int m=1;m<month;++m)days+=monthDays(year,m);
 return days+day-1;
}
int sunday(int year,int month,bool first) {
 const int day=first?1:monthDays(year,month);
 const int weekday=int((daysBefore(year,month,day)+4)%7+7)%7;
 return first?day+(7-weekday)%7:day-weekday;
}
XeenCivilTime civil(std::int64_t seconds) {
 auto days=seconds/86400;int rest=int(seconds%86400);
 if(rest<0){rest+=86400;--days;}
 int year=1970;
 while(days<0){--year;days+=365+leap(year);}
 while(days>=365+leap(year)){days-=365+leap(year);++year;}
 int month=1;
 while(days>=monthDays(year,month)){days-=monthDays(year,month);++month;}
 return {year,month,int(days)+1,rest/3600,rest/60%60,rest%60};
}
}
bool xeenDosDaylightTime(XeenCivilTime t) {
 // DAT 0xa9c8..0xaaa2: pre-1987 last April Sunday; later first Sunday.
 if(t.month<4 || t.month>10)return false;
 if(t.month>4 && t.month<10)return true;
 const int boundary=sunday(t.year,t.month,t.month==4 && t.year>=1987);
 if(t.month==4)return t.day>boundary || (t.day==boundary && t.hour>=2);
 return t.day<boundary || (t.day==boundary && t.hour<=1);
}
std::uint32_t xeenDosTime(XeenCivilTime t) {
 if(t.year<1980 || t.year>2107)throw std::invalid_argument("DOS wall clock requires a civil date from 1980 through 2107");
 if(t.month<1 || t.month>12 || t.day<1 ||
    t.day>monthDays(t.year,t.month) || t.hour<0 || t.hour>23 || t.minute<0 ||
    t.minute>59 || t.second<0 || t.second>59)throw std::invalid_argument("Invalid DOS civil time");
 return static_cast<std::uint32_t>(daysBefore(t.year,t.month,t.day)*86400+
  t.hour*3600+t.minute*60+t.second+18000-(xeenDosDaylightTime(t)?3600:0));
}
XeenCivilTime xeenDosLocalTime(std::uint32_t seconds) {
 // DAT 0x08872..0x08885: subtraction is a dword, then the calendar
 // routine clamps a signed-negative adjusted value to the epoch.
 const std::uint32_t adjusted=seconds-18000u;
 const std::int64_t standard=(adjusted&0x80000000u)?0:adjusted;
 auto t=civil(standard);
 return xeenDosDaylightTime(t)?civil(standard+3600):t;
}
std::string xeenPanelCivilTime(std::uint32_t seconds) {
 const auto t=xeenDosLocalTime(seconds);char text[32];
 std::snprintf(text,sizeof(text),"%2d:%02d:%02d%c",t.hour%12?t.hour%12:12,t.minute,t.second,t.hour>=12?'p':'a');
 return text;
}
std::string xeenPanelPlayTime(std::uint32_t seconds) {
 // DAT 0xf286..0xf2e8: only the low word of hours*3600 is subtracted.
 const auto hours=static_cast<std::uint16_t>(seconds/3600);
 const auto rest=seconds-static_cast<std::uint16_t>(hours*3600u);char text[48];
 std::snprintf(text,sizeof(text),"%02u:%02u:%02u",unsigned(hours),unsigned(static_cast<std::uint16_t>(rest/60)),unsigned(rest%60));
 return text;
}
}
