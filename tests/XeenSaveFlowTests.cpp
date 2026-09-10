#include "XeenSaveGameplayTestSupport.h"
#include <fstream>
#include <iostream>
using namespace gameplay_test;
using save_test::rejects;
namespace fs=std::filesystem;
const XeenCamera start{1,1,1,XeenDirection::North};
Bytes diskBytes(const fs::path &path) {
 std::ifstream in(path,std::ios::binary);check(bool(in),"cannot read Application fixture");
 return Bytes(std::istreambuf_iterator<char>(in),{});
}
void legacyUpgrade(const fs::path &path) {
 const auto original=save_test::nonzeroLegacy();
 {std::ofstream out(path,std::ios::binary|std::ios::trunc);
  out.write(reinterpret_cast<const char*>(original.data()),original.size());out.close();check(bool(out),"cannot write independent v1 fixture");}
 Fixture f;save_test::distinctiveInitialItems(f.initial.roster);
 const auto legacy=XeenSaveFormat::decode(original);
 const auto expected=save_test::expectedLegacy(legacy,f.initial.roster);
 auto services=f.services();bool observed=false;
 services.observeGameplay=[&](XeenWorld &world,XeenEventSystem &,const XeenPartyState &party,XeenCamera &camera,const XeenGameFlags &flags){
  sameSnapshot(expected,XeenSaveState::capture(f.signature,party,camera,flags,world));
  check(&party.party.member(party.roster,0)==&party.roster.at(18)&&
   &party.party.member(party.roster,2)==&party.roster.at(18),"Application legacy membership aliases");
  check(diskBytes(path)==original,"Application startup migrated v1 on disk");observed=true;
 };
 services.show=[&](const auto&,const auto &handle,const auto&,const auto&,const auto &status){
  check(observed,"legacy owners not observed before gameplay input");
  check(diskBytes(path)==original,"read/startup changed v1 bytes");
  handle(SaveGameAction{}); // The production action handler used by F9.
  check(status().find("Saved")!=std::string::npos,"Application legacy upgrade save failed");
  const auto bytes=diskBytes(path);check(bytes[8]==2&&bytes[9]==0,"F9 did not write v2");
  sameSnapshot(expected,XeenSaveFile::read(path));return true;
 };
 check(Application().playGameplay(services,start,path,true)==0&&observed,"production legacy resume failed");
 // Different initial items prove that v2 restoration uses its saved complete records.
 Fixture next;save_test::distinctiveInitialItems(next.initial.roster);
 for(unsigned i=0;i<30;++i){auto &c=next.initial.roster.at(i);c.weapons={};c.armor={};c.accessories={};c.miscellaneous={};}
 auto resumed=next.services();bool restored=false;
 resumed.observeGameplay=[&](XeenWorld &world,XeenEventSystem &,const XeenPartyState &party,XeenCamera &camera,const XeenGameFlags &flags){
  sameSnapshot(expected,XeenSaveState::capture(next.signature,party,camera,flags,world));restored=true;
 };
 resumed.show=[&](const auto&,const auto&,const auto&,const auto&,const auto&){check(restored,"v2 owners not observed");return true;};
 check(Application().playGameplay(resumed,start,path,true)==0&&restored,"production upgraded v2 restore failed");
 std::cout<<"Application independent v1 resume, unchanged startup bytes, F9 v2 upgrade and authoritative v2 resume passed\n";
}
void startup(const fs::path &path){
 for(bool resume:{false,true}){
  Fixture f;f.automatic=true;f.scripts[1]={record(1,1,0,12,{0,0,21,99}),record(1,1,1,12,{0,0,20,7}),record(1,1,2,0x1f,{2,1,1})};
  auto s=f.saved();s.questItems[17]=3;s.characters[0].currentHp=23;XeenSaveFile::write(path,s);
  auto services=f.services();services.show=[&](const auto &first,const auto &handle,const auto&,const auto&,const auto &status){
   check(first.pixels[0]==(resume?1:2) && first.pixels[4]==(resume?3:1),"production initial dispatch choice");
   check(first.pixels[6]==(resume?23:10),"default party flashed on resume");
   check(f.eventReads==(resume?0U:2U),"unexpected initial automatic dispatch");
   check(!f.flow->blocksGameplay() && !f.flow->presentationGeneration(),"stale presentation");
   handle(SaveGameAction{});auto saved=XeenSaveFile::read(path);
   check(saved.gameFlags[7]==!resume,"initial flag commit");
   if(resume){handle(NavigationAction::TurnRight);handle(SaveGameAction{});saved=XeenSaveFile::read(path);check(saved.camera.mapId==XeenMapIdentity(2)&&saved.questItems[17]==4&&saved.gameFlags[7],"later navigation skipped automatic event");}
   check(status().find("Saved")!=std::string::npos,"save status absent");return true;
  };
  check(Application().playGameplay(services,start,path,resume)==0,"startup failed");
 }
 // Equal counts, separate production startup graphs; no initial/default frame.
 std::vector<Bytes> firstFrames;
 for(unsigned index:{0U,1U}){
  Fixture f;auto s=f.saved();s.disabledObjects={{1,index}};XeenSaveFile::write(path,s);auto services=f.services();
  services.show=[&](const auto &first,const auto&,const auto&,const auto&,const auto&){
   firstFrames.push_back(first.pixels);check(first.pixels[10+index]==1&&first.pixels[11-index]==0,"wrong equal-count frame");
   for(const auto &frame:f.frames)check(frame.pixels==first.pixels,"a default frame composed before restored owners");
   const auto maps=f.mapReads,objects=f.objectReads;f.world->discardMapCache();
   check(f.flow->refresh(true).pixels==first.pixels&&f.mapReads>maps&&f.objectReads>objects,"genuine cache reconstruction failed");return true;
  };check(Application().playGameplay(services,start,path,true)==0,"restored graph failed");
 }
 check(firstFrames[0]!=firstFrames[1],"equal-count startup retained another graph");
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
   check(fs::exists(path)&&retained==f.flow->frame().pixels,"fresh save cleared retained label");return true;
  };
  check(Application().playGameplay(services,start,path,false)==0,"pending Application path");
 }
 Fixture f;auto services=f.services();services.show=[&](const auto&,const auto &handle,const auto&,const auto&,const auto &status){handle(SaveGameAction{});check(status().find("No save target")!=std::string::npos,"no-target feedback");return true;};
 check(Application().playGameplay(services,start,std::nullopt,false)==0,"no target session");
}
void failures(const fs::path &path){
 for(int mode=0;mode<7;++mode){
  Fixture f;auto s=f.saved();if(mode==1)s.resources.clouds.crc32++;if(mode==2)s.activeRosterIds={24};
  XeenSaveFile::write(path,s);
  if(mode==0)fs::remove(path);
  if(mode==3){std::ofstream out(path,std::ios::binary|std::ios::trunc);out<<"not a save";}
  if(mode==4){auto bytes=XeenSaveFormat::encode(s);bytes[8]=3;std::ofstream out(path,std::ios::binary|std::ios::trunc);out.write(reinterpret_cast<const char*>(bytes.data()),bytes.size());}
  if(mode==5)f.failCompose=true;if(mode==6)f.invalidFrame=true;
  auto services=f.services();bool shown=false;services.show=[&](const auto&,const auto&,const auto&,const auto&,const auto&){shown=true;return true;};
  check(Application().playGameplay(services,start,path,true)==3&&!shown,"failed resume exposed gameplay or fell back");
  fs::remove(path);
 }
 Fixture f;auto services=f.services();
 services.show=[&](const auto&,const auto &handle,const auto&,const auto&,const auto &status){
  {std::ofstream out(path);out<<"unknown";}handle(SaveGameAction{});check(status().find("Save failed")!=std::string::npos,"save failure feedback");
  handle(NavigationAction::TurnRight);fs::remove(path);handle(SaveGameAction{});
  check(XeenSaveFile::read(path).camera.direction==XeenDirection::East,"save failure stopped gameplay");return true;
 };check(Application().playGameplay(services,start,path,false)==0,"recoverable write failure");
}
void mutation(const fs::path &path){
 for(int outcome=0;outcome<3;++outcome){
  Fixture f;f.scripts[1]={record(1,1,0,0x1f,{2,1,1})};
  f.scripts[2]={record(1,1,0,12,{0,0,20,7}),record(1,1,1,12,{0,0,104,2}),record(1,1,2,12,{0,0,21,99}),
   record(1,1,3,outcome==0?0x0e:outcome==1?9:0x20,outcome==1?Bytes{44,1,4}:outcome==2?Bytes{0,0}:Bytes{}),record(1,1,4,0xff)};
  if(outcome==0)f.scripts[2]={record(1,1,0,0x19,{7,8,0}),record(1,1,1,0x12),
   record(7,8,0,9,{21,99,6}),record(7,8,1,12,{0,0,20,7}),record(7,8,2,12,{0,0,104,2}),
   record(7,8,3,12,{0,0,21,99}),record(7,8,4,0x0e),record(7,8,5,0xff),record(7,8,6,0xff)};
  auto services=f.services();services.show=[&](const auto&,const auto &handle,const auto&,const auto&,const auto&){
   handle(InteractionAction{});if(outcome==1)f.flow->abandonPresentation();if(outcome==2)handle(CancelInteractionAction{});
   check(!f.flow->blocksGameplay(),"mutation not at idle");handle(SaveGameAction{});const auto saved=XeenSaveFile::read(path);
   check(saved.questItems[17]==1&&saved.questFlags[2]&&saved.gameFlags[7]==(outcome==2)&&saved.camera.mapId==XeenMapIdentity(outcome==2?2:1),"saved mutation/rollback policy");
   check(saved.disabledObjects.size()==(outcome==0?1U:0U),"Remove error effects missing");return true;
  };check(Application().playGameplay(services,start,path,false)==0,"mutation save path");
  const auto expected=XeenSaveFile::read(path);
  check(expected.disabledEvents.size()==(outcome==0?2U:0U),"independent Remove events missing");
  Fixture restored;restored.scripts=f.scripts;auto resumed=restored.services();
  resumed.show=[&](const auto &first,const auto &handle,const auto&,const auto&,const auto&){
   check(first.pixels[0]==expected.camera.mapId.number&&first.pixels[4]==1&&first.pixels[5]==1,"mutation resume first frame");
   handle(SaveGameAction{});sameSnapshot(XeenSaveFile::read(path),expected);return true;
  };
  check(Application().playGameplay(resumed,start,path,true)==0,"mutation production resume path");
 }
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
  handle(SaveGameAction{});check(fs::exists(path),"fresh idle save after dispatch refused");current={};return true;
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
void ordinarySaveBoundary(const fs::path &path){
 fs::remove(path);Fixture f;f.ordinary=true;std::uint64_t now=0;unsigned clockCalls=0;
 auto services=f.services();services.clock=[&]{++clockCalls;return now;};
 const XeenPartyState *party=nullptr;const XeenGameFlags *flags=nullptr;XeenCamera *camera=nullptr;
 auto observe=services.observeGameplay;
 services.observeGameplay=[&](XeenWorld &w,XeenEventSystem &e,const XeenPartyState &p,XeenCamera &c,const XeenGameFlags &g){observe(w,e,p,c,g);party=&p;flags=&g;camera=&c;};
 Bytes firstBytes;
 services.show=[&](const auto&,const auto &handle,const auto&,const auto &idle,const auto &status){
  check(f.phases==std::vector<std::uint64_t>{0},"fresh startup added preflight");
  f.world->disableObject({1,0});f.flow->refresh();
  auto capture=[&]{return XeenSaveFormat::encode(XeenSaveState::capture(f.signature,*party,*camera,*flags,*f.world));};
  firstBytes=capture();now=100;idle();check(f.phases.back()==1 && capture()==firstBytes,"phase serialized into v2");
  now=199;const auto calls=clockCalls;const auto liveFrame=f.flow->frame();
  handle(InspectInventoryAction{});check(clockCalls==calls && f.phases.back()==1,"inventory changed phase/clock");
  handle(CancelInteractionAction{}); // Close the player panel before a new eligible F9.
  handle(SaveGameAction{});
  check(status().find("Saved")!=std::string::npos && clockCalls==calls && f.phases.back()==0,"preflight did not use independent zero");
  check(diskBytes(path)==firstBytes && f.flow->frame().pixels==liveFrame.pixels,"F9 mutated live frame or save bytes");
  check(f.world->isObjectDisabled({1,0}),"preflight replaced live-world observer");
  const auto n=f.phases.size();idle();check(f.phases.size()==n,"preflight moved deadline earlier");
  now=200;idle();check(f.phases.back()==2 && capture()==firstBytes,"preflight secretly rearmed live deadline");
  return true;
 };
 check(Application().playGameplay(services,start,path,false)==0,"ordinary save producer");
 Fixture restored;restored.ordinary=true;restored.automatic=true;now=500;
 auto resumed=restored.services();resumed.clock=[&]{return now;};
 resumed.show=[&](const auto &first,const auto&,const auto&,const auto &idle,const auto&){
  check(restored.phases==std::vector<std::uint64_t>({0,0}) && restored.eventReads==0,"restored phase/preflight or initial replay");
  check(restored.world->isObjectDisabled({1,0}) && first.pixels[20]==0,"restored animation/removal");
  now=599;const auto n=restored.phases.size();idle();check(restored.phases.size()==n,"restored early deadline");
  now=600;idle();check(restored.phases.back()==1,"restored first due tick");return true;
 };
 check(Application().playGameplay(resumed,start,path,true)==0 && diskBytes(path)==firstBytes,"ordinary save restored startup");
}
int main(){try{const auto dir=fs::current_path()/"save-flow-tests";fs::create_directories(dir);const auto path=dir/"session.mmsave";fs::remove(path);legacyUpgrade(path);startup(path);pending(path);failures(path);mutation(path);dispatchBoundaries(path);ordinarySaveBoundary(dir/"animation.mmsave");std::cout<<"Production Application startup, save eligibility, failures and mutation policies passed\n";return 0;}catch(const std::exception &e){std::cerr<<e.what()<<'\n';return 1;}}
