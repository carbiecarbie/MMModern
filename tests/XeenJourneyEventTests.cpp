#include "XeenExpeditionTestSupport.h"
#include "games/xeen/XeenStateEquality.h"
#include <iostream>
#include <limits>
using namespace combat_gameplay_test;
namespace fs=std::filesystem;
namespace {
using Handler=SdlWindow::FrameUpdateHandler;
XeenGameplayServices services(Harness &h) {
 auto s=h.services();
 s.maps=[](auto){return expedition_fixture::terrain();};
 s.objects=[](auto){return expedition_fixture::objects();};
 s.resources.loadEvents=[](auto){return expedition_fixture::events();};
 s.resources.loadMonsterStatistics=[]{return expedition_fixture::monsters();};
 s.texts=[](auto id){return XeenEventTextFile{id,"synthetic.txt",true,{"Synthetic discovery","","","Synthetic bones"}};};
 return s;
}
void send(Harness &h,const Handler &handler,const PlayerAction &a,bool present=true) {
 handler.beginCycle(++h.cycle);check(handler.displayedInput().has_value(),"displayed input token");
 handler.withDisplayedInput(a,*handler.displayedInput());
 if(present){check(handler.frameCurrent(),"current response frame");handler.framePresented();}
}
XeenSaveSnapshot snapshot(Harness &h){return XeenSaveState::capture(h.signature,*h.party,*h.camera,*h.flags,*h.world);}
void effects(Harness &h,unsigned count,unsigned objects,unsigned events) {
 check(h.party->questItems.at(18)==count&&h.world->sessionState().disabledObjectCount()==objects&&h.world->sessionState().disabledEventCount()==events,"exact independently published objective effects");
}
void acknowledgment(Harness &h,const Handler &handler){send(h,handler,InteractionAction{});check(h.flow->canCancelInteraction(),"WhoWill reached");send(h,handler,SelectMemberAction{4});check(h.flow->presentationGeneration()&&!h.flow->canCancelInteraction()&&!h.flow->canSave(),"acknowledgment reached without grant");}
XeenSaveSnapshot objective(const fs::path &path) {
 Harness h;auto s=services(h);std::optional<XeenSaveSnapshot> saved;
 s.show=[&](const auto &,const auto &handler,const auto &,const auto &,const auto &){handler.framePresented();saved=snapshot(h);return true;};
 check(Application().playGameplay(s,{},path,false,XeenEncounterEntry::Journey,1,2)==0&&saved.has_value(),"production fresh fixture source");
 saved->camera={20,5,14,XeenDirection::North};
 for(auto &a:saved->journey->actors){a.hp=0;a.x=a.y=-128;a.activated=false;a.lifecycle=XeenActorLifecycle::Defeated;a.accounted=true;}
 return *saved;
}
void run(const fs::path &path,const XeenSaveSnapshot &saved,const std::function<void(Harness &,XeenGameplayServices &)> &configure,int expected=0){
 XeenSaveFile::write(path,saved);Harness h;auto s=services(h);configure(h,s);bool shown=false;const auto show=s.show;s.show=[&](const auto &frame,const auto &handler,const auto &escape,const auto &idle,const auto &status){const bool result=show(frame,handler,escape,idle,status);shown=true;return result;};
 check(Application().playGameplay(s,{},path,true)==expected&&shown,"objective Application test completed after all show assertions");
}
void authority(const fs::path &path,const XeenSaveSnapshot &saved){
 run(path,saved,[&](Harness &h,XeenGameplayServices &s){s.show=[&](const auto &,const auto &handler,const auto &,const auto &,const auto &){
  handler.framePresented();
  send(h,handler,InteractionAction{},false);auto who=*h.flow->presentationGeneration();
  check(!h.flow->respond(who,SelectedCharacter{4}),"direct response refuses unpresented WhoWill");handler.framePresented();
  check(!h.flow->respond(who,XeenPresentationResponse::Acknowledged)&&!h.flow->respond(who+1,SelectedCharacter{4}),"wrong phase and generation refuse selection");
  rejects([&]{h.flow->acceptManual(XeenManualEventCompleted{10,false,false});});
  send(h,handler,SelectMemberAction{99});check(h.flow->canCancelInteraction(),"invalid live index retains WhoWill");effects(h,0,0,0);
  const auto selectingInput=*handler.displayedInput();send(h,handler,SelectMemberAction{4},false);
  auto ack=*h.flow->presentationGeneration();check(!h.flow->respond(ack,XeenPresentationResponse::Acknowledged),"direct acknowledgment requires current presented modal");
  handler.framePresented();check(!h.flow->respond(who,SelectedCharacter{4})&&!h.flow->respond(ack,SelectedCharacter{4}),"stale and wrong phase responses refuse");
  handler.withDisplayedInput(AcknowledgeAction{},selectingInput);effects(h,0,0,0);
  const auto saves=h.saves;for(const PlayerAction &a:std::vector<PlayerAction>{SaveGameAction{},NavigationAction::MoveForward,WaitAction{},InspectInventoryAction{},BlockAction{}})send(h,handler,a);
  check(h.saves==saves&&!h.flow->canSave(),"modal refusal precedes save providers");
  send(h,handler,AcknowledgeAction{},false);effects(h,1,1,5);check(!h.flow->canSave()&&!h.flow->respond(ack,XeenPresentationResponse::Acknowledged),"published effects require terminal frame and response is consumed");
  handler.framePresented();check(h.flow->canSave(),"terminal presented frame opens save");
  auto expected=saved;++expected.questItems[18];expected.disabledObjects={{20,1}};for(unsigned i=1;i<=5;++i)expected.disabledEvents.push_back({20,i});
  check(XeenSaveFormat::encode(snapshot(h))==XeenSaveFormat::encode(expected),"authority tests preserve all nonobjective state");return true;};});
}
void failures(const fs::path &path,const XeenSaveSnapshot &saved){
 auto maximum=saved;maximum.questItems[18]=std::numeric_limits<std::uint32_t>::max();
 run(path,maximum,[&](Harness &h,XeenGameplayServices &s){s.show=[&](const auto &,const auto &handler,const auto &,const auto &,const auto &){handler.framePresented();acknowledgment(h,handler);send(h,handler,AcknowledgeAction{});effects(h,std::numeric_limits<std::uint32_t>::max(),0,0);check(h.flow->canSave()&&XeenSaveFormat::encode(snapshot(h))==XeenSaveFormat::encode(maximum),"overflow preserves complete pregrant state and recovers");return true;};});
 for(unsigned phase=0;phase<3;++phase)run(path,saved,[&](Harness &h,XeenGameplayServices &s){s.show=[&,phase](const auto &,const auto &handler,const auto &,const auto &,const auto &){
  handler.framePresented();bool injected=false;
  h.flow->reportManual=[&](const auto &r){const bool suspended=std::holds_alternative<XeenEventExecutionSuspended>(r);if(!injected&&((phase==0&&suspended)||(phase==1&&suspended&&!h.flow->canCancelInteraction())||(phase==2&&std::holds_alternative<XeenManualEventCompleted>(r)))){injected=true;throw std::runtime_error("one report fault");}};
  send(h,handler,InteractionAction{});if(!injected)send(h,handler,SelectMemberAction{4});if(!injected)send(h,handler,AcknowledgeAction{});
  check(injected&&h.flow->canSave(),"report failure recovers only after fresh terminal frame");effects(h,phase==2?1:0,phase==2?1:0,phase==2?5:0);
  const auto bytes=XeenSaveFormat::encode(snapshot(h));send(h,handler,SaveGameAction{});check(XeenSaveFormat::encode(XeenSaveFile::read(path))==bytes,"fresh F9 persists surviving report-failure effects");h.flow->refresh(true);handler.framePresented();check(bytes==XeenSaveFormat::encode(snapshot(h)),"report recovery reconstruction never replays");h.flow->reportManual={};return true;};});
 run(path,saved,[&](Harness &h,XeenGameplayServices &s){auto state=std::make_shared<std::array<bool,2>>();const auto maps=s.maps;
  s.maps=[&,maps,state](auto id){if((*state)[0]&&!(*state)[1]&&h.party->questItems.at(18)==1){(*state)[1]=true;throw std::runtime_error("Remove map preparation fault");}return maps(id);};
  s.show=[&,state](const auto &,const auto &handler,const auto &,const auto &,const auto &){handler.framePresented();acknowledgment(h,handler);h.world->discardMapCache();(*state)[0]=true;send(h,handler,AcknowledgeAction{});
   check((*state)[1]&&h.flow->canSave(),"grant-only provider fault has a trusted recovery frame");effects(h,1,0,0);auto partial=snapshot(h);auto expected=saved;++expected.questItems[18];check(XeenSaveFormat::encode(partial)==XeenSaveFormat::encode(expected),"grant survives failed Remove with no partial identity set");send(h,handler,SaveGameAction{});check(XeenSaveFormat::encode(XeenSaveFile::read(path))==XeenSaveFormat::encode(partial),"fresh F9 writes exact grant-only partial failure");
   acknowledgment(h,handler);send(h,handler,AcknowledgeAction{});effects(h,2,1,5);return true;};});
 for(bool persistent:{false,true})run(path,saved,[&](Harness &h,XeenGameplayServices &s){auto failures=std::make_shared<unsigned>(0);const auto compose=s.composeEncounter;
  s.composeEncounter=[&,compose,persistent,failures](auto &w,const auto &p,const auto &c,auto ordinary,auto actor){if(w.sessionState().disabledObjectCount()&&(persistent||!*failures)){++*failures;throw std::runtime_error("postRemove composition fault");}return compose(w,p,c,ordinary,actor);};
  s.show=[&,persistent,failures](const auto &,const auto &handler,const auto &,const auto &,const auto &){handler.framePresented();acknowledgment(h,handler);bool threw=false;try{send(h,handler,AcknowledgeAction{});}catch(const std::exception &){threw=true;}
   check(*failures>0,"postRemove composer fault reached");effects(h,1,1,5);check(persistent?threw&&!h.flow->canSave():!threw&&h.flow->canSave(),"composition recovery or fatal closure preserves publication");return true;};});
}
}
void pages(const fs::path &path,const XeenSaveSnapshot &saved){
 auto injured=saved;injured.characters[injured.activeRosterIds[0]].currentHp=-1;injured.characters[injured.activeRosterIds[0]].conditions[12]=1;
 injured.characters[injured.activeRosterIds[1]].currentHp=-30;injured.characters[injured.activeRosterIds[1]].conditions[13]=1;
 run(path,injured,[&](Harness &h,XeenGameplayServices &s){
  auto loads=std::make_shared<std::array<unsigned,4>>();
  const auto maps=s.maps;s.maps=[maps,loads](auto id){++(*loads)[0];return maps(id);};
  const auto objects=s.objects;s.objects=[objects,loads](auto id){++(*loads)[1];return objects(id);};
  const auto events=s.resources.loadEvents;s.resources.loadEvents=[events,loads](auto id){++(*loads)[2];return events(id);};
  const auto compose=s.composeEncounter;s.composeEncounter=[compose](auto &w,const auto &p,const auto &c,auto ordinary,auto actor){w.map(c.mapId);w.objectFile(c.mapId);return compose(w,p,c,ordinary,actor);};
  s.texts=[loads](auto id){++(*loads)[3];std::string text;for(unsigned i=0;i<40;++i)text+="Synthetic discovery line.\n";return XeenEventTextFile{id,"pages.txt",true,{text,"","","Synthetic bones"}};};
  s.show=[&,loads](const auto &,const auto &handler,const auto &,const auto &,const auto &){handler.framePresented();const auto before=snapshot(h);
   send(h,handler,InteractionAction{});send(h,handler,SelectMemberAction{0});check(h.flow->canCancelInteraction(),"Unconscious choice retains WhoWill");
   send(h,handler,SelectMemberAction{1});check(h.flow->canCancelInteraction(),"Dead choice retains WhoWill");effects(h,0,0,0);
   send(h,handler,SelectMemberAction{4});const auto count=h.flow->presenter().pageCount();check(count>1&&h.flow->presenter().pageIndex()==0,"synthetic discovery uses multiple real presenter pages");
   const auto generation=*h.flow->presentationGeneration();check(!h.flow->respond(generation,XeenPresentationResponse::Presented),"direct Presented cannot skip undisplayed discovery pages");
   for(unsigned page=0;page+1<count;++page){check(h.flow->presenter().pageIndex()==page,"pagination advances exactly one visible page");
    for(unsigned pass=0;pass<3;++pass){const auto priorLoads=*loads;h.eventSystem->discardScriptCache();h.eventSystem->discardTextCache();h.world->discardMapCache();h.flow->refresh(true);
     check(!h.flow->respond(generation,XeenPresentationResponse::Presented),"reconstruction must be presented before response");handler.framePresented();for(unsigned resource=0;resource<4;++resource)check((*loads)[resource]>priorLoads[resource],"combined modal rebuild reloads every discarded resource cache");
     check(h.flow->presentationGeneration()==generation&&h.flow->presenter().pageIndex()==page&&h.flow->presenter().pageCount()==count,"cache reconstruction retains semantic page and continuation generation");
     effects(h,0,0,0);check(h.party->encounterContext==before.journey->context&&h.world->sessionState().journeyRandom()==before.journey->random,"modal reconstruction advances no clock or RNG");}
    send(h,handler,AcknowledgeAction{});
   }
   check(h.flow->presentationGeneration()!=generation&&!h.flow->canCancelInteraction(),"discovery pages lead to separate Action44 acknowledgment");effects(h,0,0,0);
   const auto discovery=h.flow->frame();send(h,handler,AcknowledgeAction{});effects(h,1,1,5);check(h.flow->canSave(),"multipage collection becomes quiet after frame");bool ink=false;for(unsigned y=143;y<199;++y)for(unsigned x=0;x<320;++x){check(discovery.pixels[y*320+x]==h.flow->frame().pixels[y*320+x],"final discovery page remains unchanged through separate Action44 and success");ink|=discovery.pixels[y*320+x]!=h.base.pixels[y*320+x];}check(ink,"retained final discovery is visibly drawn");
   const auto previous=*loads;h.eventSystem->discardScriptCache();h.eventSystem->discardTextCache();h.world->discardMapCache();send(h,handler,InteractionAction{});
   check((*loads)[0]>previous[0]&&(*loads)[1]>previous[1]&&(*loads)[2]>previous[2],"explicit effective-None repeat really reloads immutable map MOB and EVT");effects(h,1,1,5);
   check((*loads)[3]>0,"original text provider was used");return true;};
 });
}
void integrity(const fs::path &path,const XeenSaveSnapshot &saved){
 for(unsigned seam=0;seam<6;++seam)for(bool replace:{false,true})run(path,saved,[&](Harness &h,XeenGameplayServices &s){
  struct State{bool armed=false,injected=false;unsigned reentrant=0;};auto state=std::make_shared<State>();
  const auto attack=[&,state,replace]{if(!state->armed||state->injected)return;state->injected=true;
   check(!h.flow->canSave(),"callback cannot capture a live Event operation");
   const auto generation=h.flow->presentationGeneration();
   if(generation)check(!h.flow->respond(*generation,XeenPresentationResponse::Acknowledged),"reentrant response refused during callback");
   h.flow->handle(InteractionAction{},h.flow->displayedInput());++state->reentrant;
   auto *flags=const_cast<XeenGameFlags *>(h.flags);if(replace){const auto values=flags->values();flags->~XeenGameFlags();new(flags)XeenGameFlags(values);}else flags->set(7);
  };
  if(seam==0){const auto provider=s.resources.loadEvents;s.resources.loadEvents=[provider,attack](auto id){attack();return provider(id);};}
  if(seam==1){const auto provider=s.texts;s.texts=[provider,attack](auto id){attack();return provider(id);};}
  if(seam==2){const auto provider=s.maps;s.maps=[provider,attack](auto id){attack();return provider(id);};}
  if(seam==3){const auto provider=s.objects;s.objects=[provider,attack](auto id){attack();return provider(id);};}
  if(seam==5){const auto compose=s.composeEncounter;s.composeEncounter=[compose,attack](auto &w,const auto &p,const auto &c,auto ordinary,auto actor){attack();return compose(w,p,c,ordinary,actor);};}
  s.show=[&,state,attack,seam](const auto &,const auto &handler,const auto &,const auto &,const auto &){handler.framePresented();
   if(seam==0)h.eventSystem->discardScriptCache();if(seam==1)h.eventSystem->discardTextCache();if(seam==2||seam==3)h.world->discardMapCache();
   if(seam==4)h.flow->reportManual=[attack](const auto &){attack();};
   state->armed=true;try{send(h,handler,InteractionAction{});}catch(const std::exception &){}
   check(state->injected&&state->reentrant==1&&!h.flow->canSave(),"provider/report/composer owner mutation cannot recover quiet authority");effects(h,0,0,0);
   const auto saves=h.saves;try{send(h,handler,SaveGameAction{});}catch(const std::exception &){}check(saves==h.saves,"integrity violation refuses saving before I/O");return true;
  };
 },replace?3:0);
}
int main(){try{const auto directory=fs::temp_directory_path()/("mmodern-m31-events-"+std::to_string(GetCurrentProcessId())+"-"+std::to_string(GetTickCount64()));fs::create_directory(directory);const auto path=directory/"objective.mmsave";const auto saved=objective(path);authority(path,saved);failures(path,saved);pages(path,saved);integrity(path,saved);std::cout<<"Journey objective authority and failure controls passed\n";return 0;}catch(const std::exception &e){std::cerr<<e.what()<<'\n';return 1;}}
