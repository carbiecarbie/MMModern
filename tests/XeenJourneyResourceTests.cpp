#include "XeenJourneyTestSupport.h"
#include "XeenRegionalTestSupport.h"
#include "XeenJourneyResourceTestSupport.h"
#include <iostream>
using namespace combat_test;
int main(){try {
 const auto bytes=chr();const auto mon=regional_test::statistics();const auto evt=regional_test::events(23);
 auto r=regional_test::resources();
 XeenJourneySetup setup{bytes,XeenGameplayContextFormat::parse(pty()),mon,evt,56,14,r.regionalManifest};
 setup.purse=XeenMonsterTreasure{};setup.regionalRecovery=XeenRegionalRecoveryState{};
 setup.regionalText=regional_test::texts(23);setup.learnedNames=XeenLearnedSpellNames{};
 setup.learnedNamesProvider=r.loadLearnedSpellNames;setup.vertigoManifest=r.vertigoManifest;
 setup.bank=XeenBankBalances{};setup.cityEventsProvider=[]{return regional_test::events(28);};
 journey_resources_test::run([&]{return XeenPartyLoader().loadFromResources(bytes,pty());},setup,
  regional_test::map,regional_test::objects,regional_test::signature(),r);
 std::cout<<"312 fresh/restored regional resource renewal cases passed\n";return 0;
}catch(const std::exception &e){std::cerr<<e.what()<<'\n';return 1;}}
