#ifndef MMODERN_COMBAT_GAMEPLAY_TEST_SUPPORT_H
#define MMODERN_COMBAT_GAMEPLAY_TEST_SUPPORT_H
#include "XeenCombatTestSupport.h"
#include "XeenSaveGameplayTestSupport.h"
#include "XeenChildProcessTestSupport.h"
#include "formats/xeen/XeenAssetSource.h"
#include "games/xeen/XeenInstallationDetector.h"
#include "games/xeen/XeenMapLoader.h"
#include "games/xeen/XeenEventLoader.h"
#include "games/xeen/XeenEventTextLoader.h"
#include "games/xeen/XeenGameFlagsLoader.h"
#include "games/xeen/CloudsMapComposer.h"
#include "games/xeen/CloudsUiComposer.h"
namespace combat_gameplay_test {
using namespace combat_test;
struct Harness {
 XeenFontFormat font{gameplay_test::fontBytes()};
 std::unique_ptr<XeenAssetSource> assets;
 std::unique_ptr<XeenEventLoader> eventLoader;
 XeenMapLoader mapLoader;
 XeenEventFlow *flow=nullptr;
 XeenCombat *combat=nullptr;
 XeenCombatBoundary *combatBoundary=nullptr;
 const XeenPartyState *party=nullptr;
 XeenWorld *world=nullptr;
 XeenCamera *camera=nullptr;
 const XeenGameFlags *flags=nullptr;
 XeenEventSystem *eventSystem=nullptr;
 XeenSaveResourceSignature signature=save_test::sample().resources;
 unsigned mapCalls=0,mobCalls=0;
 std::uint64_t now=0,cycle=0;
 unsigned saves=0,compositions=0;
 std::size_t retainedRng=0;
 std::uint64_t observedOrdinary=0;
 XeenMonsterAppearance observedAppearance;
 IndexedFrame base;
 std::optional<XeenCombatRandom> random;
 bool badTerrain=false;
 explicit Harness(const std::optional<std::filesystem::path> &game={}) {
  if (!game) return;
  const auto installation=XeenInstallationDetector().detect(*game);
  check(installation&&installation->hasDarkside(),"World of Xeen required");
  signature=XeenSaveFile::fingerprint(*installation);
  assets=std::make_unique<XeenAssetSource>(*installation,320,200);
  font=XeenFontFormat(assets->readArchiveResource("fnt"));
  eventLoader=std::make_unique<XeenEventLoader>([&](const std::string &name)->std::optional<Bytes>{
   if(!assets->hasInitialResource(name))return {};return assets->readInitialResource(name);});
 }
 XeenGameplayServices services(unsigned seed=1) {
  XeenGameplayServices s{
   {signature,[&]{return assets?XeenPartyLoader().loadInitialCloudsParty(*assets):XeenPartyLoader().loadFromResources(chr(),pty());},
    [&](XeenMapIdentity id){return assets?eventLoader->load(id):events();}},
   [&]{return assets?XeenGameFlagsLoader().loadInitialCloudsFlags(*assets):XeenGameFlags{};},
   [&](XeenMapIdentity id){
    ++mapCalls;
    if(assets)return mapLoader.loadGeometryMap(*assets,id);
    auto value=map();if(badTerrain)value.geometry.flags=1;return value;
   },
   [&](XeenMapIdentity id){++mobCalls;return assets?mapLoader.loadObjects(*assets,id):objects();},
   [](XeenMapIdentity){return XeenEventTextFile{};},font,
   [](XeenWorld &,const XeenPartyState &,const XeenCamera &,std::uint64_t)->XeenEventFlow::Composition{
    throw std::runtime_error("Combat routed through ordinary composition");}, {},
   [&](XeenEventFlow &f,const XeenCamera &){flow=&f;}, {},
   [&](XeenWorld &w,XeenEventSystem &e,const XeenPartyState &p,XeenCamera &c,const XeenGameFlags &f){world=&w;party=&p;camera=&c;flags=&f;eventSystem=&e;}
  };
  s.resources.loadInitialCharacters=[&]{return assets?assets->readInitialResource("maze.chr"):chr();};
  s.resources.loadInitialContext=[&]{return XeenGameplayContextFormat::parse(assets?assets->readInitialResource("maze.pty"):pty());};
  s.resources.loadMonsterStatistics=[&]{return assets?XeenMonsterFormat::parse(*assets->readCloudsMonsterStatisticsFromDarkArchive()):statistics();};
  s.clock=[&]{return now;};
  s.prepareCombat=[&,seed](XeenWorld &w,XeenPartyState &p,XeenCamera &c,XeenCombatBoundary &b){
   check(w.sessionState().encounterEntry()==XeenEncounterEntry::Diagnostic27,"typed reservation precedes providers");
   const auto bytes=assets?assets->readInitialResource("maze.chr"):chr();
   const auto context=XeenGameplayContextFormat::parse(assets?assets->readInitialResource("maze.pty"):pty());
   const auto stats=assets?XeenMonsterFormat::parse(*assets->readCloudsMonsterStatisticsFromDarkArchive()):statistics();
   auto value=std::make_unique<XeenCombat>(w,p,c,b,bytes,context,stats,assets?eventLoader->load(20):events(),random.value_or(XeenCombatRandom(seed)));
   combat=value.get();combatBoundary=&b;
   return value;
  };
  s.validateEncounterSprite=[&](std::uint8_t image){if(assets)assets->validateNormalMonster(image);};
  s.validateCombatSprite=[&](std::uint8_t image){if(assets)assets->validateAttackMonster(image);};
  s.composeEncounter=[&](XeenWorld &w,const XeenPartyState &p,const XeenCamera &c,std::uint64_t ordinary,XeenMonsterAppearance actor){
   ++compositions;
   observedOrdinary=ordinary;
   observedAppearance=actor;
   check(actor.valid(),"bounded production appearance");
   check(p.roster.combatMarked(),"composition borrows marked roster");
   XeenEventFlow::Composition out;
   if(assets)out.frame=CloudsMapComposer().compose(*assets,w,p,c,{610},nullptr,ordinary,&out.containsOrdinaryAnimation,actor);
   else {out.frame.width=320;out.frame.height=200;out.frame.pixels.resize(64000);out.containsOrdinaryAnimation=true;}
   base=out.frame;
   return out;
  };
  s.observeSaveStage=[&](auto){++saves;};
  return s;
 }
 Phase phase() const {return flow&&flow->completed()?Phase::Victory:fight().phase();}
 const XeenCombatResult &result() const {return flow?flow->encounter()->combatResult():fight().result();}
 std::size_t randomPosition() const {return flow&&flow->completed()?retainedRng:fight().random().position();}
 const XeenCombat &fight() const {check(!flow||!flow->completed(),"retired combat pointer access");return *combat;}
 XeenCombat &fight() {check(!flow||!flow->completed(),"retired combat pointer access");return *combat;}
 void visibleScene() {
  if(flow->inventoryOpen())return;
  const auto &f=flow->frame();
  for(int y=8;y<135;++y)for(int x=8;x<223;++x)
   check(f.pixels[y*320+x]==base.pixels[y*320+x],"combat panels obscure scene");
 }
 void press(const SdlWindow::FrameUpdateHandler &handler,const PlayerAction &action) {
  if(handler.framePresented)handler.framePresented();
  handler.beginCycle(++cycle);
  check(handler.displayedInput().has_value(),"displayed ticket exists");
  handler.withDisplayedInput(action,*handler.displayedInput());
  check(handler.frameCurrent(),"current returned frame");
  if(handler.framePresented)handler.framePresented();
  visibleScene();
 }
 void tick(const SdlWindow::FrameUpdateHandler &handler,const SdlWindow::IdleFrameHandler &idle) {
  if(!flow->completed())retainedRng=fight().random().position();
  now+=100;handler.beginCycle(++cycle);idle();check(handler.frameCurrent(),"current idle frame");
  if(handler.framePresented)handler.framePresented();
  visibleScene();
 }
};

}
#endif
