#ifndef MMODERN_REGIONAL_SAVE_GAMEPLAY_TEST_SUPPORT_H
#define MMODERN_REGIONAL_SAVE_GAMEPLAY_TEST_SUPPORT_H
#include "XeenSaveGameplayTestSupport.h"
#include "XeenRegionalTestSupport.h"
namespace regional_save_test {
using namespace gameplay_test;
struct Fixture {
 XeenFontFormat font{fontBytes()};
 XeenItemCatalog catalog;
 XeenEventFlow *flow=nullptr; XeenWorld *world=nullptr; XeenEventSystem *events=nullptr;
 const XeenPartyState *party=nullptr; XeenCamera *camera=nullptr; const XeenGameFlags *flags=nullptr;
 unsigned compositions=0; std::uint64_t cycle=0,now=0;
 std::vector<std::uint64_t> phases;
 XeenSaveSnapshot saved=regional_test::snapshot();
 XeenGameplayServices services(){
  XeenGameplayServices s{regional_test::resources(),[]{return XeenGameFlags{};},regional_test::map,regional_test::objects,regional_test::texts,font,
   [](auto &,const auto &,const auto &,auto){return XeenEventFlow::Composition{IndexedFrame{320,200,Bytes(64000)},true};}, {},
   [&](auto &f,const auto &){flow=&f;}, {},
   [&](auto &w,auto &e,const auto &p,auto &c,const auto &g){events=&e;world=&w;party=&p;camera=&c;flags=&g;}};
  s.composeEncounter=[&](auto &w,const auto &p,const auto &c,auto phase,auto){
   ++compositions;phases.push_back(phase);IndexedFrame f{320,200,Bytes(64000)};
   f.pixels[0]=c.mapId.number;f.pixels[1]=c.x;f.pixels[2]=c.y;f.pixels[3]=static_cast<unsigned>(c.direction);
   f.pixels[4]=p.questItems.at(17);f.pixels[6]=p.roster.at(0).currentHp;
   f.pixels[10]=w.isObjectDisabled({23,0});f.pixels[20]=phase%251;
   return XeenEventFlow::Composition{f,true};};
  s.clock=[&]{return now;};s.catalog=&catalog;s.validateEncounterSprite=[](auto){};s.validateCombatSprite=[](auto){};return s;
 }
 void present(const SdlWindow::FrameUpdateHandler &h){h.framePresented(flow->frame().presentation());}
 void send(const SdlWindow::FrameUpdateHandler &h,const PlayerAction &a){present(h);h.beginCycle(++cycle);const auto token=h.displayedInput();check(token.has_value(),"regional displayed input");h.withDisplayedInput(a,*token);present(h);}
 XeenSaveSnapshot capture(){return XeenSaveState::capture(regional_test::signature(),*party,*camera,*flags,*world);}
};
inline void phirna(XeenGameplayServices &s){
 s.objects=[](auto id){auto o=regional_test::objects(id);o.entities.objects.clear();for(unsigned i=0;i<13;++i)o.entities.objects.push_back({1,1,0,0,7});o.entities.objects.push_back({8,2,0,0,26});return o;};
 s.resources.loadEvents=[](auto id){auto e=regional_test::events(id);if(id==XeenMapIdentity(23)){
  const unsigned sites[]{125,126,130,131,132,133,127,128,129,134,135};
  const unsigned op[]{0x20,0x29,0x09,0x0c,0x0e,0x12,0x12,0x12,0x12,0x12,0x12};
  const std::vector<Bytes> args{{0,3},{0},{0x2c,1,3},{0,0,0x15,0x63},{},{},{},{},{},{},{}};
  for(unsigned i=0;i<11;++i){auto &r=e.records[sites[i]];r.x=8;r.y=2;r.direction=4;r.line=i;r.opcode=op[i];r.parameters=args[i];r.lengthField=5+r.parameters.size();}
 }return e;};
 s.npcDraw=[](auto &,auto,auto){};
}

}
#endif
