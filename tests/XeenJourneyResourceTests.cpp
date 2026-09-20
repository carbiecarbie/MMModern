#include "XeenJourneyTestSupport.h"
#include "XeenSaveTestSupport.h"
#include "XeenJourneyResourceTestSupport.h"
#include <iostream>
using namespace combat_test;
int main(){try {
 const auto bytes=chr();const auto mon=statistics();const auto evt=events();
 journey_resources_test::run([&]{return XeenPartyLoader().loadFromResources(bytes,pty());},
  XeenJourneySetup{bytes,XeenGameplayContextFormat::parse(pty()),mon,evt,56},
  [](auto id){auto m=map();m.geometry.id=id.number;return m;},
  [](auto id){auto o=objects();o.mapId=id;return o;},save_test::sample().resources);
 std::cout<<"312 fresh/restored Journey resource renewal cases passed\n";return 0;
}catch(const std::exception &e){std::cerr<<e.what()<<'\n';return 1;}}
