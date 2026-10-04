#include "XeenSaveGameplayTestSupport.h"
#include "XeenRegionalSaveGameplayTestSupport.h"
#include "XeenChildProcessTestSupport.h"
#include "XeenCheckpointTestSupport.h"
#include "games/xeen/CloudsUiComposer.h"
#include "games/xeen/XeenCharacterRules.h"
#include <iostream>
using namespace gameplay_test;
namespace fs=std::filesystem;
using Mode=XeenInventoryMode;
using Status=XeenTransferStatus;
namespace mmodern {
struct XeenInventoryTestAccess {
	static void exhaust(XeenEventFlow &flow) { flow._inventoryEpoch=std::numeric_limits<std::uint64_t>::max()-2; }
};
}
namespace {
const XeenCamera start{1,1,1,XeenDirection::North};
struct Quiet { std::ostringstream out;std::streambuf *old=std::cout.rdbuf(out.rdbuf());~Quiet(){std::cout.rdbuf(old);} };
void seed(Fixture &f) {
	f.initial.roster.at(0).name="gyp%\x01\xff";
	f.initial.roster.at(1).name="Receiver";
	for(auto *items:{&f.initial.roster.at(0).weapons,&f.initial.roster.at(0).armor,&f.initial.roster.at(0).accessories,&f.initial.roster.at(0).miscellaneous})
		*items={{{10,37,1,0},{10,37,1,0},{},{},{},{},{},{},{}}};
}
void gameplay() {
 Fixture f;seed(f);f.ordinary=true;auto services=f.services();XeenPartyState *live=nullptr;unsigned reports=0;
 services.observeGameplay=[&](auto &,auto &,const auto &p,auto &,const auto &){live=&const_cast<XeenPartyState &>(p);};
 services.show=[&](const auto &,const auto &handle,const auto &escape,const auto &idle,const auto &status){
  const auto before=remove_test::partySnapshot(*live);
  handle(SelectMemberAction{0});check(f.flow->inventoryOpen() && escape(),"F1 sheet opens");
  handle(DialogKeyAction{'q'});handle(DialogKeyAction{InputKey::Escape});
  handle(DialogKeyAction{'e'});handle(DialogKeyAction{InputKey::Escape});
  check(remove_test::partySnapshot(*live)==before,"unsupported sheet actions mutated");
  handle(DialogKeyAction{'i'});check(f.flow->inventorySelection().mode==Mode::Browse,"I sheet items");
  handle(DialogKeyAction{'d'});handle(DialogKeyAction{'1'});handle(DialogKeyAction{InputKey::Escape});
  handle(DialogKeyAction{'q'});handle(DialogKeyAction{InputKey::Escape});
  check(remove_test::partySnapshot(*live)==before,"unsupported items mutated");
  handle(DialogKeyAction{'1'}); // Discard selection remained highlighted; deselect.
  handle(DialogKeyAction{'1'});handle(SaveGameAction{});check(status().find("inventory")!=std::string::npos,"modal save refused");
  const auto selected=f.flow->inventorySelection().record;idle();check(xeenSameItem(selected,f.flow->inventorySelection().record),"idle changed selection");
  f.flow->reportInventory=[&](const auto &r){++reports;check(r.status==Status::Success && !f.flow->inventorySelection().slot,"report after publication");handle(SelectMemberAction{1});handle(SaveGameAction{});};
  handle(DialogKeyAction{InputKey::F1+1});
  check(reports==1 && live->roster.at(0).weapons[1].id==0 && live->roster.at(1).weapons[0].id==37,"immediate F2 transfer once");
  const auto after=remove_test::partySnapshot(*live);handle(AcknowledgeAction{});check(remove_test::partySnapshot(*live)==after,"Enter replay");
  handle(DialogKeyAction{InputKey::Escape});check(f.flow->inventoryOpen(),"items Esc returns sheet");
  handle(DialogKeyAction{InputKey::Escape});check(!f.flow->inventoryOpen(),"sheet Esc closes");return true;
 };Quiet quiet;check(Application().playGameplay(services,start,{},false)==0,"original inventory flow");
}
void invalidation() {
 for(unsigned mutation=0;mutation<4;++mutation){Fixture f;seed(f);auto services=f.services();XeenPartyState *live=nullptr;
 services.observeGameplay=[&](auto &,auto &,const auto &p,auto &,const auto &){live=&const_cast<XeenPartyState &>(p);};
 services.show=[&](const auto &,const auto &handle,const auto &,const auto &,const auto &){
  handle(InspectInventoryAction{});handle(SelectInventorySlotAction{0});
  if(mutation==0){const auto copy=live->roster.at(0);live->roster.at(0)=copy;f.flow->invalidateInventory();}
  else if(mutation==1){live->party=XeenParty::fromRosterIds({1,0});f.flow->invalidateInventory();}
  else if(mutation==2)live->roster.at(0).weapons[0].state=9;
  else XeenInventoryTestAccess::exhaust(*f.flow);
  const auto before=remove_test::partySnapshot(*live);handle(SelectMemberAction{1});
  check(remove_test::partySnapshot(*live)==before,"stale transfer mutated");return true;
 };Quiet quiet;check(Application().playGameplay(services,start,{},false)==0,"transfer invalidation");}
}
void failure(){
 Fixture f;seed(f);auto services=f.services();const XeenPartyState *live=nullptr;
 services.observeGameplay=[&](auto &,auto &,const auto &p,auto &,const auto &){live=&p;};
 services.show=[&](const auto &,const auto &handle,const auto &,const auto &,const auto &){
  handle(InspectInventoryAction{});handle(SelectInventorySlotAction{0});
  f.flow->reportInventory=[](const auto &){throw std::runtime_error("transfer reporting fault");};
  handle(SelectMemberAction{1});check(!f.flow->inventoryOpen() && live->roster.at(1).weapons[0].id==37,"published transfer recovery");
  handle(InspectInventoryAction{});handle(AcknowledgeAction{});check(live->roster.at(1).weapons[1].id==0,"recovery replay");return true;
 };Quiet quiet;check(Application().playGameplay(services,start,{},false)==0,"transfer recovery");
}
void automaticGuard(){
 Fixture f;seed(f);f.automatic=true;f.scripts[1]={record(1,1,0,9,{44,1,1}),record(1,1,1,0x12)};auto services=f.services();
 services.show=[&](const auto &,const auto &handle,const auto &,const auto &,const auto &){
  const auto gen=f.flow->presentationGeneration();handle(SelectMemberAction{0});handle(InspectInventoryAction{});
  check(!f.flow->inventoryOpen() && f.flow->presentationGeneration()==gen,"sheet disturbed automatic Event");return true;
 };Quiet quiet;check(Application().playGameplay(services,start,{},false)==0,"automatic guard");
}
void sdl(){
 Fixture f;seed(f);f.ordinary=true;f.initial.roster.at(0).name="Source";auto services=f.services();unsigned index=0,loops=0;bool queued=false;
 const std::vector<SDL_Keycode> keys={SDLK_F1,SDLK_i,SDLK_1,SDLK_F2,SDLK_ESCAPE,SDLK_ESCAPE};
 services.show=[&](const auto &first,const auto &handle,const auto &escape,const auto &idle,const auto &status){
  auto native=handle;
  std::function<void()> pendingKey;
  native.beginCycle=[&](auto cycle){handle.beginCycle(cycle);if(pendingKey){auto send=std::move(pendingKey);pendingKey={};send();}};
  static_cast<std::function<std::optional<IndexedFrame>(const PlayerAction &)> &>(native)=[&](const PlayerAction &a){++index;queued=false;return handle(a);};
  if(handle.withPresentedInput)native.withPresentedInput=[&](const auto &a,const auto &input,const auto &frame){++index;queued=false;return handle.withPresentedInput(a,input,frame);};
  return SdlWindow().showInteractive(first,"Original inventory SDL",native,escape,[&]()->std::optional<IndexedFrame>{
   check(++loops<200,("SDL item test stalled at key "+std::to_string(index)).c_str());
   if(!queued){queued=true;pendingKey=[&]{SDL_Event e{};if(index==keys.size()){e.type=SDL_QUIT;SDL_PushEvent(&e);queued=true;}
   else {e.type=SDL_KEYDOWN;e.key.keysym.sym=keys[index];SDL_PushEvent(&e);e.key.repeat=1;SDL_PushEvent(&e);e.type=SDL_KEYUP;e.key.repeat=0;SDL_PushEvent(&e);queued=true;}};}
   return idle();
  },status);
 };Quiet quiet;check(Application().playGameplay(services,start,{},false)==0 && index==keys.size(),"SDL sheet/item strict lifecycle");
}
int child(const fs::path &path,bool resume) {
 regional_save_test::Fixture f;
 for(auto *items:{&f.saved.characters[0].weapons,&f.saved.characters[0].armor,&f.saved.characters[0].accessories,&f.saved.characters[0].miscellaneous})
  *items={{{10,37,1,0},{10,37,1,0},{},{},{},{},{},{},{}}};
 for(auto *items:{&f.saved.characters[18].weapons,&f.saved.characters[18].armor,&f.saved.characters[18].accessories,&f.saved.characters[18].miscellaneous}) *items={};
 auto expected=f.saved;
 for(auto owner:{0,18})for(auto *items:{&expected.characters[owner].weapons,&expected.characters[owner].armor,&expected.characters[owner].accessories,&expected.characters[owner].miscellaneous})
  *items={{{10,37,1,0},{},{},{},{},{},{},{},{}}};
 if(!resume)XeenSaveFile::write(path,f.saved);
 auto services=f.services();SdlWindow::FrameUpdateHandler nested;SdlWindow::IdleFrameHandler nestedIdle;bool preflightReentered=false;
 auto compose=services.composeEncounter;
 services.composeEncounter=[&](auto &w,const auto &p,const auto &c,auto phase,auto actor){
  if(nested&&&w!=f.world){const auto count=f.compositions;nested(InspectInventoryAction{});nested(SaveGameAction{});nestedIdle();check(!f.flow->inventoryOpen()&&f.compositions==count,"save preflight allowed reentrancy");preflightReentered=true;}
  return compose(w,p,c,phase,actor);
 };
 services.show=[&](const auto &,const auto &raw,const auto &,const auto &idle,const auto &status){
  f.present(raw);nested=raw;nestedIdle=idle;
  const auto original=XeenSaveFormat::encode(XeenSaveFile::read(path));
  const auto handle=[&](const PlayerAction &a){f.send(raw,a);};
  check(!f.flow->inventoryOpen(),"startup inventory persisted");handle(InspectInventoryAction{});
  for(unsigned category=0;category<4;++category){
   if(!resume){handle(SelectInventorySlotAction{0});handle(SelectMemberAction{1});handle(SaveGameAction{});check(XeenSaveFormat::encode(XeenSaveFile::read(path))==original,"open F9 wrote");f.flow->refresh(true);f.present(raw);}
   else for(unsigned owner=0;owner<2;++owner){handle(SelectMemberAction{owner});handle(SelectInventorySlotAction{0});check(f.flow->inventorySelection().record.id==37,"restored UI did not inspect owner");handle(SelectInventorySlotAction{0});}
   if(category!=3){handle(SelectMemberAction{0});handle(DialogKeyAction{unsigned("wacm"[category+1])});}
  }
  handle(CancelInteractionAction{});sameSnapshot(expected,f.capture());
  if(!resume){check(XeenSaveFormat::encode(XeenSaveFile::read(path))==original,"deferred save");handle(SaveGameAction{});check(status().find("Saved")!=std::string::npos&&preflightReentered,"production F9/preflight guard failed");}
  sameSnapshot(expected,XeenSaveFile::read(path));return true;
 };
 Quiet quiet;const int result=Application().playGameplay(services,{},path,true);if(result)std::cerr<<quiet.out.str();return result;
}
void restart(const fs::path &exe) {
	const auto dir=fs::current_path()/("inventory-restart-"+std::to_string(GetCurrentProcessId())+"-"+std::to_string(GetTickCount64()));fs::create_directory(dir);
	const auto path=dir/"transfer.mmsave";
	const auto p=child_test::launch(exe,{L"producer",path.wstring()},dir/"producer.log");check(p.exit==0,"producer failed");
	std::ifstream input(path,std::ios::binary);const std::string bytes{std::istreambuf_iterator<char>(input),{}};input.close();
	const auto c=child_test::launch(exe,{L"consumer",path.wstring()},dir/"consumer.log");check(c.exit==0&&p.pid!=c.pid,"fresh consumer failed");
	std::ifstream after(path,std::ios::binary);check(bytes==std::string(std::istreambuf_iterator<char>(after),{}),"consumer rewrote producer disk");
}
}
int main(int argc,char **argv){try{
 if(argc==3)return child(fs::absolute(argv[2]),std::string(argv[1])=="consumer");
 if(argc==2){sdl();return 0;}
 gameplay();invalidation();failure();automaticGuard();restart(fs::absolute(argv[0]));return 0;
}catch(const std::exception &e){std::cerr<<e.what()<<'\n';return 1;}}
