#ifndef MMODERN_REGIONAL_GAMEPLAY_TEST_SUPPORT_H
#define MMODERN_REGIONAL_GAMEPLAY_TEST_SUPPORT_H
#include "XeenRegionalCombatTestSupport.h"
#include "XeenSaveGameplayTestSupport.h"
#include "XeenChildProcessTestSupport.h"
namespace regional_gameplay_test {
using namespace combat_test;
struct Harness {
 XeenFontFormat font{gameplay_test::fontBytes()};
 XeenEventFlow *flow=nullptr;
 XeenCombatBoundary *combatBoundary=nullptr;
 const XeenPartyState *party=nullptr;
 XeenWorld *world=nullptr; XeenCamera *camera=nullptr; const XeenGameFlags *flags=nullptr;
 XeenEventSystem *eventSystem=nullptr;
 std::uint64_t now=0,cycle=0,observedOrdinary=0;
 std::filesystem::path savePath;
 unsigned saves=0,compositions=0,seed=1;
 IndexedFrame base;
 XeenMonsterAppearance observedAppearance;
 std::optional<XeenCombatRandom> random;
 XeenGameplayServices services(unsigned value=1) {
  seed=value;
  XeenGameplayServices s{regional_test::resources(),[]{return XeenGameFlags{};},regional_test::map,regional_test::objects,
   regional_test::texts,font,
   [](auto &,const auto &,const auto &,auto){return XeenEventFlow::Composition{IndexedFrame{320,200,Bytes(64000)},true};}, {},
   [&](auto &f,const auto &){flow=&f;combatBoundary=&const_cast<XeenEncounterFlow *>(f.encounter())->boundary();}, {},
   [&](auto &w,auto &e,const auto &p,auto &c,const auto &f){world=&w;eventSystem=&e;party=&p;camera=&c;flags=&f;}};
  s.clock=[&]{return now;};s.validateEncounterSprite=[](auto){};s.validateCombatSprite=[](auto){};
  s.composeEncounter=[&](auto &,const auto &,const auto &,auto ordinary,auto actor){
   ++compositions;observedOrdinary=ordinary;observedAppearance=actor;
   base=IndexedFrame{320,200,Bytes(64000)};return XeenEventFlow::Composition{base,true};};
  s.observeSaveStage=[&](auto){++saves;};return s;
 }
 int run(const XeenGameplayServices &s){
  auto saved=regional_combat_test::combatSnapshot(XeenCombatRandom(seed));
  const auto path=savePath=child_test::freshDirectory(std::filesystem::current_path()/"regional-gameplay")/"synthetic.mmsave";
  XeenSaveFile::write(path,saved);
  struct Tape {XeenCombatRandom *old=regional_combat_test::tape;~Tape(){regional_combat_test::tape=old;}} scope;
  regional_combat_test::tape=random?&*random:nullptr;
  return Application().playGameplay(s,{},path,true);
 }
 bool retired() const {return flow&&!flow->encounter()->combat();}
 Phase phase() const {return retired()?Phase::Victory:fight().phase();}
 XeenCombatResult result() const {return flow->encounter()->combatResult();}
 std::size_t randomPosition() const {return retired()?std::uint64_t(world->sessionState().journeyRandom()->count):fight().random().position();}
 const XeenCombat &fight() const {check(!retired(),"Retired combat pointer access");return *flow->encounter()->combat();}
 XeenCombat &fight(){check(!retired(),"Retired combat pointer access");return *const_cast<XeenCombat *>(flow->encounter()->combat());}
 void present(const SdlWindow::FrameUpdateHandler &h){if(h.framePresented)h.framePresented(flow->frame().presentation());}
 void press(const SdlWindow::FrameUpdateHandler &h,const PlayerAction &a){
  present(h);h.beginCycle(++cycle);check(h.displayedInput().has_value(),"Displayed ticket exists");
  h.withDisplayedInput(a,*h.displayedInput());check(h.frameCurrent(),"Current returned frame");present(h);
 }
 void tick(const SdlWindow::FrameUpdateHandler &h,const SdlWindow::IdleFrameHandler &idle){
  present(h);now+=100;h.beginCycle(++cycle);idle();check(h.frameCurrent(),"Current idle frame");present(h);
 }
};
}
#endif
