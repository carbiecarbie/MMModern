// Opt-in original topology with synthetic sign text and fault providers.
// These representation controls are separate from the live-actor CLI routes.
#include "XeenCombatGameplayTestSupport.h"
#include "games/xeen/XeenRegionalRules.h"
#include "games/xeen/XeenStateEquality.h"
#include <iostream>
using namespace combat_gameplay_test;
namespace {
XeenGameplayServices regional(Harness &h) {
 auto s=h.services();
 s.resources.regionalManifest=[&](const auto &m,const auto &o,const auto &e,const auto &mon){
  xeenValidateRegionalManifest(m,o,e,mon,h.assets->readInitialResource("maze0023.dat"),h.assets->readInitialResource("maze0023.mob"),h.assets->readInitialResource("maze0023.evt"));
 };
 s.texts=[](auto id){XeenEventTextFile t{id,"synthetic-sign.txt",true,std::vector<std::string>(17)};t.strings[16]="Synthetic regional sign";return t;};
 s.composeEncounter=[](auto &,const auto &,const auto &,auto,auto){XeenEventFlow::Composition c;c.frame.width=320;c.frame.height=200;c.frame.pixels.resize(64000);return c;};
 return s;
}
auto bytes(Harness &h){return XeenSaveFormat::encode(XeenSaveState::capture(h.signature,*h.party,*h.camera,*h.flags,*h.world));}
}
int main(int argc,char **argv) {
 try {
  check(argc==3,"usage: regional_event_original <installation> <temporary-save>");
  const std::filesystem::path game=argv[1],path=argv[2];
  XeenSaveSnapshot source;
  {Harness h(game);auto s=regional(h);s.show=[&](const auto &,const auto &handler,const auto &,const auto &,const auto &){handler.framePresented(h.flow->frame().presentation());source=XeenSaveState::capture(h.signature,*h.party,*h.camera,*h.flags,*h.world);return true;};
   check(Application().playGameplay(s,{},path,false,XeenEncounterEntry::Journey,1,3)==0,"Regional source preparation");}
  // Representation-only persistent activation permits each restored quiet view.
  // No actor is removed or relocated, and this is not a navigation witness.
  for(auto &a:source.journey->actors)a.activated=true;
  for(auto cell:{std::pair<int,int>{0,1},{4,5},{5,9},{5,13},{7,7},{8,2},{8,10},{9,11},{10,13},{12,12}})for(unsigned d=0;d<4;++d) {
   auto saved=source;saved.camera={23,cell.first,cell.second,static_cast<XeenDirection>(d)};XeenSaveFile::write(path,saved);
   Harness h(game);auto s=regional(h);unsigned noEvent=0,completed=0;
   s.configureFlow=[&](auto &f,const auto &){h.flow=&f;f.reportManual=[&](const auto &r){noEvent+=std::holds_alternative<XeenManualEventNoEvent>(r);completed+=std::holds_alternative<XeenManualEventCompleted>(r);};};
   bool shown=false;
   s.show=[&](const auto &,const auto &handler,const auto &,const auto &idle,const auto &){
    shown=true;handler.framePresented(h.flow->frame().presentation());const auto before=bytes(h);
    const bool sign=cell.first==5 && cell.second==9 && d==0;
    const auto expected=xeenRegionalEvent(h.eventLoader->load(23),*h.camera);
    handler.beginCycle(++h.cycle);const auto input=*handler.displayedInput();handler.withDisplayedInput(InteractionAction{},input);
    check(!h.flow->canSave(),"Manual interaction must present its result before capture");
    for(const PlayerAction &a:std::vector<PlayerAction>{NavigationAction::MoveForward,WaitAction{},SaveGameAction{},InspectInventoryAction{}})handler.withDisplayedInput(a,input);
    h.world->discardMapCache();h.eventSystem->discardScriptCache();h.eventSystem->discardTextCache();
    h.flow->refresh(true);handler.framePresented(h.flow->frame().presentation());
    for(unsigned n=0;!h.flow->canSave() && n<100;++n){h.now+=100;handler.beginCycle(++h.cycle);idle();handler.framePresented(h.flow->frame().presentation());}
    check(h.flow->canSave() && bytes(h)==before,"Manual address/facing, stale input and reconstruction preserve complete state");
    check(sign ? completed==1 : expected ? completed==0 && noEvent==0 : noEvent==1,"Original manual admission result");
    if(expected && !sign)check(h.flow->encounter()->notice().find("Unsupported event "+std::to_string(*expected))!=std::string::npos,"Unsupported original record notice");
    rejects([&]{h.flow->acceptAutomatic(XeenAutomaticEventCompleted{});});
    return true;
   };
   check(Application().playGameplay(s,{},path,true)==0 && shown,"Regional manual event case");
  }
  for(unsigned fault=0;fault<5;++fault) {
   auto saved=source;saved.camera={23,5,9,XeenDirection::North};if(fault==0 || fault==1)saved.disabledEvents={{23,56}};
   XeenSaveFile::write(path,saved);Harness h(game);auto s=regional(h);bool changed=false,shown=false;
   const auto load=s.resources.loadEvents;
   s.resources.loadEvents=[&](auto id){auto f=load(id);if(changed)f.records[56].parameters={17};return f;};
   s.show=[&](const auto &,const auto &handler,const auto &,const auto &,const auto &){
    shown=true;handler.framePresented(h.flow->frame().presentation());const auto before=bytes(h);
    if(fault<=1) {
     handler.beginCycle(++h.cycle);handler.withDisplayedInput(InteractionAction{},*handler.displayedInput());handler.framePresented(h.flow->frame().presentation());
     check(h.flow->canSave() && bytes(h)==before,"Effective None sign preserves independent overlay");
    }
    if(fault==0)return true;
    if(fault==1 || fault==2){changed=true;h.eventSystem->discardScriptCache();}
    if(fault==3)const_cast<XeenMap &>(h.world->map(23)).geometry.runX=11;
    if(fault==4)const_cast<XeenActor &>(h.world->sessionState().actors()[0]).statistics->raw[20]^=1;
    bool failed=false;try{h.flow->refresh(true);handler.framePresented(h.flow->frame().presentation());}catch(const std::exception &){failed=true;}
    check(failed && !h.flow->canSave(),"Changed warm/reloaded resources fail, including disabled sign");return true;
   };
   check(Application().playGameplay(s,{},path,true)==0 && shown,"Regional event resource failure case");
  }
  std::cout<<"Regional original event addresses: 40 facing cases, disabled sign and four resource-failure controls passed\n";return 0;
 }catch(const std::exception &e){std::cerr<<e.what()<<'\n';return 1;}
}
