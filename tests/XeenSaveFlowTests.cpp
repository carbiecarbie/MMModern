#include "XeenRegionalSaveGameplayTestSupport.h"
#include <fstream>
#include <iostream>
using namespace gameplay_test;
using save_test::rejects;
namespace fs=std::filesystem;
const XeenCamera start{1,1,1,XeenDirection::North};
Bytes diskBytes(const fs::path &path){std::ifstream in(path,std::ios::binary);check(bool(in),"cannot read Application fixture");return Bytes(std::istreambuf_iterator<char>(in),{});}
void writeBytes(const fs::path &path,const Bytes &bytes){std::ofstream out(path,std::ios::binary|std::ios::trunc);out.write(reinterpret_cast<const char *>(bytes.data()),bytes.size());check(bool(out),"fixture write");}
void olderPolicy(const fs::path &path){
 const auto old=save_test::nonzeroLegacy();writeBytes(path,old);
 regional_save_test::Fixture f;auto services=f.services();bool shown=false;
 services.show=[&](const auto&,const auto&,const auto&,const auto&,const auto&){shown=true;return true;};
 check(Application().playGameplay(services,{},path,true)==3&&!shown&&diskBytes(path)==old,"older startup published or rewrote file");
 services.show=[&](const auto&,const auto &handle,const auto&,const auto&,const auto &status){f.present(handle);f.send(handle,SaveGameAction{});check(status().find("Saved")!=std::string::npos,"F9 older overwrite failed");sameSnapshot(f.capture(),XeenSaveFile::read(path));return true;};
 check(Application().playGameplay(services,{},path,false,XeenEncounterEntry::Journey,1)==0,"fresh Journey older overwrite");
}
void startup(const fs::path &path){
 std::vector<Bytes> frames;
 for(unsigned index:{0u,1u}){
  regional_save_test::Fixture f;f.saved.camera={23,8,2,XeenDirection::North};f.saved.questItems[17]=3;f.saved.characters[0].currentHp=23;f.saved.disabledObjects={{23,index}};
  XeenSaveFile::write(path,f.saved);const auto original=diskBytes(path);auto services=f.services();regional_save_test::phirna(services);
  unsigned mapReads=0,objectReads=0,eventReads=0,textReads=0,automatic=0;
  services.maps=[&](auto id){++mapReads;return regional_test::map(id);};
  const auto objectProvider=services.objects;services.objects=[&](auto id){++objectReads;return objectProvider(id);};
  const auto scriptProvider=services.resources.loadEvents;services.resources.loadEvents=[&](auto id){++eventReads;return scriptProvider(id);};
  services.texts=[&](auto id){++textReads;return regional_test::texts(id);};
  auto configure=services.configureFlow;services.configureFlow=[&](auto &flow,const auto &c){configure(flow,c);flow.reportAutomatic=[&](const auto&){++automatic;};};
  auto compose=services.composeEncounter;std::vector<Bytes> composed;
  services.composeEncounter=[&](auto &w,const auto &p,const auto &c,auto phase,auto actor){auto frame=compose(w,p,c,phase,actor);frame.frame.pixels[11]=w.isObjectDisabled({23,1});composed.push_back(frame.frame.pixels);return frame;};
  services.show=[&](const auto &first,const auto &handle,const auto&,const auto&,const auto &status){
   f.present(handle);check(first.pixels[6]==23&&first.pixels[4]==3&&first.pixels[10+index]&&!first.pixels[11-index],"restored first frame used default or wrong equal-count owners");
   frames.push_back(first.pixels);for(const auto &pixels:composed)check(std::equal(pixels.begin(),pixels.begin()+12,first.pixels.begin()),"default frame flashed before restored owners");
   check(!automatic&&!f.flow->presentationGeneration()&&diskBytes(path)==original,"restore replayed event or rewrote disk");
   auto &events=*f.events;
   f.send(handle,InteractionAction{});check(f.flow->blocksGameplay(),"resumed interaction did not dispatch");f.send(handle,CancelInteractionAction{});
   const auto maps=mapReads,objects=objectReads,scripts=eventReads,texts=textReads;const auto retained=f.flow->frame().pixels;
   f.world->discardMapCache();events.discardScriptCache();events.discardTextCache();
   check(f.flow->refresh(true).pixels==retained,"cache reconstruction changed restored frame");f.present(handle);
   f.send(handle,InteractionAction{});check(f.flow->blocksGameplay(),"resumed interaction did not dispatch");f.send(handle,CancelInteractionAction{});
   check(mapReads>maps&&objectReads>objects&&eventReads>scripts&&textReads>texts,"cache reconstruction did not call each provider");
   f.send(handle,SaveGameAction{});check(status().find("Saved")!=std::string::npos&&diskBytes(path)==original,"eligible F9 changed durable state");return true;
  };
  check(Application().playGameplay(services,{},path,true)==0,"current startup graph failed");
 }
 check(frames[0]!=frames[1],"equal-count identities collapsed");
}
void failures(const fs::path &path){
 for(int mode=0;mode<7;++mode){
  fs::remove(path);regional_save_test::Fixture f;auto s=f.saved;if(mode==1)s.resources.clouds.crc32++;if(mode==2)s.activeRosterIds={24};XeenSaveFile::write(path,s);
  if(mode==0)fs::remove(path);if(mode==3)writeBytes(path,{'b','a','d'});if(mode==4){auto bytes=XeenSaveFormat::encode(s);bytes[8]=9;writeBytes(path,bytes);}
  auto services=f.services();auto compose=services.composeEncounter;
  services.composeEncounter=[&](auto &w,const auto &p,const auto &c,auto phase,auto actor){if(mode==5)throw std::runtime_error("injected first-frame failure");auto frame=compose(w,p,c,phase,actor);if(mode==6)frame.frame.width=0;return frame;};
  bool shown=false;services.show=[&](const auto&,const auto&,const auto&,const auto&,const auto&){shown=true;return true;};
  check(Application().playGameplay(services,{},path,true)==3&&!shown,"failed resume exposed gameplay or fell back");
 }
 fs::remove(path);regional_save_test::Fixture f;auto services=f.services();
 services.show=[&](const auto&,const auto &h,const auto&,const auto&,const auto &status){f.present(h);writeBytes(path,{'u','n','k','n','o','w','n'});const auto old=diskBytes(path);f.send(h,SaveGameAction{});check(status().find("Save failed")!=std::string::npos&&diskBytes(path)==old,"unknown destination protection");f.send(h,NavigationAction::TurnRight);fs::remove(path);f.send(h,SaveGameAction{});check(XeenSaveFile::read(path).camera.direction==XeenDirection::North,"write failure stopped current gameplay");return true;};
 check(Application().playGameplay(services,{},path,false,XeenEncounterEntry::Journey,1)==0,"recoverable current write failure");
}
void pending(const fs::path &path){
 for(int kind=0;kind<7;++kind){
  fs::remove(path);Fixture f;
  XeenEventRecord request=record(1,1,0,1,{1}); // Display; long text forces pages.
  if(kind==0)f.text.strings[1]=std::string(3000,'X');
  if(kind==1)request=record(1,1,0,9,{44,1,1});
  if(kind==2)request=record(1,1,0,9,{44,0,1});
  if(kind==3)request=record(1,1,0,5,{0,1,9,1,1});
  if(kind==4||kind==5)request=record(1,1,0,0x20,{0,1});
  if(kind==6)request=record(1,1,0,0x31,{1,0});
  f.scripts[1]={request,record(1,1,1,0x12)};
  auto services=f.services();services.show=[&](const auto&,const auto &handle,const auto&,const auto&,const auto &status){
   handle(InteractionAction{});check(f.flow->blocksGameplay(),"fixture did not suspend");
   if(kind==5){handle(SelectMemberAction{5});check(f.flow->blocksGameplay(),"invalid selection ended WhoWill");}
   const auto generation=f.flow->presentationGeneration();const auto page=f.flow->presenter().pageIndex();
   const auto pixels=f.flow->frame().pixels;const auto reads=f.eventReads,composes=f.compositions;
   const auto timing=f.flow->presenter().npcTiming();
   handle(InspectInventoryAction{});handle(TransferInventoryAction{});handle(SelectInventorySlotAction{8});handle(EquipmentInventoryAction{});
   check(!f.flow->inventoryOpen(),"pending event opened inventory");
   handle(SaveGameAction{});
   check(!fs::exists(path)&&f.flow->presentationGeneration()==generation&&f.flow->presenter().pageIndex()==page&&f.flow->frame().pixels==pixels&&f.eventReads==reads&&f.compositions==composes,"refused save mutated presentation or performed preparation");
   check(f.flow->presenter().npcTiming().deadline==timing.deadline,"save changed NPC timing");
   check(status().find("Cannot save while an interaction is pending.")!=std::string::npos,"pending feedback");
   for(unsigned guard=0;f.flow->blocksGameplay()&&guard<200;++guard){
    if(kind==4||kind==5)handle(CancelInteractionAction{});else if(kind==2)handle(NoAction{});else handle(AcknowledgeAction{});
   }
   check(!f.flow->blocksGameplay()&&!fs::exists(path),"refused save was queued");
   const auto retained=f.flow->frame().pixels;handle(SaveGameAction{});
   check(!fs::exists(path)&&retained==f.flow->frame().pixels,"unsaveable explorer F9 cleared retained label");return true;
  };
  check(Application().playGameplay(services,start,path,false)==0,"pending Application path");
 }
 Fixture f;auto services=f.services();services.show=[&](const auto&,const auto &handle,const auto&,const auto&,const auto &status){handle(SaveGameAction{});check(status().find("Map exploration cannot save.")!=std::string::npos,"no-target feedback");return true;};
 check(Application().playGameplay(services,start,std::nullopt,false)==0,"no target session");
}
void dispatchBoundaries(const fs::path &path){
 fs::remove(path);Fixture f;auto services=f.services();
 SdlWindow::FrameUpdateHandler current;
 auto configure=services.configureFlow;
 services.configureFlow=[&](XeenEventFlow &flow,const XeenCamera &camera){
  configure(flow,camera);
  flow.reportManual=[&](const auto&){if(current)current(SaveGameAction{});};
 };
 services.show=[&](const auto&,const auto &handle,const auto&,const auto&,const auto &status){
  current=handle;handle(InteractionAction{});
  check(!fs::exists(path)&&status().find("idle gameplay boundary")!=std::string::npos,"event callback saved reentrantly");
  handle(SaveGameAction{});check(!fs::exists(path),"unsaveable explorer wrote after dispatch");current={};return true;
 };
 check(Application().playGameplay(services,start,path,false)==0,"dispatch guard");
 // A fatal automatic report retains prior movement, but production exits and may not save afterward.
 fs::remove(path);Fixture fatal;auto fatalServices=fatal.services();auto mapProvider=fatalServices.maps;
 fatalServices.maps=[&](XeenMapIdentity id){auto m=mapProvider(id);m.geometry.cells[33].rawAttributes=0x10;return m;};
 fatal.scripts[1]={record(1,2,0,12,{0,0,21,99}),record(1,2,1,0xff)};
 fatalServices.show=[&](const auto&,const auto &handle,const auto&,const auto&,const auto &status){
  rejects([&]{handle(NavigationAction::MoveForward);},"automatic error");
  check(fatal.frames.back().pixels[2]==2,"automatic failure rolled back prior movement");
  handle(SaveGameAction{});check(!fs::exists(path)&&status().find("idle gameplay boundary")!=std::string::npos,"fatal failure allowed saving");
  return false;
 };
 check(Application().playGameplay(fatalServices,start,path,false)==4,"fatal shutdown result");
}
void currentSaveBoundary(const fs::path &path){
 fs::remove(path);regional_save_test::Fixture f;auto services=f.services();unsigned clocks=0;services.clock=[&]{++clocks;return f.now;};Bytes initial;
 SdlWindow::FrameUpdateHandler nested;bool reentered=false;auto compose=services.composeEncounter;
 services.composeEncounter=[&](auto &w,const auto &p,const auto &c,auto phase,auto actor){if(nested&&&w!=f.world){const auto count=f.compositions;nested(SaveGameAction{});nested(InspectInventoryAction{});check(!f.flow->inventoryOpen()&&f.compositions==count,"F9 preflight allowed reentrancy");reentered=true;}return compose(w,p,c,phase,actor);};
 services.show=[&](const auto&,const auto &h,const auto&,const auto &idle,const auto &status){
  f.present(h);nested=h;initial=XeenSaveFormat::encode(f.capture());f.now=100;idle();f.present(h);check(f.phases.back()==1&&XeenSaveFormat::encode(f.capture())==initial,"cosmetic phase serialized");
  f.now=199;const auto calls=clocks;const auto pixels=f.flow->frame().pixels;f.send(h,SaveGameAction{});
  check(status().find("Saved")!=std::string::npos&&reentered&&clocks==calls&&f.phases.back()==0,"F9 preflight phase/clock/reentrancy");
  check(diskBytes(path)==initial&&f.flow->frame().pixels==pixels,"F9 changed current durable or presentation state");
  const auto n=f.phases.size();idle();check(f.phases.size()==n,"F9 moved deadline earlier");f.now=200;idle();f.present(h);check(f.phases.back()==2&&XeenSaveFormat::encode(f.capture())==initial,"F9 rearmed deadline");return true;
 };
 check(Application().playGameplay(services,{},path,false,XeenEncounterEntry::Journey,1)==0,"current save animation boundary");
 regional_save_test::Fixture restored;restored.now=500;auto resumed=restored.services();
 resumed.show=[&](const auto &first,const auto &h,const auto&,const auto &idle,const auto&){restored.present(h);check(first.pixels[20]==0&&restored.phases==std::vector<std::uint64_t>({0,0}),"restored phase/preflight");idle();restored.present(h);check(restored.phases.back()==1,"restored first idle tick");restored.now=599;const auto n=restored.phases.size();idle();check(restored.phases.size()==n,"restored early deadline");restored.now=600;idle();restored.present(h);check(restored.phases.back()==2,"restored scheduled tick");return true;};
 check(Application().playGameplay(resumed,{},path,true)==0&&diskBytes(path)==initial,"current restored startup changed file");
}
void developerTarget(const fs::path &path) {
 regional_save_test::Fixture f;auto services=f.services();f.saved.name="Managed name";
 XeenSaveFile::write(path,f.saved);const auto original=diskBytes(path);
 const auto loose=path.parent_path()/"developer.mmsave";fs::remove(loose);
 services.initialSlot=2;services.developerSavePath=loose;
 services.show=[&](const auto &,const auto &h,const auto &,const auto &,const auto &status) {
  f.present(h);f.send(h,SaveGameAction{});
  check(status().find("Saved")!=std::string::npos && diskBytes(loose)==original && diskBytes(path)==original,
   "managed current slot redirected/disabled explicit developer F9");return true;
 };
 check(Application().playGameplay(services,{},path,true)==0,"independent loose developer save path");
}
int main(){try{const auto dir=fs::current_path()/"save-flow-tests";fs::create_directories(dir);const auto path=dir/"session.mmsave";fs::remove(path);olderPolicy(path);startup(path);pending(path);failures(path);dispatchBoundaries(path);currentSaveBoundary(dir/"animation.mmsave");developerTarget(dir/"managed.mmsave");std::cout<<"Current Application startup, save eligibility, overwrite and failure paths passed\n";return 0;}catch(const std::exception &e){std::cerr<<e.what()<<'\n';return 1;}}
